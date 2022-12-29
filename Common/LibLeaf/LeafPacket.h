#pragma once

#include <AK/String.h>
#include <AK/Types.h>

/* leaf object
 *
 * {
 *      "path": "com.mapleos.test",
 *      "functions": {
 *          "name": "test", // represented as UTF-8.
 *          "kind":  0, // 0 for async, 1 for sync.
 *          "sig": {
 *              "arguments": [],
 *              "returning": []
 *          }
 *      },
 *      "attributes": {
 *          "name": "person",
 *          "type": 0 // 0 is for string.
 *      },
 *      "signals": {} not implemented yet. Won't for a while probably.
 * }
 *
 * example leaf packet:
 *
 *     direction = Direction::Request
 *     kind = RequestKind::Procedure
 *     path = com.mapleos.test
 *
 */

namespace Leaf {

// NOTE: The PacketHeader should keep enough information
// for the bus to route it, and forward it.
struct [[gnu::packed]] PacketHeader {
    // TODO: Support signals.
    enum RequestKind : u8 {
        Procedure = 0,
        Attribute = 1,
    };

    enum Direction : u8 {
        Response = 0,
        Request = 1,
    };

    // NOTE: Each client should be responsible for creating an
    // id we can track their transaction by since calls to the bus
    // are multistep. client -> but -> service -> bus -> client.
    u32 id;

    Direction direction : 1;
    RequestKind request_kind : 4;

    u16 index; // 0-7 are reserved indexes.

    // NOTE: We need to keep track of how many file descriptors were sent over
    // this packet, so we can forward them. This is not necessary for the rest
    // of the data the packet came with.
    u8 fd_amount;

    // NOTE: The client must also send the size of additional data they are sending along.
    // this data would be function arguments or function returns values. This is also
    // responsible for keeping data such as the function or attribute name.
    u32 data_size;
};

class [[gnu::packed]] Packet {
public:
private:
};

}