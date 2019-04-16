#include <mcga/proc/worker_subprocess.hpp>

using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::nanoseconds;

namespace mcga::proc {

WorkerSubprocess::WorkerSubprocess(const nanoseconds& timeLimit, Work run)
        : endTime(high_resolution_clock::now() + timeLimit) {
    auto pipe = createAnonymousPipe();
    subprocess = Subprocess::Fork([&pipe, &run]() {
        delete pipe.first;
        run(pipe.second);
        delete pipe.second;
    });
    pipeReader = pipe.first;

    delete pipe.second;
}

WorkerSubprocess::WorkerSubprocess(WorkerSubprocess&& other) noexcept
        : subprocess(other.subprocess), pipeReader(other.pipeReader),
          endTime(other.endTime) {
    other.subprocess = nullptr;
    other.pipeReader = nullptr;
}

WorkerSubprocess::~WorkerSubprocess() {
    delete subprocess;
    delete pipeReader;
}

Message WorkerSubprocess::getNextMessage(int maxConsecutiveFailedReadAttempts) {
    return pipeReader->getNextMessage(maxConsecutiveFailedReadAttempts);
}

bool WorkerSubprocess::isFinished() {
    return subprocess->isFinished();
}

Subprocess::KillResult WorkerSubprocess::kill() {
    return subprocess->kill();
}

bool WorkerSubprocess::isExited() {
    return subprocess->isExited();
}

int WorkerSubprocess::getReturnCode() {
    return subprocess->getReturnCode();
}

bool WorkerSubprocess::isSignaled() {
    return subprocess->isSignaled();
}

int WorkerSubprocess::getSignal() {
    return subprocess->getSignal();
}

Subprocess::FinishStatus WorkerSubprocess::getFinishStatus() {
    if (!isFinished()) {
        if (high_resolution_clock::now() <= endTime) {
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

}  // namespace mcga::proc
