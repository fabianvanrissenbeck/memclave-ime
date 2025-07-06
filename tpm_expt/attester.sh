#!/usr/bin/env bash
# Usage: sudo ./attester.sh <work_dir> <aik_ctx> <hypervisor> <loader>
# Example: sudo ./attester.sh wk_dir initial_output/aik.ctx /usr/bin/kvm user.img

set -eu

# Ensure running as root
if [ "$(id -u)" -ne 0 ]; then
  echo "Error: attester must be run as root. Use sudo." >&2
  exit 1
fi

# Check arguments
if [ "$#" -ne 4 ]; then
  echo "Usage: sudo $0 <work_dir> <aik_ctx> <hypervisor> <loader>" >&2
  exit 1
fi

WDIR="$1"
AIK_CTX="$2"
HYP="$3"
LOADER="$4"
mkdir -p "$WDIR"

echo "[Attester] Generating nonce..."
head -c16 /dev/urandom | xxd -p > "$WDIR/nonce.bin"

echo "[Attester] Extending PCR16 with hypervisor hash..."
HYP_HASH=$(sha256sum "$HYP" | awk '{print $1}')
tpm2_pcrextend 16:sha256=$HYP_HASH

echo "[Attester] Extending PCR15 with loader hash..."
LOADER_HASH=$(sha256sum "$LOADER" | awk '{print $1}')
tpm2_pcrextend 15:sha256=$LOADER_HASH

echo "[Attester] Requesting TPM quote..."
tpm2_quote -c "$AIK_CTX" -l sha256:16,15 \
  -q "$WDIR/nonce.bin" \
  -m "$WDIR/quote.out" \
  -s "$WDIR/sig.bin"

echo "[Attester] Bundling artifacts..."
tar -C "$WDIR" -cf "$WDIR/attestation_bundle.tar" nonce.bin quote.out sig.bin

echo "Attestation bundle created at $WDIR/attestation_bundle.tar"
