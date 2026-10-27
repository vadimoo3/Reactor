// Отжиг по сетке любого размера (наработка Reactor Lab), модель = reactor-protium/search.cpp.
#include "rx.h"
using namespace std;
struct Layout{int W=19,H=16;string g;int f[5]={1,1,1,1,0};};
// Общая цель всех способов (критерий reactor-profit): чистая мощность = min(производство, автопродажа) − цена заправки / срок топлива, × темп Chrono.
struct Ev{double generated=0,cost=0,P=0,Ptot=0,net=0,Ntot=0,H=0,cool=0,deficit=0;int bad=0,fuel=0;bool feed=false;};
struct Par{double secs,T0,T1,pen,margin;unsigned seed;int mode,threads,W,H,minF,maxF,steps;long long ips;bool fl[5],a1,a2,a4,tOut,tVent,m2,noSim,noCool,cool,route;};
struct Adj{vector<array<int,4>>nb;vector<int>cnt;};
static const Adj&getAdj(int W,int H){static thread_local map<int,Adj>c;int key=W*1000+H;auto it=c.find(key);if(it!=c.end())return it->second;
 Adj A;A.nb.resize(W*H);A.cnt.assign(W*H,0);for(int i=0;i<W*H;i++){int x=i%W,y=i/W;auto add=[&](int j){A.nb[i][A.cnt[i]++]=j;};if(x>0)add(i-1);if(x<W-1)add(i+1);if(y>0)add(i-W);if(y<H-1)add(i+W);}
 return c[key]=A;}
static int core(char c){return c=='1'?1:c=='2'?2:c=='4'?4:0;}
// Масштаб очков топлива относительно Protium: температуры и штрафы заданы в единицах Protium, у Dolorium/Nefastium мощность в тысячи раз меньше.
static double unitScale(){return max(1e-12,M.fuelPow/2500.);}
// Путь тепла по соседям (как в simulate): тепло стержня делится между соседями с ёмкостью, без них уходит в корпус; корпус сбрасывает через Outlet;
// Exchanger объединяет соседей в сеть теплообмена. Возвращает перегрузку: сеть не успевает снимать тепло своими Vent, корпус переполнен, тепло в Capacitor.
static double localOver(const Layout&L,const array<int,256>&n){const Adj&A=getAdj(L.W,L.H);int N=L.W*L.H;const string&g=L.g;double pp=n['P'],k=n['K'];
 double rate=M.rate0*(M.bEx+L.f[2])*2*(1+M.capBoost*k),vr=M.vr0*(1+M.ventBoost*(M.bAv+L.f[0])*k);
 double vc=M.vc0*(1+M.sinkBoost*(M.bHs+L.f[1])*pp),ec=M.ec0*(M.bEx+L.f[2])*(1+M.reBoost*(M.bRe+L.f[3])*pp),hc=M.hullPer*max(pp,1.);
 auto capOf=[&](char c){return c=='E'?ec:c=='V'?vc:c=='C'?M.ccap:c=='K'?M.kcap:0.;};
 vector<double>ld(N,0);double hull=0;
 for(int i=0;i<N;i++)if(int co=core(g[i])){int pulse=co==1?1:co==2?4:12,s=0;for(int a=0;a<A.cnt[i];a++){pulse+=core(g[A.nb[i][a]]);s+=capOf(g[A.nb[i][a]])>0;}
  double q=pulse*pulse*M.fuelHeat/co;if(s){for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];if(capOf(g[j])>0)ld[j]+=q/s;}}else hull+=q;}
 int br=0;for(int i=0;i<N;i++)if(g[i]=='O')for(int a=0;a<A.cnt[i];a++)br+=capOf(g[A.nb[i][a]])>0;
 // круг корпуса: Outlet раздаёт корпус соседям, Inlet забирает у соседей обратно в корпус; несколько проходов до установившегося потока
 vector<double>base=ld;double h0=hull;for(int it=0;it<12;it++){ld=base;double flow=br?min(hull,br*rate):0;
  if(br)for(int i=0;i<N;i++)if(g[i]=='O')for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];if(capOf(g[j])>0)ld[j]+=flow/br;}
  double pulled=0;for(int i=0;i<N;i++)if(g[i]=='I')for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];double t=min(rate,max(0.,ld[j]-(g[j]=='V'?vr:0)));ld[j]-=t;pulled+=t;}hull=h0+pulled;}
 double over=max(0.,hull-br*rate-hc*M.drain)+(h0>=hc?h0:0);
 // сети теплообмена: Exchanger связывает себя с соседями с ёмкостью; в каждой сети приток минус съём Vent не должен заполнить её ёмкость за симуляцию
 vector<int>par(N);iota(par.begin(),par.end(),0);function<int(int)>fd=[&](int x){return par[x]==x?x:par[x]=fd(par[x]);};
 for(int i=0;i<N;i++)if(g[i]=='E')for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];if(g[j]!='K'&&capOf(g[j])>0)par[fd(j)]=fd(i);}
 vector<double>in(N,0),rm(N,0),cp(N,0);for(int i=0;i<N;i++){double c=capOf(g[i]);if(c<=0)continue;if(g[i]=='K'){over+=ld[i]>0?ld[i]:0;continue;}int r=fd(i);in[r]+=ld[i];cp[r]+=c;if(g[i]=='V')rm[r]+=vr;}
 for(int i=0;i<N;i++)if(fd(i)==i&&cp[i]>0){double net=in[i]-rm[i];if(net>0&&net*M.simSteps>=cp[i]*0.98)over+=net;}
 return over;}
static double density(const Layout&L,int i){static const bool mono=S("fuelName")=="monastium";if(!mono)return 1.;int n=0,x=i%L.W,y=i/L.W;for(int yy=max(0,y-3);yy<=min(L.H-1,y+3);yy++)for(int xx=max(0,x-3);xx<=min(L.W-1,x+3);xx++)n+=L.g[yy*L.W+xx]!='.';return 1.-.02*n;}
static Ev evalG(const Par&p,const Layout&L){const Adj&A=getAdj(L.W,L.H);int N=L.W*L.H;const string&g=L.g;array<int,256>n{};for(char c:g)n[(unsigned char)c]++;Ev e;
 e.cost=PR.fuel(n)+PR.upgrades(L.f[0],L.f[1],L.f[2],L.f[3],L.f[4])+PR.parts(n);double H=0,Pw=0;
 for(int i=0;i<N;i++){int co=core(g[i]);if(!co)continue;e.fuel++;int pulse=co==1?1:co==2?4:12,r=0,cc=0;
  for(int k=0;k<A.cnt[i];k++){int j=A.nb[i][k];pulse+=core(g[j]);r+=g[j]=='R';e.bad+=g[j]=='K';cc+=g[j]=='C';}
  H+=pulse*pulse*M.fuelHeat/co;Pw+=pulse*M.fuelPow*(1+M.refl*r)*density(L,i);if(co==1&&cc==4)e.feed=true;}
 double held=0;int branches=0,returning=0;
 for(int i=0;i<N;i++){if(core(g[i])){int sinks=0,cc=0,pulse=core(g[i])==1?1:core(g[i])==2?4:12;for(int a=0;a<A.cnt[i];a++){char c=g[A.nb[i][a]];sinks+=c=='E'||c=='V'||c=='C'||c=='K';cc+=c=='C';pulse+=core(c);}if(sinks)held+=pulse*pulse*M.fuelHeat/core(g[i])*cc/sinks;}
  if(g[i]=='O')for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];char c=g[j];branches+=c=='E'||c=='V'||c=='C'||c=='K';if(c=='C')for(int b=0;b<A.cnt[j];b++)if(g[A.nb[j][b]]=='I'){returning++;break;}}}
 if(returning&&branches>returning)held+=H*returning/(branches-returning);e.feed=held>0;if(e.feed){held-=n['P']*M.hullPer*M.drain;if(held>0)Pw*=max(1.,M.boost(held));}
 e.generated=Pw;Pw=min(Pw,M.sales(n['K']));e.P=Pw;e.Ptot=Pw*(M.bCh+L.f[4])/M.bCh;e.net=Pw-PR.fuel(n)*1e6/M.fuelLife;e.Ntot=e.net*(M.bCh+L.f[4])/M.bCh;e.H=H;e.cool=n['V']*M.vr0*(1+M.ventBoost*(M.bAv+L.f[0])*n['K']);
 e.deficit=p.noCool?0:max(0.,H*(1+p.margin/100.)-e.cool);if(p.route&&!p.noCool)e.deficit+=localOver(L,n);return e;}
