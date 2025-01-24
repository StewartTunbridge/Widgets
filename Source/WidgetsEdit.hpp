///////////////////////////////////////////////////////////////////////////////
//
// EDIT BOX (from _Container)
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////


typedef enum {coDelete, coCut, coCopy, coPaste, coSelectAll} _ClipboardOp;

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

class _EditCommon: public _Container
  {
    protected:
      int TextLen;
      bool Multiline;
      _Point Posn;
      int IndexLine;   // Index of Start of Current Line
      int IndexSel [2];   // Index of current Selection
      int KeyPrev;
      int RedrawFrom, RedrawTo;
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
      //
      bool Move (int Movement);
      int LineLength (void);
      void Insert (int Pos, int Size, char *St);
      void Delete (int Pos, int Size);
      void FindChar (int x, int y);
      _Point CursorLocation (_Font *Font_);
      void RedrawAdd (int Line);
      int DrawTextLine (_Font *Font_, int Index, int y);
      bool ProcessKeyDown (_Event *Event, _Font *Font_);
    public:
      _Action Action;
      bool Changed;
      _EditCommon (_Container *Parent, _Rect Rect, char *Text, _Action Action_ = NULL, bool Multi = false);
      void Clipboard (_ClipboardOp ClipboardOp);
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
