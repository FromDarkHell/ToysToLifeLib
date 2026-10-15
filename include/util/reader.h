#pragma once
#include <cstdint>
#include <cstddef>

class Reader
{
public:
    static uint32_t readUInt32LE(const uint8_t *buf, size_t offset = 0)
    {
        return (uint32_t)buf[offset] | ((uint32_t)buf[offset + 1] << 8) | ((uint32_t)buf[offset + 2] << 16) | ((uint32_t)buf[offset + 3] << 24);
    }

    static uint16_t readUInt16LE(const uint8_t *buf, size_t offset = 0)
    {
        return (uint16_t)buf[offset] | ((uint16_t)buf[offset + 1] << 8);
    }

    static uint32_t readUInt32BE(const uint8_t *buf, size_t offset = 0)
    {
        return ((uint32_t)buf[offset] << 24) | ((uint32_t)buf[offset + 1] << 16) | ((uint32_t)buf[offset + 2] << 8) | (uint32_t)buf[offset + 3];
    }

    static uint16_t readUInt16BE(const uint8_t *buf, size_t offset = 0)
    {
        return ((uint16_t)buf[offset] << 8) | (uint16_t)buf[offset + 1];
    }

    static uint64_t readUInt64LE(const uint8_t *buf, size_t offset = 0)
    {
        return (uint64_t)readUInt32LE(buf, offset) | ((uint64_t)readUInt32LE(buf, offset + 4) << 32);
    }

    static uint64_t readUInt64BE(const uint8_t *buf, size_t offset = 0)
    {
        return ((uint64_t)readUInt32BE(buf, offset) << 32) | (uint64_t)readUInt32BE(buf, offset + 4);
    }

    static void writeUInt32LE(uint8_t *buf, int offset, uint32_t value)
    {
        buf[offset] = (uint8_t)(value & 0xFF);
        buf[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
        buf[offset + 2] = (uint8_t)((value >> 16) & 0xFF);
        buf[offset + 3] = (uint8_t)((value >> 24) & 0xFF);
    }

    static void writeUInt32BE(uint8_t *buf, int offset, uint32_t value)
    {
        buf[offset + 3] = (uint8_t)(value & 0xFF);
        buf[offset + 2] = (uint8_t)((value >> 8) & 0xFF);
        buf[offset + 1] = (uint8_t)((value >> 16) & 0xFF);
        buf[offset + 0] = (uint8_t)((value >> 24) & 0xFF);
    }

    static void writeUInt64LE(uint8_t *buf, int offset, uint64_t value)
    {
        writeUInt32LE(buf, offset, (uint32_t)(value & 0xFFFFFFFF));
        writeUInt32LE(buf, offset + 4, (uint32_t)(value >> 32));
    }

    static void writeUInt64BE(uint8_t *buf, int offset, uint64_t value)
    {
        writeUInt32BE(buf, offset, (uint32_t)(value >> 32));
        writeUInt32BE(buf, offset + 4, (uint32_t)(value & 0xFFFFFFFF));
    }

    static void writeUInt16LE(uint8_t *buf, int offset, uint16_t value)
    {
        buf[offset] = (uint8_t)(value & 0xFF);
        buf[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
    }

    static void writeUInt16BE(uint8_t *buf, int offset, uint16_t value)
    {
        buf[offset + 1] = (uint8_t)(value & 0xFF);
        buf[offset + 0] = (uint8_t)((value >> 8) & 0xFF);
    }
};