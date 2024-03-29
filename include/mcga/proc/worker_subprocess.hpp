#pragma once

#include <chrono>
#include <memory>
#include <utility>

#include "pipe.hpp"
#include "subprocess.hpp"

namespace mcga::proc {

class WorkerSubprocess : public Subprocess {
  public:
    template<class Work>
    WorkerSubprocess(const std::chrono::nanoseconds& timeLimit, Work&& work)
            : startTime(std::chrono::high_resolution_clock::now()),
              timeLimit(timeLimit) {
        auto [reader, writer] = createAnonymousPipe();
        pipeReader = std::move(reader);
        subprocess
          = Subprocess::Fork([writer = std::move(writer),
                              work = std::forward<Work>(work)]() mutable {
                std::forward<Work>(work)(std::move(writer));
            });
        writer.reset();
    }

    WorkerSubprocess(WorkerSubprocess&& other) noexcept
            : subprocess(std::move(other.subprocess)),
              pipeReader(std::move(other.pipeReader)),
              startTime(other.startTime), timeLimit(other.timeLimit) {
    }

    WorkerSubprocess(const WorkerSubprocess& other) = delete;

    ~WorkerSubprocess() override = default;

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

    void waitBlocking() override {
        subprocess->waitBlocking();
    }

    Message getNextMessage(int maxConsecutiveFailedReadAttempts = -1) {
        return pipeReader->getNextMessage(maxConsecutiveFailedReadAttempts);
    }

  private:
    std::unique_ptr<Subprocess> subprocess;
    std::unique_ptr<PipeReader> pipeReader;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::duration timeLimit;
};

}  // namespace mcga::proc
