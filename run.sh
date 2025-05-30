#!/bin/bash
docker run -it \
  --mount type=bind,src="$(pwd)/cmake-build-debug",dst=/cmake-build-debug,ro \
  --mount type=bind,src=$1,dst=/subkernel,ro \
  fabianvanrissenbeck/upmem:latest dpurun /cmake-build-debug/ime /subkernel