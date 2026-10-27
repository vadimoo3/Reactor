from PIL import Image,ImageDraw,ImageFont
import pathlib,json
D=pathlib.Path(__file__).resolve().parent;root=D.parent/'results';r=json.loads((root/'search'/'results.json').read_text(encoding='utf-8'))
font=next((f for f in ['/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf','C:/Windows/Fonts/segoeui.ttf','C:/Windows/Fonts/arial.ttf'] if pathlib.Path(f).exists()))
colors=dict(F='#85522d',C='#216479',I='#987338',R='#66518c',P='#465273',K='#645e34',O='#296b61',V='#3c6d5a',**{'1':'#803951','2':'#a24761','4':'#c45878'})
sprites=dict(F='Fuel5-4',C='Coolant5',I='Inlet5',R='Reflector5',P='Plate5',K='Capacitor5',O='Outlet5',V='Vent5',**{'1':'Fuel6-1','2':'Fuel6-2','4':'Fuel6-4'})
assets=D.parent/'icons'
def setup(w,h):
 im=Image.new('RGB',(w,h),'#101827');d=ImageDraw.Draw(im)
 def txt(x,y,s,size=22,color='#e5edf7'):d.text((x,y),s,font=ImageFont.truetype(font,size),fill=color)
 return im,d,txt
def grid(im,d,txt,s,x,y,cell):
 for xx in range(19):txt(x+xx*cell+cell*.29,y-26,str(xx+1),13,'#becbdd')
 for yy,row in enumerate(s['grid']):
  txt(x-28,y+yy*cell+cell*.25,str(yy+1),13,'#becbdd')
  for xx,c in enumerate(row):
   px,py=x+xx*cell,y+yy*cell;d.rectangle((px,py,px+cell-1,py+cell-1),fill=colors[c],outline='#6e7a8c');ico=Image.open(assets/(sprites[c]+'.png')).convert('RGBA').resize((int(cell*.64),int(cell*.64)),Image.Resampling.NEAREST);im.paste(ico,(px+int(cell*.18),py+2),ico)
   txt(px+2,py+int(cell*.66),c,max(10,int(cell*.25)))
   if c in 'CI':d.rectangle((px,py,px+cell-1,py+cell-1),outline='#66e2ef',width=3)
im,d,txt=setup(1720,1270);s=r['hybrid-R-p0'];txt(35,22,'72 QUAD DOLORIUM · МАЛЫЙ КОНТУР ДЛЯ FORCEFUL FISSION',29)
txt(35,72,'741,54 B/тик базовая мощность · ≈873,15 B/тик после выхода на режим',25,'#a7ddbc')
txt(35,113,'Текущие улучшения из risave.txt / Build 24. Служебные компоненты — Ultimate.',20,'#b8c7dc');grid(im,d,txt,s,55,177,51)
x=1080;txt(x,177,'СОСТАВ — 304 КЛЕТКИ',24,'#f1c575');names=dict(F='Quad Dolorium',R='Neutron Reflector',C='Coolant Cell',I='Heat Inlet',O='Heat Outlet',V='Heat Vent',K='Capacitor',P='Reactor Plating')
for z,c in enumerate('FRCIOVKP'):
 yy=228+z*61;d.rectangle((x,yy,x+36,yy+36),fill=colors[c]);txt(x+8,yy+3,c,21);txt(x+52,yy,f"{s['counts'][c]} × {names[c]}",23)
txt(x,751,'C + I: поддержание бонуса нагрева.',20,'#69d6e2');txt(x,784,'Это не аварийная защита Vent.',20,'#69d6e2')
txt(x,839,'Инфраструктура: ≈86,96 T.',21);txt(x,873,'Полная загрузка: ≈1,454 Qa.',21)
txt(x,925,'Следующий Potent Dolorium: 200 T',20,'#f1c575');txt(x,957,'≈1,310 T/тик при том же тепле.',21,'#a7ddbc')
txt(35,1032,'Тепло: 1 536,11 B/тик · охлаждение: 1 783,40 B/тик · запас: 16,10%.',24,'#a7ddbc')
txt(35,1073,'Без одного Outlet: 1 689,53 B/тик · запас: 9,99%. Корпус: 8,605 T.',23)
txt(35,1115,'Coolant находится в строке 9, столбце 11; Inlet — прямо под ним. Остальное тепло идёт в корпус.',19,'#b8c7dc')
txt(35,1150,'Сборка на паузе → холодный старт → добавление топлива. Для непрерывного AFK нужна автозамена.',19,'#b8c7dc')
txt(35,1190,'Проверено 100 000 тактов модели, включая отказ Outlet. В оригинальном движке не испытано.',19,'#f1c575')
txt(35,1220,'Seed: 20261026, кандидат 431455; малый контур: seed 6102702, итерация 1058.',18,'#b8c7dc')
im.save(root/'reactor-72-hybrid.png')
im,d,txt=setup(1550,1180);txt(35,25,'ДВА ЭТАПА РАЗВИТИЯ · ПРОВЕРЕННЫЕ КАНДИДАТЫ МОДЕЛИ',28)
for x,key,title in [(50,'transition-5-D','ЭТАП 1 · 75 QUAD DOLORIUM'),(820,'transition-100-N','ЭТАП 2 · СМЕШАННЫЙ NEFASTIUM')]:
 s=r[key];txt(x,87,title,23,'#f1c575');txt(x,125,f"≈{s['warm_power']/1000:.3f} T/тик · база {s['base_power']/1000:.3f} T/тик",22,'#a7ddbc');grid(im,d,txt,s,x,194,35)
 if '5-D' in key:
  lines=['75 F · 10 R · 1 C · 1 I · 19 O · 76 V · 102 K · 20 P','Полная стоимость с улучшениями: 4,984 Qa.','Potent Dolorium: уровень 1 → 3, всего 2,2 Qa.','Heat Vents: 6 → 7, 250 T. Heatsinks: 5 → 6, 10 T.','Active Venting: 6 → 7, 1 Qa.','Тепло 2,394 T/тик; охлаждение 2,823 T/тик.','Автозамена Dolorium оплачивается отдельно.']
 else:
  lines=['11 одиночных N (1) + 5 двойных N (2) + 14 F.','40 R · 1 C · 1 I · 21 O · 84 V · 106 K · 21 P','Полная стоимость с улучшениями: 98,323 Qa.','Те же улучшения охлаждения, что слева.','Potent D: 1 → 2; Potent N: 0 → 1 (17,5 Qa).','Forceful Fission: 5 → 6 (100 T).','Тепло 2,816 T/тик; охлаждение 3,239 T/тик.']
 for j,line in enumerate(lines):txt(x,784+j*35,line,18)
txt(35,1052,'Обозначения: F — Quad Dolorium; 1/2 — Single/Double Nefastium; остальные детали — Ultimate.',19,'#b8c7dc')
txt(35,1091,'Nefastium без автозамены выгорает за 86 400 тиков. Автозамена N стоит отдельно 3,586 Qi.',20,'#f1c575')
txt(35,1129,'Мощность с бонусом нагрева рассчитана по восстановленной последовательности такта; нужна сверка в игре.',18,'#b8c7dc')
im.save(root/'reactor-upgrade-path.png')
