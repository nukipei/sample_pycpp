# sample

C++（nanobind）で実装したエラトステネスのふるいを Python から呼び出すサンプルパッケージです。
ビルドは uv + scikit-build-core、Linux wheel は GitLab CI（manylinux_2_28）、Windows wheel はローカルの Windows 環境でビルドします。

## 使用例

```python
from sample import sieve

sieve(30)  # [2, 3, 5, 7, 11, 13, 17, 19, 23, 29]
```

- `n < 0` → `ValueError`
- int 以外（`2.5`, `"10"` 等）→ `TypeError`

## 開発手順（WSL / Ubuntu）

> **注意**: WSL では `/mnt/c` 配下で作業しないでください（ファイル I/O が遅く、パーミッションや改行の問題が起きやすい）。`~/` 配下に clone してください。

前提ツール:

```bash
sudo apt install build-essential cmake   # cmake が無い場合は scikit-build-core が PyPI の cmake を自動取得します
curl -LsSf https://astral.sh/uv/install.sh | sh
```

```bash
uv sync             # C++ 拡張をビルドして editable インストール
uv run pytest -v
uv build            # sdist と wheel（この wheel は manylinux ではない。配布には使わない）
```

C++（`src/**/*.cpp`, `src/**/*.hpp`）や `CMakeLists.txt` を変更すると、`[tool.uv] cache-keys` により次の `uv sync` / `uv run` で自動的に再ビルドされます。

### Linux wheel（manylinux）をローカルでビルド

CI と同じ処理を Docker で実行できます。

```bash
docker run --rm -v "$PWD":/io -w /io quay.io/pypa/manylinux_2_28_x86_64 bash scripts/build_manylinux.sh
```

`wheelhouse/` に `sample-<ver>-cp3XX-cp3XX-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl` が Python バージョン数分生成され、それぞれテストされます。

## リリース手順

1. `pyproject.toml` の `version` を更新してコミット
2. タグ `vX.Y.Z` を作成して push → CI が Linux wheel と sdist を GitLab Package Registry に公開

   ```bash
   git tag v0.1.0
   git push origin v0.1.0
   ```

3. Windows 機（PowerShell、WSL ではない）で該当タグを checkout し、Windows wheel をビルド・公開

   ```powershell
   git checkout v0.1.0
   .\scripts\build_windows.ps1
   $env:GITLAB_URL = "https://<your-gitlab>"
   $env:GITLAB_PROJECT_ID = "<project id>"
   $env:GITLAB_TOKEN = "<deploy token / PAT>"   # リポジトリには書かない
   # Deploy Token の場合は $env:GITLAB_USERNAME = "<deploy token username>" も設定
   .\scripts\publish.ps1
   ```

   Windows 側の前提: uv、Visual Studio Build Tools（C++ によるデスクトップ開発ワークロード）、Git。
   開発中の試しビルドは `.\scripts\build_windows.ps1 -AllowDirty`（公開はしないこと）。

wheel はプラットフォームごとに別ファイル（`manylinux_*_x86_64` / `win_amd64`）で、同じバージョン番号のもとに Package Registry に登録されます。

## 利用者側のインストール

```bash
uv add sample --index https://<your-gitlab>/api/v4/projects/<project id>/packages/pypi/simple
```

または `pyproject.toml` に設定:

```toml
[[tool.uv.index]]
name = "gitlab"
url = "https://<your-gitlab>/api/v4/projects/<project id>/packages/pypi/simple"
explicit = true

[tool.uv.sources]
sample = { index = "gitlab" }
```

`sample` は一般的な名前で公開 PyPI に同名パッケージが存在し得るため、必ず `[tool.uv.sources]` で GitLab の index に固定してください（`explicit = true` により他パッケージは通常の PyPI から取得されます）。

プライベートプロジェクトの場合は `UV_INDEX_GITLAB_USERNAME` / `UV_INDEX_GITLAB_PASSWORD` 環境変数でトークンを渡してください。

## 構成

```text
src/cpp/          Python 非依存の C++ コア（sieve.hpp / sieve.cpp）
src/bindings/     nanobind バインディング（拡張モジュール _core）
src/sample/       Python パッケージ（__init__.py, _core.pyi, py.typed）
tests/python/     pytest
scripts/          build_manylinux.sh / build_windows.ps1 / publish.ps1
```