struct SimRes{bool ok=false;double mE=0,mV=0,mC=0,mH=0,sold=0,gross=0;int worstV=-1;string why;};
static SimRes simulate(const Layout&L,int method,int fault,int steps){const Adj&A=getAdj(L.W,L.H);int N=L.W*L.H;const string&g=L.g;SimRes R;
 vector<double>h(N,0),cap(N,0),source(N,0),previous(N,0);bool skip=PI("simSkip")!=0;array<int,256>n{};for(char c:g)n[(unsigned char)c]++;double pp=n['P'],k=n['K'];
 double rate=M.rate0*(M.bEx+L.f[2])*2*(1+M.capBoost*k),vr=M.vr0*(1+M.ventBoost*(M.bAv+L.f[0])*k);
 double vc=M.vc0*(1+M.sinkBoost*(M.bHs+L.f[1])*pp),ec=M.ec0*(M.bEx+L.f[2])*(1+M.reBoost*(M.bRe+L.f[3])*pp),hc=M.hullPer*max(pp,1.);
 vector<int>ex,vents,cells,tracked;vector<pair<int,int>>exchange;vector<int>outlets,inlets;vector<pair<char,vector<int>>>circuit;
 for(int i=0;i<N;i++){cap[i]=g[i]=='E'?ec:g[i]=='V'?vc:g[i]=='C'?M.ccap:g[i]=='K'?M.kcap:0;if(cap[i]>0)tracked.push_back(i);if(g[i]=='E')ex.push_back(i);if(g[i]=='V')vents.push_back(i);if(g[i]=='C')cells.push_back(i);}
 for(int i:ex){int c=A.cnt[i];for(int a=0;a<c;a++){int j=A.nb[i][method%2?c-1-a:a];if(cap[j]>0)exchange.push_back({i,j});}}
 for(int i=0;i<N;i++)if(g[i]=='O'||g[i]=='I')for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];if(cap[j]>0){if(g[i]=='O')outlets.push_back(j);else inlets.push_back(j);}}
 for(int i=0;i<N;i++)if(g[i]=='O'||g[i]=='I'){vector<int>neighbors;for(int a=0;a<A.cnt[i];a++)if(cap[A.nb[i][a]]>0)neighbors.push_back(A.nb[i][a]);circuit.push_back({g[i],move(neighbors)});}
 double hull=0,basePower=0,hullSource=0;int samples=0;
 for(int i=0;i<N;i++)if(int co=core(g[i])){int pulse=co==1?1:co==2?4:12,refs=0,sinks=0;for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];pulse+=core(g[j]);refs+=g[j]=='R';sinks+=cap[j]>0;}
  basePower+=pulse*M.fuelPow*(1+M.refl*refs)*density(L,i);double q=pulse*pulse*M.fuelHeat/co;if(sinks){for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];if(cap[j]>0)source[j]+=q/sinks;}}else hullSource+=q;}
 int ft=max(1,min(M.faultT,steps-1));
 for(int t=0;t<steps;t++){
  bool probe=skip&&t%32==1;double previousHull=hull;if(probe)previous=h;
  bool on=!((fault==3&&t>=M.expStart&&t<M.expEnd)||(fault==4&&t>=M.expStart));
  double gross=on?basePower*(hull>0?M.boost(hull):1.):0;double sold=min(gross,M.sales(k));
  if(t>=max(0,steps-1000)){R.sold+=sold;R.gross+=gross;samples++;}
  if(on){hull+=hullSource;for(int i:tracked)h[i]+=source[i];}
  for(int pass=0;pass<(method%2?2:1);pass++)for(auto ij:exchange){int i=ij.first,j=ij.second;double delta=(h[i]+h[j])/(cap[i]+cap[j])*cap[j]-h[j];double q=clamp(clamp(delta,-rate,rate),-h[j],h[i]);h[j]+=q;h[i]-=q;}
  double flow=outlets.empty()?0:min(hull,outlets.size()*rate);if(fault==1&&t>=ft&&t<ft+M.outFault)flow=0;
  if(method<2){hull-=flow;if(!outlets.empty())for(int j:outlets)h[j]+=flow/outlets.size();for(int i:vents)h[i]=max(0.,h[i]-((fault==2&&t>=ft&&t<ft+M.ventFault)?0:vr));for(int j:inlets){double q=min(rate,h[j]);h[j]-=q;hull+=q;}}
  else{double per=outlets.empty()?0:flow/outlets.size();auto apply=[&](const pair<char,vector<int>>&c){for(int j:c.second){if(c.first=='O'){double q=min(per,hull);h[j]+=q;hull-=q;}else{double q=min(rate,h[j]);h[j]-=q;hull+=q;}}};
   if(method==2||method==3){for(int u=0;u<(int)circuit.size();u++)apply(circuit[method==3?circuit.size()-1-u:u]);}
   else{char first=method==4?'O':'I';for(auto&c:circuit)if(c.first==first)apply(c);for(auto&c:circuit)if(c.first!=first)apply(c);}
   for(int i:vents)h[i]=max(0.,h[i]-((fault==2&&t>=ft&&t<ft+M.ventFault)?0:vr));}
  for(int i:ex)R.mE=max(R.mE,h[i]/cap[i]);for(int i:vents)if(h[i]/cap[i]>R.mV){R.mV=h[i]/cap[i];R.worstV=i;}for(int i:cells)R.mC=max(R.mC,h[i]/cap[i]);R.mH=max(R.mH,hull/hc);
  if(hull>=hc){R.why="корпус t="+to_string(t);return R;}for(int i:tracked)if(h[i]>=cap[i]){R.why=string(1,g[i])+" t="+to_string(t);return R;}
  hull=max(0.,hull-hc*M.drain);
  if(probe&&hull==previousHull&&h==previous){int until=steps;for(int event:{ft,ft+M.outFault,ft+M.ventFault,M.expStart,M.expEnd,steps-1000})if(event>t+1)until=min(until,event);int sampleTicks=max(0,until-max(t+1,steps-1000));R.sold+=sold*sampleTicks;R.gross+=gross*sampleTicks;samples+=sampleTicks;t=until-1;}}
 R.sold/=max(1,samples);R.gross/=max(1,samples);R.ok=true;return R;}
static Ev measuredEval(const Par&p,const Layout&L){Ev e=evalG(p,L);if(e.bad||e.fuel<1)return e;
 double sold=1e300,gross=1e300;for(int m=0;m<6;m++){auto r=simulate(L,m,0,max(M.simSteps,M.expEnd+(int)M.fuelLife));if(!r.ok){e.bad++;return e;}sold=min(sold,r.sold);gross=min(gross,r.gross);}
 e.generated=gross;e.P=sold;e.Ptot=sold*(M.bCh+L.f[4])/M.bCh;array<int,256>n{};for(char c:L.g)n[(unsigned char)c]++;e.net=sold-PR.fuel(n)*1e6/M.fuelLife;e.Ntot=e.net*(M.bCh+L.f[4])/M.bCh;return e;}
// Общая проверка (стандарт reactor-protium + остывание из reactor-full-random/modules): 2 порядка обмена × штатно, Outlet off, Vent off, выгорание топлива.
static bool verify(const Par&p,const Layout&L,string*rep=nullptr,double*fill=nullptr){if(fill)*fill=0;vector<int>faults={0};if(p.tOut)faults.push_back(1);if(p.tVent)faults.push_back(2);if(p.cool){faults.push_back(3);faults.push_back(4);}bool all=true;char buf[256];
 for(int m=0;m<(p.m2?6:1);m++)for(int f:faults){SimRes r=simulate(L,m,f,p.steps);if(fill)*fill=max(*fill,max(max(r.mE,r.mV),max(r.mC,r.mH)));
  if(rep){snprintf(buf,sizeof buf,"  обмен %d, %s: %s  Exch %.1f%% Vent %.1f%% Cool %.1f%% Корпус %.1f%% %s\n",m+1,f==0?"штатно":f==1?"Outlet off":f==2?"Vent off":f==3?"замена топлива":"выгорание",r.ok?"OK":"СБОЙ",r.mE*100,r.mV*100,r.mC*100,r.mH*100,r.ok?"":r.why.c_str());*rep+=buf;}
  if(!r.ok){all=false;if(!rep)return false;}}return all;}
