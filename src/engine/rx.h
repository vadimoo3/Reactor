// Общие параметры всех инструментов. GUI передаёт значения в переменных окружения RX_<key>.
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

// kind: n - число, b - флажок, t - переключатель-чип (opts = символ компонента для иконки), f - файл, c - выбор из opts ("a|b|c")
struct RxParam{const char*grp;const char*key;const char*label;const char*def;char kind;const char*opts;};
struct RxGroup{const char*id;const char*title;};
struct RxTool{const char*id;const char*title;const char*dir;const char*groups;const char*desc;};

inline const RxGroup RX_GROUPS[]={
 {"save","Уровни улучшений"},{"price","Бюджет и цены"},{"engine","Константы игры"},{"early","Ранняя модель (Nefelium)"},
 {"an","Отжиг"},{"gd","Направленный случайный"},{"fx","Точное топливо"},{"rr","Чистый случайный"},{"pk","Отбор"},{"tl","Блоки из случайного"},{"fr","Полный случайный"},{"mo","Модули"},{"pf","Прибыль"},{"pv","Валидатор прибыли"},
 {"pr","Protium"},{"rf","Доводка Protium"},{"pl","Проверка Protium"},{"dr","Прямой эталон"},
 {"sv","Поиск v2"},{"hy","Гибрид"},{"tr","Переходы"},{"ve","Сводка"},{"th","Тепловая проверка"},
};

inline const RxTool RX_TOOLS[]={
 {"an","Отжиг: свободный поиск раскладки (многопоточный)","anneal","an,save,price,engine","Единая популяция: структурные и случайные операторы, отжиг, скрещивание, восстановление охлаждения и общий отбор."},
 {"tl","Блочная организация: повтор лучшего блока по сетке","blocks-tile","tl,an,save,price,engine","Блок w×h из лучших кандидатов, повторённый по всей сетке; отжиг по клеткам блока."},
 {"fr","Полный случайный поиск + проверка топа","full-random","fr,save,price,engine","reactor-full-random: мутации эталона и случайные сетки, затем полная симуляция лучших."},
 {"mo","Перебор модулей 3x3","modules","mo,save,price,engine","reactor-modules: комбинации готовых модулей, цикл 'работа + остывание'."},
 {"pf","Поиск прибыльной раскладки","profit","pf,save,price,engine","reactor-profit: лидеры по группам топливо/охлаждение/питание, затем симуляция."},
 {"pv","Валидатор прибыли","profit","pv,save,price,engine","reactor-profit/validate-profit: 4 порядка обхода x пауза между циклами."},
 {"pr","Поиск Protium","protium","pr,save,price,engine","reactor-protium/search: блоки E/V/O/P, Forceful-питание, симуляция с авариями."},
 {"rf","Доводка Protium","protium","rf,save,price,engine","reactor-protium/refine: перебор числа Plating и Exchanger для best.txt."},
 {"pl","Проверка схемы (Protium-модель)","protium","pl,save,price,engine","Общий симулятор: 6 порядков обработки × штатно, аварии, замена топлива и выгорание."},
 {"dr","Прямой эталон без Exchanger","protium","dr,save,price,engine","reactor-protium/direct-reference: стержни с Reflector и кресты O+V."},
 {"sv","Ранний поиск v2 (Nefelium)","search","sv,early","reactor-search/search-v2: мозаики F/C/I/R, профили улучшений p0..p2."},
 {"hy","Гибрид C+I","search","hy,early","reactor-search/hybrid: вставка Coolant+Inlet в direct-раскладки."},
 {"tr","Переходы по бюджету","search","tr,early","reactor-search/transitions: замена F на Protium-стержни под бюджеты."},
 {"gd","Направленный случайный (тепловой баланс)","guided","gd,save,price,engine","reactor-guided: случайное топливо, Reflector, Plating по теплу, Outlet+Vent с расчётом нагрузки."},
 {"fx","Точное количество топлива","fixed-fuel","fx,save,price,engine","reactor-fixed-FUEL: заданное число клеток одного типа, ищутся только позиции и охлаждение."},
 {"rr","Чистый случайный","random-raw","rr,save,price,engine","reactor-random: равновероятные поля и поля со случайными весами U^8."},
 {"ve","Сводка результатов поиска","search","ve,early","reactor-search/verify-v2: results.json и verification-input.txt по всем схемам."},
 {"th","Тепловая проверка","search","th,early","reactor-search/thermal-check: потеря выхода и истечение топлива для verification-input."},
};

