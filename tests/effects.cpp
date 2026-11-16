/* GPL-3.0. Graph validation contracts, including >5 effects and invalid mutations. */
#include <resona/effects.hpp>
#include <cassert>
#include <limits>
int main() {
    // A table of another platform version, a truncated table and missing calls are refused.
    resona::Effects helper;resona_platform old{};old.version=RESONA_PLATFORM_VERSION+1;old.size=sizeof(old);
    assert(helper.begin(&old)==RESONA_ERR_UNSUPPORTED);
    old.version=RESONA_PLATFORM_VERSION;old.size=offsetof(resona_platform,effects_begin);assert(helper.begin(&old)==RESONA_ERR_UNSUPPORTED);
    old.size=sizeof(old);assert(helper.begin(&old)==RESONA_ERR_UNSUPPORTED);

    uint32_t h[10],order[10],incoming[10];for(uint32_t i=0;i<10;i++)h[i]=i+1;
    resona_effect_route r[16]{};r[0]={RESONA_EFFECT_USB,1,1,0,0,0};
    for(uint32_t i=1;i<10;i++)r[i]={i,i+1,1,0,0,0};
    r[10]={10,RESONA_EFFECT_OUTPUT,1,0,0,0};
    resona_effect_graph g={1,sizeof(g),11,r};
    assert(!resona_effect_graph_order(&g,h,10,order,incoming));
    for(uint32_t i=0;i<10;i++)assert(order[i]==i);
    r[11]={5,2,1,0,0,0};g.route_count=12;assert(resona_effect_graph_order(&g,h,10,order,incoming)); // cycle
    r[11]=r[10];assert(resona_effect_graph_order(&g,h,10,order,incoming)); // duplicate
    r[11]={RESONA_EFFECT_USB,6,0.5f,1,0,0};assert(!resona_effect_graph_order(&g,h,10,order,incoming)); // fan-in
    r[11].source_channel=2;assert(resona_effect_graph_order(&g,h,10,order,incoming));
    r[11]={RESONA_EFFECT_MAIN,RESONA_EFFECT_CAPTURE,1,1,1,0};assert(!resona_effect_graph_order(&g,h,10,order,incoming));
    r[11].destination=RESONA_EFFECT_OUTPUT;assert(resona_effect_graph_order(&g,h,10,order,incoming));
    r[11]={1,RESONA_EFFECT_CAPTURE,std::numeric_limits<float>::quiet_NaN(),0,0,0};assert(resona_effect_graph_order(&g,h,10,order,incoming));
    r[11].gain=std::numeric_limits<float>::infinity();assert(resona_effect_graph_order(&g,h,10,order,incoming));
    r[11].gain=1;r[11].source=99;assert(resona_effect_graph_order(&g,h,10,order,incoming));
    g.route_count=RESONA_EFFECT_MAX_ROUTES+1;assert(resona_effect_graph_order(&g,h,10,order,incoming));
    g={1,sizeof(g),0,nullptr};assert(!resona_effect_graph_order(&g,nullptr,0,nullptr,nullptr));
    resona_effect_source s;resona_effect_source_init(&s,RESONA_EFFECT_PACK);assert(s.size==sizeof(s)&&s.model==RESONA_EFFECT_AUTO_MODEL&&s.id[0]==0);
}
