#!/bin/bash

docker run --mount type=bind,src=./,dst=/project,ro --workdir /project -it ghcr.io/deinernstjetzt/upmem:latest dpu-lldb "$@"
