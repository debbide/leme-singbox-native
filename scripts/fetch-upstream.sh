#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TAG="${SINGBOX_TAG:-v1.14.0}"
COMMIT="${SINGBOX_COMMIT:-0b8995879f29a9b98ee027bc17b75e101445b238}"
DEST="$ROOT/upstream/sing-box"

rm -rf "$DEST"
git clone --filter=blob:none --no-checkout https://github.com/SagerNet/sing-box.git "$DEST"
git -C "$DEST" fetch --depth 1 origin "$COMMIT"
git -C "$DEST" checkout --detach "$COMMIT"

ACTUAL="$(git -C "$DEST" rev-parse HEAD)"
if [ "$ACTUAL" != "$COMMIT" ]; then
  echo "Unexpected sing-box commit: $ACTUAL" >&2
  exit 1
fi

printf 'Using sing-box %s at %s\n' "$TAG" "$ACTUAL"
