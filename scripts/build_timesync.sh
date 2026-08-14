#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
# 使用随项目集成的 timesync-core 源码构建 ts_masterd/clk_bridge/ts_ctl，并复制到 bin/
SRC_DIR="$ROOT/vendor/timesync-core"
BIN_DIR="$ROOT/bin"

if [[ ! -d "$SRC_DIR" ]]; then
  echo "未找到源码目录：$SRC_DIR" >&2
  exit 1
fi

mkdir -p "$BIN_DIR"

make -C "$SRC_DIR" ts_masterd clk_bridge ts_ctl

cp "$SRC_DIR/ts_masterd" "$BIN_DIR/"
cp "$SRC_DIR/clk_bridge" "$BIN_DIR/"
cp "$SRC_DIR/ts_ctl" "$BIN_DIR/"

echo "构建完成，已复制到 $BIN_DIR"
