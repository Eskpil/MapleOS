#include <AK/ByteReader.h>
#include <AK/JsonObject.h>
#include <AK/LexicalPath.h>
#include <AK/MemoryStream.h>
#include <AK/Random.h>
#include <AK/Vector.h>

#include <LibEvent/EventLoop.h>
#include <LibEvent/Signals.h>
#include <LibEvent/TcpServer.h>
#include <LibEvent/TcpStream.h>
#include <LibEvent/Timer.h>

#include <LibLeaf/Object.h>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

class TestObject : Leaf::Object
    , public RefCounted<TestObject> {
public:
    ErrorOr<bool> serialize(JsonObject& object) override
    {
        object.set("test"sv, m_test_data.to_deprecated_string());
        object.set("test_fd"sv, m_test_fd);

        return true;
    }

    ErrorOr<bool> deserialize(JsonObject const& object) override
    {
        m_test_data = TRY(String::from_deprecated_string(object.get("test"sv).to_deprecated_string()));
        m_test_fd = object.get("test_fd"sv).as_i32();
        return true;
    }

    ~TestObject() override = default;

    explicit TestObject(String data, int test_fd)
        : m_test_data(move(data))
        , m_test_fd(test_fd)
    {
    }

    explicit TestObject(JsonObject object)
    {
        MUST(serialize(object));
    }

    explicit TestObject() { }

    String data() const { return m_test_data; }
    int fd() const { return m_test_fd; }

public:
    int m_test_fd;
    String m_test_data;
};

static auto ip = IPv4Address { 127, 0, 0, 1 };

int main2(void)
{
    Function<u16()> get_port = [&] {
        auto new_port = get_random<u16>();
        if (1024 > new_port) {
            return get_port();
        }
        return new_port;
    };

    auto port = get_port();

    auto event_loop = MUST(Event::EventLoop::create());

    auto tcp_server = MUST(Event::TcpServer::create(Event::SocketType::IPv4));
    // MUST(tcp_server->bind(LexicalPath("/run/interface.sock")));

    auto success_or_error = tcp_server->bind(ip, port);
    if (success_or_error.is_error()) {
        outln("bind: {}", success_or_error.error());
        return 1;
    }

    tcp_server->on_connection = [&](NonnullRefPtr<Event::TcpStream> const& stream) {
        stream->on_ready = [stream]() {
            auto object_or_error = Leaf::Object::read<TestObject>(stream);
            if (object_or_error.is_error()) {
                outln("object_read: {}", object_or_error.error());
                return;
            }

            auto object = object_or_error.release_value();
            outln("message = {}", object->data());
            outln("\tfd = {}", object->fd());
        };
    };

    success_or_error = tcp_server->listen();
    if (success_or_error.is_error()) {
        outln("listen: {}", success_or_error.error());
        return 1;
    }

    auto timer = MUST(Event::Timer::create_repeating(Time::from_seconds(2)));

    int count = 0;
    timer->on_action = [&] {
        auto stream_or_error = Event::TcpStream::connect(ip, port);
        if (stream_or_error.is_error()) {
            outln("connect: {}", stream_or_error.error());
            return;
        }
        auto stream = stream_or_error.release_value();

        auto test_fd = open("/dev/null", O_RDONLY);
        if (test_fd == -1)
            test_fd = 2;

        auto object = adopt_ref(*new TestObject(MUST(String::formatted("Hello: {}", count++)), test_fd));

        MUST(Leaf::Object::write<TestObject>(stream, object));
    };

    return event_loop->exec();
}

int main()
{
    auto event_loop = MUST(Event::EventLoop::create());

    auto timer = MUST(Event::Timer::create_repeating(Time::from_seconds(1)));
    timer->on_action = [&] {
        outln("Vigdis er kul");
    };

    return event_loop->exec();
}