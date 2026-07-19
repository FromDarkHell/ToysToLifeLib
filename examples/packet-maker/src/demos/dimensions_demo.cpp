#include "dimensions_demo.h"

#include <Arduino.h>

#include "crypto/tea.h"
#include "lego/packet.h"
#include "lego/packets.h"
#include "lego/tag.h"

using namespace LEGODimensions;

void runDimensionsDemo()
{
    TEA crypto;
    crypto.setKey(TEA_KEY);

    // Build a WAKE command packet and print it as hex
    CommandPacket wake = WakePacket::build(/*cid=*/0x00);
    Serial.print("WAKE packet: ");
    Serial.println(wake.toHexString());

    // Build a COLAL command and parse it back with ColorAllPacket
    CommandPacket colal = ColorAllPacket::build(/*cid=*/0x01,
                                                PadColor::Red(), PadColor::Green(), PadColor::Blue());
    Serial.print("COLAL packet: ");
    Serial.println(colal.toHexString());

    ColorAllPacket::ColorStatus colors = ColorAllPacket::fromCommand(colal);
    Serial.printf("  center=#%06X left=#%06X right=#%06X\n",
                  colors.centerColor.toHex(), colors.leftColor.toHex(), colors.rightColor.toHex());

    // Build a SEED response, round-tripped through the TEA crypto instance
    SeedPacket::SeedStatus seedStatus{.seed = 0x12345678, .conf = 0xCAFEBABE};
    ResponsePacket seedResponse = SeedPacket::fromStatus(/*cid=*/0x02, seedStatus, &crypto);
    Serial.print("SEED response: ");
    Serial.println(seedResponse.toHexString());

    // Create a couple of toy tags and dump their raw NFC buffers
    CharacterTag gandalf(0x00A5, "Gandalf");
    char gandalfUid[15];
    gandalf.getUIDStr(gandalfUid);
    Serial.printf("CharacterTag '%s' (id=0x%04X, uid=%s)\n", gandalf.name, gandalf.id, gandalfUid);

    VehicleTag cart(0x01F4, "Wizard's Cart", /*upg23=*/0, /*upg25=*/0);
    char cartUid[15];
    cart.getUIDStr(cartUid);
    Serial.printf("VehicleTag '%s' (id=0x%04X, uid=%s)\n", cart.name, cart.id, cartUid);
}
