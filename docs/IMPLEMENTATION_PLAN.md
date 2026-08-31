# Leme sing-box Native 实施计划

## 1. 项目目标

为 Leme Hub 构建并维护 sing-box 1.14 的嵌入式动态库及稳定对接层，支持�以下平台：

- Windows x64: `windows/amd64`, 输出 DLL
- Linux x64: `linux/amd64`, 输出 SO
- Linux ARM64: `linux/arm64`, 输出 SO

本仓库只负责原生核心、C ABI、构建流水线和产物验证。Leme Hub 应用仓库通过固定版本的 Release 产物接入。

## 2. 核心约束

- 上游 sing-box 固定到明确 tag 和 commit
- sing-box 1.14 使用 Go 1.25 或更高版本编译
- 不向应用层直接暴露 experimental/libbox 原始接口
- 对外提供由本项目维护的稳定 C ABI
- 三个平台提供相同的函数语义和错误模型
- 动态库版本与 sing-box 版本分别管理
- 所有发布产物必须附带 SHA-256、构建信息和许可证文件
- 第一阶段不接入 Node N-API，只验证动态库和 C ABI

## 3. 第一阶段最小 C ABI

计划提供以下接口：

- `leme_core_abi_version`
- `leme_core_singbox_version`
- `leme_core_go_version`
- `leme_core_check_config`
- `leme_core_start`
- `leme_core_reload`
- `leme_core_stop`
- `leme_core_status`
- `leme_core_last_error`
- `leme_core_free_string`

生命周期约束：

- 同一进程第一阶段只允许一个核心实例
- 所有导出函数不得让 Go panic 穿过 ABI 边界
- 字符串返回值必须由动态库分配，并使用统一释放函数释放
- start、reload、stop 必须串行化
- 错误通过返回码与 `last_error` 同时提供
- 回调接口留到第二阶段，避免第一版引入跨线程回调风险

## 4. 源码组织

```text
leme-singbox-native/
  bridge/                 Go C ABI 导出层
  scripts/                拉取上游、构建、打包和校验脚本
  tests/                  ABI smoke tests 和配置测试
  docs/                   架构、构建和接入文档
  .github/workflows/      三平台 CI
  dist/                   本地构建产物，不提交 Git
  upstream/               构建时拉取，不直接提交完整上游源码
```

## 5. 构建策略

### Windows x64

- 使用 Windows runner
- Go 1.25
- MinGW-w64 或 sing-ffi 所需的官方兼容工具链
- 输出 `leme-singbox.dll` 和对应 C 头文件
- 使用 `dumpbin` 或 `objdump` 检查导出符号

### Linux x64

- 使用 Ubuntu x64 runner 原生编译
- Go 1.25 和 GCC
- 输出 `libleme-singbox.so`
- 使用 `readelf`, `file`, `ldd` 检查架构、符号和运行时依赖

### Linux ARM64

优先顺序：

1. GitHub ARM64 runner 原生构建
2. ARM64 容器或 QEMU runner 构建
3. 最后才考虑 x64 上使用 `aarch64-linux-gnu-gcc` 交叉编译

输出 `libleme-singbox.so`，并检查 ELF 架构为 AArch64。

## 6. sing-box 构建标签

初始版本仅保留 Leme Hub 当前需要的能力：

- `with_gvisor`
- `with_quic`
- `with_wireguard`
- `with_utls`
- `with_naive_outbound`
- `with_clash_api`
- `badlinkname`
- `tfogo_checklinkname0`

是否加入 Tailscale、OpenVPN、OpenConnect、USB/IP 等标签，必须根据实际产品需求单独评估。默认不加入，控制体积和依赖复杂度。

Windows 是否需要 `with_purego` 将在首个编译实验中确认。

## 7. 实施阶段

### 阶段 A: 编译验证

- 固定 sing-box 1.14.0 tag 和 commit
- 创建最小 Go bridge
- 导出版本和配置检查接口
- 编译 Windows x64 DLL
- 编译 Linux x64 SO
- 编译 Linux ARM64 SO
- 检查动态库格式、架构和导出符号

成功标准：三个平台均可加载动态库，并可调用版本和配置检查接口。

### 阶段 B: 生命周期

- 实现 start、reload、stop
- 实�现线程安全状态机
- 捕获 panic 并转换为错误
- 添加端口监听 smoke test
- 添加重复启停和错误配置测试

成功标准：同一进程内可稳定完成 100 次启动和停止，不泄漏监听端口，不发生崩溃。

### 阶段 C: Node 对接层

- 使用 Node-API 创建 `.node` 模块
- 保持三个平台相同的 JavaScript API
- 异步执行阻塞操作
- 后续使用 ThreadSafeFunction 接入日志和状态回调
- 为 Electron 打包配置 `asarUnpack`

成��功标准：普通 Node.js 和 Electron 主进程都能完成 check、start、reload、stop。

### 阶段 D: Leme Hub 集成

- 在应用中增加 `CoreRuntime` 抽象
- 保留原来的 `ProcessCoreRuntime`
- 新增 `EmbeddedCoreRuntime`
- 提供运行时切换和快速回退
- 完成 Windows TUN、Linux TUN、DNS、selector 和规则集测试

## 8. 发布策略

Release 版本格式：

```text
v1.14.0-r1
```

其中：

- `1.14.0` 是上游 sing-box 版本
- `r1` 是本项目 ABI 或构建修订版本

每个 Release 包含：

```text
leme-singbox-win32-x64.zip
leme-singbox-linux-x64.tar.gz
leme-singbox-linux-arm64.tar.gz
checksums.txt
manifest.json
THIRD_PARTY_NOTICES.md
LICENSE
```

manifest 至少记录：

- runtime version
- ABI version
- sing-box version 和 commit
- Go version
- 构建标签
- 目标 OS 和架构
- 动态库文件名和 SHA-256
- 构建时间和 CI commit

## 9. 风险与处理

### experimental/libbox API 变化

通过内部适配层隔离，应用只依赖本项目稳定 ABI。升级 sing-box 时只调整 bridge。

### 动态库带崩宿主进程

所有导出入口增加 panic recovery。应用集成阶段保留 EXE 后备运行时。

### CGO 和交叉编译复杂度

优先使用对应平台原生 CI runner，避免依赖本地开发机和复杂交叉工具链。

### Electron ABI 变化

Node 层使用 Node-API，不直接依赖 V8 API。动态库本身与 Electron ABI 解耦。

### GPL 合规

保留 sing-box GPL-3.0-or-later 许可证、上游源码获取方式、修改说明和第三方声明。发布前进行单独许可证检查。

## 10. 当前下一步

1. 添加版本锁定文件
2. 创建最小 Go C ABI bridge
3. 实现 version 和 check_config
4. 创建三平台 GitHub Actions 构建矩阵
5. 构建并检查三个动态库
6. 增加 C 语言 smoke test
7. 形成首个 `v1.14.0-r1` 测试产物
