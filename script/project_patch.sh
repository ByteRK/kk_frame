#!/bin/bash
###
 # @Author: Ricken
 # @Email: me@ricken.cn
 # @Date: 2026-06-17 01:45:00
 # @LastEditTime: 2026-06-17 01:45:00
 # @FilePath: /kk_frame/script/project_patch.sh
 # @Description: 生成项目自身修改部分的补丁
 # @BugList:
 #
 # Copyright (c) 2026 by Ricken, All Rights Reserved.
 #
###

# 切换到当前脚本所在目录
cd $(dirname $0)

# 获取项目根目录
SRC_DIR=$(git rev-parse --show-toplevel 2>/dev/null)
PATCH_FILE=$SRC_DIR/project_changes.patch
if [ -z "$SRC_DIR" ]; then
    echo "错误：当前目录不是Git项目仓库！"
    exit 1
fi

# 打印路径
echo "项目目录:     $SRC_DIR"
echo "生成补丁文件: $PATCH_FILE"
echo ""

# 进入项目目录
cd $SRC_DIR

# 清空/创建补丁文件
> $PATCH_FILE

# 生成已跟踪文件的修改（包含已暂存和未暂存）
git diff --binary HEAD >> $PATCH_FILE

# 生成新增文件的补丁
git ls-files --others --exclude-standard | while IFS= read -r file; do
    if [ "$SRC_DIR/$file" = "$PATCH_FILE" ]; then
        continue
    fi

    if [ -f "$file" ]; then
        git diff --no-index --binary /dev/null "$file" >> "$PATCH_FILE"
        echo "添加新文件: $file"
    else
        echo "警告: 文件不存在 - $file"
    fi
done

# 打印结果
echo ""
echo "补丁已生成: $PATCH_FILE"
echo "文件大小: $(du -h "$PATCH_FILE" | cut -f1)"
echo "使用 git apply $PATCH_FILE --allow-empty 应用补丁"
