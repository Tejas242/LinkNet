#include <gtest/gtest.h>
#include "linknet/crypto.h"
#include <string>
#include <vector>
#include <algorithm>

namespace linknet {
namespace test {

class CryptoTest : public ::testing::Test {
 protected:
  void SetUp() override {
    crypto_provider = crypto::CryptoFactory::Create();
  }
  
  void TearDown() override {
    crypto_provider.reset();
  }
  
  std::unique_ptr<crypto::CryptoProvider> crypto_provider;
};

TEST_F(CryptoTest, SymmetricEncryption) {
  // Create a test message
  std::string plain_text = "This is a test message for encryption";
  ByteBuffer plain_buffer(plain_text.begin(), plain_text.end());
  
  // Generate a random key and nonce
  crypto::Key key = crypto_provider->GenerateKey();
  crypto::Nonce nonce = crypto_provider->GenerateNonce();
  
  // Encrypt the message
  ByteBuffer cipher_buffer = crypto_provider->Encrypt(plain_buffer, key, nonce);
  
  // Ensure the ciphertext is not the same as the plaintext
  EXPECT_NE(plain_buffer, cipher_buffer);
  
  // Decrypt the message
  ByteBuffer decrypted_buffer = crypto_provider->Decrypt(cipher_buffer, key, nonce);
  
  // Ensure the decrypted text matches the original plaintext
  EXPECT_EQ(plain_buffer, decrypted_buffer);
  
  // Convert back to string
  std::string decrypted_text(decrypted_buffer.begin(), decrypted_buffer.end());
  EXPECT_EQ(plain_text, decrypted_text);
}

TEST_F(CryptoTest, AsymmetricEncryption) {
  // Create a test message
  std::string plain_text = "This is a test message for asymmetric encryption";
  ByteBuffer plain_buffer(plain_text.begin(), plain_text.end());
  
  // Generate key pairs for sender and receiver
  crypto::KeyPair sender_keys = crypto_provider->GenerateKeyPair();
  crypto::KeyPair receiver_keys = crypto_provider->GenerateKeyPair();
  
  // Encrypt the message using receiver's public key and sender's private key
  ByteBuffer cipher_buffer = crypto_provider->AsymmetricEncrypt(
      plain_buffer, receiver_keys.public_key, sender_keys.private_key);
  
  // Ensure the ciphertext is not the same as the plaintext
  EXPECT_NE(plain_buffer, cipher_buffer);
  
  // Decrypt the message using sender's public key and receiver's private key
  ByteBuffer decrypted_buffer = crypto_provider->AsymmetricDecrypt(
      cipher_buffer, sender_keys.public_key, receiver_keys.private_key);
  
  // Ensure the decrypted text matches the original plaintext
  EXPECT_EQ(plain_buffer, decrypted_buffer);
  
  // Convert back to string
  std::string decrypted_text(decrypted_buffer.begin(), decrypted_buffer.end());
  EXPECT_EQ(plain_text, decrypted_text);
}

TEST_F(CryptoTest, DigitalSignature) {
  // Create a test message
  std::string message = "This is a message to be signed";
  ByteBuffer message_buffer(message.begin(), message.end());
  
  // Generate a signature key pair - must use signature keys, not encryption keys
  crypto::SignatureKeyPair keys = crypto_provider->GenerateSignatureKeyPair();
  
  // Sign the message
  ByteBuffer signature = crypto_provider->Sign(message_buffer, keys.private_key);
  
  // Verify the signature
  bool result = crypto_provider->Verify(message_buffer, signature, keys.public_key);
  EXPECT_TRUE(result);
  
  // Modify the message and verify it fails
  message_buffer[0] = ~message_buffer[0];
  result = crypto_provider->Verify(message_buffer, signature, keys.public_key);
  EXPECT_FALSE(result);
}

TEST_F(CryptoTest, Hashing) {
  // Test input
  std::string input1 = "Hello, world!";
  std::string input2 = "Hello, World!";
  
  // Hash the input
  ByteBuffer hash1 = crypto_provider->Hash(input1);
  ByteBuffer hash2 = crypto_provider->Hash(input1);
  ByteBuffer hash3 = crypto_provider->Hash(input2);
  
  // The same input should produce the same hash
  EXPECT_EQ(hash1, hash2);
  
  // Different input should produce different hashes
  EXPECT_NE(hash1, hash3);
}

}  // namespace test
}  // namespace linknet
