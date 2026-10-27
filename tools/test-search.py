import json
import os
from pathlib import Path
import subprocess
import sys
import uuid


root = Path(__file__).resolve().parents[1]
engine = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / "runs/build/ReactorEngine.exe"
suite = root / "runs" / ("search-" + uuid.uuid4().hex[:12])
suite.mkdir()
base = {key: value for key, value in os.environ.items() if not key.startswith("RX_")}
defaults = dict(anAttempts=360, anSecs=30, anSeed=20261027, anThreads=2,
                anVerify=48, anMinF=6, anMaxF=6, anA1=1, anA2=0, anA4=0,
                anFav=1, anFhs=0, anFex=0, anFre=0, anFch=0,
                anLibrary=str(root / "results/profit"))
results = []


def run(name, changes=None, expected=0):
    folder = suite / name
    folder.mkdir()
    settings = defaults | (changes or {})
    env = base | {"RX_" + key: str(value) for key, value in settings.items()}
    process = subprocess.run([str(engine), "--tool", "an"], cwd=folder, env=env,
                             capture_output=True, encoding="utf-8", timeout=120)
    (folder / "stdout.txt").write_text(process.stdout, encoding="utf-8")
    (folder / "stderr.txt").write_text(process.stderr, encoding="utf-8")
    (folder / "parameters.json").write_text(json.dumps(settings, indent=2), encoding="utf-8")
    assert process.returncode == expected, (name, process.returncode, process.stderr)
    if expected:
        assert "OVER_BUDGET" in process.stderr
        assert not list(folder.glob("best*.txt"))
        return folder, {}, []
    stats = dict(line.split("=", 1) for line in (folder / "stats.txt").read_text(encoding="utf-8").splitlines())
    manifest = [line.split("\t") for line in (folder / "best.top.tsv").read_text(encoding="utf-8").splitlines()]
    assert int(stats["attempts"]) == int(settings["anAttempts"]), stats
    for fields in manifest:
        layout = (folder / fields[0]).read_text(encoding="utf-8").splitlines()
        grid = "".join(layout[1:])
        assert len(layout[1:]) == int(settings.get("anH", 16))
        assert all(len(row) == int(settings.get("anW", 19)) for row in layout[1:])
        assert sum(c in "124" for c in grid) == int(settings["anMinF"])
        assert all(c not in grid for c, flag in [("1", "anA1"), ("2", "anA2"), ("4", "anA4")] if not int(settings[flag]))
        assert float(fields[4]) <= float(settings.get("budget", 19.4504787631017))
        flags = list(map(int, layout[0].split()[4:9]))
        assert all(not value or int(settings[key]) for value, key in zip(flags, ["anFav", "anFhs", "anFex", "anFre", "anFch"]))
    return folder, stats, manifest


first, stats, manifest = run("historical")
assert manifest and max(float(row[2]) for row in manifest) >= 21028.0
operators = (first / "operators.tsv").read_text(encoding="utf-8").splitlines()[1:]
assert len(operators) == 12 and all(int(line.split("\t")[1]) > 0 for line in operators)
results.append({"test": "historical winner and all 12 operators", "best_T": float(stats["best_net_B"]) / 1000})
second, replay, again = run("replay")
assert stats == replay
assert manifest == again
for row in manifest:
    assert (first / row[0]).read_bytes() == (second / row[0]).read_bytes()
results.append({"test": "same seed and attempts reproduce results", "pass": True})
third, serial_stats, serial_manifest = run("serial", dict(anThreads=1))
assert stats == serial_stats and manifest == serial_manifest
results.append({"test": "parallel validation preserves deterministic results", "pass": True})

small = dict(anLibrary="", anAttempts=480, anMinF=9, anMaxF=9, anW=8, anH=7,
             fuelHeat=0.01, fuelPow=1, f1=0.5, fuelLife=96, expStart=96,
             expEnd=192, simSteps=256, faultT=128, anVerify=36,
             anFav=0, anFhs=0, anFex=0, anFre=0, anFch=0)
_, other, schemes = run("rectangular", small)
assert schemes, other
results.append({"test": "8x7 with exactly nine Single, no upgrades", "pass": True})
_, _, _ = run("over-budget", dict(anLibrary="", anMinF=6, anMaxF=6, budget=1), expected=3)
results.append({"test": "impossible fuel request rejected", "pass": True})
_, _, quad_schemes = run("quad-only", dict(small, anMinF=2, anMaxF=2, anA1=0, anA4=1, f4=1))
assert quad_schemes
results.append({"test": "Quad-only constraints", "pass": True})
(suite / "report.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
print(json.dumps({"directory": str(suite), "results": results}, indent=2))
