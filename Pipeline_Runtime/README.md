# Pipeline Runtime 输入输出说明

`Pipeline_Runtime/` 是三阶段准静态 PEEC 流水线的部署目录。执行根目录的 `Build_Pipeline.ps1` 后，以下程序会被复制到此目录：

```text
Quasi_Static_Model_Extract.exe
Quasi_Static_Solver.exe
Post_Processing.exe
```

三个程序默认使用 `Pipeline_Runtime/Data/` 作为数据根目录。建议通过 `Run_Pipeline.ps1` 按顺序运行。

## 快速运行

```powershell
.\Run_Pipeline.ps1
```

等价于依次执行：

```powershell
.\Quasi_Static_Model_Extract.exe
.\Quasi_Static_Solver.exe
.\Post_Processing.exe
```

可按需复用已有中间结果：

```powershell
.\Run_Pipeline.ps1 -SkipModelExtract
.\Run_Pipeline.ps1 -SkipModelExtract -SkipSolver
```

| 参数 | 行为 |
| --- | --- |
| `-SkipModelExtract` | 跳过模型提取，复用 `Data/COMMON_DATA/` 中已有的 PEEC 矩阵和 `Post_Model.txt`。 |
| `-SkipSolver` | 跳过求解器，复用已有的 `Data/COMMON_DATA/Post_Source.txt`。 |

跳过阶段前应确认中间文件来自同一套模型和配置。

## 数据目录

