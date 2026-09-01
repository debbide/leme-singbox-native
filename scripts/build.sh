#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_OS="${TARGET_OS:-$(go env GOOS)}"
TARGET_ARCH="${TARGET_ARCH:-$(go env GOARCH)}"
VERSION="${SINGBOX_VERSION:-1.14.0}"
GARBLE_BIN="${GARBLE_BIN:-garble}"
GARBLE_SEED="${GARBLE_SEED:-random}"
GOGARBLE="${GOGARBLE:-github.com/debbide/leme-singbox-native}"
TAGS="${BUILD_TAGS:-with_gvisor,with_quic,with_wireguard,with_utls,with_clash_api,badlinkname,tfogo_checklinkname0}"

if [ "$TARGET_OS" = "windows" ] && [[ ",$TAGS," != *,with_purego,* ]]; then
  TAGS="${TAGS},with_purego"
fi

OUT_DIR="$ROOT/dist/${TARGET_OS}-${TARGET_ARCH}"
mkdir -p "$OUT_DIR"

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
