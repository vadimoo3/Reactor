// Порт direct-reference.py: лучшая прямая схема без Exchanger (стержни с Reflector + кресты O/V).
#include "rx.h"
using namespace std;
int main(){const int N=304;vector<int>A[N];for(int i=0;i<N;i++){if(i%19)A[i].push_back(i-1);if(i%19<18)A[i].push_back(i+1);if(i>=19)A[i].push_back(i-19);if(i+19<N)A[i].push_back(i+19);}
 mt19937 rng((unsigned)PI("drSeed"));auto rr=[&](int n){return (int)(rng()%max(1,n));};uniform_real_distribution<double>U(0,1);
 long long trials=PI("drTrials");int maxF=(int)PI("drMaxF"),pMin=(int)PI("drPmin"),pRange=(int)PI("drPrange");double dbl=P("drDouble"),coolM=P("drCoolM");
 bool have=false;double bSold=0,bPow=0,bHeat=0,bCool=0,bCost=0;long long bSeed=0;string bGrid;
 for(long long it=0;it<trials;it++){if(it&&it%20000==0)cerr<<"попыток "<<it<<" лучший "<<bSold<<"\n";string g(N,'K');int n=1+rr(maxF);
  for(int q=0;q<n;q++)for(int t=0;t<100;t++){int i=rr(N);bool ok=g[i]=='K';for(int j:A[i])ok&=g[j]=='K'||g[j]=='R';if(!ok)continue;g[i]=U(rng)<1-dbl?'1':'2';for(int j:A[i])g[j]='R';break;}
  int pc=pMin+rr(pRange);for(int i=N-1;i>=0;i--){if(g[i]=='K'){g[i]='P';pc--;}if(!pc)break;}
  double energy=0,heat=0;for(int i=0;i<N;i++)if(g[i]=='1'||g[i]=='2'){int r=0;for(int j:A[i])r+=g[j]=='R';int pulse=g[i]=='1'?1:4;energy+=M.fuelPow*pulse*(1+M.refl*r);heat+=pulse*pulse*M.fuelHeat/(g[i]=='1'?1:2);}
  if(have&&energy<=bSold)continue;
  for(int phase=0;phase<5;phase++){vector<int>centers;for(int i=0;i<N;i++)if((i%19+2*(i/19))%5==phase&&A[i].size()==4){bool ok=g[i]=='K';for(int j:A[i])ok&=g[j]=='K';if(ok)centers.push_back(i);}
   for(int no=1;no<=(int)centers.size();no++){string gg=g;for(int z=0;z<no;z++){gg[centers[z]]='O';for(int j:A[centers[z]])gg[j]='V';}
    array<int,128>cnt{};for(char c:gg)cnt[c]++;int k=cnt['K'];double cool=cnt['V']*M.vr0*(1+M.ventBoost*(M.bAv+1)*k),cost=PR.fuel(cnt)+PR.upgrades(1,0,0,0,0)+PR.parts(cnt);
    if(cool<coolM*heat||cost>PR.budget)continue;double sold=min(energy,M.sales(k));if(have&&sold<=bSold)continue;
    have=true;bSold=sold;bPow=energy;bHeat=heat;bCool=cool;bCost=cost;bSeed=it;bGrid=gg;}}}
 if(!have){cerr<<"ничего не найдено\n";return 1;}
 map<char,int>cnt;for(char c:bGrid)cnt[c]++;ofstream js("direct-reference.json");js<<setprecision(15)<<"{\"attempts\":"<<trials<<",\"best\":{\"sold_B\":"<<bSold<<",\"power_B\":"<<bPow<<",\"heat_B\":"<<bHeat<<",\"cooling_B\":"<<bCool<<",\"cost_Qa\":"<<bCost<<",\"seed\":"<<bSeed<<",\"counts\":{";
 bool first=true;for(auto[c,n]:cnt){js<<(first?"":",")<<'"'<<c<<"\":"<<n;first=false;}js<<"},\"grid\":[";for(int y=0;y<16;y++)js<<(y?",":"")<<'"'<<bGrid.substr(y*19,19)<<'"';js<<"]}}\n";
 ofstream t("direct-reference.txt");t<<setprecision(12)<<bSold<<' '<<bHeat<<' '<<bCool<<' '<<bCost<<" 1 0 0 0 0 "<<bSeed<<'\n';for(int y=0;y<16;y++)t<<bGrid.substr(y*19,19)<<'\n';
 cout<<"продажа "<<bSold<<" мощность "<<bPow<<" тепло "<<bHeat<<" охлаждение "<<bCool<<" цена "<<bCost<<" Qa -> direct-reference.json/.txt\n";return 0;}
