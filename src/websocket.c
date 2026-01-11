/*
 * websocket.c - websocket protocol for durismud
 *
 * implements rfc 6455 websocket for browser clients. handles http upgrade
 * handshake, frame parsing, and message routing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "websocket.h"
#include "ws_handlers.h"
#include "json_utils.h"
#include "structs.h"
#include "prototypes.h"
#include "comm.h"
#include "utils.h"
#include "gmcp.h"
#include "db.h"

extern struct descriptor_data *descriptor_list;

static int ws_listen_fd = -1;

/* skip header name and leading whitespace */
static const char *skip_header_value(const char *line, size_t header_len) {
    const char *value = line + header_len;
    while (*value == ' ' || *value == '\t') value++;
    return value;
}

/* send for non-blocking sockets, fail fast on wouldblock */
static int websocket_send_all(int fd, const void *buf, size_t len) {
    const unsigned char *ptr = (const unsigned char *)buf;
    size_t remaining = len;

    while (remaining > 0) {
        ssize_t sent = send(fd, ptr, remaining, MSG_NOSIGNAL);
        if (sent > 0) {
            ptr += sent;
            remaining -= sent;
        } else if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else {
            return -1;
        }
    }
    return 0;
}

/* base64 encoding helper */
static char *base64_encode(const unsigned char *input, int length) {
    BIO *b64 = NULL, *bio = NULL;
    BUF_MEM *bufferPtr;
    char *output = NULL;

    b64 = BIO_new(BIO_f_base64());
    if (!b64) return NULL;

    bio = BIO_new(BIO_s_mem());
    if (!bio) {
        BIO_free(b64);
        return NULL;
    }

    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    output = (char *)malloc(bufferPtr->length + 1);
    if (output) {
        memcpy(output, bufferPtr->data, bufferPtr->length);
        output[bufferPtr->length] = '\0';
    }

    BIO_free_all(bio);
    return output;
}

/* generate websocket accept key from client key (rfc 6455 section 4.2.2) */
void websocket_generate_accept_key(const char *client_key, char *accept_key) {
    char concat[WS_CONCAT_BUFFER_SIZE];
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];

    accept_key[0] = '\0';

    snprintf(concat, sizeof(concat), "%s%s", client_key, WS_MAGIC_STRING);

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return;

    if (EVP_DigestInit_ex(ctx, EVP_sha1(), NULL) != 1 ||
        EVP_DigestUpdate(ctx, concat, strlen(concat)) != 1 ||
        EVP_DigestFinal_ex(ctx, sha1_hash, NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        return;
    }
    EVP_MD_CTX_free(ctx);

    char *encoded = base64_encode(sha1_hash, SHA_DIGEST_LENGTH);
    if (encoded) {
        snprintf(accept_key, WS_ACCEPT_KEY_SIZE, "%s", encoded);
        free(encoded);
    }
}

