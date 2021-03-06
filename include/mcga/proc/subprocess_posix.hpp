#pragma once

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>

#include <system_error>

namespace mcga::proc::internal {

class PosixSubprocessHandler : public Subprocess {
  public:
    explicit PosixSubprocessHandler(pid_t pid): pid(pid) {
    }

    bool isFinished() override {
        if (killed || finished) {
            return true;
        }
        int wStatus;
        int ret = waitpid(pid, &wStatus, WNOHANG);
        if (ret < 0) {
            throw std::system_error(
              errno, std::generic_category(), "PosixSubprocessHandler:waitpid");
        }
        if (ret == 0) {
            return false;
        }
        finished = true;
        lastWaitStatus = wStatus;
        return true;
    }

    KillResult kill() override {
        if (isFinished()) {
            return ALREADY_DEAD;
        }
        int killStatus = ::kill(pid, SIGKILL);
        if (killStatus < 0) {
            if (errno == ESRCH) {
                return ALREADY_DEAD;
            }
            throw std::system_error(
              errno, std::generic_category(), "PosixSubprocessHandler:kill");
        }
        return KILLED;
    }

    bool isExited() override {
        return isFinished() && WIFEXITED(lastWaitStatus);
    }

    int getReturnCode() override {
        if (isExited()) {
            return WEXITSTATUS(lastWaitStatus);
        }
        return -1;
    }

    bool isSignaled() override {
        return isFinished() && WIFSIGNALED(lastWaitStatus);
    }

    int getSignal() override {
        if (isSignaled()) {
            return WTERMSIG(lastWaitStatus);
        }
        return -1;
    }

    FinishStatus getFinishStatus() override {
        if (!isFinished()) {
            return NO_EXIT;
        }
        if (WIFSIGNALED(lastWaitStatus)) {
            return SIGNAL_EXIT;
        }
        if (WEXITSTATUS(lastWaitStatus) != 0) {
            return NON_ZERO_EXIT;
        }
        return ZERO_EXIT;
    }

  private:
    pid_t pid;
    bool killed = false;
    bool finished = false;
    int lastWaitStatus = 0;
};

} // namespace mcga::proc::internal

namespace mcga::proc {

inline Subprocess* Subprocess::Fork(const std::function<void()>& func) {
    pid_t forkPid = fork();
    if (forkPid < 0) {
        throw std::system_error(
          errno, std::generic_category(), "PosixSubprocessHandler:fork");
    }
    if (forkPid == 0) {  // child process
        func();
        exit(0);
    }
    return new internal::PosixSubprocessHandler(forkPid);
}

}  // namespace mcga::proc
