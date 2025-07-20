///////////////////////////////////////////////////////////////////////////////
//
// LIBRARY
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2000, 2024 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _lib
#define _lib

#include <stdlib.h>
#include "time.h"

#define SIZEARRAY(X) (int)(sizeof(X)/sizeof(X[0]))

#define IntMin (1<<(sizeof(int)*8-1))   // Top bit set = largest negative number
#define IntMax (~IntMin)   // inverse of above => largest positive number

#define Cntrl(x) ((x) & 0x1F)

typedef char unsigned byte;
typedef short unsigned word;
typedef long long int longint;   // 64 bit integer both Linux & Windoze)

//extern void DebugAdd (char *Message);
//extern void DebugAdd (char *Message, int Num);

extern const unsigned int Bit [32];   // BitNumber -> BitMask
extern const char *BoolText [2];   // bool -> String
extern const char *MonthNames [12];


///////////////////////////////////////////////////////////////////////////////
//
// COLOUR

typedef union
  {
    int RGBA;
    byte Prime [sizeof (int)];
  } _Colour;

inline byte AlphaGet (int *Colour)
  {
    return ((_Colour *) Colour)->Prime [3];
  }

inline void AlphaSet (int *Colour, byte Alpha)
  {
    ((_Colour *) Colour)->Prime [3] = Alpha;
  }


// COLOUR CONSTANTS
extern int cBlack;
extern int cWhite;
extern int cRed;
extern int cGreen;
extern int cBlue;

extern int cRedLight;
extern int cGreenLight;
extern int cBlueLight;

extern int cRedDark;
extern int cGreenDark;
extern int cBlueDark;

extern int cAqua;
extern int cAquaLight;
extern int cAquaDark;

extern int cGray;
extern int cGrayDark;

extern int cPurple;
extern int cPurpleLight;

extern int cOrange;
extern int cYellow;
extern int cPink;
extern int cBrown;
extern int cMaroon;

// Colour Breakup
#define ColourR(Colour) (byte)(Colour & 0xFF)
#define ColourG(Colour) (byte)((Colour >> 8) & 0XFF)
#define ColourB(Colour) (byte)((Colour >> 16) & 0xFF)
#define ColourA(Colour) (byte)((Colour >> 24) & 0xFF)

extern int AlphaBlend (int Colour1, int Colour2, int Alpha);
extern int ColourAverage (int Colour1, int Colour2);
extern int RGBToColour (byte r, byte g, byte b);
extern void ColourToRGB (int Colour, byte *R, byte *G, byte *B);
extern int ColourAdjust (int Colour, int Pcnt);   // Adjust up/down: Pcnt 100=unchanged, 0=Black, 200=White
extern int ColourGraded (int Colour1, int Colour2, int GradNum, int GradDen);


///////////////////////////////////////////////////////////////////////////////
//
// C STRINGS

// Characters

#define cr 0x0D
#define lf 0x0A
#define bs 0x08
#define tab 0x09
#define esc 0x1B
#define bel 0x07

#define Control(Char) (Char & 0x1F)

extern bool IsDigit (char c);
extern bool IsAlpha (char c);
extern bool IsUpper (char c);
extern bool IsLower (char c);
extern char UpCase (char c);
extern char LoCase (char c);

extern int UTF8Read (char ** Ch);
extern int UTF8Next (char *St, int Index);
extern int UTF8Prev (char *St, int Index);
extern int UTF32ToStr (char **Dest, int Unicode);
//extern void UTF8FromInt (char **Dest, int Unicode);
extern int UTF16Read (word **UFT16);

extern int StrLen (const char *St);

// Dynamic String Assign from Dynamic String
extern void StrAssign (char **Dest, char *Source);

// Dynamic String Assign from Static String
extern void StrAssignCopy (char **Dest, const char *Source);

// Static Strings
//extern void StrSwap (char *S1, char *S2);   // Swap contents

