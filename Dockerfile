FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG USER_ID=1000
ARG GROUP_ID=1000

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    build-essential \
    ca-certificates \
    cmake \
    git \
    ninja-build \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --break-system-packages "conan>=2.0,<3.0"

RUN BUILD_GROUP="$(getent group "${GROUP_ID}" | cut -d: -f1 || true)" \
    && if [ -z "${BUILD_GROUP}" ]; then \
        groupadd --gid "${GROUP_ID}" builder; \
        BUILD_GROUP=builder; \
    fi \
    && BUILD_USER="$(getent passwd "${USER_ID}" | cut -d: -f1 || true)" \
    && if [ -z "${BUILD_USER}" ]; then \
        useradd --uid "${USER_ID}" --gid "${GROUP_ID}" --create-home --shell /bin/bash builder; \
        BUILD_USER=builder; \
    fi \
    && HOME_DIR="$(getent passwd "${USER_ID}" | cut -d: -f6 || true)" \
    && mkdir -p "${HOME_DIR:-/home/builder}" \
    && chown -R "${USER_ID}:${GROUP_ID}" "${HOME_DIR:-/home/builder}"

ENV HOME=/home/builder
RUN mkdir -p "${HOME}" && chown -R "${USER_ID}:${GROUP_ID}" "${HOME}"

USER ${USER_ID}:${GROUP_ID}
WORKDIR /workspace

CMD ["/bin/bash"]
