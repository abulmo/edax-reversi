# simple sample
FROM gcc:13-bookworm

WORKDIR /opt/edax

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        make \
        clang \
        p7zip-full \
        curl \
    && rm -rf /var/lib/apt/lists/*

COPY src /opt/edax/src
COPY LICENSE /opt/edax/LICENSE

RUN set -eux; \
    mkdir -p /opt/edax/bin; \
    arch="$(uname -m)"; \
    case "$arch" in \
      x86_64|amd64) EDAX_ARCH='x86-64-v3' ;; \
      aarch64|arm64) EDAX_ARCH='armv8.5-a' ;; \
      *) echo "Unsupported architecture: $arch" >&2; exit 1 ;; \
    esac; \
    cd /opt/edax/src; \
    make build ARCH="$EDAX_ARCH" COMP=clang OS=linux; \
    cd /opt/edax; \
    curl -fL -o eval.7z https://github.com/abulmo/edax-reversi/releases/download/v4.4/eval.7z; \
    7z x -y eval.7z; \
    rm -f eval.7z; \
    printf '#!/bin/sh\nset -eu\nexec /opt/edax/bin/lEdax-%s "$@"\n' "$EDAX_ARCH" > /usr/local/bin/edax; \
    chmod +x /usr/local/bin/edax

ENTRYPOINT ["/usr/local/bin/edax"]
