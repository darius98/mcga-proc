#include <mcga/test.hpp>

#include <csignal>
#include <iostream>

#include <array>
#include <thread>

#include "mcga/proc/subprocess.hpp"

using namespace mcga::proc;

TEST_CASE("Subprocess") {
    test("Fork into process doing nothing, after 50ms", [] {
        auto proc = Subprocess::Fork([] {});
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        expect(proc->isFinished());
        expect(proc->isExited());
        expect(proc->getReturnCode() == 0);
        expect(!proc->isSignaled());
        expect(proc->kill() == Subprocess::ALREADY_DEAD);
        expect(proc->getFinishStatus() == Subprocess::ZERO_EXIT);
    });

    test("Fork into process exiting with code 17, after 50ms", [] {
        auto proc = Subprocess::Fork([] {
            exit(17);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        expect(proc->isFinished());
        expect(proc->isExited());
        expect(proc->getReturnCode() == 17);
        expect(!proc->isSignaled());
        expect(proc->kill() == Subprocess::ALREADY_DEAD);
        expect(proc->getFinishStatus() == Subprocess::NON_ZERO_EXIT);
    });

    test("Fork into KBS SIGINT process, after 50ms", [&] {
        auto proc = Subprocess::Fork([] {
            raise(SIGINT);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        expect(proc->isFinished());
        expect(!proc->isExited());
        expect(proc->isSignaled());
        expect(proc->getSignal() == SIGINT);
        expect(proc->kill() == Subprocess::ALREADY_DEAD);
        expect(proc->getFinishStatus() == Subprocess::SIGNAL_EXIT);
    });

    test("Fork into KBS SIGINT process, waitBlocking", [&] {
        auto proc = Subprocess::Fork([] {
            raise(SIGINT);
        });
        expect(!proc->isFinished());
        proc->waitBlocking();
        expect(proc->isFinished());
        expect(!proc->isExited());
        expect(proc->isSignaled());
        expect(proc->getSignal() == SIGINT);
        expect(proc->kill() == Subprocess::ALREADY_DEAD);
        expect(proc->getFinishStatus() == Subprocess::SIGNAL_EXIT);

        proc->waitBlocking();
        expect(proc->isFinished());
        expect(!proc->isExited());
        expect(proc->isSignaled());
        expect(proc->getSignal() == SIGINT);
        expect(proc->kill() == Subprocess::ALREADY_DEAD);
        expect(proc->getFinishStatus() == Subprocess::SIGNAL_EXIT);
    });

    test("Fork into KBS SIGINT process, waitBlocking after waiting 50ms", [&] {
        auto proc = Subprocess::Fork([] {
            raise(SIGINT);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        proc->waitBlocking();
        expect(proc->isFinished());
        expect(!proc->isExited());
        expect(proc->isSignaled());
        expect(proc->getSignal() == SIGINT);
        expect(proc->kill() == Subprocess::ALREADY_DEAD);
        expect(proc->getFinishStatus() == Subprocess::SIGNAL_EXIT);

        proc->waitBlocking();
        expect(proc->isFinished());
        expect(!proc->isExited());
        expect(proc->isSignaled());
        expect(proc->getSignal() == SIGINT);
        expect(proc->kill() == Subprocess::ALREADY_DEAD);
        expect(proc->getFinishStatus() == Subprocess::SIGNAL_EXIT);
    });

    test("Fork into infinite spin process, after 50ms", [&] {
        auto proc = Subprocess::Fork([] {
            std::atomic_int spins = 0;
            while (spins >= 0) {
                spins += 1;
            }
        });
        cleanup([&] {
            // TODO: ASan on MacOS says there's an error here somewhere.
            //  Sometimes proc is nullptr when it gets here, other times
            //  there's a BUS error when dereferencing Executable::te_call
            //  from the VTable.
            proc->kill();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        expect(!proc->isFinished());
        expect(!proc->isExited());
        expect(!proc->isSignaled());
        expect(proc->kill() == Subprocess::KILLED);
        expect(proc->getFinishStatus() == Subprocess::NO_EXIT);
    });

    test("Invoke sleep", [&] {
        std::string sleep = "/bin/sleep";
        std::string time = "0.1";
        std::array<char* const, 3> argv{
          sleep.data(),
          time.data(),
          nullptr,
        };
        std::array<char* const, 1> envp{
          nullptr,
        };
        std::unique_ptr<Subprocess> proc
          = Subprocess::Invoke(sleep.data(), argv.data(), envp.data());
        expect(!proc->isFinished());
        expect(!proc->isExited());
        expect(!proc->isSignaled());
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        expect(proc->isFinished());
        expect(proc->isExited());
        expect(proc->getReturnCode() == 0);
    });
}
