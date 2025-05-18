# Security Model

LinkNet's security model provides a comprehensive framework for protecting user communications, ensuring privacy, integrity, and authenticity in a peer-to-peer environment.

## Overview

Security is a core design principle in LinkNet, not an afterthought. The system employs a multi-layered approach to security, using modern cryptographic algorithms and best practices to protect all communications between peers.

## Security Goals

LinkNet's security model addresses the following key objectives:

1. **Confidentiality**: Ensure that only intended recipients can read messages and files
2. **Integrity**: Guarantee that messages and files cannot be modified without detection
3. **Authenticity**: Verify the identity of peers to prevent impersonation
4. **Forward Secrecy**: Protect past communications even if keys are later compromised
5. **Non-repudiation**: Provide proof of message origin
6. **Metadata Protection**: Limit the exposure of communication metadata
7. **Resilience**: Withstand sophisticated attack vectors

## Threat Model

LinkNet's security design considers various potential threats:

| Threat | Description | Mitigation |
|--------|-------------|------------|
| **Passive Eavesdropping** | Attackers capturing network traffic | End-to-end encryption of all communications |
| **Man-in-the-Middle** | Intercepting and possibly altering communications | Strong peer authentication and key verification |
| **Impersonation** | Pretending to be a legitimate peer | Public key cryptography and digital signatures |
| **Message Tampering** | Modifying message contents | Authenticated encryption and message integrity checks |
| **Replay Attacks** | Re-sending previously captured messages | Unique message IDs and timestamps |
| **Traffic Analysis** | Analyzing metadata to infer information | Padding, mixing, and metadata minimization |
| **Key Compromise** | Theft of cryptographic keys | Forward secrecy and regular key rotation |
| **Side-Channel Attacks** | Extracting information through timing, power, etc. | Constant-time operations and memory protection |

## Cryptographic Building Blocks

LinkNet leverages modern cryptographic primitives:

### Symmetric Encryption

For bulk data encryption and message confidentiality:
- **XChaCha20-Poly1305**: AEAD encryption with 256-bit keys
- **AES-256-GCM**: Alternative when hardware acceleration is available

### Asymmetric Encryption

For key exchange and authentication:
- **X25519**: Elliptic curve Diffie-Hellman for key agreement
- **Ed25519**: Edwards-curve Digital Signature Algorithm for signatures

### Hash Functions

For data integrity and derivation:
- **BLAKE2b**: Fast cryptographic hash function
- **SHA-256**: Used for compatibility with certain protocols

### Key Derivation

For generating keys from passwords:
- **Argon2id**: Memory-hard password hashing
- **HKDF**: Hash-based key derivation for session keys

## Key Management

### Identity Keys

Each peer has a permanent identity represented by:
- Ed25519 key pair for digital signatures
- X25519 key pair for encryption

These keys are:
- Generated during first application launch
- Stored securely on the local device
- Used to authenticate the peer across sessions

### Session Keys

Temporary keys used for message encryption:
- Generated fresh for each connection
- Used for symmetric encryption of data
- Rotated periodically during long sessions

### Key Exchange

Session keys are established using:
1. X25519 ECDH key agreement
2. HKDF key derivation with mixed public and private inputs
3. Authenticated with identity signatures

```
┌────────────┐                       ┌────────────┐
│   Peer A   │                       │   Peer B   │
└─────┬──────┘                       └──────┬─────┘
      │                                     │
      │ 1. Generate ephemeral key pair     │ 1. Generate ephemeral key pair
      │    (ea_pub, ea_priv)               │    (eb_pub, eb_priv)
      │                                     │
      │ 2. Sign: sig_a = Sign(ea_pub, A_id)│
      │                                     │
      │    A_id, ea_pub, sig_a             │
      │────────────────────────────────────▶│ 3. Verify signature using A's 
      │                                     │    public identity key
      │                                     │
      │                                     │ 4. Sign: sig_b = Sign(eb_pub, B_id)
      │    B_id, eb_pub, sig_b             │
      │◀────────────────────────────────────│
      │                                     │
      │ 5. Verify signature using B's      │
      │    public identity key             │
      │                                     │
      │ 6. Compute shared secret:          │ 6. Compute shared secret:
      │    ss = ECDH(ea_priv, eb_pub)      │    ss = ECDH(eb_priv, ea_pub)
      │                                     │
      │ 7. Derive session keys:            │ 7. Derive session keys:
      │    k_ab, k_ba = HKDF(ss, A_id|B_id)│    k_ab, k_ba = HKDF(ss, A_id|B_id)
```

## Encryption Layers

LinkNet implements security at multiple layers:

### Transport Security

- Optional TLS for connections in environments that support it
- Certificate validation for TLS connections
- TCP/IP socket encryption when TLS is unavailable

### Message Security

- Each message individually encrypted
- Message authentication codes (MACs) for integrity
- Unique IVs/nonces for every message

### Content Security

- Additional encryption for sensitive content
- Different keys for different content types
- Optional password protection for critical files

## Authentication Process

Peers authenticate each other through:

1. **Identity Verification**: Public keys are exchanged and verified
2. **Challenge-Response**: Proves possession of the corresponding private key
3. **Trust Establishment**: Users can verify fingerprints through out-of-band channels

### Trust Model

LinkNet employs a direct trust model:
- No centralized certificate authorities
- Users directly verify peer identities
- Trust-on-first-use with optional fingerprint verification
- Persistent identity storage for previously verified peers

## Security Features

### Perfect Forward Secrecy

LinkNet ensures that compromise of long-term keys cannot retrospectively decrypt past communications:
- Ephemeral session keys for each connection
- Regular rotation of encryption keys during long sessions

### Message Authentication

All messages include:
- Digital signature from the sender
- Message authentication code (MAC)
- Tamper-evident serialization

### Side-Channel Protection

Mitigations against timing attacks and other side channels:
- Constant-time cryptographic operations
- Memory protection for sensitive data
- Secure erasure of keys after use

## Implementation Best Practices

LinkNet follows security best practices:

- **No Custom Crypto**: Relies on established libraries and algorithms
- **Updated Dependencies**: Regular updates to cryptographic libraries
- **Secure Defaults**: Secure configuration by default
- **Fail Secure**: If a security operation fails, the system errs on the side of security
- **Defense in Depth**: Multiple security layers that don't depend on each other

## Security Considerations for Extensions

When extending LinkNet, follow these security guidelines:

1. **Preserve End-to-End Encryption**: Never send unencrypted data
2. **Validate All Inputs**: Treat all external data as potentially malicious
3. **Respect Key Separation**: Use different keys for different purposes
4. **Minimize Trust Requirements**: Don't introduce new trusted third parties
5. **Consider Metadata Leakage**: Be aware of information leaked by timing, sizes, etc.

## Security Audit Process

LinkNet's security is regularly evaluated:

- **Static Analysis**: Automated tools to detect security issues
- **Dynamic Testing**: Fuzzing and runtime analysis
- **Code Review**: Security-focused code reviews
- **External Audits**: Third-party security audits

## Related Documentation

- [Cryptography](../components/cryptography.md) - Details of the cryptographic implementation
- [Network Layer](../components/network.md) - Security at the network level
- [File Transfer](../components/file_transfer.md) - Security for file transfers
