# obu.dll と説明書をまとめて tmp\obu.zip を作る。
# 先に msdev で obu（obu.dsw）を Release でビルドしておくこと。7z が必要。

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2
. (Join-Path $PSScriptRoot 'make_zip_common.ps1')

# zip の中の名前 → satoriya からのパス
$files = [ordered]@{
	'obu.dll'        = 'obu\Release\obu.dll'
	'obu_readme.txt' = 'obu\obu_readme.txt'
}

Test-ZipFiles $PSScriptRoot $files
$zip = New-ReleaseZip $PSScriptRoot 'obu' $files
Write-Host "$zip を作りました。"
