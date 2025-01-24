///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - EDIT BOX (from _Container)
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////


#include "Widgets.hpp"
#include "WidgetsDriver.hpp"
#include "WidgetsEdit.hpp"

#include <string.h>

#define EditMargin 8


/*
_Edit::_Edit (_Container *Parent, _Rect Rect, char *Text, int Max, _Action Action_) : _Container (Parent, Rect, Text)
  {
    LenMax = Max;
    Action = Action_;
    xIndex = xOffset = 0;
    Colour = cTextBgnd;
    if (!Form->KeyFocus)
      Form->KeyFocus = this;
    NewFocus = false;
  }

bool _Edit::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    int Len;
    char *TextNew;
    int i;
    //
    Res = false;
    TextNew = NULL;
    if (CheckFocus (Event, Offset))
      NewFocus = true;
    if (IsEventMine (Event, Offset))
      {
        if (Event->Type == etMouseDown)
          {
            Res = true;
            if (Event->Key == KeyMouseLeft)
              {
                xIndex = TextIndex (FontFind (), Text, Event->X - Offset.x - EditMargin + xOffset);
                NewFocus = false;
              }
            else if (Event->Key == KeyMouseWheelUp)
              xIndex = UTF8Next (Text, xIndex);
            else if (Event->Key == KeyMouseWheelDown)
              xIndex = UTF8Prev (Text, xIndex);
            Invalidate (true);
          }
        else if (Event->Type == etKeyDown)
          {
            //#define DebugShowKeys
            #ifdef DebugShowKeys
            char s [16], *ps;
            ps = s;
            StrCat (&ps, "Key: 0x");
            NumToHex (&ps, Event->Key, -1);
            *ps = 0;
            DebugAdd (s);
            #endif
            Res = true;
            Len = StrLen (Text);
            if (Event->Key == bs)
              {
                i = xIndex;
                xIndex = UTF8Prev (Text, i);
                if (xIndex != i)
                  {
                    TextNew = (char *) malloc (Len);
                    strncpy (TextNew, Text, xIndex);
                    strcpy (&TextNew [xIndex], &Text [i]);
                  }
              }
            else if (Event->Key == KeyDel)
              {
                i = UTF8Next (Text, xIndex);
                if (i != xIndex)
                  {
                    TextNew = (char *) malloc (Len);
                    strncpy (TextNew, Text, xIndex);
                    strcpy (&TextNew [xIndex], &Text [i]);
                  }
              }
            else if (Event->Key == KeyLeft)
              xIndex = UTF8Prev (Text, xIndex);
            else if (Event->Key == KeyRight)
              xIndex = UTF8Next (Text, xIndex);
            else if (Event->Key == KeyHome)
              xIndex = 0;
            else if (Event->Key == KeyEnd)
              xIndex = Len;
            else if (Event->Key >= ' ' && Event->Key < 0x80)
              {
                if (NewFocus)
                  {
                    Text [0] = 0;
                    Len = 0;
                    xIndex = 0;
                    xOffset = 0;
                  }
                if (Len < LenMax || LenMax == 0)
                  {
                    TextNew = (char *) malloc (Len + 2);
                    strncpy (TextNew, Text, xIndex);
                    TextNew [xIndex] = Event->Key;
                    strcpy (&TextNew [xIndex + 1], &Text [xIndex]);
                    xIndex++;
                  }
              }
            else
              Res = false;
            NewFocus = false;
            Invalidate (true);
          }
      }
    if (TextNew)
      {
        StrAssign (&Text, TextNew);
        if (Action)
          Action (this);
      }
    return Res;
  }

void _Edit::DrawCustom (void)
  {
    _Point Size;
    _Point xPos;
    int TextLen;
    int x, y;
    char ch;
    //
    Size = TextMeasure (Font, Text);
    if (Size.y == 0)
      Size.y = Rect.Height / 2;
    TextLen = StrLen (Text);
    if (xIndex > TextLen)
      xIndex = TextLen;
    ch = Text [xIndex];
    Text [xIndex] = 0;
    xPos = TextMeasure (Font, Text);
    Text [xIndex] = ch;
    if (xPos.x < xOffset)
      xOffset = xPos.x;
    if (xPos.x - xOffset > Rect.Width - 2 * EditMargin)
      xOffset = xPos.x - Rect.Width + 2 * EditMargin;
    x = EditMargin - xOffset;
    y = (Rect.Height - Size.y) / 2;
    if (Form->KeyFocus == this)
      TextCallBackCursor = &Text [xIndex];
    TextOutAligned (Font, {x, y, 99, 99}, Text, aLeft);
    TextCallBackCursor = nullptr;
    //DrawCursor (x + xPos.x, y, Size.y);
    DrawBezel (2);
  }
*/


