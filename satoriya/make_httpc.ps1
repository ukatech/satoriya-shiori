# httpc.dll と説明書をまとめて tmp\httpc.zip を作る。
# 先に msdev で httpc（httpc.dsw）を Release でビルドしておくこと。7z が必要。

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2
. (Join-Path $PSScriptRoot 'make_zip_common.ps1')

# zip の中の名前 → satoriya からのパス
$files = [ordered]@{
	'httpc.dll'        = 'httpc\Release\httpc.dll'
	'httpc_readme.txt' = 'httpc\httpc.txt'
}

Test-ZipFiles $PSScriptRoot $files
$zip = New-ReleaseZip $PSScriptRoot 'httpc' $files
Write-Host "$zip を作りました。"
