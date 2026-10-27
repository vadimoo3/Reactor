// Движок: ReactorEngine.exe --params | --tool <id>. Параметры приходят в переменных RX_<key>.
#include "rx.h"
int rxRunTool(const std::string&id);
static std::string clean(const char*s){std::string r=s?s:"";for(char&c:r)if(c=='\t'||c=='\n')c=' ';return r;}
int main(int argc,char**argv){
 if(argc>=2&&!strcmp(argv[1],"--params")){
  for(auto&g:RX_GROUPS)printf("G\t%s\t%s\n",g.id,clean(g.title).c_str());
  for(auto&t:RX_TOOLS)printf("T\t%s\t%s\t%s\t%s\t%s\n",t.id,clean(t.title).c_str(),t.dir,t.groups,clean(t.desc).c_str());
  for(auto&p:RX_PARAMS)printf("P\t%s\t%s\t%s\t%s\t%c\t%s\n",p.grp,p.key,clean(p.label).c_str(),clean(p.def).c_str(),p.kind,clean(p.opts).c_str());
  return 0;}
 if(argc>=3&&!strcmp(argv[1],"--tool"))return rxRunTool(argv[2]);
 fprintf(stderr,"usage: ReactorEngine --params | --tool <id>\n");return 1;}
