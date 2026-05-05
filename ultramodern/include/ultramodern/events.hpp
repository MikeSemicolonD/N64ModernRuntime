#ifndef __EVENTS_HPP__
#define __EVENTS_HPP__

#include <cstdint>
#include <utility>
#include <vector>

namespace ultramodern {
    // Forward LLE RDP byte ranges into the renderer on the gfx thread.
    // Used by the Factor5 ucode DPC bridge so RDP submissions are processed
    // on the same thread as HLE, avoiding races on shared renderer state.
    void submit_rdp_range(uint32_t lo_phys, uint32_t hi_phys);

    // Bulk-enqueue many RDP byte ranges as one action. Lets the bridge avoid
    // flooding the action_queue with millions of tiny per-cmd enqueues.
    void submit_rdp_range_batch(std::vector<std::pair<uint32_t, uint32_t>>&& ranges);

    namespace events {
        struct callbacks_t {
            using vi_callback_t = void();
            using gfx_init_callback_t = void();

            /**
             * Called in each VI.
             */
            vi_callback_t* vi_callback;

            /**
             * Called before entering the gfx main loop.
             */
            gfx_init_callback_t* gfx_init_callback;
        };

        void set_callbacks(const callbacks_t& callbacks);
    }
}

#endif
