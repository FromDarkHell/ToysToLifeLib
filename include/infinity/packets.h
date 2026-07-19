#include "infinity/packet.h"

namespace DisneyInfinity
{
    /**
     * @brief A WAKE packet is the first (and only) step of the handshaking process, sent by the game
     * to the portal to kick things off. Unlike LEGO Dimensions, there's no SEED/CHALLENGE step
     * afterwards since Disney Infinity doesn't encrypt anything. Its payload is always the fixed
     * ASCII string "(c) Disney 2013".
     *
     */
    struct WakePacket : CommandPacket
    {
        static constexpr const char *PAYLOAD = "(c) Disney 2013";
        static constexpr uint8_t PAYLOAD_SIZE = 15;

        /// @brief Builds a WAKE command packet with the fixed "(c) Disney 2013" ASCII payload.
        /// @param cid CID to send this command with
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid)
        {
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::WAKE), cid,
                                        reinterpret_cast<const uint8_t *>(PAYLOAD), PAYLOAD_SIZE);
        }

        /// @brief Parses an incoming WAKE command and checks it carries the expected payload.
        /// @param packet Raw CommandPacket received from the device
        /// @return true if the payload matches the expected string
        static bool fromCommand(const CommandPacket &packet)
        {
            return packet.payloadSize() == PAYLOAD_SIZE &&
                   memcmp(packet.payload(), PAYLOAD, PAYLOAD_SIZE) == 0;
        }
    };

    /**
     * @brief A COL packet changes the color of a specific pad.
     *
     */
    struct ColorPacket : CommandPacket
    {
        // Parsed contents of a COL command payload
        struct ColorStatus
        {
            PadLocation padLocation;
            PadColor padColor;
        };

        /// @brief Parses an incoming COL command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed COL command
        static ColorStatus fromCommand(const CommandPacket &packet)
        {
            const uint8_t *decrypted = packet.payload();

            const PadLocation padIndex = static_cast<PadLocation>(decrypted[0]);
            const PadColor padColor = PadColor::fromBuffer(&decrypted[1]);

            return {padIndex, padColor};
        }

        /// @brief Builds a COL command that sets a single pad to a specific color.
        /// @param cid CID to send this command with
        /// @param location Which pad to color
        /// @param color The color to set the pad to
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, PadLocation location, PadColor color)
        {
            uint8_t payload[4] = {static_cast<uint8_t>(location), color.r, color.g, color.b};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::COL), cid, payload, sizeof(payload));
        }
    };

    /**
     * @brief A FLASH packet lets you flash a specific pad to the current color for `onTicks` ticks,
     * and then flash to `offColor` for `offTicks` ticks. It does this cycle `count` number of times.
     *
     */
    struct FlashPacket : CommandPacket
    {
        struct FlashStatus
        {
            PadLocation padLocation;
            uint8_t onTicks;
            uint8_t offTicks;
            uint8_t count;

            PadColor offColor;
        };

        /// @brief Parses an incoming FLASH command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed FLASH command
        static FlashStatus fromCommand(const CommandPacket &packet)
        {
            const uint8_t *decrypted = packet.payload();

            const PadLocation padIndex = static_cast<PadLocation>(decrypted[0]);
            const PadColor padColor = PadColor::fromBuffer(&decrypted[4]);

            return {padIndex, decrypted[1], decrypted[2], decrypted[3], padColor};
        }

        /// @brief Builds a FLASH command that flashes a single pad.
        /// @param cid CID to send this command with
        /// @param location Which pad to flash
        /// @param onTicks How many ticks to stay on the current color
        /// @param offTicks How many ticks to stay on offColor
        /// @param count Number of cycles to flash (0x00 = infinite)
        /// @param offColor The color to flash to
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, PadLocation location, uint8_t onTicks,
                                   uint8_t offTicks, uint8_t count, PadColor offColor)
        {
            uint8_t payload[7] = {static_cast<uint8_t>(location), onTicks, offTicks, count,
                                  offColor.r, offColor.g, offColor.b};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::FLASH), cid, payload, sizeof(payload));
        }
    };

    /**
     * @brief A FLSAL packet lets you flash all three pads in a single packet, each with its own
     * enable/color/rate. This mirrors LEGO Dimensions' FLSHALL, not its FLSAL (there's no
     * per-location-byte variant in Disney Infinity).
     *
     */
    struct FlashAllPacket : CommandPacket
    {
        struct FlashStatus
        {
            struct PadFlashStatus
            {
                uint8_t enable;
                uint8_t onTicks;
                uint8_t offTicks;
                uint8_t count;

                PadColor offColor;
            };

            PadFlashStatus centerPad;
            PadFlashStatus leftPad;
            PadFlashStatus rightPad;
        };

        /// @brief Parses an incoming FLSAL command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed FLSAL command
        static FlashStatus fromCommand(const CommandPacket &packet)
        {
            const uint8_t *decrypted = packet.payload();
            FlashStatus result;

            for (int i = 0; i < 3; i++)
            {
                int offset = (i * 7);
                const PadColor padColor = PadColor::fromBuffer(&decrypted[offset + 4]);

                FlashStatus::PadFlashStatus flashStatus = {decrypted[offset + 0], decrypted[offset + 1],
                                                           decrypted[offset + 2], decrypted[offset + 3],
                                                           padColor};

                if (i == 0)
                {
                    result.centerPad = flashStatus;
                }
                else if (i == 1)
                {
                    result.leftPad = flashStatus;
                }
                else if (i == 2)
                {
                    result.rightPad = flashStatus;
                }
            }

            return result;
        }

        /// @brief Builds a FLSAL command that flashes all three pads in one packet.
        /// @param cid CID to send this command with
        /// @param centerPad Flash state for the center pad
        /// @param leftPad Flash state for the left pad
        /// @param rightPad Flash state for the right pad
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, const FlashStatus::PadFlashStatus &centerPad,
                                   const FlashStatus::PadFlashStatus &leftPad,
                                   const FlashStatus::PadFlashStatus &rightPad)
        {
            uint8_t payload[21];
            const FlashStatus::PadFlashStatus *pads[3] = {&centerPad, &leftPad, &rightPad};

            for (int i = 0; i < 3; i++)
            {
                const int offset = i * 7;
                payload[offset + 0] = pads[i]->enable;
                payload[offset + 1] = pads[i]->onTicks;
                payload[offset + 2] = pads[i]->offTicks;
                payload[offset + 3] = pads[i]->count;
                payload[offset + 4] = pads[i]->offColor.r;
                payload[offset + 5] = pads[i]->offColor.g;
                payload[offset + 6] = pads[i]->offColor.b;
            }

            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::FLSAL), cid, payload, sizeof(payload));
        }
    };

    /**
     * @brief A FADE packet lets you fade from the current color to the next color x amount of times.
     *
     */
    struct FadePacket : CommandPacket
    {
        struct FadeStatus
        {
            PadLocation padLocation;
            uint8_t speed;
            uint8_t cycles;

            PadColor color;
        };

        /// @brief Parses an incoming FADE command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed FADE command
        static FadeStatus fromCommand(const CommandPacket &packet)
        {
            const uint8_t *decrypted = packet.payload();

            const PadLocation padIndex = static_cast<PadLocation>(decrypted[0]);
            const PadColor padColor = PadColor::fromBuffer(&decrypted[3]);

            return {padIndex, decrypted[1], decrypted[2], padColor};
        }

        /// @brief Builds a FADE command that fades a single pad to a color.
        /// @param cid CID to send this command with
        /// @param location Which pad to fade
        /// @param speed Fade speed
        /// @param cycles Number of cycles to fade (0x00 = infinite)
        /// @param color The color to fade to
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, PadLocation location, uint8_t speed, uint8_t cycles, PadColor color)
        {
            uint8_t payload[6] = {static_cast<uint8_t>(location), speed, cycles, color.r, color.g, color.b};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::FADE), cid, payload, sizeof(payload));
        }
    };

    /**
     * @brief A FADAL packet lets you fade all three pads in a single packet, each with its own
     * enable/color/rate.
     *
     */
    struct FadeAllPacket : CommandPacket
    {
        struct FadeStatus
        {
            struct PadFadeStatus
            {
                uint8_t enable;
                uint8_t speed;
                uint8_t cycles;

                PadColor color;
            };

            PadFadeStatus centerPad;
            PadFadeStatus leftPad;
            PadFadeStatus rightPad;
        };

        /// @brief Parses an incoming FADAL command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed FADAL command
        static FadeStatus fromCommand(const CommandPacket &packet)
        {
            const uint8_t *decrypted = packet.payload();
            FadeStatus result;

            for (int i = 0; i < 3; i++)
            {
                int offset = (i * 6);
                const PadColor padColor = PadColor::fromBuffer(&decrypted[offset + 3]);

                FadeStatus::PadFadeStatus fadeStatus = {decrypted[offset + 0], decrypted[offset + 1],
                                                        decrypted[offset + 2], padColor};

                if (i == 0)
                {
                    result.centerPad = fadeStatus;
                }
                else if (i == 1)
                {
                    result.leftPad = fadeStatus;
                }
                else if (i == 2)
                {
                    result.rightPad = fadeStatus;
                }
            }

            return result;
        }

        /// @brief Builds a FADAL command that fades all three pads in one packet.
        /// @param cid CID to send this command with
        /// @param centerPad Fade state for the center pad
        /// @param leftPad Fade state for the left pad
        /// @param rightPad Fade state for the right pad
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, const FadeStatus::PadFadeStatus &centerPad,
                                   const FadeStatus::PadFadeStatus &leftPad,
                                   const FadeStatus::PadFadeStatus &rightPad)
        {
            uint8_t payload[18];
            const FadeStatus::PadFadeStatus *pads[3] = {&centerPad, &leftPad, &rightPad};

            for (int i = 0; i < 3; i++)
            {
                const int offset = i * 6;
                payload[offset + 0] = pads[i]->enable;
                payload[offset + 1] = pads[i]->speed;
                payload[offset + 2] = pads[i]->cycles;
                payload[offset + 3] = pads[i]->color.r;
                payload[offset + 4] = pads[i]->color.g;
                payload[offset + 5] = pads[i]->color.b;
            }

            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::FADAL), cid, payload, sizeof(payload));
        }
    };
}
