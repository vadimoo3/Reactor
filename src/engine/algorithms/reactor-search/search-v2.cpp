#include "rx.h"
using namespace std;const int W=19,H=16,N=304;vector<int>a[N];mt19937 rng;double best[2][2][3]={};long long attempts=0,layouts=0,variants=0;string dir="";static const int svMinF=(int)PI("svMinF"),svExactF=(int)PI("svExactF");static const double svMaxH=P("svMaxH");
void eval(string g,int mode,long long seed){
if(mode==0){for(int i=0;i<N;i++)if(g[i]=='C'){bool ok=0;for(int j:a[i])ok|=g[j]=='I';if(!ok)g[i]='R';}for(int i=0;i<N;i++)if(g[i]=='F'){bool ok=0;for(int j:a[i])ok|=g[j]=='C';if(!ok)g[i]='R';}for(int i=0;i<N;i++)if(g[i]=='C'){bool ok=0;for(int j:a[i])ok|=g[j]=='F';if(!ok)g[i]='K';}for(int i=0;i<N;i++)if(g[i]=='I'){bool ok=0;for(int j:a[i])ok|=g[j]=='C';if(!ok)g[i]='K';}}
for(int i=0;i<N;i++)if(g[i]=='K')for(int j:a[i])if(g[j]=='F'){g[i]='R';break;}
int nf=count(g.begin(),g.end(),'F');if(svExactF>0?nf!=svExactF:nf<svMinF)return;layouts++;
double heat=0,bare=0;array<double,N>load{},p{};
for(int i=0;i<N;i++)if(g[i]=='F'){int f=0,c=0;for(int j:a[i]){f+=g[j]=='F';c+=g[j]=='C';}int pulse=12+4*f;double h=pulse*pulse*EM.FH/4;p[i]=pulse*EM.FP;bare+=p[i];heat+=h;if(c)for(int j:a[i])if(g[j]=='C')load[j]+=h/c;}
if(heat>svMaxH)return;
vector<pair<double,int>>sh;for(int i=0;i<N;i++)if(g[i]=='R'){double v=0;for(int j:a[i])v+=p[j]*EM.refl;sh.push_back({v,i});}sort(sh.begin(),sh.end());
for(int nr=0;nr<2;nr++)for(int np:{12,16,20,24,28}){
 string b=g;int pc=0;for(auto [v,i]:sh){if(nr&&pc==np)break;b[i]='P';pc++;}for(int i=N-1;i>=0&&pc<np;i--)if(b[i]=='K'){b[i]='P';pc++;}if(pc<np||EM.hull*pc<4*heat)continue;
 double power=bare;for(int i=0;i<N;i++)if(b[i]=='R')for(int j:a[i])power+=p[j]*EM.refl;
 bool useful=0;for(int pr=0;pr<3;pr++)useful|=power>best[mode][nr][pr]+1e-8;if(!useful)continue;
 int free=count(b.begin(),b.end(),'K');for(int phase=0;phase<5;phase++){vector<int>centers;for(int i=0;i<N;i++)if((i%W+2*(i/W))%5==phase&&b[i]=='K'&&a[i].size()==4){bool ok=1;for(int j:a[i]){ok&=b[j]=='K';for(int k:a[j])ok&=b[k]!='I';}if(ok)centers.push_back(i);}
 for(int no=2;no<=centers.size();no++){int k=free-5*no;double rate=EM.rate*(1+EM.rateK*k);bool ok=1;for(int i=0;i<N;i++)if(b[i]=='C'){int ins=0;for(int j:a[i])ins+=b[j]=='I';if(load[i]>.8*rate*ins)ok=0;}if(!ok)continue;
 for(int pr=0;pr<3;pr++){if(power<=best[mode][nr][pr]+1e-8)continue;variants++;double vent=EM.vent*(pr?8:7)*(1+(pr==2?.35:.30)*k);double vcap=EM.vcap*(pr?8:7)*(1+(pr?.30:.25)*pc);if(4*no*vent<1.15*heat||4*(no-1)*vent<heat||heat/(4*(no-1))>.5*vcap)continue;
 best[mode][nr][pr]=power;string out=b;for(int z=0;z<no;z++){out[centers[z]]='O';for(int j:a[centers[z]])out[j]='V';}string name=string(mode?"direct":"buffer")+(nr?"-R":"-noR")+"-p"+to_string(pr);ofstream f(dir+name+".txt");f<<setprecision(12)<<power<<" "<<heat<<" "<<4*no*vent<<" "<<4*(no-1)*vent<<" "<<pc<<" "<<seed<<"\n";for(int y=0;y<H;y++)f<<out.substr(y*W,W)<<"\n";cerr<<name<<" "<<power<<" F"<<nf<<" H"<<heat<<"\n";
 }}}
}}
int main(int argc,char**argv){for(int i=0;i<N;i++){if(i%W)a[i].push_back(i-1);if(i%W<18)a[i].push_back(i+1);if(i>=W)a[i].push_back(i-W);if(i+W<N)a[i].push_back(i+W);}long long runs=PI("svRuns"),att=PI("svAtt");for(int run=0;run<runs;run++){rng.seed((unsigned)(PI("svSeed")+run*PI("svStep")));for(long long z=0;z<att;z++){if(z&&z%500000==0)cerr<<"запуск "<<run<<" попыток "<<z<<" подходящих "<<layouts<<"\n";attempts++;int mode=z%2,tw=2+rng()%6,th=2+rng()%6,bw=7+rng()%12,bh=6+rng()%10;if(bw*bh<65||bw*bh>220)continue;string t(tw*th,'F');if(mode==0){int ni=1+rng()%max(1,tw*th/4);for(int j=0;j<ni;j++)t[rng()%t.size()]='I';for(int y=0;y<th;y++)for(int x=0;x<tw;x++)if(t[y*tw+x]=='F'&&(t[y*tw+(x+1)%tw]=='I'||t[y*tw+(x+tw-1)%tw]=='I'||t[((y+1)%th)*tw+x]=='I'||t[((y+th-1)%th)*tw+x]=='I'))t[y*tw+x]='C';}else{int prob=25+rng()%51;for(char &c:t)if(rng()%100<prob)c='R';}
string g(N,'K');for(int y=0;y<bh;y++)for(int x=0;x<bw;x++)g[y*W+x]=t[(y%th)*tw+x%tw];if(z%7==0)for(int j=0;j<1+rng()%12;j++){int i=(rng()%bh)*W+rng()%bw;if(g[i]=='F')g[i]='R';else if(g[i]=='R')g[i]='F';}eval(g,mode,run*att+z);}
ofstream s(dir+"search-stats.txt");s<<"attempts="<<attempts<<" qualified_layouts="<<layouts<<" improving_cooling_checks="<<variants<<"\n";cerr<<"RUN "<<run<<" DONE\n";}return 0;}
