#include <mcga/test.hpp>
#include <mcga/test_ext/matchers.hpp>

#include "mcga/proc/pipe.hpp"

using namespace mcga::test;
using namespace mcga::test::matchers;
using namespace mcga::proc;
using namespace std;

TEST_CASE(PipeTest, "Pipe") {
    PipeReader* reader = nullptr;
    PipeWriter* writer = nullptr;

    setUp([&] { tie(reader, writer) = createAnonymousPipe(); });

    tearDown([&] {
        delete reader;
        delete writer;
        reader = nullptr;
        writer = nullptr;
    });

    test("Sending one message", [&] {
        writer->sendMessage(1, 2, 3);

        Message message = reader->getNextMessage();
        int x;
        int y;
        int z;
        expect(message.isInvalid(), isFalse);
        message >> x >> y >> z;
        expect(x, isEqualTo(1));
        expect(y, isEqualTo(2));
        expect(z, isEqualTo(3));
    });

    test("Sending 100 messages, reading one by one", [&] {
        int x;
        int y;
        int z;
        Message message;

        for (int i = 1; i <= 100; ++i) {
            writer->sendMessage(3 * i - 2, 3 * i - 1, 3 * i);
            message = reader->getNextMessage();
            expect(!message.isInvalid(),
                   "Message " + to_string(i) + "is invalid!");
            message >> x >> y >> z;
            expect(x, isEqualTo(3 * i - 2));
            expect(y, isEqualTo(3 * i - 1));
            expect(z, isEqualTo(3 * i));
        }
    });

    test("Sending 100 messages, reading them 4 at a time", [&] {
        int x;
        int y;
        int z;
        Message message;

        for (int i = 1; i <= 100; ++i) {
            writer->sendMessage(3 * i - 2, 3 * i - 1, 3 * i);

            if (i % 4 == 0) {
                for (int j = i - 3; j <= i; ++j) {
                    message = reader->getNextMessage();
                    expect(!message.isInvalid(),
                           "Message " + to_string(j) + "is invalid!");
                    message >> x >> y >> z;
                    expect(x, isEqualTo(3 * j - 2));
                    expect(y, isEqualTo(3 * j - 1));
                    expect(z, isEqualTo(3 * j));
                }
            }
        }
    });

    test("Sending 100 messages, reading all of them afterwards", [&] {
        for (int i = 1; i <= 100; ++i) {
            writer->sendMessage(3 * i - 2, 3 * i - 1, 3 * i);
        }

        for (int i = 1; i <= 100; ++i) {
            auto message = reader->getNextMessage();
            expect(!message.isInvalid(),
                   "Message " + to_string(i) + " is invalid!");
            int x, y, z;
            message >> x >> y >> z;
            expect(x, isEqualTo(3 * i - 2));
            expect(y, isEqualTo(3 * i - 1));
            expect(z, isEqualTo(3 * i));
        }
    });

    test("Reading a message without writing one", [&] {
        Message message = reader->getNextMessage(100);  // 100 read attempts
        expect(message.isInvalid(), isTrue);
    });
}
