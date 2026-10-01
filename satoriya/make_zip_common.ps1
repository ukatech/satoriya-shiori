# make_*.ps1 から読み込む（. で取り込む）共通の処理。単体では何もしない。
# $files は「zip の中の名前 → satoriya からのパス」の [ordered] ハッシュ。

# 7z を探す
function Find-SevenZip {
	$cmd = Get-Command 7z -ErrorAction SilentlyContinue
	if ( $cmd ) {
		return $cmd.Source
	}
	$path = Join-Path $env:ProgramFiles '7-Zip\7z.exe'
	if ( Test-Path $path ) {
		return $path
	}
	throw '7z が見つかりません。7-Zip を入れて PATH を通してください。'
}

# 同梱するファイルがそろっているか確かめ、ファイルバージョンと更新日時を表示する
function Test-ZipFiles($root, $files) {
	foreach ( $name in $files.Keys ) {
		$path = Join-Path $root $files[$name]
		if ( -not (Test-Path $path) ) {
			throw "$path がありません。Release でビルドしてください。"
		}
		$item = Get-Item $path
		$v = $item.VersionInfo.FileVersion
		if ( $v ) { $v = $v -replace '\s', '' } else { $v = '-' }
		Write-Host ('{0,-20} {1,-12} {2}' -f $name, $v, $item.LastWriteTime)
	}
}

# $files を tmp\<名前> に並べて tmp\<名前>.zip を作り、zip のパスを返す
function New-ReleaseZip($root, $zipName, $files) {
	$sevenZip = Find-SevenZip
	$tmp = Join-Path $root 'tmp'
	$stage = Join-Path $tmp $zipName
	$zip = Join-Path $tmp "$zipName.zip"
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
	return $zip
}