/* initialize websocket subsystem */
int websocket_init(int port) {
    struct sockaddr_in sa;
    int opt = 1;

    ws_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ws_listen_fd < 0) {
        perror("websocket_init: socket");
        return -1;
    }

    /* allow socket reuse */
    if (setsockopt(ws_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("websocket_init: setsockopt SO_REUSEADDR");
        close(ws_listen_fd);
        return -1;
    }

    /* set up address */
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;

    /* bind */
    if (bind(ws_listen_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("websocket_init: bind");
        close(ws_listen_fd);
        return -1;
    }

    /* listen */
    if (listen(ws_listen_fd, WS_LISTEN_BACKLOG) < 0) {
        perror("websocket_init: listen");
        close(ws_listen_fd);
        return -1;
    }

    /* non-blocking */
    if (fcntl(ws_listen_fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("websocket_init: fcntl");
        close(ws_listen_fd);
        return -1;
    }

    statuslog(56, "WebSocket server listening on port %d", port);
    return ws_listen_fd;
}

/* shutdown websocket subsystem */
void websocket_shutdown(void) {
    if (ws_listen_fd >= 0) {
        close(ws_listen_fd);
        ws_listen_fd = -1;
    }
}

/* accept new websocket connection */
int websocket_accept(int listen_fd, struct descriptor_data *d) {
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    int new_fd;
    int opt = 1;

    new_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
    if (new_fd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("websocket_accept: accept");
        }
        return -1;
    }

    /* tcp_nodelay for low latency */
    setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    /* non-blocking */
    if (fcntl(new_fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("websocket_accept: fcntl");
        close(new_fd);
        return -1;
    }

    /* initialize descriptor for websocket */
    d->descriptor = new_fd;
    d->websocket = 1;
    d->gmcp_enabled = 1;  /* websocket clients always have gmcp */
    d->ws_state = WS_STATE_CONNECTING;
    d->ws_handshake_done = 0;
    d->ws_last_ping = 0;
    d->ws_pong_received = 0;

    /* get peer address */
    strncpy(d->host, inet_ntoa(peer.sin_addr), sizeof(d->host) - 1);

    statuslog(56, "WebSocket connection from %s", d->host);

    return 0;
}

/* parse http upgrade request, returns 1 if valid websocket upgrade */
int websocket_parse_handshake(struct descriptor_data *d, const char *buf, size_t len) {
    char *line, *saveptr;
    char *request = NULL;
    char ws_key[WS_KEY_BUFFER_SIZE] = {0};
    int is_upgrade = 0;
    int is_websocket = 0;
    int version_ok = 0;

    request = (char *)malloc(len + 1);
    if (!request) return -1;
    memcpy(request, buf, len);
    request[len] = '\0';

    line = strtok_r(request, "\r\n", &saveptr);
    while (line) {
        if (strncmp(line, "GET ", 4) == 0) {
            /* valid http get */
        }
        else if (strncasecmp(line, "Upgrade:", 8) == 0) {
            const char *value = skip_header_value(line, 8);
            if (strcasecmp(value, "websocket") == 0) {
                is_websocket = 1;
            }
        }
        else if (strncasecmp(line, "Connection:", 11) == 0) {
            const char *value = skip_header_value(line, 11);
            if (strcasestr(value, "Upgrade") != NULL) {
                is_upgrade = 1;
            }
        }
        else if (strncasecmp(line, "Sec-WebSocket-Key:", 18) == 0) {
            const char *value = skip_header_value(line, 18);
            strncpy(ws_key, value, sizeof(ws_key) - 1);
        }
        else if (strncasecmp(line, "Sec-WebSocket-Version:", 22) == 0) {
            const char *value = skip_header_value(line, 22);
            if (atoi(value) == WS_PROTOCOL_VERSION) {
                version_ok = 1;
            }
        }
        else if (strncasecmp(line, "User-Agent:", 11) == 0) {
            const char *value = skip_header_value(line, 11);
            strncpy(d->client_name, value, sizeof(d->client_name) - 1);
            d->client_name[sizeof(d->client_name) - 1] = '\0';
            if (strstr(value, "Firefox")) {
                snprintf(d->client_name, sizeof(d->client_name), "Firefox");
            } else if (strstr(value, "Chrome")) {
                snprintf(d->client_name, sizeof(d->client_name), "Chrome");
            } else if (strstr(value, "Safari")) {
                snprintf(d->client_name, sizeof(d->client_name), "Safari");
            } else if (strstr(value, "Edge")) {
                snprintf(d->client_name, sizeof(d->client_name), "Edge");
            }
        }
        /* x-forwarded-for - only trust from local/private proxies */
        else if (strncasecmp(line, "X-Forwarded-For:", 16) == 0) {
            if (strcmp(d->host, "127.0.0.1") == 0 || strcmp(d->host, "::1") == 0 ||
                strncmp(d->host, "10.", 3) == 0) {
                const char *value = skip_header_value(line, 16);
                char client_ip[INET6_ADDRSTRLEN];
                int i = 0;
                while (value[i] && value[i] != ',' && value[i] != ' ' &&
                       i < (int)(sizeof(client_ip) - 1)) {
                    client_ip[i] = value[i];
                    i++;
                }
                client_ip[i] = '\0';

                /* validate with inet_pton */
                struct in_addr ipv4;
                struct in6_addr ipv6;
                if (inet_pton(AF_INET, client_ip, &ipv4) == 1 ||
                    inet_pton(AF_INET6, client_ip, &ipv6) == 1) {
                    strncpy(d->host, client_ip, sizeof(d->host) - 1);
                    d->host[sizeof(d->host) - 1] = '\0';
                    /* hostname lookup */
                    char cmd[256];
                    snprintf(cmd, sizeof(cmd),
                             "host %s | sed -e 's/.*pointer\\ \\(.*\\)\\./\\1/g;t;d' > lib/etc/hosts/%d &",
                             d->host, d->descriptor);
                    system(cmd);
                }
            }
        }

        line = strtok_r(NULL, "\r\n", &saveptr);
    }

    free(request);

    if (!is_upgrade || !is_websocket || !version_ok || ws_key[0] == '\0') {
        return 0;
    }

    return websocket_complete_handshake(d, ws_key);
}

/* send http upgrade response */
int websocket_complete_handshake(struct descriptor_data *d, const char *key) {
    char accept_key[WS_ACCEPT_KEY_SIZE];
    char response[WS_RESPONSE_BUFFER_SIZE];
    int len;
    struct descriptor_data *k, *next_k;

    websocket_generate_accept_key(key, accept_key);
    if (accept_key[0] == '\0') return -1;

    len = snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept_key);

    if (websocket_send_all(d->descriptor, response, len) < 0) {
        return -1;
    }

    d->ws_state = WS_STATE_OPEN;
    d->ws_handshake_done = 1;
    d->connected = 60;  /* ready for account login */

    /*
     * duplicate connection check: kick any other unauthenticated websocket
     * connections from the same ip (handles hmr, page refresh, etc.)
     */
    for (k = descriptor_list; k; k = next_k) {
        next_k = k->next;

        /* skip self */
        if (k == d) continue;

        /* only kick unauthenticated websocket connections from same ip */
        if (k->websocket && k->ws_handshake_done &&
            !k->account &&  /* not logged in yet */
            k->connected != CON_PLAYING &&
            strcmp(k->host, d->host) == 0) {

            statuslog(56, "WebSocket: Kicking stale connection from %s (new connection established)",
                      k->host);

            ws_send_system(k, "kicked", "New connection established from your browser.");
            websocket_close(k, WS_CLOSE_NORMAL, "New connection");
            close_socket(k);
        }
    }

    statuslog(56, "WebSocket handshake complete for %s", d->host);

    /* send welcome message - client is ready for login */
    ws_send_system(d, "connected", "Welcome to NewDuris MUD!");

    return 1;
}

/* build and send a websocket frame */
static int websocket_send_frame(struct descriptor_data *d, int opcode,
                                const void *data, size_t len) {
    unsigned char *frame;
    size_t frame_len;
    size_t offset = 0;
    int result;

    if (!d || d->descriptor < 0) return -1;

    if (len <= WS_LEN_7BIT_MAX) {
        frame_len = 2 + len;
    } else if (len <= WS_LEN_16BIT_MAX) {
        frame_len = 4 + len;
    } else {
        frame_len = 10 + len;
    }

    frame = (unsigned char *)malloc(frame_len);
    if (!frame) return -1;

    frame[offset++] = 0x80 | (opcode & 0x0F);

    if (len <= WS_LEN_7BIT_MAX) {
        frame[offset++] = (unsigned char)len;
    } else if (len <= WS_LEN_16BIT_MAX) {
        frame[offset++] = 126;
        frame[offset++] = (len >> 8) & 0xFF;
        frame[offset++] = len & 0xFF;
    } else {
        frame[offset++] = 127;
        frame[offset++] = 0;
        frame[offset++] = 0;
        frame[offset++] = 0;
        frame[offset++] = 0;
        frame[offset++] = (len >> 24) & 0xFF;
        frame[offset++] = (len >> 16) & 0xFF;
        frame[offset++] = (len >> 8) & 0xFF;
        frame[offset++] = len & 0xFF;
    }

    if (len > 0 && data) {
        memcpy(frame + offset, data, len);
    }

    if (d->descriptor < 0 || !is_desc_valid(d)) {
        free(frame);
        return -1;
    }

    result = websocket_send_all(d->descriptor, frame, frame_len);
    free(frame);

    return result;
}

/* send text frame */
int websocket_send_text(struct descriptor_data *d, const char *text) {
    if (!text) return -1;
    return websocket_send_frame(d, WS_OPCODE_TEXT, text, strlen(text));
}

/* send binary frame */
int websocket_send_binary(struct descriptor_data *d, const void *data, size_t len) {
    return websocket_send_frame(d, WS_OPCODE_BINARY, data, len);
}

/* send json message with type wrapper */
int websocket_send_json(struct descriptor_data *d, const char *type,
                        const char *package, const char *json) {
    char *message;
    int result;

    if (package) {
        /* gmcp-style message */
        message = json_build_gmcp_message(package, json);
    } else {
        /* simple message - json is already complete */
        message = strdup(json);
    }

    if (!message) return -1;

    result = websocket_send_text(d, message);
    free(message);
    return result;
}

/* send close frame */
int websocket_send_close(struct descriptor_data *d, int code, const char *reason) {
    unsigned char payload[WS_CLOSE_PAYLOAD_SIZE];
    size_t len = 0;

    if (code > 0) {
        payload[0] = (code >> 8) & 0xFF;
        payload[1] = code & 0xFF;
        len = 2;

        if (reason) {
            size_t reason_len = strlen(reason);
            if (reason_len > sizeof(payload) - 2) {
                reason_len = sizeof(payload) - 2;
            }
            memcpy(payload + 2, reason, reason_len);
            len += reason_len;
        }
    }

    d->ws_state = WS_STATE_CLOSING;
    return websocket_send_frame(d, WS_OPCODE_CLOSE, payload, len);
}

/* send ping frame */
int websocket_send_ping(struct descriptor_data *d) {
    return websocket_send_frame(d, WS_OPCODE_PING, NULL, 0);
}

/* send pong frame */
int websocket_send_pong(struct descriptor_data *d, const char *data, size_t len) {
    return websocket_send_frame(d, WS_OPCODE_PONG, data, len);
}

/* parse websocket frame, returns bytes consumed (-1 on error, 0 if need more data) */
int websocket_parse_frame(struct descriptor_data *d, const char *buf, size_t len,
                          char **payload, size_t *payload_len, int *opcode, int *fin_out) {
    size_t offset = 0;
    size_t frame_len;
    size_t data_len;
    int fin, op, mask_bit;
    unsigned char mask_key[4];
    size_t i;

    *payload = NULL;
    *payload_len = 0;
    *opcode = -1;
    if (fin_out) *fin_out = 1;  /* default to fin=1 */

    /* need at least 2 bytes for header */
    if (len < 2) return 0;

    /* parse first byte */
    fin = (buf[0] >> 7) & 0x01;
    op = buf[0] & 0x0F;
    offset++;

    /* parse second byte */
    mask_bit = (buf[1] >> 7) & 0x01;
    data_len = buf[1] & 0x7F;
    offset++;

    /* extended length */
    if (data_len == 126) {
        if (len < 4) return 0;
        data_len = ((unsigned char)buf[2] << 8) | (unsigned char)buf[3];
        offset += 2;
    } else if (data_len == 127) {
        if (len < 10) return 0;
        /* parse full 64-bit length */
        uint64_t full_len =
            ((uint64_t)(unsigned char)buf[2] << 56) |
            ((uint64_t)(unsigned char)buf[3] << 48) |
            ((uint64_t)(unsigned char)buf[4] << 40) |
            ((uint64_t)(unsigned char)buf[5] << 32) |
            ((uint64_t)(unsigned char)buf[6] << 24) |
            ((uint64_t)(unsigned char)buf[7] << 16) |
            ((uint64_t)(unsigned char)buf[8] << 8) |
            (uint64_t)(unsigned char)buf[9];
        if (full_len > WS_MAX_FRAME_SIZE) {
            return -1;
        }
        data_len = (size_t)full_len;
        offset += 8;
    } else if (data_len > WS_MAX_FRAME_SIZE) {
        return -1;
    }

    /* mask key (client must mask) */
    if (mask_bit) {
        if (len < offset + 4) return 0;
        memcpy(mask_key, buf + offset, 4);
        offset += 4;
    }

    /* check if we have full payload */
    frame_len = offset + data_len;
    if (len < frame_len) return 0;

    /* allocate and unmask payload */
    if (data_len > 0) {
        *payload = (char *)malloc(data_len + 1);
        if (!*payload) return -1;

        memcpy(*payload, buf + offset, data_len);

        if (mask_bit) {
            for (i = 0; i < data_len; i++) {
                (*payload)[i] ^= mask_key[i % 4];
            }
        }

        (*payload)[data_len] = '\0';
        *payload_len = data_len;
    }

    *opcode = op;
    if (fin_out) *fin_out = fin;

    /* handle control frames */
    if (op == WS_OPCODE_CLOSE) {
        websocket_send_close(d, WS_CLOSE_NORMAL, NULL);
        d->ws_state = WS_STATE_CLOSED;
    } else if (op == WS_OPCODE_PING) {
        websocket_send_pong(d, *payload, *payload_len);
        free(*payload);
        *payload = NULL;
        *payload_len = 0;
    } else if (op == WS_OPCODE_PONG) {
        /* mark pong received for dead connection detection */
        d->ws_pong_received = 1;
        free(*payload);
        *payload = NULL;
        *payload_len = 0;
    }

    return (int)frame_len;
}

/* close websocket connection properly */
void websocket_close(struct descriptor_data *d, int code, const char *reason) {
    if (!d) return;

    if (d->ws_state == WS_STATE_OPEN) {
        websocket_send_close(d, code, reason);
    }

    d->ws_state = WS_STATE_CLOSED;
}

/* free websocket-specific data */
void websocket_free(struct descriptor_data *d) {
    if (!d) return;

    if (d->ws_handshake_buffer) {
        free(d->ws_handshake_buffer);
        d->ws_handshake_buffer = NULL;
        d->ws_handshake_len = 0;
    }

    if (d->ws_fragment_buffer) {
        free(d->ws_fragment_buffer);
        d->ws_fragment_buffer = NULL;
        d->ws_fragment_len = 0;
    }

    if (d->ws_message_buffer) {
        free(d->ws_message_buffer);
        d->ws_message_buffer = NULL;
        d->ws_message_len = 0;
        d->ws_message_opcode = 0;
    }

    d->websocket = 0;
    d->ws_state = WS_STATE_CLOSED;
}

/* helper: handle a complete websocket message (after fragmentation reassembly) */
static void websocket_handle_message(struct descriptor_data *d, int opcode,
                                      char *payload, size_t payload_len) {
    if (opcode == WS_OPCODE_TEXT && payload) {
        /* parse json and extract command/data */
        cJSON *json = cJSON_Parse(payload);
        if (json) {
            const char *type = NULL;
            const char *cmd = NULL;
            cJSON *type_item = cJSON_GetObjectItem(json, "type");
            cJSON *cmd_item = cJSON_GetObjectItem(json, "cmd");
            cJSON *data_item = cJSON_GetObjectItem(json, "data");

            if (type_item && cJSON_IsString(type_item))
                type = type_item->valuestring;
            if (cmd_item && cJSON_IsString(cmd_item))
                cmd = cmd_item->valuestring;

            if (type && strcmp(type, "cmd") == 0 && cmd) {
                /* use websocket command handler */
                ws_handle_command(d, cmd, data_item);
            } else if (type && strcmp(type, "gmcp") == 0) {
                /* handle gmcp package */
                cJSON *pkg_item = cJSON_GetObjectItem(json, "package");
                if (pkg_item && cJSON_IsString(pkg_item)) {
                    char *data_str = data_item ? cJSON_PrintUnformatted(data_item) : NULL;
                    size_t gmcp_len = strlen(pkg_item->valuestring) + (data_str ? strlen(data_str) + 2 : 1);
                    char *gmcp_msg = (char *)malloc(gmcp_len);
                    if (gmcp_msg) {
                        if (data_str) {
                            snprintf(gmcp_msg, gmcp_len, "%s %s", pkg_item->valuestring, data_str);
                        } else {
                            strcpy(gmcp_msg, pkg_item->valuestring);
                        }
                        gmcp_handle_input(d, gmcp_msg, strlen(gmcp_msg));
                        free(gmcp_msg);
                    }
                    if (data_str) free(data_str);
                }
            } else if (d->connected == CON_PLAYING) {
                /* in-game: pass raw text as command */
                if (data_item && cJSON_IsString(data_item)) {
                    write_to_q(data_item->valuestring, &d->input, 0);
                } else if (cmd) {
                    write_to_q(cmd, &d->input, 0);
                }
            }
            cJSON_Delete(json);
        } else {
            /* not valid json - treat as raw text command if in game */
            if (d->connected == CON_PLAYING) {
                write_to_q(payload, &d->input, 0);
            }
        }
    }
    /* binary frames ignored for now */
}

/* process incoming websocket data, called from game loop */
int websocket_process_input(struct descriptor_data *d) {
    char buf[WS_INPUT_BUFFER_SIZE];
    ssize_t bytes_read;
    int consumed;
    char *payload;
    size_t payload_len;
    int opcode, fin;
    size_t offset;
    char *new_buf;

    if (!d || d->descriptor < 0) return -1;

    bytes_read = read(d->descriptor, buf, sizeof(buf));

    if (bytes_read < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0;
        }
        return -1;
    }

    if (bytes_read == 0) {
        return -1;
    }

    /* buffer http handshake until complete */
    if (!d->ws_handshake_done) {
        new_buf = (char *)realloc(d->ws_handshake_buffer,
                                   d->ws_handshake_len + bytes_read + 1);
        if (!new_buf) return -1;
        d->ws_handshake_buffer = new_buf;
        memcpy(d->ws_handshake_buffer + d->ws_handshake_len, buf, bytes_read);
        d->ws_handshake_len += bytes_read;
        d->ws_handshake_buffer[d->ws_handshake_len] = '\0';

        /* check for complete http request */
        if (d->ws_handshake_len < 4 ||
            strstr(d->ws_handshake_buffer, "\r\n\r\n") == NULL) {
            if (d->ws_handshake_len > WS_MAX_HANDSHAKE_SIZE) {
                return -1;
            }
            return 0;
        }

        int result = websocket_parse_handshake(d, d->ws_handshake_buffer,
                                                d->ws_handshake_len);
        free(d->ws_handshake_buffer);
        d->ws_handshake_buffer = NULL;
        d->ws_handshake_len = 0;

        if (result <= 0) {
            return -1;
        }
        return 0;
    }

    /*
     * append to tcp fragment buffer for proper frame reassembly.
     * this handles frames split across tcp packets.
     */
    new_buf = (char *)realloc(d->ws_fragment_buffer, d->ws_fragment_len + bytes_read);
    if (!new_buf) {
        return -1;  /* oom */
    }
    d->ws_fragment_buffer = new_buf;
    memcpy(d->ws_fragment_buffer + d->ws_fragment_len, buf, bytes_read);
    d->ws_fragment_len += bytes_read;

    /*
     * process all complete frames in buffer.
     * handles multiple frames arriving in a single read().
     */
    offset = 0;
    while (offset < d->ws_fragment_len) {
        consumed = websocket_parse_frame(d, d->ws_fragment_buffer + offset,
                                         d->ws_fragment_len - offset,
                                         &payload, &payload_len, &opcode, &fin);

        if (consumed < 0) {
            /* protocol error - close connection */
            return -1;
        }

        if (consumed == 0) {
            /* incomplete frame, wait for more data */
            break;
        }

        offset += consumed;

        /* skip control frames (already handled in parse_frame) */
        if (opcode == WS_OPCODE_CLOSE || opcode == WS_OPCODE_PING ||
            opcode == WS_OPCODE_PONG) {
            if (opcode == WS_OPCODE_CLOSE) {
                /* clean up and signal close */
                if (offset < d->ws_fragment_len) {
                    memmove(d->ws_fragment_buffer, d->ws_fragment_buffer + offset,
                            d->ws_fragment_len - offset);
                    d->ws_fragment_len -= offset;
                } else {
                    free(d->ws_fragment_buffer);
                    d->ws_fragment_buffer = NULL;
                    d->ws_fragment_len = 0;
                }
                return -1;
            }
            continue;  /* ping/pong already handled */
        }

        /*
         * handle websocket message fragmentation (rfc 6455).
         * fin=0 means more fragments coming.
         * continuation opcode (0x00) continues previous message.
         */
        if (opcode == WS_OPCODE_CONTINUATION) {
            /* continuation frame - append to message buffer */
            if (!d->ws_message_buffer) {
                /* got continuation without initial frame - protocol error */
                if (payload) free(payload);
                return -1;
            }
            if (payload && payload_len > 0) {
                new_buf = (char *)realloc(d->ws_message_buffer,
                                          d->ws_message_len + payload_len + 1);
                if (!new_buf) {
                    free(payload);
                    return -1;
                }
                d->ws_message_buffer = new_buf;
                memcpy(d->ws_message_buffer + d->ws_message_len, payload, payload_len);
                d->ws_message_len += payload_len;
                d->ws_message_buffer[d->ws_message_len] = '\0';
                free(payload);
                payload = NULL;
            }

            if (fin) {
                /* final fragment - deliver complete message */
                websocket_handle_message(d, d->ws_message_opcode,
                                         d->ws_message_buffer, d->ws_message_len);
                free(d->ws_message_buffer);
                d->ws_message_buffer = NULL;
                d->ws_message_len = 0;
                d->ws_message_opcode = 0;
            }
        } else if (!fin) {
            /* first fragment of a multi-frame message */
            if (d->ws_message_buffer) {
                /* already have a fragmented message in progress - protocol error */
                if (payload) free(payload);
                return -1;
            }
            d->ws_message_opcode = opcode;
            d->ws_message_buffer = payload;  /* take ownership */
            d->ws_message_len = payload_len;
            payload = NULL;  /* don't free, we own it now */
        } else {
            /* complete single-frame message */
            websocket_handle_message(d, opcode, payload, payload_len);
            if (payload) {
                free(payload);
                payload = NULL;
            }
        }
    }

    /* remove consumed data from buffer */
    if (offset > 0) {
        if (offset >= d->ws_fragment_len) {
            /* consumed everything */
            free(d->ws_fragment_buffer);
            d->ws_fragment_buffer = NULL;
            d->ws_fragment_len = 0;
        } else {
            /* keep remainder */
            memmove(d->ws_fragment_buffer, d->ws_fragment_buffer + offset,
                    d->ws_fragment_len - offset);
            d->ws_fragment_len -= offset;
        }
    }

    return 0;
}

/* check if descriptor has pending websocket data */
int websocket_has_pending(struct descriptor_data *d) {
    if (!d) return 0;
    return (d->ws_fragment_buffer != NULL && d->ws_fragment_len > 0);
}