//////////////////////////////////////////////////////////////////////////////
//
// Edit Text: Multi & Single Line - Common Class

int LineNext (char *Text, int Index)
  {
    while ((byte) Text [Index] >= ' ')   // step past printables
      Index++;
    if (Text [Index])   // was a complete line
      return Index + 1;   // Step past /n
    return 0;   // no next line
  }

_EditCommon::_EditCommon (_Container *Parent, _Rect Rect, char *Text, _Action Action_, bool Multi)
  : _Container (Parent, Rect, Text)
  {
    Action = Action_;
    Multiline = Multi;
    Colour = cTextBgnd;
    if (!Form->KeyFocus)
      Form->KeyFocus = this;
    Posn = {0, 0};
    IndexLine = 0;
    IndexSel [0] = IndexSel [1] = 0;
    TextLen = StrLen (Text);
    Changed = false;
    RedrawFrom = RedrawTo = -1;
  }

bool _EditCommon::Move (int Movement)
  {
    bool Res;
    int i, i_;
    //
    Res = false;
    i = IndexLine;
    while (Movement > 0)
      {
        i_ = LineNext (Text, i);
        if (i_ == 0)   // Out of Lines
          break;
        i = i_;
        Posn.y++;   // adjust position
        Movement--;
        Res = true;
      }
    while (Movement < 0)
      {
        if (i == 0)   // Movement complete OR At start of Text
          break;
        i--;   // Step onto prev line /n
        while (i > 0 && (byte) Text [i - 1] >= ' ')   // step past printables
          i--;
        Posn.y--;   // adjust position
        Movement++;
        Res = true;
      }
    IndexLine = i;
    return Res;
  }

void _EditCommon::Insert (int Pos, int Size, char *St)
  {
    char *TextNew;
    //
    TextNew = (char *) malloc (TextLen + Size + 1);
    if (Text)
      {
        strncpy (TextNew, Text, Pos);
        strncpy (&TextNew [Pos], St, Size);
        strcpy (&TextNew [Pos + Size], &Text [Pos]);
      }
    else
      {
        strncpy (TextNew, St, Size);
        TextNew [Size] = 0;
      }
    StrAssign (&Text, TextNew);
    TextLen += Size;
    Changed = true;
  }

void _EditCommon::Delete (int Pos, int Size)
  {
    if (Text [Pos])
      {
        while (true)
          {
            Text [Pos] = Text [Pos + Size];
            if (Text [Pos] == 0)
              break;
            Pos++;
          }
        TextLen += Size;
        Changed = true;
      }
  }

int _EditCommon::LineLength (void)
  {
    int i;
    //
    i = 0;
    if (Text)
      while ((byte) Text [IndexLine + i] >= ' ')
        i++;
    return i;
  }

void _EditCommon::FindChar (int x, int y)
  {
    _Font *Font_;
    int Line;
    //
    Font_ = FontFind ();
    Line = Max (0, y / Font_->YAdvance);
    Move (Line - Posn.y);
    Posn.x = Limit (TextIndex (Font_, &Text [IndexLine], x), 0, LineLength ());
  }

_Point _EditCommon::CursorLocation (_Font *Font_)
  {
    _Point Res;
    char c;
    //
    c = Text [IndexLine + Posn.x];
    Text [IndexLine + Posn.x] = 0;
    Res = TextMeasure (Font_, &Text [IndexLine]);
    Text [IndexLine + Posn.x] = c;
    Res.y = Posn.y * Font_->YAdvance;
    return Res;
  }

