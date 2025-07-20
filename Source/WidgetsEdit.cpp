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

extern void RenderFillRect (_Rect Rect, int Colour);

#define EditMargin 4


//////////////////////////////////////////////////////////////////////////////
//
// Edit Text: Multi & Single Line - Common Class

int LineNext (char *Text, int Index)
  {
    char c;
    //
    if (Text && *Text)
      {
        while (true)   // step past printables
          {
            c = Text [Index];
            if (c == 0)
              return 0;
            if (c == lf)
              return Index + 1;   // Step past /n
            if (c == cr)
              {
                Index++;   // step past cr
                if (Text [Index] == lf)
                  Index++;   // step past lf
                return Index;
              }
            Index++;
          }
      }
    return 0;   // no text
  }

int LinePrev (char *Text, int Index)
  {
    char c;
    //
    if (Text && *Text)
      {
        if (Index == 0)
          return 0;
        Index--;
        if (Text [Index] == lf)
          if (Index)
            if (Text [Index - 1] == cr)
              Index --;
        while (true)   // step past printables
          {
            if (Index == 0)
              return 0;
            c = Text [Index - 1];
            if (c == lf || c == cr)
              return Index;
            Index--;
          }
      }
    return 0;   // no text
  }

int PrevWord (char *St, int Pos)
  {
    while (Pos && ((byte) (St [Pos - 1]) <= ' '))
      Pos--;
    while (Pos && ((byte) (St [Pos - 1]) > ' '))
      Pos--;
    return Pos;
  }

int NextWord (char *St, int Pos)
  {
    while ((byte) (St [Pos]) > ' ')
      Pos++;
    while ((byte) (St [Pos]) <= ' ' && St [Pos] != 0)
      Pos++;
    return Pos;
  }

bool IsIdentifier (char c)
  {
    if (IsAlpha (c) || c == '_' || IsDigit (c))
      return true;
    return false;
  }

_EditCommon::_EditCommon (_Container *Parent, _Rect Rect, char *Text_, _Action Action_, bool Multi)
  : _Container (Parent, Rect, Text_)
  {
    char *s;
    //
    if (Text == NULL)
      TextSet ("");
    if (!Multi)
      while (s = StrPos (Text, '\n'))   // Look for \n, exit when none left
          *s = ' ';   // replace \n with ' '
    Action = Action_;
    EditDrawLineCallBack = NULL;
    Multiline = Multi;
    Hex = false;
    EOL = eolLF;
    Colour = cTextBgnd;
    if (!Form->Focused)
      Form->Focused = this;
    Posn = {0, 0};
    IndexLine = 0;
    IndexBookmark = 0;
    Selected [0] = Selected [1] = 0;
    TextLen = StrLen (Text);
    Changed = false;
    KeyPrev = 0;
    RedrawFrom = RedrawTo = -1;
    UndoList = NULL;
  }

_EditCommon::~_EditCommon (void)
  {
    _Undo *u;
    //
    while (UndoList)
      {
        free (UndoList->Text);
        u = UndoList;
        ListRemove (UndoList, &UndoList);
        free (u);
      }
  }

void _EditCommon::Changing (void)
  {
    Changed = true;
    ChangedNew = true;
    if (Action)
      Action (this);
  }

void _EditCommon::TextSet (char *Text_)
  {
    char *p;
    //
    if (Text_ == NULL)
      _Container::TextSet ("");
    else
      _Container::TextSet (Text_);
    if (!Multiline)   // Single line Edit filters out multiple lines
      {
        p = StrPos (Text, '\n');
        if (p)
          *p = 0;   // kill taxt from the \n
      }
    TextLen = StrLen (Text);
    Posn = {0, 0};
    IndexLine = 0;
    Selected [0] = Selected [1] = 0;
    Changed = false;
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
        if (i)
          {
            i = LinePrev (Text, i);
            Posn.y--;   // adjust position
          }
        Movement++;
        Res = true;
      }
    IndexLine = i;
    return Res;
  }

