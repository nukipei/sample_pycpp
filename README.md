# sample

C++（nanobind）で実装したエラトステネスのふるいを Python から呼び出すサンプルパッケージです。
開発は Windows ネイティブ / WSL（Ubuntu）のどちらでも可能です。
ビルドは uv + scikit-build-core、Linux wheel は GitLab CI（cibuildwheel + manylinux_2_28）、Windows wheel はローカルの Windows 環境（cibuildwheel）でビルドします。

## 使用例

```python
from sample import sieve

sieve(30)  # [2, 3, 5, 7, 11, 13, 17, 19, 23, 29]
```

- `n < 0` → `ValueError`
- int 以外（`2.5`, `"10"` 等）→ `TypeError`

### 巡回セールスマン問題（TSP）

bit DP による厳密解です。OR-Tools と同様に、モデルを作って設定し `solve()` で求解します。

```python
from sample import TspModel

model = TspModel(num_nodes=4, depot=0)
model.set_distance_matrix([
    [0, 10, 15, 20],
    [10, 0, 35, 25],
    [15, 35, 0, 30],
    [20, 25, 30, 0],
])
# model.set_distance(0, 1, 10)            # 個別設定
# model.set_distance_callback(lambda i, j: ...)  # コールバックでの設定（solve 時に評価）

solution = model.solve()
solution.route     # [0, 2, 3, 1, 0]（同距離の逆回りが最適になる場合もある）
solution.distance  # 80
```

- ノード数の上限は `MAX_TSP_NODES`（16、デポを含む）。超えると `ValueError`
- 距離は 0 以上 1e12 以下の整数（非対称でも可）
- 範囲外の値・形状不正 → `ValueError`、int 以外 → `TypeError`

## 開発手順（Windows ネイティブ）

WSL を使わず、Windows（PowerShell）上でそのまま開発・ビルド・テストできます。

前提ツール:

- uv
- Visual Studio Build Tools（「C++ によるデスクトップ開発」ワークロード。MSVC が必要）
- Git

```powershell
uv sync             # C++ 拡張をビルドして editable インストール
uv run pytest -v
uv build            # sdist と wheel
```

- 日本語ロケールの Windows でも文字化けしないよう、MSVC には `/utf-8` を指定しています（`CMakeLists.txt`）。
- 配布用の Windows wheel は `.\scripts\build_windows.ps1` でビルドします（[リリース手順](#リリース手順)を参照）。

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

CI と同じ処理を cibuildwheel で実行できます（Docker が必要）。

```bash
uvx cibuildwheel --platform linux --output-dir wheelhouse
```

対象バージョンやテストは `pyproject.toml` の `[tool.cibuildwheel]` で管理しています。
`wheelhouse/` に次の wheel が生成され、それぞれ manylinux コンテナ内でテストされます。

- `sample-<ver>-cp311-cp311-manylinux_*_x86_64.whl`
- `sample-<ver>-cp312-abi3-manylinux_*_x86_64.whl`（Stable ABI。Python 3.12 以降で共用）
- `sample-<ver>-cp314-cp314t-manylinux_*_x86_64.whl`（free-threaded）

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

ビルド済み wheel（Linux `manylinux_2_28` x86_64 / Windows `win_amd64`）を特定のディレクトリに配置して配布します。利用者側に C++ コンパイラは不要です。
以下では wheel の配置先を `<wheel-dir>` と表記します（例: `/srv/wheels`、`\fileserver\share\wheels`、`C:\wheels`）。

| Python | 使われる wheel |
| --- | --- |
| 3.11 | `cp311-cp311` |
| 3.12 / 3.13 / 3.14 | `cp312-abi3`（Stable ABI） |
| 3.14 free-threaded（3.14t） | `cp314-cp314t` |

`<wheel-dir>` には Linux / Windows 両方の wheel を同居させて構いません。pip / uv が実行環境に合うものを自動で選びます。

`sample` は一般的な名前で公開 PyPI に同名パッケージが存在し得るため、以下の例では `--no-index` で PyPI を参照せず、`<wheel-dir>` だけから取得します（`sample` は依存パッケージを持ちません）。

### pip

```bash
pip install sample --no-index --find-links <wheel-dir>
```

バージョンを固定する場合は `sample==0.1.0` とします。

### uv

プロジェクトに追加する場合:

```bash
uv add sample --no-index --find-links <wheel-dir>
```

または `pyproject.toml` に設定して `uv add sample`:

```toml
[[tool.uv.index]]
name = "wheels"
url = "<wheel-dir>"      # 絶対パス、または file:///... 形式
format = "flat"
explicit = true

[tool.uv.sources]
sample = { index = "wheels" }
```

`explicit = true` と `[tool.uv.sources]` により、`sample` だけをこのディレクトリから取得し、他のパッケージは通常の PyPI から取得されます。

プロジェクトを作らず現在の環境に入れる場合（pip 互換）:

```bash
uv pip install sample --no-index --find-links <wheel-dir>
```

### wheel ファイルを直接指定

```bash
pip install <wheel-dir>/sample-0.1.0-cp312-abi3-win_amd64.whl
uv pip install <wheel-dir>/sample-0.1.0-cp312-abi3-win_amd64.whl
```

### 動作確認

```bash
python -c "from sample import sieve; print(sieve(30))"
```

## 構成

```text
src/cpp/          Python 非依存の C++ コア（sieve / tsp）
src/bindings/     nanobind バインディング（拡張モジュール _core）
src/sample/       Python パッケージ（__init__.py, _core.pyi, py.typed）
tests/python/     pytest
scripts/          build_windows.ps1 / publish.ps1
```
