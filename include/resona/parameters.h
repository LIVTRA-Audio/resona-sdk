/* GPL-3.0: bounded latest-value parameter inbox for UI/control -> audio handoff. */
#ifndef RESONA_PARAMETERS_H
#define RESONA_PARAMETERS_H
#include <stdint.h>
#define RESONA_PARAMETER_INBOX_CAPACITY 12u
/* Zero-initialize in init(). One consumer (audio); producers never touch DSP state.
 * Independent parameters coalesce to their latest value, not an ordered MIDI queue.
 * Only lock-free 32-bit atomics are used: no allocation, spin or audio-time lock. */
typedef struct resona_parameter_inbox {
    uint32_t bits[RESONA_PARAMETER_INBOX_CAPACITY];
    uint32_t pending;
} resona_parameter_inbox;
static inline void resona_parameter_publish(resona_parameter_inbox *q,uint32_t index,float value)
{
    if(index>=RESONA_PARAMETER_INBOX_CAPACITY)return;
    uint32_t bits;__builtin_memcpy(&bits,&value,sizeof bits);
    __atomic_store_n(&q->bits[index],bits,__ATOMIC_RELEASE);
    __atomic_fetch_or(&q->pending,1u<<index,__ATOMIC_RELEASE);
}
/* At the start of process(): for each returned bit, apply values[index] to DSP.
 * Updates arriving during consumption are retained for this or the next block. */
static inline uint32_t resona_parameter_consume(resona_parameter_inbox *q,float values[12])
{
    uint32_t mask=__atomic_exchange_n(&q->pending,0u,__ATOMIC_ACQUIRE);
    for(uint32_t i=0;i<RESONA_PARAMETER_INBOX_CAPACITY;i++)if(mask&(1u<<i)){
        uint32_t bits=__atomic_load_n(&q->bits[i],__ATOMIC_ACQUIRE);
        __builtin_memcpy(&values[i],&bits,sizeof bits);
    }
    return mask;
}
#endif
