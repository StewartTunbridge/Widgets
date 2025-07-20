///////////////////////////////////////////////////////////////////////////////
//
// LIBRARY
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2000, 2025 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////
//
// History
// -------
// Remove references to SDL (done not tested)
//
// Added UTF8 and UTF16 support
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
  #include <sys/utime.h>
#else
  #include <unistd.h>
  #include <linux/rtc.h>
  #include <sys/time.h>
  #include <utime.h>
  #include <errno.h>
  #include <sys/statvfs.h>
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

extern void DebugAddInt (const char *St, int n);
extern void DebugAddS (const char *S1, const char *s2);

/*
#ifdef _Windows
  char PathChar = '\\';
#else
  char PathChar = '/';
#endif
*/


///////////////////////////////////////////////////////////////////////////////
//
// COLOUR

// COLOUR CONSTANTS

int cBlack = 0x000000;
int cWhite = 0xFFFFFF;
int cRed = 0x0000FF;
int cGreen = 0x00FF00;
int cBlue = 0xFF0000;

int cRedLight   =  0xC0C0FF;
int cGreenLight =  0xD0FFD0;
int cBlueLight  =  0xFFD0D0;

int cRedDark   =  0x000080;
int cGreenDark =  0x006000;
int cBlueDark =  0xA00000;

int cAqua = (cGreen | cBlue);
int cAquaLight =  cGreenLight | cBlueLight;
int cAquaDark =  cGreenDark | cBlueDark;

int cGray = 0xD0D0D0;
int cGrayDark = 0x808080;

int cPurple = 0xFF00FF;
int cPurpleLight = 0xFFD0FF;

int cOrange = 0x00A0FF;
int cYellow = 0x00FFFF;
int cPink = 0x8080FF;
int cBrown = 0x005190;
int cMaroon = 0x000080;

int AlphaBlend (int Colour1, int Colour2, int Alpha)
  {
    _Colour c1, c2;
    //
    c1.RGBA = Colour1;
    c2.RGBA = Colour2;
    c1.Prime [0] = (int) c1.Prime [0] * Alpha / 0xFF + c2.Prime [0] * (0xFF - Alpha) / 0xFF;
    c1.Prime [1] = (int) c1.Prime [1] * Alpha / 0xFF + c2.Prime [1] * (0xFF - Alpha) / 0xFF;
    c1.Prime [2] = (int) c1.Prime [2] * Alpha / 0xFF + c2.Prime [2] * (0xFF - Alpha) / 0xFF;
    return c1.RGBA;
  }



int ColourAverage (int Colour1, int Colour2)
  {
    return AlphaBlend (Colour1, Colour2, 0x80);
  }

int RGBToColour (byte r, byte g, byte b)
  {
    /*
    int Colour;
    byte *RGB = ((byte *) &Colour);
    RGB [0] = r;
    RGB [1] = g;
    RGB [2] = b;
    return Colour;*/
    return ((int) b << 16) | ((int) g << 8) | ((int) r);
  }

void ColourToRGB (int Colour, byte *R, byte *G, byte *B)
  {
    /* byte *RGB = ((byte *) &Colour);
    *R = RGB [0];
    *G = RGB [1];
    *B = RGB [2]; */
    *R = Colour;
    *G = Colour >> 8;
    *B = Colour >> 16;
  }

// Adjust Colour up/down: Pcnt 100=unchanged, 0=Black, 200=White
int ColourAdjust (int Colour, int Pcnt)
  {
    if (Pcnt == 100)
      return Colour;
    if (Pcnt < 100)
      return AlphaBlend (Colour, cBlack, Pcnt * 0xFF / 100);
    return AlphaBlend (cWhite, Colour, (Pcnt - 100) * 0xFF / 100);
  }

