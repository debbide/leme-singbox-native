# C ABI v1

第一阶段 ABI 只提供无状态能力，用于验证三个目标平台的动态库生成、加载和配置检查。

## 内存规则

`leme_core_singbox_version`、`leme_core_go_version` 和 `leme_core_last_error` 返回的字符串由动态库分配。调用方读取后必须调用 `leme_core_free_string`。

## 返回码

`leme_core_check_config` 返回 `0` 表示成功，非 `0` 表示失败。失败详情通过 `leme_core_last_error` 获取。

## 线程安全

错误信息存储已加读写锁。生命周期 API 加入后，start、reload、stop 将使用独立互斥锁��串行化。
