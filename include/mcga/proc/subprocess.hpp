#pragma once

#include <functional>

namespace mcga::proc {

class Subprocess {
  public:
    enum FinishStatus {
        NO_EXIT,
        ZERO_EXIT,
        NON_ZERO_EXIT,
        TIMEOUT,
        SIGNAL_EXIT,
    };

    enum KillResult { KILLED, ALREADY_DEAD };

    static Subprocess* Fork(const std::function<void()>& func);

    virtual ~Subprocess() = default;

    virtual bool isFinished() = 0;

    virtual bool isExited() = 0;

    virtual int getReturnCode() = 0;

    virtual bool isSignaled() = 0;

    virtual int getSignal() = 0;

    virtual KillResult kill() = 0;

    virtual FinishStatus getFinishStatus() = 0;
};

}  // namespace mcga::proc