inline const RxParam RX_PARAMS[]={
 // ---- модель игры (reactor-protium/search.cpp = thermal-base)
 {"engine","fuelHeat","Тепло стержня","1250",'n'},{"engine","fuelPow","Мощн. стержня","2500",'n'},{"engine","refl","Бонус Reflector","0.1",'n'},
 {"engine","forceful","Forceful коэф.","0.06",'n'},{"engine","rate0","Обмен база","0.50625",'n'},{"engine","capBoost","Capac.→обмен","0.30",'n'},
 {"engine","vr0","Vent охл. база","2.024",'n'},{"engine","ventBoost","Capac.→Vent","0.05",'n'},{"engine","vc0","Vent ёмк. база","10.12",'n'},
 {"engine","sinkBoost","Plat.→Vent ёмк.","0.05",'n'},{"engine","ec0","Exch ёмк. база","10.125",'n'},{"engine","reBoost","Plat.→Exch ёмк.","0.05",'n'},
 {"engine","ccap","Coolant ёмк.","14693",'n'},{"engine","kcap","Capac. ёмк.","2e-08",'n'},{"engine","hullPer","Корпус/Plating","1229.312",'n'},
 {"engine","drain","Утечка корпуса","0.0001",'n'},{"engine","sellB","Продажа/Capac.","38.416",'n'},{"save","sellLvl","Проводка, ур.","17",'n'},
 {"save","sellPct","Линии, ур.","13",'n'},{"save","bEx","Heat Exchangers, ур.","7",'n'},{"save","bAv","Active Venting, ур.","6",'n'},
 {"save","bHs","Heat Sinks, ур.","6",'n'},{"save","bRe","Reinforced Exch., ур.","6",'n'},{"save","bCh","Chrono, ур.","7",'n'},
 {"engine","simSteps","Тиков симуляции","6500",'n'},{"engine","faultT","Тик аварии","5500",'n'},{"engine","outFault","Outlet off, тиков","8",'n'},
 {"engine","ventFault","Vent off, тиков","5",'n'},{"engine","expStart","Замена: начало","3600",'n'},{"engine","expEnd","Замена: конец","7200",'n'},
 {"engine","fuelLife","Жизнь стержня, тик","3600",'n'},
 {"engine","simSkip","Пропуск неизменного состояния","1",'b'},
 // ---- бюджет и цены
 {"price","budget","Бюджет, Qa","19.4504787631017",'n'},{"price","f1","Single, Qa","3",'n'},{"price","f2","Double, Qa","6.6",'n'},
 {"price","f4","Quad, Qa","14.52",'n'},{"price","pav","Цена +1 Active Venting, Qa","1",'n'},{"price","phs","Цена +1 Heat Sinks, Qa","1",'n'},
 {"price","pex","Цена +1 Heat Exch., Qa","0.6",'n'},{"price","pre","Цена +1 Reinforced, Qa","1",'n'},{"price","pch","Цена +1 Chrono, Qa","10",'n'},
 {"price","pwi","Цена +1 Проводка, Qa","0.762939453125",'n'},{"price","pli","Цена +1 Линии, Qa","1",'n'},
 {"price","cE","Exchanger, мQa","0.256",'n'},{"price","cO","Outlet, мQa","0.256",'n'},{"price","cI","Inlet, мQa","0.256",'n'},
 {"price","cV","Vent, мQa","0.195312",'n'},{"price","cC","Coolant, мQa","0.8",'n'},{"price","cP","Plating, мQa","0.65536",'n'},
 {"price","cK","Capacitor, мQa","0.65536",'n'},{"price","cR","Reflector, мQa","0.000005",'n'},
 // ---- ранняя модель (reactor-search)
 {"early","eFH","Тепло F","0.226966",'n'},{"early","eFP","Мощн. F","0.453932",'n'},{"early","eNH","Тепло/мощн. N","51.87",'n'},
 {"early","eRate","Обмен база","3.54375",'n'},{"early","eRateK","Capac.→обмен","0.25",'n'},{"early","eVent","Vent охл. база","0.1265",'n'},
 {"early","eVcap","Vent ёмк. база","1.265",'n'},{"early","eHull","Корпус/Plating","537.824",'n'},{"early","eCcap","Coolant ёмк.","10495",'n'},
 {"early","eRefl","Бонус Reflector","0.1",'n'},
 // ---- отжиг
 {"an","anMode","Старт","0",'c',"с нуля|от файла|рестарты"},{"an","anStart","Файл старта","",'f'},{"an","anSecs","Время, с","120",'n'},
 {"an","anThreads","Потоки","4",'n'},{"an","anSeed","Seed","1",'n'},{"an","anIps","Итер/рестарт","300000",'n'},
 {"an","anT0","Темп. нач.","900",'n'},{"an","anT1","Темп. кон.","15",'n'},{"an","anPen","Штраф охл.","4",'n'},
 {"an","anMargin","Запас охл., %","2",'n'},{"an","anW","Ширина W","19",'n'},{"an","anH","Высота H","16",'n'},
 {"an","anMinF","Мин. стержней","0",'n'},{"an","anMaxF","Макс. (0=∞)","0",'n'},{"an","anA1","Single","1",'t',"1"},
 {"an","anA2","Double","1",'t',"2"},{"an","anA4","Quad","1",'t',"4"},
 {"an","anFav","+1 Active Venting","1",'t',""},{"an","anFhs","+1 Heat Sinks","1",'t',""},{"an","anFex","+1 Heat Exch.","1",'t',""},
 {"an","anFre","+1 Reinforced","1",'t',""},{"an","anFch","+1 Chrono","0",'t',""},{"an","anM2","2-й порядок обм.","1",'b'},
 {"an","anOut","Тест Outlet off","0",'b'},{"an","anVent","Тест Vent off","0",'b'},{"an","anNoSim","Без симуляции","0",'b'},
 {"an","anNoCool","Не треб. охлажд.","0",'b'},
 {"an","anLibrary","Папка стартовых схем","",'f'},{"an","anStrategy","Приоритет поиска","blocks",'f'},
 {"an","anAttempts","Попыток (0 = по времени)","0",'n'},{"an","anPopulation","Размер популяции","96",'n'},
 {"an","anVerify","Полных проверок за запуск","240",'n'},{"an","anOutFile","Итоговый файл","best.txt",'f'},
 {"an","anObjective","Цель: profit или heat","profit",'f'},
 // ---- полный случайный
 {"fr","frAttempts","Попыток","100000",'n'},{"fr","frSeed","Seed","123456",'n'},{"fr","frVerify","Проверить топ","50",'n'},
 {"fr","frSteps","Тиков проверки","7200",'n'},{"fr","frMaxF","Макс. стержней","6",'n'},{"fr","frChrono","Chrono 1 из","20",'n'},
 {"fr","frRef","Эталон","reference-layout.txt",'f'},{"fr","frStable","Стабильная","stable-reference.txt",'f'},
 // ---- модули
 {"mo","moDraws","Попыток","500000",'n'},{"mo","moSeed","Seed","2026102711",'n'},{"mo","moSlots","Модулей всего","30",'n'},
 {"mo","moMaxF","Макс. стержней","6",'n'},{"mo","moPlat","Plating","8",'n'},{"mo","moTicks","Тиков цикла","7200",'n'},
 {"mo","moAv","+1 Active Venting","1",'t',""},{"mo","moHs","+1 Heat Sinks","0",'t',""},{"mo","moEx","+1 Heat Exch.","0",'t',""},
 // ---- прибыль
 {"pf","pfLoops","Попыток","1000000",'n'},{"pf","pfSeed","Seed","2026102710",'n'},{"pf","pfMinF","Мин. стержней","2",'n'},
 {"pf","pfMaxF","Макс. стержней","6",'n'},{"pf","pfMaxPlat","Макс. Plating","22",'n'},{"pf","pfKeep","Лидеров в группе","6",'n'},
 {"pf","pfChrono","Chrono 1 из","20",'n'},{"pf","pfSteps","Тиков проверки","7200",'n'},
 {"pv","pvFile","Схема (пусто=winner)","",'f'},{"pv","pvTicks","Тиков","100000",'n'},
 // ---- protium
 {"pr","prLoops","Попыток","1000000",'n'},{"pr","prSeed","Seed","2026102603",'n'},{"pr","prTested","Макс. симуляций","1500",'n'},
 {"pr","prMaxF","Макс. стержней","6",'n'},{"pr","prPmin","Plating мин.","20",'n'},{"pr","prPrange","Plating разброс","33",'n'},
 {"pr","prCoolM","Запас охлаждения","1.15",'n'},{"pr","prHullM","Запас корпуса","8.5",'n'},{"pr","prVentF","Заполн. Vent","0.58",'n'},
 {"rf","rfFile","Исходная схема","best.txt",'f'},{"rf","rfPmin","Plating от","48",'n'},{"rf","rfPmax","Plating до","60",'n'},
 {"rf","rfPfinal","Plating итог","51",'n'},{"rf","rfExFinal","Exch итог","1",'b'},{"rf","rfFill","Макс. заполн.","0.90",'n'},
 {"rf","rfMinCool","Мин. охлаждение","5750",'n'},
 {"pl","plFile","Схема","final.txt",'f'},{"pl","plSteps","Тиков","100000",'n'},
 {"dr","drTrials","Попыток","100000",'n'},{"dr","drSeed","Seed","2026102604",'n'},{"dr","drMaxF","Макс. стержней","6",'n'},
 {"dr","drDouble","Доля Double","0.12",'n'},{"dr","drPmin","Plating мин.","20",'n'},{"dr","drPrange","Plating разброс","30",'n'},
 {"dr","drCoolM","Запас охлаждения","1.15",'n'},
 // ---- reactor-search
 {"sv","svRuns","Запусков","8",'n'},{"sv","svAtt","Попыток/запуск","2500000",'n'},{"sv","svSeed","Seed","20261026",'n'},
 {"sv","svStep","Шаг seed","7919",'n'},{"sv","svMinF","Мин. F","35",'n'},{"sv","svExactF","Ровно F (0 = нет; dolorium-60-search)","0",'n'},{"sv","svMaxH","Макс. тепло","3500",'n'},
 {"hy","hyIter","Итераций","7000",'n'},{"hy","hySeed","Seed","6102702",'n'},
 {"tr","trAtt","Попыток/схема","250000",'n'},{"tr","trSeed","Seed","6102701",'n'},{"tr","trB1","Бюджет 1","5",'n'},
 {"tr","trB2","Бюджет 2","10",'n'},{"tr","trB3","Бюджет 3","25",'n'},{"tr","trB4","Бюджет 4","100",'n'},
 {"gd","gdTrials","Попыток","10000",'n'},{"gd","gdMaxF","Макс. топливных клеток","6",'n'},{"gd","gdSeed","Seed","2026102713",'n'},
 {"fx","fxCount","Топливных клеток","6",'n'},{"fx","fxType","Тип (1, 2, 4)","1",'n'},{"fx","fxTrials","Попыток","1000",'n'},{"fx","fxSeed","Seed","2026102714",'n'},
 {"rr","rrLoops","Попыток","3000000",'n'},{"rr","rrSeed","Seed","2026102712",'n'},
 {"tl","tlDir","Папка схем-источников блоков","",'f'},
 {"pk","pkTop","Сколько лучших разных","1",'n'},{"pk","pkScore","Режим оценки генератора","0",'n'},
 {"pk","pkCount","Точное число топливных клеток (0 = любое)","0",'n'},{"pk","pkMaxVerify","Лимит проверок","400",'n'},{"pk","pkFloor","Только лучше исходной","0",'b'},
 {"pk","fuelName","Топливо","protium",'f'},{"pk","pkDir","Папка кандидатов","",'f'},{"pk","pkBase","Исходная схема","",'f'},{"pk","pkOut","Итоговый файл","",'f'},
 {"th","thFile","Вход","verification-input.txt",'f'},{"th","thTicks","Тиков","100000",'n'},{"th","thExp","Тик истечения","86401",'n'},
};

