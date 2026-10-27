// Отжиг по сетке любого размера (наработка Reactor Lab), модель = reactor-protium/search.cpp.
#include "rx.h"
using namespace std;
struct Layout{int W=19,H=16;string g;int f[5]={1,1,1,1,0};};
// Общая цель всех способов (критерий reactor-profit): чистая мощность = min(производство, автопродажа) − цена заправки / срок топлива, × темп Chrono.
struct Ev{double cost=0,P=0,Ptot=0,net=0,Ntot=0,H=0,cool=0,deficit=0;int bad=0,fuel=0;bool feed=false;};
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
static Ev evalG(const Par&p,const Layout&L){const Adj&A=getAdj(L.W,L.H);int N=L.W*L.H;const string&g=L.g;array<int,256>n{};for(char c:g)n[(unsigned char)c]++;Ev e;
 e.cost=PR.fuel(n)+PR.upgrades(L.f[0],L.f[1],L.f[2],L.f[3],L.f[4])+PR.parts(n);double H=0,Pw=0;
 for(int i=0;i<N;i++){int co=core(g[i]);if(!co)continue;e.fuel++;int pulse=co==1?1:co==2?4:12,r=0,cc=0;
  for(int k=0;k<A.cnt[i];k++){int j=A.nb[i][k];pulse+=core(g[j]);r+=g[j]=='R';e.bad+=g[j]=='K';cc+=g[j]=='C';}
  H+=pulse*pulse*M.fuelHeat/co;Pw+=pulse*M.fuelPow*(1+M.refl*r);if(co==1&&cc==4)e.feed=true;}
 if(e.feed){double held=M.fuelHeat-n['P']*M.hullPer*M.drain;if(held>0)Pw*=M.boost(held);}
 Pw=min(Pw,M.sales(n['K']));e.P=Pw;e.Ptot=Pw*(M.bCh+L.f[4])/M.bCh;e.net=Pw-PR.fuel(n)*1e6/M.fuelLife;e.Ntot=e.net*(M.bCh+L.f[4])/M.bCh;e.H=H;e.cool=n['V']*M.vr0*(1+M.ventBoost*(M.bAv+L.f[0])*n['K']);
 e.deficit=p.noCool?0:max(0.,H*(1+p.margin/100.)-e.cool);if(p.route&&!p.noCool)e.deficit+=localOver(L,n);return e;}
