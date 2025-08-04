import sys
from Crypto.Cipher import ChaCha20_Poly1305

key = bytes([b + 0x80 for b in range(0, 32)])
iv = bytes([0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7])

with open("/dev/urandom", "rb") as f:
    data = f.read(4160)

data = data[0:32] + bytes(16) + data[48:]

# with open("/dev/urandom", "rb") as f:
#     data = f.read(4160)

# data = bytes(4160)
# assert(len(data) % 16 == 0)

# with open("msg.sk", "rb") as f:
#     data = f.read()

# data = data[0:4] + bytes(28) + data[32:]
# assert(len(data) % 16 == 0)

cipher = ChaCha20_Poly1305.new(key=key, nonce=iv)
cipher.update(data)
txt, tag = cipher.encrypt_and_digest(b"")

sys.stdout.buffer.write(tag + data)