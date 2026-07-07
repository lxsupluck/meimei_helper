#!/bin/bash
set -e

# ============================================
# 快速更新 GitHub 仓库
# 用法: ./scripts/push.sh "提交信息"
#        ./scripts/push.sh                (使用默认提交信息)
# ============================================

cd "$(dirname "$0")/.."

# 获取提交信息
if [ -n "$1" ]; then
    MSG="$1"
else
    MSG="更新于 $(date '+%Y-%m-%d %H:%M')"
fi

echo "=== Git 状态 ==="
git status --short

# 检查是否有改动
if git diff --quiet && git diff --cached --quiet; then
    echo ""
    echo "没有文件改动，无需提交。"
    exit 0
fi

echo ""
echo "=== 提交信息: $MSG ==="

# 暂存本地修改，拉取远端，再恢复
echo ""
echo "=== 拉取远端更新 ==="
git stash push -m "auto-stash before push"
if ! git pull --rebase origin main; then
    echo "⚠ 拉取失败，尝试恢复 stash..."
    git stash pop
    echo "请手动处理后再推送。"
    exit 1
fi

# 恢复暂存的修改
if git stash list | grep -q "auto-stash before push"; then
    echo ""
    echo "=== 恢复本地修改 ==="
    if ! git stash pop; then
        echo "⚠ 恢复时有冲突，请手动解决冲突后执行: git add . && git commit && git push"
        exit 1
    fi
fi

# 添加所有改动
git add .

# 提交
git commit -m "$MSG"

# 推送
echo ""
echo "=== 推送到 GitHub ==="
git push origin main

echo ""
echo "✅ 完成！已推送至 github.com:lxsupluck/meimei_helper.git"