extern char *StrPos (char *St, const char *Target, int TargetLen = 0);   // Search for string
extern char *StrPos (char *St, const char Target);      // Search for char
extern char *StrPosLast (char *St, const char Target);  // Search for last occurence of Target
extern char *StrPosLast (char *St, const char *Target);  // Search for last occurence of Target
extern char *StrPosPrev (char *St, char *Pos, const char *Target, int TargetLen = 0);   // Search for Previous occurence of Target

extern int StrCmp (const char *s1, const char *s2, int n = 0);   // Compare: 0 => s1 = s2  -ve => s1 < s2  +ve => s1 > s2. Set n for length limited

extern bool StrSame (const char *S1, const char *S2);

extern void StepSpace (char **Pos);

// Fields
extern char *StrGetItem (char **String, const char Separator);   // Caller must free result
extern void StrSetItem (char *Items, int n, char *Item);   // Items is static. Must be long enough

extern bool StrMatch (char **St, char *Target);   // Does St start with Target? If so, move St past it

// Conversion to numbers
extern bool StrError;
extern int StrGetNumBaseFixed (char **Pos, int Len, int Base);
extern int StrGetNumFixed (char **Pos, int Len);   // Get a cardinal of fixed length
extern int StrGetNum (char **Pos);   // Get a cardinal. Return -1 if none there
extern int StrGetHex (char **Pos);   // Get a hex number. Return -1 if none there
extern int StrGetHHBlock (char **Pos, byte *Data, int Max);   // Converts packed hex bytes to raw
extern float StrGetReal (char **Pos);   // Get a number form {n}[.{n}]
//extern int StrGetMoney (char **Pos);   // {n}[.{n}] converted to cents
//extern void StrGetStr (char **Pos, int Len, char *Result);   // Result is NULL terminated.
extern int StrGetHexAscii (char *Text, byte *Data, int MaxData);   // Returns data size. -1 if error

// Conversion to String
#define DigitsZeros  0x100
#define DigitsCommas 0x200
extern char IntToHexDigit (int n);
extern int NumToStrBase (char **Dest, unsigned long int n, word Digits, unsigned int Base);
extern int NumToStr (char **Dest, unsigned long int n, word Digits);
extern int NumToStr (char **Dest, unsigned long int n);
extern int IntToStr (char **Dest, long int n);
extern int IntToStr (char **Dest, long int n, int Digits);
extern int NumToHex (char **Dest, unsigned long int n, word Digits = 0);
extern int IntToStrDecimals (char **Dest, long int n, int Decimals, word Digits = 0);
extern int FloatToStr (char **Dest, float Value, int Places, word Digits = 0);
extern int StrAddDateTime (char **Dest, tm *DateTime);
extern int StrAddCurrentDateTime (char **Dest);
extern bool StrGetDateTime (char **St, tm *DateTime);


extern int StrFormatDateTime (char **St, tm *DateTime, char *Format);   // Format date time as per Format %{0-9}[ymdhns]
extern int StrTimeDelta (char **St, int Time);   // Convert seconds to days, hours etc
extern int IntToStrScaled (char **Dest, longint n);

extern int DataToHex (char **Dest, byte *Data, int DataLen); // return Size
extern int DataToHexAscii (char **Dest, byte *Data, int DataLen, int Layout = 0);

// Get & Convert Fields
//extern int StrGetNumField (char **Pos, int Default);
//extern int StrGetMoneyField (char **Pos);
//extern int StrGetHexField (char **Pos);

// Concatenate string
extern int StrCat (char **Dest, const char *St);   // Append String
extern int StrCat (char **Dest, const char Ch);   // Append Char
extern int StrCat (char **Dest, const char Ch, int n);   // Append Char repeated
extern int StrCat (char **Dest, const char *St, int n);   // Append String length limited
extern void StrInsert (char *Dest, char Ch);
extern void StrInsert (char *Dest, char *St);
extern void StrDelete (char *Ch);
extern void StrDelete (char *Ch, int n);
extern void StrAppend (char *Dest, char Ch);
extern void StrAppend (char *Dest, char *St);
extern bool StrChangeCase (char *St, int Len);   // Cycle thru letter case of sub-string: aaa Aaa AAA

