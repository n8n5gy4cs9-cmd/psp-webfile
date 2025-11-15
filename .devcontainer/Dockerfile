FROM ubuntu:22.04

# Install build dependencies required for building the PSP toolchain
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    ca-certificates \
    autoconf \
    automake \
    bison \
    flex \
    libncurses5-dev \
    libreadline-dev \
    libusb-1.0-0-dev \
    texinfo \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    libelf-dev \
    libssl-dev \
    python3 \
    python3-pip \
    ca-certificates \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Create a standard installation prefix for the PSP toolchain and SDK
ENV PSPDEV=/opt/pspdev
ENV PSPSDK=${PSPDEV}/psp/sdk
ENV PATH=${PATH}:${PSPDEV}/bin:${PSPSDK}/bin

RUN mkdir -p ${PSPDEV}

# Create workspace mountpoint used by Codespaces (the CLI mounts repository into /workspaces)
RUN mkdir -p /workspaces
WORKDIR /workspaces

# Copy helper setup script (executed by devcontainer postCreateCommand) and make it executable
# COPY uses the build context (repository root). The script lives in .devcontainer/, so reference it explicitly.
COPY .devcontainer/setup.sh /tmp/devcontainer-setup.sh
RUN chmod +x /tmp/devcontainer-setup.sh

# Keep container running with a shell by default
CMD ["/bin/bash"]

