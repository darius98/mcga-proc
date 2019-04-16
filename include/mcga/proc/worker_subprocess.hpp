#pragma once

#include <chrono>

#include <mcga/proc/pipe.hpp>
#include <mcga/proc/subprocess.hpp>

namespace mcga::proc {

class WorkerSubprocess : public Subprocess {
  public:
    typedef const std::function<void(PipeWriter*)>& Work;

    WorkerSubprocess(const std::chrono::nanoseconds& timeLimit, Work run);

    WorkerSubprocess(WorkerSubprocess&& other) noexcept;

    WorkerSubprocess(const WorkerSubprocess& other) = delete;

    ~WorkerSubprocess() override;

    bool isFinished() override;

    KillResult kill() override;

    bool isExited() override;

    int getReturnCode() override;

    bool isSignaled() override;

    int getSignal() override;

    FinishStatus getFinishStatus() override;

    Message getNextMessage(int maxConsecutiveFailedReadAttempts = -1);

  private:
    Subprocess* subprocess;
    PipeReader* pipeReader;
    std::chrono::high_resolution_clock::time_point endTime;
};

}  // namespace mcga::proc
