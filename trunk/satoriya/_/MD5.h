#ifndef MyMD5H
#define MyMD5H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
class MyMD5
{
protected:
    //フィールド
         unsigned int *FpX;
         
    //メソッド
         //補助メソッド
         unsigned int __fastcall F(unsigned int X, unsigned int Y, unsigned int Z);
         unsigned int __fastcall G(unsigned int X, unsigned int Y, unsigned int Z);
         unsigned int __fastcall H(unsigned int X, unsigned int Y, unsigned int Z);
         unsigned int __fastcall I(unsigned int X, unsigned int Y, unsigned int Z);
    
         unsigned int __fastcall Round(unsigned int a, unsigned int b, unsigned int FGHI,
                                       unsigned int k, unsigned int s, unsigned int i);
    
         void __fastcall Round1(unsigned int &a, unsigned int b, unsigned int c, unsigned int d,
                                unsigned int k,  unsigned int s, unsigned int i);
         void __fastcall Round2(unsigned int &a, unsigned int b, unsigned int c, unsigned int d,
                                unsigned int k,  unsigned int s, unsigned int i);
         void __fastcall Round3(unsigned int &a, unsigned int b, unsigned int c, unsigned int d,
                                unsigned int k,  unsigned int s, unsigned int i);
         void __fastcall Round4(unsigned int &a, unsigned int b, unsigned int c, unsigned int d,
                                unsigned int k,  unsigned int s, unsigned int i);

         void __fastcall MD5_Round_Calculate(const unsigned char *block,
                                             unsigned int &A, unsigned int &B, unsigned int &C, unsigned int &D);

public:
    //メソッド
          //基本メソッド output require 33 bytes
          void __fastcall MD5_String(void * data, unsigned long size, char * output);
};
//---------------------------------------------------------------------------
#endif