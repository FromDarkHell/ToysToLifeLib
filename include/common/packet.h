#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "crypto/tea.h"
#include <cstdlib>

/**
 * @brief Shared packet structure for toys-to-life protocols that happen to be (near) byte-identical
 * at the transport level, e.g. LEGO Dimensions and Disney Infinity.
 */
namespace Common
{
    constexpr int TOYPAD_MAX_PACKET_SIZE = 0x20;

    enum class PacketValidationError : uint8_t
    {
        OK = 0x00,
        BAD_TYPE = 0x01,
        MAGIC = 0x02,
        LENGTH = 0x03,
        CHECKSUM = 0x04
    };

    /**
     * @brief BasePacket holds the raw byte buffer along with the length/checksum/hex-dump helpers
     * that are identical across the protocols sharing this namespace. Each protocol's
     * CommandPacket/ResponsePacket/EventPacket wraps this with its own type-byte constant.
     */
    struct BasePacket
    {
        uint8_t data[TOYPAD_MAX_PACKET_SIZE];

        BasePacket() { memset(data, 0x00, TOYPAD_MAX_PACKET_SIZE); }

        uint8_t type() const { return data[0]; }
        uint8_t length() const { return data[1]; }
        uint8_t checksum() const { return data[length() + 2]; }
        virtual const uint8_t *payload() const = 0;
        virtual uint8_t payloadSize() const = 0;

        uint8_t computeChecksum() const
        {
            uint16_t sum = 0;
            for (uint8_t i = 0; i < length() + 2; ++i)
            {
                sum += data[i];
            }
            return static_cast<uint8_t>(sum & 0xFF);
        }

        PacketValidationError isValid(uint8_t expectedType) const
        {
            if (type() != expectedType)
            {
                return PacketValidationError::BAD_TYPE;
            }

            if (length() + 2u >= TOYPAD_MAX_PACKET_SIZE)
            {
                return PacketValidationError::LENGTH;
            }

            if (computeChecksum() != checksum())
            {
                return PacketValidationError::CHECKSUM;
            }

            return PacketValidationError::OK;
        }

        const char *toHexString() const
        {
            static char hexString[TOYPAD_MAX_PACKET_SIZE * 2 + 1];
            for (size_t i = 0; i < TOYPAD_MAX_PACKET_SIZE; ++i)
            {
                sprintf(&hexString[i * 2], "%02X", data[i]);
            }
            return hexString;
        }

        const char *payloadToHexString() const
        {
            static char hexString[TOYPAD_MAX_PACKET_SIZE * 2 + 1];
            for (size_t i = 0; i < payloadSize(); ++i)
            {
                sprintf(&hexString[i * 2], "%02X", payload()[i]);
            }
            return hexString;
        }

        // Encrypts the payload in-place and recomputes the checksum.
        // Returns false if TEA encryption fails or there is no payload.
        bool encryptPayload(TEA *tea)
        {
            if (payloadSize() == 0)
            {
                return false;
            }

            uint8_t encrypted[TOYPAD_MAX_PACKET_SIZE]{};
            if (!tea->encrypt(payload(), encrypted))
            {
                return false;
            }

            memcpy(&data[4], encrypted, payloadSize());
            data[payloadSize() + 4] = computeChecksum();
            return true;
        }

        // Decrypts the payload and returns the decrypted array
        // Returns nullptr if TEA decryption fails or there is no payload.
        static const uint8_t *decryptPayload(const BasePacket *packet, TEA *tea)
        {
            if (packet->payloadSize() == 0)
            {
                return nullptr;
            }

            uint8_t *decrypted = (uint8_t *)malloc(TOYPAD_MAX_PACKET_SIZE);
            if (!tea->decrypt(packet->payload(), decrypted))
            {
                free(decrypted);
                return nullptr;
            }

            return decrypted;
        }

    protected:
        ~BasePacket() = default;
    };

