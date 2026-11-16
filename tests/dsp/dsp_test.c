/* GPL-3.0. Portable contract tests; native equivalence is a separate device check. */
#include <resona/dsp.h>
#include <assert.h>
#include <math.h>
int main(void) {
    int16_t a[2]={-32768,-32768};int64_t sum;
    assert(resona_dsp_dot_q15(0,a,a,2,&sum)==0 && sum==2147483648LL);
    assert(resona_dsp_dot_q15(0,a,a,4097,&sum)==-1);
    assert(resona_dsp_floor16(-1)==-1 && resona_dsp_floor16(-65537)==-2);
    float x[8]={-INFINITY,-1,-0.5f,0,0.5f,1,INFINITY,NAN};
    int16_t q[8];float y[8];
    assert(!resona_dsp_f32_to_q15(0,x,q,8));
    const int16_t expected[8]={-32768,-32768,-16384,0,16384,32767,32767,0};
    for(unsigned i=0;i<8;i++)assert(q[i]==expected[i]);
    assert(!resona_dsp_q15_to_f32(0,q,y,8));
    for(unsigned i=0;i<8;i++)assert(y[i]==expected[i]/32768.0f);
    int16_t ring[1]={-32768},out[3];resona_dsp_ring r={ring,1,0,32767};
    assert(!resona_dsp_ring_q15(0,&r,1,out,3));assert(r.index==0 && out[0]==-32767);
    r.index=1;assert(resona_dsp_ring_q15(0,&r,1,out,3)==-1);
    return 0;
}
