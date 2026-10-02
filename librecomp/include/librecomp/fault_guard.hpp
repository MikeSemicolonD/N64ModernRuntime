#ifndef __RECOMP_FAULT_GUARD_HPP__
#define __RECOMP_FAULT_GUARD_HPP__

#include <cstdint>

// POSIX counterpart of the Windows SEH wrapper around recompiled thread entries.
namespace recomp::fault_guard {
    struct Info {
        int signo;
        uintptr_t addr;
        // Host PC of the faulting instruction (0 where the platform is not handled).
        uintptr_t pc;
    };

    // Runs fn(arg). Returns false if a SIGSEGV/SIGBUS was raised inside it (info filled), true otherwise.
    // Faults outside a guarded call are forwarded to the previously installed handler.
    bool run(void (*fn)(void*), void* arg, Info* info);
}

#endif
