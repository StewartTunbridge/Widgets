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

extern const char *EOLText [];   // = {"\n", "\r\n", "\r"};

class _EditCommon;

typedef enum {coDelete, coCut, coCopy, coPaste, coSelectAll} _ClipboardOp;

typedef void (*_EditDrawLineCallBack) (_EditCommon *EditCommon, char *Line, int LineLength);

typedef enum {opNull, opDel, opIns, opOvr} _Op;

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
      int IndexBookmark;
      int KeyPrev;
      int RedrawFrom, RedrawTo;
      bool MovingSelection;
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
      //
      bool IsWithinSelection (int Index);
      int LineNext (int Index);
      int LinePrev (int Index);
      bool Move (int Movement);   // Move up / down lines
      int LineLength (int Index);
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
      int TextLen;   // Length of Text
      _Point Posn;   // Cursor position rel to top of file
      int IndexLine;   // Index of Start of Current Line
      int Selected [2];   // Index of current Selection
      void Insert (int Pos, int Size, char *St);   // Insert St into Text
      void Delete (int Pos, int Size);   // Delete Size from Text
      void MoveToIndex (int Index);   // Move to specified Index
      void MoveToPosn (_Point Posn);   // Move to specified Cursor position
      void ScrollToCursor (void);   // adjust Scroll to show cursor
      _Action Action;   // Call when Text changes
      _EditDrawLineCallBack EditDrawLineCallBack;   // Callback prior to Drawing each line
      bool Changed, ChangedNew;   // Text Changed flag
      _EditCommon (_Container *Parent, _Rect Rect, char *Text = NULL, _Action Action_ = NULL, bool Multi = false);
     ~_EditCommon (void);
      void TextSet (void);
      void TextSet (char *Text);
      bool SelectWord (void);   // Select word at cursor: True if successful
      void SelectLine (void);   // Select line at cursor
      void BookmarkSwap (void);   // Swap cursor with Bookmark
      void Clipboard (_ClipboardOp ClipboardOp);   // All Clipboard operations
      void Undo (void);   // Perform an Undo if possible (all Text changes go thru Insert/Delete/TextSet
      void UndoClearList (void);   // Clear Undo list
      bool UndoAvailable (void);   // Anything to Undo?
      bool FileLoad (char *Filename, bool Hex = false);   // Loads Text from file. Force Hex format.
      bool FileSave (char *Filename);   // Saves Text to File
      bool Hex;   // Set by FileLoad. Used in FileSave
      _EOL EOL;   // "
      _UTF UTF;   // "
  };

// Single & Multi Line Widgets

class _Edit: public _EditCommon
  {
    public:
      _Edit (_Container *Parent, _Rect Rect, char *Text = NULL, _Action Action_ = NULL);
  };

class _EditMultiline: public _EditCommon
  {
    public:
      _EditMultiline (_Container *Parent, _Rect Rect, char *Text = NULL, _Action Action_ = NULL);
  };

// Edit Text: with Popups

class _EditPopup: public _EditCommon
  {
    public:
      _MenuPopup *pmEdit;
      _EditPopup (_Container *Parent, _Rect Rect, char *Text = NULL, _Action Action_ = NULL, bool Multi = false);
  };

// Edit Text: with Popups and Drop List

class _EditPopupDropList : public _Container
  {
    public:
      _Align Align;
      int Selected;
      _Action Action;
      _SelectionList *SelectionList;
      _EditPopup *EditPopup;
      void DropListAdd (char *Item, int MaxItems);
      _EditPopupDropList (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_ = nullptr);
    protected:
      void DrawCustom (void);
      bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

#endif
