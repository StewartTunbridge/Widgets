///////////////////////////////////////////////////////////////////////////////
//
// LIBRARY
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////
//
// History
// -------
// Remove references to SDL (done not tested)
//
///////////////////////////////////////////////////////////////////////////////


#include "lib.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#ifdef _Windows
  #include <io.h>
  #include <sysinfoapi.h>
#else
  #include <unistd.h>
  #include <linux/rtc.h>
  #include <sys/time.h>
  #include <errno.h>
#endif
#include <sys/stat.h>
#include <dirent.h>


const unsigned int Bit [32] =
  {
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
    0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000,
    0x1000000, 0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000
  };

const char *BoolText [2] = {"False", "True"};

#ifdef _Windows
  char PathChar = '\\';
#else
  char PathChar = '/';
#endif


///////////////////////////////////////////////////////////////////////////////
//
// COLOUR

int ColourAverage (int Colour1, int Colour2)
  {
    int Res, Mask;
    //
    Res = 0;
    Mask = 0xFF0000;
    while (Mask)
      {
        Res |= (((Colour1 & Mask) + (Colour2 & Mask)) / 2) & Mask;
        Mask >>= 8;
      }
    return Res;
  }

int RGBToColour (byte r, byte g, byte b)
  {
    return ((int) b << 16) | ((int) g << 8) | ((int) r);
  }

// Adjust Colour up/down: Pcnt 100=unchanged, 0=Black, 200=White
int ColourAdjust (int Colour, int Pcnt)
  {
    unsigned int Mask;
    unsigned int c;
    //
    if (Pcnt == 100)
      return Colour;
    if (Pcnt <= 0)
      return cBlack;
    if (Pcnt >= 200)
      return cWhite;
    Mask = 0xFF0000;
    while (Mask)
      {
        c = Colour & Mask;
        if (Pcnt <= 100)
          c = c * Pcnt / 100;
        else
          c = Mask - ((Mask - c) * (200 - Pcnt) / 100);
        Colour = (Colour & ~Mask) | (Min (c, Mask) & Mask);
        Mask = Mask >> 8;
      }
    return Colour;
  }

byte Grad (byte Col1, byte Col2, int GradNum, int GradDen)
  {
    int x = Col1 + ((Col2 - Col1) * GradNum + GradDen / 2) / GradDen;
    return x;
  }

int ColourGraded (int Colour1, int Colour2, int GradNum, int GradDen)
  {
    byte r = Grad (ColourR (Colour1), ColourR (Colour2), GradNum, GradDen);
    byte g = Grad (ColourG (Colour1), ColourG (Colour2), GradNum, GradDen);
    byte b = Grad (ColourB (Colour1), ColourB (Colour2), GradNum, GradDen);
    return RGBToColour (r, g, b);
  }


/*
///////////////////////////////////////////////////////////////////////////////
//
// List Template

template <class T>
_List::_List (_List **Start)
  {
    _List *Sibling;
    //
    Next = Prev = NULL;
    if (*Start)
      {
        Sibling = *Start;
        while (Sibling->Next)
          Sibling = Sibling->Next;
        Sibling->Next = this;
        Prev = Sibling;
      }
    else
      *Start = this;
  }

void _List::Unlink (_List **Start)
  {
    // Destroy children
//?    while (Children)
// ?     delete Children;
    // Unlink from family
    if (Prev)
      Prev->Next = Next;
    else // first child
      if (*Start)
        *Start = Next;
    if (Next)
      Next->Prev = Prev;
  }

/*
//create _Tree class
_List* _List::AddChild ()
  {
    _List *NewNode, *Sibling;
    //
    NewNode = new _List ();
    NewNode->Parent = this;
    if (Children == nullptr)
      Children = NewNode;
    else
      {
        Sibling = Children;
        while (Sibling->Next)
          Sibling = Sibling->Next;
        Sibling->Next = NewNode;
        NewNode->Prev = Sibling;
      }
  }


??????????????????????
void _List::AddStart (void)
  {
    _List *Node;
    //
    Node = this;
    while (Prev)
      Node =
    Prev = NULL;
    Next = *Start;
    if (*Start)
      (*Start)->Prev = this;
    *Start = this;
  }

void _List::AddEnd (_List **Start)
  {
    // search to end first
    Prev = NULL;
    Next = *Start;
    if (*Start)
      (*Start)->Prev = this;
    *Start = this;
  }

void _List::Remove (_List **Start)
  {
    if (Prev)
      {
        Prev->Next = Next;
        Next->Prev = Prev;
      }
    else
      {
        *Start = Next;
        if (*Start)
          (*Start)->Prev = NULL;
      }
  }
*/


///////////////////////////////////////////////////////////////////////////////
//
// Dynamic Array: Element [0] is the size of user data. [1] is the start, indexed from 0.
//
// Initialize Array to NULL
// This really annoys Paul until I make it a c++ class

int ArrayGet (int **Array, int Index, int Default)
  {
    if (*Array == NULL || Index >= **Array)
      return Default;
    if ((*Array) [Index + 1] == ArrayItemUndefined)
      return Default;
    return (*Array) [Index + 1];
  }

void ArraySet (int **Array, int Index, int Value)
  {
    int *ArrayNew;
    int i;
    //
    if (*Array == NULL || Index >= **Array)
      {
        ArrayNew = (int *) malloc ((Index + 2) * sizeof (int));
        for (i = 0; i <= Index; i++)
          ArrayNew [i + 1] = ArrayItemUndefined;
        if (*Array)
          {
            for (i = 0; i < **Array; i++)
              ArrayNew [i + 1] = (*Array) [i + 1];
            free (*Array);
          }
        *Array = ArrayNew;
        *Array [0] = Index + 1;
      }
    (*Array) [Index + 1] = Value;
  }

