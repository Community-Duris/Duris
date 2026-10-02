FROM ubuntu:24.04@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54

# Compiler and recovery dependencies have the same maintained owner as local
# setup. This image contains tools only; the caller supplies the exact Git batch.
COPY packaging/duris-build-deps.equivs /tmp/duris-build-deps.equivs
RUN apt-get update \
    && apt-get install --no-install-recommends --yes equivs docker.io iproute2 \
    && cd /tmp && equivs-build duris-build-deps.equivs \
    && apt-get install --yes /tmp/duris-build-deps_1.0_all.deb \
    && rm -rf /var/lib/apt/lists/* /tmp/duris-build-deps*

WORKDIR /suite
ENTRYPOINT []
CMD ["sleep", "infinity"]
