# Messaging Between DPU and Guest

We need a solid mechanism to communicate just via MRAM. Without access
to a DPU's control registers, this needs to happen with support from the
hypervisor.

## Hypervisor Extensions

We extend the DPU's set of available features with three new instructions,
implemented via a fault based mechanism:

| Pseudo-Instruction | Actual Instruction | Description                                                                                  |
|:------------------:|:------------------:|:---------------------------------------------------------------------------------------------|
|       `lmr`        |  `fault 0xA50001`  | Switch the MRAM MUX to face the DPU.                                                         |
|       `umr`        |  `fault 0xA50002`  | Switch the MRAM MUX to face the host.                                                        |
|       `rdl`        |  `fault 0xA50010`  | Raise the DPU ready line. This causes the DPU to wait until the ready line is lowered again. |

The first two instructions are added for security reasons, the third one
to synchronise accesses with the host.

The guest OS, instead of communicating via the usual CI, gets access to a
virtual control interface (VCI) provided by the hypervisor. This VCI
implements the following functionality:

|  Function   | Description                                                      |
|:-----------:|:-----------------------------------------------------------------|
| `MUX_STATE` | Read the state of the MUX's of one particular rank.              |
| `GET_READY` | Read the state of the DPU ready line.                            |
| `CLR_READY` | Clear the state of the DPU ready line, continuing DPU execution. |    

## Messaging Mechanism

We can create a simple, half-duplex channel in MRAM by using the DPU ready line.
If the guest wants to send a message, it first waits for the DPU ready line to
be raised and then writes to message to MRAM. Once this is done the guest
lowers the line starting processing on the DPU. The DPU writes a message into
MRAM and then raises the ready line again.

## Complete Workflow

### Boot Process

1. Hypervisor starts.
2. DPU Pre-Loader is written to all involved DPUs. The system subkernels as
   well as the trusted loader are part of the Pre-Loaders image and are deployed
   as part of it. The Pre-Loader is started up.
3. Pre-Loader generates the system key and the exchange key pair, the latter
   of which is stored as part of the key exchange subkernel. Then the pre-loader
   encrypts and authenticates all system subkernels and stores them in MRAM.
   The public part of the exchange keypair is stored in MRAM.
   Then it replaces itself with the trusted loader, including the IRAM copy
   of the system key. All other references to the key are wiped. The
   pre-loader raises the DPU ready line right before an instruction that
   would transfer control to the trusted loader.
4. The hypervisor reads the public key from MRAM and publishes it. It lowers
   the DPU ready line and boots up the guest OS.
5. The trusted loader boots up with the init subkernel, that takes care of
   messaging between the guest system and itself. To wait for a message,
   the messaging kernel raises the DPU ready line.

### Normal Operation

1. The guest OS waits for the ready line to be raised. The guest OS instructs
   the DPU to load the KeyExchange subkernel with a client's public key
   as the parameter and the tag and address of the first client subkernel
   in MRAM.
2. The KeyExchange subkernel performs the relevant calculations, and then
   instructs the trusted loader to load the client subkernel with the just
   derived key.
3. The client subkernel is executed and performs some calculations. To terminate,
   it instructs the trusted loader to load the init subkernel with the system
   key and to purge WRAM and IRAM.

## Key Exchange

We perform a key-exchange using [IES](https://en.wikipedia.org/wiki/Integrated_Encryption_Scheme).
This allows sending the first client subkernel and the client public key
at the same time to the DPU without any roundtrips from DPU to client.