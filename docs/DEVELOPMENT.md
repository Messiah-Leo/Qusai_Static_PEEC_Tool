# Development Guide

## Build Configuration

The MKL-dependent projects import `BuildConfiguration.props` from the repository root. The file resolves oneAPI paths in this order:

1. MSBuild properties passed explicitly, such as `/p:PeecOneApiRoot=...`.
2. oneAPI environment variables: `ONEAPI_ROOT`, `MKLROOT`, and `CMPLR_ROOT`.
3. The default installation root: `D:\Intel\oneAPI`, using the `latest` MKL and compiler directories.

Override the default root without editing project files:

```powershell
.\Build_All.ps1 -Configuration Release -PeecOneApiRoot C:\Intel\oneAPI
```

For direct MSBuild usage, pass the property explicitly:

```powershell
msbuild .\Quasi_Static_Model_Extract\Quasi_Static_Model_Extract.sln /p:Configuration=Release /p:Platform=x64 /p:PeecOneApiRoot=C:\Intel\oneAPI
```

## Build Entry Points

Build every project:

```powershell
.\Build_All.ps1
```

Build the three-stage runtime pipeline and deploy executables into `Pipeline_Runtime/`:

```powershell
.\Build_Pipeline.ps1
```

## Repository Hygiene

- Do not commit `.vcxproj.user`, `.vs/`, `x64/`, or executable files.
- Treat `Data/Output/`, `Data/output/`, `Pipeline_Runtime/Data/COMMON_DATA/`, `Pipeline_Runtime/Data/Source_Plot/`, and `Pipeline_Runtime/Data/Field_Plot/` as generated output directories.
- Keep the tracked matrix and mesh files already present in module `Data/` directories as reproducible fixtures unless a deliberate fixture refresh is needed.
- Place machine-specific oneAPI paths in environment variables or MSBuild properties rather than project files.

## Validation

Before publishing changes:

```powershell
git diff --check
.\Build_Pipeline.ps1
.\Build_All.ps1
```
