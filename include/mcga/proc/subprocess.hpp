#pragma once

#include <memory>

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

    static std::unique_ptr<Subprocess> Fork(auto&& callable);

    static std::unique_ptr<Subprocess>
      Invoke(char* exe, char* const* argv, char* const* envp = nullptr);

    virtual ~Subprocess() = default;

    virtual bool isFinished() = 0;

    virtual bool isExited() = 0;

    virtual int getReturnCode() = 0;

    virtual bool isSignaled() = 0;

    virtual int getSignal() = 0;

    virtual KillResult kill() = 0;

    virtual FinishStatus getFinishStatus() = 0;

    virtual void waitBlocking() = 0;
};

}  // namespace mcga::proc

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include "subprocess_posix.hpp"
#else
#error "Non-unix systems are not currently supported by mcga::proc."
#endif
