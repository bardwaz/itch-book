#pragma once

#include "itch_messages.h"
#include <cstddef>

namespace itch {

class ItchParser {
public:
    // Parse one message from the buffer.
    // Returns the number of bytes consumed (including the 2-byte length prefix).
    // Fills `msg` with the parsed message.
    // Returns 0 if the buffer is too short to parse a full message.
    size_t parse_message(const char* buf, size_t len, Message& msg) const;

    // Get the expected message body size for a given type code (NOT including length prefix)
    static size_t message_size(char type_code);
};

} // namespace itch
