#pragma once

#include "kamenrider/packet.h"

namespace KamenRider
{
    /**
     * @brief A WAKE packet is the first step of the handshaking process.
     *
     */
    struct WakePacket : CommandPacket
    {
        static constexpr const char *PAYLOAD = "(c) Bandai 2014";
        static constexpr uint8_t PAYLOAD_SIZE = 15;

        /// @brief Builds a WAKE command packet with the fixed payload.
        /// @param cid CID to send this command with
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid)
        {
            return CommandPacket::build(
                static_cast<uint8_t>(GatewayCommand::WAKE), cid,
                reinterpret_cast<const uint8_t *>(PAYLOAD), PAYLOAD_SIZE);
        }

        /// @brief Builds the response to a WAKE command
        ///
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid)
        {
            uint8_t RESPONSE[24] = {
                0x00, 0x07, 0x00, 0x03, 0x02, 0x09, 0x20, 0x03, 0xF5, 0x00, 0x19, 0x42,
                0x52, 0xB7, 0xB9, 0xA1, 0xAE, 0x2B, 0x88, 0x42, 0x05, 0xFE, 0xE0, 0x1C};

            return ResponsePacket::build(
                static_cast<uint8_t>(GatewayCommand::WAKE),
                cid,
                RESPONSE,
                24);
        }
    };

    /**
     * @brief A COL packet sets the gate's color. Unlike LEGO Dimensions there's no pad index, the
     * payload is just `[r][g][b]`.
     *
     */
    struct ColorPacket : CommandPacket
    {
        static constexpr uint8_t PAYLOAD_SIZE = 3;

        /// @brief Parses an incoming COL command.
        /// @param packet Raw CommandPacket received from the game
        /// @return The color to set
        static PadColor fromCommand(const CommandPacket &packet)
        {
            return PadColor::fromBuffer(packet.payload());
        }

        /// @brief Builds a COL command that sets the gate's color.
        /// @param cid CID to send this command with
        /// @param color The color to set
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, PadColor color)
        {
            uint8_t payload[PAYLOAD_SIZE] = {color.r, color.g, color.b};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::COL), cid, payload, sizeof(payload));
        }

        /// @brief Builds the blank acknowledgement to a COL command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid)
        {
            return ResponsePacket::build(static_cast<uint8_t>(GatewayCommand::COL), cid);
        }
    };

    /**
     * @brief A FLASH packet flashes the gate between its current color (for `onTicks` ticks) and
     * `offColor` (for `offTicks` ticks), `count` times. This is LEGO Dimensions' FLSH layout minus
     * the pad index: `[onTicks][offTicks][count][r][g][b]`.
     *
     * This layout is inferred from a single capture (`3C3CC8FFFAAF`), which reads as 60/60 ticks,
     * 200 cycles, color FFFAAF. That same capture could also be two RGB colors (`3C3CC8` then
     * `FFFAAF`), so this still needs confirming against more samples.
     *
     */
    struct FlashPacket : CommandPacket
    {
        static constexpr uint8_t PAYLOAD_SIZE = 6;

        struct FlashStatus
        {
            uint8_t onTicks;
            uint8_t offTicks;
            uint8_t count;

            PadColor offColor;
        };

        /// @brief Parses an incoming FLASH command.
        /// @param packet Raw CommandPacket received from the game
        /// @return A parsed FLASH command
        static FlashStatus fromCommand(const CommandPacket &packet)
        {
            const uint8_t *payload = packet.payload();
            return {payload[0], payload[1], payload[2], PadColor::fromBuffer(&payload[3])};
        }

        /// @brief Builds a FLASH command.
        /// @param cid CID to send this command with
        /// @param onTicks How many ticks to stay on the current color
        /// @param offTicks How many ticks to stay on `offColor`
        /// @param count Number of cycles to flash
        /// @param offColor The color to flash to
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, uint8_t onTicks, uint8_t offTicks, uint8_t count, PadColor offColor)
        {
            uint8_t payload[PAYLOAD_SIZE] = {onTicks, offTicks, count, offColor.r, offColor.g, offColor.b};
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::FLASH), cid, payload, sizeof(payload));
        }

        /// @brief Builds the blank acknowledgement to a FLASH command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid)
        {
            return ResponsePacket::build(static_cast<uint8_t>(GatewayCommand::FLASH), cid);
        }
    };

    /**
     * @brief A TGLST packet asks for the UIDs of every tag currently placed on the gate. Each entry in
     * the response is `[TAG_ENTRY_MARKER][uid * 7]`.
     *
     */
    struct TagListPacket : CommandPacket
    {
        static constexpr uint8_t ENTRY_SIZE = 1 + TAG_UID_SIZE;

        /**
         * @brief How many entries fit in one packet: 32 bytes, minus the 4-byte header and the
         * checksum, leaves 27 bytes, so 3 entries.
         *
         * RPCS3 lists every placed tag, but only copies the first 32 bytes of each response onto the
         * wire, so with 4+ tags it sends a truncated packet with no checksum. How real hardware
         * reports 4+ tags is unknown.
         */
        static constexpr uint8_t MAX_ENTRIES = (PLAYPAD_MAX_PACKET_SIZE - 5) / ENTRY_SIZE;

        /// @brief Builds a TGLST command requesting the current tag list. Carries no payload.
        /// @param cid CID to send this command with
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid)
        {
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::TGLST), cid);
        }

        /**
         * @brief Builds a TGLST response listing the given tags' UIDs.
         *
         * @param cid CID mirrored from the incoming command
         * @param uids The UIDs of the currently placed tags
         * @param tagCount The # of UIDs in `uids`; anything past `MAX_ENTRIES` is dropped
         * @return ResponsePacket
         */
        static ResponsePacket buildResponse(uint8_t cid, const uint8_t (*uids)[TAG_UID_SIZE], size_t tagCount)
        {
            if (tagCount > MAX_ENTRIES)
            {
                tagCount = MAX_ENTRIES;
            }

            uint8_t payload[MAX_ENTRIES * ENTRY_SIZE]{};
            for (size_t i = 0; i < tagCount; i++)
            {
                payload[i * ENTRY_SIZE] = TAG_ENTRY_MARKER;
                memcpy(&payload[(i * ENTRY_SIZE) + 1], uids[i], TAG_UID_SIZE);
            }

            return ResponsePacket::build(static_cast<uint8_t>(GatewayCommand::TGLST), cid, payload,
                                         static_cast<uint8_t>(tagCount * ENTRY_SIZE));
        }
    };

    /**
     * @brief Where a READ/WRITE lands: the targeted tag's UID, plus the sector/block within it.
     *
     */
    struct TagBlockAddress
    {
        uint8_t uid[TAG_UID_SIZE];
        uint8_t sector;
        uint8_t block;

        bool isValid() const { return sector < TAG_SECTOR_COUNT && block < TAG_BLOCKS_PER_SECTOR; }

        /// @brief The MIFARE_S20 page index this address maps to. Only meaningful if `isValid()`.
        uint8_t page() const { return (sector * TAG_BLOCKS_PER_SECTOR) + block; }

        /// @brief Parses `[uid * 7][sector][block]` from the start of a command payload.
        static TagBlockAddress fromPayload(const uint8_t *payload)
        {
            TagBlockAddress address{};
            memcpy(address.uid, payload, TAG_UID_SIZE);
            address.sector = payload[TAG_UID_SIZE];
            address.block = payload[TAG_UID_SIZE + 1];
            return address;
        }

        /// @brief Writes `[uid * 7][sector][block]` to the start of `out`.
        void toPayload(uint8_t *out) const
        {
            memcpy(out, uid, TAG_UID_SIZE);
            out[TAG_UID_SIZE] = sector;
            out[TAG_UID_SIZE + 1] = block;
        }

        static constexpr uint8_t PAYLOAD_SIZE = TAG_UID_SIZE + 2;
    };

    /**
     * @brief A READ packet requests one 16-byte block from the tag with a given UID. A READ payload is
     * `[uid * 7][sector][block]`.
     *
     * The response is `[status][data * 16]`. RPCS3 always sends a status of `0x00`, answering with
     * zeroed data if the tag isn't placed or the address is out of range.
     *
     */
    struct ReadPacket : CommandPacket
    {
        static constexpr uint8_t RESPONSE_SIZE = 1 + TAG_BLOCK_SIZE; // status + data

        /// @brief Parses an incoming READ command.
        static TagBlockAddress fromCommand(const CommandPacket &packet)
        {
            return TagBlockAddress::fromPayload(packet.payload());
        }

        /// @brief Builds a READ command requesting one block of a tag's data.
        /// @param cid CID to send this command with
        /// @param address The tag UID and sector/block to read
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, const TagBlockAddress &address)
        {
            uint8_t payload[TagBlockAddress::PAYLOAD_SIZE]{};
            address.toPayload(payload);
            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::READ), cid, payload, sizeof(payload));
        }

        /// @brief Builds a READ response carrying one block of tag data.
        /// @param cid CID mirrored from the incoming command
        /// @param data 16 bytes of tag data at the requested address
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid, const uint8_t data[TAG_BLOCK_SIZE])
        {
            uint8_t payload[RESPONSE_SIZE]{};
            payload[0] = 0x00;
            memcpy(&payload[1], data, TAG_BLOCK_SIZE);

            return ResponsePacket::build(static_cast<uint8_t>(GatewayCommand::READ), cid, payload, RESPONSE_SIZE);
        }
    };

    /**
     * @brief A WRITE packet writes one 16-byte block to the tag with a given UID. A WRITE payload is
     * `[uid * 7][sector][block][data * 16]`.
     *
     * The response is a blank acknowledgement.
     *
     */
    struct WritePacket : CommandPacket
    {
        struct WriteRequest
        {
            TagBlockAddress address;
            uint8_t data[TAG_BLOCK_SIZE];
        };

        /// @brief Parses an incoming WRITE command.
        static WriteRequest fromCommand(const CommandPacket &packet)
        {
            WriteRequest req{};
            req.address = TagBlockAddress::fromPayload(packet.payload());
            memcpy(req.data, packet.payload() + TagBlockAddress::PAYLOAD_SIZE, TAG_BLOCK_SIZE);
            return req;
        }

        /// @brief Builds a WRITE command that writes one block of a tag's data.
        /// @param cid CID to send this command with
        /// @param address The tag UID and sector/block to write
        /// @param data The 16 bytes of block data to write
        /// @return CommandPacket
        static CommandPacket build(uint8_t cid, const TagBlockAddress &address, const uint8_t data[TAG_BLOCK_SIZE])
        {
            uint8_t payload[TagBlockAddress::PAYLOAD_SIZE + TAG_BLOCK_SIZE]{};
            address.toPayload(payload);
            memcpy(&payload[TagBlockAddress::PAYLOAD_SIZE], data, TAG_BLOCK_SIZE);

            return CommandPacket::build(static_cast<uint8_t>(GatewayCommand::WRITE), cid, payload, sizeof(payload));
        }

        /// @brief Builds the blank acknowledgement to a WRITE command.
        /// @param cid CID mirrored from the incoming command
        /// @return ResponsePacket
        static ResponsePacket buildResponse(uint8_t cid)
        {
            return ResponsePacket::build(static_cast<uint8_t>(GatewayCommand::WRITE), cid);
        }
    };

    /**
     * @brief A TagEventPacket notifies the game when a tag is placed on or removed from the gate. The
     * layout is `[0x56][length][TAG_ENTRY_MARKER][direction][uid * 7][checksum]`.
     *
     * RPCS3 only sends these once the gate has been woken (see WakePacket).
     *
     */
    struct TagEventPacket : EventPacket
    {
        /**
         * @brief Which way a tag moved. Note this is the reverse of Disney Infinity's encoding.
         *
         */
        enum TagMovementDirection : uint8_t
        {
            REMOVED = 0x00,
            PLACED = 0x01
        };

        /// @brief Builds an event packet announcing a tag was placed or removed.
        /// @param uid The tag's 7-byte UID
        /// @param direction Whether the tag was placed or removed
        /// @return EventPacket
        static EventPacket build(const uint8_t uid[TAG_UID_SIZE], TagMovementDirection direction)
        {
            EventPacket pkt;
            pkt.data[0] = static_cast<uint8_t>(PacketType::EVENT);
            pkt.data[1] = 2 + TAG_UID_SIZE; // marker + direction + UID
            pkt.data[2] = TAG_ENTRY_MARKER;
            pkt.data[3] = static_cast<uint8_t>(direction);
            memcpy(&pkt.data[4], uid, TAG_UID_SIZE);
            pkt.data[4 + TAG_UID_SIZE] = pkt.computeChecksum();

            return pkt;
        }
    };
}