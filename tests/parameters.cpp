// Host concurrency contract test. Compile outside this source tree.
#include <resona/parameters.h>
#include <atomic>
#include <cassert>
#include <thread>
int main()
{
    static_assert(__atomic_always_lock_free(4,0),"parameter handoff requires lock-free words");
    resona_parameter_inbox q={};std::atomic<bool> done{false};float latest[12]={};
    std::thread writer([&]{for(unsigned n=1;n<=100000;n++)resona_parameter_publish(&q,n%12,(float)n);done.store(true);});
    auto consume=[&]{float values[12];auto mask=resona_parameter_consume(&q,values);
        for(unsigned i=0;i<12;i++)if(mask&(1u<<i)){
            assert(values[i]>=latest[i]);assert((unsigned)values[i]%12==i);latest[i]=values[i];
        }};
    while(!done.load())consume();writer.join();consume();
    for(unsigned i=0;i<12;i++){unsigned expected=100000-100000%12+i;if(expected>100000)expected-=12;assert(latest[i]==expected);}
    resona_parameter_publish(&q,12,1);float values[12];assert(resona_parameter_consume(&q,values)==0);
}
