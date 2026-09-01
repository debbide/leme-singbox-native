#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_OS="${TARGET_OS:-$(go env GOOS)}"
TARGET_ARCH="${TARGET_ARCH:-$(go env GOARCH)}"
VERSION="${SINGBOX_VERSION:-1.14.0}"
PINNED_GARBLE="$ROOT/.tools/garble-v0.15.0/garble.exe"
if [ -n "${GARBLE_BIN:-}" ]; then
  GARBLE_BIN="$GARBLE_BIN"
elif [ -x "$PINNED_GARBLE" ]; then
  GARBLE_BIN="$PINNED_GARBLE"
else
  GARBLE_BIN="garble"
fi
GARBLE_SEED="${GARBLE_SEED:-random}"
GOGARBLE="${GOGARBLE:-github.com/debbide/leme-singbox-native}"
TAGS="${BUILD_TAGS:-with_gvisor,with_quic,with_wireguard,with_utls,with_clash_api,badlinkname,tfogo_checklinkname0}"

if [ "$TARGET_OS" = "windows" ] && [[ ",$TAGS," != *,with_purego,* ]]; then
  TAGS="${TAGS},with_purego"
fi

OUT_DIR="$ROOT/dist/${TARGET_OS}-${TARGET_ARCH}"
mkdir -p "$OUT_DIR"

if ! command -v "$GARBLE_BIN" >/dev/null 2>&1 && [ ! -x "$GARBLE_BIN" ]; then
  echo "garble was not found; install mvdan.cc/garble@v0.15.0 or set GARBLE_BIN" >&2
  exit 1
fi

if [ "$TARGET_OS" = "windows" ] && ! command -v "${CC:-gcc}" >/dev/null 2>&1; then
  echo "Windows c-shared builds require MinGW-w64 GCC in PATH or CC set to its executable" >&2
  exit 1
fi

case "$TARGET_OS" in
  windows) LIBRARY="$OUT_DIR/leme-singbox.dll" ;;
  linux) LIBRARY="$OUT_DIR/libleme-singbox.so" ;;
  *) echo "Unsupported target OS: $TARGET_OS" >&2; exit 1 ;;
esac

CGO_ENABLED=1 GOOS="$TARGET_OS" GOARCH="$TARGET_ARCH" GARBLE_SEED="$GARBLE_SEED" GOGARBLE="$GOGARBLE" \
  "$GARBLE_BIN" -literals -tiny build -buildmode=c-shared -trimpath \
  -tags "$TAGS" \
  -ldflags "-X github.com/sagernet/sing-box/constant.Version=$VERSION -X runtime.godebugDefault=multipathtcp=0,tlssha1=1,tlsunsafeekm=1 -s -w -buildid= -checklinkname=0" \
  -o "$LIBRARY" ./bridge

HEADER="${LIBRARY%.*}.h"
if [ -f "$HEADER" ]; then
  mv "$HEADER" "$OUT_DIR/leme-singbox.h"
fi

printf 'Built %s\n' "$LIBRARY"
