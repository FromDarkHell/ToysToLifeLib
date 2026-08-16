#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <constants.h>

using namespace ToysToLifeLib;

namespace DisneyInfinity
{
    constexpr int PLAYPAD_INTERFACE = 0x0000;

    constexpr int PLAYPAD_WRITE_EP = 0x0002;
    constexpr int PLAYPAD_READ_EP = 0x0081;

    constexpr int PLAYPAD_MAX_PACKET_SIZE = 0x0020;
    constexpr int PLAYPAD_DELAY_MS = 100;

    constexpr USBIdentifier PS3Identifier = {
        0x0E6F, 0x0129};
    constexpr USBIdentifier X360Identifier = {
        0x24C6, 0xFA00};

    /**
     * @brief Unlike LEGO Dimensions (which reuses 0x55 for both commands and responses), Disney
     * Infinity uses a distinct type byte for commands (0xFF), responses (0xAA), and events (0xAB).
     *
     */
    enum class PacketType : uint8_t
    {
        COMMAND = 0xFF,
        RESPONSE = 0xAA,
        EVENT = 0xAB,
    };

    /**
     * @brief An enum which describes all of the command types.
     *
     */
    enum class GatewayCommand : uint8_t
    {
        // A WAKE packet is used to start the handshaking process with a portal.
        WAKE = 0x80,

        // A SEED packet is the next step in the handshaking process, and involves setting the PRNG to
        // the correct seed
        SEED = 0x81,
        // A CHALLENGE packet is the 3rd & final step and involves checking if the PRNG implementation
        // is correct, in addition to the encryption.
        CHALLENGE = 0x83,

        // Sets the current color of the specifically indexed pad
        COL = 0x90,

        // Fades the specific pad to a specific color over x amount of ticks and cycles it y amount of
        // times
        FADE = 0x92,
        // Flashes the specific pad back and forth from a specific color over x amount of ticks and
        // cycles it y amount of times
        FLASH = 0x93,

        // Fades all pads in a single command to a specific color over x amount of ticks and cycles it y
        // amount of times
        FADAL = 0x96,
        // Flashes all pads in a single command to a specific color over x amount of ticks and cycles it
        // y amount of times
        FLSAL = 0x97,

        // Returns info about all of the currently placed figures
        GETTOYS = 0xA1,
        // Reads NFC data from a figure
        READ = 0xA2,
        // Writes NFC data onto a figure
        WRITE = 0xA3,

        // Gets the raw UID of a figure at a given index
        MODEL = 0xB4,
    };

    static ToypadPlatform getToypadPlatformFromIDs(USBIdentifier identity)
    {
        if (identity == PS3Identifier)
            return ToypadPlatform::PS3;
        if (identity == X360Identifier)
            return ToypadPlatform::X360;

        return ToypadPlatform::UNK;
    }
}