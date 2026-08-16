#pragma once

#include <Arduino.h>
#include "tags/NTAG213.h"
#include "util/reader.h"

using namespace Tags;

namespace LEGODimensions
{
    /**
     * @brief Base class for all Lego Dimensions toy tags.
     *        Extends NTAG213 with game metadata (id, name, padIndex, type)
     *        as well as JSON serialization / deserialization.
     */
    class ToyTag : public NTAG213
    {
    public:
        /**
         * @brief An ID describing the tag in the JSON database of characters/vehicles
         *
         */
        uint16_t id;

        /**
         * @brief A string describing the tags name, used for display/serialization purposes
         *
         */
        char name[48];

        /**
         * @brief The current location of the tag on the playpad
         *
         */
        TagIndex padIndex;

        /**
         * @brief The type of this toytag (Vehicle or Character)
         *
         */
        ToyTagType type;

        ToyTag() : NTAG213(), id(0), padIndex(TagIndex::Unplaced), type(ToyTagType::Unknown)
        {
            name[0] = '\0';
            setUID(randomUID()); // Generated once, lives in _buf permanently
        }

        /**
         * @brief Gets the current type of the toytag as a string, used for JSON serialization
         *
         * @return const char*
         */
        const char *typeString() const
        {
            switch (type)
            {
            case ToyTagType::Character:
                return "character";
            case ToyTagType::Vehicle:
                return "vehicle";
            default:
                return "unknown";
            }
        }
    };

    /**
     * @brief A Lego Dimensions vehicle / gadget tag.
     *
     * Page Layout:
     *   Page 23 (offset 0x8C) : upgradesP23  - uint32 LE
     *   Page 24 (offset 0x90) : id           - uint16 LE
     *   Page 25 (offset 0x94) : upgradesP25  - uint32 LE
     *   Page 26 (offset 0x98) : 0x0001       - uint16 BE (verification)
     * For some reason, these pages can also get offset by 0x12 when being written *shrug*
     */
    class VehicleTag : public virtual ToyTag
    {
    public:
        uint32_t upgradesP23;
        uint32_t upgradesP25;

        // Page offsets (byte addresses inside the 180-byte buffer)
        static const uint8_t OFFSET_UPGRADES_P23 = 0x23 * 4; // page 23 * 4
        static const uint8_t OFFSET_ID = 0x24 * 4;           // page 24 * 4
        static const uint8_t OFFSET_UPGRADES_P25 = 0x25 * 4; // page 25 * 4
        static const uint8_t OFFSET_VERIFY = 0x26 * 4;       // page 26 * 4

        VehicleTag() : ToyTag(), upgradesP23(0), upgradesP25(0) { type = ToyTagType::Vehicle; }

        VehicleTag(uint16_t vehicleId, const char *vehicleName, uint32_t upg23 = 0, uint32_t upg25 = 0)
            : ToyTag(), upgradesP23(upg23), upgradesP25(upg25)
        {
            type = ToyTagType::Vehicle;
            id = vehicleId;
            strlcpy(name, vehicleName, sizeof(name));
            commitToBuffer();
        }

        /**
         * @brief Commits all of the stored vehicle data (upgrades/ID + verification) to the underlying
         * NFC byte buffer
         *
         */
        void commitToBuffer()
        {
            // Page 23 - upgrade part 1 (uint32 LE)
            Reader::writeUInt32LE(_buf, OFFSET_UPGRADES_P23, upgradesP23);

            // Page 24 - vehicle ID (uint16 LE)
            Reader::writeUInt16LE(_buf, OFFSET_ID, id);

            // Page 25 - upgrade part 2 (uint32 LE)
            Reader::writeUInt32LE(_buf, OFFSET_UPGRADES_P25, upgradesP25);

            // Page 26 - Verification uint16 0x0001
            Reader::writeUInt16BE(_buf, OFFSET_VERIFY, 0x0001);
        }

        /**
         * @brief Parses the underlying NFC buffer in order to fill the Upgrade/ID fields (also checks
         * the verification byte)
         *
         */
        void readFromBuffer()
        {
            upgradesP23 = Reader::readUInt32LE(_buf, OFFSET_UPGRADES_P23);
            id = Reader::readUInt16LE(_buf, OFFSET_ID);
            upgradesP25 = Reader::readUInt32LE(_buf, OFFSET_UPGRADES_P25);

            const uint16_t verificationByte = Reader::readUInt16BE(_buf, OFFSET_VERIFY);
            // TODO: Check the verification byte
        }
    };

    /**
     * @brief A Lego Dimensions character tag.
     *
     * The character ID is handled slightly differently from the READ/normal NFC bytes, by the game via
     * CMD_MODEL command, and not written into the NFC byte pages directly. The raw buffer stays mostly
     * zeroed - the game basically only checks that page 26 is 0x00.
     */
    class CharacterTag : public virtual ToyTag
    {
    public:
        CharacterTag() : ToyTag() { type = ToyTagType::Character; }

        CharacterTag(uint16_t charId, const char *charName) : ToyTag()
        {
            type = ToyTagType::Character;
            id = charId;
            strlcpy(name, charName, sizeof(name));
            // Page 26 must be 0x00 for character verification - already zero
            // from NTAG213's memset, but we assert it explicitly for clarity.
            Reader::writeUInt32BE(_buf, 0x98, 0x000000);
        }
    };
}