int _EditCommon::DrawTextLine (_Font *Font_, int Index, int y)
  {
    _Point Size;
    //
    if (RenderFlags & rRedrawPart)
      DrawRectangle ({0, y, Rect.Width, Font_->YAdvance}, -1, -1, ColourFind ());//cTextBgnd);
    Size = TextMeasure (Font_, &Text [Index]);
    if (Multiline)
      TextOutAligned (Font_, {0, y, 0, 0}, &Text [Index], aLeft, aLeft);
    else
      TextOutAligned (Font_, {EditMargin, 0, Rect.Width, Rect.Height}, &Text [Index], aLeft, aCenter);
    //DebugAdd ("Text Draw Line", Index);   // ####
    return Size.x;
  }

void _EditCommon::DrawCustom (void)
  {
    _Point Size;
    int Index;
    int Line;
    int i;
    _Font *Font_;
    byte Style;
    //
    if (!Text)
      return;
    Time1 = Time2 = Time3 = 0;
    int Tick = ClockMS ();//####
    /*if (RenderFlags & rRedrawPart)
      if (RedrawTo == -1)
        DebugAdd ("Text Redraw Part - no range"); //####
      else
        DebugAddP ("Text Redraw Part Lines - ", {RedrawFrom, RedrawTo}); //####
    else
      DebugAdd ("Text Redraw ALL"); //####
    DebugAddP ("Text: IndexSelected ", {IndexSel [0], IndexSel [1]});*/ //####
    Font_ = FontFind ();
    if (Font_ == NULL)
      return;
    Style = Font_->Style;
    if (Colour >= 0 && ColourGrad < 0)   // Solid background
      Font_->Style |= fsFillBG;
    if (Form->KeyFocus == this)
      TextCallBackCursor = &Text [IndexLine + Posn.x];
    TextCallBackSelection [0] = nullptr;
    if (IndexSel [0] != IndexSel [1])   // Selection exists?
      {
        TextCallBackSelection [0] = &Text [IndexSel [0]];   // Let TextOut know
        TextCallBackSelection [1] = &Text [IndexSel [1]];
      }
    Size = {0, 0};
    Index = 0;
    Line = 0;
    while (true)
      {
        if (~RenderFlags & rRedrawPart || (Line >= RedrawFrom && Line <= RedrawTo))
          if (Size.y + Font_->YAdvance >= Scroll.y && Size.y - Scroll.y - Font_->YAdvance < Rect.Height)
            {
              i = DrawTextLine (Font_, Index, Size.y);
              Size.x = Max (i, Size.x);
            }
        Size.y += Font_->YAdvance;
        Line++;
        Index = LineNext (Text, Index);
        if (Index == 0 || !Multiline)
          break;
      }
    TextCallBackCursor = nullptr;
    TextCallBackSelection [0] = nullptr;
    if (Multiline)
      if (Size.x != SizeVirtual.x || Size.y != SizeVirtual.y)
        {
          SizeVirtual = Size;
          Invalidate (true);
        }
    Font_->Style = Style;
    if (!Multiline)
      DrawBezel (2);
    RedrawFrom = RedrawTo = -1;
    DebugAdd ("Edit Draw time (mS) ", ClockMS () - Tick); //####
    DebugAdd ("EditDraw RenderBitmap time (uS) ", Time1); //####
    DebugAdd ("EditDraw RenderBitmap make trans masks time (uS) ", Time2); //####
    DebugAdd ("EditDraw RenderBitmap put up img time (uS) ", Time3); //####
  }

