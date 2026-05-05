#include <bitset>
#include <thread>
#include <atomic>
#include <cstdio>
#include <unordered_map>
#include <mutex>

#include "blockingconcurrentqueue.h"

#include "ultramodern/ultra64.h"
#include "ultramodern/ultramodern.hpp"

namespace mqdiag {
    // Per-source indices: 0=Timer,1=Sp,2=Si,3=Ai,4=Vi,5=Pi,6=Dp,7=Untagged
    static constexpr int kNumSrc = 8;
    static const char *kSrcName[kNumSrc] = { "Tmr", "Sp", "Si", "Ai", "Vi", "Pi", "Dp", "Unt" };
    struct Counts {
        std::atomic<uint64_t> sends{0}, recvs{0}, external{0}, delivered{0}, drop_blocked{0};
        std::atomic<uint64_t> blocked_lost{0}, blocked_requeued{0};
        std::atomic<uint64_t> ext_by_src[kNumSrc]{};
    };
    static std::unordered_map<uint32_t, Counts> counts;
    static std::mutex counts_mu;

    static Counts &get(uint32_t mq) {
        std::lock_guard<std::mutex> g(counts_mu);
        return counts[mq];
    }

    static void bump_send(uint32_t mq)       { get(mq).sends.fetch_add(1, std::memory_order_relaxed); }
    static void bump_recv(uint32_t mq)       { get(mq).recvs.fetch_add(1, std::memory_order_relaxed); }
    static void bump_ext(uint32_t mq, int src_idx) {
        auto &c = get(mq);
        c.external.fetch_add(1, std::memory_order_relaxed);
        if (src_idx >= 0 && src_idx < kNumSrc) {
            c.ext_by_src[src_idx].fetch_add(1, std::memory_order_relaxed);
        }
    }
    static void bump_delivered(uint32_t mq)  { get(mq).delivered.fetch_add(1, std::memory_order_relaxed); }
    static void bump_blocked(uint32_t mq)    { get(mq).drop_blocked.fetch_add(1, std::memory_order_relaxed); }
    static void bump_blocked_lost(uint32_t mq)     { get(mq).blocked_lost.fetch_add(1, std::memory_order_relaxed); }
    static void bump_blocked_requeued(uint32_t mq) { get(mq).blocked_requeued.fetch_add(1, std::memory_order_relaxed); }
}

