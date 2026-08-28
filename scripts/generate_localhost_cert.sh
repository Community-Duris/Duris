#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
CERT_DIR="$ROOT_DIR/certs"
CERT_FILE="$CERT_DIR/localhost.crt"
KEY_FILE="$CERT_DIR/localhost.key"

if [ -e "$CERT_FILE" ] || [ -e "$KEY_FILE" ]; then
    echo "Localhost certificate already exists; remove both generated files before rotating it." >&2
    exit 1
fi

if ! command -v openssl >/dev/null 2>&1; then
    echo "openssl is required to generate the localhost certificate." >&2
    exit 1
fi

TEMP_DIR=$(mktemp -d)
trap 'rm -rf -- "$TEMP_DIR"' EXIT

mkdir -p "$CERT_DIR"
umask 077
openssl req -x509 -newkey rsa:3072 -sha256 -nodes -days 365 \
    -subj "/CN=localhost" \
    -addext "subjectAltName=DNS:localhost,IP:127.0.0.1,IP:::1" \
    -addext "keyUsage=critical,digitalSignature,keyEncipherment" \
    -addext "extendedKeyUsage=serverAuth" \
    -keyout "$TEMP_DIR/localhost.key" \
    -out "$TEMP_DIR/localhost.crt"

install -m 0600 "$TEMP_DIR/localhost.key" "$KEY_FILE"
install -m 0644 "$TEMP_DIR/localhost.crt" "$CERT_FILE"

echo "Generated ignored localhost TLS certificate: $CERT_FILE"
echo "Generated ignored localhost TLS key (mode 0600): $KEY_FILE"