int ColourGraded (int Colour1, int Colour2, int GradNum, int GradDen)
  {
    return AlphaBlend (Colour2, Colour1, GradNum * 0xFF / GradDen);
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

int ArrayGetInt (int **Array, int Index, int Default)
  {
    if (*Array == NULL || Index >= **Array)
      return Default;
    if ((*Array) [Index + 1] == ArrayItemUndefined)
      return Default;
    return (*Array) [Index + 1];
  }

void ArraySetInt (int **Array, int Index, int Value)
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

bool ArrayGetBool (bool **Array, int Index, bool Default)
  {
    int Size;
    //
    Size = 0;
    if (*Array)
      Size = *((int *) *Array);
    if (Index >= Size)
      return Default;
    if ((byte) ((*Array) [Index + 1]) == 0xFF)
      return Default;
    return (*Array) [Index + sizeof (int)];
  }

void ArraySetBool (bool **Array, int Index, bool Value)
  {
    bool *ArrayNew;
    int Size, SizeNew;
    //
    Size = 0;
    if (*Array)
      Size = *((int *) *Array);
    if (Index >= Size)
      {
        SizeNew = 1;
        while (SizeNew <= Index)
          SizeNew = SizeNew << 1;
        ArrayNew = (bool *) malloc (SizeNew * sizeof (bool) + sizeof (int));
        memset (ArrayNew, 0xFF, SizeNew * sizeof (bool) + sizeof (int));
        if (Size)   // Copy old array to new (larger) array
          {
            memmove (ArrayNew, *Array, Size * sizeof (bool) + sizeof (int));
            free (*Array);
          }
        *((int *) ArrayNew) = SizeNew;
        *Array = ArrayNew;
      }
    (*Array) [Index + sizeof (int)] = Value;
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

bool IsUpper (char c)
  {
    return c >= 'A' && c <= 'Z';
  }

bool IsLower (char c)
  {
    return c >= 'a' && c <= 'z';
  }

bool IsAlpha (char c)
  {
    return IsLower (c) || IsUpper (c);
  }

char UpCase (char c)
  {
    if (IsLower (c))
      c ^= ('A' ^ 'a');
    return c;
  }

char LoCase (char c)
  {
    if (IsUpper (c))
      c ^= ('A' ^ 'a');
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

// 000uvvvvwwwwxxxxyyyyzzzz
// ---kjihgfedcba9876543210
//                  -------  => 0yyyzzzz
//              -----------  => 110xxxyy 10yyzzzz
//         ----------------  => 1110wwww 10xxxxyy 10yyzzzz
//    ---------------------  => 11110uvv 10vvwwww 10xxxxyy 10yyzzzz

int UTF32ToStr (char **Dest, int Unicode)
  {
    int l;
    //
    l = 0;
    if (Unicode < 0x80)
      l += StrCat (Dest, (char) Unicode);
    else
      {
        if (Unicode < 0x800)
          l += StrCat (Dest, 0xC0 | ((Unicode >> 6) & 0x1F));
        else
          {
            if (Unicode < 0x10000)
              l += StrCat (Dest, 0xE0 | ((Unicode >> 12) & 0x0F));
            else
              {
                l += StrCat (Dest, 0xF0 | ((Unicode >> 18) & 0x07));
                l += StrCat (Dest, 0x80 | ((Unicode >> 12) & 0x3F));
              }
            l += StrCat (Dest, 0x80 | ((Unicode >> 6) & 0x3F));
          }
        l += StrCat (Dest, 0x80 | (Unicode & 0x3F));
      }
    return l;
  }

// Encoding to UTF16
// 00000000bbbbccccddddeeee unchanged
// 0000aaaabbbbccccddddeeee => 110110aaaabbbbcc 110111ccddddeeee

int UTF16Read (word **UTF16)   // Reads a UTF16 character from a UTF16 string.
  {
    int c1, c2;
    //
    c1 = **UTF16;
    if (c1 == 0)
      return 0;
    (*UTF16)++;
    if ((c1 & 0xFC00) != 0xD800)   // not the special case, no need the 2nd word
      return c1;
    c2 = **UTF16;
    if (c2 == 0)
      return -1;
    (*UTF16)++;
    if ((c2 & 0xFC00) != 0xDC00)   // not the special case 2nd word so error
      return -1;
    return ((c1 & 0x3FF) << 10) | (c2 & 0x3FF);
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

char *StrPos (char *St, const char *Target, int TargetLen)
  {
    int b;
    //
    if (St == NULL)
      return NULL;
    if (Target == NULL)
      return St;
    b = 0;
    while (true)
      {
        if ((TargetLen > 0 && b >= TargetLen) || Target [b] == 0)   // Found
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
        if (*St == Target)
          return St;
        if (*St == 0)
          return NULL;
        St++;
      }
  }

char *StrPosLast (char *St, const char Target)
  {
    char *Res;
    //
    Res = NULL;
    if (St)
      while (*St)
        {
          if (*St == Target)
            Res = St;
          St++;
        }
    return Res;
  }

char *StrPosLast (char *St, const char *Target)  // Search for last occurence of Target
  {
    char *p, *Res;
    //
    p = St - 1;
    Res = NULL;
    while (true)
      {
        p = StrPos (p + 1, Target);
        if (p == NULL)
          return Res;
        Res = p;
      }
  }

char *StrPosPrev (char *St, char *Pos, const char *Target, int TargetLen)
  {
    char *a, *b;
    char *Res;
    //
    Res = NULL;
    a = St;
    while (true)
      {
        b = StrPos (a, Target, TargetLen);
        if (b == NULL)
          break;
        if (b >= Pos)
          break;
        Res = b;
        a = b + 1;
      }
    return Res;
  }

int StrCmp (const char *s1, const char *s2, int n)
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
        if (n)
          if (--n == 0)
            return 0;
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
      while (**Pos == ' ' || **Pos == '\t' || **Pos == '\n' || **Pos == '\r')
        (*Pos)++;
  }

char *StrGetItem (char **Pos, char Separator)   // Get a substring, delimited by Separator. USER MUST FREE
  {
    char *ItemStart;
    char *Res;
    //
    //StepSpace (Pos);
    if (*Pos == 0 || **Pos == 0)
      return NULL;   // No string OR End of String
    ItemStart = *Pos;
    while (true)
      {
        if ((**Pos == Separator) || (**Pos == 0))
          break;
        (*Pos)++;
      }
    Res = (char *) malloc (*Pos - ItemStart + 1);
    memcpy (Res, ItemStart, *Pos - ItemStart);
    Res [*Pos - ItemStart] = 0;
    // Step past separator
    if (**Pos)
      (*Pos)++;
    return Res;
  }

char *StrNextTab (char *St)
  {
    char *a;
    //
    a = StrPos (St, '\t');
    if (a)   // no more tabs so add one
      return a + 1;
    a = StrPos (St, '\0');
    *a++ = '\t';
    *a = 0;
    return a;
  }

void StrSetItem (char *Items, int n, char *Item)   // Items is static. Must be long enough
  {
    char *p, *q;
    //
    p = Items;
    while (n)
      {
        p = StrNextTab (p);
        n--;
      }
    q = StrNextTab (p);
    StrDelete (p, q - p - 1);
    StrInsert (p, Item);
  }

bool StrMatch (char **St, char *Target)
  {
    char *s;
    //
    s = *St;
    while (*Target)
      if (*s++ != *Target++)
        return false;
    *St = s;
    return true;
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

int StrGetHexAscii (char *Text, byte *Data, int MaxData)   // Returns data size. Sets StrError
  {
    bool Ascii;
    int Index;
    int x;
    //
    Index = 0;
    Ascii = false;
    StrError = false;
    if (Text)
      while (true)
        {
          if ((*Text == 0) || StrError)
            break;
          if (Ascii)
            {
              if (*Text == '\"')   // Switch mode
                Ascii = false;
              else
                {
                  if (Data && Index < MaxData)
                    Data [Index] = *Text;
                  Index++;
                }
              Text++;
            }
          else
            {
              StepSpace (&Text);
              if (*Text)
                if (*Text == '\"')   // Switch mode
                  {
                    Ascii = true;
                    Text++;
                  }
                else
                  {
                    x = StrGetHex (&Text);
                    if (x >= 0x100)
                      StrError = true;
                    else if (Data && Index < MaxData)
                      Data [Index] = x;
                    Index++;
                  }
            }
        }
    if (Ascii)   // forgot to close quotes
      StrError = true;
    if (StrError)
      return -1;
    return Index;
  }

// Conversion to String

char IntToHexDigit (int n)
  {
    if (n <= 9)
      return '0' + n;
    return 'A' + n - 10;
  }

int NumToStrBase (char **Dest, unsigned long int n, word Digits, unsigned int Base)
  {
    int Res;
    int i, j;
    char temp [32];
    //
    Res = 0;
    // Convert to decimal characters (in reverse order)
    i = j = 0;
    do
      {
        temp [i++] = IntToHexDigit (n % Base);
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
          Res += StrCat (Dest, '0');
        else
          Res += StrCat (Dest, ' ');
        j++;
      }
    // Add formatted number to result (in correct order)
    while (i)
      Res += StrCat (Dest, temp [--i]);
    return Res;
  }

int NumToStr (char **Dest, unsigned long int n, word Digits)
  {
    return NumToStrBase (Dest, n, Digits, 10);
  }

int NumToStr (char **Dest, unsigned long int n)
  {
    return NumToStrBase (Dest, n, 0, 10);
  }

int IntToStr (char **Dest, long int n, int Digits)
  {
    if (n < 0)
      {
        StrCat (Dest, '-');
        return NumToStr (Dest, (unsigned int) -n, Digits) + 1;
      }
    return NumToStr (Dest, n, Digits);
  }

int IntToStr (char **Dest, long int n)
  {
    return IntToStr (Dest, n, 0);
  }

int NumToHex (char **Dest, unsigned long int n, word Digits)
  {
    return NumToStrBase (Dest, n, Digits, 16);
  }

int IntToStrDecimals (char **Dest, long int n, int Decimals, word Digits)
  {
    int Res;
    int m;
    int i;
    //
    Res = 0;
    if (n < 0)
      {
        if (Dest && *Dest)
          *(*Dest)++ = '-';
        Res++;
        n = -n;
      }
    m = 1;
    for (i = 0; i < Decimals; i++)
      m = m * 10;
    Res += NumToStr (Dest, n / m, Digits);
    if (Decimals)
      {
        Res += StrCat (Dest, '.');
        Res += NumToStr (Dest, n % m, Decimals | DigitsZeros);
      }
    return Res;
  }

int FloatToStr (char **Dest, float Value, int Places, word Digits)
  {
    int i;
    //
    for (i = 0; i < Places; i++)
      Value *= 10.0;
    return IntToStrDecimals (Dest, (long int) Value, Places, Digits);
 }

int StrAddDateTime (char **Dest, tm *DateTime)
  {
    int Res;
    //
    Res = 0;
    Res += NumToStr (Dest, DateTime->tm_year + 1900, 4 | DigitsZeros);
    Res += StrCat (Dest, '/');
    Res += NumToStr (Dest, DateTime->tm_mon, 2 | DigitsZeros);
    Res += StrCat (Dest, '/');
    Res += NumToStr (Dest, DateTime->tm_mday, 2 | DigitsZeros);
    Res += StrCat (Dest, ' ');
    Res += NumToStr (Dest, DateTime->tm_hour, 2 | DigitsZeros);
    Res += NumToStr (Dest, DateTime->tm_min, 2 | DigitsZeros);
    Res += StrCat (Dest, ':');
    Res += NumToStr (Dest, DateTime->tm_sec, 2 | DigitsZeros);
    return Res;
  }

int StrAddCurrentDateTime (char **Dest)
  {
    tm DateTime;
    //
    GetDateTime (&DateTime);
    return StrAddDateTime (Dest, &DateTime);
  }

bool StrGetDateTime (char **St, tm *DateTime)
  {
    int hhmm;
    //
    DateTime->tm_year = StrGetNum (St);
    if (DateTime->tm_year > 1900) //%%%%
      if (StrMatch (St, "/"))
        {
          DateTime->tm_year -= 1900;
          DateTime->tm_mon = StrGetNum (St);
          if (StrMatch (St, "/"))
            {
              DateTime->tm_mday = StrGetNum (St);
              hhmm = StrGetNum (St);
              DateTime->tm_hour = hhmm / 100;
              DateTime->tm_min = hhmm % 100;
              if (StrMatch (St, ":"))   // seconds is optional
                DateTime->tm_sec = StrGetNum (St);
              if (**St == 0)   // date string is ok
                return true;
            }
        }
    return false;
  }

const char *MonthName [] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Format date time as per Format
// eg "Some Text %d2/%m2/%y4  %h2:%n2:%s2"
int StrFormatDateTime (char **St, tm *DateTime, char *Format)
  {
    int Res;
    char c;
    int x;
    //
    Res = 0;
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
                  Res += NumToStr (St, DateTime->tm_mday, x | DigitsZeros);
                else if (c == 'M')
                  if ((x == 3) && (DateTime->tm_mon < SIZEARRAY (MonthName)))
                    Res += StrCat (St, MonthName [DateTime->tm_mon]);
                  else
                    Res += NumToStr (St, DateTime->tm_mon + 1, x | DigitsZeros);
                else if (c == 'Y')
                  if (x == 2)
                    Res += NumToStr (St, DateTime->tm_year % 100, 2 | DigitsZeros);
                  else
                    Res += NumToStr (St, DateTime->tm_year + 1900, x | DigitsZeros);
                else if (c == 'H')
                  Res += NumToStr (St, DateTime->tm_hour, x | DigitsZeros);
                else if (c == 'N')
                  Res += NumToStr (St, DateTime->tm_min, x | DigitsZeros);
                else if (c == 'S')
                  Res += NumToStr (St, DateTime->tm_sec, x | DigitsZeros);
              }
          }
        else
          {
            **St = *Format++;
            (*St)++;
          }
      }
    return Res;
  }

int StrTimeDelta (char **St, int Time)   // Convert seconds to days, hours etc
  {
    const int TimeDay = 60 * 60 * 24;
    const int TimeHour = 60 * 60;
    const int TimeMin = 60;
    int Res;
    //
    Res = 0;
    if (Time >= TimeDay)   // >= 100 hours, show days
      {
        Res += IntToStr (St, Time / TimeDay);
        Res += StrCat (St, " day");
        if (Time >= 2 * TimeDay)
          Res += StrCat (St, 's');
        Res += StrCat (St, ' ');
        Time = Time % TimeDay;
      }
    if (Time >= TimeHour)
      {
        Res += IntToStr (St, Time / TimeHour);
        Res += StrCat (St, "h ");
        Time = Time % TimeHour;
      }
    if (Time >= TimeMin)
      {
        Res += IntToStr (St, Time / TimeMin);
        Res += StrCat (St, "m ");
        Time = Time % TimeMin;
      }
    if (Time)
      {
        Res += IntToStr (St, Time);
        Res += StrCat (St, "s ");
      }
    return Res;
  }

char Multiplier [] = "kMGTPEZY";

int IntToStrScaled (char **Dest, longint n)
  {
    int x;
    int Res;
    //
    Res = 0;
    if (n < 0)
      {
        StrCat (Dest, '-');
        Res++;
        n = -n;
      }
    x = 0;
    while (n >= 10000)
      {
        n /= 1000;
        x++;
      }
    Res += IntToStr (Dest, n, 0 | DigitsZeros | DigitsCommas);
    if (x)
      {
        StrCat (Dest, Multiplier [x - 1]);
        Res++;
      }
    return Res;
  }

int DataToHex (char **Dest, byte *Data, int DataLen)
  {
    int Res;
    int i;
    //
    Res = 0;
    i = 0;
    while (true)
      {
        Res += NumToStrBase (Dest, Data [i], 2 | DigitsZeros, 16);
        i++;
        if (i >= DataLen)
          break;
        if ((i % 32) == 0)
          Res += StrCat (Dest, '\n');
        else
          {
            Res += StrCat (Dest, ' ');
            if ((i % 8) == 0)
              Res += StrCat (Dest, ' ');
          }
      }
    return Res;
  }

int DataToHexAscii (char **Dest, byte *Data, int DataLen, int Layout)
  {
    int i, Res, Res_;
    bool Hex = true;
    bool HexOld;
    //
    i = 0;
    Res_ = Res = 0;
    if (DataLen)
      while (true)
        {
          HexOld = Hex;
          Hex = ((Data [i] < ' ') || (Data [i] >= 0x80) || Data [i] == '"');
          if (!Hex && HexOld)
            Res += StrCat (Dest, '\"');
          if (Hex & !HexOld)
            Res += StrCat (Dest, "\" ");
          if (Hex)
            {
              Res += NumToStrBase (Dest, Data [i], 2 | DigitsZeros, 16);
              Res += StrCat (Dest, ' ');
            }
          else
            Res += StrCat (Dest, Data [i]);
          i++;
          if (i >= DataLen)
            break;
          if (Hex && Layout > 0)
            if (Res - Res_ >= Layout)   // New "Line needed"
              {
                Res += StrCat (Dest, '\n');
                Res_ = Res;
              }
        }
    if (!Hex)
      Res += StrCat (Dest, "\" ");
    return Res;
  }

// Concatenate string/char/fixed string

int StrCat (char **Dest, const char *St)   // Append String
  {
    int Res;
    //
    Res = 0;
    if (St)
      while (*St)
        {
          if (Dest && *Dest)
            *(*Dest)++ = *St;
          St++;
          Res++;
        }
    return Res;
  }

int StrCat (char **Dest, const char Ch)   // Append Char
  {
    if (Dest && *Dest)
      *(*Dest)++ = Ch;
    return 1;
  }

int StrCat (char **Dest, const char Ch, int n)   // Append Char repeated
  {
    int i;
    //
    if (*Dest)
      for (i = 0; i < n; i++)
        *(*Dest)++ = Ch;
    return n;
  }

int StrCat (char **Dest, const char *St, int n)   // Append String length limited
  {
    int Len;
    //
    Len = 0;
    if (St)
      while (true)
        {
          if (n == 0)
            break;
          if (*St == 0)
            break;
          StrCat (Dest, *St++);
          Len++;
          n--;
        }
    return Len;
  }

void StrInsert (char *Dest, char Ch)
  {
    char c;
    //
    if (Ch)
      while (true)
        {
          c = *Dest;
          *Dest++ = Ch;
          if (Ch == 0)
            break;
          Ch = c;
        }
  }

void StrInsert (char *Dest, char *St)
  {
    if (St)
      while (*St)
        StrInsert (Dest++, *St++);
  }

void StrDelete (char *Ch)
  {
    if (Ch)
      while (*Ch)
        {
          Ch [0] = Ch [1];
          Ch++;
        }
  }

void StrDelete (char *Ch, int n)
  {
    if (Ch)
      while (*Ch)
        {
          Ch [0] = Ch [n];
          Ch++;
        }
  }

void StrAppend (char *Dest, char Ch)
  {
    Dest = StrPos (Dest, (char) 0);
    *Dest++ = Ch;
    *Dest = 0;
  }

void StrAppend (char *Dest, char *St)
  {
    Dest = StrPos (Dest, (char) 0);
    StrCat (&Dest, St);
    *Dest = 0;
  }

bool StrChangeCase (char *St, int Len)
  {
    bool Res;
    int nlc, nuc;
    int Pass, i, FirstAlpha;
    char c;
    //
    Res = false;
    FirstAlpha = -1;
    Len = Max (Len, 1);
    nlc = nuc = 0;
    for (Pass = 0; Pass < 2; Pass++)
      for (i = 0; i < Len; i++)
        {
          c = St [i];
          if (IsAlpha (c))
            if (Pass == 0)
              {
                if (FirstAlpha < 0)
                  FirstAlpha = i;
                if (IsLower (c))
                  nlc++;
                else
                  nuc++;
              }
            else   // Pass == 1
              {
                if (nlc == 0)   // no lower case so downshift all
                  St [i] = LoCase (c);
                else if (nuc > 0)   // mixed case so upshift all
                  St [i] = UpCase (c);
                else   // nuc == 0   no upper case so upshift the 1st
                  if (i == FirstAlpha)
                    St [i] = UpCase (c);
                if (c != St [i])
                  Res = true;
              }
        }
    return Res;
  }

char* StrMake (char *s1, char *s2)   // Makes new string from 2 strings. User must free
  {
    char *Res, *r;
    //
    Res = (char *) malloc (StrLen (s1) + StrLen (s2) + 1);
    r = Res;
    StrCat (&r, s1);
    //StrCat (&r, ' ');
    StrCat (&r, s2);
    *r = 0;
    return Res;
  }

char* StrMake (char *s1, int n)   // Makes new string from part of 1 string. User must free
  {
    char *Res;
    //
    Res = (char *) malloc (n + 1);
    memcpy (Res, s1, n);
    Res [n] = 0;
    return Res;
  }

/*
char *StrReplace (char *St, char *Target, char *Replacement)   // Creates dynamic string with all occurences of Target replaced with Replacement
  {
    char *Res, r;
    char *s, *s_;
    int p;
    //
    Res = NULL;
    while (true)
      {
        s = St;
        l = 0;
        while (true)
          {
            s_ = StrPos (s_, Target);
            if (s_ == NULL)
              {
                if (Res
              break;
            if (p == 1)
              memcpy (r, s, s_ - s);
            l += s_ - s;
  }
*/

// Filenames

void StrRemoveTrailingSlash (char *Path)
  {
    int l;
    //
    l = StrLen (Path);
    if (l > 1 && Path [l - 1] == PathDelimiter)
      Path [l - 1] = 0;
  }

char *StrFindFileExtension (char *FileName)
  {
    if (*FileName == '.')   // Read only file tag
      FileName++;  // step past
    return StrPosLast (FileName, '.');
  }

char *StrFindFilename (char *FileName)
  {
    char *Res;
    //
    Res = StrPosLast (FileName, PathDelimiter);
    if (Res)   // Slash found
      return Res + 1;   // return file past it
    return FileName;   // no slash, return original Filename
  }

void StrAddExtension (char *Filename, char *Extension)
  {
    char *p;
    //
    p = StrFindFileExtension (Filename);
    if (!p)   // no extension
      {
        p = StrPos (Filename, (char) 0);
        StrCat (&p, Extension);
        *p = 0;
      }
  }

void StrForceExtension (char *Filename, char *Extension)
  {
    char *p;
    //
    p = StrFindFileExtension (Filename);
    if (p)   // no extension
      *p = 0;
    StrAddExtension (Filename, Extension);
  }

bool StrExtractPath (char *Filepath, char ** Path)
  {
    char *p = StrPosLast (Filepath, PathDelimiter);
    if (p == NULL)
      return false;
    *p = 0;
    *Path = NULL;
    StrAssignCopy (Path, Filepath);
    *p = PathDelimiter;
    return true;
  }

char *ResourcePath = NULL;

void ResourcePathSet (char *argv0)
  {
    char *f;
    //
    StrAssignCopy (&ResourcePath, argv0);   // Get the full programme path
    f = StrFindFilename (ResourcePath);
    if (f == ResourcePath)   // no path specified in command line
      GetCurrentPath (&ResourcePath);
    else
      *f = 0;   // Remove file from argv0
    DebugAddS ("Resource Path", ResourcePath);
  }

char* ResourcePathAdd (char *Filename)   // caller must free
  {
    char *Res, *f, *r;
    //
    Res = NULL;
    //if (Filename [0] != PathDelimiter)
    f = StrFindFilename (Filename);
    if ((f == Filename) && ResourcePath)   // no path is specified AND ResourcePath is
      {
        Res = (char *) malloc (StrLen (ResourcePath) + StrLen (f) + 8);
        r = Res;
        StrCat (&r, ResourcePath);
        if (*(r - 1) != PathDelimiter)   // no trailing slash?
          StrCat (&r, PathDelimiter);   // so add one
        StrCat (&r, f);
        *r = 0;
      }
    else
      StrAssignCopy (&Res, Filename);
    return Res;
  }

int FileOpen_ (char *Filename, _FileOpenMode fom)
  {
    int File;
    char *FilePath;
    //
    FilePath = ResourcePathAdd (Filename);
    File = FileOpen (FilePath, fom);
    free (FilePath);
    return File;
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
  }

void Limit (int *Value, int MinValue, int MaxValue)
  {
    if (MaxValue >= MinValue)
      *Value = Min (Max (*Value, MinValue), MaxValue);
    else
      *Value = MinValue;
  }*/

word SwapWordBytes (word Word)
  {
    return (Word >> 8) | ((Word & 0xFF) << 8);
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

float Sin (float Rad)
  {
    float res;
    float num, den;
    int i;
    //
    while (Rad >= 2 * Pi)
      Rad -= 2 * Pi;
    while (Rad < 0)
      Rad += 2 * Pi;
    num = 1.0;
    den = 1.0;
    res = 0.0;
    i = 1;
    while (num / den > FloatError)
      {
        num *= Rad;  // calculate succesive powers of x
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

float Cos (float Rad)
  {
    return Sin (Rad + Pi / 2);
  }

float SinDeg (float deg)
  {
    return Sin (deg * Pi / 180.0);
  }

float CosDeg (float deg)
  {
    return SinDeg (deg + 90.0);
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

int ClockuS (void)   // micro seconds since ?
  {
#ifdef _Windows
    return GetTickCount () * 1000;
#else
    struct timeval t;
    //
    gettimeofday (&t, NULL);
    return (t.tv_sec * 1000000) + (t.tv_usec);
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
    DateTime->tm_year += 1900;
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
// FILING / DIRECTORY

int FileOpen (const char *Filename, _FileOpenMode FileOpenMode)
  {
    char *Filename_;
    //
    // Make Backup?
    if (FileOpenMode == foWrite)
      {
        Filename_ = NULL;
        StrAssignCopy (&Filename_, Filename);
        *(StrPos (Filename_, '\0') - 1) = '~';   // Change Last character
        if (FileExists (Filename_))
          FileDelete (Filename_);
        FileRename (Filename, Filename_);
        StrAssignCopy (&Filename_, NULL);
      }
    #ifdef _Windows
    if (FileOpenMode == foRead)
      return open (Filename, O_RDONLY | O_BINARY);
    if (FileOpenMode == foWrite)
      return open (Filename, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);   // Open file for writing
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

bool FileDelete (const char *Filename)
  {
    return (remove (Filename) == 0);
  }

bool FileRename (const char *OldFilename, const char *NewFilename)
  {
    return (rename (OldFilename, NewFilename) == 0);
  }

bool FileExists (const char *Filename)
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

bool FileWriteLine (int File, char *St)
  {
    char eol;
    bool Res;
    //
    Res = false;
    eol = '\n';
    if (FileWrite (File, (byte *) St, StrLen (St)))
      if (FileWrite (File, (byte *) &eol, 1))
        Res = true;
    return Res;
  }

// Reads Complete File: Text (ASCII/UTF8/UTF16->UTF8) and Binary (->Mixed Hex+ASCII)
// Returns pointer to contents. Sets Hex, Size & EOL
// If Hex is true, Hex/ASCII conversion is forced.

char *FileTextReadAll (char *Filename, int SizeMax, bool *Hex, int *Size, _EOL *EOL)
  {
    bool Res;
    int File;
    word BOM;
    int nCR, nLF, nTAB, nNA;
    int c;
    char *Buffer, *Buffer_, *p;
    word *p16;
    int Size_;
    //
    Res = false;
    Buffer = Buffer_ = NULL;
    *EOL = eolLF;
    *Size = 0;
    File = FileOpen (Filename, foRead);
    if (File >= 0)
      {
        *Size = FileSize (File);
        if (*Size < SizeMax)
          {
            Buffer = (char *) malloc (*Size + 1);
            if (Buffer)
              if (FileRead (File, (byte *) Buffer, *Size) == *Size)
                {
                  Buffer [*Size] = 0;   // Terminate
                  nCR = nLF = nTAB = nNA = 0;
                  BOM = *((word *) Buffer);
                  if (!*Hex && (BOM == 0xFFFE || BOM == 0xFEFF))   // UTF16 file
                    {
                      Size_ = 0;
                      p16 = (word *) Buffer;
                      p16++;
                      p = NULL;
                      while (true)
                        {
                          if (p16 < (word *) &(Buffer [*Size]))
                            c = UTF16Read (&p16);   // Read UTF8 character  swap bytes if
                          else
                            c = 0;
                          if (BOM == 0xFFFE)
                            c = SwapWordBytes (c);
                          //DebugAddHex ("UFT16", c);//####
                          Size_ += UTF32ToStr (&p, c);
                          if (c == 0)
                            {
                              if (Buffer_)
                                break;
                              Buffer_ = (char *) malloc (Size_);
                              p = Buffer_;
                              Size_ = 0;
                              p16 = (word *) Buffer;
                              p16++;
                            }
                        }
                      free (Buffer);
                      Buffer = Buffer_;
                      *Size = Size_ - 1;   // don't include terminating '\0'
                    }
                  if (!*Hex && (*Size > 16))   // determine if text, UTF8 or Binary
                    {
                      p = Buffer;
                      while (true)
                        {
                          c = UTF8Read (&p);   // Read UTF8 character
                          //DebugAddHex ("UFT8", c);//####
                          if (c == 0)
                            break;
                          if (c < 0)
                            nNA++;
                          else if (c == cr) nCR++;
                          else if (c == lf) nLF++;
                          else if (c == tab) nTAB++;
                          else if (c < ' ') nNA++;
                        }
                      *Hex = (p != Buffer + *Size) || (nNA > *Size / 4);   // > 25% non ascii "crap"? Looks like a binary file
                    }
                  if (*Hex && (*Size * 3 < SizeMax))   // Hex file (and of acceptable size)
                    {
                      Size_ = DataToHexAscii ((char **) NULL, (byte *) Buffer, *Size, 80);
                      Buffer_ = (char *) malloc (Size_ + 1);
                      if (Buffer_)
                        {
                          p = Buffer_;
                          DataToHexAscii (&p, (byte *) Buffer, *Size, 80);
                          *p = 0;   // Terminate
                          free (Buffer);
                          Buffer = Buffer_;
                          *Size = Size_;
                          Res = true;
                        }
                    }
                  else if (!*Hex)   // normal text file
                    {
                      *EOL = eolLF;
                      if (nLF || nCR)   // Determine EOL type
                        if (nLF > nCR)
                          *EOL = eolLF;
                        else if (nCR > nLF)
                          *EOL = eolCR;
                        else
                          *EOL = eolCRLF;
                      Res = true;
                    }
                }
          }
        FileClose (File);
       }
    DebugAddInt ("TextLoad: EOL", *EOL);
    if (!Res)
      free (Buffer);
    return Buffer;
  }

/*
    int File, Size;
    byte *Contents;
    //
    Contents = NULL;
    File = FileOpen (Filename, foRead);
    if (File >= 0)
      {
        Size = FileSize (File);
        Contents = (char *) malloc (Size) + 1;
        if (FileRead (File, Contents, Size) != Size)
          {
            free (Contents)
            Contents = NULL;
          }
        FileClose (File);
      }
    return Contents;
  }*/

bool ForcePath (char *Path, bool FileIncluded)
  {
    char *x1, *x2;
    char c;
    //
    x1 = Path;
    if (*x1 == PathDelimiter)
      x1++;
    while (true)
      {
        x2 = StrPos (x1, PathDelimiter);
        if (x2 == NULL)   // have found the last slash
          {
            if (FileIncluded)
              return true;
            x2 = StrPos (x1, '\0');   // find end
          }
        c = *x2;
        *x2 = 0;   // tag end
        if (!DirExists (Path))
          if (!DirMake (Path))
            return false;
        if (c == 0)   // we are done
          return true;
        *x2++ = c;   // restore slash and step past
        x1 = x2;
      }
  }

bool GetCurrentPath (char **Path)   // Caller must free Result
  {
    char *p1;
    //
    *Path = NULL;
    if (chdir (".") == 0)   // Try to be in a valid place (####)
      {
        p1 = (char *) malloc (MaxPath + 1);
        getcwd (p1, MaxPath + 1);   // %%%% Windows can return "(unreachable)..."
        #ifndef _Windows
        if (p1 [0] == PathDelimiter)   // Valid full path
        #endif // _Windows
          StrAssignCopy (Path, p1);
        free (p1);
        return true;
      }
    return false;
  }

bool FileDateTimeWrite (char *Filename, time_t DateTime)
  {
    #ifdef _Windows
    struct _utimbuf utb;
    //
    utb.actime = DateTime;
    utb.modtime = DateTime;
    if (_utime (Filename, &utb) != 0)   // fail
      return false;
    #else
    struct utimbuf utb;
    //
    utb.actime = DateTime;
    utb.modtime = DateTime;
    if (utime (Filename, &utb) != 0)   // fail
      return false;
    #endif // _Windows
    return true;
  }

/*    #ifdef _Windows
    _utime (Filename

    HANDLE f;
    longint DateTime_;
    struct _FILETIME ft;
    //
    // Convert Linux time to magic windows time
    DateTime_ = DateTime;
    // Calculate Windows time. Offset 369 years and resolution 100nS
    //DateTime_ = (DateTime_ + 11644473600L) * 10000000L;
    DateTime_ = DateTime_ + 11644473600L;  // 134774 * 24 * 60 * 60
    //DateTime_ = DateTime_ + 3600L;   // fudge ######
    DateTime_ = DateTime_ * 10000000L;  // scale 1 second to 100nS
    f = CreateFile (Filename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ft.dwLowDateTime = (DWORD) DateTime_;
    ft.dwHighDateTime = ((longint) DateTime_) >> 32L;
    SetFileTime (f, &ft, &ft, &ft);
    CloseHandle (f);
    #else
    struct utimbuf utb;
    //
    utb.actime = DateTime;
    utb.modtime = DateTime;
    if (utime (Filename, &utb) != 0)   // fail
      return false;
    #endif // _Windows
    return true;
  }*/

bool DirItemFromFilename (char *Filename, _DirItem *Item)
  {
    struct stat st;
    int LnkSiz;
    //
    memset (Item, 0, sizeof (_DirItem));
    #ifdef _Windows
    if (stat (Filename, &st) == 0)
    #else
    if (lstat (Filename, &st) == 0)
    #endif
      {
        StrAssignCopy (&Item->Name, Filename);//StrFindFilename (Filename));
        Item->Directory = S_ISDIR (st.st_mode) != 0;
        Item->SymLink = false;
        #ifdef _Windows
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
                Item->Size = ((longint) szhi << 32L) | (longint) szlo;
              }
          }
        #else
        Item->Size = st.st_size;
        Item->SymLink = S_ISLNK (st.st_mode) != 0;
        if (Item->SymLink)   // This is a symbolic link
          {
            Item->SymLinkTarget = (char *) malloc (Item->Size + 1);
            LnkSiz = readlink (Item->Name, Item->SymLinkTarget, Item->Size);   // read link target
            if (LnkSiz == Item->Size)   // correct length of target string
              Item->SymLinkTarget [LnkSiz] = 0;   // terminate string
            else
              {
                free (Item->SymLinkTarget);
                Item->SymLinkTarget = NULL;
              }
          }
        #endif
        Item->DateTime = st.st_mtime;
        Item->Attrib = st.st_mode;   // Keep for permissions etc
        Item->Tagged = false;
        //Item->Data = NULL;
        return true;
      }
    return false;
  }

void DirItemFree (_DirItem *Item)
  {
    free (Item->Name);
    Item->Name = NULL;
    free (Item->SymLinkTarget);
    Item->SymLinkTarget = NULL;
    free (Item->Data);
    //Item->Data = NULL;
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

void DirRead (const char *Path, _DirItem **Dir, _DirReadCallback CallBack, bool Recurse)
  {
    char *FilePath, *f;
    _DirItem *New;
    struct dirent *de;
    DIR *dir;
    bool Include;
    //
    FilePath = (char *) malloc (MaxPath);
    New = NULL;
    if (Path)
      dir = opendir (Path);
    else
      dir = opendir (".");
    if (dir != NULL)
      {
        while (true)
          {
            de = readdir (dir);
            if (de == NULL)
              break;
            if (!StrSame (de->d_name, ".")) //&& !StrSame (de->d_name, ".."))
              {
                if (New == NULL)
                  {
                    New = (_DirItem *) malloc (sizeof (_DirItem));
                    memset (New, 0, sizeof (_DirItem));
                  }
                f = FilePath;   // Build full file path (if Path specified)
                if (Path)
                  {
                    StrCat (&f, Path);
                    StrCat (&f, PathDelimiter);
                  }
                StrCat (&f, de->d_name);
                *f = 0;
                if (DirItemFromFilename (FilePath, New))
                  {
                    if (New->Directory && Recurse && !StrSame (de->d_name, ".."))
                      DirRead (FilePath, Dir, CallBack, Recurse);
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
    free (FilePath);
  }

bool DirExists (char *Path)
  {
    DIR *d;
    //
    d = opendir (Path);
    if (d)
      {
        closedir (d);
        return true;
      }
    return false;
  }

bool DirMake (char *Name)
  {
    #ifdef _Windows
    return mkdir (Name) == 0;
    #else
    return mkdir (Name, S_IRWXU | S_IRWXG | S_IRWXO) == 0;
    #endif
  }

bool DiskSpace (char *Path, longint *Capacity, longint *Free)
  {
#ifdef _Windows
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD FreeClusters;
    DWORD Clusters;
    if (GetDiskFreeSpace (Path, &SectorsPerCluster, &BytesPerSector, &FreeClusters, &Clusters))
      {
        *Capacity = (longint) SectorsPerCluster * (longint) BytesPerSector * (longint) Clusters;
        *Free = (longint) SectorsPerCluster * (longint) BytesPerSector * (longint) FreeClusters;
        return true;
      }
#else
    struct statvfs DriveStats;
    //
    if (statvfs (Path, &DriveStats) == 0)
      {
        *Capacity = (longint) DriveStats.f_blocks * (longint) DriveStats.f_bsize;
        *Free = (longint) DriveStats.f_bfree * (longint) DriveStats.f_bsize;
        return true;
      }
#endif // _Windows
    return false;
  }


//////////////////////////////////////////////////////////////////////////////////////
//
// LOG

bool CurrentDateTimeToStr (char **St, bool Date, bool Time)
  {
    tm DateTime;
    //
    if (GetDateTime (&DateTime))
      {
        if (Date)
          {
            NumToStr (St, DateTime.tm_year + 1900, 4 | DigitsZeros);
            NumToStr (St, DateTime.tm_mon + 1, 2 | DigitsZeros);
            NumToStr (St, DateTime.tm_mday, 2 | DigitsZeros);
          }
        if (Time)
          {
            NumToStr (St, DateTime.tm_hour, 2 | DigitsZeros);
            NumToStr (St, DateTime.tm_min, 2 | DigitsZeros);
            NumToStr (St, DateTime.tm_sec, 2 | DigitsZeros);
          }
        return true;
      }
    return false;
  }

bool Log (char *Line)
  {
    char *Name, *n;
    char *NameFull;
    tm DateTime;
    int File;
    bool Res;
    //
    Res = false;
    Name = (char *) malloc (Max (StrLen (Line) + 64, StrLen (ResourcePath) + 64));
    if (GetDateTime (&DateTime))
      {
        n = Name;
        NumToStr (&n, DateTime.tm_year, 4 | DigitsZeros);
        NumToStr (&n, DateTime.tm_mon + 1, 2 | DigitsZeros);
        NumToStr (&n, DateTime.tm_mday, 2 | DigitsZeros);
        StrCat (&n, ".log");
        *n = 0;
        NameFull = ResourcePathAdd (Name);
        File = FileOpen (NameFull, foAppend);   // Open the log file
        if (File >= 0)
          {
            n = Name;
            NumToStr (&n, DateTime.tm_hour, 2 | DigitsZeros);
            NumToStr (&n, DateTime.tm_min, 2 | DigitsZeros);
            *n++ = ':';
            NumToStr (&n, DateTime.tm_sec, 2 | DigitsZeros);
            *n++ = '\t';
            StrCat (&n, Line);
            *n++ = '\n';
            FileWrite (File, (byte *) Name, n - Name);
            FileClose (File);
            Res = true;
          }
        free (NameFull);
      }
    free (Name);
    return Res;
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
