#include "lego/packet.h"
#include <tags/ntag213.h>

using namespace Tags;

namespace LEGODimensions
{
    /**
     * @brief A WAKE packet is the first step of the handshaking process, sent by the game to the
     * playpad to kick things off. Its payload is always the fixed ASCII string "(c) LEGO 2014".
     *
     */
    struct WakePacket : CommandPacket
    {
        static constexpr const char *PAYLOAD = "(c) LEGO 2014";
        static constexpr uint8_t PAYLOAD_SIZE = 13;

        /// @brief Builds a WAKE command packet with the fixed "(c) LEGO 2014" ASCII payload.
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
     * @brief A SEED packet is used to seed the PRNG, as well as a part of the authentication process.
     * The main payload (first 8 bytes) of the packet are encrypted via TEA.
     *
     */
    struct SeedPacket : CommandPacket
    {
        static constexpr uint8_t PAYLOAD_SIZE = 8;

        // Parsed/decrypted contents of a SEED command payload
        struct SeedStatus
        {
            uint32_t seed; // read as LE - burtle.init(seed)
            uint32_t conf; // read as BE - echoed back in response
        };

        /// @brief Parses and decrypts an incoming SEED command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @param crypto TEA instance with the shared key already set
        /// @return Decrypted SeedStatus, or {0,0} on failure
        static SeedStatus fromCommand(const CommandPacket &packet, TEA *crypto)
        {
            const uint8_t *decrypted = BasePacket::decryptPayload(&packet, crypto);
            if (!decrypted)
            {
                return {0, 0};
            }

            const uint32_t seed = Reader::readUInt32LE(decrypted, 0x00);
            const uint32_t conf = Reader::readUInt32BE(decrypted, 0x04);

            free((void *)decrypted);
            return {seed, conf};
        }

        /// @brief Builds an encrypted SEED response to send back to the device.
        ///        Response payload: [conf_u32be][0x00 x4] encrypted
        /// @param cid    CID mirrored from the incoming command
        /// @param status SeedStatus from fromCommand()
        /// @param crypto TEA instance with the shared key already set
        /// @return ResponsePacket
        static ResponsePacket fromStatus(uint8_t cid, const SeedStatus &status, TEA *crypto)
        {
            uint8_t payload[PAYLOAD_SIZE]{};
            Reader::writeUInt32BE(payload, 0x00, status.conf); // conf as BE at offset 0

            // Encrypt the full 8-byte block before building the packet
            uint8_t encrypted[PAYLOAD_SIZE]{};
            if (!crypto->encrypt(payload, encrypted))
            {
                return {};
            }

            ResponsePacket pkt = ResponsePacket::build(cid, encrypted, PAYLOAD_SIZE);
            return pkt;
        }

