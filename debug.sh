#!/bin/bash

docker run --mount type=bind,src=./,dst=/project,ro --workdir /project -it fabianvanrissenbeck/upmem:ubuntu dpu-lldb "$@"
