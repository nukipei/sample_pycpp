cmake_minimum_required(VERSION 3.26...3.31)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Development.SABIModule は nanobind の STABLE_ABI (abi3) に必須（CMake 3.26+）
find_package(Python 3.11 REQUIRED COMPONENTS Interpreter Development.Module Development.SABIModule)

# nanobind の CMake パッケージの場所を Python から取得する
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE nanobind_ROOT
  COMMAND_ERROR_IS_FATAL ANY
)
find_package(nanobind CONFIG REQUIRED)

# 警告オプション（/utf-8 は日本語ロケール Windows での文字化け対策）
if(MSVC)
  set(SAMPLE_WARNINGS /W4 /utf-8)
else()
  set(SAMPLE_WARNINGS -Wall -Wextra)
endif()

# Python 非依存のコアライブラリ
add_library(sieve_core STATIC src/cpp/sieve.cpp)
target_include_directories(sieve_core PUBLIC src/cpp/include)
target_compile_options(sieve_core PRIVATE ${SAMPLE_WARNINGS})
if(NOT WIN32)
  set_target_properties(sieve_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Python 拡張モジュール
nanobind_add_module(_core STABLE_ABI FREE_THREADED NB_STATIC src/bindings/module.cpp)
target_link_libraries(_core PRIVATE sieve_core)
target_compile_options(_core PRIVATE ${SAMPLE_WARNINGS})

install(TARGETS _core LIBRARY DESTINATION sample)
