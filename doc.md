# サンプルプロジェクト作成手順書（uv + C++ 拡張 + GitLab CI）

この手順書は、Claude（Claude Code 等）にサンプルプロジェクトを一から作らせるための指示書です。
上から順に実施し、各ステップの「完了条件」を満たしてから次へ進んでください。

---

## 0. 目的とゴール

C++ で実装した関数を Python から呼べるパッケージを作り、以下を満たすこと。

- WSL（Ubuntu）上で `uv sync` → `uv run pytest` だけで開発・テストが回る
- 社内 GitLab の CI で **Linux 用 wheel（manylinux）** をビルド・テストできる
- **Windows 用 wheel はローカルの Windows 環境でスクリプト1本でビルド**できる
- wheel は **プラットフォームごとに別ファイル**（1つの wheel に複数 OS のバイナリを同梱しない）
- Linux / Windows の wheel を同じバージョン番号で GitLab Package Registry に登録できる

## 1. 前提・技術スタック（変更しないこと）

| 項目 | 採用 |
|---|---|
| パッケージ管理 | uv |
| ビルドバックエンド | scikit-build-core |
| C++ ビルド | CMake（3.15 以上）、C++17 |
| バインディング | nanobind |
| テスト | pytest（Python）、C++ 単体テストは任意（GoogleTest） |
| Linux wheel | manylinux_2_28 イメージ内で `uv build` + `auditwheel repair` |
| Windows wheel | ローカル Windows で `cibuildwheel` |
| 対象 Python | 3.11〜3.14（CPython、x86_64 / win_amd64 のみ） |

- 各ツールの**バージョンは作業時点の最新安定版を確認して**指定すること（記憶に頼らない）。
- プロジェクト名（PyPI 名）: `cppsieve`、import 名: `cppsieve`

## 2. 搭載する関数の仕様

### `cppsieve.sieve(n: int) -> list[int]`

エラトステネスのふるいで **n 以下** の素数を昇順のリストで返す。

| 入力 | 期待される挙動 |
|---|---|
| `n < 0` | `ValueError` を送出 |
| `n == 0`, `n == 1` | `[]` |
| `n == 2` | `[2]` |
| `n == 10` | `[2, 3, 5, 7]` |
| `n == 30` | `[2, 3, 5, 7, 11, 13, 17, 19, 23, 29]` |
| `n == 1_000_000` | 要素数 78498、末尾 999983 |
| int 以外（`2.5`, `"10"` 等） | `TypeError` |

実装要件:

- C++ のコア関数 `std::vector<std::int64_t> sieve(std::int64_t n)` を Python 非依存で実装する（`src/cpp/`）。
  - ふるいの配列は `std::vector<std::uint8_t>`（`vector<bool>` は避ける）。
  - `i * i <= n` までループし、`i * i` から消し始める。オーバーフローしない型で計算する。
  - 負数は `std::invalid_argument` を投げる（nanobind が `ValueError` に変換する）。
- バインディング側（`src/bindings/module.cpp`）で、計算中は GIL を解放する（`nb::gil_scoped_release`）。
- docstring を付ける。
- 型スタブ `src/cppsieve/_core.pyi` を用意し、`py.typed` を置く。

## 3. ディレクトリ構成

```
cppsieve/
├── pyproject.toml
├── uv.lock
├── .python-version
├── .gitignore
├── CMakeLists.txt
├── .gitlab-ci.yml
├── README.md
├── scripts/
│   ├── build_windows.ps1      # Windows wheel のローカルビルド
│   ├── publish.ps1            # Windows wheel のアップロード
│   └── build_manylinux.sh     # CI 用 Linux wheel ビルド
├── src/
│   ├── cpp/
│   │   ├── include/cppsieve/sieve.hpp
│   │   └── sieve.cpp
│   ├── bindings/
│   │   └── module.cpp         # 拡張モジュール名は _core
│   └── cppsieve/
│       ├── __init__.py        # from ._core import sieve; __all__ = ["sieve"]
│       ├── _core.pyi
│       └── py.typed
└── tests/
    └── python/
        └── test_sieve.py
```

## 4. 作業手順

### Step 1: 環境確認（WSL）

- 作業ディレクトリが WSL のファイルシステム上（`~/` 配下）であることを確認する。`/mnt/c` 配下なら警告して止まる。
- `uv --version`、`cmake --version`、`g++ --version`（または `c++`）を確認する。
  不足していれば導入コマンドを提示する（`sudo apt install build-essential cmake` 等）。勝手に sudo を実行しない。

