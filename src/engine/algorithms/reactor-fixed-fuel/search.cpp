#include "../reactor-guided/engine.hpp"
// reactor-fixed-FUEL: точное количество fxCount топливных клеток одного типа fxType (1/2/4). Попытки fxTrials, seed fxSeed.
// Тип и количество не меняются; при нехватке бюджета — OVER_BUDGET. Результаты в results-ТИП-КОЛИЧЕСТВО-seedНОМЕР.
int main(int argc,char**argv){
 int exact_count=(int)PI("fxCount");int typeCode=(int)PI("fxType");int trials=(int)PI("fxTrials");unsigned seed=(unsigned)PI("fxSeed");double budget=PR.budget,venting=PR.av;
 char selected=typeCode==1?'1':typeCode==2?'2':typeCode==4?'4':0;string type=selected=='1'?"single":selected=='2'?"double":"quad";
 if(!selected||exact_count<1||exact_count>N||trials<1||budget<=0)return 2;
 string dest="results-"+type+"-"+to_string(exact_count)+"-seed"+to_string(seed);
 filesystem::create_directories(dest);filesystem::remove(dest+"/best.txt");filesystem::remove(dest+"/results.csv");
 loadFuels();rng.seed(seed);
 ofstream request(dest+"/request.txt");request<<setprecision(14)<<"exact_count "<<exact_count<<"\ntype "<<type<<"\nattempts "<<trials<<"\nseed "<<seed<<"\nbudget_Qa "<<budget<<"\n";request.close();
 double fuel_price=exact_count*fuels[selected].cost;
 if(fuel_price+venting>budget){ofstream st(dest+"/stats.txt");st<<"status OVER_BUDGET\nfuel_cost_Qa "<<fuel_price<<"\nrequired_active_venting_Qa "<<venting<<"\nbudget_Qa "<<budget<<"\n";cerr<<"OVER_BUDGET: "<<exact_count<<" "<<type<<"; топливо "<<fuel_price<<" Qa + Active Venting "<<venting<<" Qa; бюджет "<<budget<<" Qa\n";return 3;}
 for(int i=0;i<N;i++){if(i%19)a[i].push_back(i-1);if(i%19<18)a[i].push_back(i+1);if(i>=19)a[i].push_back(i-19);if(i<285)a[i].push_back(i+19);}
 long built=0,thermal=0,passed=0;double best=-1e99;ofstream log(dest+"/results.csv");log<<"attempt,fuel_count,heat_B,cooling_B,worst_vent_load_B,vent_capacity_B,cost_Qa,status,net_B,reason,tick\n";
 for(int z=0;z<trials;z++){string g(N,'K');int nf=exact_count;double fc=0;for(int j=0;j<nf;j++){int i=rng()%N;if(g[i]!='K'){j--;continue;}g[i]=selected;fc+=fuels[g[i]].cost;}if(fc+venting>budget)continue;for(int i=0;i<N;i++)if(cores(g[i]))for(int j:a[i])if(g[j]=='K')g[j]='R';double heat=0;for(int i=0;i<N;i++)if(int c=cores(g[i])){int pulse=fuels[g[i]].pulse;for(int j:a[i])pulse+=cores(g[j]);heat+=fuels[g[i]].heat*pulse*pulse/c;}
 int pn=max(1,(int)ceil(heat/M.hullPer))+rng()%5;if(pn>count(g.begin(),g.end(),'K'))continue;for(int j=0;j<pn;j++){int i=rng()%N;if(g[i]!='K'){j--;continue;}g[i]='P';}bool viable=false;double cooling=0,worst=0,vc=0,cost=0;
 for(int step=0;step<60;step++){vector<int> places;for(int i=0;i<N;i++)if(g[i]=='K'){int fresh=0,shared=0;for(int j:a[i]){fresh+=g[j]=='K';shared+=g[j]=='V';}if(fresh&&shared==0)places.push_back(i);}if(places.empty())break;int i=places[rng()%places.size()];g[i]='O';for(int j:a[i])if(g[j]=='K')g[j]='V';array<int,128>n{};for(char c:g)n[c]++;int k=n['K'],v=n['V'];double vr=M.vr0*(1+M.ventBoost*(M.bAv+1)*k),rate=M.rate0*M.bEx*2*(1+M.capBoost*k);vc=M.vc0*(1+M.sinkBoost*M.bHs*n['P']);cooling=v*vr;
 int edges=0,maxdegree=0,active=0;for(int j=0;j<N;j++)if(g[j]=='V'){int d=0;for(int q:a[j])d+=g[q]=='O';edges+=d;maxdegree=max(maxdegree,d);active+=d>0;}worst=edges?heat*maxdegree/edges:1e99;
 // A generated outlet has only vents as heat-absorbing neighbors. No vent shared between outlets in this routing policy.
 cost=fc+venting+PR.parts(n);
 if(cost<=budget&&active*vr>=heat*1.02&&worst<=vr&&edges*rate>=heat){viable=true;if(rng()%3==0)break;}}
 built++;if(!viable)continue;
 // Re-evaluate the final field: later additions may lower K and invalidate an earlier feasible state.
 array<int,128>n{};for(char c:g)n[c]++;double vr=M.vr0*(1+M.ventBoost*(M.bAv+1)*n['K']);int edges=0,md=0;for(int i=0;i<N;i++)if(g[i]=='V'){int d=0;for(int j:a[i])d+=g[j]=='O';edges+=d;md=max(md,d);}worst=edges?heat*md/edges:1e99;cooling=n['V']*vr;if(cooling<heat*1.02||worst>vr||cost>budget)continue;thermal++;int actual=0;for(char v:g)if(cores(v)){if(v!=selected)throw runtime_error("Fuel type changed");actual++;}if(actual!=exact_count)throw runtime_error("Fuel count changed");auto r=test(g,0);if(r.ok){auto q=test(g,1);if(!q.ok)r=q;else r.net=min(r.net,q.net);}log<<z<<','<<nf<<','<<heat<<','<<cooling<<','<<worst<<','<<vc<<','<<cost<<','<<r.ok<<','<<r.net<<','<<r.reason<<','<<r.tick<<'\n';if(!r.ok)continue;passed++;if(r.net>best){best=r.net;ofstream out(dest+"/best.txt");out<<setprecision(12)<<"net_B "<<best<<" cost_Qa "<<cost<<" heat_B "<<heat<<" cooling_B "<<cooling<<" attempt "<<z<<'\n';for(int y=0;y<16;y++)out<<g.substr(y*19,19)<<'\n';}}
 ofstream st(dest+"/stats.txt");st<<"status "<<(passed?"FOUND":"NO_SOLUTION_FOUND")<<" exact_count "<<exact_count<<" type "<<type<<"\n";st<<"attempts "<<trials<<" constructed "<<built<<" thermal_screen_pass "<<thermal<<" simulated_pass "<<passed<<" best_net_B "<<best<<'\n';cout<<"done "<<type<<" x"<<exact_count<<": "<<trials<<' '<<thermal<<' '<<passed<<' '<<best<<'\n';return 0;}
