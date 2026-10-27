import concurrent.futures
import json
import hashlib
import os
from pathlib import Path
import subprocess
import shutil
import time


root = Path(__file__).resolve().parents[2]
folder = Path(__file__).resolve().parent
engine = folder / "ReactorEngine.exe"
shutil.copyfile(root / "runs/build/ReactorEngine.exe", engine)
(folder / "engine.sha256").write_text(hashlib.sha256(engine.read_bytes()).hexdigest() + "\n", encoding="ascii")
base = {key: value for key, value in os.environ.items() if not key.startswith("RX_")}
raw = subprocess.check_output([str(engine), "--params"], env=base).decode("utf-8")
defaults = {parts[2]: parts[4] for line in raw.splitlines() if (parts := line.split("\t"))[0] == "P"}
profiles = json.loads((folder / "profiles.json").read_text(encoding="utf-8"))


def run(name):
    dest = folder / name
    params = defaults | dict(line.split("=", 1) for line in (dest / "parameters.txt").read_text(encoding="utf-8").splitlines())
    params["anThreads"] = "1"
    (dest / "parameters.txt").write_text("\n".join(f"{k}={v}" for k, v in params.items()) + "\n", encoding="utf-8")
    env = base | {"RX_" + key: str(value) for key, value in params.items()}
    start = time.monotonic()
    with (dest / "stdout.txt").open("w", encoding="utf-8") as out, (dest / "stderr.txt").open("w", encoding="utf-8") as err:
        proc = subprocess.run([str(engine), "--tool", "an"], cwd=dest, env=env, stdout=out, stderr=err, timeout=300)
    elapsed = time.monotonic() - start
    manifest = [line.split("\t") for line in (dest / "best.top.tsv").read_text(encoding="utf-8").splitlines()]
    winner = Path(manifest[0][0])
    env["RX_plFile"] = str(winner)
    assessment = subprocess.check_output([str(engine), "--tool", "ev"], cwd=dest, env=env).decode("utf-8")
    (dest / "assessment.txt").write_text(assessment, encoding="utf-8")
    validation = subprocess.check_output([str(engine), "--tool", "pl"], cwd=dest, env=env).decode("utf-8")
    (dest / "validation.txt").write_text(validation, encoding="utf-8")
    print(name, "seconds", round(elapsed, 2), "exit", proc.returncode, "winner", manifest[0], flush=True)
    print(assessment.splitlines()[0], flush=True)
    return name, dict(seconds=elapsed, manifest=manifest, assessment=assessment,
                      validation=validation, exit_code=proc.returncode,
                      upgrade_cost=profiles[name]["upgrade_cost"])


with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
    results = dict(executor.map(run, profiles))
(folder / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
