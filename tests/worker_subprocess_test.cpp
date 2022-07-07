#include <chrono>
#include <thread>

#include <mcga/test.hpp>
#include <mcga/test_ext/matchers.hpp>

#include "mcga/proc/worker_subprocess.hpp"

using namespace mcga::proc;

constexpr auto fifty_ms = std::chrono::milliseconds(50);

TEST_CASE("Worker subprocess") {
    test("Send a message, then die", [] {
        auto proc = new WorkerSubprocess(
          fifty_ms, [](std::unique_ptr<PipeWriter> writer) {
              writer->sendMessage(2, 0, 1, 9);
          });
        cleanup([&] {
            proc->kill();
            delete proc;
        });
        std::this_thread::sleep_for(fifty_ms);
        expect(proc->isFinished());
        expect(proc->kill() == Subprocess::ALREADY_DEAD);
        expect(proc->isExited());
        expect(proc->getReturnCode() == 0);
        expect(!proc->isSignaled());
        expect(proc->getFinishStatus() == Subprocess::ZERO_EXIT);
        auto message = proc->getNextMessage(32);
        expect(!message.isInvalid());
        int a, b, c, d;
        message >> a >> b >> c >> d;
        expect(a, 2);
        expect(b, 0);
        expect(c, 1);
        expect(d, 9);
    });

    test("Do nothing and die: getNextMessage(32) is invalid", [&] {
        auto proc
          = new WorkerSubprocess(fifty_ms, [](std::unique_ptr<PipeWriter>) {});
        cleanup([&] {
            proc->kill();
            delete proc;
        });
        std::this_thread::sleep_for(fifty_ms);
        expect(proc->getNextMessage(32).isInvalid());
    });

    test("Timeout: getFinishStatus()==TIMEOUT", [&] {
        auto proc
          = new WorkerSubprocess(fifty_ms, [](std::unique_ptr<PipeWriter>) {
                auto endTime
                  = std::chrono::high_resolution_clock::now() + 4 * fifty_ms;
                std::atomic_int spins = 0;
                while (std::chrono::high_resolution_clock::now() <= endTime) {
                    spins += 1;
                }
            });
        cleanup([&] {
            proc->kill();
            delete proc;
        });
        std::this_thread::sleep_for(2 * fifty_ms);
        expect(proc->getFinishStatus(), Subprocess::TIMEOUT);
    });
}
