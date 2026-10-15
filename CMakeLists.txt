# このファイルは C++ 部分のビルド手順を書いた CMake の設定です。
# 普段は自分で cmake を叩く必要はなく、`uv sync` や `pip install .` を実行すると
# scikit-build-core（pyproject.toml 参照）が自動でこのファイルを読み込んでビルドします。

# 必要な CMake の最低バージョン。「3.26...3.31」は 3.26 以上で、3.31 までのルールに合わせて動かす、という意味。
cmake_minimum_required(VERSION 3.26...3.31)

# プロジェクト名とバージョンは pyproject.toml の [project] からそのまま受け取る。
# SKBUILD_ で始まる変数は scikit-build-core が用意してくれるもの。
# こうしておくと、名前やバージョンを 2 か所で書き直さずに済む。
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION} LANGUAGES CXX)

# C++17 でコンパイルする。
set(CMAKE_CXX_STANDARD 17)
# 17 が使えないコンパイラの場合は、黙って古い規格に落とさずエラーにする。
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# GNU 拡張などのコンパイラ独自機能を使わず、標準 C++ だけで書く（別のコンパイラでも動きやすくなる）。
set(CMAKE_CXX_EXTENSIONS OFF)

# Python 本体とビルドに必要な部品を探す。
#   Interpreter            : python コマンド本体
#   Development.Module     : 拡張モジュールを作るためのヘッダやライブラリ
#   Development.SABIModule : Stable ABI（abi3）用の部品
# Development.SABIModule は nanobind の STABLE_ABI を使うために必須（CMake 3.26 以降で利用可）。
find_package(Python 3.11 REQUIRED COMPONENTS Interpreter Development.Module Development.SABIModule)

# nanobind の CMake 用ファイルが置かれている場所を、Python に問い合わせて取得する。
# 取得した場所を nanobind_ROOT に入れておくと、次の find_package が見つけてくれる。
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE nanobind_ROOT
  COMMAND_ERROR_IS_FATAL ANY
)
find_package(nanobind CONFIG REQUIRED)

# コンパイラの警告オプション。
# MSVC（Windows の Visual C++）と、GCC / Clang（Linux など）ではオプションの書き方が違うので分けている。
#   /W4          : MSVC で警告を多めに出す
#   /utf-8       : ソースを UTF-8 として読む。日本語ロケールの Windows だと文字化けや警告が出やすいための対策
#   -Wall -Wextra: GCC / Clang で警告を多めに出す
if(MSVC)
  set(SAMPLE_WARNINGS /W4 /utf-8)
else()
  set(SAMPLE_WARNINGS -Wall -Wextra)
endif()

# 素数を求める処理と巡回セールスマン問題(TSP)の処理本体を「静的ライブラリ（STATIC）」としてビルドする。
# Python に一切依存しない作りにしてあるので、C++ だけのテストや他のプログラムからも再利用できる。
add_library(sieve_core STATIC src/cpp/sieve.cpp src/cpp/tsp.cpp)
# ヘッダファイルの置き場所。PUBLIC にしてあるので、このライブラリを使う側からも
# #include <sample/sieve.hpp> のように読み込める。
target_include_directories(sieve_core PUBLIC src/cpp/include)
# 警告オプションはこのライブラリ自身のビルドだけに使う（PRIVATE）。
target_compile_options(sieve_core PRIVATE ${SAMPLE_WARNINGS})
# Linux では、静的ライブラリを共有ライブラリ（.so）に取り込むために
# 位置独立コード（PIC）として作る必要がある。Windows では不要なので除外している。
if(NOT WIN32)
  set_target_properties(sieve_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Python から import できる拡張モジュール（_core）を作る。
#   STABLE_ABI  : Python 3.12 以降で共通の abi3 形式にする。Python のバージョンごとにビルドし直さなくてよい
#   FREE_THREADED: GIL なし（free-threaded）版の Python 3.13 以降でも動くようにする
#   NB_STATIC   : nanobind 本体を拡張モジュールに埋め込む。利用者が別途インストールする必要がなくなる
nanobind_add_module(_core STABLE_ABI FREE_THREADED NB_STATIC src/bindings/module.cpp)
# 先ほど作った sieve_core をリンクして、Python から呼べるようにする。
target_link_libraries(_core PRIVATE sieve_core)
target_compile_options(_core PRIVATE ${SAMPLE_WARNINGS})

# ビルドした _core を wheel 内の sample パッケージの中へ配置する。
# これで Python 側から `from sample import _core` のように読み込める。
install(TARGETS _core LIBRARY DESTINATION sample)
