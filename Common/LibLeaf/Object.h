#pragma once

#include <AK/Error.h>
#include <AK/FixedArray.h>
#include <AK/JsonObject.h>
#include <AK/JsonParser.h>

#include "LibEvent/Pidfd.h"
#include <LibEvent/TcpStream.h>

namespace Leaf {

class Object {
public:
    template<class T>
    static ErrorOr<NonnullRefPtr<T>> read(NonnullRefPtr<Event::TcpStream> stream)
    {
        Vector<u8> bytes;

        while (true) {
            auto array = TRY(FixedArray<u8>::try_create(1024));

            auto nread = ::read(stream->fd(), array.data(), array.size());
            if (nread == -1) {
                outln("errno = {}", strerror(errno));
                return Error::from_errno(errno);
            }

            bytes.append(array.data(), nread);

            if (nread != array.size())
                break;
        }

        if (bytes.size() == 0)
            return Error::from_string_literal("something went wrong with read, got 0 bytes");

        auto parser = JsonParser(StringView(bytes.data(), bytes.size()));
        auto value = TRY(parser.parse());
        auto json_object = value.as_object();

        auto remote_pid = json_object.get("__pid__"sv).as_u32();
        auto pidfd = TRY(Event::Pidfd::open(remote_pid));

        json_object.for_each_member([&](StringView key, JsonValue const& fd_object) {
            if (key.contains("fd"sv)) {
                auto new_fd = MUST(pidfd->getfd(fd_object.as_u32()));
                json_object.set(key, new_fd);
            }
            return;
        });

        // FIXME: This feels incredibly wrong (no type security)
        auto object = adopt_ref(*new T());
        TRY(object->deserialize(json_object));

        return object;
    }

    template<class T>
    static ErrorOr<bool> write(NonnullRefPtr<Event::TcpStream> stream, NonnullRefPtr<T> object)
    {
        auto serialized = JsonObject {};
        TRY(object->serialize(serialized));

        serialized.set("__pid__"sv, getpid());

        auto builder = StringBuilder();
        serialized.serialize(builder);

        auto bytes = builder.to_byte_buffer();

        TRY(stream->write(bytes));

        return true;
    }

    virtual ErrorOr<bool> serialize(JsonObject&)
    {
        VERIFY_NOT_REACHED();
    };

    virtual ErrorOr<bool> deserialize(JsonObject const&)
    {
        VERIFY_NOT_REACHED();
    };

    virtual ~Object() = default;
    explicit Object() = default;

protected:
};

}