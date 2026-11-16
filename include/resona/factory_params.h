/* GPL-3.0. Factory control metadata recorded through the public SDK API.
 * Metadata only: no factory DSP, models or private implementation. */
#ifndef RESONA_FACTORY_PARAMS_H
#define RESONA_FACTORY_PARAMS_H
#include <resona/platform.h>
static inline uint32_t resona_factory_param_count(uint32_t engine,uint32_t variant)
{
    const uint8_t counts[6]={2,2,6,7,4,3};
    if(engine<1u||engine>5u||variant>(engine==1u?1u:0u))return 0;
    return counts[engine==1u?variant:engine];
}
static inline int resona_factory_param_info(uint32_t engine,uint32_t variant,uint32_t index,resona_effect_param*out)
{
    const uint8_t offsets[6]={0,2,4,10,17,21};
    static const resona_effect_param data[24]={
        {"THRESHOLD","",0.0f,1.0f,0.200000003f,0.00999999978f,0u},
        {"RELEASE","",0.0f,1.0f,0.469999999f,0.00999999978f,0u},
        {"SENSITIVITY","",0.0f,1.0f,0.5f,0.00999999978f,0u},
        {"RELEASE","",0.0f,1.0f,0.319999993f,0.00999999978f,0u},
        {"THRESHOLD","",0.0f,1.0f,0.670000017f,0.00999999978f,0u},
        {"RATIO","",0.0f,1.0f,0.159999996f,0.00999999978f,0u},
        {"KNEE","",0.0f,1.0f,0.0f,0.00999999978f,0u},
        {"ATTACK","",0.0f,1.0f,0.0500000007f,0.00999999978f,0u},
        {"RELEASE","",0.0f,1.0f,0.100000001f,0.00999999978f,0u},
        {"MAKEUP","",0.0f,1.0f,0.5f,0.00999999978f,0u},
        {"GAIN","",0.0f,1.0f,0.5f,0.00999999978f,0u},
        {"BASS","dB",0.0f,1.0f,0.600000024f,0.00999999978f,0u},
        {"MID","dB",0.0f,1.0f,0.600000024f,0.00999999978f,0u},
        {"TREBLE","dB",0.0f,1.0f,0.600000024f,0.00999999978f,0u},
        {"LEVEL","",0.0f,1.0f,0.800000012f,0.00999999978f,0u},
        {"MASTER","",0.0f,1.0f,1.0f,0.00999999978f,0u},
        {"PRESENCE","",0.0f,1.0f,0.5f,0.00999999978f,0u},
        {"LEVEL","",0.0f,1.0f,0.5f,0.00999999978f,0u},
        {"BASS","",0.0f,1.0f,0.5f,0.00999999978f,0u},
        {"MID","",0.0f,1.0f,0.5f,0.00999999978f,0u},
        {"TREBLE","",0.0f,1.0f,0.5f,0.00999999978f,0u},
        {"LOW CUT","",0.0f,1.0f,0.0f,0.00999999978f,0u},
        {"HIGH CUT","",0.0f,1.0f,1.0f,0.00999999978f,0u},
        {"LEVEL","",0.0f,1.0f,0.5f,0.020833334f,0u},
    };
    uint32_t count=resona_factory_param_count(engine,variant);
    if(!out||index>=count)return RESONA_ERR_ARG;
    *out=data[offsets[engine==1u?variant:engine]+index];return RESONA_OK;
}
#endif
