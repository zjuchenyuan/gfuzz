FROM ubuntu:20.04

MAINTAINER zjuchenyuan
LABEL description="Dockerfile for fuzzer base image"

# support apt mirror selection for faster build
# you can build with --build-arg APT=mirrors.aliyun.com
ARG APT

SHELL ["/bin/bash", "-c"]

ENV DEBIAN_FRONTEND=noninteractive \
    PYTHONIOENCODING=utf8 \
    LC_ALL=C.UTF-8 \
    LANG=C.UTF-8

# install proper tools
RUN ([ "$APT" == "" ] || (sed -i "s/security.ubuntu.com/$APT/g" /etc/apt/sources.list && sed -i "s/archive.ubuntu.com/$APT/g" /etc/apt/sources.list) ) &&\
    apt-get update && \
    apt-get install -y vim nano htop autoconf automake build-essential libtool cmake git sudo software-properties-common gperf libselinux1-dev  bison texinfo flex zlib1g-dev libexpat1-dev libmpg123-dev wget curl python3-pip unzip pkg-config clang llvm-dev &&\
    python3 -m pip install -U pip

RUN apt update && apt install -y ninja-build &&\
    git clone  https://github.com/llvm/llvm-project.git &&\
    cd llvm-project/llvm/tools &&\
    git clone  https://go.googlesource.com/gollvm &&\
    cd gollvm &&\
    git clone  https://go.googlesource.com/gofrontend &&\
    cd libgo &&\
    git clone  https://github.com/libffi/libffi.git &&\
    git clone  https://github.com/ianlancetaylor/libbacktrace.git

ENV LD_LIBRARY_PATH=/build-debug/tools/gollvm/libgo

RUN cd /llvm-project && git checkout 96b0b9a5e &&\
    cd llvm/tools/gollvm && git checkout d30fc0bf &&\
    cd gofrontend && git checkout ae20684902 &&\
    cd ../libgo/libffi && git checkout 8111cd0692 &&\
    cd ../libbacktrace && git checkout f24e9f4

RUN mkdir /build-debug &&\
    cd /build-debug &&\
    cmake -DCMAKE_BUILD_TYPE=Debug -DLLVM_USE_LINKER=gold -G Ninja ../llvm-project/llvm &&\
    ninja gollvm &&\
    ninja install-gollvm -v