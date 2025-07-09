# Components and Setup

The project is currently made up of the following repositories:
1. [ime](https://projects.cispa.saarland/fabian.van-rissenbeck/ime)
    + Includes documentation, all DPU code including the TL and
      all involved subkernels
2. [ci-switch](https://projects.cispa.saarland/fabian.van-rissenbeck/ci-switch)
    + Tool that performs all of the hypervisor tasks such as recovering
      the DPU faults, switching the MUX and so on. Communicates via
      a UNIX socket with QEMU.
3. [ime-client-library](https://projects.cispa.saarland/fabian.van-rissenbeck/ime-client-library#)
    + Library used by applications running on the guest system (without CI access)
      to communicate with the messaging kernel and the modified PIM
      architecture in general.
4. [qemu](https://gitlab.fachschaften.org/ba/qemu)
    + Modified QEMU that is capable of mapping parts of a PIM rank into
      a guest system. Communicates with the CI-switch via UNIX sockets.
5. [common](https://gitlab.fachschaften.org/ba/common)
    + Definitions used by multiple projects, such as the ci-switch,
      qemu, the ime-client-library, linux kernel driver and so on.
6. [ba](https://gitlab.fachschaften.org/fabianvanrissenbeck/ba)
    + Mostly stuff used during the bachelor's thesis. Only the
      linux driver is still used in the state of this repository.

## Compiling and Setting up QEMU

The chios server does not have all build dependencies of QEMU.
I resorted to building qemu in a docker container. Simply build
the container and run it. Then run the commented out configure command
present in the Dockerfile in a separate build directory and compile
by running `make build -j12`. Copy the resulting executable and
all binaries found in the build directory to chios.

The modified qemu can be used just like any other qemu. Use it to
create a KVM enables VM running debian 12. Right now I have the following
script for launching the VM:
```bash
#!/bin/sh
QEMU_PATH=/home/fabian.vanrissenbeck/vm/bin
QEMU_EXEC=./qemu-system-x86_64
SCRIPT_PATH=$(pwd)
LIBRARY_PATH=/home/fabian.vanrissenbeck/vm/bin

cd $QEMU_PATH

LD_LIBRARY_PATH=$LIBRARY_PATH $QEMU_EXEC \
    --nographic \
    --enable-kvm \
    -drive file=../debian.qcow2,if=virtio,media=disk \
    -cpu host \
    -m 16G

cd $SCRIPT_PATH
```

The qemu configuration build only supports a very simple networking
scheme and therefore does not get an external IP. All traffic is NAT'ed
by default. You can ssh out though and create a reverse tunnel:

```bash
#!/bin/bash
ssh -N -R 127.0.0.1:26171:localhost:22 chios
```

By creating a tunnel on your developtment machine via
```bash
ssh -NL 26171:localhost:26171 chios
```
you should be able to ssh into the VM by running
```
ssh -p 26171 user@localhost
```

QEMU depends on the CI-switch to be booted up before starting
by itself. Once QEMU is started, the CI-switch can be stopped
and started again at will.

## Compiling the linux kernel module

Running `make` and then using `insmod` to load the module
should suffice. The driver automatically detects all present
PIM ranks.

## Compiling and Running the CI-switch

The ci-switch is build via cmake. It depends only on UPMEM's DPU
library. The ci-switch right now expectes the following files to
be present:
+ `ime` - The trusted loader
+ `msg.sk` - The messaging subkernel
+ `reset.elf` - A simple DPU kernel that clears all registers. Needs to be compiled manually.

## Compiling the TL and the Subkernels

The TL and Subkernels are all build via cmake in the main repository.
They are all separate targets, building all of them is best. The
subkernels have to be converted into the `.sk` format after
cmake has finished. Remember to pass the correct DPU toolchain
information when configuring the cmake project, otherwise your
not compiling to the DPU architecture:
```
-DCMAKE_TOOLCHAIN_FILE=/${UPMEM_HOME}/share/upmem/cmake/dpu.cmake
```