#!/usr/bin/env bash
# manylinux_2_28 コンテナ内で Linux wheel をビルド・修復・テストする。
#
# 使い方（コンテナ内、リポジトリルートで実行）:
#   bash scripts/build_manylinux.sh
# ローカル（Docker）:
#   docker run --rm -v "$PWD":/io -w /io quay.io/pypa/manylinux_2_28_x86_64 bash scripts/build_manylinux.sh
#
# 環境変数:
#   PYTHON_TAGS  対象の CPython タグ（既定: "cp311 cp312 cp313 cp314"）
#   UV_VERSION   導入する uv のバージョン指定（既定: 最新）
set -euo pipefail

PYTHON_TAGS="${PYTHON_TAGS:-cp311 cp312 cp313 cp314}"
UV_SPEC="uv${UV_VERSION:+==${UV_VERSION}}"

# 1. uv の導入（manylinux 同梱の Python を利用）
if ! command -v uv >/dev/null 2>&1; then
  /opt/python/cp312-cp312/bin/python -m pip install --root-user-action=ignore --quiet "${UV_SPEC}"
  export PATH="/opt/python/cp312-cp312/bin:${PATH}"
fi
uv --version

rm -rf dist wheelhouse
mkdir -p dist wheelhouse

# 2. バージョンごとに wheel をビルド
for tag in ${PYTHON_TAGS}; do
  py="/opt/python/${tag}-${tag}/bin/python"
  echo "=== build: ${tag} (${py}) ==="
  uv build --wheel --python "${py}" -o dist/
done

# 3. auditwheel で manylinux タグ付きに修復
for whl in dist/*.whl; do
  auditwheel repair "${whl}" -w wheelhouse/
done
ls -l wheelhouse/

# 4. バージョンごとに新しい venv へインストールしてテスト
for tag in ${PYTHON_TAGS}; do
  py="/opt/python/${tag}-${tag}/bin/python"
  venv="/tmp/venv-${tag}"
  echo "=== test: ${tag} ==="
  rm -rf "${venv}"
  uv venv --python "${py}" "${venv}"
  uv pip install --python "${venv}/bin/python" pytest
  uv pip install --python "${venv}/bin/python" --no-index --find-links wheelhouse/ sample
  # ソースツリーの src/ ではなくインストール済み wheel を import させるため tests ディレクトリから実行
  (cd tests/python && "${venv}/bin/python" -m pytest -q -p no:cacheprovider)
done

echo "=== done ==="
ls -l wheelhouse/
