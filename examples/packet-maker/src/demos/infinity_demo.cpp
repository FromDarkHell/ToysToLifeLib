#include "infinity_demo.h"

#include <Arduino.h>

#include "infinity/packet.h"
#include "infinity/packets.h"

using namespace DisneyInfinity;

void runInfinityDemo()
{
    // Build a WAKE command packet and print it as hex
    CommandPacket wake = WakePacket::build(/*cid=*/0x00);
    Serial.print("WAKE packet: ");
    Serial.println(wake.toHexString());

    // Build a COL command that sets the center pad to red, and parse it back
    CommandPacket col = ColorPacket::build(/*cid=*/0x01, PadLocation::CENTER, PadColor::Red());
    Serial.print("COL packet: ");
    Serial.println(col.toHexString());

    ColorPacket::ColorStatus colStatus = ColorPacket::fromCommand(col);
    Serial.printf("  color=#%06X\n", colStatus.padColor.toHex());

    // Build a FADAL command that fades all three pads to their own colors
    FadeAllPacket::FadeStatus::PadFadeStatus centerFade{1, 10, 0, PadColor::Red()};
    FadeAllPacket::FadeStatus::PadFadeStatus leftFade{1, 10, 0, PadColor::Green()};
    FadeAllPacket::FadeStatus::PadFadeStatus rightFade{1, 10, 0, PadColor::Blue()};
    CommandPacket fadal = FadeAllPacket::build(/*cid=*/0x02, centerFade, leftFade, rightFade);
    Serial.print("FADAL packet: ");
    Serial.println(fadal.toHexString());
}