struct SimRes{bool ok=false;double mE=0,mV=0,mC=0,mH=0;string why;};
static SimRes simulate(const Layout&L,int method,int fault,int steps){const Adj&A=getAdj(L.W,L.H);int N=L.W*L.H;const string&g=L.g;SimRes R;
 vector<double>h(N,0),cap(N,0);array<int,256>n{};for(char c:g)n[(unsigned char)c]++;double pp=n['P'],k=n['K'];
 double rate=M.rate0*(M.bEx+L.f[2])*2*(1+M.capBoost*k),vr=M.vr0*(1+M.ventBoost*(M.bAv+L.f[0])*k);
 double vc=M.vc0*(1+M.sinkBoost*(M.bHs+L.f[1])*pp),ec=M.ec0*(M.bEx+L.f[2])*(1+M.reBoost*(M.bRe+L.f[3])*pp),hc=M.hullPer*max(pp,1.);
 for(int i=0;i<N;i++)cap[i]=g[i]=='E'?ec:g[i]=='V'?vc:g[i]=='C'?M.ccap:g[i]=='K'?M.kcap:0;
 double hull=0;int branches=0;for(int i=0;i<N;i++)if(g[i]=='O')for(int a=0;a<A.cnt[i];a++)if(cap[A.nb[i][a]]>0)branches++;
 int ft=max(1,min(M.faultT,steps-1));
 for(int t=0;t<steps;t++){
  if(!(fault==3&&t>=M.expStart))for(int i=0;i<N;i++)if(int co=core(g[i])){int pulse=co==1?1:co==2?4:12;for(int a=0;a<A.cnt[i];a++)pulse+=core(g[A.nb[i][a]]);
   double q=pulse*pulse*M.fuelHeat/co;int sinks=0;for(int a=0;a<A.cnt[i];a++)sinks+=cap[A.nb[i][a]]>0;
   if(sinks){for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];if(cap[j]>0)h[j]+=q/sinks;}}else hull+=q;}
  for(int pass=0;pass<(method?2:1);pass++)for(int i=0;i<N;i++)if(g[i]=='E'){int c=A.cnt[i];
   for(int jj=0;jj<c;jj++){int j=A.nb[i][method?c-1-jj:jj];if(cap[j]<=0)continue;double delta=(h[i]+h[j])/(cap[i]+cap[j])*cap[j]-h[j];double q=clamp(delta,-rate,rate);q=clamp(q,-h[j],h[i]);h[j]+=q;h[i]-=q;}}
  double flow=branches?min(hull,branches*rate):0;if(fault==1&&t>=ft&&t<ft+M.outFault)flow=0;hull-=flow;
  if(branches)for(int i=0;i<N;i++)if(g[i]=='O')for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];if(cap[j]>0)h[j]+=flow/branches;}
  for(int i=0;i<N;i++)if(g[i]=='V')h[i]=max(0.,h[i]-((fault==2&&t>=ft&&t<ft+M.ventFault)?0:vr));
  for(int i=0;i<N;i++)if(g[i]=='I')for(int a=0;a<A.cnt[i];a++){int j=A.nb[i][a];double q=min(rate,h[j]);h[j]-=q;hull+=q;}
  if(hull>=hc){R.why="корпус t="+to_string(t);return R;}
  for(int i=0;i<N;i++)if(cap[i]>0&&h[i]>=cap[i]){R.why=string(1,g[i])+" t="+to_string(t);return R;}
  if(t>=ft-1){for(int i=0;i<N;i++){if(g[i]=='E')R.mE=max(R.mE,h[i]/cap[i]);if(g[i]=='V')R.mV=max(R.mV,h[i]/cap[i]);if(g[i]=='C')R.mC=max(R.mC,h[i]/cap[i]);}R.mH=max(R.mH,hull/hc);}
  hull=max(0.,hull-hc*M.drain);}
 R.ok=true;return R;}
// Общая проверка (стандарт reactor-protium + остывание из reactor-full-random/modules): 2 порядка обмена × штатно, Outlet off, Vent off, выгорание топлива.
static bool verify(const Par&p,const Layout&L,string*rep=nullptr,double*fill=nullptr){if(fill)*fill=0;vector<int>faults={0};if(p.tOut)faults.push_back(1);if(p.tVent)faults.push_back(2);if(p.cool)faults.push_back(3);bool all=true;char buf[256];
 for(int m=0;m<(p.m2?2:1);m++)for(int f:faults){SimRes r=simulate(L,m,f,p.steps);if(fill)*fill=max(*fill,max(max(r.mE,r.mV),max(r.mC,r.mH)));
  if(rep){snprintf(buf,sizeof buf,"  обмен %d, %s: %s  Exch %.1f%% Vent %.1f%% Cool %.1f%% Корпус %.1f%% %s\n",m+1,f==0?"штатно":f==1?"Outlet off":f==2?"Vent off":"выгорание",r.ok?"OK":"СБОЙ",r.mE*100,r.mV*100,r.mC*100,r.mH*100,r.ok?"":r.why.c_str());*rep+=buf;}
  if(!r.ok){all=false;if(!rep)return false;}}return all;}