// Now, as a class
/*
class _DynamicArray
  {
    private:
      int DataSize;
      int *Data;
      //
      _DynamicArray (void);
      ~_DynamicArray (void);
    public:
      int Read (int Index, int Default);
      void Write (int Index, int Value);
  };

_DynamicArray::_DynamicArray (void)
  {
    DataSize = 0;
    Data = nullptr;
  }

_DynamicArray::~_DynamicArray (void)
  {
    if (Data)
      free (Data);
  }

int _DynamicArray::Read (int Index, int Default)
  {
    if (Index >= DataSize)
      return Default;
    if (Data [Index] == IntMin)   // undefined
      return Default;
    return Data [Index];
  }

void _DynamicArray::Write (int Index, int Value)
  {
    int *DataNew;
    int i;
    //
    if (Index >= DataSize)
      {
        DataNew = new int [Index + 1];
        for (i = 0; i <= Index; i++)
          DataNew [i] = IntMin;
        if (Data)
          {
            for (i = 0; i < DataSize; i++)
              DataNew [i] = Data [i];
            delete[] Data;
          }
        Data = DataNew;
        DataSize = Index + 1;
      }
    Data [Index] = Value;
  }
*/


//////////////////////////////////////////////////////////////////////
//
// Characters

bool IsDigit (char c)
  {
    return (c >= '0') && (c <= '9');
  }

bool IsAlpha (char c)
  {
    if ((c >= 'A') && (c <= 'Z'))
      return true;
    if ((c >= 'a') && (c <= 'z'))
      return true;
    return false;
  }

char UpCase (char c)
  {
    if (IsAlpha (c))
      c &= ~('A' ^ 'a');
    return c;
  }

// Read a UTF8 character from a string, advancing the pointer Ch

byte UTF8HeaderA [] = {0x80, 0xE0, 0xF0, 0xF8};
byte UTF8HeaderB [] = {0x00, 0xC0, 0xE0, 0xF0};

int UTF8Read (char **Ch)
  {
    int Ch_;
    int type;
    //
    type = 0;
    while (true)
      {
        if (**Ch == 0)
          return 0;
        if (type > SIZEARRAY (UTF8HeaderA))
          {
            Ch_ = -1;
            break;
          }
        if ((**Ch & UTF8HeaderA [type]) == UTF8HeaderB [type])   // match
          {
            Ch_ = ((byte) **Ch) & ~UTF8HeaderA [type];
            (*Ch)++;
            while (type-- > 0)
              if ((((byte) **Ch) & 0xC0) == 0x80)   // ok
                {
                  Ch_ = (Ch_ << 6) | (((byte) **Ch) & 0x3F);
                  (*Ch)++;
                }
              else
                {
                  Ch_ = -1;
                  break;
                }
            break;
          }
        type++;
      }
    return Ch_;
  }

int UTF8Next (char *St, int Index)
  {
    char *p;
    //
    p = &St [Index];
    UTF8Read (&p);
    return p - St;
  }

int UTF8Prev (char *St, int Index)
  {
    int a, b;
    //
    a = 0;
    while (true)
      {
        b = UTF8Next (St, a);
        if (b == a)
          break;
        if (b >= Index)
          break;
        a = b;
      }
    return a;
  }


///////////////////////////////////////////////////////////////////////////////
//
// Dynamic C Strings

// Assign a dynamic string to another dynamic string

void StrAssign (char **Dest, char *Source)   // Pass the "batton"
  {
    if (*Dest)
      free (*Dest);
    *Dest = Source;
  }

// Dynamic String Assign from Static String

void StrAssignCopy (char **Dest, const char *Source)   // Copy the "batton"
  {
    if (*Dest != Source)
      {
        if (*Dest)
          free (*Dest);
        *Dest = NULL;
        if (Source)
          {
            *Dest = (char *) malloc (strlen (Source) + 1);
            strcpy (*Dest, Source);
          }
      }
  }


///////////////////////////////////////////////////////////////////////////////
//
// Static C Strings

bool StrError;

int StrLen (const char *St)
  {
    if (St)
      return strlen (St);
    return 0;
  }

char *StrPos (char *St, const char *Target)
  {
    int b;
    //
    if (St == NULL)
      return NULL;
    b = 0;
    while (true)
      {
        if (Target [b] == 0)   // Found
          return St;
        if (*St == 0)   // Not found
          return NULL;
        if (UpCase (St [b]) == UpCase (Target [b]))
          b++;
        else
          {
            St++;
            b = 0;
          }
      }
  }

char *StrPos (char *St, const char Target)
  {
    if (St == NULL)
      return NULL;
    while (true)
      {
        if (*St == 0)
          return NULL;
        if (*St == Target)
          return St;
        St++;
      }
  }

int StrCmp (const char *s1, const char *s2)
  {
    // Are both undefined
    if ((s1 == NULL) && (s2 == NULL))
      return 0;
    // Is one undefined
    if (s1 == NULL)
      return -1;
    if (s2 == NULL)
      return 1;
    // Both defined so find a difference
    while (true)
      {
        if ((*s1 == 0) && (*s2 == 0))   // Match
          return 0;
        if (UpCase (*s1) < UpCase (*s2))
          return -1;
        if (UpCase (*s1) > UpCase (*s2))
          return +1;
        s1++;
        s2++;
      }
  }

bool StrSame (const char *S1, const char *S2)
  {
    return (StrCmp (S1, S2) == 0);
  }

// Fields

void StepSpace (char **Pos)
  {
    if (Pos)
      while (**Pos == ' ') // ((**Pos == ' ') || (**Pos == '\t') || (**Pos == '\n') || (**Pos == '\r'))
        (*Pos)++;
  }

char *StrGetItem (char **Pos, char Separator)   // Get a substring, delimited by Separator. USER MUST FREE
  {
    char *ItemStart;
    char *Res;
    //
    //StepSpace (Pos);
    if (*Pos == 0)
      return NULL;   // Error: No string
    ItemStart = *Pos;
    while (true)
      {
        if ((**Pos == Separator) || (**Pos == 0))
          break;
        (*Pos)++;
      }
    Res = NULL;
    if (*Pos != ItemStart)
      {
        Res = (char *) malloc (*Pos - ItemStart + 1);
        memcpy (Res, ItemStart, *Pos - ItemStart);
        Res [*Pos - ItemStart] = 0;
      }
    // Step past separator
    if (**Pos)
      (*Pos)++;
    return Res;
  }

