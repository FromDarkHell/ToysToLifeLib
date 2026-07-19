The Disney Infinity pads are *basically* identical to LEGO Dimensions pads lol.
The main difference is:
- Different vendor/product IDs
- Packets start with a different byte (`0xFF`)
- Different output interface ID
- Different copyright wake-up string
- Different packet-type values;
    - Get Toys: `A1`
    - Read Toy: `A2`
    - Write Toy: `A3`
    - Get UID: `B4`
    - Responses are: `AA` for command, and `AB` for events
    - Colors have an `9x` base instead of `Cx` for dimensions

Aside from that, it's *the* same lol