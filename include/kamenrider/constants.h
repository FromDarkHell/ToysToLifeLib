#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <constants.h>
#include <tags/mifare_s20.h>

using namespace ToysToLifeLib;

namespace KamenRider
{
    constexpr int PLAYPAD_INTERFACE = 0x0000;

    constexpr int PLAYPAD_WRITE_EP = 0x0001;
    constexpr int PLAYPAD_READ_EP = 0x0081;

    constexpr int PLAYPAD_MAX_PACKET_SIZE = 0x0020;
    constexpr int PLAYPAD_DELAY_MS = 100;

    constexpr USBIdentifier PS3Identifier = {0x0E6F, 0x200A};

    /// @brief Size of a tag's UID, which READ/WRITE/TGLST/events all use to identify a tag.
    constexpr uint8_t TAG_UID_SIZE = 7;

    /// @brief Size of one addressable block of tag data.
    constexpr uint8_t TAG_BLOCK_SIZE = Tags::MIFARE_S20::PAGE_SIZE;

    /// @brief Tag data is addressed as `sector` (0-4) + `block` (0-3), 20 blocks in all, which lines
    /// up with a MIFARE_S20 page index of `sector * TAG_BLOCKS_PER_SECTOR + block`.
    constexpr uint8_t TAG_SECTOR_COUNT = 5;
    constexpr uint8_t TAG_BLOCKS_PER_SECTOR = 4;

    /// @brief TGLST entries and tag events both mark a tag UID with this byte.
    constexpr uint8_t TAG_ENTRY_MARKER = 0x09;

    /**
     * @brief This is directly yoinked from LEGO Dimensions lol
     *
     */
    enum class PacketType : uint8_t
    {
        COMMAND = 0x55,
        EVENT = 0x56,
    };

    /**
     * @brief An enum which describes all of the command types.
     * This is functionally, just a smaller version of LEGO Dimensions.
     * It's missing simple things like SEED/etc
     */
    enum class GatewayCommand : uint8_t
    {
        // A WAKE packet is used to start the handshaking process with a playpad
        WAKE = 0xB0,

        // Sets the gate's color. Payload is `[r][g][b]`, with no pad index.
        COL = 0xC0,

        // TODO: Check if GETCOL or FADE are implemented on a Kamen Rider pad.

        // Flashes the gate back and forth from a specific color over x amount of ticks and cycles it
        // y amount of times. Payload layout is inferred from a single capture (see FlashPacket).
        FLASH = 0xC3,

        // Returns the UIDs of every tag currently placed on the gate
        TGLST = 0xD0,

        // Reads one 16-byte block from the tag with a given UID
        READ = 0xD2,

        // Writes one 16-byte block to the tag with a given UID
        WRITE = 0xD3,

        // The MODEL command gets extra info (IDs) from the specific NFC tag at a given index.
        // Unconfirmed - RPCS3 doesn't handle this command.
        MODEL = 0xD4,
    };

    static ToypadPlatform getToypadPlatformFromIDs(USBIdentifier identity)
    {
        if (identity == PS3Identifier)
            return ToypadPlatform::PS3;

        return ToypadPlatform::UNK;
    }
}