// Conversion to numbers

int CharToValue (char Ch, int Base)
  {
    int Val;
    //
    Ch = UpCase (Ch);
    if (IsDigit (Ch))
      Val = Ch - '0';
    else if ((Ch >= 'A') && (Ch <= 'Z'))
      Val = Ch - 'A' + 10;
    else
      return -1;
    if (Val < Base)
      return Val;
    return -1;
  }

int StrGetNumBaseFixed (char **Pos, int Len, int Base)
  {
    int Res;
    int Val;
    //
    Res = 0;
    if (*Pos == NULL)   // No String
      StrError = true;
    else
      while (true)
        {
          if (Len-- == 0)
            break;
          if (**Pos == 0)
            break;
          Val = CharToValue (**Pos, Base);
          if (Val < 0)   // invalid digit
            StrError = true;
          else
            Res = (Res * Base) + Val;
          (*Pos)++;
        }
    return Res;
  }

int StrGetNumBase (char **Pos, int Base)
  {
    int Res, Val;
    bool OK, Neg;
    //
    OK = false;
    Neg = false;
    Res = 0;
    if (*Pos)   // String exists
      {
        StepSpace (Pos);
        if (**Pos == '-')
          {
            Neg = true;
            (*Pos)++;
            StepSpace (Pos);
          }
        while (true)
          {
            Val = CharToValue (**Pos, Base);
            if (Val < 0)   // invalid digit so quit
              break;
            Res = (Res * Base) + Val;
            (*Pos)++;
            OK = true;
          }
      }
    if (!OK)
      StrError = true;
    if (Neg)
      return -Res;
    return Res;
  }

int StrGetNumFixed (char **Pos, int Len)
  {
    return StrGetNumBaseFixed (Pos, Len, 10);
  }

int StrGetNum (char **Pos)   // Get a cardinal. Return -1 if none there
  {
    return StrGetNumBase (Pos, 10);
  }

int StrGetHex (char **Pos)   // Get a hex number. Return -1 if none there
  {
    return StrGetNumBase (Pos, 16);
  }

int StrGetHHBlock (char **Pos, byte *Data, int Max)   // Converts packed hex bytes to raw
  {
    int Size;
    int Val;
    //
    Size = 0;
    while (true)
      {
        if (**Pos <= ' ')
          break;
        Val = StrGetNumBaseFixed (Pos, 2, 16);
        if (Data && (Size < Max))
          Data [Size] = Val;
        Size++;
      }
    return Size;
  }

float StrGetReal (char **Pos)
  {
    int Int;
    int Frac;
    int FracDen;
    //
    Int = 0;
    Frac = 0;
    FracDen = 1;
    if (*Pos)
      {
        StepSpace (Pos);
        if (**Pos != '.')
          Int = StrGetNum (Pos);
        if (**Pos == '.')
          {
            (*Pos)++;
            while ((**Pos >= '0') && (**Pos <= '9'))
              {
                Frac = Frac * 10 + (**Pos - '0');
                FracDen *= 10;
                (*Pos)++;
              }
          }
        if (Int < 0)  // Allow for negative values
          Frac = -Frac;
      }
    return (float) Int + (float) Frac / (float) FracDen;
  }

int StrGetHexAscii (char **Pos, byte *Data, int MaxData)   // Returns data size. Sets StrError
  {
    bool Ascii;
    int Index;
    int x;
    //
    Index = 0;
    Ascii = false;
    StrError = false;
    while (!StrError)
      {
        if (**Pos == 0)
          {
            if (Ascii)
              StrError = true;   // forgot to close quotes
            else
              return Index;
          }
        else if (Ascii)
          {
            if (**Pos == '\"')   // Switch mode
              Ascii = false;
            else
              {
                if (Index >= MaxData)
                  StrError = true;   // out of space for data
                else
                  Data [Index++] = **Pos;
              }
            (*Pos)++;
          }
        else
          {
            StepSpace (Pos);
            if (**Pos)
              if (**Pos == '\"')   // Switch mode
                {
                  Ascii = true;
                  (*Pos)++;
                }
              else
                {
                  x = StrGetHex (Pos);
                  if (x >= 0x100)
                    StrError = true;
                  else if (Index >= MaxData)
                    StrError = true;
                  else
                    Data [Index++] = x;
                }
          }
      }
    return 0;
  }

// Conversion to String

void NumToStrBase (char **Dest, unsigned long int n, word Digits, unsigned int Base)
  {
    int i, j;
    char temp [32];
    int x;
    //
    if (*Dest)
      {
        // Convert to decimal characters (in reverse order)
        i = j = 0;
        do
          {
            x = (n % Base);
            if (x < 10)
              temp [i++] = x + '0';
            else
              temp [i++] = x - 10 + 'A';
            j++;
            n = n / Base;
            if (Digits & DigitsCommas)
              if (j % 3 == 0 && n)
                temp [i++] =',';

          }
        while (n);
        // Pad out requested spaces / zeros
        while (j < (Digits & 0xFF))
          {
            if (Digits & DigitsZeros)
              *(*Dest)++ = '0';
            else
              *(*Dest)++ = ' ';
            j++;
          }
        // Add formatted number to result (in correct order)
        while (i)
          *(*Dest)++ = temp [--i];
      }
  }

void NumToStr (char **Dest, unsigned long int n, word Digits)
  {
    NumToStrBase (Dest, n, Digits, 10);
  }

void NumToStr (char **Dest, unsigned long int n)
  {
    NumToStrBase (Dest, n, 0, 10);
  }

void IntToStr (char **Dest, long int n, int Digits)
  {
    if (n < 0)
      {
        StrCat (Dest, '-');
        NumToStr (Dest, (unsigned int) -n, Digits);
      }
    else
      NumToStr (Dest, n, Digits);
  }

