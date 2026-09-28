#include "librecomp/fault_guard.hpp"

#ifndef _WIN32
#include <atomic>
#include <csetjmp>
#include <csignal>
#include <mutex>
#include <pthread.h>

// A host-side fault inside a guarded call skips destructors between the fault and run() (held locks stay held), as SEH does on Windows.
namespace {
    struct Frame {
        sigjmp_buf jmp;
        recomp::fault_guard::Info* info;
    };

    // pthread_getspecific instead of thread_local: bionic may allocate TLS lazily (malloc), which is not safe in a signal handler.
    pthread_key_t g_frame_key;
    std::atomic<int> g_active_guards{0};
    struct sigaction g_prev_segv{};
    struct sigaction g_prev_bus{};

    // Faults outside a guarded call go to whoever was installed before us (ART, debuggerd, default).
    void chain(int sig, siginfo_t* si, void* uc) {
        const struct sigaction& prev = (sig == SIGBUS) ? g_prev_bus : g_prev_segv;
        if (prev.sa_flags & SA_SIGINFO) {
            prev.sa_sigaction(sig, si, uc);
            return;
        }
        if (prev.sa_handler != SIG_DFL && prev.sa_handler != SIG_IGN) {
            prev.sa_handler(sig);
            return;
        }
        signal(sig, SIG_DFL);
        raise(sig);
    }

    void handler(int sig, siginfo_t* si, void* uc) {
        Frame* frame = (g_active_guards.load(std::memory_order_relaxed) != 0) ? static_cast<Frame*>(pthread_getspecific(g_frame_key)) : nullptr;
        if (frame == nullptr) {
            chain(sig, si, uc);
            return;
        }
        if (frame->info != nullptr) {
            frame->info->signo = sig;
            frame->info->addr = reinterpret_cast<uintptr_t>(si->si_addr);
        }
        siglongjmp(frame->jmp, 1);
    }

    void install_once() {
        static std::once_flag once;
        std::call_once(once, [] {
            pthread_key_create(&g_frame_key, nullptr);
            struct sigaction sa{};
            sa.sa_sigaction = handler;
            sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGSEGV, &sa, &g_prev_segv);
            sigaction(SIGBUS, &sa, &g_prev_bus);
        });
    }
}

bool recomp::fault_guard::run(void (*fn)(void*), void* arg, Info* info) {
    install_once();
    // Restores the outer frame however run() exits, including a C++ throw (thread_terminated) out of fn.
    struct Restore {
        void* outer = pthread_getspecific(g_frame_key);
        ~Restore() {
            pthread_setspecific(g_frame_key, outer);
            g_active_guards.fetch_sub(1, std::memory_order_relaxed);
        }
    } restore;
    g_active_guards.fetch_add(1, std::memory_order_relaxed);
    Frame frame;
    frame.info = info;
    if (sigsetjmp(frame.jmp, 1) != 0) {
        return false;
    }
    pthread_setspecific(g_frame_key, &frame);
    fn(arg);
    return true;
}
#endif
