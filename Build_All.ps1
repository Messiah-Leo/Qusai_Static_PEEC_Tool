param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [string]$PeecOneApiRoot
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. (Join-Path $root 'scripts\Build.Common.ps1')

if ($PeecOneApiRoot) {
    $env:ONEAPI_ROOT = $PeecOneApiRoot
}

$msbuild = Get-PeecMSBuild
$solutions = @(
    'Quasi_Static_Model_Extract\Quasi_Static_Model_Extract.sln',
    'Quasi_Static_Solver\Quasi_Static_Solver.slnx',
    'Post_Processing\Post_Processing.slnx',
    'Quasi_Static_DPEC\Sur_DPEC.sln',
    'Tool_Model2Circuit\Tool_Model2Circuit.slnx'
)

foreach ($solution in $solutions) {
    Invoke-PeecBuild -MSBuild $msbuild -Solution (Join-Path $root $solution) -Configuration $Configuration
}

Write-Host "Built all $Configuration x64 projects successfully."