void _EditCommon::MoveToIndex (int Index)
  {
    int i;
    //
    // Find to start of line
    i = Index;
    while (i && (Text [i - 1] >= ' '))
      i--;
    // Move to correct line
    while (IndexLine < i)
      if (!Move (+1))
        break;
    while (IndexLine > i)
      if (!Move (-1))
        break;
    // Place cursor.x in the right place on the line
    Posn.x = Index - IndexLine;
  }

void _EditCommon::Insert (int Pos, int Size, char *St)
  {
    char *TextNew;
    //
    if (Pos <= TextLen && Size > 0)
      {
        UndoAdd (opIns, Pos, Size);
        TextNew = (char *) malloc (TextLen + Size + 1);
        strncpy (TextNew, Text, Pos);
        if (St)   // St = NULL  =>  just making space
          strncpy (&TextNew [Pos], St, Size);
        strcpy (&TextNew [Pos + Size], &Text [Pos]);
        StrAssign (&Text, TextNew);
        TextLen += Size;
        if (Text [TextLen])//####
          DebugAddP ("Insert: Error", {Pos, Size});
        Changing ();
        if (Pos < IndexBookmark)
          IndexBookmark += Size;
      }
  }

void _EditCommon::Delete (int Pos, int Size)
  {
    if (Pos + Size <= TextLen)
      {
        UndoAdd (opDel, Pos, Size);
        StrDelete (&Text [Pos], Size);
        TextLen -= Size;
        if (Text [TextLen])//####
          DebugAddP ("Delete: Error", {Pos, Size});
        Changing ();
        if (Pos < IndexBookmark)
          IndexBookmark -= Size;
      }
  }

int _EditCommon::LineLength (int Index)
  {
    int i;
    char c;
    //
    i = 0;
    while (true)
      {
        c = Text [Index + i];
        if (c == cr || c == lf || c == 0)
          break;
        i++;
      }
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
    Posn.x = Limit (TextIndex (Font_, &Text [IndexLine], x), 0, LineLength (IndexLine));
  }

_Point _EditCommon::CursorLocation (void)
  {
    _Point Res;
    char c;
    //
    c = Text [IndexLine + Posn.x];
    Text [IndexLine + Posn.x] = 0;
    Res = TextMeasure (&Text [IndexLine]);
    Text [IndexLine + Posn.x] = c;
    Res.y = Posn.y * Res.y;
    return Res;
  }

void _EditCommon::ScrollToCursor (void)   // adjust Scroll to show cursor
  {
    _Point Cursor;
    _Font *Font_;
    int wMax;
    //
    wMax = Rect.Width - 2 * EditMargin;
    if (Multiline)
      wMax -= ScrollBarWidth;
    Cursor = CursorLocation ();
    Font_ = FontFind ();
    if (Multiline)
      if (Cursor.y < Scroll.y)
        Scroll.y = Cursor.y;
      else if (Cursor.y + Font_->YAdvance - Scroll.y > Rect.Height - ScrollBarWidth - 2 * EditMargin)
        Scroll.y = Cursor.y + Font_->YAdvance - Rect.Height + ScrollBarWidth + 2 * EditMargin;
    if (Cursor.x < Scroll.x)
      Scroll.x = Cursor.x;
    else if (Cursor.x - Scroll.x >= wMax)
      Scroll.x = Cursor.x - wMax; //Cursor.x - Rect.Width + ScrollBarWidth + 2 * EditMargin;
  }

int _EditCommon::DrawTextLine (int Index, int y)
  {
    _Point Size;
    int l;
    char c;
    //
    //DebugAddP ("DrawTextLine", {Index, y}); //####
    Size = TextMeasure (&Text [Index]);
    if (~RenderFlags & rRedraw)
      RenderFillRect ({Scroll.x, y, Scroll.x + Rect.Width, Size.y}, ColourFind ());
    if (Index <= TextLen)
      {
        l = LineLength (Index);
        c = Text [Index + l];
        Text [Index + l] = 0;
        if (EditDrawLineCallBack)
          EditDrawLineCallBack (&Text [Index], l);
        TextOutAligned ({EditMargin, y, 0, 0}, &Text [Index], aLeft, aLeft);
        Text [Index + l] = c;
      }
    return Size.x;
  }

