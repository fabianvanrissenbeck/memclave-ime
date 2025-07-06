#!/usr/bin/env bash
# Usage: ./verifier.sh <attestation_bundle.tar> <expected_dir> <aik_pub.pem>
# Example: ./verifier.sh wk_dir/attestation_bundle.tar initial_output initial_output/aik_pub.pem

set -eu

# Check arguments
if [ "$#" -ne 3 ]; then
  echo "Usage: $0 <bundle> <expected_dir> <aik_pub.pem>" >&2
  exit 1
fi

BUNDLE="$1"
EXP_DIR="$2"
AIK_PUB="$3"
WORK=$(mktemp -d)

echo "[Verifier] Extracting bundle to $WORK..."
tar -C "$WORK" -xf "$BUNDLE"

echo "[Verifier] Verifying TPM quote signature..."
tpm2_checkquote -u "$AIK_PUB" -m "$WORK/quote.out" -s "$WORK/sig.bin" \
  -l sha256:16,15 -q "$WORK/nonce.bin" > /dev/null

echo "[Verifier] Reading actual PCRs via PCR read..."
#PCR16_ACTUAL=$(tpm2_pcrread sha256:16 | awk '/16 :/ {print substr($2,3)}')
#PCR15_ACTUAL=$(tpm2_pcrread sha256:15 | awk '/15 :/ {print substr($2,3)}')
PCR16_RAW=$(tpm2_pcrread sha256:16 | awk -F '0x' '/16:/ {print $2}')
PCR15_RAW=$(tpm2_pcrread sha256:15 | awk -F '0x' '/15:/ {print $2}')

# Normalize to lowercase
PCR16_ACTUAL=${PCR16_RAW,,}
PCR15_ACTUAL=${PCR15_RAW,,}

PCR16_EXPECT=$(<"$EXP_DIR/expected_pcr16.hex")
PCR15_EXPECT=$(<"$EXP_DIR/expected_pcr15.hex")

echo "  PCR16 expected=$PCR16_EXPECT, actual=$PCR16_ACTUAL"
echo "  PCR15 expected=$PCR15_EXPECT, actual=$PCR15_ACTUAL"

if [ "$PCR16_ACTUAL" = "$PCR16_EXPECT" ] && [ "$PCR15_ACTUAL" = "$PCR15_EXPECT" ]; then
  echo "✅ Attestation PASS: PCR values match expected."
  exit 0
else
  echo "❌ Attestation FAIL: PCR mismatch."
  exit 1
fi
```