extern char* StrMake (char *s1, char *s2);   // Makes new string from 2 strings. User must free
extern char* StrMake (char *s1, int n);   // Makes new string from part of 1 string. User must free

// Filenames
extern void StrRemoveTrailingSlash (char *Path);
extern char *StrFindFileExtension (char *FileName);
extern char *StrFindFilename (char *FileName);
extern void StrAddExtension (char *Filename, char *Extension);
extern void StrForceExtension (char *Filename, char *Extension);
extern bool StrExtractPath (char *Filepath, char ** Path);


///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATES

template <typename T> inline T Max (T a, T b)
  {
    return a < b ? b : a;
  }

template <typename T> inline T Min (T a, T b)
  {
    return a > b ? b : a;
  }

template <typename T> inline T ValueWithin (T x, T r1, T r2)
  {
    if (r1 <= r2)
      return x >= r1 && x <= r2;
    return x >= r2 && x <= r1;
  }
template <typename T> inline T IfZero (T Val, T Default)
  {
    if (Val == 0)
      return Default;
    return Val;
  }

template <typename T> inline T Limit (T val, T min, T max)
  {
    return Min (Max (val, min), max);
  }

template <typename T> inline int Sign (T a)
  {
    if (a < 0)
      return -1;
    if (a > 0)
      return +1;
    return 0;
  }

template <typename T> inline void Swap (T &a, T &b)
  {
    T c;
    //
    c = a;
    a = b;
    b = c;
  }

template <typename T> inline T Abs (T a)
  {
    return a >= 0 ? a : -a;
  }

// List Templates

template <class T>
void ListAdd (T *This, T **Start)
  {
    T *Sibling;
    //
    This->Next = NULL;
    This->Prev = NULL;
    if (*Start == NULL)   // first thing in the list
      *Start = This;
    else   // add to the end of the list
      {
        Sibling = *Start;
        while (Sibling->Next)
          Sibling = Sibling->Next;
        Sibling->Next = This;
        This->Prev = Sibling;
      }
  }

template <class T>
void ListAddStart (T *This, T **Start)
  {
    This->Next = NULL;
    This->Prev = NULL;
    if (*Start == NULL)   // first thing in the list
      *Start = This;
    else   // add to the start of the list
      {
        (*Start)->Prev = This;   // was NULL
        This->Next = *Start;
        *Start = This;
      }
  }

template <class T>
void ListRemove (T *This, T **Start)
  {
    if (This->Prev)
      This->Prev->Next = This->Next;
    if (This->Next)
      This->Next->Prev = This->Prev;
    if (Start)
      if (*Start == This)
        *Start = This->Next;
  }

template <class T>
bool ListCheck (T *Start)
  {
    T *Start_;
    //
    if (Start == nullptr)
      return true;   // no list
    if (Start->Prev)
      return false;
    while (true)
      {
        Start_ = Start->Next;
        if (Start_ == nullptr)
          return true;
        if (Start_->Prev != Start)
          return false;
        Start = Start_;
      }
  }

template <class T>
bool ListGoto (T *Start, int TargetIndex, int *CurrentIndex, T **CurrentItem)
  {
    if (Start == nullptr)
      return false;
    if (TargetIndex < Abs (TargetIndex - *CurrentIndex) || *CurrentIndex < 0 || *CurrentItem == nullptr)
      {
        *CurrentIndex = 0;
        *CurrentItem = Start;
      }
    while (true)
      {
        if (*CurrentIndex == TargetIndex)
          return true;
        if (*CurrentIndex < TargetIndex)
          if ((*CurrentItem)->Next)
            {
              *CurrentItem = (*CurrentItem)->Next;
              (*CurrentIndex)++;
            }
          else
            return false;
        if (*CurrentIndex > TargetIndex)
          if ((*CurrentItem)->Prev)
            {
              *CurrentItem = (*CurrentItem)->Prev;
              (*CurrentIndex)--;
            }
          else
            return false;
      }
  }