void _EditCommon::DrawCustom (void)
  {
    _Point Size;
    int Index;
    int Line;
    int i;
    _Font *Font_;
    //
    if (Form->Focused == this)
      TextCallBackCursor = &Text [IndexLine + Posn.x];
    TextCallBackSelected [0] = nullptr;
    if (Selected [0] != Selected [1])   // Selection exists?
      {
        TextCallBackSelected [0] = &Text [Selected [0]];   // Let TextOut know
        TextCallBackSelected [1] = &Text [Selected [1]];
      }
    Font_ = FontFind ();
    if (Font_ == NULL)
      return;
    Size = {0, 0};
    Index = 0;
    Line = 0;
    while (true)
      {
        if (RenderFlags & rRedraw || (Line >= RedrawFrom && Line <= RedrawTo))
          if (Size.y + Font_->YAdvance >= Scroll.y && Size.y - Scroll.y - Font_->YAdvance < Rect.Height)
            {
              if (Multiline)
                i = DrawTextLine (Index, EditMargin + Size.y);
              else
                i = DrawTextLine (Index, (Rect.Height - Font_->YAdvance) / 2);
              Size.x = Max (i, Size.x);
            }
        Size.y += Font_->YAdvance;
        Line++;
        Index = LineNext (Text, Index);
        if (Index == 0 || !Multiline)
          break;
      }
    // space below text
    if (Multiline)
      RenderFillRect ({Scroll.x, Size.y + EditMargin, Scroll.x + Rect.Width, Rect.Height - EditMargin - (Size.y - Scroll.y)}, ColourFind ());
    Size.x += ScrollBarWidth;
    Size.y += ScrollBarWidth;
    TextCallBackCursor = nullptr;
    TextCallBackSelected [0] = nullptr;
    if (Multiline)
      if (Size.x != SizeVirtual.x || Size.y != SizeVirtual.y)
        {
          SizeVirtual = Size;
          Invalidate (true);
        }
    if (!Multiline)
      DrawBezel (2);
    RedrawFrom = RedrawTo = -1;
  }

/*void DebugShowChars (char *Text, int Size)
  {
    char St [200], *s;
    s = St;
    StrCat (&s, "  ->");
    for (int i = 0; i < Size; i++)
      if (Text [i] < ' ')
        StrCat (&s, '*');
      else
        StrCat (&s, Text [i]);
    *s = 0;
    DebugAdd (St);
  }*/

void _EditCommon::CharsRotate (char *Txt, int Size, int Shift)
  {
    UndoAdd (opRep, Txt - Text, Size);
    char *Txt_ = (char *) malloc (Size);
    memcpy (Txt_, Txt, Size);
    for (int i = 0; i < Size; i++)
      Txt [(i + Shift + Size) % Size] = Txt_ [i];
    free (Txt_);
    Changing ();
    //DebugShowChars (Text, Size);
  }

void _EditCommon::BookmarkSwap (void)
  {
    int bm;
    //
    bm = Selected [1];
    MoveToIndex (IndexBookmark);
    Selected [0] = Selected [1] = IndexBookmark;
    ScrollToCursor ();
    Invalidate (true);
    IndexBookmark = bm;
  }

void _EditCommon::SelectWord (void)
  {
    if (Selected [0] == Selected [1])   // no selection so select a word
      {
        while (IsIdentifier (Text [Selected [0]]))
          Selected [0]++;
        while (Selected [1] && (IsIdentifier (Text [Selected [1] - 1])))
          Selected [1]--;
      }
  }

