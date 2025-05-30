FROM debian:10.13

ENV UPMEM_NO_OS_WARNING=1

RUN apt-get update && \
    apt-get install -y \
        wget \
        build-essential \
        libedit-dev \
        cmake \
        python3-dev \
        libpython3.7-dev \
        libelf-dev \
        libnuma-dev \
        gdb \
        default-jdk \
        libudev-dev \
        xxd

RUN wget http://sdk-releases.upmem.com/2025.1.0/debian_10/upmem_2025.1.0_amd64.deb && \
    apt-get update && \
    apt-get install -y ./upmem_2025.1.0_amd64.deb

COPY ./tools/dpurun.c /src/dpurun.c

RUN clang `pkg-config --cflags dpu` -o /usr/bin/dpurun /src/dpurun.c `pkg-config --libs dpu`

CMD bash
