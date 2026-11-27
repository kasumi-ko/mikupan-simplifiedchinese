# MikuPan-SimplifiedChinese

MikuPan 是《零～zero～ / Fatal Frame》的 PS2 至 PC 原生移植项目。本仓库基于 Mikompilation 的 MikuPan，为 Windows 提供日版游戏的简体中文适配。[上游仓库](https://github.com/Mikompilation/MikuPan)

![MikuPan 标志](resources/mikupan.png)

## 适用版本与维护状态

- 本分支仅提供 Windows 构建。
- 游戏资源读取已转换为日版 NTSC-J，对应游戏编号 `SLPS-25074`，光盘启动文件为 `SLPS_250.74`。
- 为了正常显示简体中文，**必须使用非高汉化团制作的日版汉化 ISO**。本分支按该版本的字库、文字编码和资源布局适配。

当前版本为 **0.1.0 beta**，简中适配由 **kasumi-ko（霞詩@bilibili）**维护。主线以外的事件、动画和界面仍在回归验证中。[本项目仓库](https://github.com/kasumi-ko/mikupan-simplifiedchinese)

## 游戏文件

请自行准备上述非高汉化团的汉化 ISO，并从中提取游戏文件。将完整光盘目录结构保留在程序的 `binary` 文件夹内，主要文件布局如下。

```text
MikuPan/
├─ MikuPan.exe
└─ binary/
   ├─ IMG_HD.BIN
   ├─ IMG_BD.BIN
   ├─ SLPS_250.74
   ├─ MOVIE/
   └─ MOVIE2/
```

首次启动时，在游戏数据文件夹选择界面选中上述 `binary` 目录。后续可通过设置菜单中的游戏数据文件夹选项修改路径。

首次运行会转换模型及二维纹理，并在配置目录生成 `IMG_TEX_JP.BIN` 缓存。需要重新转换时，可删除该缓存后再次启动。

## Windows 构建

准备 Windows C/C++ 编译工具链、CMake 3.25 或更新版本、Ninja 和 Git。可使用 MinGW-w64 GCC，或在已配置 MSVC 开发环境的终端中构建。依赖由 CMake 配置过程自动获取。

```powershell
git clone --branch main https://github.com/kasumi-ko/mikupan-simplifiedchinese.git
cd mikupan-simplifiedchinese

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_EU_VERSION=OFF
cmake --build build --target MikuPan --parallel
```

生成的可执行文件位于 `build/MikuPan/MikuPan.exe`。运行时请保留随构建生成的资源与依赖文件。

如需复用已生成的着色器字节码，可在 CMake 配置命令中加入 `-DMIKUPAN_COMPILE_SHADERS=OFF`。

## PS2 存档转移

通过设置菜单中的 PS2 存档转移功能，可以导入或导出游戏存档。上游支持从提取的存档目录、8 MB 记忆卡以及 PSU、MAX、XPS、SPS、CBS 存档包导入，并可导出为以下格式。

- 带 ECC 的 `.ps2` 记忆卡。
- 无 ECC 的 `.bin` 记忆卡。
- `.psu`、`.max`、`.xps`、`.sps`、`.cbs` 存档包。
- 提取的游戏存档目录。

转移前请关闭游戏内的存档或读档界面。导入后通过游戏的正常读档入口加载。跨地区转换会受到原版游戏的事件和地图差异限制，工具会显示具体原因。

## 字体与界面

游戏内的简体中文显示依赖非高汉化团 ISO 中的字库与汉化资源。Port 界面使用的字体随程序资源提供。

高清字体替换功能按原始字体的指纹匹配。替换文件需保留字形布局、文件名和透明度，并放置在程序旁的 `resources/fonts/<fingerprint>/` 中。替换后重新启动，并在设置中启用高清字体选项。

## 致谢

感谢 Mikompilation 及 MikuPan 原仓库的所有贡献者，为《零～zero～ / Fatal Frame》的 PC 原生移植提供基础。感谢非高汉化团的汉化工作，感谢 prinsep 制作 MikuPan 标志。[MikuPan 原仓库](https://github.com/Mikompilation/MikuPan)

## 版权与许可

《零～zero～ / Fatal Frame》原版作品的版权署名包括 **TECMO, LTD.（特库摩）**。《零》系列及其游戏名称、角色、剧情、美术、音乐、影像、原版程序和相关商标的权利归 **KOEI TECMO GAMES CO., LTD.（光荣特库摩）**及其他相应权利人所有。

本项目为社区移植与简体中文适配项目。开源移植及适配代码遵循 GNU Affero General Public License v3.0，具体条款见仓库许可文件。[LICENSE](LICENSE)

非高汉化团制作的汉化文本与相关汉化资源保留原团队署名，相关权利归各自权利人所有。运行所需的 ISO 与光盘数据请自行准备。