        /// @brief Builds an encrypted SEED command to send to the device.
        ///        Payload: [seed_u32le][conf_u32be] encrypted
        /// @param cid    CID to send this command with
        /// @param seed   PRNG seed value
        /// @param conf   Confirmation value the device is expected to echo back
        /// @param crypto TEA instance with the shared key already set
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint32_t seed, uint32_t conf, TEA *crypto)
        {
            uint8_t payload[PAYLOAD_SIZE]{};
            Reader::writeUInt32LE(payload, 0x00, seed);
            Reader::writeUInt32BE(payload, 0x04, conf);

            uint8_t encrypted[PAYLOAD_SIZE]{};
            if (!crypto->encrypt(payload, encrypted))
            {
                return {};
            }

            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::SEED), cid, encrypted, PAYLOAD_SIZE);
        }
    };

    /**
     * @brief A CHALLENGE packet is used to help authenticate that this pad is a "legal" playpad.
     * The packet payload comes encrypted through TEA, & needs to be decrypted. Afterwards, it includes
     * a uint32 `conf` key. The seeded RNG will generate a random number, and send it back as a
     * response. This proves both the PRNG is implemented correctly, as well as TEA.
     * So it *obviously* has to be an official playpad...
     *
     */
    struct ChallengePacket : CommandPacket
    {
        static constexpr uint8_t PAYLOAD_SIZE = 8;

        // Parsed/decrypted contents of a CHALLENGE command payload
        struct ChallengeStatus
        {
            uint32_t conf; // read as BE - echoed back in response
        };

        /// @brief Parses and decrypts an incoming SEED command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @param crypto TEA instance with the shared key already set
        /// @return Decrypted ChallengeStatus, or {0xFFFFFFFF} on failure
        static ChallengeStatus fromCommand(const CommandPacket &packet, TEA *crypto)
        {
            const uint8_t *decrypted = BasePacket::decryptPayload(&packet, crypto);
            if (!decrypted)
            {
                return {0xFFFFFFFF};
            }

            const uint32_t conf = Reader::readUInt32BE(decrypted, 0x00);

            free((void *)decrypted);
            return {conf};
        }

        /// @brief Builds an encrypted SEED response to send back to the device.
        ///        Response payload: [conf_u32be][0x00 x4] encrypted
        /// @param cid    CID mirrored from the incoming command
        /// @param rand   A random PRNG number generated from Burtle
        /// @param status ChallengeStatus from fromCommand()
        /// @param crypto TEA instance with the shared key already set
        /// @return ResponsePacket
        static ResponsePacket fromStatus(uint8_t cid,
                                         uint32_t rand,
                                         const ChallengeStatus &status,
                                         TEA *crypto)
        {
            uint8_t payload[PAYLOAD_SIZE]{};
            Reader::writeUInt32LE(payload, 0x00, rand);
            Reader::writeUInt32BE(payload, 0x04, status.conf);

            // Encrypt the full 8-byte block before building the packet
            uint8_t encrypted[PAYLOAD_SIZE]{};
            if (!crypto->encrypt(payload, encrypted))
            {
                return {};
            }

            ResponsePacket pkt = ResponsePacket::build(cid, encrypted, PAYLOAD_SIZE);
            return pkt;
        }

        /// @brief Builds an encrypted CHALLENGE command to send to the device.
        ///        Payload: [conf_u32be][0x00 x4] encrypted
        /// @param cid    CID to send this command with
        /// @param conf   Confirmation value the device is expected to echo back
        /// @param crypto TEA instance with the shared key already set
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint32_t conf, TEA *crypto)
        {
            uint8_t payload[PAYLOAD_SIZE]{};
            Reader::writeUInt32BE(payload, 0x00, conf);

            uint8_t encrypted[PAYLOAD_SIZE]{};
            if (!crypto->encrypt(payload, encrypted))
            {
                return {};
            }

            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::CHALLENGE), cid, encrypted, PAYLOAD_SIZE);
        }
    };

    /**
     * @brief A COLOR packet changes the color of a specific playpad
     *
     */
    struct ColorPacket : CommandPacket
    {
        // Parsed/decrypted contents of a COLOR command payload
        struct ColorStatus
        {
            PadLocation padLocation;
            PadColor padColor;
        };

        /// @brief Parses an incoming COLOR command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed COLOR command
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
     * @brief A COLAL packet lets you change the colors of all of the specific playpads in a single
     * packet.
     *
     */
    struct ColorAllPacket : CommandPacket
    {
        // Parsed/decrypted contents of a COLORALL command payload
        struct ColorStatus
        {
            PadColor centerColor;
            PadColor leftColor;
            PadColor rightColor;
        };

        /// @brief Parses an incoming COLORALL command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed COLOR command
        static ColorStatus fromCommand(const CommandPacket &packet)
        {
            const uint8_t *decrypted = packet.payload();

            ColorStatus result{};
            for (int i = 0; i < 3; i++)
            {
                const int offset = i * 4;
                const PadLocation loc = static_cast<PadLocation>(decrypted[offset]);
                const PadColor color = PadColor::fromBuffer(&decrypted[offset + 1]);

                switch (loc)
                {
                case PadLocation::CENTER:
                    result.centerColor = color;
                    break;
                case PadLocation::LEFT:
                    result.leftColor = color;
                    break;
                case PadLocation::RIGHT:
                    result.rightColor = color;
                    break;
                default:
                    break;
                }
            }

            return result;
        }

        /// @brief Builds a COLAL command that sets the color of all three pads in one packet.
        /// @param cid CID to send this command with
        /// @param centerColor Color for the center pad
        /// @param leftColor Color for the left pad
        /// @param rightColor Color for the right pad
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, PadColor centerColor, PadColor leftColor, PadColor rightColor)
        {
            uint8_t payload[12] = {
                static_cast<uint8_t>(PadLocation::CENTER), centerColor.r, centerColor.g, centerColor.b,
                static_cast<uint8_t>(PadLocation::LEFT), leftColor.r, leftColor.g, leftColor.b,
                static_cast<uint8_t>(PadLocation::RIGHT), rightColor.r, rightColor.g, rightColor.b};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::COLAL), cid, payload, sizeof(payload));
        }
    };

    /**
     * @brief A FLSH packet lets you flash a specific playpad to the current color for `onTicks` ticks,
     * and then flash to `offColor` for `offTicks` ticks. It does this cycle `count` number of times.
     *
     * A tick is defined as ~100ms.
     *
     */
    struct FlashPacket : CommandPacket
    {
        // Parsed/decrypted contents of a CHALLENGE command payload
        struct FlashStatus
        {
            PadLocation padLocation;
            uint8_t onTicks;
            uint8_t offTicks;
            uint8_t count;

            PadColor offColor;
        };

        /// @brief Parses an incoming FLSH command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed FLSH command
        static FlashStatus fromCommand(const CommandPacket &packet)
        {
            const uint8_t *decrypted = packet.payload();

            const PadLocation padIndex = static_cast<PadLocation>(decrypted[0]);
            const PadColor padColor = PadColor::fromBuffer(&decrypted[4]);

            return {padIndex, decrypted[1], decrypted[2], decrypted[3],

                    padColor};
        }

        /// @brief Builds a FLSH command that flashes a single pad.
        /// @param cid CID to send this command with
        /// @param location Which pad to flash
        /// @param onTicks How many ticks (~100ms each) to stay on the current color
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
     * @brief A FLSHALL packet lets you flash all of the playpads in a single packet. This functions
     * equivalently to the FlashPacket, but with all of their data combined.
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

        /// @brief Parses an incoming FLSHALL command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed FLSHALL command
        static FlashStatus fromCommand(const CommandPacket &packet)
        {
            const uint8_t *decrypted = packet.payload();
            FlashStatus result;

            for (int i = 0; i < 3; i++)
            {
                int offset = (i * 7);
                const PadColor padColor = PadColor::fromBuffer(&decrypted[offset + 4]);

                // (EN: 01, ON_TICK: 0A, OFF_TICK: 14, COUNT: 0A, COLOR: FF0000)
                // (EN: 01, ON_TICK: 0A, OFF_TICK: 14, COUNT: 0F, COLOR: 00FF00)
                // (EN: 01, ON_TICK: 0A, OFF_TICK: 14, COUNT: 14, COLOR: 0000FF)
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
     * @brief A FADEALL packet lets you fade all of the playpads in a single packet. This functions
     * equivalently to the FadePacket, but with all of their data combined.
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

        /// @brief Parses an incoming FADEALL command from the device.
        /// @param packet Raw CommandPacket received from the device
        /// @return A parsed FADEALL command
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

    /**
     * @brief A GETCOL packet is used by the game to get the color of aspecific playpad
     *
     */
    struct GetColorPacket : CommandPacket
    {
        static PadLocation fromCommand(const CommandPacket &packet)
        {
            const uint8_t *decrypted = packet.payload();

            return static_cast<PadLocation>(decrypted[0]);
        }

        /// @brief Builds a GETCOL command requesting the current color of a pad.
        /// @param cid CID to send this command with
        /// @param location Which pad's color to request
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, PadLocation location)
        {
            uint8_t payload[1] = {static_cast<uint8_t>(location)};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::GETCOL), cid, payload, sizeof(payload));
        }

        static ResponsePacket buildResponse(uint8_t cid, PadColor padColor)
        {
            uint8_t payload[3]{padColor.r, padColor.g, padColor.b};

            return ResponsePacket::build(cid, payload, 3);
        }
    };

    /**
     * @brief A READ packet requests PAGES_PER_READ (4) pages of raw NFC data
     *        from the tag at the given token index, starting at the given page.
     *
     * Request payload  : [index][page]
     * Response payload : [status(0x00)][16 bytes of tag data]
     *                     where data = tag._buf[page*4 .. page*4+16]
     */
    struct ReadPacket : CommandPacket
    {
        struct ReadRequest
        {
            uint8_t index; // token index assigned at placement
            uint8_t page;  // first page to read (each page = 4 bytes)
        };

        struct ReadResponse
        {
            uint8_t status;   // 0x00 = OK, non-zero = error
            uint8_t data[16]; // PAGES_PER_READ(4) * PAGE_SIZE(4) bytes
        };

        static constexpr uint8_t RESPONSE_SIZE = 17; // 1 status + 16 data

        /// @brief Parses an incoming READ command.
        static ReadRequest fromCommand(const CommandPacket &packet)
        {
            return {
                packet.payload()[0], // index
                packet.payload()[1]  // page
            };
        }

        /// @brief Builds a READ command requesting a tag's data at a given index/page.
        /// @param cid CID to send this command with
        /// @param index Token index assigned at placement
        /// @param page First page to read (each page = 4 bytes)
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint8_t index, uint8_t page)
        {
            uint8_t payload[2] = {index, page};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::READ), cid, payload, sizeof(payload));
        }

        /// @brief Builds a successful READ response with 16 bytes of tag data.
        /// @param cid    CID mirrored from the incoming command
        /// @param tagBuf The full 180-byte NFC tag buffer
        /// @param page   Page number from the request
        static ResponsePacket buildResponse(uint8_t cid, const uint8_t *tagBuf, uint8_t page)
        {
            uint8_t payload[RESPONSE_SIZE]{};
            payload[0] = 0x00;

            const uint8_t byteOffset = page * NTAG213::PAGE_SIZE;

            // Don't read past the end of the tag buffer
            if (byteOffset + 16 <= NTAG213::TAG_SIZE)
            {
                memcpy(&payload[1], tagBuf + byteOffset, 16);
            }

            return ResponsePacket::build(cid, payload, RESPONSE_SIZE);
        }
    };

    /**
     * @brief A MODEL packet asks the emulator to identify the tag at a given
     *        token index. Both the request payload and the data portion of the
     *        response are TEA-encrypted.
     *
     * Request payload (encrypted, 8 bytes):
     *   [0]     index  - token index assigned at placement
     *   [1..3]  padding (ignored)
     *   [4..7]  conf   - u32 BE, echoed verbatim in the response
     *
     * Response payload (9 bytes):
     *   [0]     status - 0x00 OK / 0xF9 no id / 0xF2 not found
     *   [1..8]  TEA-encrypted block:
     *               [0..3] tag id as u32 LE  (0x00000000 if no id / not found)
     *               [4..7] conf as u32 BE    (always echoed)
     */
    struct ModelPacket : CommandPacket
    {
        static constexpr uint8_t PAYLOAD_SIZE = 8;
        static constexpr uint8_t RESPONSE_SIZE = 9; // 1 status + 8 encrypted

        static constexpr uint8_t STATUS_OK = 0x00;       // tag found, id returned
        static constexpr uint8_t STATUS_NO_ID = 0xF9;    // tag found but id == 0
        static constexpr uint8_t STATUS_NO_TOKEN = 0xF2; // no token at index

        struct ModelRequest
        {
            uint8_t index;
            uint32_t conf; // read as BE
        };

        /// @brief Decrypts and parses an incoming MODEL command.
        /// @param packet Raw CommandPacket from the host
        /// @param crypto TEA instance with the shared key set
        /// @return Parsed ModelRequest, or {0xFF, 0} on decryption failure
        static ModelRequest fromCommand(const CommandPacket &packet, TEA *crypto)
        {
            const uint8_t *decrypted = BasePacket::decryptPayload(&packet, crypto);
            if (!decrypted)
            {
                return {0xFF, 0};
            }

            const uint8_t index = decrypted[0];
            const uint32_t conf = Reader::readUInt32BE(decrypted, 4);

            free((void *)decrypted);
            return {index, conf};
        }

        /// @brief Builds an encrypted MODEL command requesting a tag's ID at a given index.
        /// @param cid    CID to send this command with
        /// @param index  Token index assigned at placement
        /// @param conf   Confirmation value the device is expected to echo back
        /// @param crypto TEA instance with the shared key already set
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint8_t index, uint32_t conf, TEA *crypto)
        {
            uint8_t payload[PAYLOAD_SIZE]{};
            payload[0] = index;
            Reader::writeUInt32BE(payload, 4, conf);

            uint8_t encrypted[PAYLOAD_SIZE]{};
            if (!crypto->encrypt(payload, encrypted))
            {
                return {};
            }

            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::MODEL), cid, encrypted, PAYLOAD_SIZE);
        }

        /// @brief Builds an encrypted MODEL response.
        /// @param cid    CID mirrored from the incoming command
        /// @param tagId  The tag's character/vehicle id (0 if not found)
        /// @param conf   conf value echoed from the request
        /// @param status One of STATUS_OK / STATUS_NO_ID / STATUS_NO_TOKEN
        /// @param crypto TEA instance with the shared key set
        static ResponsePacket buildResponse(uint8_t cid,
                                            uint16_t tagId,
                                            uint32_t conf,
                                            uint8_t status,
                                            TEA *crypto)
        {
            // Build the 8-byte plaintext block
            uint8_t plain[PAYLOAD_SIZE]{};
            Reader::writeUInt32LE(plain, 0, tagId); // id as LE at [0..3]
            Reader::writeUInt32BE(plain, 4, conf);  // conf as BE at [4..7]

            // Encrypted data is always written regardless of status
            uint8_t encrypted[PAYLOAD_SIZE]{};
            crypto->encrypt(plain, encrypted);

            // Assemble the 9-byte response: [status][encrypted block]
            uint8_t payload[RESPONSE_SIZE]{};
            payload[0] = status;
            memcpy(&payload[1], encrypted, PAYLOAD_SIZE);

            return ResponsePacket::build(cid, payload, RESPONSE_SIZE);
        }
    };

    /**
     * @brief A WRITE packet writes one page (4 bytes) to the NFC tag buffer at the given index on the
     * playpad. A WRITE payload is basically just `[index][page][data]`. The response will always be a
     * single status byte of `0x00`.
     *
     */
    struct WritePacket : CommandPacket
    {
        static constexpr uint8_t DATA_SIZE = NTAG213::PAGE_SIZE; // 4 bytes

        struct WriteRequest
        {
            uint8_t index;
            uint8_t page;
            uint8_t data[DATA_SIZE];
        };

        /// @brief Parses an incoming WRITE command.
        static WriteRequest fromCommand(const CommandPacket &packet)
        {
            WriteRequest req{};
            req.index = packet.payload()[0];
            req.page = packet.payload()[1];
            memcpy(req.data, packet.payload() + 2, DATA_SIZE);
            return req;
        }

        /// @brief Builds a WRITE command that writes one page to the tag at a given index.
        /// @param cid CID to send this command with
        /// @param index Token index assigned at placement
        /// @param page Page to write (each page = 4 bytes)
        /// @param data The 4 bytes of page data to write
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint8_t index, uint8_t page, const uint8_t data[DATA_SIZE])
        {
            uint8_t payload[2 + DATA_SIZE];
            payload[0] = index;
            payload[1] = page;
            memcpy(&payload[2], data, DATA_SIZE);
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::WRITE), cid, payload, sizeof(payload));
        }

        /// @brief Returns a 0x00-ed status response for when a WRITE succeeds
        static ResponsePacket buildResponse(uint8_t cid)
        {
            uint8_t status = 0x00;
            return ResponsePacket::build(cid, &status, 1);
        }
    };

    /**
     * @brief A TagList packet is used to get (basic) info about the currently placed tags and where
     * they are on the pad.
     *
     */
    struct TagListPacket : CommandPacket
    {
        /**
         * @brief A TagEntry is a bitfield (single byte) of the tag's index as well as the tags location
         * (PadLocation values).
         * The high nibble (bits 4-7) are the location.
         * The low nibble (bits 0-3) are the index
         *
         */
        struct TagEntry
        {
            uint8_t index;
            PadLocation location;

            uint8_t toByte() const
            {
                return (((static_cast<uint8_t>(location) << 4) & 0b11110000) | (index & 0b00001111));
            }
        };

        /// @brief Builds a TGLST command requesting the current tag list. Carries no payload.
        /// @param cid CID to send this command with
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid)
        {
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::TGLST), cid);
        }

        /**
         * @brief Creates a response packet based on the current TagEntry list
         *
         * @param cid Command ID to return in the response packet
         * @param tags An array of tags (up to 5-6 at most)
         * @param tagCount The # of tags in the array
         * @return ResponsePacket
         */
        static ResponsePacket buildResponse(uint8_t cid, TagEntry *tags, size_t tagCount)
        {
            static constexpr uint8_t MAX_PAYLOAD = (7 * 2); // 15 bytes
            uint8_t payload[MAX_PAYLOAD]{};

            const uint8_t payloadLen = static_cast<uint8_t>((tagCount * 2));

            for (size_t i = 0; i < tagCount; i++)
            {
                payload[(i * 2)] = tags[i].toByte();
                payload[(i * 2) + 1] = 0x00;
            }

            return ResponsePacket::build(cid, payload, payloadLen);
        }
    };

    /**
     * @brief A TagEventPacket is used to send info about when a tag moves on/off of the playpad.
     *
     */
    struct TagEventPacket : EventPacket
    {
        // Tag Placed:        `56 0b 02 00 00 00 04c834eafb7380 3b 000000000000000000000000000000000000`
        // Tag Removed:       `56 0b 02 00 00 01 04c834eafb7380 3c 000000000000000000000000000000000000`
        // Tag Placed:        `56 0b 02 00 01 00 04c835eafb7380 3d 000000000000000000000000000000000000`

        // 0x56: Event
        // 0x0B: Length
        // 0x02: Current Pad (Left)
        // 0x00: Always 0x00 (???)
        // 0x00: Index (+1 for every tag placed on the pad)
        // 0x00 (0x01): Direction

        // 04c834eafb7380: UID
        // 0x3B: Checksum

        /**
         * @brief An enum used to specify which way a given tag has moved on the playpad
         *
         */
        enum TagMovementDirection : uint8_t
        {
            PLACED = 0x00,
            REMOVED = 0x01
        };

        /**
         * @brief Builds an event packet to send, constructed of all of the parameters, and used to
         * identify that this specific tag has moved to this specific location When a tag is moved
         * between pads, a REMOVED and a PLACED event should be sent sequentially Since with a physical
         * playpad, it would have to be picked up and placed back down.
         *
         * @param currentPad The tag's current location
         * @param index The tag's index (~+1 for every tag placed on the pad)
         * @param direction The tag's movement direction (placed or removed from the playpad)
         * @param uid The UID for the given toytag
         * @return EventPacket
         */
        static EventPacket build(PadLocation currentPad,
                                 uint8_t index,
                                 TagMovementDirection direction,
                                 const uint8_t *uid)
        {
            EventPacket pkt;
            pkt.data[0] = static_cast<uint8_t>(PacketType::EVENT);
            pkt.data[1] = 11; // Payload Length

            pkt.data[2] = static_cast<uint8_t>(currentPad);
            pkt.data[3] = 0x00; // Always 0x00
            pkt.data[4] = index;
            pkt.data[5] = static_cast<uint8_t>(direction) & 0x01;
            memcpy(&pkt.data[6], uid, 7);
            pkt.data[13] = pkt.computeChecksum();

            return pkt;
        }
    };
}