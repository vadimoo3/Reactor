// Порт verify-v2.py: свойства всех схем поиска -> results.json и verification-input.txt.
#include "rx.h"
using namespace std;namespace fs=std::filesystem;static vector<int>A[304];
static int core(char c){return c=='F'?4:c=='1'?1:c=='2'?2:c=='4'?4:0;}
static string num(double v){ostringstream o;o<<setprecision(15)<<v;return o.str();}
static string props(const string&g,const string&stem,int pr,int dl,int nl,int ff){
 map<char,int>count;for(char c:g)count[c]++;vector<double>load(304,0.);double direct=0,H=0,base=0;
 for(int i=0;i<304;i++){int co=core(g[i]);if(!co)continue;for(int j:A[i])if(g[j]=='K'||g[j]=='V')cerr<<stem<<": стержень рядом с K/V в клетке "<<i<<"\n";
  int pulse=(co==1?1:co==2?4:12),r=0;vector<int>cs;for(int j:A[i]){pulse+=core(g[j]);r+=g[j]=='R';if(g[j]=='C')cs.push_back(j);}double h=pulse*pulse*(g[i]=='F'?EM.FH:EM.NH)/co;
  H+=h;base+=pulse*(g[i]=='F'?EM.FP*(1+.5*dl):EM.NH*(1+nl))*(1+EM.refl*r);if(!cs.empty())for(int j:cs)load[j]+=h/cs.size();else direct+=h;}
 for(int i=0;i<304;i++){if(g[i]=='O'){bool ok=A[i].size()==4;for(int j:A[i])ok&=g[j]=='V';if(!ok)cerr<<stem<<": Outlet "<<i<<" не окружён Vent\n";}
  if(g[i]=='V'){int o=0;bool ni=true;for(int j:A[i]){o+=g[j]=='O';ni&=g[j]!='I';}if(o!=1||!ni)cerr<<stem<<": Vent "<<i<<" нарушает правило O/I\n";}
  if(g[i]=='C'){bool ok=false;for(int j:A[i])ok|=g[j]=='I';if(!ok)cerr<<stem<<": Coolant "<<i<<" без Inlet\n";}}
 int k=count['K'],v=count['V'],p=count['P'];double rate=EM.rate*(1+EM.rateK*k),vent=EM.vent*(pr?8:7)*(1+(pr==2?.35:.3)*k),vcap=EM.vcap*(pr?8:7)*(1+(pr?.3:.25)*p),hcap=EM.hull*p;
 double sum=accumulate(load.begin(),load.end(),0.),held=max(0.,sum-hcap*.0001),mult=held>1e-6?1+.01*(5+ff)*log(held*1e9)/log(1000.):1;
 map<char,double>price{{'F',18.982},{'C',.8},{'I',.256},{'O',.256},{'V',.195312},{'K',.65536},{'P',.65536},{'R',.000005}};double costT=3586.*count['1']+7889.*count['2']+17356.*count['4'];for(auto[c,pv]:price)costT+=count[c]*pv;
 double upkeep=count['F']*18982./21600+(count['1']*3586000.+count['2']*7889000.+count['4']*17356000.)/86400;
 ostringstream o;o<<"{\"counts\":{";bool first=true;for(auto[c,n]:count){o<<(first?"":",")<<'"'<<c<<"\":"<<n;first=false;}o<<"},";
 auto kv=[&](const char*k,double x){o<<'"'<<k<<"\":"<<num(x)<<',';};
 kv("heat",H);kv("base_power",base);kv("warm_power",base*mult);kv("hull_at_start",held);kv("cooling",v*vent);kv("cooling_one_outlet_lost",(v-4)*vent);kv("vent_capacity",vcap);kv("vent_rate",vent);kv("hull_capacity",hcap);
 kv("max_coolant_load",*max_element(load.begin(),load.end()));kv("build_cost_T",costT);kv("fuel_cost_per_tick_B",upkeep);kv("profile",pr);kv("dol_power_extra",dl);kv("nef_power_extra",nl);kv("fission_extra",ff);kv("direct",direct);kv("rate",rate);
 o<<"\"grid\":[";for(int y=0;y<16;y++)o<<(y?",":"")<<'"'<<g.substr(y*19,19)<<'"';o<<"]}";
 cout<<stem<<": тепло "<<H<<" мощность "<<base<<" с прогревом "<<base*mult<<" цена "<<costT<<" T\n";return o.str();}
int main(){for(int i=0;i<304;i++){if(i%19)A[i].push_back(i-1);if(i%19<18)A[i].push_back(i+1);if(i>=19)A[i].push_back(i-19);if(i+19<304)A[i].push_back(i+19);}
 vector<fs::path>files;for(auto&e:fs::directory_iterator("."))if(e.path().extension()==".txt")files.push_back(e.path());sort(files.begin(),files.end());
 ofstream runs("verification-input.txt"),js("results.json");js<<"{\n";bool first=true;int done=0;
 for(auto&f:files){string stem=f.stem().string();if(stem.rfind("buffer-",0)&&stem.rfind("direct-",0)&&stem.rfind("hybrid-",0)&&stem.rfind("transition-",0))continue;
  ifstream in(f);vector<string>lines;string l;while(getline(in,l)){if(!l.empty()&&l.back()=='\r')l.pop_back();lines.push_back(l);}if(lines.size()!=17)continue;
  string g;for(int y=1;y<17;y++)g+=lines[y];if(g.size()!=304)continue;istringstream hs(lines[0]);vector<string>head;string t;while(hs>>t)head.push_back(t);
  bool tr=!stem.rfind("transition",0);int pr=tr&&head.size()>5?stoi(head[5]):stem.back()-'0',dl=tr&&head.size()>6?stoi(head[6]):0,nl=tr&&head.size()>7?stoi(head[7]):0,ff=tr&&head.size()>12?stoi(head[12]):0;
  js<<(first?"":",\n")<<'"'<<stem<<"\":"<<props(g,stem,pr,dl,nl,ff);first=false;runs<<stem<<' '<<pr<<' '<<dl<<' '<<nl<<' '<<ff<<' '<<g<<'\n';done++;}
 js<<"\n}\n";cout<<"схем: "<<done<<" -> results.json, verification-input.txt\n";return 0;}
