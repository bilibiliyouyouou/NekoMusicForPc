#!/usr/bin/env bash
# 从 CMakeLists.txt 同步 PKGBUILD / .SRCINFO 版本号（发 AUR 前执行）
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root"

pkgver="$(sed -n 's/^[[:space:]]*VERSION[[:space:]]*\([0-9.]*\).*/\1/p' CMakeLists.txt | head -1)"
if [[ -z "$pkgver" ]]; then
  echo "ERROR: 无法从 CMakeLists.txt 读取 VERSION" >&2
  exit 1
fi

sed -i "s/^pkgver=.*/pkgver=${pkgver}/" PKGBUILD
makepkg --printsrcinfo > .SRCINFO

echo "Synced AUR pkgver=${pkgver}"
