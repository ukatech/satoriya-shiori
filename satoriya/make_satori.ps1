# satori.dll、satorite.exe、saori\ssu.dll をまとめて tmp\satori.zip を作る。
# 先に msdev で satori / satorite / ssu を Release でビルドしておくこと。7z が必要。
# 作る前に、3つのファイルバージョンがそろっているか、satori.cpp の gSatoriVersion と
# ビルドした satori.dll / satorite.exe の中のバージョンが resource.rc と一致するかを確かめる。

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2
. (Join-Path $PSScriptRoot 'make_zip_common.ps1')

$root = $PSScriptRoot

# zip の中の名前 → ビルド出力
$files = [ordered]@{
	'satori.dll'    = 'satori\Release\satori.dll'
	'satorite.exe'  = 'satori\Release_ST\satorite.exe'
	'saori\ssu.dll' = 'satori\Release_SU\ssu.dll'
}

Test-ZipFiles $root $files

# 3つのファイルバージョンがそろっているか
$versions = @()
foreach ( $name in $files.Keys ) {
	$versions += (Get-Item (Join-Path $root $files[$name])).VersionInfo.FileVersion -replace '\s', ''
}
if ( @($versions | Select-Object -Unique).Count -ne 1 ) {
	throw 'ファイルバージョンがそろっていません。3つともビルドし直してください。'
}

# FileVersion X,XYY,Z,1 → McXYY-Z
$parts = $versions[0].Split(',')
$mc = "Mc$($parts[1])-$($parts[2])"

# satori.cpp の gSatoriVersion（ASCII の部分だけ読めればよいので Latin-1 で読む）
$latin1 = [System.Text.Encoding]::GetEncoding(28591)
$src = [System.IO.File]::ReadAllText((Join-Path $root 'satori\satori.cpp'), $latin1)
if ( $src -notmatch 'gSatoriVersion\s*=\s*L"phase (Mc\d+-\d+)"' ) {
	throw 'satori.cpp の gSatoriVersion が読めません。'
}
if ( $Matches[1] -ne $mc ) {
	throw "resource.rc（$mc）と satori.cpp（$($Matches[1])）のバージョンが違います。"
}

# ビルドした satori.dll / satorite.exe に L"phase McXYY-Z" が入っているか（ビルドし忘れの検出）
$needle = $latin1.GetString([System.Text.Encoding]::Unicode.GetBytes("phase $mc"))
foreach ( $name in @('satori.dll', 'satorite.exe') ) {
	$bytes = [System.IO.File]::ReadAllBytes((Join-Path $root $files[$name]))
	if ( $latin1.GetString($bytes).IndexOf($needle, [System.StringComparison]::Ordinal) -lt 0 ) {
		throw "$name に phase $mc が入っていません。ビルドし直してください。"
	}
}

$zip = New-ReleaseZip $root 'satori' $files
Write-Host "$zip を作りました（$mc）。"
