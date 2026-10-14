#include <cstdio>
#include <ctime>
int main(){timespec t{};if(clock_gettime(CLOCK_MONOTONIC,&t))return 1;std::printf("%lld\n",static_cast<long long>(t.tv_sec)*1000000000LL+t.tv_nsec);}
