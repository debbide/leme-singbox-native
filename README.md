# leme-singbox-native

Leme Hub 使用的 sing-box 动态库、稳定 C ABI 和 Node-API 对接层。

## 目标平台

- Windows x64
- Linux x64
- Linux ARM64

## 当前能力

- 稳定 C ABI v2
- sing-box 和 Go 版本查询
- JSON 配置检查
- 单实例启动、停止、安全重载和状态查询
- ABI 边界 panic recovery
- Node-API 动态加载适配层
- 三目标平台自动构建与发布

## 本地构建

先执行 `./scripts/fetch-upstream.sh` 拉取固定版本的 sing-box 源码，再执行 `./scripts/build.sh` 构建动态库。

Windows 的 `c-shared` 构建需要 MinGW-w64 GCC。项目固定使用 Go 1.25.5、garble v0.15.0 和 sing-box v1.14.0。

Node-API 适配层可通过 `./scripts/build-addon.sh` 单独构建。

## 测试范围

`tests/smoke.c` 覆盖 ABI 版本、配置检查、启动、重复启动、安全重载、停止、幂等停��止，以及连续 100 次启动和停止。

详细计划见 [docs/IMPLEMENTATION_PLAN.md](docs/IMPLEMENTATION_PLAN.md)，C ABI 说明见 [docs/C_ABI.md](docs/C_ABI.md)。
