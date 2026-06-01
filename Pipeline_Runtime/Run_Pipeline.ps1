param(
    [switch]$SkipModelExtract,
    [switch]$SkipSolver
)

$ErrorActionPreference = 'Stop'
$runtime = $PSScriptRoot

function Invoke-PipelineStage {
    param([string]$Executable)

    $path = Join-Path $runtime $Executable
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing pipeline stage: $path"
    }

    Write-Host "==> $Executable"
    & $path
    if ($LASTEXITCODE -ne 0) {
        throw "$Executable failed with exit code $LASTEXITCODE"
    }
}

if (-not $SkipModelExtract) {
    Invoke-PipelineStage 'Quasi_Static_Model_Extract.exe'
}
if (-not $SkipSolver) {
    Invoke-PipelineStage 'Quasi_Static_Solver.exe'
}
Invoke-PipelineStage 'Post_Processing.exe'

Write-Host 'Pipeline finished successfully.'
