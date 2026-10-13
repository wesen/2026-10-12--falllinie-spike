// Native regression for the exact copied-buffer helper used by every embedding.
#include <limits>
#include "probe-common.hpp"
int main(){try{
 CopyState state;
 const double valid[]={1,2,3};
 require(state.copy(valid,sizeof(valid)),"valid copy rejected");
 const CopyState prior=state;
 const double overflow[]={std::numeric_limits<double>::max(),std::numeric_limits<double>::max()};
 require(!state.copy(overflow,sizeof(overflow)),"finite input sum overflow accepted");
 require(state.values==prior.values&&state.count==prior.count&&state.sum==prior.sum,"overflow mutated accepted state");
 const double invalid[]={1,std::numeric_limits<double>::quiet_NaN()};
 require(!state.copy(invalid,sizeof(invalid)),"nonfinite input accepted");
 require(state.values==prior.values&&state.count==prior.count&&state.sum==prior.sum,"nonfinite input mutated accepted state");
 const double nearLimit[]={std::numeric_limits<double>::max(),0};
 require(state.copy(nearLimit,sizeof(nearLimit)),"finite maximum sum rejected");
 require(state.count==2&&state.sum==nearLimit[0],"finite maximum sum changed");
 std::cout<<"COPY STATE PASS overflow rejected; all values/count/sum preserved; finite limit accepted\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
