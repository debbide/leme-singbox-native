# C ABI v2

ABI v2 在版本查询和配置检查基础上，增加单实例核心生命周期管理。

## 内存规则

`leme_core_singbox_version`、`leme_core_go_version` 和 `leme_core_last_error` 返回的字符串由动态库分配。调用方读取后必须调用 `leme_core_free_string`。

## 返回码

`leme_core_check_config`、`leme_core_start`、`leme_core_stop` 和 `leme_core_reload` 返回 `0` 表示成功，非 `0` 表示失败。失败详情通过 `leme_core_last_error` 获取。

## 生命周期接口

- `leme_core_start(config)`：使用 JSON 配置启动核心；核心已运行时返回错误。
- `leme_core_stop()`：停止当前核心；未运行时按幂等成功处理。
- `leme_core_reload(config)`：停止当前实例并使用新配置启动。
- `leme_core_status()`：返回 `0` 表示已停止，返回 `1` 表示运行中。

## 线程安全

错误信息存储使用读写锁。启动、重载、停止和状态读取使用独立互斥锁串行化；同一进程只允许一个核心实例。

重载会先验证新配置。配置验证失败时，当前运行实例保持不变；验证成功后才停止旧实例并启动新实例。