void _EditCommon::EditAutoComplete (bool Repeat)   // search for a previous word that starts the same as this one
  {
    static int LenSearch;
    static int Pos;
    static char PastResults [256];
    int LenA, LenB, l;
    char s [100];
    char *cp;
    //
    // Determine the "key"
    //if (Repeat)
    //  DebugAdd ("Edit AutoComplete Repeat");  //####
    SelectWord ();
    LenA = Selected [0] - Selected [1];
    if (!Repeat)   // new target
      {
        LenSearch = LenA;
        Pos = Selected [1];
        PastResults [0] = 0;
      }
    if (LenSearch)
      while (true)
        {
          // Find the first occurrence of the key going backwards
          cp = StrPosPrev (Text, &Text [Pos - 1], &Text [Selected [1]], LenSearch);
          if (cp)
            Pos = cp - Text;
          else   // no matches?
            {
              Beep ();
              break;
            }
          // found a matching word
          if ((Pos == 0) || !IsIdentifier (Text [Pos - 1]))
            {
              // Find length of this identifier
              LenB = 0;
              while (IsIdentifier (Text [Pos + LenB]))
                LenB++;
              // Check for previous result
              if (LenB + 3 < SIZEARRAY (s))   // reject if too big to process past results
                {
                  cp = s;
                  StrCat (&cp, ':');
                  StrCat (&cp, &Text [Pos], LenB);
                  StrCat (&cp, ':');
                  *cp = 0;
                  cp = StrPos (PastResults, s);
                  if (cp == NULL)   // this is a new result
                    {
                      // Replace identifier-at-cursor with completed identifier
                      Delete (Selected [1], LenA);
                      Insert (Selected [1], LenB, &Text [Pos]);
                      Selected [0] = Selected [1] + LenB;   // select the whole word for next time, keep original key
                      // Add completed identifier to PastResults
                      l = StrLen (PastResults);
                      if (l + LenB + 3 < SIZEARRAY (PastResults))
                        {
                          cp = &PastResults [l];
                          StrCat (&cp, s);
                          *cp = 0;
                          DebugAdd (PastResults);//####
                        }
                      break;   // and exit
                    }
                }
            }
        }
  }

const char *SPACE = " ";

void _EditCommon::EditIndentOutdentLines (byte Key)
  {
    int i, j;
    //
    // Put Selected in order and go to start
    if (Selected [1] < Selected [0])
      Swap (Selected [1], Selected [0]);
    MoveToIndex (Selected [0]);
    Selected [0] = IndexLine;
    Posn.x = 0;
    // Start at the cursor and work down
    i = IndexLine;
    j = Posn.y;
    while (true)
      {
        if (Key == KeyRight)
          {
            Insert (i, 1, (char *) SPACE);
            if (Selected [1] != Selected [0])
              Selected [1]++;
          }
        else if (Text [i] == ' ')
          {
            Delete (i, 1);
            if (Selected [1] != Selected [0])
              Selected [1]--;
          }
        RedrawAdd (j);
        i = LineNext (Text, i);
        if (i == 0 || i >= Selected [1])
          break;
        j++;
      }
  }

int UndoInProgress = 0;

void _EditCommon::UndoAdd (_Op op, int Index, int Size)
  {
    _Undo *Undo;
    //
    if (UndoInProgress == 0)
      {
        Undo = (_Undo *) malloc (sizeof (_Undo));
        memset (Undo, 0, sizeof (_Undo));
        Undo->Op = op;
        Undo->Index = Index;
        Undo->Size = Size;
        if (op == opDel || op == opRep)
          {
            Undo->Text = (char *) malloc (Size);
            memcpy (Undo->Text, &Text [Index], Size);
          }
        ListAddStart (Undo, &UndoList);
      }
  }

