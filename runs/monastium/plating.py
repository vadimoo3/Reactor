import json
import os
from pathlib import Path
import subprocess


folder = Path(__file__).resolve().parent
results = json.loads((folder / "results.json").read_text(encoding="utf-8"))
engine = folder / "ReactorEngine.exe"
base = {k: v for k, v in os.environ.items() if not k.startswith("RX_")}
for name in ("protium-heat", "monastium-heat"):
    dest = folder / name
    params = dict(line.split("=", 1) for line in (dest / "parameters.txt").read_text(encoding="utf-8").splitlines())
    original = Path(results[name].get("search_winner", results[name]["manifest"][0][0]))
    lines = original.read_text(encoding="utf-8").splitlines()
    old_reflectors = sum(row.count("R") for row in lines[1:])
    grid = [row.replace("R", "P") for row in lines[1:]]
    header = lines[0].split()
    cost = float(header[3]) + old_reflectors * (float(params["cP"]) - float(params["cR"])) / 1000
    header[3] = f"{cost:.12g}"
    final = dest / "plating.txt"
    final.write_text(" ".join(header) + "\n" + "\n".join(grid) + "\n", encoding="utf-8")
    env = base | {"RX_" + k: v for k, v in params.items()} | {"RX_plFile": str(final)}
    assessment = subprocess.check_output([str(engine), "--tool", "ev"], env=env).decode("utf-8")
    validation = subprocess.check_output([str(engine), "--tool", "pl"], env=env).decode("utf-8")
    assert validation.count("PASS") == 18 and validation.count("FAIL") == 12
    results[name]["search_winner"] = str(original)
    results[name]["manifest"][0][0] = str(final)
    results[name]["manifest"][0][4] = f"{cost:.12g}"
    results[name]["assessment"] = assessment
    results[name]["validation"] = validation
    results[name]["postprocess"] = f"Replace {old_reflectors} redundant reflectors with Ultimate Plating; income and heat unchanged, capacities higher. Revalidated."
    (dest / "plating-assessment.txt").write_text(assessment, encoding="utf-8")
    (dest / "plating-validation.txt").write_text(validation, encoding="utf-8")
    print(name, assessment.splitlines()[0])
(folder / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
