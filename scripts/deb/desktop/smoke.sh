#!/usr/bin/env bash
#
# 冒烟测试：装上一份 .deb，在虚拟显示里跑若干秒，检查是否存活与日志是否干净。
#
#   smoke.sh <DEB 文件> [秒数，默认 20]
#
# 只依赖 xvfb / dbus-run-session，GitHub 的 ubuntu runner 上直接可用；
# 本机 docker 里也能跑（此时通常是 root，自动跳过 sudo）。
set -euo pipefail

DEB=${1:?usage: smoke.sh <DEB 文件> [秒数]}
SECS=${2:-20}

[ -f "$DEB" ] || { echo "找不到：$DEB" >&2; exit 1; }
DEB=$(readlink -f "$DEB")

if [ "$(id -u)" -eq 0 ]; then SUDO=""; else SUDO="sudo"; fi

echo "== 安装 $DEB =="
$SUDO apt-get install -y --no-install-recommends "$DEB"
echo "== 缺失的动态库数：$(ldd /usr/bin/wiliwili | grep -c 'not found' || true) =="

WORK=$(mktemp -d)
export HOME="$WORK/home" XDG_RUNTIME_DIR="$WORK/run"
mkdir -p "$HOME" "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
LOG="$WORK/wiliwili.log"

echo "== 运行 ${SECS} 秒 =="
xvfb-run -a -s "-screen 0 1280x720x24" dbus-run-session -- bash -c "
    stdbuf -oL -eL wiliwili > '$LOG' 2>&1 &
    APP=\$!
    sleep $SECS
    if kill -0 \$APP 2>/dev/null; then echo '存活'; else echo '提前退出'; fi
    kill \$APP 2>/dev/null || true
    sleep 1
    kill -9 \$APP 2>/dev/null || true
"

echo "== 日志行数：$(wc -l < "$LOG") =="
echo "== GA/analytics 命中：$(grep -icE 'analytics|google-analytics|ssl_connect' "$LOG" || true) =="
echo "== GLFW 数字错误码命中：$(grep -cE 'GLFW [0-9]+:' "$LOG" || true) =="
echo "== ERROR 行 =="
sed -E 's/\x1b\[[0-9;]*m//g' "$LOG" | grep '\[ERROR' || echo "（无）"
echo "== 日志尾部 =="
sed -E 's/\x1b\[[0-9;]*m//g' "$LOG" | tail -5
