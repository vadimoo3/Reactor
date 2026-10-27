import json
import math
import random
from pathlib import Path


root = Path(__file__).resolve().parents[2]
folder = Path(__file__).resolve().parent
save = json.loads((root / "current save/current-save.json").read_text(encoding="utf-8"))
upgrades = json.loads((folder / "source/upgrade_data.json").read_text(encoding="utf-8"))["upgrades"]
levels = save["upgrade_levels"]
by_name = {item["name"]: item for item in upgrades}
names = {13: "Improved Wiring", 15: "Improved Power Lines"}
width, height = 19 + levels[50], 16 + levels[50]
seed = 2026102701
rng = random.Random(seed)
scenarios = {
    "protium-heat": ("protium", 22, 9, "heat"),
    "protium-profit": ("protium", 22, 9, "profit"),
    "monastium-heat": ("monastium", 21, 9, "heat"),
    "monastium-profit": ("monastium", 22, 8, "profit"),
}
report = {}


def neighbors(index):
    x, y = index % width, index // width
    return [yy * width + xx for xx, yy in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1))
            if 0 <= xx < width and 0 <= yy < height]


for name, (fuel, wiring, vents, objective) in scenarios.items():
    target = levels.copy()
    for index, value in {13: wiring, 15: 17, 22: 8, 25: vents, 27: 8}.items():
        target[index] = value
    buys = []
    for index, (old, new) in enumerate(zip(levels, target)):
        if new == old:
            continue
        item = by_name[names[index]] if index in names else upgrades[index]
        cost = sum(item["base_cost"] * item["cost_multiplier"] ** value for value in range(old, new))
        buys.append(dict(index=index, name=names.get(index, item["name"]), before=old, after=new,
                         cost=cost, next_cost=item["base_cost"] * item["cost_multiplier"] ** new))
    spent = sum(item["cost"] for item in buys)
    params = dict(
        fuelName=fuel, fuelHeat=2500 if fuel == "protium" else 2000,
        fuelPow=10000 if fuel == "protium" else 16000,
        fuelLife=3600 if fuel == "protium" else 345600,
        refl=0.1 * (1 + 5 * target[37]), forceful=target[24] * .01,
        rate0=.50625 * 2 ** target[38] / 2, capBoost=.05 * target[30],
        vr0=.1265 * (1 + target[25]) * 2 ** target[38],
        ventBoost=.05, vc0=1.265 * (1 + target[25]) * 2 ** target[39],
        sinkBoost=.05, ec0=10.125 * 2 ** target[39], reBoost=.05,
        ccap=2099 * (1 + target[23]) * 2 ** target[43],
        kcap=20e-9 * (1 + target[13]) * 2 ** target[36],
        hullPer=38.416 * (1 + target[14]) * 2 ** target[36],
        drain=.0001, sellB=38.416 * 2 ** target[36] / 2,
        sellLvl=1 + target[13], sellPct=target[15], bEx=1 + target[28],
        bAv=target[27], bHs=target[26], bRe=target[29], bCh=1 + target[22],
        simSteps=100000, plSteps=100000, faultT=5500, outFault=8, ventFault=5,
        budget=(1e19 - spent) / 1e15,
        f1=3 if fuel == "protium" else 288,
        f2=6.6 if fuel == "protium" else 633.6,
        f4=14.52 if fuel == "protium" else 1452,
        cE=.256, cO=.256, cI=.256, cV=.195312, cC=.8, cP=.65536, cK=.65536,
        cR=.000005, anMargin=.1, anSeed=seed, anThreads=4, anPopulation=128,
        anAttempts=12000, anVerify=160, anObjective=objective, anStrategy="blocks",
        anW=width, anH=height, anMinF=0, anMaxF=0, anA1=1, anA2=1, anA4=1,
        anFav=0, anFhs=0, anFex=0, anFre=0, anFch=0, simSkip=1,
    )
    params["expStart"] = params["fuelLife"]
    params["expEnd"] = 2 * params["fuelLife"]
    dest = folder / name
    dest.mkdir(exist_ok=True)
    seeds = dest / "seed"
    seeds.mkdir(exist_ok=True)
    params["anLibrary"] = str(seeds)
    params["anOutFile"] = str(dest / "best.txt")
    configurations = [
        [(0, 0, "4"), (width - 1, height - 1, "4")],
        [(0, 0, "4"), (1, 1, "4"), (2, 0, "2"), (0, 2, "2")],
        [(width // 2 - 1, height // 2 - 1, "2"), (width // 2, height // 2, "2")],
        [(0, 0, "2"), (2, 0, "2"), (1, 1, "2")],
        [(0, 0, "2"), (width - 1, 0, "2"), (width - 1, height - 1, "2")],
        [(0, 0, "4")],
        [(0, 0, "4"), (width - 1, height - 1, "2")],
    ]
    count = 0
    for config in configurations:
        for support in ("P", "R"):
            for phase in range(5):
                base = ["K"] * (width * height)
                for x, y, kind in config:
                    base[y * width + x] = kind
                for x, y, _ in config:
                    for index in neighbors(y * width + x):
                        if base[index] == "K":
                            base[index] = support
                if support == "R":
                    options = [i for i, c in enumerate(base) if c == "R"]
                    base[options[-1]] = "P"
                centers = [y * width + x for y in range(1, height - 1) for x in range(1, width - 1)
                           if (x + 2 * y) % 5 == phase and base[y * width + x] == "K"
                           and all(base[i] == "K" for i in neighbors(y * width + x))]
                rng.shuffle(centers)
                for outlets in range(1, min(45, len(centers)) + 1):
                    grid = base.copy()
                    for center in centers[:outlets]:
                        grid[center] = "O"
                        for index in neighbors(center):
                            grid[index] = "V"
                    cores = [int(c) for c in grid if c in "124"]
                    heat = sum({1: 1, 2: 8, 4: 36}[c] * params["fuelHeat"] for c in cores)
                    caps = grid.count("K")
                    cooling = grid.count("V") * params["vr0"] * (1 + .05 * params["bAv"] * caps)
                    if cooling < heat * 1.001:
                        continue
                    count += 1
                    filename = seeds / f"candidate-{count}.txt"
                    filename.write_text("0 0 0 0 0 0 0 0 0 " + str(seed) + "\n" +
                                        "\n".join("".join(grid[y * width:(y + 1) * width]) for y in range(height)) + "\n",
                                        encoding="utf-8")
    (dest / "parameters.txt").write_text("\n".join(f"{k}={v}" for k, v in params.items()) + "\n", encoding="utf-8")
    report[name] = dict(fuel=fuel, objective=objective, upgrades=buys, upgrade_cost=spent,
                        total_budget=1e19, hardware_and_fuel_budget=1e19 - spent,
                        levels=target, seeds=count, geometry="five-phase disjoint Outlet+4Vent lattice, seven fuel geometries, Plating/Reflector barriers",
                        seed=seed, density_counts_self=True)
    print(name, "upgrade_Qi", spent / 1e18, "seeds", count)
(folder / "profiles.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
