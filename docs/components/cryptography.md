# Cryptography

The cryptography module in LinkNet provides comprehensive security services for all communications, ensuring confidentiality, integrity, and authenticity.

## Overview

The `CryptoProvider` interface defines a complete suite of cryptographic operations needed for secure peer-to-peer communication. LinkNet implements this interface using modern, industry-standard cryptographic algorithms from libsodium and OpenSSL.

## Key Components

### CryptoProvider Interface

Located in `include/linknet/crypto.h`, this interface declares the cryptographic operations:

```cpp
class CryptoProvider {
 public:
  virtual ~CryptoProvider() = default;
  virtual Key GenerateKey() const = 0;
  virtual KeyPair GenerateKeyPair() const = 0;
  virtual SignatureKeyPair GenerateSignatureKeyPair() const = 0;
  virtual Nonce GenerateNonce() const = 0;
  virtual ByteBuffer Encrypt(const ByteBuffer& plaintext, 
                            const Key& key, 
                            const Nonce& nonce) const = 0;
  virtual ByteBuffer Decrypt(const ByteBuffer& ciphertext, 
                            const Key& key, 
                            const Nonce& nonce) const = 0;
  virtual ByteBuffer Sign(const ByteBuffer& data, 
                         const SignPrivateKey& private_key) const = 0;
  virtual bool Verify(const ByteBuffer& data, 
                     const ByteBuffer& signature,
                     const SignPublicKey& public_key) const = 0;
  virtual ByteBuffer Hash(const ByteBuffer& data) const = 0;
  virtual ByteBuffer DeriveKey(const ByteBuffer& password, 
                              const ByteBuffer& salt,
                              size_t iterations) const = 0;
};
```

### Sodium-based Implementation

The primary implementation uses libsodium for most operations:

- **SodiumCrypto**: Concrete implementation of the CryptoProvider interface
- **Key types**: Standard sizes for different cryptographic keys and nonces

## Cryptographic Algorithms

LinkNet employs multiple algorithms for different security requirements:

### Symmetric Encryption

For encrypting message content and files:

- **Primary**: XChaCha20-Poly1305
  - 256-bit key
  - 192-bit nonce (24 bytes)
  - Authenticated encryption with associated data (AEAD)
  - High performance on all platforms (especially non-x86)

- **Fallback**: AES-256-GCM
  - Used when hardware acceleration for AES is available
  - 256-bit key
  - 96-bit IV (12 bytes)
  - Authenticated encryption

### Asymmetric Encryption

For key exchange and initial connection setup:

- **X25519** (Curve25519)
  - Elliptic-curve Diffie-Hellman key exchange
  - 256-bit keys
  - Constant-time implementation to prevent timing attacks

### Digital Signatures

For verifying peer identities and message origin:

- **Ed25519**
  - Edwards-curve Digital Signature Algorithm
  - Fast signature verification
  - Small signatures (64 bytes)

### Key Derivation

For generating encryption keys from passwords:

- **Argon2id**
  - Memory-hard function resistant to GPU/ASIC attacks
  - Configurable memory and CPU costs
  - Salt-based to prevent rainbow table attacks

### Hashing

For integrity checks and unique identifiers:

- **BLAKE2b**
  - Fast cryptographic hash function
  - Variable output size (LinkNet uses 32 bytes)
  - NIST approved

## Security Features

### Perfect Forward Secrecy

LinkNet implements perfect forward secrecy using:
- Ephemeral key pairs for each session
- Regular key rotation during long-lived sessions

Even if a private key is compromised in the future, past communications remain secure.

### Key Management

The system handles key lifecycle with care:

1. **Generation**: Secure random generation of cryptographic keys
2. **Storage**: Protected storage of private keys with appropriate permissions
3. **Distribution**: Secure exchange of public keys
4. **Rotation**: Regular rotation of session keys
5. **Destruction**: Secure wiping of keys from memory when no longer needed

### Secure Random Number Generation

All cryptographic operations that require randomness use cryptographically secure random number generators:

- On Linux/Unix: `/dev/urandom` with appropriate fallbacks
- On Windows: `BCryptGenRandom` or equivalent
- Additional entropy mixing via libsodium

## Implementation Details

### Memory Protection

The cryptography module employs several techniques to protect sensitive data in memory:

- Locking memory pages to prevent swapping to disk
- Immediate secure zeroing of memory after use
- Constant-time operations to prevent timing attacks

### Thread Safety

All cryptographic operations are thread-safe, allowing for concurrent encryption/decryption in multithreaded contexts. This is achieved through:

- Stateless design where possible
- Thread-local storage for state when needed
- Careful synchronization when shared state is required

## Error Handling

The crypto module handles errors through exceptions:

- **CryptoException**: Base class for all cryptography errors
- **KeyGenerationError**: Failed to generate a cryptographic key
- **EncryptionError**: Failed during encryption operation
- **DecryptionError**: Failed during decryption operation
- **VerificationError**: Signature verification failed
- **HashingError**: Hashing operation failed

All exceptions include detailed error information to aid in debugging.

## Performance Considerations

Several optimizations ensure efficient cryptographic operations:

- **Hardware Acceleration**: Uses AES-NI, AVX2, and other CPU extensions when available
- **Buffer Management**: Pre-allocated buffers for frequent operations
- **Algorithm Selection**: Dynamically selects the most efficient algorithm based on hardware capabilities
- **Batching**: Combines operations when appropriate to reduce overhead

## Code Examples

### Creating a CryptoProvider

```cpp
// Create the crypto provider
auto crypto = std::make_shared<SodiumCrypto>();
```

### Encrypting and Decrypting Data

```cpp
// Generate a random key
auto key = crypto->GenerateKey();

// Generate a nonce (must be unique for each encryption with the same key)
auto nonce = crypto->GenerateNonce();

// Original message
ByteBuffer message = {'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};

// Encrypt the message
ByteBuffer encrypted = crypto->Encrypt(message, key, nonce);

// Later, decrypt the message
ByteBuffer decrypted = crypto->Decrypt(encrypted, key, nonce);
```

### Digital Signatures

```cpp
// Generate a signature key pair
auto sig_keypair = crypto->GenerateSignatureKeyPair();

// Message to sign
ByteBuffer message = {'I', 'm', 'p', 'o', 'r', 't', 'a', 'n', 't', ' ', 'm', 'e', 's', 's', 'a', 'g', 'e'};

// Sign the message
ByteBuffer signature = crypto->Sign(message, sig_keypair.private_key);

// Verify the signature
bool is_valid = crypto->Verify(message, signature, sig_keypair.public_key);
```

## Related Documentation

- [Security Model](../advanced/security_model.md) - How cryptography fits into the overall security model
- [Network Layer](network.md) - Integration with network operations
- [Message System](messaging.md) - How encryption is applied to messages
