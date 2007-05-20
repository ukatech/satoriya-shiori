#ifndef SAKURACS_H
#define SAKURACS_H

#include "../_/stltool.h"

// 共通の基底クラス
class SakuraCS
{
};



// io_targetからi_delimiterの直前までを切り出す。
// i_delimiterが見つからなければ、残り全てを切り出す。
string cut_token(string& io_target, const string& i_delimiter);
// 同上。ただし見つからなかった場合はfalseを返す。
bool cut_token(string& io_target, const string& i_delimiter, string& o_token);


#endif // SAKURACS_H
