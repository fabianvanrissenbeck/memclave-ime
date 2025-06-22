import sys
import subprocess
from Crypto.Cipher import ChaCha20_Poly1305

class SubKernel:
    mac: bytes
    iv: bytes
    size_aad: int
    size: int
    text_size: int
    data_size: int
    text: bytes
    data: bytes

    def __init__(self, text: bytes, data: bytes, enc_text: bool = False, enc_data: bool = True) -> None:
        if enc_text and not enc_data:
            raise ValueError("encrypting only the text section is unsupported")

        if len(text) % 2048 != 0:
            text += bytes(2048 - len(text) % 2048)

        if len(data) % 2048 != 0:
            data += bytes(2048 - len(data) % 2048)

        self.size_aad = 64
        self.size = 64 + len(text) + len(data)

        if not enc_text:
            self.size_aad += len(text)

        if not enc_data:
            self.size_aad += len(data)

        self.text_size = len(text) // 2048
        self.data_size = len(data) // 2048
        self.text = text
        self.data = data

        self.mac = bytes(16)
        self.iv = bytes(12)

    def __bytes__(self) -> bytes:
        res = bytes([0xA5, 0xA5, 0xA5, 0xA5])

        res += self.mac
        res += self.iv
        res += self.size_aad.to_bytes(4, "little")
        res += self.size.to_bytes(4, "little")
        res += self.text_size.to_bytes(4, "little")
        res += self.data_size.to_bytes(4, "little")
        res += bytes(16)
        res += self.text
        res += self.data

        return res

    def crypt(self, key: bytes) -> bytes:
        if len(key) != 32:
            raise ValueError("key must be 32 bytes long")

        cipher = ChaCha20_Poly1305.new(key=key)
        data = bytes(self)

        cipher.update(data[:self.size_aad])
        txt, tag = cipher.encrypt_and_digest(data[self.size_aad:self.size])

        self.mac = tag
        self.iv = cipher.nonce

        data = bytes(self)[:self.size_aad] + txt
        return data


def extract_sections_from(filename):
    # llvm-objcopy -O binary --only-section .text /input.elf /tmp/iram.bin
    text = subprocess.run(["llvm-objcopy", "-O", "binary", "--only-section", ".text", filename, "-"], capture_output=True)
    data = subprocess.run(["llvm-objcopy", "-O", "binary", "--only-section", ".data", filename, "-"], capture_output=True)

    return text.stdout, data.stdout


def verify_sk_tag(sk: bytes, key: bytes) -> bool:
    if len(key) != 32:
        raise ValueError("key must be 32 bytes long")

    tag = sk[4:20]
    iv = sk[20:32]
    sk = sk[0:4] + bytes(28) + sk[32:]
    size_aad = int.from_bytes(sk[32:36], "little")

    cipher = ChaCha20_Poly1305.new(key=key, nonce=iv)
    cipher.update(sk[:size_aad])

    try:
        cipher.decrypt_and_verify(sk[size_aad:], tag)
    except (KeyError, ValueError):
        return False

    return True


def main(in_file: str, out_file: str, mode: str) -> int:
    if mode == "auth":
        enc_text = False
        enc_data = False
    elif mode == "data":
        enc_text = False
        enc_data = True
    elif mode == "all":
        enc_text = True
        enc_data = True
    else:
        print("Invalid mode specified: Expected 'auth', 'data' or 'all'")
        return 1

    # sk = subkernel_from_file(input)
    text, data = extract_sections_from(in_file)
    sk = SubKernel(text, data, enc_text=enc_text, enc_data=enc_data)

    key = bytes([b for b in range(0, 32)])

    enc_sk = sk.crypt(key)

    ## Sanity checks for encryption and authentication success
    if mode != "auth":
        assert(verify_sk_tag(enc_sk, key))
        assert(not verify_sk_tag(enc_sk[:-1] + bytes(1), key))

    print(f"Created subkernel with {sk.text_size} text and {sk.data_size} data sections.")

    with open(out_file, "wb") as f:
        f.write(enc_sk)

    return 0


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: sk.py <input elf file> <output subkernel image> <mode>")
        sys.exit(1)

    sys.exit(main(sys.argv[1], sys.argv[2], sys.argv[3]))

