param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$runtime = Join-Path $root 'Pipeline_Runtime'
$msbuildCommand = Get-Command msbuild.exe -ErrorAction SilentlyContinue
if ($msbuildCommand) {
    $msbuild = $msbuildCommand.Source
}
else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw 'Cannot find msbuild.exe or vswhere.exe.'
    }
    $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    if (-not $msbuild) {
        throw 'Cannot find msbuild.exe through vswhere.exe.'
    }
}

function Invoke-Build {
    param([string]$Solution)

    & $msbuild $Solution /m /t:Build "/p:Configuration=$Configuration" /p:Platform=x64
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed: $Solution"
    }
}

Invoke-Build (Join-Path $root 'Quasi_Static_Model_Extract\Quasi_Static_Model_Extract.sln')
Invoke-Build (Join-Path $root 'Quasi_Static_Solver\Quasi_Static_Solver.slnx')
Invoke-Build (Join-Path $root 'Post_Processing\Post_Processing.slnx')

Copy-Item -LiteralPath (Join-Path $root "Quasi_Static_Model_Extract\x64\$Configuration\Quasi_Static_Model_Extract.exe") -Destination $runtime -Force
Copy-Item -LiteralPath (Join-Path $root "Quasi_Static_Solver\x64\$Configuration\Quasi_Static_Solver.exe") -Destination $runtime -Force
Copy-Item -LiteralPath (Join-Path $root "Post_Processing\x64\$Configuration\Post_Processing.exe") -Destination $runtime -Force

Write-Host "Built and deployed $Configuration pipeline executables to $runtime"