**完了条件**: 必要ツールのバージョンが表示される。

### Step 2: 雛形作成

- `uv init --lib --build-backend scikit cppsieve` で雛形を作り、生成物を第3節の構成に合わせて整理する（pybind11 の雛形が出た場合は nanobind に置き換える）。
- `git init` し、`.gitignore` に `build/`, `dist/`, `wheelhouse/`, `.venv/`, `__pycache__/`, `*.so`, `*.pyd` を入れる。

### Step 3: pyproject.toml

最低限、以下を含めること。

```toml
[project]
name = "cppsieve"
version = "0.1.0"
description = "Sieve of Eratosthenes implemented in C++ (sample project)"
readme = "README.md"
requires-python = ">=3.11"

[build-system]
requires = ["scikit-build-core>=<最新>", "nanobind>=<最新>"]
build-backend = "scikit_build_core.build"

[tool.scikit-build]
minimum-version = "build-system.requires"
wheel.packages = ["src/cppsieve"]
cmake.build-type = "Release"
build-dir = "build/{wheel_tag}"

[tool.uv]
# C++ 変更時に uv sync で再ビルドさせる
cache-keys = [
  { file = "pyproject.toml" },
  { file = "CMakeLists.txt" },
  { file = "src/**/*.cpp" },
  { file = "src/**/*.hpp" },
]

[dependency-groups]
dev = ["pytest"]

[tool.pytest.ini_options]
testpaths = ["tests/python"]

[tool.cibuildwheel]
build = "cp311-* cp312-* cp313-* cp314-*"
skip = "*-musllinux_* *-win32 *_i686"
build-frontend = "build[uv]"
test-requires = ["pytest"]
test-command = "pytest {project}/tests/python"
```

### Step 4: CMakeLists.txt

- `find_package(Python 3.11 REQUIRED COMPONENTS Interpreter Development.Module)`
- nanobind は `python -m nanobind --cmake_dir` で場所を取得し `find_package(nanobind CONFIG REQUIRED)`
- コアは `add_library(sieve_core STATIC src/cpp/sieve.cpp)`、Linux では `POSITION_INDEPENDENT_CODE ON`
- `nanobind_add_module(_core NB_STATIC src/bindings/module.cpp)` → `sieve_core` をリンク
- `install(TARGETS _core LIBRARY DESTINATION cppsieve)`
- MSVC では `/W4 /utf-8`、GCC/Clang では `-Wall -Wextra` を付ける（`/utf-8` は日本語ロケール Windows での文字化け対策）

### Step 5: C++ 実装とバインディング

第2節の仕様どおりに実装する。

### Step 6: テスト（tests/python/test_sieve.py）

第2節の表の全ケースを `pytest.mark.parametrize` で網羅する。加えて:

- 戻り値が `list` であること
- 小さい n（0〜200）で、素朴な試し割り実装の結果と一致すること

### Step 7: ローカルでのビルド・テスト（WSL）

```bash
uv sync
uv run pytest -v
uv build            # sdist と Linux wheel（※この wheel は manylinux ではない）
```

- C++ を1行変更 → `uv run pytest` で再ビルドされることを確認する。

**完了条件**: 全テスト成功、`dist/` に sdist と wheel が生成される。

### Step 8: Linux wheel ビルドスクリプト（scripts/build_manylinux.sh）

`quay.io/pypa/manylinux_2_28_x86_64` コンテナ内で実行される前提。docker-in-docker を使わない方式とする（社内ランナーで privileged が許可されていない可能性があるため）。

処理内容:

1. `pip install uv`（またはコンテナ内 Python で uv を導入）
2. 対象バージョンごとに `/opt/python/cp3XX-cp3XX/bin/python` を使って `uv build --wheel --python <そのpython> -o dist/`
3. `auditwheel repair dist/*.whl -w wheelhouse/`
4. 各バージョンで `wheelhouse/` の wheel を新しい venv にインストールし pytest 実行
5. `set -euo pipefail` を使い、どこかで失敗したら非ゼロ終了

ローカルでも `docker run --rm -v "$PWD":/io -w /io quay.io/pypa/manylinux_2_28_x86_64 bash scripts/build_manylinux.sh` で同じことができることを README に書く。

### Step 9: .gitlab-ci.yml

```
stages: [test, build, publish]
```