void IntToStr (char **Dest, long int n)
  {
    IntToStr (Dest, n, 0);
  }

void NumToHex (char **Dest, unsigned long int n, word Digits)
  {
    NumToStrBase (Dest, n, Digits, 16);
  }

void NumToStrDecimals (char **Dest, unsigned long int n, int Decimals, word Digits)
  {
    int m;
    int i;
    //
    if (*Dest)
//      if (n == StrError)
//        StrCat (Dest, '#', Decimals + 2);
//      else
        {
          if (n < 0)
            {
              *(*Dest)++ = '-';
              n = -n;
            }
          m = 1;
          for (i = 0; i < Decimals; i++)
            m = m * 10;
          NumToStr (Dest, n / m, Digits);
          if (Decimals)
            {
              *(*Dest)++ = '.';
              NumToStr (Dest, n % m, Decimals | DigitsZeros);
            }
        }
  }

void FloatToStr (char **Dest, float Value, int Places, word Digits)
  {
    int i;
    //
    if (Value < 0)
      StrCat (Dest, '-');
    Value = Abs (Value);
    for (i = 0; i < Places; i++)
      Value *= 10.0;
    NumToStrDecimals (Dest, (unsigned long int) Value, Places, Digits);
 }

void StrAddDateTime (char **Dest)
  {
    tm DateTime;
    //
    GetDateTime (&DateTime);
    NumToStr (Dest, DateTime.tm_year + 1900, 4 | DigitsZeros);
    *(*Dest)++ = '/';
    NumToStr (Dest, DateTime.tm_mon, 2 | DigitsZeros);
    *(*Dest)++ = '/';
    NumToStr (Dest, DateTime.tm_mday, 2 | DigitsZeros);
    *(*Dest)++ = ' ';
    NumToStr (Dest, DateTime.tm_hour, 2 | DigitsZeros);
    NumToStr (Dest, DateTime.tm_min, 2 | DigitsZeros);
    *(*Dest)++ = ':';
    NumToStr (Dest, DateTime.tm_sec, 2 | DigitsZeros);
  }

const char *MonthName [] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Format date time as per Format
// eg "Some Text %d2/%m2/%y4  %h2:%m2:%s2"
void StrFormatDateTime (char **St, tm *DateTime, char *Format)
  {
    char c;
    int x;
    //
    while (*Format)
      {
        if (*Format == '%')
          {
            Format++;
            x = StrGetNum (&Format);
            if (*Format)
              {
                c = UpCase (*Format++);
                if (c == 'D')
                  NumToStr (St, DateTime->tm_mday, x);
                else if (c == 'M')
                  if ((x == 3) && (DateTime->tm_mon < SIZEARRAY (MonthName)))
                    StrCat (St, MonthName [DateTime->tm_mon]);
                  else
                    NumToStr (St, DateTime->tm_mon + 1, x);
                else if (c == 'Y')
                  if (x == 2)
                    NumToStr (St, DateTime->tm_year % 100, 2 | DigitsZeros);
                  else
                    NumToStr (St, DateTime->tm_year + 1900, x | DigitsZeros);
                else if (c == 'H')
                  NumToStr (St, DateTime->tm_hour, x | DigitsZeros);
                else if (c == 'N')
                  NumToStr (St, DateTime->tm_min, x | DigitsZeros);
                else if (c == 'S')
                  NumToStr (St, DateTime->tm_sec, x | DigitsZeros);
              }
          }
        else
          {
            **St = *Format++;
            (*St)++;
          }
      }
  }

void DataToHex (char **Dest, byte *Data, int DataLen)
  {
    int i;
    //
    i = 0;
    if (*Dest)
      if (DataLen)
        while (true)
          {
            NumToStrBase (Dest, Data [i], 2 | DigitsZeros, 16);
            i++;
            if (i >= DataLen)
              break;
            *(*Dest)++ = ' ';
            if ((i % 8) == 7)
              *(*Dest)++ = ' ';
          }
  }

void DataToHexAscii (char **Dest, byte *Data, int DataLen)
  {
    int i;
    bool Hex = true;
    bool HexOld;
    //
    i = 0;
    if (*Dest)
      {
        if (DataLen)
          while (true)
            {
              HexOld = Hex;
              Hex = ((Data [i] < ' ') || (Data [i] >= 0x80));
              if (!Hex && HexOld)
                StrCat (Dest, '\"');
              if (Hex & !HexOld)
                StrCat (Dest, "\" ");
              if (Hex)
                {
                  NumToStrBase (Dest, Data [i], 2 | DigitsZeros, 16);
                  StrCat (Dest, ' ');
                }
              else
                StrCat (Dest, Data [i]);
              i++;
              if (i >= DataLen)
                break;
            }
        if (!Hex)
          StrCat (Dest, "\" ");
      }
  }

// Concatenate string/char/fixed string

void StrCat (char **Dest, const char *St)   // Append String
  {
    if (*Dest)
      if (St)
        while (*St)
          *(*Dest)++ = *St++;
  }

void StrCat (char **Dest, const char Ch)   // Append Char
  {
    if (*Dest)
      *(*Dest)++ = Ch;
  }

void StrCat (char **Dest, const char Ch, int n)   // Append Char repeated
  {
    if (*Dest)
      while (n-- > 0)
        *(*Dest)++ = Ch;
  }

void StrCat (char **Dest, const char *St, int n)   // Append String length limited
  {
    if (*Dest)
      if (St)
        while (true)
          {
            if (n == 0)
              break;
            if (*St == 0)
              break;
            *(*Dest)++ = *St++;
            n--;
          }
  }

// Filenames

char *StrFindFileExtension (char *FileName)
  {
    char *Res;
    //
    Res = NULL;
    if (FileName)
      while (true)
        {
          if (*FileName == 0)
            break;
          if (*FileName == '.')
            Res = FileName;
          FileName++;
        }
    return Res;
  }


