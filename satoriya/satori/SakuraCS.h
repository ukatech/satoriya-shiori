#ifndef SAKURACS_H
#define SAKURACS_H

#include "../_/stltool.h"

// 共通の基底クラス
class SakuraCS
{
};



// io_targetからi_delimiterの直前までを切り出す。
// i_delimiterが見つからなければ、残り全てを切り出す。
wstring cut_token(wstring& io_target, const wstring& i_delimiter);
// 同上。ただし見つからなかった場合はfalseを返す。
bool cut_token(wstring& io_target, const wstring& i_delimiter, wstring& o_token);

// 生のリクエスト/レスポンス（バイト列）からCharsetヘッダの値を探す。見つからなければ空文字列。
std::string find_charset_header(const std::string& i_bytes);


#endif // SAKURACS_H
