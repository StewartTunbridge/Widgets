///////////////////////////////////////////////////////////////////////////////
//
// EDIT BOX (from _Container)
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024,2025 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////


#ifndef _WidgetsEdit
#define _WidgetsEdit

typedef enum {coDelete, coCut, coCopy, coPaste, coSelectAll} _ClipboardOp;

typedef void (*_EditDrawLineCallBack) (char *Line, int LineLength);

typedef enum {opNull, opDel, opIns, opRep} _Op;

struct _Undo
  {
    _Undo *Next, *Prev;
    _Op Op;   // c is a shit language
    int Index;
    int Size;
    int Shift;
    char *Text;
  };

class _EditCommon: public _Container
  {
    protected:
      bool Multiline;
      _Point Posn;
      int IndexLine;   // Index of Start of Current Line
      int IndexBookmark;
      int KeyPrev;
      int RedrawFrom, RedrawTo;
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
      //
      bool Move (int Movement);   // Move up / down lines
      int LineLength (int Index);
      void Insert (int Pos, int Size, char *St);
      void Delete (int Pos, int Size);
      void FindChar (int x, int y);
      _Point CursorLocation (void);
      void RedrawAdd (int Line);
      int DrawTextLine (int Index, int y);
      void CharsRotate (char *Text, int Size, int Shift);
      void EditAutoComplete (bool Repeat);   // search for a previous word that starts the same as this one
      void EditIndentOutdentLines (byte Key);
      bool ProcessKeyDown (_Event *Event);
      void Changing (void);
      //
      _Undo *UndoList;
      void UndoAdd (_Op op, int Index, int Size);
    public:
      int TextLen;
      int Selected [2];   // Index of current Selection
      void MoveToIndex (int Index);
      void ScrollToCursor (void);   // adjust Scroll to show cursor
      _Action Action;
      _EditDrawLineCallBack EditDrawLineCallBack;
      bool Changed, ChangedNew;
      _EditCommon (_Container *Parent, _Rect Rect, char *Text, _Action Action_ = NULL, bool Multi = false);
     ~_EditCommon (void);
      void TextSet (char *Text);
      void SelectWord (void);
      void BookmarkSwap (void);
      void Clipboard (_ClipboardOp ClipboardOp);
      bool Undo (void);
      bool FileLoad (char *Filename, bool Hex = false);
      bool FileSave (char *Filename);
      bool Hex;   // Set by FileLoad. Used in FileSave
      _EOL EOL;   // "
  };

class _Edit: public _EditCommon
  {
    public:
      _Edit (_Container *Parent, _Rect Rect, char *Text, _Action Action_ = NULL);
  };

class _EditMultiline: public _EditCommon
  {
    public:
      _EditMultiline (_Container *Parent, _Rect Rect, char *Text, _Action Action_ = NULL);
  };

#endif


/*
class _Edit: public _Container
  {
    protected:
      int LenMax;
      _Action Action;
      int xIndex, xOffset;
      bool NewFocus;
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
      virtual void DrawCustom (void);
    public:
      _Edit (_Container *Parent, _Rect Rect, char *Text, int Max, _Action Action_ = NULL);
  };
*/
