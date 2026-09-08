#include "infinity/figure_crypto.h"
#include <string.h>
#include <AES.h>
#include "crypto/sha1.h"

using namespace DisneyInfinity;

void FigureCrypto::deriveKey(const uint8_t uid[UID_SIZE], uint8_t outKey[KEY_SIZE]) {
    // First 31 bytes of a fixed constant that (not coincidentally) ends in the ASCII text
    // "(c) Disney 2013" - the same string WAKE's payload carries.
    static const uint8_t CONSTANT[31] = {0xAF, 0x62, 0xD2, 0xEC, 0x04, 0x91, 0x96, 0x8C, 0xC5,
                                          0x2A, 0x1A, 0x71, 0x65, 0xF8, 0x65, 0xFE, 0x28, 0x63,
                                          0x29, 0x20, 0x44, 0x69, 0x73, 0x6E, 0x65, 0x79, 0x20,
                                          0x32, 0x30, 0x31, 0x33};

    uint8_t buffer[sizeof(CONSTANT) + UID_SIZE];
    memcpy(buffer, CONSTANT, sizeof(CONSTANT));
    memcpy(buffer + sizeof(CONSTANT), uid, UID_SIZE);

    uint8_t digest[SHA1::HASH_SIZE];
    SHA1::hash(buffer, sizeof(buffer), digest);

    // Each 4-byte group of the first 16 digest bytes gets byte-reversed.
    for (int group = 0; group < 4; group++) {
        for (int x = 3; x >= 0; x--) {
            outKey[(3 - x) + group * 4] = digest[x + group * 4];
        }
    }
}

void FigureCrypto::buildPlaintextBlock(uint32_t figureId, uint8_t outBlock[BLOCK_SIZE]) {
    static const uint8_t BLANK_TEMPLATE[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x00, 0x00, 0x01, 0xD1, 0x1F};

    memcpy(outBlock, BLANK_TEMPLATE, sizeof(BLANK_TEMPLATE));
    outBlock[1] = (uint8_t)((figureId >> 16) & 0xFF);
    outBlock[2] = (uint8_t)((figureId >> 8) & 0xFF);
    outBlock[3] = (uint8_t)(figureId & 0xFF);

    const uint32_t checksum = crc32(outBlock, 12);
    for (int i = 3; i >= 0; i--) {
        outBlock[15 - i] = (uint8_t)((checksum >> (i * 8)) & 0xFF);
    }
}

void FigureCrypto::encryptBlock(const uint8_t key[KEY_SIZE], const uint8_t plaintext[BLOCK_SIZE],
                                 uint8_t outCiphertext[BLOCK_SIZE]) {
    AES128 aes;
    aes.setKey(key, KEY_SIZE);
    aes.encryptBlock(outCiphertext, plaintext);
}

void FigureCrypto::decryptBlock(const uint8_t key[KEY_SIZE], const uint8_t ciphertext[BLOCK_SIZE],
                                 uint8_t outPlaintext[BLOCK_SIZE]) {
    AES128 aes;
    aes.setKey(key, KEY_SIZE);
    aes.decryptBlock(outPlaintext, ciphertext);
}

uint32_t FigureCrypto::extractFigureId(const uint8_t plaintext[BLOCK_SIZE]) {
    return ((uint32_t)plaintext[1] << 16) | ((uint32_t)plaintext[2] << 8) | (uint32_t)plaintext[3];
}

uint32_t FigureCrypto::crc32(const uint8_t* data, size_t len) {
    static uint32_t table[256];
    static bool tableInit = false;

    if (!tableInit) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        tableInit = true;
    }

    uint32_t ret = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t index = (uint8_t)(ret & 0xFF) ^ data[i];
        ret = (ret >> 8) ^ table[index];
    }
    return ret;
}
