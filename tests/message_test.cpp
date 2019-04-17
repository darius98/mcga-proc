#include <cstring>

#include <mcga/test.hpp>
#include <mcga/test_ext/matchers.hpp>

#include <mcga/proc/message.hpp>

using namespace mcga::test;
using namespace mcga::test::matchers;
using namespace mcga::proc;
using namespace std;

TEST_CASE(MessageTest, "Message") {
    test("Building & reading a message from 3 ints", [] {
        auto message = Message::Build(1, 2, 3);
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
        auto message = Message::Build(x, y, z, t);
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
        string s = "abc";
        int r = 5;
        string t = "def";
        auto message = Message::Build(s, r, t);
        expect(message.isInvalid(), isFalse);
        string a;
        int b;
        string c;
        message >> a >> b >> c;
        expect(a, isEqualTo(s));
        expect(b, isEqualTo(r));
        expect(c, isEqualTo(t));
    });

    test("Default constructor provides an invalid message", [] {
        Message message;
        expect(message.isInvalid(), isTrue);
    });

    test("Message::INVALID is invalid",
         [] { expect(Message::INVALID.isInvalid(), isTrue); });

    test("Copying", [] {
        int a = 2;
        int b = 4;
        string s = "abc";
        auto message = Message::Build(a, b, s);

        expect(message.isInvalid(), isFalse);

        int x;
        int y;
        string t;
        message >> x >> y >> t;
        expect(x, isEqualTo(a));
        expect(y, isEqualTo(b));
        expect(t, isEqualTo(s));

        Message messageCopy(message);
        expect(messageCopy.isInvalid(), isFalse);
        int x2;
        int y2;
        string t2;
        messageCopy >> x2 >> y2 >> t2;
        expect(x2, isEqualTo(a));
        expect(y2, isEqualTo(b));
        expect(t2, isEqualTo(s));

        messageCopy = message;
        expect(messageCopy.isInvalid(), isFalse);
        int x3;
        int y3;
        string t3;
        messageCopy >> x3 >> y3 >> t3;
        expect(x3, isEqualTo(a));
        expect(y3, isEqualTo(b));
        expect(t3, isEqualTo(s));
    });

    test("Messages are only equal if both invalid or same object", [] {
        Message message1(Message::INVALID);
        Message message2(Message::INVALID);
        expect(message1 == message2, isTrue);

        auto a = Message::Build(1, 2, 3);
        auto b = Message::Build(1, 2, 3);

        auto& aRef = a;
        expect(a == aRef, isTrue);

        auto& aRef2 = a;
        expect(aRef == aRef2, isTrue);

        expect(a == b, isFalse);
    });

    // This test kind of uses internals of the message, but it's a lot easier
    // to test this here instead of in the PipeReader class.
    test("Reading a message from a buffer", [] {
        void* buffer = malloc(100);
        size_t bufferSize = 100;
        size_t messageSize = sizeof(int);
        int messageContent = 42;
        memcpy(buffer, &messageSize, sizeof(size_t));
        memcpy(static_cast<uint8_t*>(buffer) + sizeof(size_t),
               &messageContent,
               sizeof(int));
        Message message = Message::Read(buffer, bufferSize);
        expect(message.isInvalid(), isFalse);
        int actualContent;
        message >> actualContent;
        expect(actualContent, isEqualTo(messageContent));
    });
}