bool _EditCommon::Undo (void)
  {
    _Undo *u;
    //
    if (UndoList == NULL)
      {
        Beep ();
        return false;
      }
    MoveToIndex (UndoList->Index);
    ScrollToCursor ();
    UndoInProgress++;
    if (UndoList->Op == opDel)
      Insert (UndoList->Index, UndoList->Size, UndoList->Text);
    else if (UndoList->Op == opIns)
      Delete (UndoList->Index, UndoList->Size);
    else if (UndoList->Op == opRep)
      memcpy (&Text [UndoList->Index], UndoList->Text, UndoList->Size);
    free (UndoList->Text);
    u = UndoList;
    ListRemove (UndoList, &UndoList);
    free (u);
    UndoInProgress--;
    Invalidate (true);
    return true;
  }

const char *EOLText [] = {"\n", "\r\n", "\r"};

bool _EditCommon::ProcessKeyDown (_Event *Event)
  {
    bool Res;
    int Key;
    int i, j, k, l;
    _Font *Font_;
    char St [8], *s;
    //
    //DebugAddHex ("Edit:KeyEvent: 0x", Event->Key); //####
    Font_ = FontFind ();
    Key = Event->Key & ~KeyShift;
    Res = true;
    if ((Key >= ' ' && Key < 0x80) || Key == tab || Key == KeyUNICODE)// && ((Event->ShiftState & (KeyCntl | KeyAlt)) == 0))   // Insert visible character
      {
        Clipboard (coDelete);
        s = St;
        if (Key == KeyUNICODE)
          UTF32ToStr (&s, Event->UTF);
        else
          StrCat (&s, (char) Key);
        *s = '\0';
        Insert (IndexLine + Posn.x, StrLen (St), St);
        Posn.x = UTF8Next (&Text [IndexLine], Posn.x);
        Selected [0] = Selected [1] = IndexLine + Posn.x;
      }
    else if (Key == KeyEnter)   // Insert line break
      if (Multiline)
        {
          Clipboard (coDelete);   // replace selected text
          i = 0;   // measure indentation of current line
          while (Text [IndexLine + i] == ' ' || Text [IndexLine + i] == tab)
            i++;
          Insert (IndexLine + Posn.x, StrLen (EOLText [EOL]), (char *) EOLText [EOL]);   // break the line
          if (i)
            Insert (IndexLine + Posn.x + StrLen (EOLText [EOL]), i, &Text [IndexLine]);   // replicate the indentation
          Move (+1);
          Posn.x = i;   // place cursor beyond indent
          Selected [0] = Selected [1] = IndexLine + Posn.x;
          RedrawAdd (IntMax);
        }
      else
        Res = false;
    else if (Key == Cntrl ('D'))   // Duplicate
      {
        i = LineNext (Text, IndexLine);
        Insert (IndexLine, i - IndexLine, &Text [IndexLine]);
        RedrawAdd (IntMax);
      }
    else if (Key == (KeyRight | KeyAlt) || Key == (KeyLeft | KeyAlt))   // Indent/Outdent lines
      EditIndentOutdentLines (Key);
    else if (Key == Cntrl ('K'))   // change case
      {
        SelectWord ();
        int a = Min (Selected [0], Selected [1]);
        int b = Max (Selected [0], Selected [1]);
        MoveToIndex (a);
        UndoAdd (opRep, a, b - 1);
        if (StrChangeCase (&Text [a], b - a))
          Changing ();
      }
    else if (Key == Cntrl ('Z'))   // undo
      Undo ();
    else if (Key == (' ' |  KeyCntl)) //&& (Event->ShiftState & KeyCntl))   // Auto Complete
      EditAutoComplete (KeyPrev == (' ' | KeyCntl));
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
        if (Key == (KeyUp | KeyAlt) && IndexLine > 0)   // Move line up
          {
            j = IndexLine;   // Current
            Move (-1);   // Current - 1
            k = LineNext (Text, j);   // Current + 1
            if (k)
              CharsRotate (&Text [IndexLine], k - IndexLine, IndexLine - j);
          }
        else if (Key == (KeyDown | KeyAlt))   // Move line down
          {
            j = IndexLine;   // Current
            k = LineNext (Text, j);   // Current + 1
            l = LineNext (Text, k);   // Current + 2
            if (l && k)
              {
                CharsRotate (&Text [j], l - j, j - k);
                Move (+1);
              }
          }
        // Cursor movement
        else if ((Key & KeyMax) == KeyLeft)
          if (Posn.x)   // not start of line
            if (Key & KeyCntl)
              Posn.x = PrevWord (&Text [IndexLine], Posn.x);
            else
              Posn.x = UTF8Prev (&Text [IndexLine], Posn.x);
          else   // start of line
            {
              if (IndexLine)   // and NOT start of file
                {
                  if (Move (-1))
                    Posn.x = LineLength (IndexLine);
                }
            }
        else if ((Key & KeyMax) == KeyRight && Text [i])
          if (((byte) Text [i]) >= ' ' || Text [i] == tab)
            if (Key & KeyCntl)
              Posn.x = NextWord (&Text [IndexLine], Posn.x);
            else
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
            if (KeyPrev == KeyEnd && Multiline)
              while (true)
                {
                  i = IndexLine;
                  Move (+1);
                  if (IndexLine == i)
                    break;
                }
            Posn.x = LineLength (IndexLine);
          }
        else if (Key == KeyPageUp)
          Move (-Rect.Height / Font_->YAdvance + 1);
        else if (Key == KeyPageDown)
          Move (Rect.Height / Font_->YAdvance - 1);
        else if (Key == Cntrl ('B'))   // swap with Bookmark
          BookmarkSwap ();
        // Back space
        else if (Key == bs)
          {
            if (Selected [0] != Selected [1])
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
                        Posn.x = LineLength (IndexLine);
                        RedrawAdd (IntMax);
                      }
                    Delete (j, i - j);
                  }
              }
          }
        // Delete character
        else if ((Key & KeyMax)== KeyDel)
          {
            if (Selected [0] == Selected [1])   // nothing selected
              if (Key & KeyCntl)   // ^Del does the whole word
                SelectWord ();
            if (Selected [0] != Selected [1])
              Clipboard (coDelete);
            else if (Text [i])
              {
                // find size of character ...
                if ((byte) Text [i] >= ' ')
                  j = UTF8Next (Text, i);
                else
                  {
                    j = i + 1;
                    if (Text [i] == cr && EOL == eolCRLF)
                      j++;
                    RedrawAdd (IntMax);
                  }
                // ... and remove it
                if (j != i)
                  Delete (i, j - i);
              }
          }
        else   // nothing usefull here
          Res = false;   // so DON'T TAKE THE EVENT
        if (Res)
          {
            // Adjust Position and selection and Range check
            Posn.x = Min (Posn.x, LineLength (IndexLine));
            Selected [0] = IndexLine + Posn.x;
            if (~Event->Key & KeyShift)   // Shift pressed => Extend selection
              Selected [1] = Selected [0];
          }
      }
    if (Res)
      ScrollToCursor ();
    KeyPrev = Key;   // keep this key for next time
    return Res;
  }

