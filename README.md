# send2perclip（C++ 版）

`send2perclip.py` 的 C++ / Win32 实现：按 **Ctrl+V** 依次把剪贴板里的多行内容粘到目标程序里，
按 **Esc** 退出。行为与原 Python 脚本一致。

## 目录结构

```
CMakeLists.txt
app.rc            Windows 资源脚本：把 ks.ico 作为图标（资源 ID 1）编进 exe
ks.ico            exe 图标文件
src/
  main.cpp          主流程：读剪贴板 → 逐行写入剪贴板 → 等 Ctrl+V
  config.h          可调参数（与 Python 版的常量一一对应）
  clipboard.cpp/.h  剪贴板读写（含占用重试）
  csv.cpp/.h        剪贴板文本解析（同 Python csv.reader 默认规则）
  paste_watcher.cpp/.h  全局 Ctrl+V / Esc 监听（WH_KEYBOARD_LL 低级键盘钩子）
  text.cpp/.h       UTF-8 <-> UTF-16 转换
```

## 构建

本工程默认 **静态链接**：可执行文件不依赖 `libstdc++-6.dll` / `libgcc_s_seh-1.dll`（MinGW）
或 `MSVCP140.dll` / `VCRUNTIME140.dll`（MSVC），可以直接拷到别的 Windows 机器上运行。

- MinGW / GCC / Clang：加 `-static -static-libgcc -static-libstdc++`
- MSVC：`CMAKE_MSVC_RUNTIME_LIBRARY = MultiThreaded`（即 `/MT`、Debug 为 `/MTd`）

> 剩下的 `KERNEL32.dll`、`USER32.dll`、`ucrtbase.dll`（`api-ms-win-crt-*`）是 Windows 自带组件，
> 属于系统 API，无法也不需要静态链接。

### exe 图标

图标由 `app.rc` 提供，CMake 会自动启用 RC 语言：MSVC 用 `rc.exe`，MinGW 用 `windres`。
`app.rc` 里只有一行 `1 ICON "ks.ico"` —— 资源 ID 1 就是“程序图标”，资源管理器取 exe 中
ID 最小的 ICON 作为显示图标。

换图标只需把新的 `.ico` 覆盖 `ks.ico`（或改 `app.rc` 里的文件名）后重新构建，无需改 CMake。
`ks.ico` 里若包含多个尺寸（16/32/48/256），会被整份编进去，Windows 会按需挑分辨率。

### 命令行（Ninja，速度快，推荐）

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 命令行（Visual Studio 生成器）

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### VS Code

安装 CMake Tools 扩展后：选择 Kit → `Build`，或按 F7。
产物路径：

- Ninja / Makefile：`build/send2perclip.exe`
- Visual Studio：`build/Release/send2perclip.exe`

### GitHub Actions 自动构建

`.github/workflows/build.yml` 在 GitHub 的 Windows runner 上自动构建，本机无需装任何工具链：

| 任务 | 工具链 | 产物 |
| --- | --- | --- |
| `msvc-x64` | Visual Studio 2022 生成器，`/MT` 静态 CRT | `send2perclip-windows-x64-msvc.exe` |
| `mingw-ucrt64` | MSYS2 UCRT64 的 GCC + Ninja，`-static` | `send2perclip-windows-x64-mingw.exe` |

触发条件与行为：

- push 到 `main`：构建两条线，产物上传为 Artifact（仓库 → Actions → 对应 run → Artifacts，保留 30 天）。
- push `v*` 标签（如 `v1.0.0`）：除上面以外，再自动创建 / 更新 Release，并把两个 exe 作为附件发布。
- Pull Request、手动 `workflow_dispatch`：只构建 + 上传 Artifact。

两个 job 在构建后都会检查 exe 的导入表，一旦出现 `MSVCP140.dll`、`VCRUNTIME140.dll`、
`libstdc++-6.dll` 这类非系统依赖就直接失败 —— 防止哪天误改了静态链接配置而 CI 仍然"绿"。

发布新版本：

```powershell
git tag v1.0.0
git push origin v1.0.0
```

### 检查是否已静态链接

```powershell
# MinGW
objdump -p build\send2perclip.exe | Select-String "DLL Name"
# MSVC
dumpbin /dependents build\Release\send2perclip.exe
```

