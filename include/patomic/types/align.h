#ifndef PATOMIC_ALIGN_H
#define PATOMIC_ALIGN_H

#include <stddef.h>
#include <patomic/patomic_export.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ALIGN
 *
 * - these alignments are required for atomic operations on
 *   the specified type to be valid
 *
 * - recommended: the alignment required by the C language
 *                (always valid)
 * - minimum: the alignment required by the architecture
 *            (conditionally valid)
 * - size_within: the size and alignment of a hypothetical buffer
 *                within which the object must reside if "minimum"
 *                is to be considered valid, unless it is 0 in which
 *                case "minimum" is always valid
 *
 * NOTE:
 * - "recommended" and "minimum" are always a positive power of 2
 * - "minimum" is always <= "recommended"
 * - "size_within" has no restrictions on its value
 * - the "minimum" alignment is only valid if the object resides
 *   entirely within the buffer specified with "size_within" (unless
 *   "size_within" is 0 in which case "minimum" is always valid)
 * - the purpose of this is to communicate on x86 that operations
 *   on a type are atomic if the object doesn't cross a cache line
 *
 * EXAMPLES:
 * - type: int, op: exchange
 *   - taken to have a size and alignment of 4
 *   - LOCK XCHG on x86, which has no alignment requirement
 *   - {.recommended=4, .minimum=1, .size_within=0}
 * - type: avx_128, op: load
 *   - taken to have a size and alignment of 16
 *   - atomic on x86 if it doesn't cross a cache-line boundary
 *   - cache-line size is taken to be 64 bytes
 *   - {.recommended=16, .minimum=1, .size_within=64}
 */

typedef struct {
    size_t recommended;
    size_t minimum;
    size_t size_within;
} patomic_align_t;


/* represents the maximum cache line size in all known hardware */
/* WARNING: changing this constitutes an ABI break */
#undef PATOMIC_MAX_CACHE_LINE_SIZE
#define PATOMIC_MAX_CACHE_LINE_SIZE 128

/* Note: this may return a value <= to the macro value (always a power of 2) */
/* the value will always be at least as large as a cache line */
PATOMIC_EXPORT size_t
patomic_cache_line_size(void);


/*
 * ALIGNMENT CHECKS
 *
 * - internal cast used MAY rely on implementation defined behaviour
 * - this is only likely to happen if no atomic implementation is supported
 * - realistically a uintptr type or macro will be available on any compiler
 *   which can provide some access to an atomic implementation
 *
 * - if you need to be certain, implement these functions yourself, and don't
 *   use them if no atomic implementation is available
 */
PATOMIC_EXPORT int
patomic_align_meets_recommended(
    const volatile void *ptr,
    patomic_align_t align
);

PATOMIC_EXPORT int
patomic_align_meets_minimum(
    const volatile void *ptr,
    patomic_align_t align,
    size_t width
);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* !PATOMIC_ALIGN_H */
