// Порт hybrid.py: вставка пар Coolant+Inlet в direct-раскладки.
#include "rx.h"
using namespace std;static vector<int>A[304];
static bool calc(const string&g,int pr,array<double,7>&out){array<int,128>n{};for(char c:g)n[c]++;array<double,304>load{};double H=0,Pw=0;
 for(int i=0;i<304;i++){if(g[i]!='F')continue;int nf=0,nr=0;vector<int>cs;for(int j:A[i]){if(g[j]=='K'||g[j]=='V')return false;nf+=g[j]=='F';nr+=g[j]=='R';if(g[j]=='C')cs.push_back(j);}
  int p=12+4*nf;double h=p*p*EM.FH/4;H+=h;Pw+=p*EM.FP*(1+EM.refl*nr);for(int j:cs)load[j]+=h/cs.size();}
 int k=n['K'],v=n['V'],pc=n['P'];double rate=EM.rate*(1+EM.rateK*k),vent=EM.vent*(pr?8:7)*(1+(pr==2?.35:.3)*k),vcap=EM.vcap*(pr?8:7)*(1+(pr?.3:.25)*pc);
 if(v*vent<1.15*H||(v-4)*vent<H||H/(v-4)>.5*vcap||EM.hull*pc<4*H)return false;
 for(int i=0;i<304;i++){if(g[i]=='C'){int ins=0;for(int j:A[i])ins+=g[j]=='I';if(load[i]>.8*rate*ins)return false;}if(g[i]=='I')for(int j:A[i])if(g[j]=='V')return false;}
 double sum=0;for(double x:load)sum+=x;double held=max(0.,sum-EM.hull*pc*.0001),mult=held>1e-6?1+.05*log(held*1e9)/log(1000.):1;
 out={Pw*mult,H,vent*v,vent*(v-4),(double)pc,Pw,held};return true;}
int main(){for(int i=0;i<304;i++){if(i%19)A[i].push_back(i-1);if(i%19<18)A[i].push_back(i+1);if(i>=19)A[i].push_back(i-19);if(i+19<304)A[i].push_back(i+19);}
 mt19937 rng((unsigned)PI("hySeed"));auto rr=[&](int n){return (int)(rng()%n);};long long iters=PI("hyIter");
 for(string nr:{"R","noR"})for(int pr=0;pr<3;pr++){ifstream f("direct-"+nr+"-p"+to_string(pr)+".txt");string line,base;getline(f,line);while(getline(f,line))base+=line;if(base.size()!=304){cerr<<"нет direct-"<<nr<<"-p"<<pr<<".txt\n";continue;}
  double bscore=0;string best=base;
  for(long long z=0;z<iters;z++){string g=z%3?base:best;int changes=1+rr(3);
   for(int ch=0;ch<changes;ch++){vector<int>possible;for(int i=0;i<304;i++)if(strchr("FRK",g[i])){bool nf=false;for(int j:A[i])nf|=g[j]=='F';if(nf)possible.push_back(i);}
    if(possible.empty())break;int i=possible[rr((int)possible.size())];vector<int>ins;for(int j:A[i])if(strchr("FRK",g[j])){bool nv=false;for(int k:A[j])nv|=g[k]=='V';if(!nv)ins.push_back(j);}
    if(ins.empty())continue;int j=ins[rr((int)ins.size())];g[i]='C';g[j]='I';}
   array<double,7>v;if(calc(g,pr,v)&&v[0]>bscore){bscore=v[0];best=g;ofstream o("hybrid-"+nr+"-p"+to_string(pr)+".txt");o<<setprecision(12)<<v[0]<<' '<<v[1]<<' '<<v[2]<<' '<<v[3]<<' '<<(int)v[4]<<' '<<z<<' '<<v[5]<<' '<<v[6]<<'\n';for(int y=0;y<16;y++)o<<best.substr(y*19,19)<<'\n';}}
  map<char,int>cnt;for(char c:best)cnt[c]++;cout<<nr<<" p"<<pr<<" мощность "<<bscore<<" состав";for(auto[c,n]:cnt)cout<<' '<<c<<'='<<n;cout<<endl;}
 return 0;}
