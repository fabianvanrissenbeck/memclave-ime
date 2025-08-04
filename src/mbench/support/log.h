// src/sk_log.h
#ifndef SK_LOG_H
#define SK_LOG_H

#include <defs.h>
#include <mram.h>
#include <stdint.h>

// To disable all logging (zero overhead), simply omit -DSK_LOG_ENABLED
#ifdef SK_LOG_ENABLED

/// total MRAM per DPU
#define MRAM_SIZE_BYTES     (64u << 20)
/// we reserve 64 B at the very top
#define SK_LOG_SIZE_BYTES   64
#define SK_LOG_OFFSET       (MRAM_SIZE_BYTES - SK_LOG_SIZE_BYTES)
/// number of 8‑byte slots
#define SK_LOG_MAX_ENTRIES  (SK_LOG_SIZE_BYTES / sizeof(uint64_t))

/// Zero out the region (caller should do this once per DPU)
static inline void sk_log_init(void) {
    if (me() == 0) {
        volatile uint64_t __mram_ptr *ptr =
            (volatile uint64_t __mram_ptr *)SK_LOG_OFFSET;
        //volatile __mram uint64_t *ptr =
        //    (volatile __mram uint64_t *)(SK_LOG_OFFSET);
        for (uint32_t i = 0; i < SK_LOG_MAX_ENTRIES; i++) {
            ptr[i] = 0;
        }
    }
}

/// Write one 64‑bit value into slot [0..SK_LOG_MAX_ENTRIES)
static inline void sk_log_write_idx(uint32_t idx, uint64_t val) {
    if (idx < SK_LOG_MAX_ENTRIES) {
        volatile uint64_t __mram_ptr *ptr =
            (volatile uint64_t __mram_ptr *)SK_LOG_OFFSET;
        ptr[idx] = val;
    }
}

#else

// no-ops when logging disabled
static inline void sk_log_init(void) {}
static inline void sk_log_write_idx(uint32_t idx, uint64_t val) {
    (void)idx; (void)val;
}

#endif // SK_LOG_ENABLED

#endif // SK_LOG_H
