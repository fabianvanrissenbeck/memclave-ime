# In-Memory Enclave Blocks

A small trusted loader is deployed to each DPU at boot time. Functionality not
included in this loader is provided by blocks. A block is effectively a scaled
down DPU kernel with restricted capabilities. A block is authenticated either
with the system key (deployed by the hypervisor at boot time, static and protected)
or with a user provided key that is installed by another block. Blocks that are
not authenticated either with the system or the user key will not be loaded
and executed.

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

## Subkernel Structure

A block (while in MRAM) is one continous data structure (just like an ELF file)
that stores metadata and data on how to authenticate, decrypt and load a block on a
DPU. We probably need to write a simple ELF->Block conversion script.

### Header

| Name      | Description                                                                                |    Size | Offset |
|:----------|:-------------------------------------------------------------------------------------------|--------:|-------:|
| Magic     | Magic number to weed out stupid mistakes.                                                  |  4 Byte |      0 |
| MAC       | MAC used to authenticate the block.                                                        | 16 Byte |      4 |
| IV        | IV used for encryption and authentication.                                                 | 12 Byte |     20 |
| Size AAD  | Amount of AAD (authenticated additional data - authenticated but not encrypted) to expect. |  4 Byte |     32 |
| Size      | Size of the full block.                                                                    |  4 Byte |     36 |
| Text Size | Size of the text segment in 2048 byte increments.                                          |  4 Byte |     40 |
| Data Size | Size of the data segment in bytes.                                                         |  4 Byte |     44 |

### Format

| Name   | Description                            |              Size |
|:-------|:---------------------------------------|------------------:|
| Header | Metadata for loading and verification. | 64 Bytes (padded) |
| Text   | Code of the subkernel to load to IRAM. |  Text Size * 2048 |
| Data   | Data of the subkernel to load to WRAM. |         Data Size |

### Rational

The magic number is used to catch loads from incorrect addresses early.
The next likely error would be a failure in TAG verification which
is horrific to debug. The next 4 parameters are required to perform
an AEAD decryption. The whole subkernel is decrypted and authenticated
at once.

We place the text segment at a constant offset after the header. This
allows us to prevent any dynamic modifications to the load address.
ROP\`ing the `ldmai` instruction should therefore always result in a
memory fault, because the registers (hardcoded) aren't accessible
from outside the trusted core. The text size is in 2048 byte (or 256 instruction)
increments. We do this to hardcode the amount of code loaded through each
`ldmai` invocation.


## Switching Blocks

Blocks terminate by calling the `terminate_and_switch` procedure. This procedure
performs the following actions:
+ Wait for all threads to terminate
+ Clear the user portion of WRAM and IRAM except for the output buffer in WRAM
+ Copy the output buffer to the input buffer.
+ Load the next block according to the next block address and next block MAC values
  using the secure loading procedure.

## Properties

+ Blocks can be strongly bound together. One block can ensure than only one trusted
  block may run after it. This ensures that outputs are only handled by trusted 
  code.
+ Blocks can be kept fully confidential. While in MRAM, a block may be fully
  encrypted. We're doing this with the key exchange block for example.
+ System and user blocks are handled by the same underlying core loader. This
  reduces the amount of vulnerable loading code and complexity significantly.
  This also allows easier verification of system components, because they
  are more strongly decoupled than in a system that keeps them all in the core.
  Vulnerabilities in system components do not effect the integrity of the core
  loader, system components have the same restrictions as user ones, they for
  example are not allowed to use the banned instructions.

## Current or not yet Fully Solved Issues

+ Preventing ROP to `ldmai` gadget if more than one malicious threat exists.
+ Checking the state of all threads (reasonably sure I can do this)