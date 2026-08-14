# TSN_SoftWare（Qt Widgets）
2 个页签：

- 配置：包含 初始化配置/时间同步配置/整形配置。
- 实验：发帧/接收（对接 `1/build/tsn_multi_sender` 与 `1/build/tsn_multi_receiver`），支持配置文件读写与帧信息导出 TXT。

## 构建

```bash
cd TSN_SoftWare
qmake TSN_SoftWare.pro
make -j
```

## 运行

建议 root 运行以便执行 `ip` 等命令：

```bash
sudo ./bin/TSN_SoftWare
```

## 内置时钟同步工具

- 源码已集成到 `vendor/timesync-core`。
- 已将 `ts_masterd`、`clk_bridge`、`ts_ctl` 复制到 `bin/` 并在程序内优先使用，删除源码目录也能正常运行。
- 如需在新环境重建，可执行：

```bash
cd TSN_SoftWare
./scripts/build_timesync.sh
```

这会在本地使用集成源码构建上述 3 个工具并复制到 `bin/`。