static bool parseLayout(const string&path,Layout&L){ifstream f(path);if(!f)return false;string line,head;vector<string>rows;bool first=true;
 while(getline(f,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();bool grid=line.size()>=3&&line.find_first_not_of("124EVOICPKR.")==string::npos;if(grid)rows.push_back(line);else if(first)head=line;first=false;}
 if(rows.size()<3)return false;for(auto&s:rows)if(s.size()!=rows[0].size())return false;L.W=(int)rows[0].size();L.H=(int)rows.size();L.g.clear();for(auto&s:rows)L.g+=s;
 istringstream hs(head);vector<double>v;double x;while(hs>>x)v.push_back(x);if(v.size()>=9)for(int i=0;i<5;i++)L.f[i]=(int)v[4+i];return true;}
static void saveLayout(const string&path,const Layout&L,const Ev&e,unsigned seed){ofstream o(path);o<<setprecision(12)<<e.P<<" "<<e.H<<" "<<e.cool<<" "<<e.cost<<" "<<L.f[0]<<" "<<L.f[1]<<" "<<L.f[2]<<" "<<L.f[3]<<" "<<L.f[4]<<" "<<seed<<"\n";
 for(int y=0;y<L.H;y++)o<<L.g.substr(y*L.W,L.W)<<"\n";}
struct Shared{mutex m;Layout best;Ev bestE;double nearS=-1e300;Ev nearE;long long nearSimFail=0;atomic<double>bestP{0};atomic<long long>iters{0},sims{0},passed{0};};
static string pool(const Par&p){string s="EEEEVVVVVVOIICCPPPPPKKKKKRRRR";if(p.a1)s+="1111";if(p.a2)s+="2";if(p.a4)s+="4";return s;}
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
static void searchThread(const Par&p,Shared&S,int tid,Layout start,chrono::steady_clock::time_point t0){
 mt19937 rng(p.seed*1000003u+tid*7919u+1);string pl=pool(p),fpool=fuelPool(p);int N=p.W*p.H;auto U=[&](){return (rng()&0xFFFFFF)/16777216.;};
 auto score=[&](const Ev&e){return scoreOf(p,e);};
 auto randomLayout=[&](){Layout L;L.W=p.W;L.H=p.H;L.g.assign(N,'K');string np;for(char c:pl)if(!core(c))np+=c;string fp;if(p.a1)fp+='1';if(p.a2)fp+='2';if(p.a4)fp+='4';
  for(int i=0;i<N;i++)L.g[i]=(!fp.empty()&&U()<.03)?fp[rng()%fp.size()]:np[rng()%np.size()];for(int i=0;i<5;i++)L.f[i]=p.fl[i]?(int)(rng()%2):0;return L;};
 Layout cur=start;if(p.mode==2)cur=randomLayout();Ev ce=evalG(p,cur);double cs=score(ce);long long phaseIt=0,loc=0,simFail=0;double lastTry=0,T=p.T0,nearS=-1e300;Ev nearE;
 for(;;){
  if(cs>nearS){nearS=cs;nearE=ce;}
  if((loc&1023)==0){double el=chrono::duration<double>(chrono::steady_clock::now()-t0).count();if(el>p.secs)break;S.iters+=1024;
   double prog=p.mode==2?double(phaseIt)/p.ips:el/p.secs;T=p.T0*pow(p.T1/p.T0,min(1.,max(0.,prog)));}
  loc++;
  if(p.mode==2&&phaseIt>=p.ips){cur=randomLayout();ce=evalG(p,cur);cs=score(ce);phaseIt=0;lastTry=0;continue;}
  phaseIt++;Layout ng=cur;int m=rng()%100;
  if(m<3){int w=rng()%5;if(p.fl[w])ng.f[w]^=1;}
  else if(m<22){int i=rng()%N,j=rng()%N;swap(ng.g[i],ng.g[j]);}
  else if(m<27){int i=rng()%N;char c=ng.g[i];const Adj&A=getAdj(p.W,p.H);for(int k=0;k<A.cnt[i];k++)if(rng()%2)ng.g[A.nb[i][k]]=c;}
  else if(m<32)stamp(ng.g,p.W,p.H,rng);
  else if(m<36)fuelRing(ng.g,getAdj(p.W,p.H),rng()%N,fpool,rng,p.W,p.H);
  else{int i=rng()%N;ng.g[i]=pl[rng()%pl.size()];}
  Ev ne=evalG(p,ng);double ns=score(ne);
  if(ns>=cs||U()<exp((ns-cs)/T)){cur=std::move(ng);ce=ne;cs=ns;
   if(fuelOK(p,ce)&&ce.deficit==0&&!ce.bad&&ce.cost<=PR.budget&&ce.Ntot>S.bestP*1.002&&ce.Ntot>lastTry*1.002){
    lastTry=ce.Ntot;bool ok=true;if(!p.noSim){S.sims++;ok=verify(p,cur);if(!ok)simFail++;}
    if(ok){S.passed++;lock_guard<mutex>l(S.m);if(ce.Ntot>S.bestP){S.bestP=ce.Ntot;S.best=cur;S.bestE=ce;saveLayout("best-current.txt",cur,ce,p.seed);
      fprintf(stderr,"новый лучший: %.3f T/тик чистыми, стержней %d, цена %.3f Qa (поток %d, %.0f с)\n",ce.Ntot/1000,ce.fuel,ce.cost,tid,chrono::duration<double>(chrono::steady_clock::now()-t0).count());}}}}}
 {lock_guard<mutex>l(S.m);S.nearSimFail+=simFail;if(nearS>S.nearS){S.nearS=nearS;S.nearE=nearE;}}}

static Par readPar(){Par p{};p.mode=(int)PI("anMode");p.secs=P("anSecs");p.seed=(unsigned)PI("anSeed");p.threads=max(1,(int)PI("anThreads"));p.ips=max(1LL,PI("anIps"));p.T0=P("anT0")*unitScale();p.T1=P("anT1")*unitScale();p.pen=P("anPen");p.margin=P("anMargin");
 const char*fk[5]={"anFav","anFhs","anFex","anFre","anFch"};for(int i=0;i<5;i++)p.fl[i]=PI(fk[i])!=0;p.a1=PI("anA1")!=0;p.a2=PI("anA2")!=0;p.a4=PI("anA4")!=0;
 p.minF=(int)PI("anMinF");p.maxF=(int)PI("anMaxF");p.W=(int)PI("anW");p.H=(int)PI("anH");p.steps=M.simSteps;p.tOut=PI("anOut")!=0;p.tVent=PI("anVent")!=0;p.m2=PI("anM2")!=0;p.noSim=PI("anNoSim")!=0;p.noCool=PI("anNoCool")!=0;p.route=true;return p;}
int main(){Par p=readPar();
 if(!p.a1&&!p.a2&&!p.a4){cerr<<"не разрешено ни одного типа стержней\n";return 2;}
 Layout st;st.W=p.W;st.H=p.H;for(int i=0;i<5;i++)st.f[i]=p.fl[i];
 if(p.mode==1){if(!parseLayout(S("anStart"),st)){cerr<<"не прочитан файл старта: "<<S("anStart")<<"\n";return 2;}p.W=st.W;p.H=st.H;for(int i=0;i<5;i++)st.f[i]=st.f[i]&&p.fl[i];
  Ev e=evalG(p,st);string rep;bool ok=verify(p,st,&rep);fprintf(stderr,"старт %dx%d: %.3f T/тик, тепло %.3f, охлажд. %.3f, цена %.3f Qa\n%s",st.W,st.H,e.Ptot/1000,e.H/1000,e.cool/1000,e.cost,rep.c_str());(void)ok;}
 else st.g=string(p.W*p.H,'K');
 if(p.W<3||p.H<3||p.W>60||p.H>60){cerr<<"сетка вне 3..60\n";return 2;}
 fprintf(stderr,"отжиг %dx%d, потоков %d, %.0f с, бюджет %.4f Qa\n",p.W,p.H,p.threads,p.secs,PR.budget);
 Shared S;S.best=st;
 // доводка от файла: рабочий старт — нижняя планка, сохраняется только то, что лучше него
 if(p.mode==1){Ev e=evalG(p,st);if(fuelOK(p,e)&&e.deficit==0&&!e.bad&&e.cost<=PR.budget&&(p.noSim||verify(p,st))){S.bestP=e.Ntot;S.bestE=e;fprintf(stderr,"планка старта: %.3f T/тик чистыми\n",e.Ntot/1000);}}auto t0=chrono::steady_clock::now();vector<thread>th;for(int i=0;i<p.threads;i++)th.emplace_back(searchThread,cref(p),ref(S),i,st,t0);
 atomic<bool>done{false};thread rep([&]{int k=0;while(!done){this_thread::sleep_for(chrono::milliseconds(200));if(++k%25==0)fprintf(stderr,"... %.0f с, итераций %.1f млн, симуляций %lld, лучший %.3f T/тик\n",chrono::duration<double>(chrono::steady_clock::now()-t0).count(),S.iters/1e6,(long long)S.sims,S.bestP/1000);}});
 for(auto&t:th)t.join();done=true;rep.join();
 if(S.bestP>0){time_t tt=time(0);char ts[64];strftime(ts,sizeof ts,"%Y%m%d-%H%M%S",localtime(&tt));string fn=string("best-")+ts+".txt";saveLayout(fn,S.best,S.bestE,p.seed);
  string r;Par q=p;q.tOut=q.tVent=q.m2=q.cool=true;verify(q,S.best,&r);fprintf(stderr,"полная проверка лучшего:\n%s",r.c_str());
  fprintf(stderr,"готово: %.3f T/тик чистыми, стержней %d, цена %.3f Qa, Forceful %s -> %s\n",S.bestP/1000,S.bestE.fuel,S.bestE.cost,S.bestE.feed?"да":"нет",fn.c_str());}
 else{fprintf(stderr,"готово: подходящих схем не найдено (итераций %.1f млн)\n",S.iters/1e6);const Ev&e=S.nearE;string why;
  if(e.cost>PR.budget)why="дороже бюджета";else if(e.bad)why="стержень рядом с Capacitor";else if(e.deficit>0)why="не хватает охлаждения";
  else if(e.fuel==0)why="с любым разрешённым стержнем тепла больше, чем снимают Vent; разрешите стержни слабее или больше улучшений охлаждения";else if(e.fuel<max(1,p.minF))why="мало стержней";else if(S.nearSimFail)why="не прошёл симуляцию (перегрев при аварии или обмене)";else why="не найден за отведённое время";
  fprintf(stderr,"ближайший кандидат: тепло %.3f T/тик, охлаждение %.3f T/тик, цена %.3f Qa, стержней %d. Причина: %s\n",e.H/1000,e.cool/1000,e.cost,e.fuel,why.c_str());}
 return 0;}

// Блочная организация: схема = один блок w×h, повторённый по всей сетке со сдвигом (ox, oy). Блоки-зёрна вырезаются вокруг стержней
// лучших кандидатов из tlDir (тот же узор, только размноженный и упорядоченный) и добавляются случайные; отжиг идёт по клеткам блока.
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
 auto out=[&](const char*tag,const Layout&X){Ev e=evalG(p,X);printf("%s power=%.6g ptot=%.6g net=%.6g ntot=%.6g heat=%.6g cool=%.6g cost=%.6g fuel=%d bad=%d feed=%d deficit=%.6g budget=%.6g flags=%d%d%d%d%d\n",tag,e.P,e.Ptot,e.net,e.Ntot,e.H,e.cool,e.cost,e.fuel,e.bad,(int)e.feed,e.deficit,PR.budget,X.f[0],X.f[1],X.f[2],X.f[3],X.f[4]);};
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
   Layout X=L;for(int i=0;i<5;i++)X.f[i]=mask>>i&1;Ev e=evalG(p,X);if(e.cost>PR.budget||e.deficit>0||e.bad||e.fuel<1)continue;G.opts.push_back({X,e,f});}
  if(G.opts.empty()){rejFit++;continue;}
  sort(G.opts.begin(),G.opts.end(),[](const PickOpt&a,const PickOpt&b){return a.e.Ntot!=b.e.Ntot?a.e.Ntot>b.e.Ntot:a.e.cost<b.e.cost;});
  G.top=G.opts[0].e.Ntot;variants+=(int)G.opts.size();grids.push_back(move(G));}
 sort(grids.begin(),grids.end(),[](const Grid&a,const Grid&b){return a.top>b.top;});
 if(!quiet)fprintf(stderr,"отбор: схем %d, отклонено из-за стержней %d, из-за количества %d, не влезли в бюджет или охлаждение %d, подходящих схем %d, вариантов покупки %d\n",parsed,rejRod,rejCount,rejFit,(int)grids.size(),variants);
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
int pickMain(){Par p{};p.margin=P("anMargin");p.noCool=false;p.steps=M.simSteps;p.tOut=p.tVent=p.m2=true;
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
