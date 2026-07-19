#pragma once
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <Arduino.h>

namespace ToysToLifeLib
{
    /**
     * @brief Which physical pad section to address.
     */
    enum class PadLocation : uint8_t
    {
        ALL = 0x00,
        CENTER = 0x01,
        LEFT = 0x02,
        RIGHT = 0x03,
    };

    /**
     * @brief Simple enum used for differentiating various platforms from one another
     *
     */
    enum class ToypadPlatform : uint8_t
    {
        PS3 = 0x00,
        X360 = 0x01,
        UNK = 0xFF
    };

    /**
     * @brief A simple enum used to differentiate various supported toypad types.
     *
     */
    enum class ToypadType : uint8_t
    {
        LEGO_DIMENSIONS = 0x00,
        DISNEY_INFINITY = 0x01,

        UNK = 0xFF
    };

    /**
     * @brief A struct for defining R/G/B colors for a specific pad
     *
     */
    struct PadColor
    {
        uint8_t r, g, b;

        /**
         * @brief Returns a pad color based on a 3-byte long buffer
         *
         * @param hex The buffer to read 3 bytes off of and then convert to a color
         * @return PadColor A pad color from the first 3 bytes of the buffer
         */
        static inline PadColor fromBuffer(const uint8_t *hex) { return {hex[0], hex[1], hex[2]}; }

        /**
         * @brief Returns a pad color based on a packed 0xRRGGBB value
         *
         * @param hex The packed color value to convert
         * @return PadColor
         */
        static constexpr PadColor fromUint32(uint32_t hex)
        {
            return {
                static_cast<uint8_t>((hex >> 16) & 0xFF),
                static_cast<uint8_t>((hex >> 8) & 0xFF),
                static_cast<uint8_t>(hex & 0xFF),
            };
        }

        /**
         * @brief Returns a pad color parsed from a hex string, e.g. "FF00FF" or "#FF00FF"
         *
         * @param hex The hex string to parse (with or without a leading '#')
         * @return PadColor
         */
        static PadColor fromHex(const char *hex)
        {
            if (hex[0] == '#')
            {
                hex++;
            }
            return fromUint32(static_cast<uint32_t>(strtoul(hex, nullptr, 16)));
        }

        /**
         * @brief Converts a PadColor to a uint32 like 0xFF00FF
         *
         * @return uint32_t
         */
        inline uint32_t toHex() const { return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff); }

        /**
         * @brief Alias of toHex(), converts a PadColor to a packed 0xRRGGBB value
         *
         * @return uint32_t
         */
        inline uint32_t toUint32() const { return toHex(); }

        /**
         * @brief Linearly interpolates a color from `a` -> `b` at percentage `t`
         *
         * @param a The starting color
         * @param b The ending color
         * @param t A percentage to interpolate
         * @return PadColor The interpolated color at `t`% between `a` and `b`
         */
        static PadColor lerpColor(const PadColor &a, const PadColor &b, float t)
        {
            return {
                (uint8_t)(a.r + (int16_t)(b.r - a.r) * t),
                (uint8_t)(a.g + (int16_t)(b.g - a.g) * t),
                (uint8_t)(a.b + (int16_t)(b.b - a.b) * t),
            };
        }

        static constexpr PadColor Black() { return {0x00, 0x00, 0x00}; }
        static constexpr PadColor White() { return {0xFF, 0xFF, 0xFF}; }
        static constexpr PadColor Red() { return {0xFF, 0x00, 0x00}; }
        static constexpr PadColor Green() { return {0x00, 0xFF, 0x00}; }
        static constexpr PadColor Blue() { return {0x00, 0x00, 0xFF}; }

        bool operator==(const PadColor &o) const { return r == o.r && g == o.g && b == o.b; }
        bool operator!=(const PadColor &o) const { return !(*this == o); }
    };

    struct USBIdentifier
    {
        uint16_t VID;
        uint16_t PID;

        bool operator==(const USBIdentifier &o) const { return o.VID == VID && o.PID == PID; }
        bool operator!=(const USBIdentifier &o) const { return !(*this == o); }
    };

};
