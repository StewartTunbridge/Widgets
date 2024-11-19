///////////////////////////////////////////////////////////////////////////////
//
// LIBRARY
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _lib
#define _lib

#include <stdlib.h>
#include "time.h"

#define SIZEARRAY(X) (int)(sizeof(X)/sizeof(X[0]))

#define IntMin (1<<(sizeof(int)*8-1))   // Top bit set = largest negative number
#define IntMax (~IntMin)   // inverse of above => largest positive number

typedef char unsigned byte;
typedef short unsigned word;
typedef long int longint;

//extern void DebugAdd (char *Message);
//extern void DebugAdd (char *Message, int Num);

extern const unsigned int Bit [32];   // BitNumber -> BitMask
extern const char *BoolText [2];   // bool -> String
extern const char *MonthNames [12];


///////////////////////////////////////////////////////////////////////////////
//
// COLOUR

// COLOUR CONSTANTS
const int cBlack = 0x000000;
const int cWhite = 0xFFFFFF;
const int cRed = 0x0000FF;
const int cGreen = 0x00FF00;
const int cBlue = 0xFF0000;
const int cAqua = (cGreen | cBlue);
const int cOrange = 0x00A0FF;
const int cGray = 0xD0D0D0;
const int cYellow = 0x00FFFF;
const int cPink = 0x8080FF;
const int cBrown = 0x005190;
const int cMaroon = 0x000080;

// Colour Breakup
#define ColourR(Colour) (byte)(Colour & 0xFF)
#define ColourG(Colour) (byte)((Colour >> 8) & 0XFF)
#define ColourB(Colour) (byte)((Colour >> 16) & 0xFF)

int ColourAverage (int Colour1, int Colour2);
int RGBToColour (byte r, byte g, byte b);
int ColourAdjust (int Colour, int Pcnt);   // Adjust up/down: Pcnt 100=unchanged, 0=Black, 200=White
int ColourGraded (int Colour1, int Colour2, int GradNum, int GradDen);


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
extern char UpCase (char c);

extern int UTF8Read (char ** Ch);
extern int UTF8Next (char *St, int Index);
extern int UTF8Prev (char *St, int Index);

extern int StrLen (const char *St);

// Dynamic String Assign from Dynamic String
extern void StrAssign (char **Dest, char *Source);

// Dynamic String Assign from Static String
extern void StrAssignCopy (char **Dest, const char *Source);

// Static Strings
//extern void StrSwap (char *S1, char *S2);   // Swap contents

extern char *StrPos (char *St, const char *Target);    // Search for string
extern char *StrPos (char *St, const char Target);     // Search for char
//extern int StrPosLastCh (char *St, const char Target); // Search for last character
//extern int StrPos_ (char St [], int Start, const char Target);   // Search for char from Start. Return index or strlen

extern int StrCmp (const char *s1, const char *s2);   // Compare: 0 => s1 = s2  -ve => s1 < s2  +ve => s1 > s2
extern bool StrSame (const char *S1, const char *S2);
//extern bool StrSame (const char *S1, const char *S2, int Len);
//extern bool StrSameWild (const char *Target, const char *St);
//bool StrMatch (char *St, char *Target);

//extern void StrAppend (char *St, char Ch);

extern void StepSpace (char **Pos);

// Fields
extern char *StrGetItem (char **String, const char Separator);   // Caller must free result
//extern char *StrGetItem (char **Pos, char Separator);   // Get a symbol separated item. Return new string. User must free
//extern char *StrGetItem (char *Strings, int Item, char Separator);   // Caller must free result
//extern char *StrGetField (char **Pos);   // Get a tab separated item. Return new string. User must free
//extern int StrFieldsCount (char *St);   // Count the tab separated items in St
//extern int StrCountChar (char *St, char Target);

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
extern int StrGetHexAscii (char **Pos, byte *Data, int MaxData);   // Returns data size. -1 if error

// Conversion to String
#define DigitsZeros  0x100
#define DigitsCommas 0x200
extern void NumToStrBase (char **Dest, unsigned long int n, word Digits, unsigned int Base);
extern void NumToStr (char **Dest, unsigned long int n, word Digits);
extern void NumToStr (char **Dest, unsigned long int n);
extern void IntToStr (char **Dest, long int n);
extern void IntToStr (char **Dest, long int n, int Digits);
extern void NumToHex (char **Dest, unsigned long int n, word Digits = 0);
extern void NumToStrDecimals (char **Dest, unsigned long int n, int Decimals, word Digits = 0);
extern void FloatToStr (char **Dest, float Value, int Places, word Digits = 0);
extern void StrAddDateTime (char **St);
extern void StrFormatDateTime (char **St, tm *DateTime, char *Format);   // Format date time as per Format %{0-9}[ymdhns]

extern void DataToHex (char **Dest, byte *Data, int DataLen);
extern void DataToHexAscii (char **Dest, byte *Data, int DataLen);

// Get & Convert Fields
//extern int StrGetNumField (char **Pos, int Default);
//extern int StrGetMoneyField (char **Pos);
//extern int StrGetHexField (char **Pos);

// Concatenate string
extern void StrCat (char **Dest, const char *St);   // Append String
extern void StrCat (char **Dest, const char Ch);   // Append Char
extern void StrCat (char **Dest, const char Ch, int n);   // Append Char repeated
extern void StrCat (char **Dest, const char *St, int n);   // Append String length limited

// Filenames
extern char *StrFindFileExtension (char *FileName);


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
    Start = Start->Next;
    while (true)
      {
        if (Start == nullptr)
          return true;
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
extern void Limit (int *Value, int MinValue, int MaxValue);
//extern int Abs (int x);

// Dynamic Array
#define ArrayItemUndefined IntMin
extern int ArrayGet (int **Array, int Index, int Default);   // unSet values return Default
extern void ArraySet (int **Array, int Index, int Value);

extern int Sqr (int x);
extern int Sqrt (int a);

extern float FloatError;   // set float precision on trig / sqrt
extern float Pi;   // already set but feel free to change reality here

//extern float Abs (float a);
extern int Round (float x);
extern float Sqr (float x);
extern float Sqrt (float a);
extern float Sin (float deg);
extern float Cos (float deg);

//extern int SystemTick (void);
extern int  ClockMS ();   // ms Counter
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

extern int FileOpen (char *Filename, _FileOpenMode FileOpenMode);
extern void FileClose (int File);

extern bool FileExists (char *Filename);
extern int FileAge (char *Filename);
extern longint FileSize (char *Filename);
extern longint FileSize (int File);
extern int FileRead (int File, byte *Data, int DataSize);
extern bool FileWrite (int File, byte *Data, int DataSize);
extern void ForcePath (char *Path);
extern void GetCurrentPath (char **Path);   // Caller must free *Path

extern void StrPathHome (char **St, char *Filename);

extern void Log (char *Filename, char *Line);

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
    unsigned int Attrib;
    bool Tagged;
    void *Data;
  } _DirItem;

typedef bool _ReadDirCallback (_DirItem *Item);

extern void DirRead (_DirItem **Dir, _ReadDirCallback CallBack);
extern void DirFree (_DirItem *Dir);


#endif // _lib
