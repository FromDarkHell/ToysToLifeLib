#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <constants.h>

using namespace ToysToLifeLib;

namespace LEGODimensions
{
    constexpr int PLAYPAD_INTERFACE = 0x0000;

    constexpr int PLAYPAD_WRITE_EP = 0x0001;
    constexpr int PLAYPAD_READ_EP = 0x0081;

    constexpr int PLAYPAD_MAX_PACKET_SIZE = 0x0020;
    constexpr int PLAYPAD_DELAY_MS = 100;

    constexpr USBIdentifier PS3Identifier = {
        0x0E6F, 0x0241};
    constexpr USBIdentifier X360Identifier = {
        0x24C6, 0xFA01};

    /**
     * @brief A byte enum defining whether or not a packet is a command (aka sent by the game -> pad
     * *or* an "event" which is pad -> game)
     *
     */
    enum class PacketType : uint8_t
    {
        COMMAND = 0x55,
        EVENT = 0x56,
    };

    /**
     * @brief An enum which describes all of the command types
     *
     */
    enum class GatewayCommand : uint8_t
    {
        // A WAKE packet is used to start the handshaking process with a playpad
        WAKE = 0xB0,
        // A SEED packet is the next step in the handshaking process, and involves setting the PRNG to
        // the correct seed
        SEED = 0xB1,
        // A CHALLENGE packet is the 3rd & final step and involves checking if the PRNG implementation
        // is correct, in addition to the encryption.
        CHALLENGE = 0xB3,

        // Sets the current color of the specifically indexed pad
        COL = 0xC0,
        // Gets the current color of the specific pad
        GETCOL = 0xC1,
        // Fades the specific pad to a specific color over x amount of ticks and cycles it y amount of
        // times
        FADE = 0xC2,
        // Flashes the specific pad back and forth from a specific color over x amount of ticks and
        // cycles it y amount of times
        FLASH = 0xC3,

        FADRD = 0xC4, // TODO: Needs implemented

        // Fades all pads in a single command to a specific color over x amount of ticks and cycles it y
        // amount of times
        FADAL = 0xC6,
        // Flashes all pads in a single command to a specific color over x amount of ticks and cycles it
        // y amount of times

        FLSAL = 0xC7,
        // Colors all pads in a single command to a specific color

        COLAL = 0xC8,

        // Returns an array of bitfields of all of the current tag locations and their indexes
        TGLST = 0xD0,

        // Reads NFC data from the NFCTag
        READ = 0xD2,
        // Writes NFC data onto the NFC tag
        WRITE = 0xD3,
        // The MODEL command gets extra info (IDs) from the specific NFC tag at a given index
        MODEL = 0xD4,

        // The playpad itself automatically sets/changes the PWD for a given NFC tag.
        // This command lets you turn on/off the PWD.
        // See: https://github.com/AlinaNova21/node-ld/blob/master/src/lib/ToyPad.js
        PWD = 0xE1,

        // This command lets you turn on/off all NFC operations until another read/write/active command
        // is called
        ACTIVE = 0xE5, // TODO: Needs implemented

        // ???
        LEDSEQ = 0xFF,
    };

    /**
     * @brief Describes the current state of a pad (solid/fading/flashing)
     *
     */
    enum class PadDisplayMode : uint8_t
    {
        SOLID = 0x00,
        FADE = 0x01,
        FLASH = 0x02,
    };

    /**
     * @brief A struct defining the current fading state of a given pad
     *
     */
    struct PadFadeState
    {
        PadColor target;
        uint8_t speed;
        uint8_t count; // 0x00 = infinite
    };

    /**
     * @brief A struct defining the current flash state of a given pad
     *
     */
    struct PadFlashState
    {
        PadColor offColor;
        uint8_t onTime;
        uint8_t offTime;
        uint8_t count; // 0x00 = infinite
    };

    /**
     * @brief An enum detailing the specific types that a toytag can be
     *
     */
    enum class ToyTagType : uint8_t
    {
        Unknown = 0,
        Character = 1,
        Vehicle = 2
    };

    /**
     * @brief An enum detailing all of the locations/values that a tag can be on the playpads
     *
     */
    enum class TagIndex : int8_t
    {
        INVALID = 0x00,

        TopLeft = 0x01,
        Middle = 0x02,
        TopRight = 0x03,

        BottomLeft = 0x04,
        CenterLeft = 0x05,
        CenterRight = 0x06,
        BottomRight = 0x07,

        Unplaced = -1
    };

    static ToypadPlatform getToypadPlatformFromIDs(USBIdentifier identity)
    {
        if (identity == PS3Identifier)
            return ToypadPlatform::PS3;
        if (identity == X360Identifier)
            return ToypadPlatform::X360;

        return ToypadPlatform::UNK;
    }

    static uint8_t TEA_KEY[16] = {0x55, 0xFE, 0xF6, 0xB0, 0x62, 0xBF, 0x0B, 0x41,
                                  0xC9, 0xB3, 0x7C, 0xB4, 0x97, 0x3E, 0x29, 0x7B};
};