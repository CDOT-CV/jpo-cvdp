# Image contents: https://github.com/devcontainers/images/tree/main/src/cpp

# Use the current stable Debian C++ development environment.
ARG VARIANT="trixie"
FROM mcr.microsoft.com/devcontainers/cpp:3-${VARIANT}

# [Optional] Install CMake version different from what base image has already installed. 
# Use the bundled CMake by default; optional versions: https://cmake.org/download/
ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Optionally install the cmake for vcpkg
COPY .devcontainer/reinstall-cmake.sh /tmp/
RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake g++ make git pkg-config ca-certificates libcurl4-openssl-dev \
    libsasl2-dev libsasl2-modules libssl-dev liblz4-dev libzstd-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*
COPY scripts/install_librdkafka.sh /tmp/install_librdkafka.sh
RUN sh /tmp/install_librdkafka.sh && ldconfig

ENV REDACTION_PROPERTIES_PATH=/workspaces/jpo-cvdp/config/fieldsToRedact.txt
ENV RPM_DEBUG=true
