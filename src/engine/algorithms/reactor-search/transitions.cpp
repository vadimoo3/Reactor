#include "rx.h"
using namespace std;const int N=304,W=19;vector<int>a[N];mt19937 rng((unsigned)PI("trSeed"));string dir="";double budgets[]={P("trB1"),P("trB2"),P("trB3"),P("trB4")};double best[4][3]={};long long checks=0,valid=0;
int cores(char c){return c=='F'?4:c=='1'?1:c=='2'?2:c=='4'?4:0;}
void eval(string g,int profile,string origin,long long seed){
for(int i=0;i<N;i++)if(g[i]=='R'||g[i]=='C'){bool fuel=0;for(int j:a[i])fuel|=cores(g[j])>0;if(!fuel)g[i]='K';}
for(int i=0;i<N;i++)if(g[i]=='I'){bool coolant=0;for(int j:a[i])coolant|=g[j]=='C';if(!coolant)g[i]='K';}
array<int,128>cnt{};for(char c:g)cnt[c]++;int ns=cnt['1']+cnt['2']+cnt['4'];double dh=0,np=0,dp=0;array<double,N>load{};
for(int i=0;i<N;i++)if(int c=cores(g[i])){int p=c==1?1:c==2?4:12,r=0,nc=0;for(int j:a[i]){p+=cores(g[j]);r+=g[j]=='R';nc+=g[j]=='C';if(g[j]=='K'||g[j]=='V')return;}double h=p*p*(g[i]=='F'?EM.FH:EM.NH)/c;dh+=h;if(g[i]=='F')dp+=p*EM.FP*(1+EM.refl*r);else np+=p*EM.NH*(1+EM.refl*r);if(nc)for(int j:a[i])if(g[j]=='C')load[j]+=h/nc;}
int k=cnt['K'],v=cnt['V'],p=cnt['P'];double vent=EM.vent*(profile?8:7)*(1+(profile==2?.35:.3)*k),vcap=EM.vcap*(profile?8:7)*(1+(profile?.3:.25)*p),rate=EM.rate*(1+EM.rateK*k);checks++;
if(vent*v<1.15*dh||vent*(v-4)<dh||dh/(v-4)>.5*vcap||EM.hull*p<4*dh)return;
for(int i=0;i<N;i++)if(g[i]=='C'){int ins=0;for(int j:a[i])ins+=g[j]=='I';if(load[i]>.8*rate*ins)return;}valid++;
double held=max(0.,accumulate(load.begin(),load.end(),0.)-EM.hull*p*.0001);
double cost=(cnt['F']*18.982+cnt['C']*.8+cnt['I']*.256+cnt['O']*.256+cnt['V']*.195312+cnt['K']*.65536+cnt['P']*.65536+cnt['R']*.000005)/1000+cnt['1']*3.586+cnt['2']*7.889+cnt['4']*17.356+(profile==0?0:profile==1?.26:1.26);
for(int dl=0;dl<4;dl++)for(int nl=0;nl<2;nl++)for(int ff=0;ff<3;ff++){if(ff&&held<=1e-6)continue;double cost2=cost+(dl==0?0:dl==1?.2:dl==2?2.2:22.2)+(nl?17.5:0)+(ff==0?0:ff==1?.1:10.1);double basepower=dp*(1+.5*dl)+np*(1+nl);double power=basepower*(held>1e-6?1+.01*(5+ff)*log(held*1e9)/log(1000.):1);for(int b=0;b<4;b++)if(cost2<=budgets[b])for(int force=0;force<3;force++){if(force==1&&!ns)continue;if(force==2&&ns)continue;if(power<=best[b][force]+1e-9)continue;best[b][force]=power;string name="transition-"+to_string((int)budgets[b])+(force==0?"-any":force==1?"-N":"-D");ofstream f(dir+name+".txt");f<<setprecision(12)<<power<<" "<<dh<<" "<<vent*v<<" "<<vent*(v-4)<<" "<<cost2<<" "<<profile<<" "<<dl<<" "<<nl<<" "<<seed<<" "<<origin<<" "<<basepower<<" "<<held<<" "<<ff<<"\n";for(int y=0;y<16;y++)f<<g.substr(y*W,W)<<"\n";}}
}
int main(){for(int i=0;i<N;i++){if(i%W)a[i].push_back(i-1);if(i%W<18)a[i].push_back(i+1);if(i>=W)a[i].push_back(i-W);if(i+W<N)a[i].push_back(i+W);}for(string mode:{"direct","buffer","hybrid"})for(string r:{"-R","-noR"})for(int pr=0;pr<3;pr++){string name=mode+r+"-p"+to_string(pr),line,g;ifstream f(dir+name+".txt");getline(f,line);while(getline(f,line))g+=line;if(g.size()!=N)continue;vector<int>slots;for(int i=0;i<N;i++)if(g[i]=='F')slots.push_back(i);eval(g,pr,name,0);
long long att=PI("trAtt");cerr<<"переходы из "<<name<<"\n";for(long long z=1;z<=att;z++){string b=g;int number=1+rng()%26;for(int n=0;n<number;n++){int i=slots[rng()%slots.size()];if(b[i]!='F'&&b[i]!='R')continue;bool near=0;for(int j:a[i])near|=b[j]=='1'||b[j]=='2'||b[j]=='4';if(near)continue;int v=rng()%100;b[i]=v<80?'1':v<97?'2':'4';for(int j:a[i])if(b[j]=='F')b[j]='R';}int rem=(z%3==0)?rng()%60:0;for(char &c:b)if(c=='F'&&rng()%100<rem)c='R';eval(b,pr,name,z);}}
ofstream f(dir+"transition-stats.txt");f<<"heat_checks="<<checks<<" thermally_valid="<<valid<<"\n";return 0;}