```text
Data/
|-- model.msh
|-- set.txt
|-- DIELECTRIC.txt              # 可选
|-- Space_Mesh.msh              # POST_FIELD=1 时需要
|-- COMMON_DATA/                # 模型提取和求解器中间结果
|-- Source_Plot/                # 后处理源分布输出
`-- Field_Plot/                 # 可选空间场输出
```

`COMMON_DATA/`、`Source_Plot/` 和 `Field_Plot/` 是运行时生成目录，已被 Git 忽略。

## 1. Quasi_Static_Model_Extract.exe

作用：读取 Gmsh 网格，建立准静态 PEEC 模型，并生成求解器需要的矩阵文件。

直接运行：

```powershell
.\Quasi_Static_Model_Extract.exe
```

### 输入

| 路径 | 必需 | 说明 |
| --- | --- | --- |
| `Data/model.msh` | 是 | Gmsh 2.2 网格。当前仅支持版本 `2.2`。 |
| `Data/set.txt` | 否 | 模型提取设置。缺失时使用程序默认值。 |
| `Data/DIELECTRIC.txt` | 否 | 介质复数参数。缺失时使用空气介质默认值。 |

模型提取器读取的常用 `set.txt` 字段：

| 字段 | 说明 |
| --- | --- |
| `FS`、`FE`、`N_FP`、`PRECISION` | 频率范围和采样设置。 |
| `DIM` | 网格坐标缩放因子。 |
| `SAVE_TXT` | 非零时额外写出文本矩阵。 |

### 输出

| 路径 | 条件 | 说明 |
| --- | --- | --- |
| `Data/COMMON_DATA/LL_OO.bin` | 总是生成 | 电感矩阵，PEEC 二进制格式。 |
| `Data/COMMON_DATA/PP_OO.bin` | 总是生成 | 电势系数矩阵，PEEC 二进制格式。 |
| `Data/COMMON_DATA/B2N.txt` | 总是生成 | 支路到节点的连接矩阵。 |
| `Data/COMMON_DATA/PORT.txt` | 总是生成 | 端口数量、端口节点和参考阻抗。 |
| `Data/COMMON_DATA/Post_Model.txt` | 总是生成 | 后处理器使用的几何与连接快照。 |
| `Data/COMMON_DATA/LL_OO.txt` | `SAVE_TXT != 0` | 文本格式电感矩阵。 |
| `Data/COMMON_DATA/PP_OO.txt` | `SAVE_TXT != 0` | 文本格式电势系数矩阵。 |

## 2. Quasi_Static_Solver.exe

作用：读取 PEEC 矩阵并执行频域或时域求解。当前流水线默认使用频域模式：`Solver_SET 0`。

直接运行：

```powershell
.\Quasi_Static_Solver.exe
```

### 公共输入

| 路径 | 必需 | 说明 |
| --- | --- | --- |
| `Data/set.txt` | 否 | 求解设置。缺失时使用程序默认值。 |
| `Data/COMMON_DATA/LL_OO.bin` 或 `LL_OO.txt` | 是 | 电感矩阵。优先读取二进制文件。 |
| `Data/COMMON_DATA/PP_OO.bin` 或 `PP_OO.txt` | 是 | 电势系数矩阵。优先读取二进制文件。 |
| `Data/COMMON_DATA/B2N.txt` | 是 | 支路到节点的连接矩阵。 |
| `Data/COMMON_DATA/PORT.txt` | 是 | 端口定义。 |
| `Data/DIELECTRIC.txt` | 否 | 介质参数。缺失时使用空气介质默认值。 |

频域模式常用 `set.txt` 字段：

| 字段 | 说明 |
| --- | --- |
| `Solver_SET` | `0` 表示频域求解；`1` 表示时域求解。 |
| `FS`、`FE`、`N_FP`、`PRECISION` | 频点生成参数。 |
| `POST_PROCESSING` | 非零时导出后处理源快照。 |
| `POST_FREQ_INDEX` | 后处理快照频点，从 `1` 开始计数；小于等于 `0` 时使用最后一个频点。 |
| `POST_PORT` | 后处理快照使用的激励端口，从 `1` 开始计数。 |

### 频域输出

| 路径 | 条件 | 说明 |
| --- | --- | --- |
| `Data/COMMON_DATA/map.txt` | `Solver_SET = 0` | S 参数幅值和相位。 |
| `Data/COMMON_DATA/Z_in.txt` | `Solver_SET = 0` | 端口阻抗矩阵。 |
| `Data/COMMON_DATA/Y_in.txt` | `Solver_SET = 0` | 端口导纳矩阵。 |
| `Data/COMMON_DATA/Post_Source.txt` | `Solver_SET = 0` 且 `POST_PROCESSING != 0` | 后处理器使用的电流、电压和频点快照。 |

当前求解器导出的磁流与磁压快照为零，因此后处理器生成的 `M`、`Qm` 和 `Vm` 图也会为零。

### 时域模式现状

仓库中保留了时域求解代码和以下文件格式：

```text
Data/SourceConfig.txt
Data/Source.txt
Data/output/full_states.csv
```

但当前 `Quasi_Static_Solver.exe` 尚未在主入口中自动调用波形读取、生成与初始化流程。`Solver_SET 1` 暂不属于可直接运行的流水线能力，不应作为稳定接口使用。

## 3. Post_Processing.exe

作用：读取模型与源快照，生成可在 Gmsh 中查看的源分布图；可选计算空间 `E`、`H` 场。

直接运行：

```powershell
.\Post_Processing.exe
```

指定其他数据目录：

```powershell
.\Post_Processing.exe --data-root C:\path\to\Data
```

运行内置自检：

```powershell
.\Post_Processing.exe --self-test
```

### 输入

| 路径 | 必需 | 说明 |
| --- | --- | --- |
| `Data/set.txt` | 否 | 后处理开关。缺失时使用默认设置。 |
| `Data/COMMON_DATA/Post_Model.txt` | `POST_PROCESSING != 0` 时必需 | 模型提取器生成的几何与连接快照。 |
| `Data/COMMON_DATA/Post_Source.txt` | `POST_PROCESSING != 0` 时必需 | 求解器生成的源快照。 |
| `Data/Space_Mesh.msh` | `POST_FIELD != 0` 时必需 | Gmsh 2.2 三角形平面网格，用于空间场采样。 |

后处理设置：

| 字段 | 说明 |
| --- | --- |
| `POST_PROCESSING` | `0` 时跳过后处理。 |
| `POST_PLOT_J`、`POST_PLOT_QE`、`POST_PLOT_VE` | 控制电源分布图输出。 |
| `POST_PLOT_M`、`POST_PLOT_QM`、`POST_PLOT_VM` | 控制磁源分布图输出。 |
| `POST_FIELD` | 非零时计算空间场。 |
| `POST_FIELD_ER` | 空间场计算使用的相对介电常数，必须大于零；缺失时回退到 `ER`。 |

### 源分布输出

输出目录：`Data/Source_Plot/`

| 文件 | 条件 |
| --- | --- |
| `J_vec.msh_Re`、`J_vec.msh_Im` | `POST_PLOT_J != 0` |
| `Qe.msh_Mag`、`Qe.msh_Re`、`Qe.msh_Im` | `POST_PLOT_QE != 0` |
| `Ve.msh_Re`、`Ve.msh_Im` | `POST_PLOT_VE != 0` |
| `M_vec.msh_Re`、`M_vec.msh_Im` | `POST_PLOT_M != 0` |
| `Qm.msh_Mag`、`Qm.msh_Re`、`Qm.msh_Im` | `POST_PLOT_QM != 0` |
| `Vm.msh_Re`、`Vm.msh_Im` | `POST_PLOT_VM != 0` |

### 空间场输出

启用 `POST_FIELD 1` 后，输出目录为 `Data/Field_Plot/`：

```text
E_vec.msh_Re
E_vec.msh_Im
E_scalar.msh_Mag
H_vec.msh_Re
H_vec.msh_Im
H_scalar.msh_Mag
```

## 典型文件流

```text
Data/model.msh
Data/set.txt
        |
        v
Quasi_Static_Model_Extract.exe
        |
        +--> Data/COMMON_DATA/LL_OO.bin
        +--> Data/COMMON_DATA/PP_OO.bin
        +--> Data/COMMON_DATA/B2N.txt
        +--> Data/COMMON_DATA/PORT.txt
        `--> Data/COMMON_DATA/Post_Model.txt
                         |
                         v
              Quasi_Static_Solver.exe
                         |
                         +--> Data/COMMON_DATA/map.txt
                         +--> Data/COMMON_DATA/Z_in.txt
                         +--> Data/COMMON_DATA/Y_in.txt
                         `--> Data/COMMON_DATA/Post_Source.txt
                                          |
                                          v
                              Post_Processing.exe
                                          |
                                          +--> Data/Source_Plot/
                                          `--> Data/Field_Plot/   # 可选
```

