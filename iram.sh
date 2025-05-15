#!/bin/bash
readelf --headers $1 | awk -f iram.awk