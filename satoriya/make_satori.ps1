# satori.dll、satorite.exe、saori\ssu.dll をまとめて tmp\satori.zip を作る。
# 先に msdev で satori / satorite / ssu を Release でビルドしておくこと。7z が必要。
# 作る前に、3つのファイルバージョンがそろっているか、satori.cpp の gSatoriVersion と
# ビルドした satori.dll / satorite.exe の中のバージョンが resource.rc と一致するかを確かめる。

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2

$root = $PSScriptRoot

# zip の中の名前 → ビルド出力
$files = [ordered]@{
	'satori.dll'    = 'satori\Release\satori.dll'
	'satorite.exe'  = 'satori\Release_ST\satorite.exe'
	'saori\ssu.dll' = 'satori\Release_SU\ssu.dll'
}

# 7z を探す
$sevenZip = Get-Command 7z -ErrorAction SilentlyContinue
if ( $sevenZip ) {
	$sevenZip = $sevenZip.Source
}
else {
	$sevenZip = Join-Path $env:ProgramFiles '7-Zip\7z.exe'
	if ( -not (Test-Path $sevenZip) ) {
		throw '7z が見つかりません。7-Zip を入れて PATH を通してください。'
	}
}

# ファイルの存在とファイルバージョン
$versions = @()
foreach ( $name in $files.Keys ) {
	$path = Join-Path $root $files[$name]
	if ( -not (Test-Path $path) ) {
		throw "$path がありません。Release でビルドしてください。"
	}
	$item = Get-Item $path
	$v = $item.VersionInfo.FileVersion -replace '\s', ''
	$versions += $v
	Write-Host ('{0,-14} {1,-12} {2}' -f $name, $v, $item.LastWriteTime)
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

# tmp\satori に並べて zip にする
$tmp = Join-Path $root 'tmp'
$stage = Join-Path $tmp 'satori'
$zip = Join-Path $tmp 'satori.zip'
if ( Test-Path $stage ) { Remove-Item $stage -Recurse -Force }
if ( Test-Path $zip ) { Remove-Item $zip -Force }

foreach ( $name in $files.Keys ) {
	$dest = Join-Path $stage $name
	New-Item -ItemType Directory -Force (Split-Path $dest) | Out-Null
	Copy-Item (Join-Path $root $files[$name]) $dest
}

Push-Location $stage
try {
	& $sevenZip a -y -tzip -mx=9 -mmt=on $zip * | Out-Null
	if ( $LASTEXITCODE -ne 0 ) {
		throw "7z が失敗しました（終了コード $LASTEXITCODE）。"
	}
}
finally {
	Pop-Location
}

Write-Host "$zip を作りました（$mc）。"
