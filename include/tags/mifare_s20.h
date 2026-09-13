#pragma once
#include <Arduino.h>

namespace Tags
{
    /**
     * @brief Raw NFC Tag Buffer, this is an object which contains 20 different 16-byte pages.
     * Each of these tags gets an associated "UID"
     */
    class MIFARE_S20
    {
    public:
        static const uint8_t PAGE_SIZE = 16;
        static const uint8_t PAGE_COUNT = 20;
        static const uint16_t TAG_SIZE = PAGE_SIZE * PAGE_COUNT;

        uint8_t _buf[TAG_SIZE];
        uint8_t *data;
        uint16_t dataLen;

        MIFARE_S20() : data(_buf), dataLen(TAG_SIZE) { memset(_buf, 0, TAG_SIZE); }

        MIFARE_S20(const MIFARE_S20 &other) : data(_buf), dataLen(other.dataLen)
        {
            memcpy(_buf, other._buf, TAG_SIZE);
        }

        // Copy assignment operator
        MIFARE_S20 &operator=(const MIFARE_S20 &other)
        {
            if (this != &other)
            {
                dataLen = other.dataLen;
                memcpy(_buf, other._buf, TAG_SIZE);
                data = _buf; // Always point to OUR buffer, never the source's
            }
            return *this;
        }

        MIFARE_S20(const uint8_t *src, uint8_t len) : data(_buf), dataLen(TAG_SIZE)
        {
            memset(_buf, 0, TAG_SIZE);
            load(src, len);
        }

        /**
         * @brief Returns the UID as an output buffer
         *
         * @param out
         */
        void getUID(uint8_t out[7]) const { memcpy(out, _buf, 7); }

        /**
         * @brief Sets the tag's 7-byte UID (stored in block 0).
         *
         * @param uid
         */
        void setUID(const uint8_t uid[7]) { memcpy(_buf, uid, 7); }

        /**
         * @brief Copies a specific page from the buffer to `out`
         *
         * @param page The specific page index to copy from
         * @param out The output buffer to copy the page from
         * @return true When the copy is successful
         * @return false If the resulting page would overflow the NFC buffer
         */
        bool get(uint8_t page, uint8_t *out) const
        {
            uint8_t start = page * PAGE_SIZE;
            if (start + PAGE_SIZE > dataLen)
            {
                return false;
            }
            memcpy(out, data + start, PAGE_SIZE);
            return true;
        }

        /**
         * @brief Sets a specific page to the values from `src`
         *
         * @param page The specific page index to change
         * @param src A buffer of the page values
         * @return true When the page data is updated
         * @return false When writing the page would overflow the NFC buffer
         */
        bool set(uint8_t page, const uint8_t *src)
        {
            uint8_t start = page * PAGE_SIZE;
            if (start + PAGE_SIZE > dataLen)
            {
                return false;
            }
            memcpy(data + start, src, PAGE_SIZE);
            return true;
        }

        // ------------------------------------------------------------------
        // load / dump; operate on raw bytes.
        // ------------------------------------------------------------------
        void load(const uint8_t *src, uint8_t len)
        {
            uint8_t n = (len < TAG_SIZE) ? len : TAG_SIZE;
            memcpy(_buf, src, n);
            dataLen = n;
        }

        uint8_t dump(uint8_t *dest, uint8_t maxLen) const
        {
            uint8_t n = (dataLen < maxLen) ? dataLen : maxLen;
            memcpy(dest, data, n);
            return n;
        }

        /**
         * @brief Randomly generates a NXP MIFARE UID
         * The first byte is the NXP vendor ID
         * Bytes 1-6 are all randomly generated
         * The 7th byte is a constant (?)
         *
         * @return uint64_t Returns all 7 bytes packed into a uint64_t in little endian order
         */
        static uint64_t randomUID()
        {
            uint8_t b[8] = {
                static_cast<uint8_t>(random(0, 0xFF)),
                static_cast<uint8_t>(random(0, 0xFF)),
                static_cast<uint8_t>(random(0, 0xFF)),
                static_cast<uint8_t>(random(0, 0xFF)),
                static_cast<uint8_t>(random(0, 0xFF)),
                static_cast<uint8_t>(random(0, 0xFF)),
                static_cast<uint8_t>(random(0, 0xFF)),
                static_cast<uint8_t>(random(0, 0xFF)),
            };

            uint64_t out = 0;
            for (uint8_t i = 0; i < 8; i++)
            {
                out |= (uint64_t)b[i] << (i * 8);
            }

            return out;
        }
    };

};