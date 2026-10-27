param(
    [Parameter(Mandatory=$true)][string]$Tool,
    [string]$ParameterFile,
    [string]$Application,
    [hashtable]$Overrides = @{},
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (!$OutputDirectory) { $OutputDirectory = Join-Path $root ('runs/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + $Tool) }
$dir = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $dir | Out-Null
if (!$Application) { $Application = Join-Path $root 'ReactorLab.exe' }
$assembly = [Reflection.Assembly]::Load([IO.File]::ReadAllBytes((Resolve-Path $Application)))
$stream = $assembly.GetManifestResourceStream('ReactorEngine.exe')
$engine = Join-Path $dir 'ReactorEngine.exe'
$file = [IO.File]::Create($engine)
try { $stream.CopyTo($file) } finally { $file.Dispose(); $stream.Dispose() }
$params = @{}
& $engine --params | ForEach-Object {
    $fields = $_ -split "`t"
    if ($fields[0] -eq 'P') { $params[$fields[2]] = $fields[4] }
}
if ($ParameterFile) {
    foreach ($line in [IO.File]::ReadAllLines((Resolve-Path $ParameterFile))) {
        if ($line -match '^([^=]+)=(.*)$') { $params[$Matches[1]] = $Matches[2] }
    }
}
foreach ($key in $Overrides.Keys) { $params[$key] = [string]$Overrides[$key] }
$params.GetEnumerator() | Sort-Object Name | ForEach-Object { $_.Key + '=' + $_.Value } | Set-Content (Join-Path $dir 'parameters.txt') -Encoding utf8
$start = New-Object Diagnostics.ProcessStartInfo
$start.FileName = $engine
$start.Arguments = '--tool ' + $Tool
$start.WorkingDirectory = $dir
$start.UseShellExecute = $false
$start.CreateNoWindow = $true
$start.RedirectStandardOutput = $true
$start.RedirectStandardError = $true
$start.StandardOutputEncoding = [Text.Encoding]::UTF8
$start.StandardErrorEncoding = [Text.Encoding]::UTF8
@($start.EnvironmentVariables.Keys) | Where-Object { $_ -like 'RX_*' } | ForEach-Object { $start.EnvironmentVariables.Remove($_) }
foreach ($key in $params.Keys) { $start.EnvironmentVariables['RX_' + $key] = $params[$key] }
@{ tool=$Tool; engineHash=(Get-FileHash $engine -Algorithm SHA256).Hash; parameterFile=$ParameterFile; started=(Get-Date -Format o); model='local simulator, not original game' } | ConvertTo-Json | Set-Content (Join-Path $dir 'run.json') -Encoding utf8
$process = New-Object Diagnostics.Process
$process.StartInfo = $start
$null = $process.Start()
try { $process.PriorityClass = 'BelowNormal' } catch {}
$stdout = $process.StandardOutput.ReadToEndAsync()
$stderr = $process.StandardError.ReadToEndAsync()
$process.WaitForExit()
[IO.File]::WriteAllText((Join-Path $dir 'stdout.txt'), $stdout.Result)
[IO.File]::WriteAllText((Join-Path $dir 'stderr.txt'), $stderr.Result)
[IO.File]::WriteAllText((Join-Path $dir 'exit-code.txt'), [string]$process.ExitCode)
Write-Output $stdout.Result
Write-Output $stderr.Result
Write-Output ('Saved: ' + $dir)
if ($process.ExitCode -ne 0) { throw ('Engine exit code: ' + $process.ExitCode) }
