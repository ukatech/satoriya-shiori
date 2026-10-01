# sodate.exe、sodate_setup.exe と説明書をまとめて tmp\sodate.zip を作る。
# 先に msdev で sodate（sodate\network_updater.dsw）と sodate_setup（sodate_setup\network_updater_setup.dsw）を Release でビルドしておくこと。7z が必要。

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2
. (Join-Path $PSScriptRoot 'make_zip_common.ps1')

# zip の中の名前 → satoriya からのパス
$files = [ordered]@{
	'sodate.exe'        = 'sodate\Release\sodate.exe'
	'sodate_setup.exe'  = 'sodate_setup\Release\sodate_setup.exe'
	'sodate_readme.txt' = 'sodate\sodate_readme.txt'
}

Test-ZipFiles $PSScriptRoot $files
$zip = New-ReleaseZip $PSScriptRoot 'sodate' $files
Write-Host "$zip を作りました。"
