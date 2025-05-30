#!/bin/bash
docker run -it --mount type=bind,src="$(pwd)/cmake-build-debug",dst=/cmake-build-debug,ro fabianvanrissenbeck/upmem:latest dpu-lldb /cmake-build-debug/$1