#include "infinity/packet.h"
#include "crypto/infinity_prng.h"

namespace DisneyInfinity
{
    /**
     * @brief A WAKE packet is the first step of the handshaking process, sent by the game to the
     * portal to kick things off.
     * Disney Infinity scrambles/descrambles values bit-by-bit (see InfinityPRNG).
     *
     */
    struct WakePacket : CommandPacket
    {
        static constexpr const char *PAYLOAD = "(c) Disney 2013";
        static constexpr uint8_t PAYLOAD_SIZE = 15;

        /// @brief Builds a WAKE command packet with the fixed payload.
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

        /// @brief Builds the fixed WAKE acknowledgement
        ///
        /// I've got no clue what the deal here is, but it's snagged from an actual capture so *shrug*
        /// @return ResponsePacket
        static ResponsePacket buildResponse()
        {
            static constexpr uint8_t PAYLOAD[] = {
                0x00, 0x0f, 0x01, 0x00, 0x03, 0x02, 0x09, 0x09,
                0x43, 0x20, 0x32, 0x62, 0x36, 0x36, 0x4b, 0x34,
                0x99, 0x67, 0x31, 0x93};
            return ResponsePacket::build(0x00, PAYLOAD, sizeof(PAYLOAD));
        }
    };

    /**
     * @brief A SEED packet seeds the PRNG that CHALLENGE uses to authenticate.
     * Its 8-byte payload is a scrambled 32-bit seed (@see InfinityPRNG::descramble)
     *
     */
    struct SeedPacket : CommandPacket
    {
        static constexpr uint8_t PAYLOAD_SIZE = 8;

        /// @brief Parses and descrambles an incoming SEED command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return Descrambled seed value, ready to hand to InfinityPRNG::init
        static uint32_t fromCommand(const CommandPacket &packet)
        {
            const uint8_t *payload = packet.payload();

            uint64_t scrambled = 0;
            for (int i = 0; i < PAYLOAD_SIZE; i++)
            {
                scrambled = (scrambled << 8) | payload[i];
            }

            return InfinityPRNG::descramble(scrambled);
        }

