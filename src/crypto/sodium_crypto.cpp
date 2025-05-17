#include "linknet/crypto.h"
#include "linknet/logger.h"
#include <sodium.h>
#include <random>
#include <stdexcept>
#include <cassert>

namespace linknet {
namespace crypto {

class SodiumCryptoProvider : public CryptoProvider {
 public:
  SodiumCryptoProvider() {
    if (sodium_init() < 0) {
      LOG_FATAL("Failed to initialize sodium library");
      throw std::runtime_error("Failed to initialize sodium library");
    }
  }

  ~SodiumCryptoProvider() override = default;

  Key GenerateKey() const override {
    Key key;
    randombytes_buf(key.data(), key.size());
    return key;
  }
  
  KeyPair GenerateKeyPair() const override {
    KeyPair key_pair;
    if (crypto_box_keypair(key_pair.public_key.data(), key_pair.private_key.data()) != 0) {
      LOG_ERROR("Failed to generate keypair");
      throw std::runtime_error("Failed to generate keypair");
    }
    return key_pair;
  }
  
  Nonce GenerateNonce() const override {
    Nonce nonce;
    randombytes_buf(nonce.data(), nonce.size());
    return nonce;
  }
  
  ByteBuffer Hash(const std::string& data) const override {
    ByteBuffer hash(crypto_hash_sha256_BYTES);
    crypto_hash_sha256(hash.data(), 
                       reinterpret_cast<const unsigned char*>(data.data()), 
                       data.size());
    return hash;
  }
  
  ByteBuffer Encrypt(const ByteBuffer& plaintext, 
                     const Key& key, 
                     const Nonce& nonce) const override {
    // Output will be ciphertext + MAC
    ByteBuffer ciphertext(plaintext.size() + crypto_secretbox_MACBYTES);
    
    if (crypto_secretbox_easy(ciphertext.data(), 
                             plaintext.data(), 
                             plaintext.size(), 
                             nonce.data(), 
                             key.data()) != 0) {
      LOG_ERROR("Encryption failed");
      throw std::runtime_error("Encryption failed");
    }
    
    return ciphertext;
  }
  
  ByteBuffer Decrypt(const ByteBuffer& ciphertext, 
                     const Key& key, 
                     const Nonce& nonce) const override {
    // Check if the ciphertext is large enough to contain the MAC
    if (ciphertext.size() < crypto_secretbox_MACBYTES) {
      LOG_ERROR("Ciphertext too short");
      throw std::invalid_argument("Ciphertext too short");
    }
    
    // Output will be just the plaintext (without MAC)
    ByteBuffer plaintext(ciphertext.size() - crypto_secretbox_MACBYTES);
    
    if (crypto_secretbox_open_easy(plaintext.data(), 
                                  ciphertext.data(), 
                                  ciphertext.size(), 
                                  nonce.data(), 
                                  key.data()) != 0) {
      LOG_ERROR("Decryption failed");
      throw std::runtime_error("Decryption failed");
    }
    
    return plaintext;
  }
  
  ByteBuffer AsymmetricEncrypt(const ByteBuffer& plaintext,
                              const Key& receiver_public_key,
                              const Key& sender_private_key) const override {
    // Output will be ciphertext + MAC
    ByteBuffer ciphertext(plaintext.size() + crypto_box_MACBYTES);
    Nonce nonce = GenerateNonce();
    
    // We need to include the nonce with the ciphertext
    ByteBuffer result(nonce.size() + ciphertext.size());
    
    if (crypto_box_easy(ciphertext.data(), 
                       plaintext.data(), 
                       plaintext.size(), 
                       nonce.data(), 
                       receiver_public_key.data(), 
                       sender_private_key.data()) != 0) {
      LOG_ERROR("Asymmetric encryption failed");
      throw std::runtime_error("Asymmetric encryption failed");
    }
    
    // Copy the nonce and ciphertext to the result
    std::copy(nonce.begin(), nonce.end(), result.begin());
    std::copy(ciphertext.begin(), ciphertext.end(), result.begin() + nonce.size());
    
    return result;
  }
  
  ByteBuffer AsymmetricDecrypt(const ByteBuffer& data,
                              const Key& sender_public_key,
                              const Key& receiver_private_key) const override {
    // Check if the data is large enough to contain the nonce and MAC
    if (data.size() < NONCE_SIZE + crypto_box_MACBYTES) {
      LOG_ERROR("Encrypted data too short");
      throw std::invalid_argument("Encrypted data too short");
    }
    
    // Extract the nonce
    Nonce nonce;
    std::copy(data.begin(), data.begin() + NONCE_SIZE, nonce.begin());
    
    // Extract the ciphertext
    ByteBuffer ciphertext(data.begin() + NONCE_SIZE, data.end());
    
    // Output will be just the plaintext (without MAC)
    ByteBuffer plaintext(ciphertext.size() - crypto_box_MACBYTES);
    
    if (crypto_box_open_easy(plaintext.data(), 
                            ciphertext.data(), 
                            ciphertext.size(), 
                            nonce.data(), 
                            sender_public_key.data(), 
                            receiver_private_key.data()) != 0) {
      LOG_ERROR("Asymmetric decryption failed");
      throw std::runtime_error("Asymmetric decryption failed");
    }
    
    return plaintext;
  }
  
  ByteBuffer Sign(const ByteBuffer& message, 
                 const Key& private_key) const override {
    ByteBuffer signature(crypto_sign_BYTES);
    
    if (crypto_sign_detached(signature.data(), 
                            nullptr, 
                            message.data(), 
                            message.size(), 
                            private_key.data()) != 0) {
      LOG_ERROR("Signature generation failed");
      throw std::runtime_error("Signature generation failed");
    }
    
    return signature;
  }
  
  bool Verify(const ByteBuffer& message, 
              const ByteBuffer& signature,
              const Key& public_key) const override {
    if (signature.size() != crypto_sign_BYTES) {
      LOG_ERROR("Invalid signature size");
      return false;
    }
    
    return crypto_sign_verify_detached(signature.data(), 
                                      message.data(), 
                                      message.size(), 
                                      public_key.data()) == 0;
  }
};

std::unique_ptr<CryptoProvider> CryptoFactory::Create() {
  return std::make_unique<SodiumCryptoProvider>();
}

}  // namespace crypto
}  // namespace linknet
