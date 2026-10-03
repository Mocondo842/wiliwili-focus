#!/usr/bin/env bash
#
# 把 `cmake --install` 装出来的暂存树打成 .deb（桌面版，任意架构）。
#
#   build.sh <STAGE_DIR> <ARCH> <VERSION> <OUT_DIR>
#
#   STAGE_DIR  DESTDIR=... cmake --install build 的结果，里面应有 usr/bin/wiliwili
#   ARCH       Debian 架构名：amd64 / arm64 / ...
#   VERSION    Debian 版本号：不能含下划线、不能以连字符结尾
#   OUT_DIR    输出目录（自动创建），产物为 wiliwili_<VERSION>_<ARCH>.deb
#
# 依赖由二进制自身的动态依赖反查（ldd + dpkg -S），因此 amd64/arm64 各自算出各自的清单；
# 反查失败时退回一份保守的固定清单。
set -euo pipefail

STAGE=${1:?usage: build.sh <STAGE_DIR> <ARCH> <VERSION> <OUT_DIR>}
ARCH=${2:?usage: build.sh <STAGE_DIR> <ARCH> <VERSION> <OUT_DIR>}
VERSION=${3:?usage: build.sh <STAGE_DIR> <ARCH> <VERSION> <OUT_DIR>}
OUT=${4:?usage: build.sh <STAGE_DIR> <ARCH> <VERSION> <OUT_DIR>}

BIN="$STAGE/usr/bin/wiliwili"
[ -x "$BIN" ] || { echo "未找到暂存的二进制：$BIN" >&2; exit 1; }
case "$VERSION" in
    *[!A-Za-z0-9.+~:-]*|*_) echo "非法 Debian 版本号：$VERSION" >&2; exit 1 ;;
esac

ROOT=$(mktemp -d)
trap 'rm -rf "$ROOT"' EXIT

mkdir -p "$ROOT/usr"
cp -a "$STAGE/usr/." "$ROOT/usr/"

# 运行期依赖：只看直接链接（readelf 的 NEEDED），再反查每个 .so 由哪个已安装包提供。
# 不用 ldd 的全量输出——那会把 libmpv 的整棵依赖树都算进来。
# 注意合并 /usr 的系统上 dpkg 记录的是 /usr/lib/...，而 ldd 可能给出 /lib/...。
DEPS=""
if command -v readelf >/dev/null 2>&1 && command -v ldd >/dev/null 2>&1 && command -v dpkg >/dev/null 2>&1; then
    LDD=$(ldd "$BIN" 2>/dev/null || true)
    DEPS=$(
        readelf -d "$BIN" | awk '/\(NEEDED\)/{ gsub(/[][]/, "", $NF); print $NF }' | while read -r so; do
            [ -n "$so" ] || continue
            path=$(printf '%s\n' "$LDD" | awk -v s="$so" '$1 == s && $3 ~ /^\// { print $3; exit }')
            [ -n "$path" ] || continue
            pkg=$(dpkg -S "$path" 2>/dev/null | head -1 | cut -d: -f1 || true)
            [ -n "$pkg" ] || pkg=$(dpkg -S "/usr$path" 2>/dev/null | head -1 | cut -d: -f1 || true)
            [ -n "$pkg" ] && echo "$pkg"
        done | sort -u | sed -E 's/:(amd64|arm64|armhf|i386|all)$//' | paste -sd, -
    )
fi
if [ -z "$DEPS" ]; then
    echo "警告：依赖反查失败，使用固定清单" >&2
    DEPS="libmpv2, libwebp7, libdbus-1-3, libssl3 | libssl3t64, zlib1g, libstdc++6, libgcc-s1, libc6"
fi

mkdir -p "$ROOT/DEBIAN"
cat > "$ROOT/DEBIAN/control" <<EOF
Package: wiliwili
Version: $VERSION
Section: video
Priority: optional
Architecture: $ARCH
Depends: $DEPS
Recommends: fonts-noto-cjk
Maintainer: wiliwili-focus <noreply@github.com>
Homepage: https://github.com/Mocondo842/wiliwili-focus
Description: A third-party Bilibili client designed specifically for controller users
 wiliwili 的本地构建：去掉了系统推荐内容（首页推荐 / 热搜 / 播放页相关推荐）
 的显示入口，并修掉了搜索页崩溃与几处无害但刷屏的日志。
 内部接口、组件与 i18n 词条均未删除。
EOF

cat > "$ROOT/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -e
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true
fi
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q /usr/share/applications 2>/dev/null || true
fi
exit 0
EOF
cp "$ROOT/DEBIAN/postinst" "$ROOT/DEBIAN/postrm"
chmod 0755 "$ROOT/DEBIAN/postinst" "$ROOT/DEBIAN/postrm"

mkdir -p "$OUT"
DEB="$OUT/wiliwili_${VERSION}_${ARCH}.deb"
dpkg-deb --build --root-owner-group "$ROOT" "$DEB" >/dev/null

echo "Depends: $DEPS"
echo "已生成：$DEB ($(stat -c %s "$DEB") 字节)"
dpkg-deb -I "$DEB" | sed -n '1,12p'