inline std::string rxDefault(const char*k){for(auto&p:RX_PARAMS)if(!strcmp(p.key,k))return p.def;fprintf(stderr,"unknown param %s\n",k);return "0";}
inline std::string S(const char*k){std::string e=std::string("RX_")+k;const char*v=getenv(e.c_str());return v?v:rxDefault(k);}
inline double P(const char*k){return atof(S(k).c_str());}
inline long long PI(const char*k){return llround(P(k));}

struct RxModel{
 double fuelHeat=P("fuelHeat"),fuelPow=P("fuelPow"),refl=P("refl"),forceful=P("forceful"),rate0=P("rate0"),capBoost=P("capBoost"),
  vr0=P("vr0"),ventBoost=P("ventBoost"),vc0=P("vc0"),sinkBoost=P("sinkBoost"),ec0=P("ec0"),reBoost=P("reBoost"),ccap=P("ccap"),kcap=P("kcap"),
  hullPer=P("hullPer"),drain=P("drain"),sellB=P("sellB"),sellLvl=P("sellLvl"),sellPct=P("sellPct"),
  bEx=P("bEx"),bAv=P("bAv"),bHs=P("bHs"),bRe=P("bRe"),bCh=P("bCh"),fuelLife=P("fuelLife");
 int simSteps=(int)PI("simSteps"),faultT=(int)PI("faultT"),outFault=(int)PI("outFault"),ventFault=(int)PI("ventFault"),expStart=(int)PI("expStart"),expEnd=(int)PI("expEnd");
 double sales(double k,int wi=0,int li=0)const{return k*sellB*(sellLvl+wi)*2*.01*(sellPct+li);}
 double boost(double held)const{return 1+forceful*log(held*1e9)/log(1000.);}
};
struct RxPrice{
 double budget=P("budget"),f1=P("f1"),f2=P("f2"),f4=P("f4"),av=P("pav"),hs=P("phs"),ex=P("pex"),re=P("pre"),ch=P("pch"),wi=P("pwi"),li=P("pli"),
  E=P("cE"),O=P("cO"),I=P("cI"),V=P("cV"),C=P("cC"),Pl=P("cP"),K=P("cK"),R=P("cR");
 template<class Cnt>double fuel(const Cnt&n)const{return n['1']*f1+n['2']*f2+n['4']*f4;}
 template<class Cnt>double parts(const Cnt&n)const{return (n['E']*E+n['O']*O+n['I']*I+n['V']*V+n['C']*C+n['P']*Pl+n['K']*K+n['R']*R)/1000.;}
 double upgrades(int av_,int hs_,int ex_,int re_,int ch_,int wi_=0,int li_=0)const{return av_*av+hs_*hs+ex_*ex+re_*re+ch_*ch+wi_*wi+li_*li;}
};
struct RxEarly{double FH=P("eFH"),FP=P("eFP"),NH=P("eNH"),rate=P("eRate"),rateK=P("eRateK"),vent=P("eVent"),vcap=P("eVcap"),hull=P("eHull"),ccap=P("eCcap"),refl=P("eRefl");};

inline const RxModel M;
inline const RxPrice PR;
inline const RxEarly EM;
