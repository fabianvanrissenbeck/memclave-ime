#!/bin/bash

touch $2;
docker run -it \
  --mount type=bind,src=./tools/sk/main.py,dst=/sk.py,ro \
  --mount type=bind,src="$(realpath $1)",dst=/input.elf,ro \
  --mount type=bind,src="$(realpath $2)",dst=/output.sk \
  fabianvanrissenbeck/upmem:ubuntu python3 /sk.py /input.elf /output.sk $3