bool _EditCommon::ProcessKeyDown (_Event *Event, _Font *Font_)
  {
    bool Res;
    byte Key;
    _Point Cursor;
    int i, j;
    //
    //#define DebugShowKeys
    #ifdef DebugShowKeys
    char s [16], *ps;
    ps = s;
    StrCat (&ps, "Key: 0x");
    NumToHex (&ps, Event->Key, -1);
    *ps = 0;
    DebugAdd (s);
    #endif
    Key = Event->Key & KeyMax;
    Res = true;
    if (Key >= ' ' && Key < 0x80)   // Insert visible character
      {
        Clipboard (coDelete);
        Insert (IndexLine + Posn.x, 1, (char *) &Key);
        Posn.x++;
        IndexSel [0] = IndexSel [1] = IndexLine + Posn.x;
      }
    else if (Key == KeyEnter && Multiline)   // Insert line break
      {
        Clipboard (coDelete);
        Insert (IndexLine + Posn.x, 1, (char *) &Key);
        Move (+1);
        Posn.x = 0;
        IndexSel [0] = IndexSel [1] = IndexLine + Posn.x;
        RedrawAdd (IntMax);
      }
    // Clipboard Shortcuts
    else if (Key == Control ('C'))
      Clipboard (coCopy);
    else if (Key == Control ('X'))
      Clipboard (coCut);
    else if (Key == Control ('V'))
      Clipboard (coPaste);
    else if (Key == Control ('A'))
      Clipboard (coSelectAll);
    else
      {
        i = IndexLine + Posn.x;
        // Cursor movement
        if (Key == KeyLeft)
          if (Posn.x)
            Posn.x = UTF8Prev (&Text [IndexLine], Posn.x);
          else
            {
              if (Move (-1))
                Posn.x = LineLength ();
            }
        else if (Key == KeyRight && Text [IndexLine + Posn.x])
          if ((byte) Text [i] >= ' ')
            Posn.x = UTF8Next (&Text [IndexLine], Posn.x);
          else
            {
              Move (+1);
              Posn.x = 0;
            }
        else if (Key == KeyUp && IndexLine)
          Move (-1);
        else if (Key == KeyDown && Text [IndexLine])
          Move (+1);
        else if (Key == KeyHome)
          {
            Posn.x = 0;
            if (KeyPrev == KeyHome)
              {
                Posn.y = 0;
                IndexLine = 0;
              }
          }
        else if (Key == KeyEnd)
          {
            if (KeyPrev == KeyEnd)// && Multiline)
              while (true)
                {
                  i = IndexLine;
                  Move (+1);
                  if (IndexLine == i)
                    break;
                }
            Posn.x = LineLength ();
          }
        else if (Key == KeyPageUp)
          Move (-Rect.Height / Font_->YAdvance + 1);
        else if (Key == KeyPageDown)
          Move (Rect.Height / Font_->YAdvance - 1);
        // Back space
        else if (Key == bs)
          {
            if (IndexSel [0] != IndexSel [1])
              Clipboard (coDelete);
            else if (i)
              {
                if (Posn.x)
                  j = UTF8Prev (Text, i);
                else
                  j = i - 1;
                if (j != i)
                  {
                    if (Posn.x)
                      Posn.x = j - IndexLine;
                    else
                      {
                        Move (-1);
                        Posn.x = LineLength ();
                        RedrawAdd (IntMax);
                      }
                    Delete (j, i - j);
                  }
              }
          }
        // Delete character
        else if (Key == KeyDel)
          {
            if (IndexSel [0] != IndexSel [1])
              Clipboard (coDelete);
            else if (Text [i])
              {
                if ((byte) Text [i] >= ' ')
                  j = UTF8Next (Text, i);
                else
                  {
                    j = i + 1;
                    RedrawAdd (IntMax);
                  }
                if (j != i)
                  Delete (i, j - i);
              }
          }
        else   // nothing usefull
          Res = false;
        if (Res)
          {
            // Adjust Position and selection and Range check
            Posn.x = Min (Posn.x, LineLength ());
            IndexSel [0] = IndexLine + Posn.x;
            if (~Event->ShiftState & KeyShift)
              IndexSel [1] = IndexSel [0];
          }
      }
    if (Res)
      {
        // adjust Scroll to show cursor
        Cursor = CursorLocation (Font_);
        if (Multiline)
          if (Cursor.y < Scroll.y)
            Scroll.y = Cursor.y;
          else if (Cursor.y + Font_->YAdvance - Scroll.y > Rect.Height - ScrollBarWidth)
            Scroll.y = Cursor.y + Font_->YAdvance - Rect.Height + ScrollBarWidth;
        if (Cursor.x < Scroll.x)
          Scroll.x = Cursor.x;
        else if (Cursor.x - Scroll.x >= Rect.Width - ScrollBarWidth)
          Scroll.x = Cursor.x - Rect.Width + ScrollBarWidth;
      }
    KeyPrev = Key;   // keep this key for next time
    if (Res)
      DebugAddP ("Text Select ", {IndexSel [0], IndexSel [1]}); //#####
    return Res;
  }

void _EditCommon::RedrawAdd (int Line)
  {
    if (RedrawFrom < 0 || Line < RedrawFrom)
      RedrawFrom = Line;
    if (RedrawTo < 0 || Line > RedrawTo)
      RedrawTo = Line;
  }

