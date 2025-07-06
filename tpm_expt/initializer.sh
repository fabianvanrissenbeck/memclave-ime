#!/usr/bin/env bash

# Usage: sudo ./initializer.sh <hypervisor> <loader> <output_dir>
# Example: sudo ./initializer.sh /usr/bin/kvm user.img initial_output

set -eu

# Ensure running as root
if [ "$(id -u)" -ne 0 ]; then
  echo "Error: initializer must be run as root. Use sudo." >&2
  exit 1
fi

# Check arguments
if [ "$#" -ne 3 ]; then
  echo "Usage: sudo $0 <hypervisor> <loader> <output_dir>" >&2
  exit 1
fi

HYP="$1"
LOADER="$2"
OUTDIR="$3"
mkdir -p "$OUTDIR"

# 1) Read initial PCR values
echo "[Initializer] Reading initial PCRs..."
INIT16=$(tpm2_pcrread sha256:16 | awk -F '0x' '/16:/ {print $2}')
INIT15=$(tpm2_pcrread sha256:15 | awk -F '0x' '/15:/ {print $2}')

# 2) Compute file hashes
echo "[Initializer] Hashing input files..."
HYP_HASH=$(sha256sum "$HYP" | awk '{print $1}')
LOADER_HASH=$(sha256sum "$LOADER" | awk '{print $1}')

# 3) Compute expected chained PCR values
# PCR16_expected = SHA256( INIT16_bytes || HYP_HASH_bytes )
printf "%s%s" "$INIT16" "$HYP_HASH" | xxd -r -p | sha256sum | awk '{print $1}' > "$OUTDIR/expected_pcr16.hex"
# PCR15_expected = SHA256( INIT15_bytes || LOADER_HASH_bytes )
printf "%s%s" "$INIT15" "$LOADER_HASH" | xxd -r -p | sha256sum | awk '{print $1}' > "$OUTDIR/expected_pcr15.hex"

# 4) Save raw initial and file hashes for debug
echo "$INIT16" > "$OUTDIR/initial_pcr16.hex"
echo "$INIT15" > "$OUTDIR/initial_pcr15.hex"
echo "$HYP_HASH" > "$OUTDIR/hypervisor_hash.hex"
echo "$LOADER_HASH" > "$OUTDIR/loader_hash.hex"

echo "[Initializer] Done. Artifacts in $OUTDIR"