extern "C" void mqdiag_dump(const char *path) {
    FILE *fp = fopen(path, "w");
    if (!fp) return;
    std::lock_guard<std::mutex> g(mqdiag::counts_mu);
    fprintf(fp, "# mq_addr     sends  recvs  ext  deliv  blk  lost  rq   Tmr  Sp  Si  Ai  Vi  Pi  Dp  Unt\n");
    for (auto &kv : mqdiag::counts) {
        fprintf(fp, "0x%08X  %5llu  %5llu  %3llu  %5llu  %3llu  %4llu  %4llu",
                kv.first,
                (unsigned long long)kv.second.sends.load(),
                (unsigned long long)kv.second.recvs.load(),
                (unsigned long long)kv.second.external.load(),
                (unsigned long long)kv.second.delivered.load(),
                (unsigned long long)kv.second.drop_blocked.load(),
                (unsigned long long)kv.second.blocked_lost.load(),
                (unsigned long long)kv.second.blocked_requeued.load());
        for (int i = 0; i < mqdiag::kNumSrc; ++i) {
            fprintf(fp, "  %3llu", (unsigned long long)kv.second.ext_by_src[i].load());
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

struct QueuedMessage {
    PTR(OSMesgQueue) mq;
    OSMesg mesg;
    bool jam;
    bool requeue_if_blocked;
};

static moodycamel::BlockingConcurrentQueue<QueuedMessage> external_messages {};
std::bitset<32> requeue_enabled;

void ultramodern::set_message_queue_control(const ultramodern::MessageQueueControl& mqc) {
    requeue_enabled.reset();
    requeue_enabled.set(static_cast<int>(EventMessageSource::Timer), mqc.requeue_timer);
    requeue_enabled.set(static_cast<int>(EventMessageSource::Sp), mqc.requeue_sp);
    requeue_enabled.set(static_cast<int>(EventMessageSource::Si), mqc.requeue_si);
    requeue_enabled.set(static_cast<int>(EventMessageSource::Ai), mqc.requeue_ai);
    requeue_enabled.set(static_cast<int>(EventMessageSource::Vi), mqc.requeue_vi);
    requeue_enabled.set(static_cast<int>(EventMessageSource::Pi), mqc.requeue_pi);
    requeue_enabled.set(static_cast<int>(EventMessageSource::Dp), mqc.requeue_dp);
}

void ultramodern::enqueue_external_message_src(PTR(OSMesgQueue) mq, OSMesg msg, bool jam, EventMessageSource src) {
    mqdiag::bump_ext(static_cast<uint32_t>(mq), static_cast<int>(src));
    external_messages.enqueue({mq, msg, jam, requeue_enabled[static_cast<int>(src)]});
}

void ultramodern::enqueue_external_message(PTR(OSMesgQueue) mq, OSMesg msg, bool jam, bool requeue_if_blocked) {
    mqdiag::bump_ext(static_cast<uint32_t>(mq), 7 /* Untagged */);
    external_messages.enqueue({mq, msg, jam, requeue_if_blocked});
}

bool do_send(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, bool jam, bool block);

void dequeue_external_messages(RDRAM_ARG1) {
    QueuedMessage to_send;
    std::vector<QueuedMessage> requeued_messages{};
    uint32_t pass_drained = 0, pass_delivered = 0, pass_lost = 0, pass_requeued = 0;
    while (external_messages.try_dequeue(to_send)) {
        ++pass_drained;
        bool ok = do_send(PASS_RDRAM to_send.mq, to_send.mesg, to_send.jam, false);
        if (ok) {
            mqdiag::bump_delivered(static_cast<uint32_t>(to_send.mq));
            ++pass_delivered;
        } else {
            mqdiag::bump_blocked(static_cast<uint32_t>(to_send.mq));
            if (to_send.requeue_if_blocked) {
                requeued_messages.push_back(to_send);
                mqdiag::bump_blocked_requeued(static_cast<uint32_t>(to_send.mq));
                ++pass_requeued;
            } else {
                mqdiag::bump_blocked_lost(static_cast<uint32_t>(to_send.mq));
                ++pass_lost;
            }
        }
    }
    for (QueuedMessage& cur_mesg : requeued_messages) {
        external_messages.enqueue(cur_mesg);
    }
    if (pass_drained > 1 || pass_lost > 0) {
        static int n = 0;
        if (++n <= 200) {
            if(0) fprintf(stderr, "[mqdrain #%d] drained=%u deliv=%u lost=%u rq=%u\n",
                    n, pass_drained, pass_delivered, pass_lost, pass_requeued);
            fflush(stderr);
        }
    }
}

void ultramodern::wait_for_external_message(RDRAM_ARG1) {
    QueuedMessage to_send;
    external_messages.wait_dequeue(to_send);
    if (!do_send(PASS_RDRAM to_send.mq, to_send.mesg, to_send.jam, false) && to_send.requeue_if_blocked) {
        external_messages.enqueue(to_send);
    }
}

void ultramodern::wait_for_external_message_timed(RDRAM_ARG u32 millis) {
    QueuedMessage to_send;
    if (external_messages.wait_dequeue_timed(to_send, std::chrono::milliseconds{millis})) {
        if (!do_send(PASS_RDRAM to_send.mq, to_send.mesg, to_send.jam, false) && to_send.requeue_if_blocked) {
            external_messages.enqueue(to_send);
        }
    }
}

extern "C" void osCreateMesgQueue(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg, s32 count) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    mq->blocked_on_recv = NULLPTR;
    mq->blocked_on_send = NULLPTR;
    mq->msgCount = count;
    mq->msg = msg;
    mq->validCount = 0;
    mq->first = 0;
}

s32 MQ_GET_COUNT(OSMesgQueue *mq) {
    return mq->validCount;
}

s32 MQ_IS_EMPTY(OSMesgQueue *mq) {
    return mq->validCount == 0;
}

s32 MQ_IS_FULL(OSMesgQueue* mq) {
    return MQ_GET_COUNT(mq) >= mq->msgCount;
}

static bool is_focus_mq(uint32_t mq) { return mq == 0x80114388 || mq == 0x8011A408 || mq == 0x8011A7E8; }
static FILE *mqfocus_fp = nullptr;
static std::mutex mqfocus_mu;
static void mqfocus_log(const char *tag, uint32_t mq, OSMesgQueue *q, bool block, int outcome) {
    std::lock_guard<std::mutex> g(mqfocus_mu);
    if (!mqfocus_fp) mqfocus_fp = fopen("mqfocus.txt", "w");
    if (!mqfocus_fp) return;
    fprintf(mqfocus_fp, "%-8s mq=0x%08X valid=%d/%d block=%d outcome=%d\n",
        tag, mq, q->validCount, q->msgCount, block ? 1 : 0, outcome);
    fflush(mqfocus_fp);
}

bool do_send(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, bool jam, bool block) {
    OSMesgQueue* mq = TO_PTR(OSMesgQueue, mq_);
    if (is_focus_mq((uint32_t)mq_)) mqfocus_log("send-in", (uint32_t)mq_, mq, block, 0);
    if (!block) {
        // If non-blocking, fail if the queue is full.
        if (MQ_IS_FULL(mq)) {
            if (is_focus_mq((uint32_t)mq_)) mqfocus_log("send-full", (uint32_t)mq_, mq, block, 0);
            return false;
        }
    }
    else {
        // Otherwise, yield this thread until the queue has room.
        while (MQ_IS_FULL(mq)) {
            debug_printf("[Message Queue] Thread %d is blocked on send\n", TO_PTR(OSThread, ultramodern::this_thread())->id);
            ultramodern::thread_queue_insert(PASS_RDRAM GET_MEMBER(OSMesgQueue, mq_, blocked_on_send), ultramodern::this_thread());
            ultramodern::run_next_thread_and_wait(PASS_RDRAM1);
        }
    }
    
    if (jam) {
        // Jams insert at the head of the message queue's buffer.
        mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
        TO_PTR(OSMesg, mq->msg)[mq->first] = msg;
        mq->validCount++;
    }
    else {
        // Sends insert at the tail of the message queue's buffer.
        s32 last = (mq->first + mq->validCount) % mq->msgCount;
        TO_PTR(OSMesg, mq->msg)[last] = msg;
        mq->validCount++;
    }

    // If any threads were blocked on receiving from this message queue, pop the first one and schedule it.
    PTR(PTR(OSThread)) blocked_queue = GET_MEMBER(OSMesgQueue, mq_, blocked_on_recv);
    if (!ultramodern::thread_queue_empty(PASS_RDRAM blocked_queue)) {
        ultramodern::schedule_running_thread(PASS_RDRAM ultramodern::thread_queue_pop(PASS_RDRAM blocked_queue));
    }
    
    return true;
}

bool do_recv(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg_, bool block) {
    OSMesgQueue* mq = TO_PTR(OSMesgQueue, mq_);
    if (is_focus_mq((uint32_t)mq_)) mqfocus_log("recv-in", (uint32_t)mq_, mq, block, 0);
    if (!block) {
        // If non-blocking, fail if the queue is empty
        if (MQ_IS_EMPTY(mq)) {
            if (is_focus_mq((uint32_t)mq_)) mqfocus_log("recv-empty", (uint32_t)mq_, mq, block, 0);
            return false;
        }
    } else {
        // Otherwise, yield this thread in a loop until the queue is no longer full
        while (MQ_IS_EMPTY(mq)) {
            debug_printf("[Message Queue] Thread %d is blocked on receive\n", TO_PTR(OSThread, ultramodern::this_thread())->id);
            ultramodern::thread_queue_insert(PASS_RDRAM GET_MEMBER(OSMesgQueue, mq_, blocked_on_recv), ultramodern::this_thread());
            ultramodern::run_next_thread_and_wait(PASS_RDRAM1);
        }
    }

    if (msg_ != NULLPTR) {
        *TO_PTR(OSMesg, msg_) = TO_PTR(OSMesg, mq->msg)[mq->first];
    }
    
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;

    // If any threads were blocked on sending to this message queue, pop the first one and schedule it.
    PTR(PTR(OSThread)) blocked_queue = GET_MEMBER(OSMesgQueue, mq_, blocked_on_send);
    if (!ultramodern::thread_queue_empty(PASS_RDRAM blocked_queue)) {
        ultramodern::schedule_running_thread(PASS_RDRAM ultramodern::thread_queue_pop(PASS_RDRAM blocked_queue));
    }

    return true;
}

extern "C" s32 osSendMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, s32 flags) {
    mqdiag::bump_send(static_cast<uint32_t>(mq_));
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    bool jam = false;
    
    // Don't directly send to the message queue if this isn't a game thread to avoid contention.
    if (!ultramodern::is_game_thread()) {
        ultramodern::enqueue_external_message(mq_, msg, jam, false);
        return 0;
    }
    
    // Handle any messages that have been received from an external thread.
    dequeue_external_messages(PASS_RDRAM1);

    // Try to send the message.
    bool sent = do_send(PASS_RDRAM mq_, msg, jam, flags == OS_MESG_BLOCK);
    
    // Check the queue to see if this thread should swap execution to another.
    ultramodern::check_running_queue(PASS_RDRAM1);

    return sent ? 0 : -1;
}

extern "C" s32 osJamMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, OSMesg msg, s32 flags) {
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);
    bool jam = true;
    
    // Don't directly send to the message queue if this isn't a game thread to avoid contention.
    if (!ultramodern::is_game_thread()) {
        ultramodern::enqueue_external_message(mq_, msg, jam, false);
        return 0;
    }
    
    // Handle any messages that have been received from an external thread.
    dequeue_external_messages(PASS_RDRAM1);

    // Try to send the message.
    bool sent = do_send(PASS_RDRAM mq_, msg, jam, flags == OS_MESG_BLOCK);
    
    // Check the queue to see if this thread should swap execution to another.
    ultramodern::check_running_queue(PASS_RDRAM1);

    return sent ? 0 : -1;
}

extern "C" s32 osRecvMesg(RDRAM_ARG PTR(OSMesgQueue) mq_, PTR(OSMesg) msg_, s32 flags) {
    mqdiag::bump_recv(static_cast<uint32_t>(mq_));
    OSMesgQueue *mq = TO_PTR(OSMesgQueue, mq_);

    assert(ultramodern::is_game_thread() && "RecvMesg not allowed outside of game threads.");
    
    // Handle any messages that have been received from an external thread.
    dequeue_external_messages(PASS_RDRAM1);

    // Try to receive a message.
    bool received = do_recv(PASS_RDRAM mq_, msg_, flags == OS_MESG_BLOCK);
    
    // Check the queue to see if this thread should swap execution to another.
    ultramodern::check_running_queue(PASS_RDRAM1);

    return received ? 0 : -1;
}
