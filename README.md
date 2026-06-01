# Quasi-Static PEEC Tool

用于准静态 Partial Element Equivalent Circuit（PEEC）建模、求解、降阶、网表导出和结果后处理的 Windows C++ 工具集。

仓库包含一条可直接运行的三阶段主流水线，也保留了独立的 DPEC 降阶与模型转电路工具。示例网格、矩阵和配置文件用于复现实验；编译产物和运行输出不会提交到 Git。

## 功能概览

| 模块 | 入口 | 作用 |
| --- | --- | --- |
| `Quasi_Static_Model_Extract/` | `Quasi_Static_Model_Extract.sln` | 从 Gmsh 2.2 网格和材料配置提取 PEEC 模型。 |
| `Quasi_Static_Solver/` | `Quasi_Static_Solver.slnx` | 执行频域或时域求解，并导出端口与源快照。 |
| `Post_Processing/` | `Post_Processing.slnx` | 将源快照转换为 Gmsh 绘图文件，并可计算空间 `E`、`H` 场。 |
| `Quasi_Static_DPEC/` | `Sur_DPEC.sln` | 使用 DPEC 方法进行模型降阶。 |
| `Tool_Model2Circuit/` | `Tool_Model2Circuit.slnx` | 将 PEEC 矩阵模型转换为电路网表。 |
| `Quasi_Static_Model_Extract/Tools/BinToTxt/` | `build.bat` | 将 PEEC 二进制矩阵转换为文本矩阵。 |

## 环境要求

- Windows 10 或更高版本。
- Visual Studio，包含 MSVC `v145` 工具集和 MSBuild。
- Intel oneAPI MKL。
- PowerShell 5.1 或更高版本。

MKL 相关项目统一从根目录的 `BuildConfiguration.props` 读取路径。解析顺序如下：

1. 显式传入的 MSBuild 属性，例如 `/p:PeecOneApiRoot=...`。
2. 环境变量：`ONEAPI_ROOT`、`MKLROOT`、`CMPLR_ROOT`。
3. 默认路径：`D:\Intel\oneAPI`，并使用 `mkl\latest` 与 `compiler\latest`。

如果 oneAPI 安装在其他位置，无需逐个修改 `.vcxproj` 文件。

## 快速开始

### 构建并运行主流水线

在 Visual Studio Developer PowerShell 中执行：

```powershell
.\Build_Pipeline.ps1
.\Pipeline_Runtime\Run_Pipeline.ps1
```

如需指定 oneAPI 根目录：

```powershell
.\Build_Pipeline.ps1 -PeecOneApiRoot C:\Intel\oneAPI
```

主流水线依次运行：

1. `Quasi_Static_Model_Extract.exe`
2. `Quasi_Static_Solver.exe`
3. `Post_Processing.exe`

构建脚本会将三个可执行文件部署到 `Pipeline_Runtime/`。运行输入位于 `Pipeline_Runtime/Data/`。

三个可执行文件的详细输入输出说明见 [`Pipeline_Runtime/README.md`](Pipeline_Runtime/README.md)。

### 构建所有模块

```powershell
.\Build_All.ps1
```

该命令会构建主流水线、DPEC 降阶工具和模型转电路工具。默认配置为 `Release|x64`。

## 仓库结构

```text
.
|-- BuildConfiguration.props     # 统一 oneAPI / MKL 路径
|-- Build_Pipeline.ps1           # 构建并部署三阶段流水线
|-- Build_All.ps1                # 构建全部 Visual Studio 项目
|-- scripts/
|   `-- Build.Common.ps1         # 共享 MSBuild 定位与调用逻辑
|-- Pipeline_Runtime/
|   |-- Run_Pipeline.ps1         # 流水线运行入口
|   `-- Data/                    # 流水线输入与生成结果
|-- Quasi_Static_Model_Extract/
|-- Quasi_Static_Solver/
|-- Post_Processing/
|-- Quasi_Static_DPEC/
|-- Tool_Model2Circuit/
`-- docs/
    `-- DEVELOPMENT.md           # 开发、构建和提交约定
```

## 数据约定

各模块通常通过相邻的 `Data/` 目录读取输入。

