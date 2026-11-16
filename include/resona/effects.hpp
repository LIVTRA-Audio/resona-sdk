/* Small control-thread builder for app-owned serial/parallel effects. GPL-3.0. */
#ifndef RESONA_EFFECTS_HPP
#define RESONA_EFFECTS_HPP
#include <resona/platform.h>

namespace resona {
class Effects {
    const resona_platform *p_=nullptr;
    resona_effect_route *routes_=nullptr;
    uint32_t count_=0,capacity_=0;
    int error_=0;
    bool available() const {
        return p_ && p_->version==RESONA_PLATFORM_VERSION && p_->size==sizeof(resona_platform) &&
            p_->effects_begin && p_->effect_create && p_->effect_destroy &&
            p_->effect_param_set && p_->effect_enable && p_->effects_graph_set &&
            p_->effect_info && p_->effect_param_info && p_->effects_status;
    }
    bool room() {
        if(count_<capacity_)return true;
        if(capacity_>=RESONA_EFFECT_MAX_ROUTES){error_=RESONA_ERR_ARG;return false;}
        uint32_t n=capacity_?capacity_*2u:8u;
        if(n>RESONA_EFFECT_MAX_ROUTES)n=RESONA_EFFECT_MAX_ROUTES;
        auto next=static_cast<resona_effect_route*>(p_->alloc(n*sizeof(*routes_),16));
        if(!next){error_=RESONA_ERR_NO_MEMORY;return false;}
        for(uint32_t i=0;i<count_;i++)next[i]=routes_[i];
        if(routes_)p_->free(routes_);
        routes_=next;capacity_=n;return true;
    }
    static bool copy(char *out,const char *in) {
        uint32_t n=0;
        if(in)while(n<60u && in[n]){out[n]=in[n];n++;}
        out[n]=0;
        return !in || !in[n];
    }
public:
    Effects()=default;
    Effects(const Effects&)=delete;
    Effects& operator=(const Effects&)=delete;
    static constexpr uint32_t Guitar=RESONA_EFFECT_GUITAR;
    static constexpr uint32_t Usb=RESONA_EFFECT_USB;
    static constexpr uint32_t Bluetooth=RESONA_EFFECT_BLUETOOTH;
    static constexpr uint32_t Main=RESONA_EFFECT_MAIN;
    static constexpr uint32_t Output=RESONA_EFFECT_OUTPUT;
    static constexpr uint32_t Capture=RESONA_EFFECT_CAPTURE;
    int error() const {return error_;}
    int begin(const resona_platform *p) {
        p_=p;
        if(!available())return error_=RESONA_ERR_UNSUPPORTED;
        int rc=p_->effects_begin();
        if(rc)return error_=rc;
        routes_=nullptr;capacity_=count_=0;return error_=0;
    }
    resona_effect_handle factory(uint32_t engine,uint32_t model=RESONA_EFFECT_AUTO_MODEL,uint32_t variant=0) {
        if(error_)return 0;
        if(!available()){error_=RESONA_ERR_UNSUPPORTED;return 0;}
        resona_effect_source s;resona_effect_source_init(&s,RESONA_EFFECT_FACTORY);
        s.engine=engine;s.model=model;s.variant=variant;
        uint32_t h=0;error_=p_->effect_create(&s,&h);return error_?0:h;
    }
    resona_effect_handle pack(const char *id,const char *version="",uint32_t crc32=0) {
        if(error_)return 0;
        if(!available()){error_=RESONA_ERR_UNSUPPORTED;return 0;}
        resona_effect_source s;resona_effect_source_init(&s,RESONA_EFFECT_PACK);s.crc32=crc32;
        if(!id || !*id || !copy(s.id,id) || !copy(s.package_version,version)){error_=RESONA_ERR_ARG;return 0;}
        uint32_t h=0;error_=p_->effect_create(&s,&h);return error_?0:h;
    }
    int connect(uint32_t source,uint32_t destination,float gain=1.0f,uint8_t source_channel=0,uint8_t destination_channel=0) {
        if(error_)return error_;
        if(!available())return error_=RESONA_ERR_UNSUPPORTED;
        if(!room())return error_;
        routes_[count_++]={source,destination,gain,source_channel,destination_channel,0};
        return error_=0;
    }
    int output(uint32_t source,float gain=1.0f) {
        int rc=connect(source,Output,gain,0,0);return rc?rc:connect(source,Output,gain,0,1);
    }
    int capture(uint32_t source,float gain=1.0f) {
        int rc=connect(source,Capture,gain,0,0);return rc?rc:connect(source,Capture,gain,0,1);
    }
    void clear_routes(){count_=0;error_=0;}
    void clear_error(){error_=0;}
    int commit() {
        if(error_)return error_;
        if(!available())return error_=RESONA_ERR_UNSUPPORTED;
        resona_effect_graph g={RESONA_EFFECTS_VERSION,sizeof(g),count_,routes_};
        return error_=p_->effects_graph_set(&g);
    }
    int param(uint32_t h,uint32_t index,float value){return error_=available()?p_->effect_param_set(h,index,value):RESONA_ERR_UNSUPPORTED;}
    int enable(uint32_t h,bool on){return error_=available()?p_->effect_enable(h,on?1u:0u):RESONA_ERR_UNSUPPORTED;}
    int destroy(uint32_t h){return error_=available()?p_->effect_destroy(h):RESONA_ERR_UNSUPPORTED;}
    int info(uint32_t h,resona_effect_info *out)const{return available()?p_->effect_info(h,out):RESONA_ERR_UNSUPPORTED;}
    int param_info(uint32_t h,uint32_t i,resona_effect_param *out)const{return available()?p_->effect_param_info(h,i,out):RESONA_ERR_UNSUPPORTED;}
    int status(resona_effects_status *out)const{return available()?p_->effects_status(out):RESONA_ERR_UNSUPPORTED;}
};
}
#endif
