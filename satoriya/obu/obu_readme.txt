
おぶ


・SAORI/1.0に準拠してます。

・ユーザがブラウザでサイトを訪れたときに、
　イベント OnWebsiteVisit を送ります（DirectSSTP使用）。
　ref0はURL、ref1はタイトルです。

・引数（コマンド）を指定して呼び出すことで、以下の操作ができます。

　getURL
　
　　現在のURLを取得します。
　
　
　getTitle
　
　　現在のタイトルを取得します。
　
　　
　setFrequency
　
　　第２引数に秒数を指定してください。
　　0の場合は何もしません。
　　
　　指定した間隔で、イベント OnWebsiteStay を送ります。
　　ref0はURL、ref1はタイトル、ref2が滞在秒数です。
　
　getFrequency
　
　　setFrequencyで指定した秒数を取得します。


・全てのブラウザに対応してるわけではありません。
　IE, NN, Mozilla, いくつかのタブブラウザには対応しているはずです。
　動作はIE5.5とMozilla1.0で確認。


作者: 櫛ヶ浜やぎ
配布元: http://www.geocities.co.jp/SiliconValley-Cupertino/8536/
