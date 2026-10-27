// Все инструменты архива в одном exe: каждый исходник в своём пространстве имён.
#include "rx.h"
namespace t_an{
#include "anneal.cpp"
}
namespace t_fr{
#include "algorithms/reactor-full-random/search.cpp"
}
namespace t_mo{
#include "algorithms/reactor-modules/search.cpp"
}
namespace t_pf{
#include "algorithms/reactor-profit/search.cpp"
}
namespace t_pv{
#include "algorithms/reactor-profit/validate-profit.cpp"
}
namespace t_pr{
#include "algorithms/reactor-protium/search.cpp"
}
namespace t_rf{
#include "algorithms/reactor-protium/refine.cpp"
}
namespace t_pl{
#include "algorithms/reactor-protium/validate.cpp"
}
namespace t_dr{
#include "algorithms/reactor-protium/direct-reference.cpp"
}
namespace t_sv{
#include "algorithms/reactor-search/search-v2.cpp"
}
namespace t_hy{
#include "algorithms/reactor-search/hybrid.cpp"
}
namespace t_tr{
#include "algorithms/reactor-search/transitions.cpp"
}
namespace t_ve{
#include "algorithms/reactor-search/verify-v2.cpp"
}
namespace t_th{
#include "algorithms/reactor-search/thermal-check.cpp"
}
namespace t_gd{
#include "algorithms/reactor-guided/search.cpp"
}
namespace t_fx{
#include "algorithms/reactor-fixed-fuel/search.cpp"
}
namespace t_rr{
#include "algorithms/reactor-random/search.cpp"
}

int rxRunTool(const std::string&id){
 std::cout<<std::unitbuf;
 if(id=="an")return t_an::main();
 if(id=="ev")return t_an::evalMain();
 if(id=="pk")return t_an::pickMain();
 if(id=="tl")return t_an::tileMain();
 if(id=="gd")return t_gd::main(0,nullptr);
 if(id=="fx")return t_fx::main(0,nullptr);
 if(id=="rr")return t_rr::main(0,nullptr);
 if(id=="fr")return t_fr::main(0,nullptr);
 if(id=="mo")return t_mo::main();
 if(id=="pf")return t_pf::main(0,nullptr);
 if(id=="pv")return t_pv::main();
 if(id=="pr")return t_pr::main(0,nullptr);
 if(id=="rf")return t_rf::main();
 if(id=="pl")return t_an::validateMain();
 if(id=="dr")return t_dr::main();
 if(id=="sv")return t_sv::main(0,nullptr);
 if(id=="hy")return t_hy::main();
 if(id=="tr")return t_tr::main();
 if(id=="ve")return t_ve::main();
 if(id=="th")return t_th::main();
 std::cerr<<"неизвестный инструмент "<<id<<"\n";return 1;}
