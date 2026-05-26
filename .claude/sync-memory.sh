#!/bin/bash
# 自动同步 CLAUDE.md —— 供 Claude Code Stop 钩子调用
set -e
cd "$(dirname "$0")/.."

git add CLAUDE.md

# 检查暂存区是否有变化，有则提交推送
if git diff --cached --quiet; then
    exit 0
fi

git commit -m "[auto] 同步 AI 记忆"
git push