| 文件 | 用途 |
| --- | --- |
| `model.msh` | Gmsh 2.2 网格。 |
| `set.txt` | 模型提取、求解和后处理配置。 |
| `DIELECTRIC.txt` | 介质参数。 |
| `Source.txt`、`SourceConfig.txt` | 时域求解源配置。 |
| `LL_OO.*`、`PP_OO.*`、`B2N.txt`、`PORT.txt` | PEEC 矩阵与端口数据。 |
| `MOR_SET.txt` | DPEC 降阶配置。 |

仓库中已有的网格与矩阵文件是可复现样例。以下目录属于运行时生成结果，已加入 `.gitignore`：

```text
Pipeline_Runtime/Data/COMMON_DATA/
Pipeline_Runtime/Data/Source_Plot/
Pipeline_Runtime/Data/Field_Plot/
Quasi_Static_Model_Extract/Data/output/
Quasi_Static_Model_Extract/Data/Output/
Quasi_Static_Solver/Data/Output/
Quasi_Static_DPEC/Data/output/
```

## 主配置示例

`Pipeline_Runtime/Data/set.txt` 中常用参数：

```text
Solver_SET      0
FS              1e9
FE              10e9
N_FP            100
DIM             1e3
ER              1
SAVE_BIN        1
SAVE_TXT        0
SAVE_SP         0
POST_PROCESSING 1
POST_FREQ_INDEX -1
POST_PORT       1
POST_PLOT_J     1
POST_PLOT_QE    1
POST_PLOT_VE    1
POST_FIELD      0
POST_FIELD_ER   1
$End_Def
```

关键字段：

- `Solver_SET`: `0` 表示频域求解，`1` 表示时域求解。
- `FS`、`FE`、`N_FP`: 起始频率、终止频率和采样点数。
- `DIM`: 几何尺度因子。
- `ER`: 相对介电常数。
- `SAVE_BIN`、`SAVE_TXT`: 控制矩阵输出格式。
- `POST_FREQ_INDEX`: 从 `1` 开始计数；小于等于 `0` 时选择最后一个频点。
- `POST_PORT`: 后处理选用的端口编号。

## 后处理输出

启用 `POST_PROCESSING 1` 后，流水线会在 `Pipeline_Runtime/Data/Source_Plot/` 生成 Gmsh 绘图文件，包括 `J`、`M`、`Qe`、`Qm`、`Ve` 和 `Vm`。

当前 PEC 准静态求解器输出的磁源快照为零，因此 `M`、`Qm` 和 `Vm` 文件会保留，但数值为零。

如需计算空间场：

1. 设置 `POST_FIELD 1`。
2. 将 Gmsh 2.2 三角形平面网格放到 `Pipeline_Runtime/Data/Space_Mesh.msh`。
3. 运行流水线。

空间场输出位于 `Pipeline_Runtime/Data/Field_Plot/`：

```text
E_vec.msh_Re
E_vec.msh_Im
E_scalar.msh_Mag
H_vec.msh_Re
H_vec.msh_Im
H_scalar.msh_Mag
```

## 二进制矩阵格式

模型提取器默认使用紧凑二进制格式保存大型矩阵：

```text
PEECMAT1
uint64 rows
uint64 cols
double data[rows][cols]
```

转换为文本：

```bat
cd Quasi_Static_Model_Extract\Tools\BinToTxt
build.bat
BinToTxt.exe ..\..\Data\Output\LL_OO.bin
```

## 开发约定

- 不提交 `.vs/`、`x64/`、`.vcxproj.user`、`.exe` 或运行时输出。
- 机器相关 oneAPI 路径放在环境变量或 MSBuild 属性中。
- 修改后至少运行：

```powershell
git diff --check
.\Build_Pipeline.ps1
.\Build_All.ps1
```

更完整的说明见 [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md)。

## 已知事项

- 部分早期源码注释和样例数据仍包含混合编码。
- 历史模块对 `output/` 与 `Output/` 的大小写使用不完全一致；新代码应遵循模块现有约定。
- 现有数值代码仍会产生部分类型转换警告，构建脚本不会将警告视为错误。