        /// @brief Builds a scrambled SEED command to send to the device.
        /// @param cid CID to send this command with
        /// @param seed PRNG seed value
        /// @param garbage Filler bits interleaved with the seed's bits when scrambling
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint32_t seed, uint32_t garbage = 0)
        {
            const uint64_t scrambled = InfinityPRNG::scramble(seed, garbage);

            uint8_t payload[PAYLOAD_SIZE];
            for (int i = 0; i < PAYLOAD_SIZE; i++)
            {
                payload[i] = static_cast<uint8_t>(scrambled >> ((PAYLOAD_SIZE - 1 - i) * 8));
            }

            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::SEED), cid, payload, PAYLOAD_SIZE);
        }

        /// @brief Builds the blank acknowledgement sent back after a SEED command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid) { return ResponsePacket::blank(cid); }
    };

    /**
     * @brief A CHALLENGE packet works by advancing the PRNG, and then returning the next value (scrambled, of course)
     */
    struct ChallengePacket : CommandPacket
    {
        static constexpr uint8_t RESPONSE_SIZE = 8;

        /// @brief Builds a CHALLENGE command to send to the device. Carries no payload.
        /// @param cid CID to send this command with
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid)
        {
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::CHALLENGE), cid);
        }

        /// @brief Builds the scrambled CHALLENGE response
        /// @param cid CID mirrored from the incoming command
        /// @param nextRandom The next value produced by the seeded PRNG
        /// @param garbage Filler bits interleaved with the value's bits when scrambling
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid, uint32_t nextRandom, uint32_t garbage = 0)
        {
            const uint64_t scrambled = InfinityPRNG::scramble(nextRandom, garbage);

            uint8_t payload[RESPONSE_SIZE];
            for (int i = 0; i < RESPONSE_SIZE; i++)
            {
                payload[i] = static_cast<uint8_t>(scrambled >> ((RESPONSE_SIZE - 1 - i) * 8));
            }

            return ResponsePacket::build(cid, payload, RESPONSE_SIZE);
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

        /// @brief Builds the blank acknowledgement sent back after a COL command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid) { return ResponsePacket::blank(cid); }
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

        /// @brief Builds the blank acknowledgement sent back after a FLASH command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid) { return ResponsePacket::blank(cid); }
    };

    /**
     * @brief A FLSAL packet lets you flash all three pads in a single packet, each with its own
     * enable/color/rate.
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

        /// @brief Builds the blank acknowledgement sent back after a FLSAL command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid) { return ResponsePacket::blank(cid); }
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

        /// @brief Builds the blank acknowledgement sent back after a FADE command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid) { return ResponsePacket::blank(cid); }
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

        /// @brief Builds the blank acknowledgement sent back after a FADAL command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid) { return ResponsePacket::blank(cid); }
    };

    /**
     * @brief A UNK95 (0x95) packet is a mystery packet, I don't know what it does at all lol
     *
     */
    struct Unk95Packet : CommandPacket
    {
        static constexpr uint8_t PAYLOAD_SIZE = 12;
        static constexpr uint8_t PAYLOAD[PAYLOAD_SIZE] = {
            0x60, 0x00, 0xF5, 0x00, 0x30, 0x01, 0x60, 0x3A, 0x48, 0xC8, 0x30, 0x01};

        /// @brief Builds a UNK95 command carrying the fixed literal payload.
        /// @param cid CID to send this command with
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid)
        {
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::UNK95), cid, PAYLOAD, PAYLOAD_SIZE);
        }

        /// @brief Checks an incoming UNK95 command carries the expected fixed literal payload.
        /// @param packet Raw CommandPacket received from the device
        /// @return true if the payload matches the known literal
        static bool fromCommand(const CommandPacket &packet)
        {
            return packet.payloadSize() == PAYLOAD_SIZE &&
                   memcmp(packet.payload(), PAYLOAD, PAYLOAD_SIZE) == 0;
        }

        /// @brief Builds the blank acknowledgement sent back after a UNK95 command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid) { return ResponsePacket::blank(cid); }
    };

    /**
     * @brief A GETTOYS packet asks for a list of every figure currently placed on the pad, along
     * with which pad section it's on. This mirrors LEGO Dimensions' TGLST.
     *
     */
    struct GetToysPacket : CommandPacket
    {
        /**
         * @brief A FigureEntry is a bitfield (single byte) of the figure's insertion order as well
         * as its location (PadLocation values).
         * The high nibble (bits 4-7) are the location.
         * The low nibble (bits 0-3) are the insertion order.
         *
         */
        struct FigureEntry
        {
            uint8_t orderAdded;
            PadLocation location;

            uint8_t toByte() const
            {
                return (((static_cast<uint8_t>(location) << 4) & 0b11110000) | (orderAdded & 0b00001111));
            }
        };

        /// @brief Builds a GETTOYS command requesting the current figure list. Carries no payload.
        /// @param cid CID to send this command with
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid)
        {
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::GETTOYS), cid);
        }

        /**
         * @brief Creates a response packet based on the current FigureEntry list
         *
         * @param cid Command ID to return in the response packet
         * @param figures An array of currently-placed figures (up to 9 at most)
         * @param figureCount The # of figures in the array
         * @return ResponsePacket
         */
        static ResponsePacket buildResponse(uint8_t cid, const FigureEntry *figures, size_t figureCount)
        {
            static constexpr uint8_t MAX_PAYLOAD = (9 * 2); // 18 bytes

            uint8_t payload[MAX_PAYLOAD]{};
            const uint8_t payloadLen = static_cast<uint8_t>(figureCount * 2);

            for (size_t i = 0; i < figureCount; i++)
            {
                payload[(i * 2)] = figures[i].toByte();
                payload[(i * 2) + 1] = 0x09;
            }

            return ResponsePacket::build(cid, payload, payloadLen);
        }
    };

    /**
     * @brief A READ packet requests a one 16-byte block of a figure's data at a given
     * block index and figure index.
     *
     */
    struct ReadPacket : CommandPacket
    {
        static constexpr uint8_t DATA_SIZE = 16;
        static constexpr uint8_t RESPONSE_SIZE = 1 + DATA_SIZE; // status + data

        struct ReadRequest
        {
            uint8_t figureIndex; // figure's insertion order, as reported by GETTOYS
            uint8_t block;       // game-side block index
        };

        /// @brief Parses an incoming READ command.
        static ReadRequest fromCommand(const CommandPacket &packet)
        {
            return {packet.payload()[0], packet.payload()[1]};
        }

        /// @brief Builds a READ command requesting a figure's data at a given block.
        /// @param cid CID to send this command with
        /// @param figureIndex The figure's insertion order, as reported by GETTOYS
        /// @param block Game-side block index to read
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint8_t figureIndex, uint8_t block)
        {
            uint8_t payload[2] = {figureIndex, block};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::READ), cid, payload, sizeof(payload));
        }

        /// @brief Builds a successful READ response carrying one figure block.
        /// @param cid CID mirrored from the incoming command
        /// @param data 16 bytes of figure data at the requested block
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid, const uint8_t data[DATA_SIZE])
        {
            uint8_t payload[RESPONSE_SIZE]{};
            payload[0] = 0x00;
            memcpy(&payload[1], data, DATA_SIZE);

            return ResponsePacket::build(cid, payload, RESPONSE_SIZE);
        }
    };

    /**
     * @brief A WRITE packet writes one 16-byte block to a figure's data at a given (game-side) block
     * index. A WRITE payload is `[figureIndex][block][pad][data]`.
     *
     * The response is always a single status byte of `0x00`.
     *
     */
    struct WritePacket : CommandPacket
    {
        static constexpr uint8_t DATA_SIZE = 16;

        struct WriteRequest
        {
            uint8_t figureIndex;
            uint8_t block;
            uint8_t data[DATA_SIZE];
        };

        /// @brief Parses an incoming WRITE command.
        static WriteRequest fromCommand(const CommandPacket &packet)
        {
            WriteRequest req{};
            req.figureIndex = packet.payload()[0];
            req.block = packet.payload()[1];
            // payload[2] is a reserved byte
            memcpy(req.data, packet.payload() + 3, DATA_SIZE);
            return req;
        }

        /// @brief Builds a WRITE command that writes one block to a figure at a given index.
        /// @param cid CID to send this command with
        /// @param figureIndex The figure's insertion order, as reported by GETTOYS
        /// @param block Game-side block index to write
        /// @param data The 16 bytes of block data to write
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint8_t figureIndex, uint8_t block, const uint8_t data[DATA_SIZE])
        {
            uint8_t payload[3 + DATA_SIZE]{};
            payload[0] = figureIndex;
            payload[1] = block;
            memcpy(&payload[3], data, DATA_SIZE);

            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::WRITE), cid, payload, sizeof(payload));
        }

        /// @brief Returns a blank status response for when a WRITE succeeds.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid)
        {
            uint8_t status = 0x00;
            return ResponsePacket::build(cid, &status, 1);
        }
    };

    /**
     * @brief A MODEL packet asks for the raw 7-byte UID of the figure at a given index.
     */
    struct ModelPacket : CommandPacket
    {
        static constexpr uint8_t UID_SIZE = 7;
        static constexpr uint8_t RESPONSE_SIZE = 1 + UID_SIZE; // status + UID

        /// @brief Parses an incoming MODEL command.
        /// @param packet Raw CommandPacket received from the device
        /// @return The figure index being queried
        static uint8_t fromCommand(const CommandPacket &packet) { return packet.payload()[0]; }

        /// @brief Builds a MODEL command requesting a figure's UID at a given index.
        /// @param cid CID to send this command with
        /// @param figureIndex The figure's insertion order, as reported by GETTOYS
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint8_t figureIndex)
        {
            uint8_t payload[1] = {figureIndex};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::MODEL), cid, payload, sizeof(payload));
        }

        /// @brief Builds a successful MODEL response carrying the figure's UID.
        /// @param cid CID mirrored from the incoming command
        /// @param uid The figure's 7-byte UID
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid, const uint8_t uid[UID_SIZE])
        {
            uint8_t payload[RESPONSE_SIZE]{};
            payload[0] = 0x00;
            memcpy(&payload[1], uid, UID_SIZE);

            return ResponsePacket::build(cid, payload, RESPONSE_SIZE);
        }
    };

    /**
     * @brief A FigureEventPacket is used to notify the game when a figure moves on/off of the pad.
     *
     */
    struct FigureEventPacket : EventPacket
    {
        /**
         * @brief An enum used to specify which way a given figure has moved on the pad
         *
         */
        enum FigureMovementDirection : uint8_t
        {
            PLACED = 0x00,
            REMOVED = 0x01
        };

        /// @brief Builds an event packet identifying that a figure has moved to/from this location.
        /// @param location The figure's current (or last) location
        /// @param orderAdded The figure's insertion order, assigned the first time it's placed
        /// @param direction Whether the figure was placed or removed
        /// @return EventPacket
        static EventPacket build(PadLocation location, uint8_t orderAdded, FigureMovementDirection direction)
        {
            EventPacket pkt;
            pkt.data[0] = static_cast<uint8_t>(PacketType::EVENT);
            pkt.data[1] = 0x04; // Payload Length

            pkt.data[2] = static_cast<uint8_t>(location);
            pkt.data[3] = 0x09; // Always 0x09
            pkt.data[4] = orderAdded;
            pkt.data[5] = static_cast<uint8_t>(direction) & 0x01;
            pkt.data[6] = pkt.computeChecksum();

            return pkt;
        }
    };
}
