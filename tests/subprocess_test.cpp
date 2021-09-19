#include <mcga/test.hpp>

#include <csignal>

#include <thread>

#include "mcga/proc/subprocess.hpp"

using namespace mcga::test;
using namespace mcga::proc;

TEST_CASE("Subprocess") {
    group("Fork into process doing nothing, after 50ms", [] {
        std::unique_ptr<Subprocess> proc;

        setUp([&proc] {
            proc = Subprocess::Fork([] {});
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        });

        tearDown([&proc] { proc.reset(); });

        test("isFinished() == true", [&] { expect(proc->isFinished()); });

        test("isExited() == true", [&] { expect(proc->isExited()); });

        test("getReturnCode() == 0",
             [&] { expect(proc->getReturnCode() == 0); });

        test("isSignaled() == false", [&] { expect(!proc->isSignaled()); });

        test("kill() == ALREADY_DEAD",
             [&] { expect(proc->kill() == Subprocess::ALREADY_DEAD); });

        test("getFinishStatus() == ZERO_EXIT",
             [&] { expect(proc->getFinishStatus() == Subprocess::ZERO_EXIT); });
    });

    group("Fork into process exiting with code 17, after 50ms", [] {
        std::unique_ptr<Subprocess> proc;

        setUp([&proc] {
            proc = Subprocess::Fork([] { exit(17); });
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        });

        tearDown([&proc] { proc.reset(); });

        test("isFinished() == true", [&] { expect(proc->isFinished()); });

        test("isExited() == true", [&] { expect(proc->isExited()); });

        test("getReturnCode() == 17",
             [&] { expect(proc->getReturnCode() == 17); });

        test("isSignaled() == false", [&] { expect(!proc->isSignaled()); });

        test("kill() == ALREADY_DEAD",
             [&] { expect(proc->kill() == Subprocess::ALREADY_DEAD); });

        test("getFinishStatus() == NON_ZERO_EXIT", [&] {
            expect(proc->getFinishStatus() == Subprocess::NON_ZERO_EXIT);
        });
    });

    group("Fork into KBS SIGINT process, after 50ms", [] {
        std::unique_ptr<Subprocess> proc;

        setUp([&proc] {
            proc = Subprocess::Fork([] { raise(SIGINT); });
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        });

        tearDown([&proc] { proc.reset(); });

        test("isFinished() == true", [&] { expect(proc->isFinished()); });

        test("isExited() == false", [&] { expect(!proc->isExited()); });

        test("isSignaled() == true", [&] { expect(proc->isSignaled()); });

        test("getSignal() == SIGINT",
             [&] { expect(proc->getSignal() == SIGINT); });

        test("kill() == ALREADY_DEAD",
             [&] { expect(proc->kill() == Subprocess::ALREADY_DEAD); });

        test("getFinishStatus() == SIGNAL_EXIT", [&] {
            expect(proc->getFinishStatus() == Subprocess::SIGNAL_EXIT);
        });
    });

    group("Fork into infinite spin process, after 50ms", [&] {
        std::unique_ptr<Subprocess> proc;

        setUp([&proc] {
            proc = Subprocess::Fork([] {
                std::atomic_int spins = 0;
                while (spins >= 0) {
                    spins += 1;
                }
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        });

        tearDown([&proc] {
            proc->kill();
            proc.reset();
        });

        test("isFinished() == false", [&] { expect(!proc->isFinished()); });

        test("isExited() == false", [&] { expect(!proc->isExited()); });

        test("isSignaled() == false", [&] { expect(!proc->isSignaled()); });

        test("kill() == KILLED",
             [&] { expect(proc->kill() == Subprocess::KILLED); });

        test("getFinishStatus() == NO_EXIT",
             [&] { expect(proc->getFinishStatus() == Subprocess::NO_EXIT); });
    });
}
