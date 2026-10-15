#pragma once
#include "constants.h"

#include "lego/constants.h"
#include "infinity/constants.h"
#include "kamenrider/constants.h"

namespace ToysToLifeLib
{
    struct PadIdentifier
    {
        ToypadPlatform platform;
        ToypadType type;
    };

    inline PadIdentifier getPadIdentifier(USBIdentifier identification)
    {
        ToypadPlatform platform = DisneyInfinity::getToypadPlatformFromIDs(identification);
        if (platform != ToypadPlatform::UNK)
            return {platform, ToypadType::DISNEY_INFINITY};

        platform = LEGODimensions::getToypadPlatformFromIDs(identification);
        if (platform != ToypadPlatform::UNK)
            return {platform, ToypadType::LEGO_DIMENSIONS};

        platform = KamenRider::getToypadPlatformFromIDs(identification);
        if (platform != ToypadPlatform::UNK)
            return {platform, ToypadType::KAMEN_RIDER};

        return {ToypadPlatform::UNK, ToypadType::UNK};
    }
}