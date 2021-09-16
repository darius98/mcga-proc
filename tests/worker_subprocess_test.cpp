#include <chrono>
#include <thread>

#include <mcga/test.hpp>

#include "mcga/proc/worker_subprocess.hpp"

using namespace mcga::test;
using namespace mcga::proc;
using namespace std;
using std::chrono::high_resolution_clock;
using std::this_thread::sleep_for;

TEST_CASE(WorkerSubprocessTest, "Worker subprocess") {
    group("Send a message, then die", [] {
        std::unique_ptr<WorkerSubprocess> proc;

        setUp([&] {
            proc = std::make_unique<WorkerSubprocess>(
              50ms, [](std::unique_ptr<PipeWriter> writer) {
                  writer->sendMessage(2, 0, 1, 9);
              });
            sleep_for(50ms);
        });

        tearDown([&] { proc.reset(); });

        test("isFinished() == true", [&] { expect(proc->isFinished()); });

        test("kill() == ALREADY_DEAD",
             [&] { expect(proc->kill() == Subprocess::ALREADY_DEAD); });

        test("isExited() == true", [&] { expect(proc->isExited()); });

        test("getReturnCode() == 0",
             [&] { expect(proc->getReturnCode() == 0); });

        test("isSignaled() == false", [&] { expect(!proc->isSignaled()); });

        test("getFinishStatus() == ZERO_EXIT",
             [&] { expect(proc->getFinishStatus() == Subprocess::ZERO_EXIT); });

        test("getNextMessage(32) is the sent message", [&] {
            auto message = proc->getNextMessage(32);
            expect(!message.isInvalid(), "Message is invalid.");
            int a, b, c, d;
            message >> a >> b >> c >> d;
            expect(a == 2 && b == 0 && c == 1 && d == 9,
                   "Message content is invalid.");
        });
    });

    group("Do nothing and die", [] {
        std::unique_ptr<WorkerSubprocess> proc;

        // TODO: Add cleanup() functionality to mcga::test. This shouldn't be a
        //  group!
        tearDown([&] { proc.reset(); });

        test("getNextMessage(32) is invalid", [&] {
            proc = std::make_unique<WorkerSubprocess>(
              50ms, [](std::unique_ptr<PipeWriter>) {});
            sleep_for(50ms);
            expect(proc->getNextMessage(32).isInvalid());
        });
    });

    group("Timeout", [] {
        std::unique_ptr<WorkerSubprocess> proc;

        tearDown([&] { proc.reset(); });

        test("getFinishStatus()==TIMEOUT", [&] {
            proc = std::make_unique<WorkerSubprocess>(
              50ms, [](std::unique_ptr<PipeWriter>) {
                  auto endTime = high_resolution_clock::now() + 200ms;
                  volatile int spins = 0;
                  while (high_resolution_clock::now() <= endTime) {
                      spins += 1;
                  }
              });
            sleep_for(100ms);
            expect(proc->getFinishStatus() == Subprocess::TIMEOUT);
        });
    });
}
