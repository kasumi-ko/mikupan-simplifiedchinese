# MikuPan 简体中文版

<p align="center">
  <a href="https://github.com/Mikompilation/MikuPan">
    <img src="https://img.shields.io/badge/Mikompilation-MikuPan-0F766E?style=flat-square&amp;logo=github&amp;logoColor=white&amp;labelColor=18181B" alt="Mikompilation / MikuPan 上游仓库" height="32">
  </a>
  <br>
  <strong>原生移植上游</strong>
  &nbsp;·&nbsp;
  <a href="https://github.com/Mikompilation/MikuPan">访问 MikuPan 仓库 ↗</a>
</p>

MikuPan 是《零～zero～ / Fatal Frame》的 PlayStation®2 至 PC 原生移植项目。本仓库基于 Mikompilation 的 MikuPan，为 Windows 提供日版游戏的简体中文适配。

![MikuPan 标志](resources/mikupan.png)

## 适用版本

- 本分支仅提供 Windows 构建。
- 游戏资源读取已转换为日版 NTSC-J，对应游戏编号 `SLPS-25074`，光盘启动文件为 `SLPS_250.74`。
- 为了正常显示简体中文，**必须使用非高汉化团制作的日版汉化 ISO**。本分支按该版本的字库、文字编码和资源布局适配。

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

Windows 版启动时直接读取 `MikuPan.exe` 同级的 `binary` 目录。安装时选中 ISO 后，安装程序会将光盘文件解包到该目录。

首次运行会转换模型及二维纹理，并在配置目录生成 `IMG_TEX.BIN` 缓存。需要重新转换时，可删除该缓存后再次启动。

## 调试框

游戏运行时，按 `F1` 可以打开调试框，访问设置与调试菜单。

## Windows 构建

准备 Windows C/C++ 编译工具链、CMake 3.25 或更新版本、Ninja 和 Git。可使用 MinGW-w64 GCC，或在已配置 MSVC 开发环境的终端中构建。依赖由 CMake 配置过程自动获取。

```powershell
git clone --branch main https://github.com/kasumi-ko/MikuPan-SimplifiedChinese.git
cd MikuPan-SimplifiedChinese

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_EU_VERSION=OFF
cmake --build build --target MikuPan --parallel
```

生成的可执行文件位于 `build/MikuPan/MikuPan.exe`。运行时请保留随构建生成的资源与依赖文件。

如需复用已生成的着色器字节码，可在 CMake 配置命令中加入 `-DMIKUPAN_COMPILE_SHADERS=OFF`。

## PlayStation®2 存档转移

通过设置菜单中的 PlayStation®2 存档转移功能，可以导入或导出游戏存档。上游支持从提取的存档目录、Memory Card (8MB) (for PlayStation®2) 映像以及 PSU、MAX、XPS、SPS、CBS 存档包导入，并可导出为以下格式。

- 带 ECC 的 Memory Card (8MB) (for PlayStation®2) 映像（`.ps2`）。
- 无 ECC 的 Memory Card (8MB) (for PlayStation®2) 映像（`.bin`）。
- `.psu`、`.max`、`.xps`、`.sps`、`.cbs` 存档包。
- 提取的游戏存档目录。

转移前请关闭游戏内的存档或读档界面。导入后通过游戏的正常读档入口加载。跨地区转换会受到原版游戏的事件和地图差异限制，工具会显示具体原因。

## 字体与界面

游戏内的简体中文显示依赖非高汉化团 ISO 中的字库与汉化资源。Port 界面使用的字体随程序资源提供。

高清字体替换功能按原始字体的指纹匹配。替换文件需保留字形布局、文件名和透明度，并放置在程序旁的 `resources/fonts/<fingerprint>/` 中。替换后重新启动，并在设置中启用高清字体选项。

## 致谢

感谢 Mikompilation 及 [MikuPan](https://github.com/Mikompilation/MikuPan) 的所有贡献者，为《零～zero～ / Fatal Frame》的 PC 原生移植提供基础。感谢非高汉化团的汉化工作，感谢 prinsep 制作 MikuPan 标志。

## 版权与许可

《零～zero～ / Fatal Frame》原版作品的版权署名包括 **TECMO, LTD.**（特库摩）。《零》系列及其游戏名称、角色、剧情、美术、音乐、影像、原版程序和相关商标的权利归 **KOEI TECMO GAMES CO., LTD.**（光荣特库摩）及其他相应权利人所有。

“PlayStation”是 Sony Interactive Entertainment Inc. 的注册商标或商标。“SONY”是 Sony Group Corporation 的注册商标或商标。其他产品名称、服务名称、标志和公司名称的商标与版权归各自所有者或授权方所有。

本项目为社区移植与简体中文适配项目，开源移植及适配代码采用 [GNU Affero General Public License v3.0](LICENSE) 许可。

非高汉化团制作的汉化文本与相关汉化资源保留原团队署名，相关权利归各自权利人所有。运行所需的 ISO 与光盘数据请自行准备。