template <class T>
int ListLength (T *Start)
  {
    int n;
    //
    n = 0;
    while (Start)
      {
        n++;
        Start = Start->Next;
      }
    return n;
  }

template <class T>
T* ListSort (T *List, int ListSortCompare (T *t1, T *t2))
  {
    T *left, *right;
    T *List_;
    //T **left_, **right_;
    T *Res, *Res_;
    T **Add;
    unsigned int i;
    //
    if (List == nullptr)   // zero elements
      return nullptr;
    if (List->Next == nullptr)   // one element
      return List;
    // Divide the list into "equal" sized sublists
    left = nullptr;
    right = nullptr;
    i = 0;
    while (List)
      {
        Add = (i & 1) ? &left : &right;
        List_ = List->Next;
        if (*Add == nullptr)   // first one for this list
          {
            *Add = List;
            (*Add)->Next = nullptr;   // this is the end
            (*Add)->Prev = nullptr;   // and the beginning
          }
        else   // add to beginning of this list
          {
            List->Next = (*Add);
            (*Add) = List;
          }
        i++;
        List = List_;
      }
    // Recursively sort both sublists
    left = ListSort (left, ListSortCompare);
    right = ListSort (right, ListSortCompare);
    // Then merge the now-sorted sublists.
    Res = nullptr;
    while (true)
      {
        Add = nullptr;
        if (left && right)   // both left and right still have data
          if (ListSortCompare (left, right) < 0)   // which list is lower
            Add = &left;
          else
            Add = &right;
        else if (left)   // only the left remain
          Add = &left;
        else if (right)   // only the right remain
          Add = &right;
        else   // all done
          break;
        if (Res == nullptr)   // add the head of selected list
          {
            Res = *Add;
            Res_ = Res;
          }
        else
          {
            Res_->Next = *Add;
            Res_->Next->Prev = Res_;
            Res_ = *Add;
          }
        (*Add) = (*Add)->Next;   // go to the next item in the selected list
      }
    return Res;
  }


///////////////////////////////////////////////////////////////////////////////
//
// GENERAL

//extern void SwapInt (int *a, int *b);
//extern void SwapBytes (void *a, void *b, int n);
//extern void SwapByte (byte *a, byte *b);


//extern int Max (int v1, int v2);
//extern int Min (int v1, int v2);
//extern void Limit (int *Value, int MinValue, int MaxValue);
//extern int Abs (int x);

// Dynamic Array
#define ArrayItemUndefined IntMin
extern int ArrayGetInt (int **Array, int Index, int Default);   // unSet values return Default
extern void ArraySetInt (int **Array, int Index, int Value);
extern bool ArrayGetBool (bool **Array, int Index, bool Default);
extern void ArraySetBool (bool **Array, int Index, bool Value);

extern int Sqr (int x);
extern int Sqrt (int a);

extern float FloatError;   // set float precision on trig / sqrt
extern float Pi;   // already set but feel free to change reality here

//extern float Abs (float a);
extern int Round (float x);
extern float Sqr (float x);
extern float Sqrt (float a);
extern float Sin (float Rad);
extern float Cos (float Rad);
extern float SinDeg (float Deg);
extern float CosDeg (float Deg);

//extern int SystemTick (void);
extern int ClockMS (void);   // ms counter
extern int ClockuS (void);   // micro second counter
extern bool GetDateTime (tm *DateTime);
extern bool SetDateTime (tm *DateTime);
extern bool SetDateTimeRTC (tm *DateTime);

#define GetIPName 0x01
#define GetIPIP 0x02
#define GetIPMAC 0x04
#define GetIPLF 0x80

extern bool StrGetIP (char **Result, byte GetIPFlags);


///////////////////////////////////////////////////////////////////////////////
//
// RING BUFFER CLASS

