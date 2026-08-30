#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="${ROOT_DIR}/docs/wiki"
REPOSITORY="${GITHUB_REPOSITORY:-AlaskaChinese/EmbedPluginUI}"
REMOTE="${WIKI_REMOTE:-https://github.com/${REPOSITORY}.wiki.git}"

if [[ ! -d "${SOURCE_DIR}" ]]; then
  echo "Wiki source directory not found: ${SOURCE_DIR}" >&2
  exit 1
fi

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

WIKI_DIR="${WORK_DIR}/wiki"
git clone "${REMOTE}" "${WIKI_DIR}"

# GitHub Wiki stores pages as Markdown files at the repository root.
find "${WIKI_DIR}" -maxdepth 1 -type f -name '*.md' -delete
cp "${SOURCE_DIR}"/*.md "${WIKI_DIR}/"

cd "${WIKI_DIR}"
git config user.name "${GIT_AUTHOR_NAME:-github-actions[bot]}"
git config user.email "${GIT_AUTHOR_EMAIL:-41898282+github-actions[bot]@users.noreply.github.com}"

git add -A
if git diff --cached --quiet; then
  echo "Wiki is already up to date."
  exit 0
fi

git commit -m "docs: sync EmbedPluginUI bilingual wiki"
git push origin HEAD:master

echo "Published wiki: https://github.com/${REPOSITORY}/wiki"
