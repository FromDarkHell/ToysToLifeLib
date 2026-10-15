#pragma once

#include <Arduino.h>
#include "tags/mifare_s20.h"

using namespace Tags;

namespace DisneyInfinity
{
    /**
     * @brief A Disney Infinity figure/playset piece.
     *
     */
    class Figure : public MIFARE_S20
    {
    public:
        /**
         * @brief An ID describing the figure in the JSON database of characters/playsets/discs
         *
         */
        uint32_t id;

        /// @brief Whether a figure is currently placed on the pad.
        bool present;

        /// @brief The order this figure was first placed in. Used for simple addressing
        // 0xFF means it was not placed.
        uint8_t orderAdded;

        Figure() : id(0), present(false), orderAdded(0xFF) { randomizeUID(); }

        explicit Figure(uint32_t figureId) : id(figureId), present(false), orderAdded(0xFF)
        {
            randomizeUID();
        }

        /// @brief Maps a game-side block index to its actual offset in the storage buffer.
        static uint8_t toStorageBlock(uint8_t block) { return block == 0 ? 1 : block * 4; }

        /// @brief Reads a 16-byte block (see `toStorageBlock`).
        /// @param block The game-side block index from a READ command
        /// @param out A 16-byte output buffer
        /// @return true if the block was within bounds and copied out
        bool getBlock(uint8_t block, uint8_t *out) const
        {
            const uint8_t storageBlock = toStorageBlock(block);
            if (storageBlock >= PAGE_COUNT)
            {
                return false;
            }

            memcpy(out, _buf + (storageBlock * PAGE_SIZE), PAGE_SIZE);
            return true;
        }

        /// @brief Writes a 16-byte block (see `toStorageBlock`).
        /// @param block The game-side block index from a WRITE command
        /// @param data The 16 bytes of block data to write
        /// @return true if the block was within bounds and written
        bool setBlock(uint8_t block, const uint8_t *data)
        {
            const uint8_t storageBlock = toStorageBlock(block);
            if (storageBlock >= PAGE_COUNT)
            {
                return false;
            }

            memcpy(_buf + (storageBlock * PAGE_SIZE), data, PAGE_SIZE);
            return true;
        }
    };
}