///////////////////////////////////////////////////////////////////////////////
//
// GENERAL
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Integer stuff

/*int Max (int v1, int v2)
  {
    if (v1 > v2)
      return v1;
    return v2;
  }

int Min (int v1, int v2)
  {
    if (v1 < v2)
      return v1;
    return v2;
  }

int Abs (int a)
  {
    if (a >= 0)
      return a;
    return -a;
  }*/

void Limit (int *Value, int MinValue, int MaxValue)
  {
    if (MaxValue >= MinValue)
      *Value = Min (Max (*Value, MinValue), MaxValue);
    else
      *Value = MinValue;
  }

int Sqr (int x)
  {
    return x * x;
  }

int Sqrt (int a)
  {
    int x;
    //
    x = Sqrt ((float) a);
    if (Abs (Sqr (x) - a) < Abs (Sqr (x + 1) - a))
      return x;
    return x + 1;
  }

/*void SwapInt (int *a, int *b)
  {
    int c;
    //
    c = *a;
    *a = *b;
    *b = c;
  }*/

///////////////////////////////////////////////////////////////////////////////
// FLOAT stuff

float FloatError = 0.000000001;
float Pi = 3.14159265358979323;

/*float Abs (float a)
  {
    if (a >= 0.0)
      return a;
    return -a;
  }*/

float Sqr (float x)
  {
    return x * x;
  }

float Sqrt (float a)
  {
    float x, x0;
    //
    if (a <= 0)
      return 0;
    x = a / 2;
    do
      {
        x0 = x;
        x = (x0 + a / x0) / 2.0;
      }
    while (Abs (x - x0) > FloatError);
    return x;
  }

float Sin (float deg)
  {
    float res;
    float rad;
    float num, den;
    int i;
    //
    while (deg > 360.0)
      deg = deg - 360.0;
    rad = deg * Pi / 180.0;
    num = 1.0;
    den = 1.0;
    res = 0.0;
    i = 1;
    while (num / den > FloatError)
      {
        num *= rad;  // calculate succesive powers of x
        den *= i;   // calculate succesive factorials
        if (i & 1)   // odd: 1, 3, 5 ...
          if (i & 2)  // every 2nd odd: 3, 7, 11 ...
            res -= num / den;
          else  // every other 2nd odd: 1, 5, 7 ...
            res += num / den;
        i++;
      }
    return res;
  }

float Cos (float deg)
  {
    return Sin (deg + 90.0);
  }

int Round (float x)
  {
    if (x > 0)
      return x + 0.5;
    return x - 0.5;
  }


///////////////////////////////////////////////////////////////////////////////
// TIME DATE stuff

int ClockMS (void)   // milli seconds since ?
  {
#ifdef _Windows
    return GetTickCount ();
#else
    struct timeval t;
    //
    gettimeofday (&t, NULL);
    return (t.tv_sec * 1000) + (t.tv_usec / 1000);
#endif
  }

bool GetDateTime (tm *DateTime)
  {
    time_t RawTime;
    struct tm *info;
    //
    if (time (&RawTime) < 0)
      return false;
    info = localtime (&RawTime);
    memcpy (DateTime, info, sizeof (tm));
    #ifdef _Windows
    DateTime->tm_year += 1900;
    #endif // _Windows
    return true;
  }

#ifdef _Windows

#include <winsock2.h>
#include <windows.h>

bool SetDateTime (tm *DateTime)
  {
    SYSTEMTIME SystemTime;
    //
    SystemTime.wYear = DateTime->tm_year;// + 1900;
    SystemTime.wMonth = DateTime->tm_mon + 1;
    SystemTime.wDay = DateTime->tm_mday;
    SystemTime.wDayOfWeek = DateTime->tm_wday;
    SystemTime.wHour = DateTime->tm_hour;
    SystemTime.wMinute = DateTime->tm_min;
    SystemTime.wSecond = DateTime->tm_sec;
    SystemTime.wMilliseconds = 0;
    return SetLocalTime (&SystemTime);
  }

#else

bool SetDateTime (tm *DateTime)
  {
    time_t RawTime;
    timeval tv;
    //
    RawTime = mktime (DateTime);
    tv.tv_sec = RawTime;
    tv.tv_usec = 0;
    return settimeofday (&tv, NULL) == 0;
  }

#endif // _Windows

/*
bool SetDateTimeRTC (tm *DateTime)
  {
    int fd;
    bool OK;
    //
    OK = false;
    fd = open ("/dev/rtc", O_RDONLY);
    if (fd >= 0)
      {
        if (ioctl (fd, RTC_SET_TIME, &DateTime) >= 0)
          OK = true;
        close (fd);
      }
    return OK;
  }
*/


///////////////////////////////////////////////////////////////////////////////
// TCP stuff

#ifdef _Windows

bool StrGetIP (char **St, byte Flags)
  {
    return false;
  }

bool GetIP (char **Result, char *Interface)   // Interface ignored in Windows
  {
    WORD wVersionRequested;
    WSADATA wsaData;
    char name [255];
    PHOSTENT hostinfo;
    struct in_addr *IA;
    unsigned long IP;
    int x;
    bool OK;
    //
    OK = false;
    wVersionRequested = MAKEWORD (2, 0);
    if (WSAStartup (wVersionRequested, &wsaData) == 0)
      {
        if (gethostname ( name, sizeof (name)) == 0)
          if (hostinfo = gethostbyname (name))
            {
              IA = (struct in_addr *) hostinfo->h_addr_list [0];
              IP = IA->S_un.S_addr;
              // Format Result
              x = 0;
              while (true)
                {
                  NumToStr (Result, IP & 0xFF);
                  IP >>= 8;
                  if (++x >= 4)
                    break;
                  StrCat (Result, ".");
                }
              OK = true;
            }
        WSACleanup( );
      }
    return OK;
  }

#else

#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>


