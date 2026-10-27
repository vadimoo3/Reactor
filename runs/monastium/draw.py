import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


root = Path(__file__).resolve().parents[2]
folder = Path(__file__).resolve().parent
data = json.loads((folder / "summary.json").read_text(encoding="utf-8"))
font_path = "C:/Windows/Fonts/segoeui.ttf"
bold_path = "C:/Windows/Fonts/seguisb.ttf"
font = lambda size, bold=False: ImageFont.truetype(bold_path if bold else font_path, size)
names = {"K": "Capacitor", "V": "Vent", "O": "Outlet", "I": "Inlet", "E": "Exchanger", "C": "Coolant", "P": "Plate", "R": "Reflector"}
labels = {"K": "Ultimate Capacitor", "V": "Ultimate Vent", "O": "Ultimate Outlet", "P": "Ultimate Plating", "R": "Ultimate Reflector"}


for name, metrics in data.items():
    rows = [line for line in Path(metrics["file"]).read_text(encoding="utf-8").splitlines()[1:]
            if line and all(c in "124EVOPICKR." for c in line)]
    w, h = len(rows[0]), len(rows)
    image = Image.new("RGB", (1100, 1210), "#0c1323")
    draw = ImageDraw.Draw(image)
    draw.text((48, 25), "Reactor Incremental · Build 24", font=font(31, True), fill="#eef2ff")
    title = "Финал перед престижем" if metrics["goal"] == "heat" else "Накопление денег"
    draw.text((48, 70), title + " · " + metrics["fuel"].title(), font=font(25, True), fill="#5be1b5")
    draw.text((48, 110), f"Тепло {metrics['heat_T']:g} T/тик     Продажа {metrics['sale_T']:.3f} T/тик     Чистыми {metrics['net_T']:.3f} T/тик", font=font(21), fill="#e3e9fb")
    draw.text((48, 143), f"План покупки: {metrics['total_build_cost_Qi']:.4f} Qi · все детали Ultimate (T5) · 20 × 17", font=font(19), fill="#a6b5d0")
    cell, x0, y0 = 44, 100, 212
    for x in range(w):
        text = str(x + 1)
        draw.text((x0 + x * cell + cell / 2, y0 - 31), text, anchor="mm", font=font(19), fill="#acb8cf")
    for y in range(h):
        draw.text((x0 - 30, y0 + y * cell + cell / 2), str(y + 1), anchor="mm", font=font(19), fill="#acb8cf")
        for x in range(w):
            code = rows[y][x]
            xx, yy = x0 + x * cell, y0 + y * cell
            draw.rectangle((xx, yy, xx + cell - 1, yy + cell - 1), fill="#131f33", outline="#33405a")
            if code == ".":
                continue
            if code in "124":
                sprite = f"Fuel{8 if metrics['fuel'] == 'monastium' else 7}-{code}"
            else:
                sprite = names[code] + "5"
            tile = Image.open(root / "icons" / (sprite + ".png")).convert("RGBA")
            image.paste(tile, (xx + (cell - tile.width) // 2, yy + (cell - tile.height) // 2), tile)
            if code in "124":
                draw.rectangle((xx + 1, yy + 1, xx + cell - 2, yy + cell - 2), outline="#ffdb70", width=2)
    legend = [(key, count) for key, count in metrics["counts"].items() if key != "."]
    columns = 3 if len(legend) <= 6 else 4
    for i, (code, count) in enumerate(legend):
        xx, yy = 48 + (i % columns) * (1004 // columns), 994 + (i // columns) * 52
        if code in "124":
            label = {"1": "Single", "2": "Double", "4": "Quad"}[code] + " " + metrics["fuel"].title()
            sprite = f"Fuel{8 if metrics['fuel'] == 'monastium' else 7}-{code}"
        else:
            label, sprite = labels.get(code, code), names[code] + "5"
        tile = Image.open(root / "icons" / (sprite + ".png")).convert("RGBA")
        image.paste(tile, (xx, yy), tile)
        draw.text((xx + 41, yy + 6), f"{label} ×{count}", font=font(16), fill="#e1e8f7")
    positions = "; ".join(f"({fuel['column']},{fuel['row']}) — {fuel['cores']} ядра" for fuel in metrics["fuels"])
    draw.text((48, 1110), "Топливо: " + positions, font=font(17), fill="#efcf81")
    draw.text((48, 1142), "Координаты: колонка, строка от верхнего левого угла. Фоновые клетки K — конденсаторы.", font=font(17), fill="#a6b5d0")
    draw.text((48, 1173), "Расчёт собственной модели; штатно проходит, аварийное отключение охлаждения не выдерживает.", font=font(17), fill="#a6b5d0")
    output = root / "results/profit" / (name + ".png")
    image.save(output)
    print(output)
