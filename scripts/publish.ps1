<#
.SYNOPSIS
    wheelhouse/ の Windows wheel (win_amd64) を GitLab Package Registry にアップロードする。

.DESCRIPTION
    認証情報は以下の環境変数から読む（スクリプトやリポジトリにトークンを書かないこと）。
      GITLAB_URL         例: https://gitlab.example.com
      GITLAB_PROJECT_ID  数値のプロジェクト ID
      GITLAB_TOKEN       Deploy Token または Personal Access Token（api / write_package_registry スコープ）
      GITLAB_USERNAME    （任意）Deploy Token のユーザー名。PAT の場合は省略可（既定: __token__）

.EXAMPLE
    $env:GITLAB_URL = "https://gitlab.example.com"
    $env:GITLAB_PROJECT_ID = "123"
    $env:GITLAB_TOKEN = "<token>"
    .\scripts\publish.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

$missing = @("GITLAB_URL", "GITLAB_PROJECT_ID", "GITLAB_TOKEN") | Where-Object {
    -not [Environment]::GetEnvironmentVariable($_)
}
if ($missing) {
    throw "環境変数が未設定です: $($missing -join ', ')`n例: `$env:GITLAB_URL = 'https://gitlab.example.com'"
}

$wheels = @(Get-ChildItem "wheelhouse" -Filter "*win_amd64.whl" -ErrorAction SilentlyContinue)
if ($wheels.Count -eq 0) {
    throw "wheelhouse/ に win_amd64 の wheel がありません。先に scripts/build_windows.ps1 を実行してください。"
}

$baseUrl = $env:GITLAB_URL.TrimEnd("/")
$publishUrl = "$baseUrl/api/v4/projects/$($env:GITLAB_PROJECT_ID)/packages/pypi"
$username = if ($env:GITLAB_USERNAME) { $env:GITLAB_USERNAME } else { "__token__" }

Write-Host "アップロード先: $publishUrl"
$wheels | ForEach-Object { Write-Host "  $($_.Name)" }

# トークンはコマンドライン引数に載せず環境変数で uv に渡す
$env:UV_PUBLISH_USERNAME = $username
$env:UV_PUBLISH_PASSWORD = $env:GITLAB_TOKEN
# PowerShell 5.1 ではネイティブコマンドの stderr 出力が Stop 設定下でエラー扱いになるため一時的に Continue にする
$ErrorActionPreference = "Continue"
try {
    uv publish --publish-url $publishUrl ($wheels | ForEach-Object { $_.FullName })
    $exitCode = $LASTEXITCODE
} finally {
    $ErrorActionPreference = "Stop"
    Remove-Item Env:UV_PUBLISH_USERNAME, Env:UV_PUBLISH_PASSWORD -ErrorAction SilentlyContinue
}
if ($exitCode -ne 0) { throw "uv publish が失敗しました (exit $exitCode)。" }
Write-Host "完了"
