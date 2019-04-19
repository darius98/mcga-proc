#pragma once

#include <chrono>

#include "pipe.hpp"
#include "subprocess.hpp"

namespace mcga::proc {

class WorkerSubprocess : public Subprocess {
  public:
    using Work = const std::function<void(PipeWriter*)>&;

    WorkerSubprocess(const std::chrono::nanoseconds& timeLimit, Work run)
            : startTime(std::chrono::high_resolution_clock::now()),
              timeLimit(timeLimit) {
        auto pipe = createAnonymousPipe();
        subprocess = Subprocess::Fork([&pipe, &run]() {
            delete pipe.first;
            run(pipe.second);
            delete pipe.second;
        });
        pipeReader = pipe.first;

        delete pipe.second;
    }

    WorkerSubprocess(WorkerSubprocess&& other) noexcept
            : subprocess(other.subprocess), pipeReader(other.pipeReader),
              startTime(other.startTime), timeLimit(other.timeLimit) {
        other.subprocess = nullptr;
        other.pipeReader = nullptr;
    }

    WorkerSubprocess(const WorkerSubprocess& other) = delete;

    ~WorkerSubprocess() override {
        delete subprocess;
        delete pipeReader;
    }

    std::chrono::nanoseconds elapsedTime() const {
        return std::chrono::high_resolution_clock::now() - startTime;
    }

    bool isFinished() override {
        return subprocess->isFinished();
    }

    Subprocess::KillResult kill() override {
        return subprocess->kill();
    }

    bool isExited() override {
        return subprocess->isExited();
    }

    int getReturnCode() override {
        return subprocess->getReturnCode();
    }

    bool isSignaled() override {
        return subprocess->isSignaled();
    }

    int getSignal() override {
        return subprocess->getSignal();
    }

    Subprocess::FinishStatus getFinishStatus() override {
        if (!isFinished()) {
            if (elapsedTime() < timeLimit) {
                return NO_EXIT;
            }
            auto killStatus = kill();
            if (killStatus == Subprocess::ALREADY_DEAD) {
                // The child might have finished during a context switch.
                // In this case, return false so we can retry waiting it later.
                return NO_EXIT;
            }
            return TIMEOUT;
        }
        return subprocess->getFinishStatus();
    }

    Message getNextMessage(int maxConsecutiveFailedReadAttempts = -1) {
        return pipeReader->getNextMessage(maxConsecutiveFailedReadAttempts);
    }

  private:
    Subprocess* subprocess;
    PipeReader* pipeReader;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::duration timeLimit;
};

}  // namespace mcga::proc
