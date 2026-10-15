#pragma once

#include <Arduino.h>
#include "kamenrider/constants.h"
#include "tags/mifare_s20.h"
#include "util/reader.h"

using namespace Tags;

namespace KamenRider
{
    /**
     * @brief Base class for all Kamen Rider toy tags.
     *        Extends MIFARE_S20 with game metadata (id).
     *
     * The tag layout matches RPCS3's figure creator (rpcs3qt/kamen_rider_dialog.cpp):
     *  - Page 0: `[0x04][uid * 5][0x80][0x89][0x44][0x00][0xC2]...`
     *  - Page 1 (the "figure block"): `[random][03 00 00 01 0E 0A 0A][id, LE u32][crc32, LE u32]`
     */
    class ToyTag : public MIFARE_S20
    {
    public:
        /**
         * @brief An ID describing the tag in the JSON database of characters/vehicles
         *
         */
        uint32_t id;

        ToyTag() : id(0)
        {
            randomizeKamenRiderUID();
            commitToBuffer();
        }

        explicit ToyTag(uint32_t tagId) : id(tagId)
        {
            randomizeKamenRiderUID();
            commitToBuffer();
        }

        /**
         * @brief Writes a new random UID shaped like RPCS3's: an NXP vendor byte (0x04), 5 random
         * bytes, then 0x80.
         *
         */
        void randomizeKamenRiderUID()
        {
            uint8_t uid[TAG_UID_SIZE] = {0x04};
            for (uint8_t i = 1; i < TAG_UID_SIZE - 1; i++)
            {
                uid[i] = static_cast<uint8_t>(random(0, 0x100));
            }
            uid[TAG_UID_SIZE - 1] = 0x80;
            setUID(uid);
        }

        /**
         * @brief (Re)writes the fixed page 0 bytes after the UID, and the figure block encoding `id`.
         * Must be called any time `id` changes.
         *
         */
        void commitToBuffer()
        {
            _buf[7] = 0x89;
            _buf[8] = 0x44;
            _buf[10] = 0xC2;

            uint8_t figureBlock[TAG_BLOCK_SIZE] = {
                static_cast<uint8_t>(random(0, 0x100)), 0x03, 0x00, 0x00, 0x01, 0x0E, 0x0A, 0x0A};
            Reader::writeUInt32LE(figureBlock, 8, id);
            Reader::writeUInt32LE(figureBlock, 12, figureCrc32(figureBlock));

            set(FIGURE_BLOCK_PAGE, figureBlock);
        }

        static constexpr uint8_t FIGURE_BLOCK_PAGE = 1;

        /**
         * @brief The CRC Kamen Rider figures store in bytes 12-15 of the figure block, covering bytes
         * 0-11. This is the standard reflected CRC-32 polynomial, but with an initial value of 0 and
         * no final XOR, so it doesn't match a stock CRC-32 implementation.
         *
         * @param block The 16-byte figure block (only the first 12 bytes are read)
         * @return uint32_t
         */
        static uint32_t figureCrc32(const uint8_t block[TAG_BLOCK_SIZE])
        {
            uint32_t crc = 0;
            for (uint8_t i = 0; i < 12; i++)
            {
                crc ^= block[i];
                for (uint8_t bit = 0; bit < 8; bit++)
                {
                    crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
                }
            }
            return crc;
        }
    };
}