class TRingBuffer
  {
    public:
      int BufferSize;
      byte *Buffer;
      int BufferA, BufferB;
      int AddTime;
      //
      TRingBuffer (int Size);
      ~TRingBuffer (void);
      void Add (void *Data, int DataSize);
      int ReadSize (void);
      int WriteSize (void);
      byte Read (int Offset);   // Read from Buffer (offset, does not discard)
      void Discard (int Size);   // Remove Size from Buffer. If Size < 0 discard all
      int ReadBlock (void *Data, int DataSize, bool Remove);   // Read from Buffer
      char* ReadLine (void);   // Return a new complete ASCII line from Buffer (caller must free)
      bool AddLine (char *St);   // Add a string with cr+lf
  };


///////////////////////////////////////////////////////////////////////////////
//
// FILING / DIRECTORY / LOGGING

#define MaxPath 1024

#ifdef _Windows
#define PathDelimiter '\\'
//typedef unsigned int mode_t;
#else
#define PathDelimiter '/'
#endif

typedef enum {foRead, foWrite, foAppend} _FileOpenMode;
typedef enum {eolLF, eolCRLF, eolCR} _EOL;

extern int FileOpen (const char *Filename, _FileOpenMode FileOpenMode);
extern void FileClose (int File);

extern bool FileDelete (const char *Filename);
extern bool FileRename (const char *OldFilename, const char *NewFilename);
extern bool FileExists (const char *Filename);
extern int FileAge (char *Filename);
extern longint FileSize (char *Filename);
extern longint FileSize (int File);
extern int FileRead (int File, byte *Data, int DataSize);
extern bool FileWrite (int File, byte *Data, int DataSize);
extern bool FileWriteLine (int File, char *St);
extern bool ForcePath (char *Path, bool FileIncluded);
extern bool GetCurrentPath (char **Path);   // Caller must free *Path

extern char *FileTextReadAll (char *Filename, int SizeMax, bool *Hex, int *Size, _EOL *EOL);   // In: Filename, SizeMax. In/Out: Hex. Out: Size, EOL.

extern char *ResourcePath;
extern void  ResourcePathSet (char *argv0);   // Sewt the ResourcePath from argv0 and the current path
extern char *ResourcePathAdd (char *Filename);   // Returns a string from the Home directory and this Filename. Caller must free
extern int FileOpen_ (char *Filename, _FileOpenMode FileOpenMode);   // Opens File in ResourcePath if no Path included

extern void StrPathHome (char **St, char *Filename);
extern bool DiskSpace (char *Path, longint *Capacity, longint *Free);

extern bool CurrentDateTimeToStr (char **St, bool Date, bool Time);
extern bool Log (char *Line);

/*
// C LIST

typedef struct _CList
  {
    _CList *Next;
    // plus whatever
  } _CList;

typedef int _CListSortCompare (void *Data1, void *Data2);

// Sort List (Merge Sort)
extern _CList *ListSort (_CList **List, _CListSortCompare *ListSortCompare);
// Count Items in List
extern int ListCount (_CList *List);
// Go to IndexTarget in the List. Currently at *Index **Item
extern bool ListIndex (_CList *List, int IndexTarget, int *Index, _CList **Item);
*/

// DIRECTORY

typedef struct _DirItem  // Directory Item. This is also a _List item
  {
    _DirItem *Next, *Prev;
    //
    char *Name;
    longint Size;
    time_t DateTime;
    bool Directory;
    bool SymLink;
    unsigned int Attrib;
    char *SymLinkTarget;
    bool Tagged;
    void *Data;
  } _DirItem;

typedef bool _DirReadCallback (_DirItem *Item);

extern void DirRead (const char *Path, _DirItem **Dir, _DirReadCallback CallBack, bool Recurse = false);
extern void DirFree (_DirItem *Dir);
extern void DirItemFree (_DirItem *Item);

extern bool DirItemFromFilename (char *Filename, _DirItem *Item);
extern bool FileDateTimeWrite (char *Filename, time_t DateTime);

extern bool DirExists (char *Path);
extern bool DirMake (char *Name);


#endif // _lib
