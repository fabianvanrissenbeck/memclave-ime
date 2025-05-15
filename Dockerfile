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
        libudev-dev

RUN wget http://sdk-releases.upmem.com/2025.1.0/debian_10/upmem_2025.1.0_amd64.deb && \
    apt-get update && \
    apt-get install -y ./upmem_2025.1.0_amd64.deb

CMD bash
