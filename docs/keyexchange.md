# Key Exchange Mechanism

+ Simple DH scheme where each DPU has a constant public key
  $\text{PK}_D=g^{\text{SK}_D}$
+ Each client generates a new ephemeral key pair
  $\text{PK}_C=g^{\text{SK}_C}$
+ The DPU maintains a monotonically increasing counter $\text{n}$
+ The client key is derived via $\text{KDF}(\text{PK}_C^{\text{SK}_D} \mid n)$

**Value Storage**

+ The private key is stored as part of the key exchange kernels data section.
  As such, it is encrypted using the system key.
+ The counter can be stored in a trusted thread: Each time it is booted,
  it increments its storage registers by one and dump those into a
  pre-defined memory location.

**Flow**

1. The client requests to initiate the key exchange from the messaging
   subkernel. The messaging subkernel loads the key exchange subkernel.
2. The key exchange subkernel publishes its counter and waits for the client.
3. The client derives the shared secret.
4. The client writes an authenticated/encrypted subkernel into MRAM, as well
   as its public key. The DPU derives the shared secret and then loads the subkernel.
5. The client provided subkernel is executed.