<#
.SYNOPSIS
    Windows (win_amd64) 用 wheel を cibuildwheel でローカルビルドする。

.DESCRIPTION
    Windows の PowerShell（WSL ではない）で、リポジトリルートから実行する。
    前提: uv, Visual Studio Build Tools (C++ ワークロード), Git

.PARAMETER AllowDirty
    開発時のみ: 未コミット変更・タグ無しコミットでもビルドを許可する。

.EXAMPLE
    .\scripts\build_windows.ps1
    .\scripts\build_windows.ps1 -AllowDirty
#>
[CmdletBinding()]
param(
    [switch]$AllowDirty
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

function Invoke-Native {
    param([scriptblock]$Command)
    # PowerShell 5.1 ではネイティブコマンドの stderr 出力が Stop 設定下でエラー扱いになるため、
    # 実行中のみ Continue にして終了コードで成否を判定する
    $prev = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $Command
    } finally {
        $ErrorActionPreference = $prev
    }
    if ($LASTEXITCODE -ne 0) {
        throw "コマンドが失敗しました (exit $LASTEXITCODE): $Command"
    }
}

# pyproject.toml の version を取得
$versionLine = Select-String -Path "pyproject.toml" -Pattern '^\s*version\s*=\s*"([^"]+)"' | Select-Object -First 1
if (-not $versionLine) {
    throw "pyproject.toml から version を取得できませんでした。"
}
$Version = $versionLine.Matches[0].Groups[1].Value
Write-Host "pyproject.toml version: $Version"

# 1. クリーンなタグ付きコミットであることを検査
$Tag = $null
if ($AllowDirty) {
    Write-Warning "-AllowDirty 指定: クリーン/タグ検査をスキップします（開発用。公開しないこと）。"
} else {
    $status = git status --porcelain
    if ($LASTEXITCODE -ne 0) { throw "git status に失敗しました。" }
    if ($status) {
        throw "作業ツリーに未コミットの変更があります。コミットするか -AllowDirty を指定してください。`n$($status -join "`n")"
    }
    $ErrorActionPreference = "Continue"
    $Tag = git describe --exact-match --tags HEAD 2>$null
    $ErrorActionPreference = "Stop"
    if ($LASTEXITCODE -ne 0 -or -not $Tag) {
        throw "HEAD にタグが付いていません。リリースタグ (vX.Y.Z) を checkout してください。"
    }
    $Tag = $Tag.Trim()
    Write-Host "git tag: $Tag"
    if ($Tag -ne "v$Version") {
        throw "git タグ ($Tag) と pyproject.toml の version (v$Version) が一致しません。"
    }
}

# 2. cibuildwheel でビルド
if (Test-Path "wheelhouse") {
    Get-ChildItem "wheelhouse" -Filter "*win_amd64.whl" | Remove-Item -Force
}
Invoke-Native { uvx cibuildwheel --platform windows --output-dir wheelhouse }

# 3. 生成物の検査
$wheels = @(Get-ChildItem "wheelhouse" -Filter "*win_amd64.whl")
if ($wheels.Count -eq 0) {
    throw "win_amd64 の wheel が生成されていません。"
}
Write-Host ""
Write-Host "生成された wheel:"
foreach ($w in $wheels) {
    Write-Host "  $($w.Name)"
    $wheelVersion = ($w.Name -split "-")[1]
    if ($wheelVersion -ne $Version) {
        throw "wheel のバージョン ($wheelVersion) が pyproject.toml ($Version) と一致しません: $($w.Name)"
    }
}
Write-Host ""
Write-Host "OK: version=$Version tag=$(if ($Tag) { $Tag } else { '(skip)' }) wheels=$($wheels.Count)"