    /**
     * @brief A command packet comes from the game to tell the peripheral to do something specific. A
     * response packet will (usually) be sent in return. Each command comes with a Command ID (`cid`),
     * which is used to tell the game that the peripheral is responding to a specific command.
     *
     * `TypeByte` is the protocol's fixed value for the first packet byte (e.g. 0x55 for LEGO
     * Dimensions, 0xFF for Disney Infinity).
     *
     * The data is laid out like so: `[type][length][cmd][cid][...payload][checksum]`
     */
    template <uint8_t TypeByte>
    struct CommandPacket : BasePacket
    {
        uint8_t cid() const { return data[3]; }
        uint8_t command() const { return data[2]; }
        const uint8_t *payload() const override { return &data[4]; }
        uint8_t payloadSize() const override { return length() > 2 ? length() - 2 : 0; }

        PacketValidationError isValid() const { return BasePacket::isValid(TypeByte); }

        static CommandPacket build(uint8_t cmd,
                                   uint8_t cid,
                                   const uint8_t *payload = nullptr,
                                   uint8_t payloadLen = 0)
        {
            CommandPacket pkt;
            pkt.data[0] = TypeByte;
            pkt.data[1] = payloadLen + 2;
            pkt.data[2] = cmd;
            pkt.data[3] = cid;

            if (payload && payloadLen > 0)
            {
                memcpy(&pkt.data[4], payload, payloadLen);
            }

            pkt.data[payloadLen + 4] = pkt.computeChecksum();
            return pkt;
        }
    };

    /**
     * @brief An event packet is a simple wrapper around the BasePacket, used for when the peripheral
     * has an update (i.e. an NFC tag has been placed). Unlike `CommandPacket`, an EventPacket doesn't
     * have a `cid`.
     *
     * The packet is laid out like so: `[type][length][cmd][...payload][checksum]`
     */
    template <uint8_t TypeByte>
    struct EventPacket : BasePacket
    {
        const uint8_t *payload() const override { return &data[3]; }
        uint8_t payloadSize() const override { return length() > 1 ? length() - 1 : 0; }

        PacketValidationError isValid() const { return BasePacket::isValid(TypeByte); }
    };

    /**
     * @brief A response packet is sent from the peripheral back to the console, in order to tell the
     * game specific info about a given command.
     *
     * `TypeByte` is the protocol's fixed value for the first response byte. Some protocols reuse the
     * same byte as their command packets (LEGO Dimensions: 0x55 for both), while others use a distinct
     * one (Disney Infinity: 0xAA for responses vs 0xFF for commands).
     *
     * Layout: `[type][length][cid][...payload][checksum]`
     */
    template <uint8_t TypeByte>
    struct ResponsePacket : BasePacket
    {
        uint8_t cid() const { return data[2]; }
        const uint8_t *payload() const override { return &data[3]; }
        uint8_t payloadSize() const override { return length() > 1 ? length() - 1 : 0; }

        PacketValidationError isValid() const { return BasePacket::isValid(TypeByte); }

        /**
         * @brief Crafts a base ResponsePacket with a given CID and payload
         *
         * @param cid The originating command's `cid` value to use as an ID for this response.
         * @param payload The payload data to return, defaults to nullptr/blank.
         * @param payloadLen The length of the payload data to return
         * @return ResponsePacket A fully serialized, checksummed, ResponsePacket.
         */
        static ResponsePacket build(uint8_t cid,
                                    const uint8_t *payload = nullptr,
                                    uint8_t payloadLen = 0)
        {
            ResponsePacket pkt;
            pkt.data[0] = TypeByte;
            pkt.data[1] = payloadLen + 1; // length = cid + payload
            pkt.data[2] = cid;

            if (payload && payloadLen > 0)
            {
                memcpy(&pkt.data[3], payload, payloadLen);
            }

            pkt.data[payloadLen + 3] = pkt.computeChecksum();
            return pkt;
        }

        /**
         * @brief Crafts a blank (payload-less) ResponsePacket using a given `cid`.
         *
         * @param cid The originating command's `cid` value to use as an ID for this response.
         * @return ResponsePacket A fully serialized, checksummed, ResponsePacket.
         */
        static ResponsePacket blank(uint8_t cid)
        {
            ResponsePacket pkt;
            pkt.data[0] = TypeByte;
            pkt.data[1] = 1; // length = cid
            pkt.data[2] = cid;

            pkt.data[3] = pkt.computeChecksum();
            return pkt;
        }
    };
}
