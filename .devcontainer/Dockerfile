# https://mcr.microsoft.com/en-us/product/devcontainers/base/about
FROM mcr.microsoft.com/devcontainers/base:bookworm

RUN apt-get update && \
    apt-get install -y \
      sudo \
      locales \
      procps \
      net-tools \
      iputils-ping \
      dnsutils \
      bash-completion \
      less \
      curl \
      wget \
      tar \
      xz-utils \
      unzip \
      git \
      vim \
      tmux \
      jq \
      fzf \
      ansible \
      build-essential \
      llvm-19-dev \
      lld-19 \
      clang-19 \
      nasm \
      acpica-tools \
      uuid-dev \
      qemu-system-x86 \
      qemu-utils \
      gdb \
      xauth \
      python3-distutils-extra \
      ca-certificates \
      ovmf \
      dosfstools \
      parted

# ロケール設定
RUN echo 'ja_JP.UTF-8 UTF-8' >> /etc/locale.gen && \
    locale-gen && \
    update-locale LANG=ja_JP.UTF-8
RUN ln -sf /usr/share/zoneinfo/Asia/Tokyo /etc/localtime

COPY .devcontainer/conf/ansible /root/ansible
RUN ansible-playbook -K -i /root/ansible/inventory /root/ansible/provision.yml