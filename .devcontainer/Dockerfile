FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Instala ferramentas essenciais de build e debugging
RUN apt-get update && apt-get install -y \
    build-essential \
    clang lldb lld llvm \
    valgrind \
    gdb \
    binutils \
    binutils-x86-64-linux-gnu \
    gcc-x86-64-linux-gnu \
    qemu-system-x86 \
    nasm \
    sudo \
    curl \
    unzip \
    git \
    python3 \
    python3-pip \
    xz-utils \
    grub-pc-bin \
    grub-common \
    xorriso \
    mtools \
    && apt-get clean \
    && ln -sf /usr/bin/x86_64-linux-gnu-as /usr/local/bin/as



# Instala XMake v3.0.1 (binário bundle)
RUN curl -LO https://github.com/xmake-io/xmake/releases/download/v3.0.1/xmake-bundle-v3.0.1.linux.x86_64 \
    && chmod +x xmake-bundle-v3.0.1.linux.x86_64 \
    && mv xmake-bundle-v3.0.1.linux.x86_64 /usr/local/bin/xmake

# Verifica se grub-mkrescue está disponível
RUN if ! which grub-mkrescue > /dev/null; then \
    echo "Error: grub-mkrescue not found. Please check your GRUB installation." && exit 1; \
    fi

# Cria usuário padrão para o DevPod com permissões sudo
RUN useradd -m -s /bin/bash devpod \
    && echo "devpod ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

    # Cria script de build
RUN echo '#!/bin/bash\nxmake f --mode=release --toolchain=FKernel_Compiling && xmake' > /usr/local/bin/build-fkernel \
    && chmod +x /usr/local/bin/build-fkernel

# Define o usuário padrão no container
USER devpod
WORKDIR /home/devpod