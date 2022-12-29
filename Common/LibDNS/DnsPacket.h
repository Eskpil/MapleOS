#pragma once

#include <AK/Endian.h>
#include <AK/Types.h>

namespace DNS {

struct [[gnu::packed]] PacketHeader {
    NetworkOrdered<u16> m_id;

    bool m_recursion_desired : 1;
    bool m_truncated : 1;
    bool m_authoritative_answer : 1;
    u8 m_opcode : 4;
    bool m_query_or_response : 1;
    u8 m_response_code : 4;
    bool m_checking_disabled : 1;
    bool m_authenticated_data : 1;
    bool m_zero : 1;
    bool m_recursion_available : 1;

    NetworkOrdered<u16> m_question_count;
    NetworkOrdered<u16> m_answer_count;
    NetworkOrdered<u16> m_authority_count;
    NetworkOrdered<u16> m_additional_count;
};

static_assert(sizeof(PacketHeader) == 12);

class Packet {
private:
    PacketHeader m_header;
};

}