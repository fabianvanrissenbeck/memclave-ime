# In-Memory Enclave Blocks

A small trusted loader is deployed to each DPU at boot time. Functionality not
included in this loader is provided by blocks. A block is effectively a scaled
down DPU kernel with restricted capabilities. A block is authenticated either
with the system key (deployed by the hypervisor at boot time, static and protected)
or with a user provided key that is installed by a block. Blocks that do not fall
into these categories will not be loaded.

Each block defines an input-space (in bytes), an output-space (in bytes) and the
(MRAM)-address as well as the MAC of the next block to be executed. These
inputs and outputs are used to transfer data from one block to another and are
the only thing that remains of prior blocks in WRAM. Everything else in WRAM
is purged. This does not however apply to MRAM.

## Global State

| Name | Purpose | Size |
|:-----|:--------|-----:|
| User Key | Key used to authenticate user provided blocks and to encrypt/decrypt them | 32 byte |
| Next Block Address | Address (MRAM) to load the next block from. | 8 Byte |
| Next Block MAC | Authenticator of the next block to load | 16 + 12 Byte |

## List of System Blocks

**The block loading block**

This blocks purpose is to wait for a LOAD_BLOCK message issued by the host.
Once the message arrives, it replaces the next block address and next block
mac with the ones provided by the host. This system block is special, because
it is used as a fallback when loading blocks fails in a recoverable way. 

**The key exchange block**

The key exchange block contains code required to perform key exchanges via IES.
(a half static Diffie-Hellman exchange where the DPU has a static key pair)
It takes the users public key as an input and computes the shared secret,
which replaces the global user key. The key exchange block also carries the DPUs
private key (not the system key), so this block has to be encrypted when in MRAM.

## General Workflow

1. The trusted loader is deployed to the DPU and the DPU boots up. The DPUs MRAM
   is exposed to the host system (the potential adversary) which boots up itself.
   The DPU initially loads the block loading block.
2. The host (or some client connected to the host) writes a load block message into
   MRAM and signals the DPU to process it. At this stage, no user key is 
   deployed to the DPU, which means that only blocks that carry the MAC generated
   using the system key can be loaded. (so first party code only)
   The most obvious candidate for loading at this stage is the key exchange block
   which would receive the users public key as the input.
3. The DPU does all the secure lockdown and loading stuff. The block is executed.
   After execution, the key exchange block switches the user key to the shared DH
   secret. The key exchange block specifies the block loading block as the next
   block by default.
4. The host provides some user block that is authenticated using the user key.
   This block gets some host provided inputs, does some calculation and provides
   some outputs.
5. If the host wishes to perform more actions, it can do so either by specifiying
   the block loading block as the next block or some other block already in MRAM
   as the next block.

## Block Structure

A block (while in MRAM) is one continous data structure (just like an ELF file)
that stores metadata and data on how to authenticate, decrypt and load a block on a
DPU. We probably need to write a simple ELF->Block conversion script.

### Header

| Name | Description | Size |
|:-----|:------------|-----:|
| MAC | MAC used to authenticate the block. | 16 Byte |
| IV | IV used for encryption and authentication. | 12 Byte |
| Size AAD | Amount of AAD (authenticated additional data - authenticated but not encrypted) to expect. | 4 Byte |
| Size | Size of the full block. | 4 Byte |
| Input Size | Amount of input the block expects. | 4 Byte |
| Output Size | Amount of output the block produces. | 4 Byte |
| Text Size | Amount of code the block consists of. | 4 Byte |
| Text Offset | Offset (in this block) to the instruction data. | 4 Byte |
| Data Size | Amount of data to load into WRAM. | 4 Byte |
| Data Offset | Offset (in this block) to the actual data to store in WRAM. | 4 Byte |

### Rational

Cryptographic parameters come first. Everything below the IV and MAC is at least
authenticated, but can also be encrypted. Only size aad and size parameters are
never encrypted and only authenticated. The whole block is decrypted and 
authenticated at once, with the aad coming first and the decrypted data coming
second. By changing out the text and data offset fields, one can choose whether
text or data are encrypted or not. If for example only data is supposed to be
encrypted, it is placed last in memory after text and the size aad and offset
parameters are set so that only data is encrypted and everything else is treated
as aad.

## Switching Blocks

Blocks terminate by calling the `terminate_and_switch` procedure. This procedure
performs the following actions:
+ Wait for all threads to terminate
+ Clear the user portion of WRAM and IRAM except for the output buffer in WRAM
+ Copy the output buffer to the input buffer.
+ Load the next block according to the next block address and next block MAC values
  using the secure loading procedure.