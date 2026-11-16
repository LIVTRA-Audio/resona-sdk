// GPL-3.0. Portable helper contract tests; compile/run outside the SDK source folder.
#include <resona/startup_helpers.h>
#include <cassert>
#include <cstring>
static resona_startup_info info;
static unsigned writes;
static int fail_write;
static int get(resona_startup_info*out){*out=info;return 0;}
static int set(const void*data,uint32_t n){writes++;if(fail_write)return RESONA_ERR_STORAGE;info.splash_bytes=n;info.splash_crc32=n?resona_splash_crc32(data,n):0;return 0;}
int main(){
    uint8_t data[1600];resona_splash_builder b;
    assert(resona_splash_begin(&b,data,sizeof data,4000)==0);
    assert(resona_splash_run(&b,55040,0)==RESONA_ERR_ARG);
    assert(resona_splash_frame(&b,200)==0);
    assert(resona_splash_run(&b,55039,0)==0);
    assert(resona_splash_frame(&b,200)==RESONA_ERR_STATE);
    assert(resona_splash_finish(&b)==RESONA_ERR_STATE);
    assert(resona_splash_run(&b,2,0)==RESONA_ERR_ARG);
    assert(resona_splash_run(&b,1,0)==0);
    int bytes=resona_splash_finish(&b);assert(bytes==48&&resona_splash_valid(data,bytes));
    resona_platform p={};p.version=RESONA_PLATFORM_VERSION;p.size=sizeof p;p.startup_info=get;p.startup_splash_set=set;
    assert(resona_startup_splash_apply(&p,data,bytes)==1&&writes==1);
    assert(resona_startup_splash_apply(&p,data,bytes)==0&&writes==1);
    data[40]=0;data[41]=0;resona_splash_put(data+8,resona_splash_crc32(data+32,bytes-32));
    assert(!resona_splash_valid(data,bytes));assert(resona_startup_splash_apply(&p,data,bytes)==RESONA_ERR_ARG&&writes==1);
    assert(resona_startup_splash_apply(&p,NULL,0)==1&&writes==2);
    assert(resona_startup_splash_apply(&p,NULL,0)==0&&writes==2);
    assert(resona_startup_splash_apply(&p,NULL,1)==RESONA_ERR_ARG);
    p.version=RESONA_PLATFORM_VERSION+1;assert(resona_startup_splash_apply(&p,NULL,0)==RESONA_ERR_UNSUPPORTED);p.version=RESONA_PLATFORM_VERSION;
    uint8_t guard[45];memset(guard,0xa5,sizeof guard);
    assert(resona_splash_begin(&b,guard,43,4000)==0&&resona_splash_frame(&b,16)==0);
    assert(resona_splash_run(&b,55040,0)==RESONA_ERR_NO_MEMORY&&guard[43]==0xa5&&guard[44]==0xa5);
    assert(resona_splash_begin(&b,data,sizeof data,0x80000000u)==RESONA_ERR_ARG);
    assert(resona_splash_begin(&b,data,sizeof data,0)==0);
    for(unsigned i=0;i<128;i++){assert(resona_splash_frame(&b,16)==0);assert(resona_splash_run(&b,55040,(uint16_t)i)==0);}
    assert(resona_splash_frame(&b,16)==RESONA_ERR_FULL);bytes=resona_splash_finish(&b);assert(resona_splash_valid(data,bytes));
    fail_write=1;assert(resona_startup_splash_apply(&p,data,bytes)==RESONA_ERR_STORAGE);
}
