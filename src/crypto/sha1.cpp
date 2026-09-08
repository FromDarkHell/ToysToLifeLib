#include "crypto/sha1.h"
#include <string.h>
#include "util/reader.h"

static inline uint32_t leftRotate(uint32_t x, uint8_t bits) {
    return (x << bits) | (x >> (32 - bits));
}

SHA1::SHA1() {
    reset();
}

void SHA1::reset() {
    _h[0] = 0x67452301;
    _h[1] = 0xEFCDAB89;
    _h[2] = 0x98BADCFE;
    _h[3] = 0x10325476;
    _h[4] = 0xC3D2E1F0;
    _chunkSize = 0;
    _length = 0;
}

void SHA1::update(const void* data, size_t len) {
    _length += ((uint64_t)len) << 3;

    const uint8_t* d = (const uint8_t*)data;
    while (len > 0) {
        uint8_t size = 64 - _chunkSize;
        if (size > len) {
            size = (uint8_t)len;
        }

        memcpy(((uint8_t*)_w) + _chunkSize, d, size);
        _chunkSize += size;
        len -= size;
        d += size;

        if (_chunkSize == 64) {
            processChunk();
            _chunkSize = 0;
        }
    }
}

void SHA1::finalize(uint8_t hash[HASH_SIZE]) {
    uint8_t* wbytes = (uint8_t*)_w;

    if (_chunkSize <= (64 - 9)) {
        wbytes[_chunkSize] = 0x80;
        memset(wbytes + _chunkSize + 1, 0x00, 64 - 8 - (_chunkSize + 1));
        Reader::writeUInt32BE(wbytes, 56, (uint32_t)(_length >> 32));
        Reader::writeUInt32BE(wbytes, 60, (uint32_t)_length);
        processChunk();
    } else {
        wbytes[_chunkSize] = 0x80;
        memset(wbytes + _chunkSize + 1, 0x00, 64 - (_chunkSize + 1));
        processChunk();
        memset(wbytes, 0x00, 64 - 8);
        Reader::writeUInt32BE(wbytes, 56, (uint32_t)(_length >> 32));
        Reader::writeUInt32BE(wbytes, 60, (uint32_t)_length);
        processChunk();
    }

    for (uint8_t i = 0; i < 5; ++i) {
        Reader::writeUInt32BE(hash, i * 4, _h[i]);
    }
}

void SHA1::hash(const void* data, size_t len, uint8_t out[HASH_SIZE]) {
    SHA1 sha;
    sha.update(data, len);
    sha.finalize(out);
}

// Reference: https://en.wikipedia.org/wiki/SHA-1
void SHA1::processChunk() {
    uint8_t* wbytes = (uint8_t*)_w;
    uint32_t w[80];
    for (uint8_t i = 0; i < 16; ++i) {
        w[i] = Reader::readUInt32BE(wbytes, i * 4);
    }

    uint32_t a = _h[0];
    uint32_t b = _h[1];
    uint32_t c = _h[2];
    uint32_t d = _h[3];
    uint32_t e = _h[4];

    for (uint8_t i = 0; i < 80; ++i) {
        uint32_t f, k;

        if (i < 16) {
            // w[i] already loaded above
        } else {
            w[i] = leftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        uint32_t temp = leftRotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = leftRotate(b, 30);
        b = a;
        a = temp;
    }

    _h[0] += a;
    _h[1] += b;
    _h[2] += c;
    _h[3] += d;
    _h[4] += e;
}
