FROM ubuntu:24.04 AS build

ARG BUILD_JOBS=2

RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
        build-essential \
        gawk \
        libbsd-dev \
        libcjson-dev \
        libcurl4-gnutls-dev \
        libgnutls28-dev \
        libhiredis-dev \
        libmariadb-dev-compat \
        libssl-dev \
        libxml2-dev \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/duris
COPY . .

RUN make -j"${BUILD_JOBS}" build-server build-area-tools \
    && rm -rf bin/objects


FROM ubuntu:24.04 AS runtime

RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
        bash \
        binutils \
        ca-certificates \
        curl \
        dos2unix \
        findutils \
        gawk \
        gzip \
        libbsd0 \
        libcjson1 \
        libcurl3t64-gnutls \
        libgnutls30t64 \
        libhiredis1.1.0 \
        libmariadb3 \
        libssl3t64 \
        libxml2 \
        mariadb-client \
        openssl \
        python3 \
        xxd \
        zlib1g \
    && rm -rf /var/lib/apt/lists/* \
    && groupadd --gid 10001 duris \
    && useradd --uid 10001 --gid duris --home-dir /opt/duris --no-create-home duris

WORKDIR /opt/duris
COPY --from=build --chown=duris:duris /opt/duris /opt/duris

RUN install -d -o duris -g duris -m 0700 \
        /opt/duris/Players \
        /opt/duris/Players/Tradeskills \
        /var/lib/duris \
        /var/lib/duris/critical-command-journal \
        /var/lib/duris/db-backups \
        /var/lib/duris/player-journal \
        /var/lib/duris/tls \
    && for letter in a b c d e f g h i j k l m n o p q r s t u v w x y z; do \
        install -d -o duris -g duris -m 0700 \
            "/opt/duris/Players/$letter" \
            "/opt/duris/Players/Tradeskills/$letter"; \
    done \
    && install -d -o duris -g duris -m 0755 \
        /opt/duris/logs \
        /opt/duris/logs/log \
        /opt/duris/logs/old-logs \
        /opt/duris/logs/player-log \
    && rm -f /opt/duris/duris.crt /opt/duris/duris.key \
    && ln -s /var/lib/duris/tls/duris.crt /opt/duris/duris.crt \
    && ln -s /var/lib/duris/tls/duris.key /opt/duris/duris.key

USER duris

EXPOSE 4000 4001 4050

HEALTHCHECK --interval=10s --timeout=5s --start-period=90s --retries=30 \
    CMD ["./scripts/healthcheck.sh"]

ENTRYPOINT ["./deploy/docker/entrypoint.sh"]
