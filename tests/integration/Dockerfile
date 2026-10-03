FROM mysql:8.0.46@sha256:7dcddc01f13bab2f15cde676d44d01f61fc9f99fe7785e86196dfc07d358ae2b AS mysql_restore
FROM ubuntu:24.04@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54

# Both restore engines run with private datadirs and sockets in the backup row.
# Preserve the pinned MySQL executable's relative dependency lookup and basedir.
COPY --from=mysql_restore /usr/sbin/mysqld /opt/mysql8/sbin/mysqld
COPY --from=mysql_restore /usr/lib64/mysql/private /opt/mysql8/lib64/mysql/private
COPY --from=mysql_restore /usr/lib64/mysql/plugin /opt/mysql8/lib/plugin
COPY --from=mysql_restore /usr/share/mysql-8.0 /opt/mysql8/share/mysql-8.0
COPY --from=mysql_restore /lib64/libaio.so.1 /usr/local/lib/libaio.so.1

# Compiler and recovery dependencies have the same maintained owner as local
# setup. This image contains tools only; the caller supplies the exact Git batch.
COPY packaging/duris-build-deps.equivs /tmp/duris-build-deps.equivs
RUN apt-get update \
    && apt-get install --no-install-recommends --yes equivs docker.io iproute2 curl systemd \
         mariadb-server mariadb-client libmariadb-dev-compat \
    && cd /tmp && equivs-build duris-build-deps.equivs \
    && apt-get install --yes /tmp/duris-build-deps_1.0_all.deb \
    && rm -rf /var/lib/apt/lists/* /tmp/duris-build-deps* \
    && ln -s /opt/mysql8/sbin/mysqld /usr/local/bin/mysqld \
    && ldconfig \
    && mysqld --no-defaults --version

WORKDIR /suite
# The caller's mounted checkout retains its host UID inside this fixture image.
RUN git config --system --add safe.directory /suite
ENTRYPOINT []
CMD ["sleep", "infinity"]
