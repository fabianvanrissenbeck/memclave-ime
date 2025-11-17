FROM ubuntu:22.04

RUN apt update && \
    apt install -y build-essential git cmake ninja-build python3-dev && \
    git clone --depth=1 https://github.com/upmem/llvm-project.git

RUN sed -i -f - llvm-project/llvm/lib/Target/DPU/DPURegisterInfo.cpp << EOF
/reserved.set(DPU::D22);/i \ \ reserved.set(DPU::D20);\n\ \ reserved.set(DPU::R20);\n\ \ reserved.set(DPU::R21);
/reserved.set(DPU::MAJ_D22);/i \ \ reserved.set(DPU::MAJ_D20);\n\ \ reserved.set(DPU::MAJ_R20);\n\ \ reserved.set(DPU::MAJ_R21);
EOF
# RUN sed -i '/reserved.set(DPU::D22);/i \\ \\ reserved.set(DPU::D20);\n\ \ reserved.set(DPU::R20);\n\ \ reserved.set(DPU::R21); /reserved.set(DPU::MAJ_D22);/i \ \ reserved.set(DPU::MAJ_D20);\n\ \ reserved.set(DPU::MAJ_R20);\n\ \ reserved.set(DPU::MAJ_R21); \' llvm-project/llvm/lib/Target/DPU/DPURegisterInfo.cpp
RUN sed -i '1i #include <cstdint>\n' llvm-project/llvm/include/llvm/Support/Signals.h

RUN cd llvm-project && \
    mkdir -p build && \
    cmake -B build \
          -G Ninja \
          -DLLVM_ENABLE_PROJECTS=clang \
          -DCMAKE_BUILD_TYPE=Debug \
          -DLLVM_TARGETS_TO_BUILD="DPU" \
          -DLLVM_BUILD_TOOLS=NO \
          -DLLVM_DEFAULT_TARGET_TRIPLE=dpu-upmem-dpurte \
          -DDEFAULT_SYSROOT=/usr/share/upmem/include \
          ./llvm && \
    cmake --build build --target=clang -j 4

FROM ubuntu:22.04

ENV UPMEM_NO_OS_WARNING=1

RUN apt-get update && \
    apt-get install -y \
        wget \
        build-essential \
        libedit-dev \
        cmake \
        python3-dev \
        libelf-dev \
        libnuma-dev \
        gdb \
        default-jdk \
        libudev-dev \
	python3-pip \
        xxd

RUN wget http://sdk-releases.upmem.com/2025.1.0/ubuntu_22.04/upmem_2025.1.0_amd64.deb && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y ./upmem_2025.1.0_amd64.deb

RUN pip3 install pycryptodome

COPY ./tools/dpurun.c /src/dpurun.c

RUN clang `pkg-config --cflags dpu` -o /usr/bin/dpurun /src/dpurun.c `pkg-config --libs dpu`

COPY --from=0 /llvm-project/build/bin/clang-12 /usr/bin/clang-12

CMD bash
