#include "box.hpp"
#include <stdexcept> // std::runtime_error
#include <cstring> // std::memcpy

#define CRYPTO_ASSERT(condition, message) GLOBED_ASSERT(condition, "crypto error: " message)
#define CRYPTO_ERR_CHECK(result, message) CRYPTO_ASSERT(result == 0, message)

using namespace util::data;

CryptoBox::CryptoBox(byte* key) {
    // sodium_init returns 0 on success, 1 if already initialized, -1 on fail
    CRYPTO_ASSERT(sodium_init() != -1, "sodium_init failed");
    CRYPTO_ERR_CHECK(crypto_box_keypair(publicKey, secretKey), "crypto_box_keypair failed");

    if (key != nullptr) {
        setPeerKey(key);
    }
}

byte* CryptoBox::getPublicKey() noexcept {
    return publicKey;
}

void CryptoBox::setPeerKey(byte* key) noexcept {
    std::memcpy(peerPublicKey, key, crypto_box_PUBLICKEYBYTES);
}

size_t CryptoBox::encryptInto(const byte* src, byte* dest, size_t size) {
    byte* nonce = dest;
    randombytes_buf(nonce, NONCE_LEN);

    byte* ciphertext = dest + NONCE_LEN;
    CRYPTO_ERR_CHECK(crypto_box_easy(ciphertext, src, size, nonce, peerPublicKey, secretKey), "crypto_box_easy failed");

    return size + PREFIX_LEN;
}

size_t CryptoBox::encryptInto(const std::string& src, byte* dest) {
    return encryptInto(reinterpret_cast<const byte*>(src.c_str()), dest, src.size());
}

size_t CryptoBox::encryptInto(const bytevector& src, byte* dest) {
    return encryptInto(src.data(), dest, src.size());
}

size_t CryptoBox::encryptInPlace(byte* data, size_t size) {
    return encryptInto(data, data, size);
}

bytevector CryptoBox::encrypt(const bytevector& src) {
    return encrypt(src.data(), src.size());
}

bytevector CryptoBox::encrypt(const byte* src, size_t size) {
    bytevector output(size + PREFIX_LEN);
    encryptInto(src, output.data(), size);
    return output;
}

bytevector CryptoBox::encrypt(const std::string& src) {
    return encrypt(reinterpret_cast<const byte*>(src.c_str()), src.size());
}



size_t CryptoBox::decryptInto(const util::data::byte* src, util::data::byte* dest, size_t size) {
    CRYPTO_ASSERT(size >= PREFIX_LEN, "message is too short");

    const byte* nonce = src;
    const byte* ciphertext = src + NONCE_LEN;

    size_t plaintextLength = size - PREFIX_LEN;
    size_t ciphertextLength = size - NONCE_LEN;

    CRYPTO_ERR_CHECK(crypto_box_open_easy(dest, ciphertext, ciphertextLength, nonce, peerPublicKey, secretKey), "crypto_box_open_easy failed");

    return plaintextLength;
}

size_t CryptoBox::decryptInPlace(util::data::byte* data, size_t size) {
    return decryptInto(data, data, size);
}

bytevector CryptoBox::decrypt(const bytevector& src) {
    return decrypt(src.data(), src.size());
}

bytevector CryptoBox::decrypt(const byte* src, size_t size) {
    size_t plaintextLength = size - PREFIX_LEN;

    bytevector plaintext(plaintextLength);
    decryptInto(src, plaintext.data(), size);

    return plaintext;
}

std::string CryptoBox::decryptToString(const bytevector& src) {
    auto vec = decrypt(src);
    return std::string(vec.begin(), vec.end());
}

std::string CryptoBox::decryptToString(const byte* src, size_t size) {
    auto vec = decrypt(src, size);
    return std::string(vec.begin(), vec.end());
}