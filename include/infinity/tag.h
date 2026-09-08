#pragma once

#include <Arduino.h>

namespace DisneyInfinity
{
    /**
     * @brief A Disney Infinity figure/playset piece.
     *
     * Infinity figures store their 320 bytes of NFC data as 20 fixed 16-byte blocks, and the game addresses them through a remapped block index rather than the raw block number
     * - block 0 (game-side) reads/writes storage block 1
     * - Every other block N reads/writes storage block `N * 4`.
     *
     * The figure's UID (used to answer MODEL requests) lives in the first 7 bytes of storage block 0,
     * which isn't editable by the game
     */
    class Figure
    {
    public:
        static const uint8_t BLOCK_SIZE = 16;
        static const uint8_t BLOCK_COUNT = 20;
        static const uint16_t DATA_SIZE = BLOCK_SIZE * BLOCK_COUNT; // 320

        uint8_t _buf[DATA_SIZE];

        /// @brief Whether a figure is currently placed on the pad.
        bool present;

        /// @brief The order this figure was first placed in. Used for simple addressing
        // 0xFF means it was not placed.
        uint8_t orderAdded;

        Figure() : present(false), orderAdded(0xFF) { memset(_buf, 0, DATA_SIZE); }

        /// @brief Gets the figure's 7-byte UID (stored in the unreachable storage block 0).
        void getUID(uint8_t out[7]) const { memcpy(out, _buf, 7); }

        /// @brief Sets the figure's 7-byte UID (stored in the unreachable storage block 0).
        void setUID(const uint8_t uid[7]) { memcpy(_buf, uid, 7); }

        /// @brief Maps a game-side block index to its actual offset in the storage buffer.
        static uint8_t toStorageBlock(uint8_t block) { return block == 0 ? 1 : block * 4; }

        /// @brief Reads a 16-byte block (see `toStorageBlock`).
        /// @param block The game-side block index from a READ command
        /// @param out A 16-byte output buffer
        /// @return true if the block was within bounds and copied out
        bool getBlock(uint8_t block, uint8_t *out) const
        {
            const uint8_t storageBlock = toStorageBlock(block);
            if (storageBlock >= BLOCK_COUNT)
            {
                return false;
            }

            memcpy(out, _buf + (storageBlock * BLOCK_SIZE), BLOCK_SIZE);
            return true;
        }

        /// @brief Writes a 16-byte block (see `toStorageBlock`).
        /// @param block The game-side block index from a WRITE command
        /// @param data The 16 bytes of block data to write
        /// @return true if the block was within bounds and written
        bool setBlock(uint8_t block, const uint8_t *data)
        {
            const uint8_t storageBlock = toStorageBlock(block);
            if (storageBlock >= BLOCK_COUNT)
            {
                return false;
            }

            memcpy(_buf + (storageBlock * BLOCK_SIZE), data, BLOCK_SIZE);
            return true;
        }
    };
}
