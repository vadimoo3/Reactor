#define main unused_old_main
#include "../reactor-protium/search.cpp"
#undef main
struct Result {Cand c;double net,gross,sales;int wiring,lines;string group;};
map<string,vector<Result>> leaders; map<string,long long> attempts,feasible;vector<int> nb[304];
int main(int argc,char**argv){long long loops=PI("pfLoops");int minF=max(1,(int)PI("pfMinF")),maxF=max(minF,(int)PI("pfMaxF")),maxPlat=max(1,(int)PI("pfMaxPlat")),keep=max(1,(int)PI("pfKeep")),chrono=max(1,(int)PI("pfChrono")); rng.seed((unsigned)PI("pfSeed"));for(int i=0;i<N;i++){if(i%19)a[i].push_back(i-1);if(i%19<18)a[i].push_back(i+1);if(i>=19)a[i].push_back(i-19);if(i+19<N)a[i].push_back(i+19);}
for(long long z=0;z<loops;z++){if(z&&z%200000==0)cerr<<"попыток "<<z<<" групп "<<leaders.size()<<"\n";
 string g(N,'K');int nf=minF+rng()%(maxF-minF+1),kind=rng()%6,feed=rng()%6,family=rng()%3;string group=to_string(nf)+"_type"+to_string(kind)+"_cool"+to_string(family)+"_feed"+to_string(feed); attempts[group]++;
 bool fail=false;for(int f=0;f<nf;f++){
 bool placed=false;for(int t=0;t<70;t++) {int x=1+rng()%17,y=1+rng()%14,i=y*19+x;char fuel=(kind==1&&f==0)?'2':(kind==2&&f<2)?'2':(kind==3&&f==0)?'4':'1';
 if(kind==5&&f>=2)break;
 if(feed==3&&f==0){if(y<2)continue;vector<int> used={i,i-1,i+1,i+19,i-19,i-38,i-20,i-18};bool ok=true;for(int j:used)ok&=g[j]=='K';if(!ok)continue;g[i]=fuel;g[i-1]=g[i+1]=g[i+19]='R';g[i-19]='C';g[i-38]=g[i-20]=g[i-18]='I';}
 else if(feed>0&&feed<3&&f==0){bool ok=true;for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)ok&=g[i+dy*19+dx]=='K';if(!ok)continue;g[i]=fuel;for(int j:a[i])g[j]='C';if(feed==1)g[i-1]=g[i+1]='R';for(int dy:{-1,1})for(int dx:{-1,1})g[i+dy*19+dx]='I';}
 else if(kind==4||kind==5){int len=kind==4?3:2;if(f%len) {placed=true;break;} if(x+len>=18)continue;bool ok=true;vector<int> fs;for(int d=0;d<len;d++){fs.push_back(i+d);ok&=g[i+d]=='K';for(int j:a[i+d])ok&=g[j]=='K'||g[j]=='R';}if(!ok)continue;for(int j:fs)g[j]='1';for(int j:fs)for(int k:a[j])if(g[k]=='K')g[k]='R';}
 else{bool ok=g[i]=='K';for(int j:a[i])ok&=g[j]=='K'||g[j]=='R';if(!ok)continue;g[i]=fuel;for(int j:a[i])g[j]='R';}placed=true;break;}
 if(kind==5&&f>=2)break;if(!placed){fail=true;break;}}
 if(fail)continue;
 int recirc=0;if(feed>=4){int wanted=feed-3;for(int q=0;q<wanted;q++){bool ok=false;for(int t=0;t<100;t++){int x=1+rng()%17,y=1+rng()%14,i=y*19+x;bool clear=true;for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)clear&=g[i+dy*19+dx]=='K';if(!clear)continue;g[i]='O';for(int j:a[i])g[j]='C';for(int dy:{-1,1})for(int dx:{-1,1})g[i+dy*19+dx]='I';recirc++;ok=true;break;}if(!ok){fail=true;break;}}}if(fail)continue;
 // Direct cross lattice, or buffered 5x5 blocks, optionally mixed.
 if(family){for(int yy=2;yy<14;yy+=5)for(int xx=2;xx<17;xx+=5){if(rng()%4==0)continue;int i=yy*19+xx;bool ok=g[i]=='K';for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++)if(abs(dx)+abs(dy)<=2)ok&=g[i+dy*19+dx]=='K';if(!ok)continue;g[i]='O';for(int j:a[i])g[j]='E';for(int dy:{-1,1})for(int dx:{-1,1})g[i+dy*19+dx]='V';for(auto [dx,dy]:vector<pair<int,int>>{{2,0},{-2,0},{0,2},{0,-2}})g[i+dy*19+dx]=(family==2&&rng()%2)?'C':'V';}}
 int phase=rng()%5;vector<int>centers;for(int i=0;i<N;i++)if(a[i].size()==4&&(i%19+2*(i/19))%5==phase){bool ok=g[i]=='K';for(int j:a[i])ok&=g[j]=='K';if(ok)centers.push_back(i);}shuffle(centers.begin(),centers.end(),rng);int want=rng()%(centers.size()+1);for(int q=0;q<want;q++){int i=centers[q];bool ok=g[i]=='K';for(int j:a[i])ok&=g[j]=='K';if(!ok)continue;g[i]='O';for(int j:a[i])g[j]='V';}
 int pc=1+rng()%maxPlat;for(int i=N-1;i>=0&&pc;i--)if(g[i]=='K'){g[i]='P';pc--;}
 bool bad=false;for(int i=0;i<N;i++)if(g[i]=='I')for(int j:a[i])bad|=g[j]=='V';if(bad)continue; array<int,128>n{};for(char v:g)n[v]++;int av=rng()%2,hs=rng()%2,ex=rng()%2,re=0,ch=(rng()%chrono==0),wi=rng()%2,li=rng()%2;double fuelcost=PR.fuel(n);
 double cost=fuelcost+PR.upgrades(av,hs,ex,re,ch,wi,li)+PR.parts(n);if(cost>budget)continue;
 double H=0,P=0,held=0;for(int i=0;i<N;i++)if(int co=core(g[i])){int pulse=co==1?1:co==2?4:12,r=0,cs=0;for(int j:a[i]){pulse+=core(g[j]);r+=g[j]=='R';cs+=g[j]=='C';}double h=pulse*pulse*M.fuelHeat/co;H+=h;P+=pulse*M.fuelPow*(1+M.refl*r);if(cs)held+=h;}
 double cooling=n['V']*M.vr0*(1+M.ventBoost*(M.bAv+av)*n['K']);if(cooling<H)continue; double hc=n['P']*M.hullPer;if(hc<=H)continue;
 // Feed's inlets must evacuate its coolant heat; exact check later.
 if(recirc)held+=H*4*recirc/max(1,n['V']); double gross=P;if(held>0)gross*=M.boost(max(1.,held-hc*M.drain));
 double sales=M.sales(n['K'],wi,li);double sold=min(gross,sales),net=sold-fuelcost*1e6/M.fuelLife;feasible[group]++;
 // Keep independent leaders in every actual fuel/family/feed group, never stop at first pass.
 string key=to_string(n['1'])+"S"+to_string(n['2'])+"D"+to_string(n['4'])+"Q_f"+to_string(family)+"_feed"+to_string(feed);Result r{{g,av,hs,ex,re,ch,cost,sold,H,cooling,(int)z},net,gross,sales,wi,li,key};auto &v=leaders[key];v.push_back(r);sort(v.begin(),v.end(),[](Result a,Result b){return a.net*(M.bCh+a.c.ch)>b.net*(M.bCh+b.c.ch);});if((int)v.size()>keep)v.resize(keep);
}
cerr<<"симуляция лидеров ("<<leaders.size()<<" групп)...\n";ofstream out("leaders.tsv");out<<"group\tseed\tnet_B\tgross_B\tsold_B\theat_B\tcooling_B\tcost_Qa\twiring\tlines\tchrono\tpassed\n";int tested=0,passed=0;double best=-1e99;sim_steps=(int)PI("pfSteps");for(auto &[key,rs]:leaders)for(auto &r:rs){tested++;bool ok=true;for(int m=0;m<2;m++)if(!sim(r.c,m,0))ok=false;out<<key<<'\t'<<r.c.seed<<'\t'<<r.net<<'\t'<<r.gross<<'\t'<<r.c.power<<'\t'<<r.c.heat<<'\t'<<r.c.cool<<'\t'<<r.c.cost<<'\t'<<r.wiring<<'\t'<<r.lines<<'\t'<<r.c.ch<<'\t'<<ok<<'\n';if(!ok)continue;passed++;string file=key+"_"+to_string(r.c.seed)+".txt";ofstream f(file);auto c=r.c;f<<setprecision(12)<<c.power<<' '<<c.heat<<' '<<c.cool<<' '<<c.cost<<' '<<c.av<<' '<<c.hs<<' '<<c.ex<<' '<<c.re<<' '<<c.ch<<' '<<c.seed<<'\n';for(int y=0;y<16;y++)f<<c.g.substr(y*19,19)<<'\n';if(r.net*(M.bCh+c.ch)>best){best=r.net*(M.bCh+c.ch);ofstream b("winner-path.txt");b<<file;cerr<<"лидер "<<file<<" net "<<r.net<<"\n";}}
ofstream stats("stats.txt");stats<<"attempts="<<loops<<" groups="<<leaders.size()<<" simulated="<<tested<<" passed="<<passed<<" best_net_B_per_sec="<<best<<'\n';for(auto [key,n]:attempts)stats<<key<<" attempted "<<n<<" analytic_pass "<<feasible[key]<<'\n';cerr<<"done "<<loops<<" tested "<<tested<<" passed "<<passed<<" best "<<best<<'\n';return 0;}
