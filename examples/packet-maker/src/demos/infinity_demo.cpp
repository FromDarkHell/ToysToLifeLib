#include "infinity_demo.h"

#include <Arduino.h>

#include "crypto/infinity_prng.h"
#include "infinity/packet.h"
#include "infinity/packets.h"
#include "infinity/tag.h"

using namespace DisneyInfinity;

void runInfinityDemo()
{
    // Build a WAKE command packet and print it as hex, along with the fixed hardware
    // acknowledgement (note its cid is always 0x00, regardless of the command's cid)
    CommandPacket wake = WakePacket::build(/*cid=*/0x00);
    Serial.print("WAKE packet: ");
    Serial.println(wake.toHexString());

    ResponsePacket wakeResponse = WakePacket::buildResponse();
    Serial.print("WAKE response: ");
    Serial.println(wakeResponse.toHexString());

    // Walk through the SEED -> CHALLENGE handshake using InfinityPRNG's scrambling
    CommandPacket seed = SeedPacket::build(/*cid=*/0x01, /*seed=*/0x12345678);
    Serial.print("SEED packet: ");
    Serial.println(seed.toHexString());

    uint32_t descrambledSeed = SeedPacket::fromCommand(seed);
    Serial.printf("  descrambled seed=0x%08X\n", descrambledSeed);

    InfinityPRNG prng;
    prng.init(descrambledSeed);

    CommandPacket challenge = ChallengePacket::build(/*cid=*/0x02);
    Serial.print("CHALLENGE packet: ");
    Serial.println(challenge.toHexString());

    ResponsePacket challengeResponse = ChallengePacket::buildResponse(/*cid=*/0x02, prng.next());
    Serial.print("CHALLENGE response: ");
    Serial.println(challengeResponse.toHexString());

    // Build a COL command that sets the center pad to red, and parse it back
    CommandPacket col = ColorPacket::build(/*cid=*/0x03, PadLocation::CENTER, PadColor::Red());
    Serial.print("COL packet: ");
    Serial.println(col.toHexString());

    ColorPacket::ColorStatus colStatus = ColorPacket::fromCommand(col);
    Serial.printf("  color=#%06X\n", colStatus.padColor.toHex());

    // Build a FADAL command that fades all three pads to their own colors
    FadeAllPacket::FadeStatus::PadFadeStatus centerFade{1, 10, 0, PadColor::Red()};
    FadeAllPacket::FadeStatus::PadFadeStatus leftFade{1, 10, 0, PadColor::Green()};
    FadeAllPacket::FadeStatus::PadFadeStatus rightFade{1, 10, 0, PadColor::Blue()};
    CommandPacket fadal = FadeAllPacket::build(/*cid=*/0x04, centerFade, leftFade, rightFade);
    Serial.print("FADAL packet: ");
    Serial.println(fadal.toHexString());

    // Create a figure, place it on the center pad, and dump the GETTOYS/MODEL responses for it
    Figure syndrome;
    syndrome.present = true;
    syndrome.orderAdded = 0x00;
    const uint8_t syndromeUid[7] = {0x04, 0xA5, 0xBD, 0xFA, 0x54, 0x42, 0x80};
    syndrome.setUID(syndromeUid);

    EventPacket figurePlaced = FigureEventPacket::build(PadLocation::CENTER, syndrome.orderAdded,
                                                        FigureEventPacket::PLACED);
    Serial.print("Figure placed event: ");
    Serial.println(figurePlaced.toHexString());

    CommandPacket gettoys = GetToysPacket::build(/*cid=*/0x05);
    Serial.print("GETTOYS packet: ");
    Serial.println(gettoys.toHexString());

    GetToysPacket::FigureEntry figures[] = {{syndrome.orderAdded, PadLocation::CENTER}};
    ResponsePacket gettoysResponse = GetToysPacket::buildResponse(/*cid=*/0x05, figures, 1);
    Serial.print("GETTOYS response: ");
    Serial.println(gettoysResponse.toHexString());

    CommandPacket model = ModelPacket::build(/*cid=*/0x06, syndrome.orderAdded);
    Serial.print("MODEL packet: ");
    Serial.println(model.toHexString());

    uint8_t modelUid[ModelPacket::UID_SIZE];
    syndrome.getUID(modelUid);
    ResponsePacket modelResponse = ModelPacket::buildResponse(/*cid=*/0x06, modelUid);
    Serial.print("MODEL response: ");
    Serial.println(modelResponse.toHexString());

    // Write a block to the figure, then read it back through the same game-side block index
    uint8_t writeData[WritePacket::DATA_SIZE];
    memset(writeData, 0xAB, sizeof(writeData));
    CommandPacket write = WritePacket::build(/*cid=*/0x07, syndrome.orderAdded, /*block=*/1, writeData);
    Serial.print("WRITE packet: ");
    Serial.println(write.toHexString());

    WritePacket::WriteRequest writeRequest = WritePacket::fromCommand(write);
    syndrome.setBlock(writeRequest.block, writeRequest.data);

    CommandPacket read = ReadPacket::build(/*cid=*/0x08, syndrome.orderAdded, /*block=*/1);
    Serial.print("READ packet: ");
    Serial.println(read.toHexString());

    uint8_t readData[ReadPacket::DATA_SIZE];
    syndrome.getBlock(/*block=*/1, readData);
    ResponsePacket readResponse = ReadPacket::buildResponse(/*cid=*/0x08, readData);
    Serial.print("READ response: ");
    Serial.println(readResponse.toHexString());
}