void _EditCommon::RedrawAdd (int Line)
  {
    if (RedrawFrom < 0 || Line < RedrawFrom)
      RedrawFrom = Line;
    if (RedrawTo < 0 || Line > RedrawTo)
      RedrawTo = Line;
    Invalidate (false, true);   // partial redraw
    //DebugAddP ("Edit RedrawZone", {RedrawFrom, RedrawTo});//####
  }

bool _EditCommon::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    _Point PosnOld;
    int Sel [2];
    static int Clicks = 0;
    int Len;
    //
    Res = false;
    ChangedNew = false;
    if (Event->Type != etKeyDown || Event->Key != tab || !Multiline)
      FocusCheck (Event, Offset);
    if (Form->Focused == this && Event->Type == etClipboard)
      if (ClipboardData && ClipboardData [0])   // Insert ClipboardData
        {
          Clipboard (coDelete);
          Len = StrLen (ClipboardData);
          RedrawAdd (Posn.y);
          Insert (Selected [0], Len, ClipboardData);
          Selected [0] += Len;
          Selected [1] = Selected [0];
          MoveToIndex (Selected [0]);
          RedrawAdd (IntMax);
        }
    if (IsEventMine (Event, Offset))
      {
        //DebugAddP ("Edit:Event", {Event->Type, Event->MouseKeys}); //####
        PosnOld = Posn;
        Sel [0] = Selected [0];
        Sel [1] = Selected [1];
        //RedrawFrom = RedrawTo = -1;
        Offset.x += EditMargin - Scroll.x;
        Offset.y += EditMargin - Scroll.y;
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)
          {
            Res = true;
            if (++Clicks == 4)
              Clicks = 1;
            FindChar (Event->X - Offset.x, Event->Y - Offset.y);
            Selected [0] = IndexLine + Posn.x;
            if (~Event->ShiftState & KeyShift || Event->Type == etMouseMove)
              {
                Selected [1] = Selected [0];
                if (Clicks == 2)   // double click => select word
                  {
                    Selected [1] = IndexLine + Posn.x;
                    if (Posn.x && Text [IndexLine + Posn.x - 1] > ' ')   // not at the start of the word
                      Selected [1] = IndexLine + PrevWord (&Text [IndexLine], Posn.x);
                    Posn.x = NextWord (&Text [IndexLine], Posn.x);
                    Selected [0] = IndexLine + Posn.x;
                  }
                else if (Clicks == 3)
                  {
                    Selected [1] = IndexLine;
                    Move (+1);
                    Posn.x = 0;
                    Selected [0] = IndexLine;
                  }
              }
            KeyPrev = 0;
          }
        else if (Event->Type == etMouseMove)   // Mouse Move
          {
            Clicks = 0;
            if (Event->MouseKeys == Bit [KeyMouseLeft - 1])   // Mouse Drag
              {
                Res = true;
                FindChar (Event->X - Offset.x, Event->Y - Offset.y);
                Selected [0] = IndexLine + Posn.x;
              }
          }
        else if (Event->Type == etKeyDown)
          if (Event->Key & KeyMax)   // not just a shift key by itself
            Res = ProcessKeyDown (Event);
          else
            Res = false;
      }
    if (Res && ((Posn.x != PosnOld.x || Posn.y != PosnOld.y || Sel [1] != Selected [1]) || ChangedNew))
      {
        RedrawAdd (PosnOld.y);
        if (Multiline)
          {
            RedrawAdd (Posn.y);
            if (Selected [1] != Sel [1] && Sel [1] != Sel [0])   // unrelated new selection
              Invalidate (true);   // full redraw
          }
        //DebugAddP ("Text Select ", {Selected [0], Selected [1]}); //#####
        //DebugAddP ("Posn", Posn);   //####
      }
    return Res;
  }