| ジョブ | 内容 | 条件 |
|---|---|---|
| `test` | `ghcr.io/astral-sh/uv` 系の Python イメージで `uv sync --locked` → `uv run pytest`（build-essential・cmake を導入） | 全 push / MR |
| `build:linux` | manylinux イメージで `scripts/build_manylinux.sh`、`wheelhouse/*.whl` を artifacts に保存 | 全 push / MR |
| `build:sdist` | `uv build --sdist`、`dist/*.tar.gz` を artifacts に保存 | 全 push / MR |
| `publish` | `uv publish` で Linux wheel と sdist を Package Registry へ | **タグ push 時のみ** |

publish の設定:

```yaml
script:
  - uv publish
      --publish-url "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/pypi"
      --username gitlab-ci-token
      --password "${CI_JOB_TOKEN}"
      wheelhouse/*.whl dist/*.tar.gz
```

- 社内プロキシ環境を考慮し、`HTTP_PROXY` 等は CI/CD 変数で渡せるようにコメントで明記する。
- Windows ジョブは作らない（ローカルビルド方針）。将来ランナーを用意したときのために、`build:windows` ジョブ案をコメントアウトで残す（`scripts/build_windows.ps1` を呼ぶだけの形）。

### Step 10: Windows wheel ビルドスクリプト（scripts/build_windows.ps1）

Windows 側（WSL ではない）の PowerShell で実行する前提。前提ツール: uv、Visual Studio Build Tools（C++ ワークロード）、Git。

処理内容:

1. **クリーンなタグ付きコミットであることを検査**（`git status --porcelain` が空、`git describe --exact-match --tags` が成功）。満たさなければエラー終了。`-AllowDirty` スイッチで開発時のみスキップ可能にする。
2. `uvx cibuildwheel --platform windows --output-dir wheelhouse`
3. 生成された wheel のファイル名一覧と、`pyproject.toml` のバージョン・git タグが一致することを表示・検査する。

`$ErrorActionPreference = "Stop"` を設定すること。

### Step 11: Windows wheel のアップロード（scripts/publish.ps1）

- `uv publish` で `wheelhouse/*win_amd64.whl` のみを GitLab Package Registry にアップロードする。
- 認証情報は環境変数 `GITLAB_URL`, `GITLAB_PROJECT_ID`, `GITLAB_TOKEN`（Deploy Token または Personal Access Token）から読む。**スクリプトやリポジトリにトークンを書かない。**
- 未設定ならわかりやすいエラーで終了する。

### Step 12: README.md

以下を記載する。

- 概要と使用例（`from cppsieve import sieve; sieve(30)`）
- 開発手順（WSL）: `uv sync` / `uv run pytest`
- リリース手順:
  1. `pyproject.toml` のバージョンを更新しコミット
  2. タグ `vX.Y.Z` を push → CI が Linux wheel と sdist を公開
  3. Windows 機で該当タグを checkout し `scripts/build_windows.ps1` → `scripts/publish.ps1`
- 利用者側のインストール方法（`uv add cppsieve --index <GitLab Registry URL>` / `pyproject.toml` の `[[tool.uv.index]]` 設定例）
- WSL では `/mnt/c` 配下で作業しないこと

## 5. 受け入れ基準（最後に Claude が自己チェックする）

- [ ] WSL で `uv sync && uv run pytest` が全件成功
- [ ] C++ を変更後、`uv run pytest` で再ビルドされ変更が反映される
- [ ] `scripts/build_manylinux.sh` を manylinux コンテナで実行し、`manylinux_2_28_x86_64` タグの wheel が Python バージョン数分生成され、各テストが成功
- [ ] `.gitlab-ci.yml` が YAML として妥当（可能なら `gitlab-ci-lint` 相当の確認、少なくとも構文チェック）
- [ ] wheel のタグが正しい（`py3-none-any` になっていない）こと、1つの wheel に1プラットフォームのバイナリのみ含むことを `unzip -l` で確認
- [ ] PowerShell スクリプトは WSL 上では実行できないため、構文と手順の見直しのみ行い、未検証である旨を報告に明記
- [ ] リポジトリ内にトークン・社内 URL のハードコードがない

## 6. やらないこと

- 1つの wheel に Linux と Windows のバイナリを同梱する
- Linux から Windows 向けにクロスコンパイルする
- `sudo` やシステム設定の変更を確認なしに実行する
- 実際の Package Registry へのアップロード（スクリプト作成まで。実行は人が行う）

## 7. 完了報告に含めること

- 作成したファイル一覧
- 実行したコマンドとテスト結果の要約
- 未検証の項目（Windows スクリプト、実際の CI 実行など）と、人が確認すべき点
- 社内環境依存で調整が必要な箇所（プロキシ、ランナータグ、イメージのミラー先など）