只要列表里**没有** `libstdc++-6.dll` / `libgcc_s_seh-1.dll` / `MSVCP140.dll`，就是静态链接成功。

## 清理构建中间文件

```powershell
# 1) 只删可执行文件与 .o 等中间产物，保留 CMake 配置 —— 清完可直接再构建，最快
cmake --build build --target clean

# 2) 连 CMakeCache.txt、CMakeFiles 一起删掉（CMake 配置全部清空）
#    注意：清完之后必须重新执行一次 configure，否则 cmake --build 会报
#    “not a CMake build directory (missing CMakeCache.txt)”
cmake --build build --target distclean
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # 重新 configure

# 3) 最彻底：直接删掉整个构建目录（推荐，等价于 1+2 且不留任何残留）
Remove-Item -Recurse -Force build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

构建目录 `build/` 里各类文件的含义：

| 文件 / 目录 | 说明 | 能否手动删 |
| --- | --- | --- |
| `send2perclip.exe` | 最终产物 | 能（`clean` 会删） |
| `CMakeFiles/send2perclip.dir/*.o` | 目标文件 | 能（`clean` 会删） |
| `CMakeCache.txt` | CMake 缓存（编译器路径、缓存变量） | 能，但之后必须重新 configure |
| `CMakeFiles/`、`build.ninja`、`Makefile` | CMake 生成的构建脚本 | 能，但之后必须重新 configure |
| `.ninja_deps` / `.ninja_log` | Ninja 依赖与日志 | 能（一般不用手动删） |

## 用法

1. 先复制要依次粘贴的内容（多行文本，逗号分隔即按单元格拆分）。
2. 运行 `send2perclip.exe`，会显示 `共 N 行数据。按 Ctrl+V 依次粘贴，按 Esc 退出。`
3. 切到目标程序，每按一次 **Ctrl+V** 粘一行；程序自动把下一行写入剪贴板。
4. 按 **Esc** 结束。

## 与 Python 版的对应关系

| Python | C++ |
| --- | --- |
| `pyperclip.paste()` | `clipboard::readText()`（优先 `CF_UNICODETEXT`，退回 `CF_TEXT`） |
| `pyperclip.copy()` | `clipboard::writeText()`（`OpenClipboard` 失败时按 `kClipboardRetries` 重试） |
| `csv.reader(StringIO(content))` | `csv::parseRows()`（逗号分隔、`""` 转义、`\r\n`/`\n`/`\r`） |
| `[row for row in reader if any(cell.strip() ...)]` | `csv::isBlankRow()` 过滤 |
| `keyboard.add_hotkey('ctrl+v' / 'esc')` | `PasteWatcher` + `SetWindowsHookExW(WH_KEYBOARD_LL)` |
| `keyboard.is_pressed('ctrl'/'v')` | 钩子里维护 `ctrl_left_/ctrl_right_/v_down_` 状态 |
| `time.sleep(POLL_INTERVAL)` | `Sleep(cfg::kPollIntervalMs)` |
| `print('\a')` | 控制台写 `BEL`，输出被重定向时用 `Beep()` 兜底 |

### 重要差异 / 注意点

- **必须泵消息**：低级键盘钩子的回调是在调用 `PeekMessage`/`GetMessage` 时投递进来的，
  所以 `waitForNext` / `waitKeyRelease` 等待期间会不断调用 `pumpMessages()`。
  这也是本实现不使用独立工作线程的原因（单线程更简单，且不会漏掉按键）。
- **“跳行”修复保留**：检测到 Ctrl+V 后先等按键松开（`waitKeyRelease`），
  再等 `cfg::kPasteSettleDelayMs`（默认 250 ms）让目标程序读完剪贴板，才写入下一行。
  目标程序很慢时调大该值即可。
- **触发抖动**：`cfg::kDebounceMs`（默认 150 ms）内的重复 Ctrl+V 只算一次，避免长按的自动重复。
- 剪贴板文本按 `csv.reader` 的默认方言解析，即**只以逗号分列**（从 Excel 复制的制表符内容会保留原样，
  这一点与原 Python 脚本相同）。
- 程序不需要目标程序变成前台窗口以外的任何配合；钩子只在运行时生效，退出时会卸载。