bool _EditCommon::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res, ChangedOld;
    _Font *Font_;
    _Point PosnOld;
    int Sel [2];
    //
    Res = false;
    CheckFocus (Event, Offset);
    if (IsEventMine (Event, Offset))
      {
        ChangedOld = Changed;
        Changed = false;
        PosnOld = Posn;
        Sel [0] = IndexSel [0];
        Sel [1] = IndexSel [1];
        //RedrawFrom = RedrawTo = -1;
        Offset.x -= Scroll.x;
        Offset.y -= Scroll.y;
        if (!Multiline)
          {
            Offset.x += EditMargin;
            Offset.y += EditMargin;
          }
        Font_ = FontFind ();
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)
          {
            Res = true;
            FindChar (Event->X - Offset.x, Event->Y - Offset.y);
            IndexSel [0] = IndexLine + Posn.x;
            if (~Event->ShiftState & KeyShift || Event->Type == etMouseMove)
              IndexSel [1] = IndexSel [0];
            KeyPrev = 0;
          }
        else if (Event->Type == etMouseMove && Event->MouseKeys == Bit [KeyMouseLeft - 1])
          {
            Res = true;
            FindChar (Event->X - Offset.x, Event->Y - Offset.y);
            IndexSel [0] = IndexLine + Posn.x;
          }
        else if (Event->Type == etKeyDown)
          Res = ProcessKeyDown (Event, Font_);
      }
    if (Res && (Posn.x != PosnOld.x || Posn.y != PosnOld.y || Changed))
      {
        //if (~RenderFlags & rRedraw)   // not already doing a full redraw (ie scroll, resize ...)
          {
            RedrawAdd (PosnOld.y);
            Invalidate (true);
            if (Multiline)
              {
                RenderFlags |= rRedrawPart;
                RedrawAdd (Posn.y);
                if (Sel [0] != Sel [1] && IndexSel [1] != Sel [1])   // totally new selection
                  RenderFlags &= ~rRedrawPart;
              }
          }
        if (Changed)
          if (Action)
            Action (this);
        Changed |= ChangedOld;
      }
    return Res;
  }

void _EditCommon::Clipboard (_ClipboardOp ClipboardOp)
  {
    int a, b;
    char Ch;
    char *St;
    int Len;
    //
    a = b = 0;
    if (IndexSel [0] != IndexSel [1])
      {
        a = Min (IndexSel [0], IndexSel [1]);
        b = Max (IndexSel [0], IndexSel [1]);
        // Take a copy if ...
        if (ClipboardOp == coCut || ClipboardOp == coCopy)
          {
            Ch = Text [b];
            Text [b] = 0;
            ClipboardSet (&Text [a]);
            Text [b] = Ch;
          }
        // Delete selection if ...
        if (ClipboardOp == coCut || ClipboardOp == coDelete || ClipboardOp == coPaste)
          {
            //Move to start of selection
            while (IndexLine < a)
              if (!Move (+1))
                break;
            while (IndexLine > a)
              if (!Move (-1))
                break;
            Posn.x = a - IndexLine;
            // Delete selection and select nothing
            Delete (a, b - a);
            IndexSel [0] = IndexSel [1] = 0;
            Changed = true;
            RedrawAdd (IntMax);
            Invalidate (true);
          }
      }
    if (ClipboardOp == coPaste)
      {
        // Get Clipboard
        St = ClipboardGet ();
        Len = StrLen (St);
        // Insert is as new Selection
        Insert (IndexLine + Posn.x, Len, St);
        Changed = true;
        RedrawAdd (IntMax);
        Invalidate (true);
        // Clean up
        free (St);
      }
    if (ClipboardOp == coSelectAll)
      {
        IndexSel [0] = 0;
        IndexSel [1] = TextLen;
        Invalidate (true);
        RenderFlags &= ~rRedrawPart;
      }
  }


//////////////////////////////////////////////////////////////////////////////
//
// Edit Text: Multi & Single Line

_Edit::_Edit (_Container *Parent, _Rect Rect, char *Text, _Action Action_)
  : _EditCommon (Parent, Rect, Text, Action_, false)
  {
  }

_EditMultiline::_EditMultiline (_Container *Parent, _Rect Rect, char *Text, _Action Action_)
  : _EditCommon (Parent, Rect, Text, Action_, true)
  {
  }