static bool parseLayout(const string&path,Layout&L){ifstream f(path);if(!f)return false;string line,head;vector<string>rows;bool first=true;
 while(getline(f,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();bool grid=line.size()>=3&&line.find_first_not_of("124FEVOICPKR.")==string::npos;if(grid)rows.push_back(line);else if(first)head=line;first=false;}
 if(rows.size()<3)return false;for(auto&s:rows)if(s.size()!=rows[0].size())return false;L.W=(int)rows[0].size();L.H=(int)rows.size();L.g.clear();for(auto&s:rows)L.g+=s;for(char&c:L.g)if(c=='F')c='4';
 istringstream hs(head);vector<double>v;double x;while(hs>>x)v.push_back(x);if(v.size()>=9)for(int i=0;i<5;i++)L.f[i]=(int)v[4+i];return true;}
static void saveLayout(const string&path,const Layout&L,const Ev&e,unsigned seed){ofstream o(path);o<<setprecision(12)<<e.P<<" "<<e.H<<" "<<e.cool<<" "<<e.cost<<" "<<L.f[0]<<" "<<L.f[1]<<" "<<L.f[2]<<" "<<L.f[3]<<" "<<L.f[4]<<" "<<seed<<"\n";
 for(int y=0;y<L.H;y++)o<<L.g.substr(y*L.W,L.W)<<"\n";}
struct Shared{mutex m;Layout best;Ev bestE;double nearS=-1e300;Ev nearE;long long nearSimFail=0;atomic<double>bestP{0};atomic<long long>iters{0},sims{0},passed{0};};
static string pool(const Par&p){string s="EEEEVVVVVVOIICCPPPPPKKKKKRRRR....";if(p.a1)s+="1111";if(p.a2)s+="2";if(p.a4)s+="4";return s;}
static double scoreOf(const Par&p,const Ev&e){if(e.cost>PR.budget)return -1e18;double u=unitScale(),s=e.Ntot-p.pen*e.deficit-1e4*u*e.bad;if(p.maxF&&e.fuel>p.maxF)s-=1e6*u*(e.fuel-p.maxF);return s;}
static bool fuelOK(const Par&p,const Ev&e){return e.fuel>=max(1,p.minF)&&(!p.maxF||e.fuel<=p.maxF);}
// Стержень с кольцом: топливо в клетке i, соседи-Capacitor становятся Reflector (иначе одна клетка топлива рядом с Capacitor всегда штрафуется); у Single иногда блок Forceful: крест Coolant и Inlet по углам.
template<class R>static void fuelRing(string&g,const Adj&A,int i,const string&fp,R&rng,int W=0,int H=0){if(fp.empty())return;g[i]=fp[rng()%fp.size()];int x=W?i%W:0,y=W?i/W:0;
 if(W&&g[i]=='1'&&rng()%2&&x>0&&y>0&&x<W-1&&y<H-1){for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if(dx||dy)g[(y+dy)*W+x+dx]=dx&&dy?'I':'C';return;}
 for(int k=0;k<A.cnt[i];k++){char&c=g[A.nb[i][k]];if(c=='K')c='R';}}
// Повтор участка: прямоугольник bw×bh копируется в другое место сетки (размножает удачный узор).
template<class R>static void stamp(string&g,int W,int H,R&rng){int bw=2+rng()%min(5,W-1),bh=2+rng()%min(5,H-1);int sx=rng()%(W-bw+1),sy=rng()%(H-bh+1),dx=rng()%(W-bw+1),dy=rng()%(H-bh+1);
 string src=g;for(int y=0;y<bh;y++)for(int x=0;x<bw;x++)g[(dy+y)*W+dx+x]=src[(sy+y)*W+sx+x];}
static string fuelPool(const Par&p){string fp;if(p.a1)fp+='1';if(p.a2)fp+='2';if(p.a4)fp+='4';return fp;}
static Par readPar(){Par p{};p.mode=(int)PI("anMode");p.secs=P("anSecs");p.seed=(unsigned)PI("anSeed");p.threads=clamp((int)PI("anThreads"),1,max(1,(int)thread::hardware_concurrency()/2));p.ips=max(1LL,PI("anIps"));p.T0=P("anT0")*unitScale();p.T1=P("anT1")*unitScale();p.pen=P("anPen");p.margin=P("anMargin");
 const char*fk[5]={"anFav","anFhs","anFex","anFre","anFch"};for(int i=0;i<5;i++)p.fl[i]=PI(fk[i])!=0;p.a1=PI("anA1")!=0;p.a2=PI("anA2")!=0;p.a4=PI("anA4")!=0;
 p.minF=(int)PI("anMinF");p.maxF=(int)PI("anMaxF");p.W=(int)PI("anW");p.H=(int)PI("anH");p.steps=max(M.simSteps,M.expEnd+(int)M.fuelLife);p.tOut=PI("anOut")!=0;p.tVent=PI("anVent")!=0;p.m2=PI("anM2")!=0;p.noSim=PI("anNoSim")!=0;p.noCool=PI("anNoCool")!=0;p.route=true;return p;}
struct Tile{int w=3,h=3,ox=0,oy=0;string t;int f[5]={0,0,0,0,0};};
static Layout expandTile(const Par&p,const Tile&T){Layout L;L.W=p.W;L.H=p.H;L.g.resize(p.W*p.H);for(int y=0;y<p.H;y++)for(int x=0;x<p.W;x++)L.g[y*p.W+x]=T.t[((y+T.oy)%T.h)*T.w+(x+T.ox)%T.w];for(int i=0;i<5;i++)L.f[i]=T.f[i];return L;}
static void tileThread(const Par&p,Shared&S,int tid,const vector<Tile>&seeds,chrono::steady_clock::time_point t0){
 mt19937 rng(p.seed*1000003u+tid*7919u+17);string pl=pool(p),fpool=fuelPool(p);auto U=[&](){return (rng()&0xFFFFFF)/16777216.;};
 int maxW=min(p.W,10),maxH=min(p.H,10);long long simFail=0;double nearS=-1e300;Ev nearE;
 auto randomTile=[&](){Tile T;T.w=3+rng()%(maxW-2);T.h=3+rng()%(maxH-2);T.t.assign(T.w*T.h,'K');for(auto&c:T.t)c=pl[rng()%pl.size()];for(int i=0;i<5;i++)T.f[i]=p.fl[i]?(int)(rng()%2):0;return T;};
 auto ringT=[&](Tile&T,int i){if(fpool.empty())return;int x=i%T.w,y=i/T.w;T.t[i]=fpool[rng()%fpool.size()];bool feed=T.t[i]=='1'&&T.w>=3&&T.h>=3&&rng()%2;if(feed){for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if(dx||dy)T.t[((y+dy+T.h)%T.h)*T.w+(x+dx+T.w)%T.w]=dx&&dy?'I':'C';return;}int nb[4]={y*T.w+(x+1)%T.w,y*T.w+(x+T.w-1)%T.w,((y+1)%T.h)*T.w+x,((y+T.h-1)%T.h)*T.w+x};for(int j:nb)if(j!=i&&T.t[j]=='K')T.t[j]='R';};
 auto resize=[&](const Tile&T,int w,int h){Tile R=T;R.w=w;R.h=h;R.t.resize(w*h);for(int y=0;y<h;y++)for(int x=0;x<w;x++)R.t[y*w+x]=T.t[(y%T.h)*T.w+x%T.w];R.ox%=w;R.oy%=h;return R;};
 for(int k=0;;k++){
  Tile cur=k*p.threads+tid<(int)seeds.size()?seeds[k*p.threads+tid]:(k%2&&!seeds.empty()?seeds[rng()%seeds.size()]:randomTile());
  Ev ce=evalG(p,expandTile(p,cur));double cs=scoreOf(p,ce),lastTry=0,T=p.T0;
  for(long long it=0;it<p.ips;it++){
   if((it&1023)==0){double el=chrono::duration<double>(chrono::steady_clock::now()-t0).count();if(el>p.secs)goto done;S.iters+=1024;T=p.T0*pow(p.T1/p.T0,double(it)/p.ips);}
   if(cs>nearS){nearS=cs;nearE=ce;}
   Tile nt=cur;int m=rng()%100,n=nt.w*nt.h;
   if(m<3){int w=rng()%5;if(p.fl[w])nt.f[w]^=1;}
   else if(m<25){int i=rng()%n,j=rng()%n;swap(nt.t[i],nt.t[j]);}
   else if(m<35)ringT(nt,rng()%n);
   else if(m<40){int w=nt.w,h=nt.h;if(rng()%2)w=clamp(w+(rng()%2?1:-1),2,maxW);else h=clamp(h+(rng()%2?1:-1),2,maxH);nt=resize(nt,w,h);}
   else if(m<44){nt.ox=rng()%nt.w;nt.oy=rng()%nt.h;}
   else nt.t[rng()%n]=pl[rng()%pl.size()];
   Layout L=expandTile(p,nt);Ev ne=evalG(p,L);double ns=scoreOf(p,ne);
   if(ns>=cs||U()<exp((ns-cs)/T)){cur=std::move(nt);ce=ne;cs=ns;
    if(fuelOK(p,ce)&&ce.deficit==0&&!ce.bad&&ce.cost<=PR.budget&&ce.Ntot>S.bestP*1.002&&ce.Ntot>lastTry*1.002){
     lastTry=ce.Ntot;bool ok=true;if(!p.noSim){S.sims++;ok=verify(p,L);if(!ok)simFail++;}
     if(ok){S.passed++;lock_guard<mutex>l(S.m);if(ce.Ntot>S.bestP){S.bestP=ce.Ntot;S.best=L;S.bestE=ce;saveLayout("best-current.txt",L,ce,p.seed);
       fprintf(stderr,"новый лучший: %.3f T/тик чистыми, стержней %d, блок %dx%d, цена %.3f Qa (поток %d, %.0f с)\n",ce.Ntot/1000,ce.fuel,cur.w,cur.h,ce.cost,tid,chrono::duration<double>(chrono::steady_clock::now()-t0).count());}}}}}}
 done:{lock_guard<mutex>l(S.m);S.nearSimFail+=simFail;if(nearS>S.nearS){S.nearS=nearS;S.nearE=nearE;}}}
int tileMain(){Par p=readPar();p.mode=2;if(!p.a1&&!p.a2&&!p.a4){cerr<<"не разрешено ни одного типа стержней\n";return 2;}
 if(p.W<3||p.H<3||p.W>60||p.H>60){cerr<<"сетка вне 3..60\n";return 2;}
 // зёрна: окна разных размеров вокруг стержней каждой схемы-кандидата, лучшие по оценке
 namespace fs=std::filesystem;error_code ec;string dir=S("tlDir");vector<string>files;
 if(!dir.empty()&&fs::exists(dir,ec))for(auto&e:fs::recursive_directory_iterator(dir,ec))if(e.is_regular_file()&&e.path().extension()==".txt"&&e.file_size()<40000)files.push_back(e.path().string());
 sort(files.begin(),files.end());
 const int sz[][2]={{3,3},{4,3},{3,4},{4,4},{5,3},{3,5},{5,4},{4,5},{5,5},{6,4},{4,6},{6,5},{5,6},{6,6},{7,5},{5,7},{8,4},{4,8},{7,7},{8,6}};
 vector<pair<double,Tile>>cand;set<string>seen;int srcN=0;
 for(auto&f:files){Layout L;if(!parseLayout(f,L))continue;bool okRod=true;for(char c:L.g)if((c=='1'&&!p.a1)||(c=='2'&&!p.a2)||(c=='4'&&!p.a4))okRod=false;if(!okRod)continue;srcN++;
  vector<int>fu;for(int i=0;i<L.W*L.H;i++)if(core(L.g[i]))fu.push_back(i);if(fu.size()>40)fu.resize(40);
  for(int fi:fu)for(auto&s:sz){int w=min(s[0],L.W),h=min(s[1],L.H);if(w>p.W||h>p.H)continue;int x0=clamp(fi%L.W-w/2,0,L.W-w),y0=clamp(fi/L.W-h/2,0,L.H-h);
   Tile T;T.w=w;T.h=h;T.t.resize(w*h);for(int y=0;y<h;y++)for(int x=0;x<w;x++)T.t[y*w+x]=L.g[(y0+y)*L.W+x0+x];for(int i=0;i<5;i++)T.f[i]=L.f[i]&&p.fl[i];
   string key=to_string(w)+"x"+to_string(h)+T.t;if(!seen.insert(key).second)continue;cand.push_back({scoreOf(p,evalG(p,expandTile(p,T))),T});}}
 sort(cand.begin(),cand.end(),[](const pair<double,Tile>&a,const pair<double,Tile>&b){return a.first>b.first;});
 vector<Tile>seeds;for(size_t i=0;i<cand.size()&&seeds.size()<(size_t)max(8,4*p.threads);i++)seeds.push_back(cand[i].second);
 fprintf(stderr,"блоки %dx%d: схем-источников %d, блоков-зёрен %d (из %d), потоков %d, %.0f с\n",p.W,p.H,srcN,(int)seeds.size(),(int)cand.size(),p.threads,p.secs);
 Shared S;S.best.W=p.W;S.best.H=p.H;auto t0=chrono::steady_clock::now();vector<thread>th;for(int i=0;i<p.threads;i++)th.emplace_back(tileThread,cref(p),ref(S),i,cref(seeds),t0);
 for(auto&t:th)t.join();
 if(S.bestP>0){time_t tt=time(0);char ts[64];strftime(ts,sizeof ts,"%Y%m%d-%H%M%S",localtime(&tt));string fn=string("blocks-")+ts+".txt";saveLayout(fn,S.best,S.bestE,p.seed);
  fprintf(stderr,"готово: %.3f T/тик чистыми, стержней %d, цена %.3f Qa -> %s\n",S.bestP/1000,S.bestE.fuel,S.bestE.cost,fn.c_str());}
 else fprintf(stderr,"готово: блочной схемы, прошедшей проверку, не найдено (итераций %.1f млн)\n",S.iters/1e6);
 return 0;}

// Пересчёт одной схемы: флаги из файла (A) и флаги с переключателей отжига (B), с текущим сейвом и ценами.
int evalMain(){Par p{};p.margin=P("anMargin");p.noCool=false;Layout L;if(!parseLayout(S("plFile"),L)){cerr<<"не прочитана схема\n";return 2;}
 const char*fk[5]={"anFav","anFhs","anFex","anFre","anFch"};
 auto out=[&](const char*tag,const Layout&X){Ev e=measuredEval(p,X);printf("%s power=%.6g ptot=%.6g net=%.6g ntot=%.6g heat=%.6g cool=%.6g cost=%.6g fuel=%d bad=%d feed=%d deficit=%.6g budget=%.6g generated=%.12g flags=%d%d%d%d%d\n",tag,e.P,e.Ptot,e.net,e.Ntot,e.H,e.cool,e.cost,e.fuel,e.bad,(int)e.feed,e.deficit,PR.budget,e.generated,X.f[0],X.f[1],X.f[2],X.f[3],X.f[4]);};
 out("A",L);Layout B=L;for(int i=0;i<5;i++)B.f[i]=PI(fk[i])!=0;out("B",B);return 0;}

// Отбор: все схемы из pkDir (+ pkBase) -> только разрешённые стержни, сам подбирает покупку улучшений,
// проверяет полной симуляцией (оба порядка обмена, Outlet off, Vent off), лучший пишет в pkOut.
struct PickOpt{Layout L;Ev e;string src;double fill=0;int tier=0;};
static int pkCount=0;
// Топ-K непохожих (не меньше 8 отличающихся клеток) схем, прошедших проверку p, по чистой мощности.
// Для каждой схемы: при каждом уровне чистой мощности сначала самый сильный набор охлаждающих улучшений, затем самый дешёвый проходящий.
static vector<PickOpt> pickRun(const Par&p,const vector<string>&files,const bool rod[3],const bool up[5],int maxVerify,int K,int&tried,bool quiet){
 struct Grid{vector<PickOpt>opts;double top=0;};vector<Grid>grids;set<string>seen;int rejRod=0,rejFit=0,rejCount=0,parsed=0,variants=0;
 for(auto&f:files){Layout L;if(!parseLayout(f,L))continue;parsed++;if(!seen.insert(L.g).second)continue;bool okRod=true;for(char c:L.g){if(c=='1'&&!rod[0])okRod=false;if(c=='2'&&!rod[1])okRod=false;if(c=='4'&&!rod[2])okRod=false;}
  if(!okRod){rejRod++;continue;}
  if(pkCount>0){int cells=0;for(char c:L.g)cells+=c=='1'||c=='2'||c=='4';if(cells!=pkCount){rejCount++;continue;}}Grid G;
  for(int mask=0;mask<32;mask++){bool okm=true;for(int i=0;i<5;i++)if((mask>>i&1)&&!up[i])okm=false;if(!okm)continue;
   Layout X=L;for(int i=0;i<5;i++)X.f[i]=mask>>i&1;Ev e=evalG(p,X);if(e.cost>PR.budget||e.deficit>0||e.bad||e.fuel<1)continue;e=measuredEval(p,X);if(e.bad)continue;G.opts.push_back({X,e,f});}
  if(G.opts.empty()){rejFit++;continue;}
  sort(G.opts.begin(),G.opts.end(),[](const PickOpt&a,const PickOpt&b){return a.e.Ntot!=b.e.Ntot?a.e.Ntot>b.e.Ntot:a.e.cost<b.e.cost;});
  G.top=G.opts[0].e.Ntot;variants+=(int)G.opts.size();grids.push_back(move(G));}
 sort(grids.begin(),grids.end(),[](const Grid&a,const Grid&b){return a.top>b.top;});
 if(!quiet)fprintf(stderr,"отбор: файлов схем %d, уникальных полей %d, отклонено из-за стержней %d, из-за количества %d, не влезли в бюджет или охлаждение %d, подходящих схем %d, вариантов покупки %d\n",parsed,(int)seen.size(),rejRod,rejCount,rejFit,(int)grids.size(),variants);
 auto pc=[](const Layout&L){return L.f[0]+L.f[1]+L.f[2]+L.f[3];};
 auto hd=[](const string&a,const string&b){if(a.size()!=b.size())return 1<<20;int d=0;for(size_t i=0;i<a.size();i++)d+=a[i]!=b[i];return d;};
 vector<PickOpt>res;tried=0;
 for(auto&G:grids){if((int)res.size()>=K&&G.top<=res.back().e.Ntot)break;if(tried>=maxVerify)break;
  bool similar=false;for(auto&r:res)if(hd(r.L.g,G.opts[0].L.g)<8){similar=true;break;}if(similar)continue;
  map<int,pair<bool,double>>done;
  auto check=[&](const PickOpt&o,double&fl){int k=o.L.f[0]|o.L.f[1]<<1|o.L.f[2]<<2|o.L.f[3]<<3;auto it=done.find(k);if(it!=done.end()){fl=it->second.second;return it->second.first;}tried++;double f=0;bool ok=verify(p,o.L,nullptr,&f);done[k]={ok,f};fl=f;return ok;};
  vector<double>levels;for(auto&o:G.opts)if(levels.empty()||fabs(levels.back()-o.e.Ntot)>1e-9)levels.push_back(o.e.Ntot);
  for(double lv:levels){if((int)res.size()>=K&&lv<=res.back().e.Ntot)break;if(tried>=maxVerify)break;
   vector<PickOpt>c;for(auto&o:G.opts)if(fabs(o.e.Ntot-lv)<=1e-9)c.push_back(o);
   sort(c.begin(),c.end(),[&](const PickOpt&a,const PickOpt&b){return pc(a.L)!=pc(b.L)?pc(a.L)>pc(b.L):a.e.cost<b.e.cost;});
   double f0;if(!check(c[0],f0))continue;PickOpt chosen=c[0];chosen.fill=f0;
   sort(c.begin(),c.end(),[](const PickOpt&a,const PickOpt&b){return a.e.cost<b.e.cost;});
   int extra=0;for(auto&o:c){if(o.e.cost>=chosen.e.cost||extra>=15)break;extra++;double f1;if(check(o,f1)){chosen=o;chosen.fill=f1;break;}}
   res.push_back(chosen);sort(res.begin(),res.end(),[](const PickOpt&a,const PickOpt&b){return a.e.Ntot>b.e.Ntot;});if((int)res.size()>K)res.resize(K);break;}}
 return res;}
static string upNames(const int f[5]){const char*un[5]={"Active Venting","Heat Sinks","Heat Exch.","Reinforced","Chrono"};string s;for(int i=0;i<5;i++)if(f[i])s+=(s.empty()?"":", ")+string(un[i]);return s;}
int pickMain(){Par p{};p.margin=P("anMargin");p.noCool=false;p.steps=max(M.simSteps,M.expEnd);p.tOut=p.tVent=p.m2=true;
 pkCount=(int)PI("pkCount");int maxV=max(10,(int)PI("pkMaxVerify"));int K=max(1,(int)PI("pkTop"));
 bool rod[3]={PI("anA1")!=0,PI("anA2")!=0,PI("anA4")!=0};const char*fk[5]={"anFav","anFhs","anFex","anFre","anFch"};bool up[5];for(int i=0;i<5;i++)up[i]=PI(fk[i])!=0;
 vector<string>files;namespace fs=std::filesystem;error_code ec;string dir=S("pkDir"),base=S("pkBase"),out=S("pkOut");
 if(!dir.empty()&&fs::exists(dir,ec))for(auto&e:fs::recursive_directory_iterator(dir,ec))if(e.is_regular_file()&&e.path().extension()==".txt"&&e.file_size()<40000)files.push_back(e.path().string());
 if(!base.empty())files.push_back(base);
 // уровень 1 (protium): аварии Outlet/Vent + остывание + оба порядка обмена; уровень 2 (profit, full-random): штатно + остывание
 Par strict=p;strict.cool=true;Par normal=p;normal.tOut=normal.tVent=false;normal.cool=true;
 // оценка генератора для распределения времени: лучший чистый результат уровня 2 при небольшом числе проверок
 if(PI("pkScore")){int t=0;auto r=pickRun(normal,files,rod,up,maxV,1,t,true);printf("SCORE\t%.6g\t%d\n",r.empty()?-1.0:r[0].e.Ntot,(int)files.size());return 0;}
 int tried=0,tn=0;auto S1=pickRun(strict,files,rod,up,maxV,K,tried,false);auto N1=pickRun(normal,files,rod,up,maxV,K,tn,true);
 for(auto&o:S1)o.tier=1;for(auto&o:N1)o.tier=2;
 // «Улучшение»: остаётся только то, что по чистой мощности лучше исходной схемы с её лучшей покупкой улучшений
 if(PI("pkFloor")&&!base.empty()){int t=0;vector<string>bf={base};auto bs=pickRun(strict,bf,rod,up,maxV,1,t,true),bn=pickRun(normal,bf,rod,up,maxV,1,t,true);
  double bfl=max(bs.empty()?-1e300:bs[0].e.Ntot,bn.empty()?-1e300:bn[0].e.Ntot),lim=bfl+fabs(bfl)*1e-3;
  auto drop=[&](vector<PickOpt>&v){v.erase(remove_if(v.begin(),v.end(),[&](const PickOpt&o){return o.e.Ntot<=lim;}),v.end());};drop(S1);drop(N1);
  if(bfl>-1e300)fprintf(stderr,"планка исходной схемы: %.3f T/тик чистыми\n",bfl/1000);
  if(S1.empty()&&N1.empty()){fprintf(stderr,"НЕ ЛУЧШЕ: схемы лучше исходной (%.3f T/тик чистыми) не найдено, исходная не изменена\n",bfl/1000);printf("NOTBETTER\t%.6g\n",bfl);return 4;}}
 auto save=[&](const string&fn,const PickOpt&o){ofstream w(fn);w<<setprecision(12)<<o.e.Ptot<<" "<<o.e.H<<" "<<o.e.cool<<" "<<o.e.cost<<" "<<o.L.f[0]<<" "<<o.L.f[1]<<" "<<o.L.f[2]<<" "<<o.L.f[3]<<" "<<o.L.f[4]<<" "<<0<<"\n";
  for(int y=0;y<o.L.H;y++)w<<o.L.g.substr(y*o.L.W,o.L.W)<<"\n";};
 if(out.empty())out="best.txt";string stem=out.size()>4&&out.substr(out.size()-4)==".txt"?out.substr(0,out.size()-4):out;string alt=stem+"-без-запаса.txt";
 bool hasS=!S1.empty(),hasN=!N1.empty();bool altBetter=hasN&&(!hasS||N1[0].e.Ntot>S1[0].e.Ntot*1.001);
 // общий список лучших: защищённые и без запаса, по чистой мощности, без повторов сетки
 vector<PickOpt>top;set<string>used;auto add=[&](const PickOpt&o){if(used.insert(o.L.g).second)top.push_back(o);};
 for(auto&o:S1)add(o);for(auto&o:N1)add(o);sort(top.begin(),top.end(),[](const PickOpt&a,const PickOpt&b){return a.e.Ntot>b.e.Ntot;});if((int)top.size()>K)top.resize(K);
 if(hasS&&!used.count(S1[0].L.g)){}
 // главный — лучший защищённый, затем лучший без запаса (если выгоднее), затем остальные
 vector<pair<string,PickOpt>>files2;int idx=2;
 if(hasS)files2.push_back({out,S1[0]});if(altBetter)files2.push_back({alt,N1[0]});
 for(auto&o:top){bool dup=false;for(auto&f:files2)if(f.second.L.g==o.L.g)dup=true;if(dup)continue;files2.push_back({stem+"-"+to_string(idx++)+".txt",o});}
 ofstream man(stem+".top.tsv");
 for(auto&f:files2){save(f.first,f.second);man<<setprecision(10)<<f.first<<"\t"<<f.second.tier<<"\t"<<f.second.e.Ntot<<"\t"<<f.second.e.Ptot<<"\t"<<f.second.e.cost<<"\t"<<f.second.fill<<"\t"<<f.second.src<<"\n";}
 man.close();
 if(altBetter)fprintf(stderr,"БЕЗ ЗАПАСА: %.3f T/тик чистыми, цена %.3f Qa, заполнение %.0f%%, купить: %s. Штатно и при остывании проходит, отключения Outlet/Vent не выдерживает\n",N1[0].e.Ntot/1000,N1[0].e.cost,N1[0].fill*100,upNames(N1[0].L.f).empty()?"ничего":upNames(N1[0].L.f).c_str());
 if(hasS){string ups=upNames(S1[0].L.f);
  fprintf(stderr,"ЛУЧШИЙ: %.3f T/тик чистыми (производство %.3f), цена %.3f Qa, заполнение %.0f%%, купить: %s (проверено симуляцией: %d, лучших разных: %d)\n",S1[0].e.Ntot/1000,S1[0].e.Ptot/1000,S1[0].e.cost,S1[0].fill*100,ups.empty()?"ничего":ups.c_str(),tried,(int)files2.size());
  printf("BEST\t%s\t%s\n",out.c_str(),S1[0].src.c_str());return 0;}
 fprintf(stderr,"отбор: ни одна схема не прошла симуляцию с авариями (проверено %d)\n",tried);
 if(hasN)return 0;
 bool allUp[5]={1,1,1,1,1},allRod[3]={1,1,1};bool missUp=false,missRod=false;for(int i=0;i<5;i++)missUp|=!up[i];for(int i=0;i<3;i++)missRod|=!rod[i];
 int t2=0;vector<PickOpt>h;
 if(missUp&&!(h=pickRun(normal,files,rod,allUp,maxV,1,t2,true)).empty()){int need[5];for(int i=0;i<5;i++)need[i]=h[0].L.f[i]&&!up[i];
  fprintf(stderr,"ПОДСКАЗКА: если разрешить покупку %s, нашлась бы схема на %.3f T/тик чистыми\n",upNames(need).c_str(),h[0].e.Ntot/1000);}
 else if(missRod&&!(h=pickRun(normal,files,allRod,missUp?allUp:up,maxV,1,t2,true)).empty()){string r;const char*rn[3]={"Single","Double","Quad"};for(char c:h[0].L.g){int k=c=='1'?0:c=='2'?1:c=='4'?2:-1;if(k>=0&&!rod[k]&&r.find(rn[k])==string::npos)r+=(r.empty()?"":", ")+string(rn[k]);}
  fprintf(stderr,"ПОДСКАЗКА: если включить стержни %s, нашлась бы схема на %.3f T/тик чистыми\n",r.c_str(),h[0].e.Ntot/1000);}
 return 3;}

struct Member{Layout L;Ev e;double score=0;string group;int origin=0;};
struct Search{
 Par p;mt19937 rng;vector<Member>population;vector<PickOpt>winners;set<string>seenFields,seenCandidates,checked;map<string,PickOpt>thermalCache;
 array<long long,12>draws{},admitted{},success{};long long attempts=0,screened=0,simulated=0,duplicates=0,wrongFuel=0,overBudget=0;
 int capacity,verifyLimit;long long limit;double best=-1e300;chrono::steady_clock::time_point started;string strategy;
 Search(Par q):p(q),rng(q.seed),capacity(clamp((int)PI("anPopulation"),12,1024)),verifyLimit(max(1,(int)PI("anVerify"))),limit(max(0LL,PI("anAttempts"))),started(chrono::steady_clock::now()),strategy(S("anStrategy")){}
 double elapsed()const{return chrono::duration<double>(chrono::steady_clock::now()-started).count();}
 bool expired()const{return limit?attempts>=limit:elapsed()>=p.secs;}
 int number(int n){return n>0?(int)(rng()%n):0;}
 double unit(){return (rng()&0xffffff)/16777216.;}
 string key(const Layout&L,bool flags=true)const{string k=to_string(L.W)+"x"+to_string(L.H)+":"+L.g;if(flags)for(int f:L.f)k+=char('0'+f);return k;}
 double rank(const Ev&e)const{return S("anObjective")=="heat"?min(e.H,e.generated):e.Ntot;}
 string group(const Layout&L)const{array<int,5>c{};for(char v:L.g)c[core(v)]++;return to_string(c[1])+"S"+to_string(c[2])+"D"+to_string(c[4])+"Q";}
 bool allowed(const Layout&L)const{int count=0;for(char c:L.g){if((c=='1'&&!p.a1)||(c=='2'&&!p.a2)||(c=='4'&&!p.a4))return false;count+=core(c)>0;}return count>=max(1,p.minF)&&(!p.maxF||count<=p.maxF);}
 Layout blank(){Layout L;L.W=p.W;L.H=p.H;L.g.assign(p.W*p.H,'K');for(int i=0;i<5;i++)L.f[i]=p.fl[i]?number(2):0;return L;}
 int fuelMaximum()const{double price=1e300;if(p.a1)price=min(price,PR.f1);if(p.a2)price=min(price,PR.f2);if(p.a4)price=min(price,PR.f4);int budgetMax=price>0?(int)min(double(p.W*p.H),floor(PR.budget/price)):p.W*p.H;return min(p.maxF?p.maxF:p.W*p.H,budgetMax);}
 bool fixFuel(Layout&L){string fp=fuelPool(p);vector<int>fu,free;for(int i=0;i<(int)L.g.size();i++){char c=L.g[i];if(core(c)){if(fp.find(c)==string::npos)L.g[i]=fp[number((int)fp.size())];fu.push_back(i);}else free.push_back(i);}
  int lo=max(1,p.minF),hi=fuelMaximum();if(hi<lo)return false;int target=clamp((int)fu.size(),lo,hi);shuffle(fu.begin(),fu.end(),rng);while((int)fu.size()>target){L.g[fu.back()]='K';fu.pop_back();}shuffle(free.begin(),free.end(),rng);while((int)fu.size()<target&&!free.empty()){int i=free.back();free.pop_back();L.g[i]=fp[number((int)fp.size())];fu.push_back(i);}return (int)fu.size()==target;}
 void rings(Layout&L){const Adj&A=getAdj(L.W,L.H);for(int i=0;i<(int)L.g.size();i++)if(core(L.g[i]))for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];if(L.g[j]=='K')L.g[j]='R';}}
 bool module(Layout&L,int x,int y,int type,bool overwrite=false){int w=L.W,h=L.H;if(x<1||y<1||x>=w-1||y>=h-1)return false;int i=y*w+x;vector<pair<int,char>>v;
  if(type<4){v.push_back({i,type<2?'O':fuelPool(p)[number((int)fuelPool(p).size())]});char c=type==0?'V':type==1?'C':type==2?'R':'I';for(int j:{i-1,i+1,i-w,i+w})v.push_back({j,c});if(type==1)for(int dx:{-1,1})for(int dy:{-1,1})v.push_back({i+dx+dy*w,'I'});}
  else if(type==4){v={{i,'I'},{i-1,'O'},{i+1,'O'}};for(int dx:{-1,1})for(int dy:{-1,1})v.push_back({i+dx+dy*w,'V'});}
  else if(type==5){v.push_back({i,fuelPool(p)[number((int)fuelPool(p).size())]});for(int j:{i-1,i+1,i-w,i+w})v.push_back({j,'C'});for(int dx:{-1,1})for(int dy:{-1,1})v.push_back({i+dx+dy*w,'I'});}
  else{if(x<2||y<2||x>=w-2||y>=h-2)return false;v.push_back({i,'O'});for(int j:{i-1,i+1,i-w,i+w})v.push_back({j,'E'});for(int dx:{-1,1})for(int dy:{-1,1})v.push_back({i+dx+dy*w,'V'});for(int j:{i-2,i+2,i-2*w,i+2*w})v.push_back({j,number(5)?'V':'C'});}
  for(auto t:v)if(!overwrite&&L.g[t.first]!='K'&&L.g[t.first]!=t.second)return false;for(auto t:v)L.g[t.first]=t.second;return true;}
 void cooling(Layout&L){Par fast=p;fast.route=false;fast.margin=0;Ev e=evalG(fast,L);int N=(int)L.g.size();
  int target=clamp((int)ceil(e.H/max(1e-12,M.hullPer))+number(5),1,N);int countP=(int)count(L.g.begin(),L.g.end(),'P');
  for(int k=0;k<N&&countP<target;k++){int i=number(N);if(L.g[i]=='K'){L.g[i]='P';countP++;}}
  for(int k=0;k<N/2;k++){e=evalG(fast,L);if(e.cool>=e.H*(1+p.margin/100.)&&k>0)break;int x=1+number(L.W-2),y=1+number(L.H-2);module(L,x,y,number(5)?0:6);}
 }
 Layout construct(int family){Layout L=blank();const Adj&A=getAdj(p.W,p.H);int N=p.W*p.H,lo=max(1,p.minF),hi=max(lo,fuelMaximum());int nf=lo+number(hi-lo+1);string fp=fuelPool(p);
  if(family==0){string types="KVE OICPR";types.erase(remove(types.begin(),types.end(),' '),types.end());vector<double>w;double sum=0;for(char c:types){(void)c;double v=pow(unit(),number(2)?8:1)+1e-8;w.push_back(sum+=v);}for(char&c:L.g){double v=unit()*sum;int j=0;while(j+1<(int)w.size()&&v>w[j])j++;c=types[j];}}
  if(family==1){int ox=number(3),oy=number(3);for(int y=oy+1;y<p.H-1;y+=3)for(int x=ox+1;x<p.W-1;x+=3)if(number(3))module(L,x,y,number(5));}
  if(family==2){int ox=number(5),oy=number(5);for(int y=oy+2;y<p.H-2;y+=5)for(int x=ox+2;x<p.W-2;x+=5)if(number(8))module(L,x,y,6);}
  if(family==3){int bw=1+number(max(1,p.W)),bh=1+number(max(1,p.H)),ox=number(2),oy=number(4);for(int y=0;y<bh;y++)for(int x=0;x<bw;x++){int xx=x+ox,yy=y+oy;L.g[y*p.W+x]=yy%2==0?(xx%2?'V':'E'):yy%4==1?(xx%2?'P':'O'):'K';}}
  if(family==4){int tw=2+number(min(7,p.W)-1),th=2+number(min(7,p.H)-1);string tile(tw*th,'R');for(char&c:tile)c=number(3)?fp[number((int)fp.size())]:"CIR"[number(3)];int bw=1+number(p.W),bh=1+number(p.H);for(int y=0;y<bh;y++)for(int x=0;x<bw;x++)L.g[y*p.W+x]=tile[(y%th)*tw+x%tw];}
  vector<int>pos(N);iota(pos.begin(),pos.end(),0);shuffle(pos.begin(),pos.end(),rng);int existing=0;for(char c:L.g)existing+=core(c)>0;
  for(int i:pos)if(existing<nf&&!core(L.g[i])){L.g[i]=fp[number((int)fp.size())];existing++;}
  if(family==5){L=blank();int placed=0;for(int i:pos){if(placed>=nf)break;bool clear=L.g[i]=='K';for(int a=0;a<A.cnt[i];a++)clear&=L.g[A.nb[i][a]]=='K'||L.g[A.nb[i][a]]=='R';if(!clear)continue;L.g[i]=fp[number((int)fp.size())];placed++;for(int a=0;a<A.cnt[i];a++)L.g[A.nb[i][a]]='R';}
   for(int k=0;k<1+number(3);k++)for(int t=0;t<N;t++)if(module(L,1+number(p.W-2),1+number(p.H-2),number(3)?1:5))break;}
  fixFuel(L);if(family!=0)rings(L);if(family!=0)cooling(L);return L;}
 Member parent(){int i=number((int)population.size()),j=number((int)population.size());return population[unit()<.25?i:(population[i].score>population[j].score?i:j)];}
 Layout mutate(int op){if(population.empty())return construct(op%6);Layout L=parent().L;int N=(int)L.g.size();string types=pool(p);
  if(op==6){int times=1+number(8);for(int t=0;t<times;t++){int i=number(N);if(number(3))L.g[i]=types[number((int)types.size())];else swap(L.g[i],L.g[number(N)]);}}
  if(op==7){int x=1+number(p.W-2),y=1+number(p.H-2);module(L,x,y,number(7),true);}
  if(op==8){Layout other=parent().L;int bw=1+number(p.W),bh=1+number(p.H),x0=number(p.W-bw+1),y0=number(p.H-bh+1);for(int y=0;y<bh;y++)for(int x=0;x<bw;x++)L.g[(y0+y)*p.W+x0+x]=other.g[(y0+y)*p.W+x0+x];}
  if(op==9){if(number(2))stamp(L.g,p.W,p.H,rng);else{int bw=2+number(min(10,p.W)-1),bh=2+number(min(10,p.H)-1),sx=number(p.W-bw+1),sy=number(p.H-bh+1),ox=number(bw),oy=number(bh);string src=L.g;for(int y=0;y<p.H;y++)for(int x=0;x<p.W;x++)L.g[y*p.W+x]=src[(sy+(y+oy)%bh)*p.W+sx+(x+ox)%bw];}}
  if(op==10){int i=number(N);if(core(L.g[i]))L.g[i]=fuelPool(p)[number((int)fuelPool(p).size())];else fuelRing(L.g,getAdj(p.W,p.H),i,fuelPool(p),rng,p.W,p.H);}
  if(op==11){auto probe=simulate(L,0,0,64);if(probe.worstV>=0&&probe.mV>.7){for(int t=0;t<N;t++){int j=number(N);if(L.g[j]=='K'){L.g[j]='P';break;}}}const Adj&A=getAdj(p.W,p.H);for(int cell=0;cell<N;cell++)if(L.g[cell]=='C'){bool inlet=false;for(int a=0;a<A.cnt[cell];a++)inlet|=L.g[A.nb[cell][a]]=='I';if(!inlet)for(int a=0;a<A.cnt[cell];a++){int j=A.nb[cell][a];if(L.g[j]=='K'){L.g[j]='I';break;}}}int i=number(N);if(L.g[i]=='P'||L.g[i]=='V'||L.g[i]=='O'||L.g[i]=='E'||L.g[i]=='C'||L.g[i]=='I')L.g[i]='K';else cooling(L);}
  if(number(4)==0){int f=number(5);if(p.fl[f])L.f[f]^=1;}
  fixFuel(L);if(op!=6||number(2))rings(L);if(op==8||op==10||number(8)==0)cooling(L);return L;}
 int selectOperator(){int initial=(int)(attempts%12);if(attempts<120||unit()<.25)return initial;double sum=0;array<double,12>w{};for(int i=0;i<12;i++){double reward=(admitted[i]+8.*success[i])/max(1LL,draws[i]);w[i]=.1+sqrt(reward);if(strategy=="random"&&(i==0||i==6))w[i]*=2;if((strategy=="blocks"||strategy=="tile")&&(i==1||i==2||i==3||i==7||i==9))w[i]*=2;sum+=w[i];}double v=unit()*sum;for(int i=0;i<12;i++){v-=w[i];if(v<=0)return i;}return 11;}
 bool admit(Layout L,int op){if(!allowed(L)){wrongFuel++;return false;}for(int i=0;i<5;i++)if(L.f[i]&&!p.fl[i]){wrongFuel++;return false;}
  string k=key(L);seenFields.insert(key(L,false));if(!seenCandidates.insert(k).second){duplicates++;return false;}
  Par fast=p;fast.route=false;Ev e=evalG(fast,L);double scale=unitScale();double score=rank(e)-p.pen*e.deficit-1e4*scale*e.bad-max(0.,e.cost-PR.budget)*1e5*scale;
  if(e.cost>PR.budget)overBudget++;if(!e.bad&&e.cost<=PR.budget&&e.deficit==0)screened++;
  Member m{L,e,score,group(L),op};int similar=-1;for(int i=0;i<(int)population.size();i++)if(population[i].group==m.group){int d=0;for(int j=0;j<(int)L.g.size();j++)d+=L.g[j]!=population[i].L.g[j];if(d<4){similar=i;break;}}
  if(similar>=0){double progress=limit?double(attempts)/max(1LL,limit):elapsed()/p.secs;double temperature=max(1e-12,p.T0*pow(max(1e-12,p.T1)/max(1e-12,p.T0),clamp(progress,0.,1.)));if(population[similar].score>=score&&unit()>=exp((score-population[similar].score)/temperature))return false;population[similar]=move(m);}
  else{population.push_back(move(m));if((int)population.size()>capacity){map<string,int>counts;for(auto&v:population)counts[v.group]++;int worst=-1;for(int i=0;i<(int)population.size();i++)if(counts[population[i].group]>1&&(worst<0||population[i].score<population[worst].score))worst=i;if(worst<0)worst=(int)(min_element(population.begin(),population.end(),[](const Member&a,const Member&b){return a.score<b.score;})-population.begin());bool kept=worst!=(int)population.size()-1;population.erase(population.begin()+worst);if(!kept)return false;}}
  if(op>=0)admitted[op]++;return true;}
 PickOpt assess(const Member&m)const{Par q=p;q.route=false;q.steps=max(M.simSteps,M.expEnd+(int)M.fuelLife);q.m2=q.cool=true;q.tOut=q.tVent=false;
  string thermal=key(m.L,false);for(int i=0;i<4;i++)thermal+=char('0'+m.L.f[i]);PickOpt o;auto hit=thermalCache.find(thermal);
  if(hit!=thermalCache.end()){o=hit->second;o.L=m.L;o.e=m.e;o.e.generated=hit->second.e.generated;double ratio=(M.bCh+m.L.f[4])/M.bCh;o.e.P=o.e.Ptot=hit->second.e.P;o.e.Ptot*=ratio;array<int,256>n{};for(char c:m.L.g)n[(unsigned char)c]++;o.e.net=o.e.P-PR.fuel(n)*1e6/M.fuelLife;o.e.Ntot=o.e.net*ratio;}
  else{Ev e=m.e;double sold=1e300,gross=1e300,fill=0;for(int method=0;method<6;method++){auto r=simulate(m.L,method,0,q.steps);if(!r.ok)return PickOpt{};sold=min(sold,r.sold);gross=min(gross,r.gross);fill=max(fill,max(max(r.mE,r.mV),max(r.mC,r.mH)));for(int f:{3,4}){auto c=simulate(m.L,method,f,q.steps);if(!c.ok)return PickOpt{};fill=max(fill,max(max(c.mE,c.mV),max(c.mC,c.mH)));}}
   e.generated=gross;e.P=sold;e.Ptot=sold*(M.bCh+m.L.f[4])/M.bCh;array<int,256>n{};for(char c:m.L.g)n[(unsigned char)c]++;e.net=sold-PR.fuel(n)*1e6/M.fuelLife;e.Ntot=e.net*(M.bCh+m.L.f[4])/M.bCh;o={m.L,e,"",fill,2};
   bool strict=true;double strictFill=fill;for(int method=0;method<6&&strict;method++)for(int f:{1,2}){auto r=simulate(m.L,method,f,q.steps);if(!r.ok){strict=false;break;}strictFill=max(strictFill,max(max(r.mE,r.mV),max(r.mC,r.mH)));}o.tier=strict?1:2;if(strict)o.fill=strictFill;}
  return o;
 }
 void commit(const Member&m,PickOpt o){if(!o.tier)return;string k=key(m.L);string thermal=key(m.L,false);for(int i=0;i<4;i++)thermal+=char('0'+m.L.f[i]);thermalCache[thermal]=o;
  o.src="candidate-"+to_string(winners.size()+1)+".txt";saveLayout(o.src,o.L,o.e,p.seed);winners.push_back(o);if(m.origin>=0)success[m.origin]++;
  for(auto&v:population)if(key(v.L)==k){v.score=rank(o.e);v.e=o.e;}
  if(rank(o.e)>best){best=rank(o.e);saveLayout("best-current.txt",o.L,o.e,p.seed);fprintf(stderr,"новый лучший: %.6f T/тик чистыми, стержней %d, цена %.6f Qa, проверок %lld\n",o.e.Ntot/1000,o.e.fuel,o.e.cost,simulated);}

 }
 void validate(const Member&m,bool seed=false){string k=key(m.L);if(checked.count(k)||(!seed&&simulated>=verifyLimit))return;if(m.e.cost>PR.budget||m.e.bad||m.e.deficit>0)return;checked.insert(k);simulated++;commit(m,assess(m));}
 void checkPool(int count){vector<Member>c=population,batch;stable_sort(c.begin(),c.end(),[](const Member&a,const Member&b){return a.score>b.score;});
  for(auto&m:c){if((int)batch.size()>=count||simulated>=verifyLimit||(!limit&&elapsed()>=p.secs))break;if(m.e.bad||m.e.cost>PR.budget||m.e.deficit>0||checked.count(key(m.L)))continue;checked.insert(key(m.L));simulated++;batch.push_back(m);}
  vector<PickOpt>results(batch.size());int threads=min(p.threads,(int)batch.size());vector<thread>workers;for(int t=0;t<threads;t++)workers.emplace_back([&,t]{for(int i=t;i<(int)batch.size();i+=threads)results[i]=assess(batch[i]);});for(auto&t:workers)t.join();for(int i=0;i<(int)batch.size();i++)commit(batch[i],results[i]);
 }
 void seeds(){namespace fs=std::filesystem;vector<string>files;error_code ec;string dir=S("anLibrary");if(!dir.empty()&&fs::exists(dir,ec))for(auto&f:fs::recursive_directory_iterator(dir,ec))if(f.is_regular_file()&&f.path().extension()==".txt"&&f.file_size()<40000)files.push_back(f.path().string());sort(files.begin(),files.end());string start=S("anStart");if(!start.empty())files.insert(files.begin(),start);
  ofstream inputs("seeds.tsv");filesystem::create_directories("seed");vector<Member>original;set<string>unique;for(auto&f:files){Layout L;if(!parseLayout(f,L)||L.W!=p.W||L.H!=p.H)continue;bool adapted=!allowed(L);if(adapted&&!fixFuel(L))continue;if(!allowed(L)||!unique.insert(key(L,false)).second)continue;string snapshot="seed/input-"+to_string(unique.size())+".txt";ifstream input(f,ios::binary);ofstream saved(snapshot,ios::binary);saved<<input.rdbuf();saved.close();inputs<<snapshot<<'\t'<<f<<'\n';for(int mask=0;mask<32;mask++){Layout X=L;bool ok=true;for(int i=0;i<5;i++){X.f[i]=mask>>i&1;if(X.f[i]&&!p.fl[i])ok=false;}if(!ok)continue;Par fast=p;fast.route=false;Ev e=evalG(fast,X);admit(X,-1);if(e.cost<=PR.budget&&!e.bad&&e.deficit==0)original.push_back({X,e,rank(e),group(X),f==start&&!adapted?(equal(begin(X.f),end(X.f),begin(L.f))?-3:-2):-1});}}
  stable_sort(original.begin(),original.end(),[](const Member&a,const Member&b){return a.origin!=b.origin?a.origin<b.origin:a.score!=b.score?a.score>b.score:a.e.Ntot!=b.e.Ntot?a.e.Ntot>b.e.Ntot:a.e.cost<b.e.cost;});
  for(auto&m:original){if(simulated>=max(1,verifyLimit/3)||(!limit&&elapsed()>=p.secs*.4))break;validate(m,true);}fprintf(stderr,"стартовый пул: файлов %d, разных топологий %d, проверенных вариантов %lld\n",(int)files.size(),(int)unique.size(),simulated);
 }
 int finish(){ofstream st("stats.txt");st<<setprecision(14)<<"seed="<<p.seed<<"\nstrategy="<<strategy<<"\nattempts="<<attempts<<"\nunique_fields="<<seenFields.size()<<"\nunique_candidates="<<seenCandidates.size()<<"\nduplicates="<<duplicates<<"\nwrong_fuel="<<wrongFuel<<"\nover_budget="<<overBudget<<"\nscreened="<<screened<<"\nvalidated="<<simulated<<"\npassed="<<winners.size()<<"\nbudget_Qa="<<PR.budget<<"\nW="<<p.W<<"\nH="<<p.H<<"\nmin_fuel="<<p.minF<<"\nmax_fuel="<<p.maxF<<"\nsteps="<<max(M.simSteps,M.expEnd+(int)M.fuelLife)<<"\nbest_score="<<best<<"\nbest_net_B="<<(winners.empty()?-1e300:max_element(winners.begin(),winners.end(),[](const PickOpt&a,const PickOpt&b){return a.e.Ntot<b.e.Ntot;})->e.Ntot)<<"\n";
  ofstream ops("operators.tsv");ops<<"operator\tattempts\tadmitted\tpassed\n";const char*names[]={"random","modules","exchanger","checker","mosaic","recirculation","mutation","module-mutation","crossover","tile","fuel","repair"};for(int i=0;i<12;i++)ops<<names[i]<<'\t'<<draws[i]<<'\t'<<admitted[i]<<'\t'<<success[i]<<'\n';
  stable_sort(winners.begin(),winners.end(),[&](const PickOpt&a,const PickOpt&b){return rank(a.e)!=rank(b.e)?rank(a.e)>rank(b.e):a.e.Ntot!=b.e.Ntot?a.e.Ntot>b.e.Ntot:a.e.cost<b.e.cost;});string out=S("anOutFile");if(out.empty())out="best.txt";string stem=out.size()>4?out.substr(0,out.size()-4):out;ofstream man(stem+".top.tsv");int index=2,count=0;bool strictSaved=false,normalSaved=false;
  vector<PickOpt>selected;for(auto&o:winners)if(o.tier==1){selected.push_back(o);break;}for(auto&o:winners){if(selected.size()>=5)break;bool duplicate=false;for(auto&v:selected){if(key(v.L)==key(o.L)){duplicate=true;break;}if(v.L.g==o.L.g&&v.tier==o.tier){duplicate=true;break;}}if(!duplicate)selected.push_back(o);}stable_sort(selected.begin(),selected.end(),[&](const PickOpt&a,const PickOpt&b){return rank(a.e)!=rank(b.e)?rank(a.e)>rank(b.e):a.e.Ntot>b.e.Ntot;});
  for(auto&o:selected){string file;if(o.tier==1&&!strictSaved){file=out;strictSaved=true;}else if(o.tier==2&&!normalSaved){file=stem+"-без-запаса.txt";normalSaved=true;}else file=stem+"-"+to_string(index++)+".txt";saveLayout(file,o.L,o.e,p.seed);man<<setprecision(12)<<file<<'\t'<<o.tier<<'\t'<<o.e.Ntot<<'\t'<<o.e.Ptot<<'\t'<<o.e.cost<<'\t'<<o.fill<<'\t'<<o.src<<'\n';count++;}
  fprintf(stderr,"отбор: попыток %lld, уникальных полей %d, полных проверок %lld, прошедших вариантов %d, лучших разных %d\n",attempts,(int)seenFields.size(),simulated,(int)winners.size(),count);if(winners.empty())fprintf(stderr,"рабочей схемы не найдено в этом запуске\n");else fprintf(stderr,"ЛУЧШИЙ: %.6f T/тик чистыми, цена %.6f Qa, %s\n",winners.front().e.Ntot/1000,winners.front().e.cost,winners.front().tier==1?"защищённая":"без аварийного запаса");return 0;
 }
 int run(){if(!p.a1&&!p.a2&&!p.a4){cerr<<"не разрешено ни одного типа топлива\n";return 2;}if(p.W<3||p.H<3||p.W>60||p.H>60||p.secs<=0||M.bCh<=0||M.fuelLife<=0||PR.budget<=0){cerr<<"некорректные параметры поиска\n";return 2;}
  if(fuelMaximum()<max(1,p.minF)){cerr<<"OVER_BUDGET: заданное топливо не помещается в бюджет; тип и количество не изменены\n";return 3;}
  ofstream params("search-parameters.txt");for(auto&v:RX_PARAMS)params<<v.key<<'='<<(string(v.key)=="anThreads"?to_string(p.threads):S(v.key))<<'\n';params.close();fprintf(stderr,"единый поиск: seed %u, сетка %dx%d, бюджет %.9f Qa, попыток %lld (0 = по времени), приоритет %s\n",p.seed,p.W,p.H,PR.budget,limit,strategy.c_str());seeds();
  while(!expired()){int op=selectOperator();draws[op]++;Layout L=op<6?construct(op):mutate(op);attempts++;admit(move(L),op);if(attempts%128==0)checkPool(4);if(attempts%2048==0)fprintf(stderr,"... попыток %lld, разных полей %d, популяция %d, лучший %.6f T/тик\n",attempts,(int)seenFields.size(),(int)population.size(),best>-1e290?best/1000:0);}
  if(limit)checkPool(verifyLimit);return finish();}
};
int main(){Par p=readPar();if(p.mode==1&&!S("anStart").empty()){Layout L;if(!parseLayout(S("anStart"),L)){cerr<<"не прочитан файл старта\n";return 2;}p.W=L.W;p.H=L.H;}return Search(p).run();}

int validateMain(){Layout L;if(!parseLayout(S("plFile"),L)){cerr<<"не прочитана схема\n";return 2;}int steps=max((int)PI("plSteps"),M.expEnd+(int)M.fuelLife);for(int m=0;m<6;m++)for(int f=0;f<5;f++){auto r=simulate(L,m,f,steps);printf("обмен %d %s: Exch=%.9g Vent=%.9g Coolant=%.9g hull=%.9g worstVent=%d %s %s\n",m,f==0?"штатно":f==1?"Outlet off":f==2?"Vent off":f==3?"замена топлива":"выгорание",r.mE,r.mV,r.mC,r.mH,r.worstV,r.why.c_str(),r.ok?"PASS":"FAIL");}return 0;}
