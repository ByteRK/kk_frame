#!/bin/sh
#set -x

APP="kk_frame"

# 脚本路径
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# 输出目录
OUTPUT_DIR="$SCRIPT_DIR/../board"

# 编译目录
BUILD_DIR="$HOME/cdroid/outRK3506-Release"
# 编译结果目录
BUILD_RES_DIR="$BUILD_DIR/apps/$APP"
# 编译结果文件
BIN_FILE="$BUILD_RES_DIR/$APP"
PAK_FILE="$BUILD_RES_DIR/$APP.pak"

# 打包存储目录
PACKAGE_DIR="$SCRIPT_DIR/../package"

usage() {
    echo "Usage: $0 <version>"
    echo "Example:"
    echo "  $0 1.0.0"
    echo "  $0 1.0.0.1"
    exit 1
}

check_version() {
    VERSION="$1"
    echo "$VERSION" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?$'
}

calc_md5() {
    FILE="$1"

    if command -v md5sum >/dev/null 2>&1; then
        md5sum "$FILE" | awk '{print $1}'
    elif command -v md5 >/dev/null 2>&1; then
        md5 -q "$FILE"
    else
        echo "Error: neither md5sum nor md5 command found"
        exit 1
    fi
}

APP_VERSION="$1"
[ -n "$APP_VERSION" ] || usage
check_version "$APP_VERSION" || {
    echo "Error: invalid version format: $APP_VERSION"
    echo "Supported formats: x.x.x or x.x.x.x"
    exit 1
}

mkdir -p "$OUTPUT_DIR"
mkdir -p "$PACKAGE_DIR"

echo "Building $APP ..."

(
    cd "$BUILD_DIR" || exit 1
    make "$APP" -j3
)

[ -f "$BIN_FILE" ] || { echo "Error: missing $BIN_FILE"; exit 1; }
[ -f "$PAK_FILE" ] || { echo "Error: missing $PAK_FILE"; exit 1; }

echo "Copying files..."
cp "$BIN_FILE" "$OUTPUT_DIR/"
cp "$PAK_FILE" "$OUTPUT_DIR/"

DATE_STR="$(date '+%Y%m%d')"
TIME_STR="$(date '+%H%M%S')"
BASE_NAME="${APP}_V${APP_VERSION}_${DATE_STR}_${TIME_STR}"
TMP_TAR="${PACKAGE_DIR}/${BASE_NAME}_TEMP.tar.gz"

echo "Packing OUTPUT_DIR ..."
tar -czvf "$TMP_TAR" -C "$SCRIPT_DIR" "$(basename "$OUTPUT_DIR")"

MD5_VAL="$(calc_md5 "$TMP_TAR")"
FINAL_TAR="${PACKAGE_DIR}/${BASE_NAME}_${MD5_VAL}.tar.gz"

mv "$TMP_TAR" "$FINAL_TAR"

echo "Done."
echo "Package: $FINAL_TAR"