void _EditCommon::Clipboard (_ClipboardOp ClipboardOp)
  {
    int a, b;
    char Ch;
    //
    a = b = Selected [0];
    if (Selected [0] != Selected [1])
      {
        a = Min (Selected [0], Selected [1]);
        b = Max (Selected [0], Selected [1]);
        // Take a copy if ...
        if (ClipboardOp == coCut || ClipboardOp == coCopy)
          {
            Ch = Text [b];
            Text [b] = 0;
            ClipboardSet (Form->Window, &Text [a]);
            Text [b] = Ch;
          }
        // Delete selection if ...
        if (ClipboardOp == coCut || ClipboardOp == coDelete || ClipboardOp == coPaste)
          {
            //Move to start of selection
            MoveToIndex (a);
            // Delete selection and select nothing
            Delete (a, b - a);
            Selected [0] = Selected [1] = IndexLine + Posn.x;;
            // Redraw the lower portion of _Edit
            RedrawAdd (Posn.y);
            RedrawAdd (IntMax);
          }
      }
    if (ClipboardOp == coPaste)
      ClipboardGet (Form->Window);   // Initiate Clipboard Paste
    if (ClipboardOp == coSelectAll)
      {
        Selected [0] = 0;
        Selected [1] = TextLen;
        Invalidate (true);
      }
  }

