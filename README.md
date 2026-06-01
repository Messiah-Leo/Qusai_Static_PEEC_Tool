# Quasi Static PEEC Tool

Quasi Static PEEC Tool is a Windows C++ toolset for extracting, reducing, converting, and solving quasi-static PEEC circuit models. The repository contains separate Visual Studio projects for the main workflow stages, plus small utilities for model conversion and matrix inspection.

## Modules

| Path | Project | Purpose |
| --- | --- | --- |
| `Quasi_Static_Model_Extract/` | `Quasi_Static_Model_Extract.sln` | Reads a Gmsh 2.2 mesh and material/settings files, builds the PEEC model, and writes matrix/model outputs. |
| `Quasi_Static_Solver/` | `Quasi_Static_Solver.slnx` | Reads a PEEC model and solves frequency-domain or time-domain responses. |
| `Post_Processing/` | `Post_Processing.slnx` | Converts solver snapshots into Gmsh source plots and optionally evaluates spatial fields on an input plane mesh. |
| `Quasi_Static_DPEC/` | `Sur_DPEC.sln` | Performs DPEC/MOR circuit reduction and writes the reduced model plus a reduction log. |
| `Tool_Model2Circuit/` | `Tool_Model2Circuit.slnx` | Converts PEEC matrix model data to a circuit/netlist representation. |
| `Quasi_Static_Model_Extract/Tools/BinToTxt/` | standalone | Converts PEEC binary matrix files (`PEECMAT1`) back to text matrices. |

## Requirements

- Windows 10 or later.
- Visual Studio with MSVC toolset `v145` support.
- C++17/C++20 support, depending on the project configuration.
- Intel oneAPI MKL installed and available to Visual Studio.
  - Several project files currently reference paths such as `D:\Intel\oneAPI\mkl\2025.0\include`.
  - If oneAPI is installed elsewhere, update the project include/library paths before building.

## Data Layout

Each runnable project expects a sibling `Data` directory. Typical files are:

- `model.msh`: Gmsh 2.2 mesh input.
- `set.txt`: solver/extraction settings.
- `DIELECTRIC.txt`: dielectric constants.
- `Source.txt` and `SourceConfig.txt`: time-domain solver source data.
- `LL_OO.txt`, `PP_OO.txt`, `B2N.txt`, `PORT.txt`: PEEC matrix/model files.
- `MOR_SET.txt`: DPEC reduction settings.

Generated outputs are usually written to `Data/output/` or `Data/Output/`, depending on the module.

## Typical Workflow

1. Build and run `Quasi_Static_Model_Extract`.
2. Check generated PEEC model files under `Quasi_Static_Model_Extract/Data/output/`.
3. Copy or point the solver/reduction module to the generated model files as needed.
4. Build and run `Quasi_Static_Solver` for frequency/time response.
5. Optionally build and run `Post_Processing` to export source or spatial field plots.
6. Optionally build and run `Sur_DPEC` to reduce the PEEC circuit model.
7. Optionally run `Tool_Model2Circuit` to export a PrimeSim HSPICE-style netlist.

## Pipeline Runner

Build and deploy the model extractor, solver, and post-processing executables from a Visual Studio developer PowerShell:

```powershell
.\Build_Pipeline.ps1
```

Run all three stages:

```powershell
.\Pipeline_Runtime\Run_Pipeline.ps1
```

The runtime input directory is `Pipeline_Runtime/Data/`. Generated matrices and snapshots are written under `Pipeline_Runtime/Data/COMMON_DATA/`; Gmsh source plots are written under `Pipeline_Runtime/Data/Source_Plot/`.

Add these optional keys before `$End_Def` in `Data/set.txt`:

```text
POST_PROCESSING 1
POST_FREQ_INDEX -1
POST_PORT       1
POST_PLOT_J     1
POST_PLOT_QE    1
POST_PLOT_VE    1
POST_PLOT_M     1
POST_PLOT_QM    1
POST_PLOT_VM    1
POST_FIELD      0
POST_FIELD_ER   1
```

`POST_FREQ_INDEX` is one based. Values less than or equal to zero select the last solved frequency point.

Set `POST_FIELD 1` and place a Gmsh 2.2 triangle mesh at `Data/Space_Mesh.msh` to generate spatial field distributions under `Data/Field_Plot/`.

## Settings

Example `set.txt`:

```text
Solver_SET      0
FS              1e9
FE              100e9
N_FP            500
DIM             1e3
ER              1
SAVE_TXT        1
$End_Def
```

Common fields:

- `Solver_SET`: `0` for frequency-domain solving, `1` for time-domain solving.
- `FS`, `FE`: start/end frequency.
- `N_FP`: number of frequency points.
- `DIM`: geometry scale factor.
- `ER`: relative permittivity setting.
- `SAVE_TXT`: when enabled in model extraction, saves large matrix outputs as text in addition to binary.

Example `MOR_SET.txt`:

```text
MOR_METHOD      DPEC
MAX_ERROR       0.1
MAX_FREQ        10e9
MAX_NODE        100
$End_Def
```

## Matrix Outputs

The model extractor writes large PEEC matrices in a compact binary format by default:

```text
PEECMAT1
uint64 rows
uint64 cols
double data[rows][cols]
```

`Quasi_Static_Solver` and `Sur_DPEC` can read these binary matrix files. If `SAVE_TXT` is enabled, text versions are also emitted for debugging and external inspection.

## BinToTxt Utility

Build from a Visual Studio Developer Command Prompt:

```bat
cd Quasi_Static_Model_Extract\Tools\BinToTxt
build.bat
```

Or with MinGW/MSYS2:

```bat
cd Quasi_Static_Model_Extract\Tools\BinToTxt
build_gcc.bat
```

Usage:

```bat
BinToTxt.exe ..\..\Data\Output\LL_OO.bin
BinToTxt.exe ..\..\Data\Output\PP_OO.bin ..\..\Data\Output\PP_OO.txt
```

If the output path is omitted, the utility writes a `.txt` file beside the input file.

## Build Notes

- Open the relevant `.sln` or `.slnx` file in Visual Studio and select the desired configuration/platform.
- Prefer `x64 Release` for large matrix workloads.
- The solver limits MKL threads during parallel frequency solving to reduce oversubscription.
- Build artifacts such as `x64/`, `Debug/`, `Release/`, `.obj`, `.pdb`, and `.exe` are ignored by Git.

## Current Caveats

- Project files use absolute oneAPI include paths; adjust them for your local installation.
- Some source comments/data contain mixed encodings from earlier edits. Use UTF-8 where possible for new files.
- Output directories may be named `output` or `Output` by different modules; keep paths consistent when moving data between stages.
