#include <cstring>

#include <mcga/test.hpp>
#include <mcga/test_ext/matchers.hpp>

#include "mcga/proc/message.hpp"
#include "mcga/proc/serialization_std.hpp"

using namespace mcga::matchers;
using namespace mcga::proc;

template<class... Args>
Message buildMessage(const Args&... args) {
    char buffer[128];
    Message::Write([buf = buffer](const void* data, std::size_t size) mutable {
        std::memcpy(buf, data, size);
        buf += size;
    }, args...);
    return Message::Read(buffer, 128);
}

TEST_CASE("Message") {
    test("Building & reading a message from 3 ints", [] {
        auto message = buildMessage(1, 2, 3);
        int x, y, z;
        message >> x >> y >> z;
        expect(message.isInvalid(), isFalse);
        expect(x, isEqualTo(1));
        expect(y, isEqualTo(2));
        expect(z, isEqualTo(3));
    });

    test("Building & reading a message containing ints and doubles", [] {
        int x = 1;
        long long y = 2;
        double z = 3.0;
        char t = 'a';
        auto message = buildMessage(x, y, z, t);
        int a;
        long long b;
        double c;
        char d;
        message >> a >> b >> c >> d;
        expect(message.isInvalid(), isFalse);
        expect(a, isEqualTo(x));
        expect(b, isEqualTo(y));
        expect(c, isEqualTo(z));
        expect(d, isEqualTo(t));
    });

    test("Building & reading a message containing strings", [] {
        std::string s = "abc";
        int r = 5;
        std::string t = "def";
        auto message = buildMessage(s, r, t);
        expect(message.isInvalid(), isFalse);
        std::string a;
        int b;
        std::string c;
        message >> a >> b >> c;
        expect(a, isEqualTo(s));
        expect(b, isEqualTo(r));
        expect(c, isEqualTo(t));
    });

    test("Default constructor provides an invalid message", [] {
        Message message;
        expect(message.isInvalid(), isTrue);
    });

    test("Copying", [] {
        int a = 2;
        int b = 4;
        std::string s = "abc";
        auto message = buildMessage(a, b, s);

        expect(message.isInvalid(), isFalse);

        int x;
        int y;
        std::string t;
        message >> x >> y >> t;
        expect(x, isEqualTo(a));
        expect(y, isEqualTo(b));
        expect(t, isEqualTo(s));

        Message messageCopy(message);
        expect(messageCopy.isInvalid(), isFalse);
        int x2;
        int y2;
        std::string t2;
        messageCopy >> x2 >> y2 >> t2;
        expect(x2, isEqualTo(a));
        expect(y2, isEqualTo(b));
        expect(t2, isEqualTo(s));

        messageCopy = message;
        expect(messageCopy.isInvalid(), isFalse);
        int x3;
        int y3;
        std::string t3;
        messageCopy >> x3 >> y3 >> t3;
        expect(x3, isEqualTo(a));
        expect(y3, isEqualTo(b));
        expect(t3, isEqualTo(s));
    });

    test("Messages are only equal if both invalid or same object", [] {
        Message message1;
        Message message2;
        expect(message1 == message2, isTrue);

        auto a = buildMessage(1, 2, 3);
        auto b = buildMessage(1, 2, 3);

        auto& aRef = a;
        expect(a == aRef, isTrue);

        auto& aRef2 = a;
        expect(aRef == aRef2, isTrue);

        expect(a == b, isFalse);
    });

    // This test kind of uses internals of the message, but it's a lot easier
    // to test this here instead of in the PipeReader class.
    test("Reading a message from a buffer", [] {
        uint8_t buffer[100];
        size_t bufferSize = 100;
        size_t messageSize = sizeof(int);
        int messageContent = 42;
        memcpy(buffer, &messageSize, sizeof(size_t));
        memcpy(
          buffer + alignof(std::max_align_t), &messageContent, sizeof(int));
        Message message = Message::Read(buffer, bufferSize);
        expect(message.isInvalid(), isFalse);
        int actualContent;
        message >> actualContent;
        expect(actualContent, isEqualTo(messageContent));
    });
}
