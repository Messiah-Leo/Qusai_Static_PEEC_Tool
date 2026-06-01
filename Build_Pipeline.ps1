param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [string]$PeecOneApiRoot
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$runtime = Join-Path $root 'Pipeline_Runtime'
. (Join-Path $root 'scripts\Build.Common.ps1')

if ($PeecOneApiRoot) {
    $env:ONEAPI_ROOT = $PeecOneApiRoot
}

$msbuild = Get-PeecMSBuild
Invoke-PeecBuild -MSBuild $msbuild -Solution (Join-Path $root 'Quasi_Static_Model_Extract\Quasi_Static_Model_Extract.sln') -Configuration $Configuration
Invoke-PeecBuild -MSBuild $msbuild -Solution (Join-Path $root 'Quasi_Static_Solver\Quasi_Static_Solver.slnx') -Configuration $Configuration
Invoke-PeecBuild -MSBuild $msbuild -Solution (Join-Path $root 'Post_Processing\Post_Processing.slnx') -Configuration $Configuration

New-Item -ItemType Directory -Path $runtime -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root "Quasi_Static_Model_Extract\x64\$Configuration\Quasi_Static_Model_Extract.exe") -Destination $runtime -Force
Copy-Item -LiteralPath (Join-Path $root "Quasi_Static_Solver\x64\$Configuration\Quasi_Static_Solver.exe") -Destination $runtime -Force
Copy-Item -LiteralPath (Join-Path $root "Post_Processing\x64\$Configuration\Post_Processing.exe") -Destination $runtime -Force

Write-Host "Built and deployed $Configuration pipeline executables to $runtime"
