// GPL-3.0 licensed. Buffers are declared here and supplied before init().
#include <resona/module.h>
namespace {
constexpr uint32_t kDelaySamples = 65536; // 256 KiB in PSRAM
const resona_buffer_request requests[] = {
    {1, RESONA_MEMORY_FAST, RESONA_MAX_BLOCK_FRAMES * sizeof(float), 128},
    {2, RESONA_MEMORY_PSRAM, kDelaySamples * sizeof(float), 128},
};
const resona_memory_profile memory = {RESONA_MEMORY_VERSION, sizeof(resona_memory_profile), 2, requests};
const resona_param params[] = {{"TIME", 1, 1000, 250, "ms"}, {"MIX", 0, 1, 0.5f, ""}, {"FEEDBACK", 0, 0.9f, 0.25f, ""}};
struct State { float *work, *delay; uint32_t write, length; float values[3]; };
void init(void *p, const resona_host *h) {
    auto &s = *static_cast<State *>(p);
    // This descriptor requires memory-aware firmware. No heap calls in user DSP.
    const resona_buffer *w = resona_buffer_find(h->memory, 1);
    const resona_buffer *d = resona_buffer_find(h->memory, 2);
    s.work = w && w->bytes >= requests[0].bytes ? static_cast<float *>(w->data) : nullptr;
    s.delay = d && d->bytes >= requests[1].bytes ? static_cast<float *>(d->data) : nullptr;
    s.write = 0; s.length = 12000;
    for (uint32_t i=0; i<3; ++i) s.values[i]=params[i].def;
}
void reset(void *p) {
    auto &s = *static_cast<State *>(p); s.write=0;
    if(s.delay) for(uint32_t i=0; i<kDelaySamples; ++i) s.delay[i]=0;
}
void set(void *p, uint32_t n, float v) {
    auto &s = *static_cast<State *>(p);
    if(n<3) { s.values[n]=v; if(n==0) s.length=static_cast<uint32_t>(v*48); }
}
float get(const void *p,uint32_t n) { return n<3?static_cast<const State *>(p)->values[n]:0; }
void process(void *p,const float *const *in,float *const *out,uint32_t frames) {
    auto &s = *static_cast<State *>(p);
    for(uint32_t i=0;i<frames;++i) {
        float dry=in[0][i];
        if(!s.work || !s.delay) { out[0][i]=dry; continue; }
        float wet=s.delay[(s.write+kDelaySamples-s.length)%kDelaySamples];
        s.work[i]=dry+wet*s.values[2];
        s.delay[s.write]=s.work[i]; s.write=(s.write+1)%kDelaySamples;
        out[0][i]=dry*(1-s.values[1])+wet*s.values[1];
    }
}
const resona_module module={RESONA_MODULE_ABI,sizeof(resona_module),
    "org.resona.example.memory-delay","Memory Delay","0.1.0",
    RESONA_LAYOUT_MONO,RESONA_RATE_48000,sizeof(State),3,params,nullptr,
    init,reset,set,process,nullptr,nullptr,get,nullptr,&memory};
}
extern "C" RESONA_EXPORT const resona_module *resona_module_get() { return &module; }
