#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NODE_GYP_BIN="${NODE_GYP_BIN:-node-gyp}"

cd "$ROOT/addon"
"$NODE_GYP_BIN" rebuild "$@"

mkdir -p "$ROOT/dist/addon"
cp "$ROOT/addon/build/Release/leme_native.node" "$ROOT/dist/addon/leme_native.node"
printf 'Built %s\n' "$ROOT/dist/addon/leme_native.node"