#pragma once

#include "common/packet.h"
#include "infinity/constants.h"

namespace DisneyInfinity
{
    using Common::BasePacket;
    using Common::PacketValidationError;

    using CommandPacket = Common::CommandPacket<static_cast<uint8_t>(PacketType::COMMAND)>;
    using EventPacket = Common::EventPacket<static_cast<uint8_t>(PacketType::EVENT)>;
    using ResponsePacket = Common::ResponsePacket<static_cast<uint8_t>(PacketType::RESPONSE)>;
}
