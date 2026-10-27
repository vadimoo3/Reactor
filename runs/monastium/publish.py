import collections
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess


root = Path(__file__).resolve().parents[2]
folder = Path(__file__).resolve().parent
engine = folder / "ReactorEngine.exe"
profiles = json.loads((folder / "profiles.json").read_text(encoding="utf-8"))
results = json.loads((folder / "results.json").read_text(encoding="utf-8"))
save = json.loads((root / "current save/current-save.json").read_text(encoding="utf-8"))
summary = {}
base = {k: v for k, v in os.environ.items() if not k.startswith("RX_")}


for name, result in results.items():
    dest = folder / name
    params = dict(line.split("=", 1) for line in (dest / "parameters.txt").read_text(encoding="utf-8").splitlines())
    target = profiles[name]["levels"]
    for key, index, cost, multiplier in [("pav", 27, 1000, 100), ("phs", 26, 1000, 100), ("pex", 28, 600, 100),
                                          ("pre", 29, 1000, 100), ("pch", 22, 10000, 100),
                                          ("pwi", 13, 5000, 5), ("pli", 15, 100, 10)]:
        params[key] = str(cost * multiplier ** target[index] / 1e15)
    source = Path(result["manifest"][0][0])
    lines = source.read_text(encoding="utf-8").splitlines()
    width, height = int(params["anW"]), int(params["anH"])
    grid = "".join(lines[1:])
    counts = collections.Counter(grid)
    p = {key: float(value) for key, value in params.items() if re.fullmatch(r"[-+]?\d+(?:\.\d*)?(?:[Ee][-+]?\d+)?", value)}
    core = lambda c: int(c) if c in "124" else 0

    def adjacent(index):
        x, y = index % width, index // width
        return [yy * width + xx for xx, yy in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1))
                if 0 <= xx < width and 0 <= yy < height]

    fuels, generated, heat = [], 0, 0
    for index, kind in enumerate(grid):
        cores = core(kind)
        if not cores:
            continue
        pulse = {1: 1, 2: 4, 4: 12}[cores] + sum(core(grid[j]) for j in adjacent(index))
        reflectors = sum(grid[j] == "R" for j in adjacent(index))
        neighbors = adjacent(index)
        assert all(grid[j] not in "EVCK" for j in neighbors), (name, index)
        x, y = index % width, index // width
        occupied = sum(grid[yy * width + xx] != "." for yy in range(max(0, y - 3), min(height, y + 4))
                       for xx in range(max(0, x - 3), min(width, x + 4)))
        density = 1 - .02 * occupied if profiles[name]["fuel"] == "monastium" else 1
        power = pulse * p["fuelPow"] * (1 + p["refl"] * reflectors) * density
        fuel_heat = pulse * pulse * p["fuelHeat"] / cores
        fuels.append(dict(row=y + 1, column=x + 1, cores=cores, pulse=pulse, reflectors=reflectors,
                          occupied_7x7=occupied, density=density, power_T=power / 1000,
                          heat_T=fuel_heat / 1000))
        generated += power
        heat += fuel_heat
    rate = p["rate0"] * p["bEx"] * 2 * (1 + p["capBoost"] * counts["K"])
    vent_rate = p["vr0"] * (1 + p["ventBoost"] * p["bAv"] * counts["K"])
    vent_capacity = p["vc0"] * (1 + p["sinkBoost"] * p["bHs"] * counts["P"])
    hull_capacity = p["hullPer"] * counts["P"]
    branches = sum(sum(grid[j] in "EVCK" for j in adjacent(i)) for i, c in enumerate(grid) if c == "O")
    assert not counts["E"] and not counts["I"] and not counts["C"]
    assert branches * rate >= heat
    vent_loads = []
    for index, kind in enumerate(grid):
        if kind != "V":
            continue
        degree = sum(grid[j] == "O" for j in adjacent(index))
        load = heat * degree / branches
        assert degree == 1 and load <= vent_rate
        vent_loads.append(dict(row=index // width + 1, column=index % width + 1,
                               outlets=degree, incoming_T=load / 1000,
                               cooling_T=vent_rate / 1000, storage_T=vent_capacity / 1000,
                               remaining_after_vent_T=max(0, load - vent_rate) / 1000))
    sale = p["sellB"] * p["sellLvl"] * 2 * .01 * p["sellPct"] * counts["K"]
    sold = min(generated, sale)
    fuel_price = sum(counts[str(c)] * p["f" + str(c)] for c in (1, 2, 4))
    parts_price = sum(counts[c] * p[key] for c, key in dict(E="cE", O="cO", I="cI", V="cV", C="cC", P="cP", K="cK", R="cR").items()) / 1000
    lifetime = p["fuelLife"]
    net = sold - fuel_price * 1e6 / lifetime
    assert abs(net - float(result["manifest"][0][2])) < 1e-4
    assert abs(generated - float(re.search(r"generated=([\deE.+-]+)", result["assessment"]).group(1))) < 1e-4
    assert result["validation"].count("PASS") == 18
    assert all("PASS" in line for line in result["validation"].splitlines()
               if any(key in line for key in ("штатно:", "замена топлива:", "выгорание:")))
    reflector_life = 2e6 * (1 + save["upgrade_levels"][31])
    reflector_times = [reflector_life / sum(core(grid[j]) for j in adjacent(i))
                       for i, c in enumerate(grid) if c == "R" and any(core(grid[j]) for j in adjacent(i))]
    assert not reflector_times or min(reflector_times) > lifetime
    target_heat = 10 ** (12 + math.log(save["store"]["total_exotic_particles"] + 5000, 4))
    extra_heat = target_heat - save["store"]["total_heat_dissipated"]
    ticks = math.ceil(extra_heat / (heat * 1e9))
    assert save["store"]["total_power_produced"] + generated * 1e9 * ticks >= target_heat
    purchased_cycles = math.ceil(ticks / lifetime)
    metrics = dict(fuel=profiles[name]["fuel"], goal=profiles[name]["objective"], counts=dict(counts),
                   fuels=fuels, generation_T=generated / 1000, sale_T=sold / 1000, net_T=net / 1000,
                   heat_T=heat / 1000, cooling_T=counts["V"] * vent_rate / 1000,
                   worst_vent_incoming_T=max(row["incoming_T"] for row in vent_loads),
                   individual_vent_rate_T=vent_rate / 1000, individual_vent_capacity_T=vent_capacity / 1000,
                   vent_margin=(vent_rate / max(row["incoming_T"] * 1000 for row in vent_loads) - 1),
                   outlet_rate_per_branch_T=rate / 1000, outlet_branches=branches,
                   hull_capacity_T=hull_capacity / 1000, steady_hull_heat_T=0,
                   fuel_cost_Qa=fuel_price, parts_cost_Qa=parts_price,
                   upgrade_cost_Qa=profiles[name]["upgrade_cost"] / 1e15,
                   total_build_cost_Qi=(profiles[name]["upgrade_cost"] / 1e15 + fuel_price + parts_price) / 1000,
                   fuel_life_ticks=lifetime, ticks_per_second=p["bCh"], fuel_life_seconds=lifetime / p["bCh"],
                   earliest_reflector_life_ticks=min(reflector_times) if reflector_times else None,
                   ticks_to_5000_EP_reward=ticks, seconds_to_5000_EP_reward=ticks / p["bCh"],
                   fuel_cycles_to_5000_EP_reward=purchased_cycles,
                   fuel_expense_to_5000_EP_reward_Qi=purchased_cycles * fuel_price / 1000,
                   money_sold_to_5000_EP_reward_Qi=sold * ticks / 1e9,
                   cash_minus_full_fuel_purchases_to_5000_EP_reward_Qi=sold * ticks / 1e9 - purchased_cycles * fuel_price / 1000,
                   actual_heat_goal_Qi=target_heat / 1e18, validation_passed=18, validation_failed_faults=12,
                   steps=max(p["simSteps"], p["expEnd"] + lifetime), vent_loads=vent_loads,
                   engine_sha256=hashlib.sha256(engine.read_bytes()).hexdigest())
    (dest / "metrics.json").write_text(json.dumps(metrics, indent=2), encoding="utf-8")
    env = base | {"RX_" + key: str(value) for key, value in params.items()}
    env.update(RX_plFile=str(source), RX_fuelLife="3000", RX_expStart="3000", RX_expEnd="6000", RX_simSteps="10000", RX_plSteps="10000")
    checks = {}
    for skip in (0, 1):
        env["RX_simSkip"] = str(skip)
        validation = subprocess.check_output([str(engine), "--tool", "pl"], env=env).decode("utf-8")
        output = subprocess.check_output([str(engine), "--tool", "ev"], env=env).decode("utf-8")
        checks[skip] = dict(validation=validation, assessment=output)
    assert checks[0]["validation"] == checks[1]["validation"], name
    a, b = [dict(re.findall(r"(\w+)=([-+\deE.]+)", checks[key]["assessment"].splitlines()[0])) for key in (0, 1)]
    for key in a:
        assert abs(float(a[key]) - float(b[key])) < max(1e-4, abs(float(a[key])) * 1e-10), (name, key)
    (dest / "skip-check.json").write_text(json.dumps(checks, indent=2), encoding="utf-8")
    published = root / "results/profit" / (name + ".txt")
    header = lines[0].split()
    header[0] = f"{generated:.12g}"
    model_keys = ["fuelName", "fuelHeat", "fuelPow", "fuelLife", "expStart", "expEnd", "refl", "forceful",
                  "rate0", "capBoost", "vr0", "ventBoost", "vc0", "sinkBoost", "ec0", "reBoost", "ccap", "kcap",
                  "hullPer", "drain", "sellB", "sellLvl", "sellPct", "bEx", "bAv", "bHs", "bRe", "bCh",
                  "budget", "f1", "f2", "f4", "cE", "cO", "cI", "cV", "cC", "cP", "cK", "cR", "anObjective"]
    model_keys += ["pav", "phs", "pex", "pre", "pch", "pwi", "pli"]
    published.write_text(" ".join(header) + "\n" + "\n".join(lines[1:]) + "\n" +
                         "\n".join("@" + key + "=" + params[key] for key in model_keys) + "\n", encoding="utf-8")
    summary[name] = {k: v for k, v in metrics.items() if k != "vent_loads"}
    summary[name]["file"] = str(published)
    print(name, "sale", round(sold / 1000, 6), "net", round(net / 1000, 6), "heat", heat / 1000,
          "cost_Qi", round(metrics["total_build_cost_Qi"], 6), "EP_minutes", round(metrics["seconds_to_5000_EP_reward"] / 60, 2), flush=True)
(folder / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
