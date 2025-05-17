#ifndef LINKNET_CRYPTO_H_
#define LINKNET_CRYPTO_H_

#include "linknet/types.h"
#include <string>
#include <array>
#include <memory>

namespace linknet {
namespace crypto {

// Size constants
constexpr size_t KEY_SIZE = 32;
constexpr size_t NONCE_SIZE = 24;
constexpr size_t MAC_SIZE = 16;
constexpr size_t SIGN_PUBLICKEY_SIZE = 32;
constexpr size_t SIGN_SECRETKEY_SIZE = 64;  // Ed25519 secret key is 64 bytes in libsodium

// Key types
using Key = std::array<uint8_t, KEY_SIZE>;
using Nonce = std::array<uint8_t, NONCE_SIZE>;
using SignPublicKey = std::array<uint8_t, SIGN_PUBLICKEY_SIZE>;
using SignPrivateKey = std::array<uint8_t, SIGN_SECRETKEY_SIZE>;

// Key pair for asymmetric encryption
struct KeyPair {
  Key public_key;
  Key private_key;
};

// Key pair specifically for digital signatures
struct SignatureKeyPair {
  SignPublicKey public_key;
  SignPrivateKey private_key;
};

// Interface for cryptographic operations
class CryptoProvider {
 public:
  virtual ~CryptoProvider() = default;

  // Generate a new random key
  virtual Key GenerateKey() const = 0;
  
  // Generate a keypair for asymmetric encryption
  virtual KeyPair GenerateKeyPair() const = 0;
  
  // Generate a keypair specifically for digital signatures
  virtual SignatureKeyPair GenerateSignatureKeyPair() const = 0;
  
  // Generate a random nonce
  virtual Nonce GenerateNonce() const = 0;
  
  // Hash a string using a cryptographically secure hash function (SHA-256)
  virtual ByteBuffer Hash(const std::string& data) const = 0;
  
  // Symmetric encryption/decryption
  virtual ByteBuffer Encrypt(const ByteBuffer& plaintext, 
                           const Key& key, 
                           const Nonce& nonce) const = 0;
  
  virtual ByteBuffer Decrypt(const ByteBuffer& ciphertext, 
                           const Key& key, 
                           const Nonce& nonce) const = 0;
  
  // Asymmetric encryption/decryption
  virtual ByteBuffer AsymmetricEncrypt(const ByteBuffer& plaintext,
                                     const Key& receiver_public_key,
                                     const Key& sender_private_key) const = 0;
  
  virtual ByteBuffer AsymmetricDecrypt(const ByteBuffer& ciphertext,
                                     const Key& sender_public_key,
                                     const Key& receiver_private_key) const = 0;
  
  // Digital signatures
  virtual ByteBuffer Sign(const ByteBuffer& message, 
                        const SignPrivateKey& private_key) const = 0;
  
  virtual bool Verify(const ByteBuffer& message, 
                     const ByteBuffer& signature,
                     const SignPublicKey& public_key) const = 0;
};

// Factory to create a concrete implementation
class CryptoFactory {
 public:
  static std::unique_ptr<CryptoProvider> Create();
};

}  // namespace crypto
}  // namespace linknet

#endif  // LINKNET_CRYPTO_H_
