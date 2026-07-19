#include <Arduino.h>

#include "demos/dimensions_demo.h"
#include "demos/infinity_demo.h"

struct Demo
{
    const char *name;
    void (*run)();
};

static const Demo DEMOS[] = {
    {"LEGO Dimensions", runDimensionsDemo},
    {"Disney Infinity", runInfinityDemo},
};
static constexpr size_t DEMO_COUNT = sizeof(DEMOS) / sizeof(DEMOS[0]);

static void printMenu()
{
    Serial.println();
    Serial.println("Select a toys-to-life packet type:");
    for (size_t i = 0; i < DEMO_COUNT; i++)
    {
        Serial.printf("  %u) %s\n", i + 1, DEMOS[i].name);
    }
    Serial.print("> ");
}

void setup()
{
    Serial.begin(115200);
    delay(5000);
    printMenu();
}

void loop()
{
    // Non-blocking line read: accumulate characters until a newline arrives, then
    // dispatch. This is what lets the demo respond to input whenever a monitor
    // happens to connect, rather than blocking setup() waiting on Serial.
    static char lineBuf[16];
    static size_t lineLen = 0;

    while (Serial.available())
    {
        char c = (char)Serial.read();
        if (c == '\r')
        {
            continue;
        }

        if (c == '\n')
        {
            Serial.println();
            lineBuf[lineLen] = '\0';

            int choice = atoi(lineBuf);
            if (choice >= 1 && (size_t)choice <= DEMO_COUNT)
            {
                Serial.printf("--- %s demo ---\n", DEMOS[choice - 1].name);
                DEMOS[choice - 1].run();
            }
            else
            {
                Serial.println("Invalid selection.");
            }

            lineLen = 0;
            printMenu();
            continue;
        }

        if (lineLen < sizeof(lineBuf) - 1)
        {
            lineBuf[lineLen++] = c;
            Serial.print(c);
        }
    }
}