bool StrGetMAC (char **Result, char *Name)
  {
    int fd;
    struct ifreq ifr;
    int i;
    //
    if (Name)
      {
        fd = socket (AF_INET, SOCK_DGRAM, 0);
        ifr.ifr_addr.sa_family = AF_INET;
        //strncpy (ifr.ifr_name, "eth0", IFNAMSIZ-1);
        strcpy (ifr.ifr_name, Name);
        if (fd)
          {
            if (ioctl (fd, SIOCGIFHWADDR, &ifr) == 0)
              {
                close (fd);
                i = 0;
                while (true)
                  {
                    NumToHex (Result, (byte) ifr.ifr_hwaddr.sa_data [i], 2 | DigitsZeros);
                    i++;
                    if (i >= 6)
                      break;
                    StrCat (Result, '.');
                  }
                return true;
              }
          }
      }
    return false;
  }

bool StrGetIP (char **Result, byte GetIPFlags)
  {
    struct ifaddrs *addrs, *addr;
    struct sockaddr_in *sa;
    struct in_addr sia;
    byte *b;
    int i;
    bool OK;
    bool First;
    //
    OK = false;
    First = true;
    if (getifaddrs (&addrs) == 0)
      {
        addr = addrs;
        // Step thru the list of interfaces
        while (addr)
          {
            if (addr->ifa_addr)
              if (addr->ifa_addr->sa_family == AF_INET)
                {
                  OK = true;
                  if (!First)
                    if (GetIPFlags & GetIPLF)
                      StrCat (Result, '\n');
                    else
                      StrCat (Result, ' ');
                  First = false;
                  if (GetIPFlags & GetIPName)
                    {
                      StrCat (Result, addr->ifa_name);
                      StrCat (Result, ':');
                    }
                  if (GetIPFlags & GetIPIP)
                    {
                      sa = (struct sockaddr_in *) addr->ifa_addr;
                      if (sa)
                        {
                          sia = sa->sin_addr;
                          b = (byte *) &sia;
                          i = 0;
                          while (true)
                            {
                              NumToStr (Result, *b++);
                              i++;
                              if (i == 4)
                                break;
                              StrCat (Result, '.');
                            }
                          if (GetIPFlags & GetIPMAC)
                            StrCat (Result, ' ');
                        }
                    }
                  if (GetIPFlags & GetIPMAC)
                    StrGetMAC (Result, addr->ifa_name);
                }
            addr = addr->ifa_next;
          }
        freeifaddrs (addrs);
      }
    return OK;
  }

/*
bool GetIP (char **Result, char *Interface)   // Interface = "wlan0", "eth0" etc
  {
    int fd;
    struct ifreq ifr;
    struct sockaddr_in *SAI;
    unsigned long IP;
    int x;
    //
    fd = socket (AF_INET, SOCK_DGRAM, 0);
    if (fd >= 0)
      {
        ifr.ifr_addr.sa_family = AF_INET;   // I want to get an IPv4 IP address
        strncpy (ifr.ifr_name, Interface, IFNAMSIZ - 1);   // Specify Interface
        ioctl (fd, SIOCGIFADDR, &ifr);
        close (fd);
        SAI = (struct sockaddr_in *) &ifr.ifr_addr;
        IP = SAI->sin_addr.s_addr;
        // Format Result
        x = 0;
        while (true)
          {
            NumToStr (Result, IP & 0xFF);
            IP >>= 8;
            if (++x >= 4)
              break;
            StrCat (Result, ".");
          }
        return true;
      }
    return false;
  }
*/

#endif // _Windows


///////////////////////////////////////////////////////////////////////////////
//
// RING BUFFER CLASS

TRingBuffer::TRingBuffer (int Size)
  {
    int s;
    //
    // Increase to next power of 2 if necessary
    s = 1;
    while (s < Size)
      s <<= 1;
    // Initialize
    BufferSize = 0;
    Buffer = new byte [s];
    if (Buffer)
      BufferSize = s;
    BufferA = 0;
    BufferB = 0;
    AddTime = ClockMS ();
  }

TRingBuffer::~TRingBuffer (void)
  {
    if (Buffer)
      delete[] Buffer;
  }

void TRingBuffer::Add (void *Data, int DataSize)
  {
    int i;
    //
    if (DataSize)
      for (i = 0; i < DataSize; i++)
        {
          Buffer [BufferB] = ((byte *) Data) [i];
          BufferB = (BufferB + 1) & (BufferSize - 1);
        }
    AddTime = ClockMS ();
  }

int TRingBuffer::ReadSize (void)
  {
    return (BufferB - BufferA) & (BufferSize - 1);
  }

int TRingBuffer::WriteSize (void)
  {
    return BufferSize - 1 - ReadSize ();
  }

byte TRingBuffer::Read (int Offset)
  {
    return Buffer [(BufferA + Offset) & (BufferSize - 1)];
  }

void TRingBuffer::Discard (int Size)   // Remove Size from Buffer. If Size < 0 discard all
  {
    if (Size < 0)
      BufferA = BufferB;
    else
      BufferA = (BufferA + Size) & (BufferSize - 1);
  }

int TRingBuffer::ReadBlock (void *Data, int Size, bool Remove)
  {
    int Res;
    //
    Res = 0;
    while (true)
      {
        if (Res >= Size)   // Enough Data Taken
          break;
        if (Res >= ReadSize ())   // No data left
          break;
        ((byte *) Data) [Res] = Read (Res);
        Res++;
      }
    if (Remove)
      Discard (Res);
    return Res;
  }

char* TRingBuffer::ReadLine (void)   // Return a new complete ASCII line from Buffer (caller must free)
  {
    int Pass;
    int a;
    byte c;
    int n;
    char *Res;
    //
    Res = NULL;
    for (Pass = 1; Pass <= 2; Pass++)
      {
        n = 0;
        a = 0;
        while (true)
          {
            if (a == ReadSize ())
              return NULL;
            c = Read (a++);
            if (c != '\n')
              if (c == '\r')   // finished
                {
                  if (Pass == 1)
                    Res = (char *) malloc (n + 1);   // Allocate new string
                  else   // Pass == 2
                    {
                      Discard (a);
                      Res [n] = 0;   // Terminate string
                      return Res;
                    }
                  break;
                }
              else
                {
                  if (Pass == 2)
                    Res [n] = c;
                  n++;
                }
          }
      }
    return Res;
  }