#define EditFileMax (IntMax-16)
#define EditFileHexMax (EditFileMax/3)

bool _EditCommon::FileLoad (char *Filename, bool Hex_)
  {
    char *Text_;
    //
    Hex = Hex_;
    Text_ = FileTextReadAll (Filename, EditFileMax, &Hex, &TextLen, &EOL);
    StrAssign (&Text, Text_);
    Changed = false;
    return Text_ != NULL;
  }

    /* moved into lib.cpp
    bool Res;
    int File;
    longint Size;
    int nCR, nLF, nTAB, nNA;
    int c;
    char *Buffer, *BufferHex, *p;
    int SizeHex;
    //
    Res = false;
    File = FileOpen (Filename, foRead);
    if (File >= 0)
      {
        Size = FileSize (File);
        if (Size < EditFileMax)
          {
            Buffer = (char *) malloc (Size + 1);
            if (Buffer)
              if (FileRead (File, (byte *) Buffer, Size) == Size)
                {
                  Buffer [Size] = 0;   // Terminate
                  nCR = nLF = nTAB = nNA = 0;
                  if (Hex_)
                    Hex = true;
                  else if (Size > 16)
                    {
                      p = Buffer;
                      while (true)
                        {
                          c = UTF8Read (&p);   // Read UTF8 character
                          //DebugAddHex ("UFT", c);//####
                          if (c == 0)
                            break;
                          if (c < 0)
                            nNA++;
                          else if (c == cr) nCR++;
                          else if (c == lf) nLF++;
                          else if (c == tab) nTAB++;
                          else if (c < ' ') nNA++;
                        }
                      Hex = (p != Buffer + Size) || (nNA > Size / 4);   // > 25% non ascii "crap"? Looks like a binary file
                    }
                  if (Hex && (Size < EditFileHexMax))   // Hex of acceptable size
                    {
                      SizeHex = DataToHexAscii ((char **) NULL, (byte *) Buffer, Size, 80);
                      BufferHex = (char *) malloc (SizeHex + 1);
                      if (BufferHex)
                        {
                          p = BufferHex;
                          DataToHexAscii (&p, (byte *) Buffer, Size, 80);
                          *p = 0;   // Terminate
                          free (Buffer);
                          Buffer = BufferHex;
                          Size = SizeHex;
                          Res = true;
                        }
                    }
                  else if (!Hex)   // normal text file
                    {
                      if (nLF || nCR)   // Determine EOL type
                        if (nLF > nCR)
                          EOL = eolLF;
                        else if (nCR > nLF)
                          EOL = eolCR;
                        else
                          EOL = eolCRLF;
                      Res = true;
                      DebugAddInt ("TextLoad: EOL", EOL);
                    }
                  if (Res)
                    {
                      StrAssign (&Text, Buffer);
                      TextLen = Size;
                      Changed = false;
                    }
                  else
                    free (Buffer);
                }
          }
        FileClose (File);
      }
    return Res;
  }
  */

bool _EditCommon::FileSave (char *Filename)
  {
    bool Res;
    int File;
    byte *Data;
    int DataSize;
    //
    Res = false;
    if (Hex)
      {
        DataSize = StrGetHexAscii (Text, NULL, 0);
        if (DataSize > 0)
          {
            Data = (byte *) malloc (DataSize);
            if (Data)
              {
                StrGetHexAscii (Text, Data, DataSize);
                File = FileOpen (Filename, foWrite);
                if (File >= 0)
                  {
                    if (FileWrite (File, Data, DataSize))
                      Res = true;
                    FileClose (File);
                  }
                free (Data);
              }
          }
      }
    else
      {
        File = FileOpen (Filename, foWrite);
        if (File >= 0)
          {
            if (FileWrite (File, (byte *) Text, TextLen))
              Res = true;
            FileClose (File);
          }
      }
    if (Res)
      Changed = false;
    return Res;
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
