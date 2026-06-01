function Get-PeecMSBuild {
    $msbuildCommand = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if ($msbuildCommand) {
        return $msbuildCommand.Source
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw 'Cannot find msbuild.exe or vswhere.exe.'
    }

    $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' |
        Select-Object -First 1
    if (-not $msbuild) {
        throw 'Cannot find msbuild.exe through vswhere.exe.'
    }

    return $msbuild
}

function Invoke-PeecBuild {
    param(
        [Parameter(Mandatory = $true)]
        [string]$MSBuild,

        [Parameter(Mandatory = $true)]
        [string]$Solution,

        [ValidateSet('Debug', 'Release')]
        [string]$Configuration = 'Release',

        [string]$Platform = 'x64'
    )

    Write-Host "==> Building $Solution ($Configuration|$Platform)"
    & $MSBuild $Solution /m /t:Build "/p:Configuration=$Configuration" "/p:Platform=$Platform"
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed: $Solution"
    }
}