bool TRingBuffer::AddLine (char *St)
  {
    byte crlf [] = "\r\n";
    int n;
    //
    n = strlen (St);
    if (n + (int) sizeof (crlf) <= WriteSize ())
      {
        Add (St, n);
        Add (crlf, sizeof (crlf));
        return true;
      }
    return false;
  }


///////////////////////////////////////////////////////////////////////////////
//
// FILING / DIRECTORY / LOGGING

int FileOpen (char *Filename, _FileOpenMode FileOpenMode)
  {
    #ifdef _Windows
    if (FileOpenMode == foRead)
      return open (Filename, O_RDONLY | O_BINARY);
    if (FileOpenMode == foWrite)
      return open (Filename, O_RDWR | O_BINARY | O_CREAT | O_TRUNC);//, S_IREAD | S_IWRITE);   // Open file for writing
    // foAppend
    return open (Filename, O_WRONLY | O_BINARY | O_APPEND | O_CREAT, FILE_ATTRIBUTE_NORMAL);//, S_IREAD | S_IWRITE);   // Open file for appending
    #else
    if (FileOpenMode == foRead)
      return open (Filename, O_RDONLY);   // Open file for reading only
    if (FileOpenMode == foWrite)
      return open (Filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);   // Open file for writing
    return open (Filename, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);   // Open file for appending
    #endif // _Windows
  }

void FileClose (int File)
  {
    close (File);
  }

bool FileExists (char *Filename)
  {
    int File;
    //
    File = open (Filename, O_RDONLY);
    if (File < 0)
      return false;
    close (File);
    return true;
  }

longint FileSize (char *Filename)
  {
    struct stat st;
    //
    if (stat (Filename, &st))
      return -1;
    return st.st_size;
  }

longint FileSize (int File)
  {
    struct stat st;
    //
    if (File > 0)
      if (fstat (File, &st) == 0)
        return st.st_size;
    return -1;
  }

int FileAge (char *Filename)
  {
    struct stat st;
    time_t Time;
    //
    if (stat (Filename, &st))
      return -1;
    time (&Time);
    return Time - st.st_mtime;
  }

int FileRead (int File, byte *Data, int DataSize)
  {
    if (File > 0)
      return read (File, Data, DataSize);
    return 0;
  }

bool FileWrite (int File, byte *Data, int DataSize)
  {
    if (File > 0)
      if (write (File, Data, DataSize) == DataSize)
        return true;
    return false;
  }

void ForcePath (char *Path)
  {
    char *Path_;
    char *x;
    bool Done;
    //DIR *pDir;
    //struct stat PathStat;
    //
    Path_ = (char *) malloc (StrLen (Path) + 1);
    x = Path;
    Done = false;
    do
      {
        strcpy (Path_, Path);
        x = StrPos (x, PathChar);
        if (x == NULL)
          Done = true;
        else
          Path_ [x - Path] = 0;
        //mkdir (Path_);   // Windows
        #ifdef _Windows
        mkdir (Path_);   // Windows
        #else
        mkdir (Path_, 0777);   // Linux
        #endif // _Windows
        x++;
      }
    while (!Done);
    free (Path_);   // Memory leak fixed 03 Mar 2015 ****
  }

void GetCurrentPath (char **Path)   // Caller must free Result
  {
    char *p1;
    //
    if (chdir (".") == 0)   // Try to be in a valid place (####)
      {
        p1 = (char *) malloc (MaxPath + 1);
        getcwd (p1, MaxPath + 1);
        #ifndef _Windows
        if (p1 [0] == PathDelimiter)   // Valid path, not "(unreachable)..."
        #endif // _Windows
          StrAssignCopy (Path, p1);
        free (p1);
      }
  }

bool DirItemFromFilename (char *Filename, _DirItem *Item)
  {
    struct stat st;
    //
    if (stat (Filename, &st) == 0)
      {
        //memset (Item, 0, sizeof (_DirEntry));
        StrAssignCopy (&Item->Name, Filename);
        Item->Directory = S_ISDIR (st.st_mode) != 0;
        #ifdef _Windows
        //Item->SymLink = false;
        if (Item->Directory)
          Item->Size = st.st_size;
        else
          {
            DWORD szlo, szhi;   // 64 bit file sizes
            HANDLE f = CreateFile (Item->Name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (f == INVALID_HANDLE_VALUE)
              Item->Size = st.st_size;   // Use 32 bit size if we can't open the file
            else
              {
                szlo = GetFileSize (f, &szhi);
                CloseHandle (f);
                Item->Size = ((longint) szhi << (longint) 32) | szlo;
              }
          }
        #else
        Item->Size = st.st_size;
        #endif
        Item->DateTime = st.st_mtime;
        Item->Attrib = st.st_mode;   // Keep for permissions etc
        Item->Tagged = false;
        Item->Data = NULL;
        return true;
      }
    return false;
  }

void DirItemFree (_DirItem *Item)
  {
    free (Item->Name);
    Item->Name = NULL;
    free (Item->Data);
    Item->Data = NULL;
  }

void DirFree (_DirItem *Dir)
  {
    _DirItem *Dir_;
    //
    while (Dir)
      {
        Dir_ = Dir->Next;
        DirItemFree (Dir);
        free (Dir);
        Dir = Dir_;
      }
  }

typedef bool _ReadDirCallback (_DirItem *Item);

void DirRead (_DirItem **Dir, _ReadDirCallback CallBack)
  {
    _DirItem *New;
    struct dirent *de;
    DIR *dir;
    bool Include;
    //
    New = NULL;
    dir = opendir (".");
    if (dir != NULL)
      {
        while (true)
          {
            de = readdir (dir);
            if (de == NULL)
              break;
            if (!StrSame (de->d_name, ".") && !StrSame (de->d_name, ".."))
              {
                if (New == NULL)
                  {
                    New = (_DirItem *) malloc (sizeof (_DirItem));
                    memset (New, 0, sizeof (_DirItem));
                  }
                if (DirItemFromFilename (de->d_name, New))
                  {
                    Include = true;
                    if (CallBack)
                      Include = CallBack (New);
                    if (Include)   // Add to top of list
                      {
                        ListAddStart (New, Dir);
                        New = nullptr;
                      }
                    else   // not wanted
                      DirItemFree (New);   // free stuff in New
                  }
              }
          }
        closedir (dir);
      }
    if (New)
      free (New);
  }

extern void DebugAdd (const char *St, int Err);

void Log (char *Filename, char *Line)
  {
    char *Name, *x;
    tm DateTime;
    int File;
    //
    Name = (char *) malloc (Max (StrLen (Line) + 16, 2 * StrLen (Filename) + 32));
    x = Name;
    StrCat (&x, Filename);
    if (GetDateTime (&DateTime))
      {
        StrCat (&x, PathChar);
        NumToStr (&x, DateTime.tm_year, 4 | DigitsZeros);
        NumToStr (&x, DateTime.tm_mon + 1, 2 | DigitsZeros);
        *x = 0;
        ForcePath (Name);
        StrCat (&x, PathChar);
        StrCat (&x, Filename);
        NumToStr (&x, DateTime.tm_year, 4 | DigitsZeros);
        NumToStr (&x, DateTime.tm_mon + 1, 2 | DigitsZeros);
        NumToStr (&x, DateTime.tm_mday, 2 | DigitsZeros);
      }
    StrCat (&x, ".TXT");
    *x = 0;
    // was File = SDL_RWFromFile (Name, "a");   // Open the log file
    File = FileOpen (Name, foAppend);   // Open the log file
    if (File >= 0)
      {
        x = Name;
        NumToStr (&x, DateTime.tm_hour, 2 | DigitsZeros);
        NumToStr (&x, DateTime.tm_min, 2 | DigitsZeros);
        *x++ = ':';
        NumToStr (&x, DateTime.tm_sec, 2 | DigitsZeros);
        *x++ = '\t';
        StrCat (&x, Line);
        *x++ = '\r';
        *x++ = '\n';
        FileWrite (File, (byte *) Name, x - Name);
        FileClose (File);
      }
    else
      DebugAdd ("Log: Open: Error", errno);
    free (Name);
  }

void StrPathHome (char **St, char *Filename)
  {
#ifdef _Windows
    StrCat (St, getenv ("HOMEPATH"));
#else
    StrCat (St, getenv ("HOME"));
#endif
    StrCat (St, PathDelimiter);
    if (Filename)
      StrCat (St, Filename);
    **St = 0;
  }


/*
////////////////////////////////////////////////////////////////////////////
//
// LIST SORT / COUNT
//

////////////////////////////////////////////////////////////////////////////
//
// Sort ANY List of struct. First item is a pointer to the next. NULL to terminate.
// Uses Merge sort with no memory overhead (other than small stack use)

typedef int _CListSortCompare (void *Data1, void *Data2);

_CList *ListSort (_CList **List, _CListSortCompare *ListSortCompare)
  {
    _CList **left, **right;
    _CList **left_, **right_, **List_;
    _CList **Res, **Res_;
    unsigned int i;
    //
    // Base case. A list of zero or one elements is sorted, by definition.
    if (List == NULL)
      return NULL;
    if (*List == NULL)
      return (_CList *) List;
    // Divide the list into equal-sized sublists
    left = NULL;
    right = NULL;
    //
    i = 0;
    List_ = List;
    left_ = (_CList **) &left;
    right_ = (_CList **) &right;
    while (List_)
      {
        if (i & 1)   // odd index -> add left
          {
            *left_ = (_CList *) List_;
            left_ = List_;
          }
        else   // add right
          {
            *right_ = (_CList *) List_;
            right_ = List_;
          }
        i++;
        List_ = (_CList **) *List_;
      }
    *left_ = NULL;
    *right_ = NULL;
    // Recursively sort both sublists.
    left = (_CList **) ListSort (left, ListSortCompare);
    right = (_CList **) ListSort (right, ListSortCompare);
    // Then merge the now-sorted sublists.
    Res = NULL;
    Res_ = (_CList **) &Res;
    left_ = left;
    right_ = right;
    while (true)
      {
        if (left_ && right_)   // both left and right still have data
          if (ListSortCompare (left_, right_) < 0)   // add left
            {
              *Res_ = (_CList *) left_;
              left_ = (_CList **) *left_;
            }
          else   // add right
            {
              *Res_ = (_CList *) right_;
              right_ = (_CList **) *right_;
            }
        else if (left_)   // add left
          {
            *Res_ = (_CList *) left_;
            left_ = (_CList **) *left_;
          }
        else if (right_)   // add right
          {
            *Res_ = (_CList *) right_;
            right_ = (_CList **) *right_;
          }
        else
          break;
        Res_ = (_CList **) *Res_;
      }
    return (_CList *) Res;
  }

int ListCount (_CList *List)
  {
    int Res;
    //
    Res = 0;
    while (List)
      {
        Res++;
        List = List->Next;
      }
    return Res;
  }

// Go to IndexTarget in the List. Currently at *Index **Item
bool ListIndex (_CList *List, int IndexTarget, int *Index, _CList **Item)
  {
    if (*Index < 0 || IndexTarget < *Index || *Item == NULL)
      {
        *Index = 0;
        *Item = List;
      }
    while (true)
      {
        if (*Item == NULL)
          {
            *Index = -1;
            return false;
          }
        if (*Index == IndexTarget)
          return true;
        *Item = (*Item)->Next;
        (*Index)++;
      }
  }
*/
