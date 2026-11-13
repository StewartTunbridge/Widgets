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

char *TextCallBackCursor;
_Point TextCallBackCursorPosn;
char *TextCallBackSelected [2];

const char *EOLText [] = {"\n", "\r\n", "\r"};
const int   EOLLen  [] = {  1,      2,    1};

bool TextCallBackEdit (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size)
  {
    bool Selected_;
    //
    if (Posn)
      if (*Text == TextCallBackCursor)
        TextCallBackCursorPosn = *Posn;
    // Selection?
    if (Window && Posn)
      {
        if (TextCallBackSelected [0] == nullptr)
          Selected_ = false;
        else
          if (TextCallBackSelected [0] < TextCallBackSelected [1])
            Selected_ = (*Text >= TextCallBackSelected [0] && *Text < TextCallBackSelected [1]);
          else
            Selected_ = (*Text >= TextCallBackSelected [1] && *Text < TextCallBackSelected [0]);
        if (Selected_)
          Font->ColourBG = cSelected;
      }
    return false;
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

typedef enum {ccWhite, ccIdentifier, ccSymbol, ccEOL} _CharCategory;

_CharCategory Categorize (char c)
  {
    if ((byte) c == ' ' || c == '\t')
      return ccWhite;
    if (IsAlpha (c) || IsDigit (c) || c == '_')
      return ccIdentifier;
    if (c == '\r' || c == '\n')
      return ccEOL;
    return ccSymbol;
  }

int _EditCommon::LineNext (int Index)
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
            if (c == '\n' || c == '\r')
              return Index + EOLLen [EOL];   // Step past EOL
            Index++;
          }
      }
    return 0;   // no text
  }

int _EditCommon::LinePrev (int Index)
  {
    char c;
    //
    if (Text && *Text)
      {
        if (Index == 0)
          return 0;
        // Step past EOL
        Index -= EOLLen [EOL];
        // step past printables
        while (true)
          {
            if (Index == 0)
              return 0;
            c = Text [Index - 1];
            if (c == '\n' || c == '\r')
              return Index;
            Index--;
          }
      }
    return 0;   // no text
  }

_EditCommon::_EditCommon (_Container *Parent, _Rect Rect, char *Text_, _Action Action_, bool Multi)
  : _Container (Parent, Rect, Text_)
  {
    Action = Action_;
    EditDrawLineCallBack = NULL;
    Multiline = Multi;
    TabIn = true;
    TabOut = !Multiline;
    TextCallBack = &TextCallBackEdit;
    Hex = false;
    EOL = eolLF;
    UTF = utfASCII;
    Colour = cTextBgnd;
    if (!Form->Focused)
      Form->Focused = this;
    UndoList = NULL;
    KeyPrev = 0;
    MovingSelection = false;
    RedrawFrom = RedrawTo = -1;
    TextSet ();
  }

void _EditCommon::UndoClearList (void)
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

bool _EditCommon::UndoAvailable (void)
  {
    return UndoList != NULL;
  }

_EditCommon::~_EditCommon (void)
  {
    UndoClearList ();
  }

void _EditCommon::Changing (void)
  {
    Changed = true;
    ChangedNew = true;
    if (Action)
      Action (this);
  }

void _EditCommon::TextSet (void)
  {
    char *p;
    //
    if (Text == NULL)
      StrAssignCopy (&Text, "");
    if (!Multiline)   // Single line Edit filters out multiple lines
      while (true)
        {
          p = StrPos (Text, '\n');
          if (p)
            *p = ' ';   // change \n
          else
            break;
        }
    TextLen = StrLen (Text);
    Posn = {0, 0};
    IndexLine = 0;
    IndexBookmark = 0;
    Selected [0] = Selected [1] = 0;
    SizeVirtual = {0, 0};
    Scroll = {0, 0};
    UndoClearList ();
    Changed = false;
  }

void _EditCommon::TextSet (char *Text_)
  {
    _Container::TextSet (Text_);
    TextSet ();
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
        i_ = LineNext (i);
        if (i_ == 0)   // Out of Lines
          break;
        i = i_;
        Posn.y++;   // adjust position
        Movement--;
        Res = true;
      }
    while (Movement < 0)
      {
        if (i == 0)
          break;
        else
          {
            i = LinePrev (i);
            Posn.y--;   // adjust position
          }
        Movement++;
        Res = true;
      }
    IndexLine = i;
    //if (!Res)
    //  Beep ();
    return Res;
  }

void _EditCommon::MoveToIndex (int Index)
  {
    int i;
    //
    // Find to start of line
    i = Index;
    while (i > 0 && !StrLineEnd (Text [i - 1]))  // >= ' '
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
    Selected [0] = Selected [1] = Index;
  }

void _EditCommon::MoveToPosn (_Point Posn_)
  {
    Move (Posn_.y - Posn.y);
    Posn.x = Posn_.x;
    Selected [0] = Selected [1] = IndexLine + Posn.x;
    ScrollToCursor ();
    Invalidate ();
  }

void _EditCommon::Insert (int Pos, int Size, char *St)
  {
    char *TextNew;
    //
    if (Pos <= TextLen && Size > 0)
      {
        UndoAdd (opIns, Pos, Size);
        TextNew = (char *) malloc (TextLen + Size + 1);
        if (Pos)
          strncpy (TextNew, Text, Pos);
        if (St)   // St = NULL  =>  just making space
          strncpy (&TextNew [Pos], St, Size);
        if (Text)
          strcpy (&TextNew [Pos + Size], &Text [Pos]);
        else
          TextNew [Size] = 0;
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
    if (Text)
      while (true)
        {
          c = Text [Index + i];
          if (c == '\r' || c == '\n' || c == 0)
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
    if (Line != Posn.y)
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
    _Font *Font_;
    int l;
    //
    //DebugAddP ("DrawTextLine", {Index, y}); //####
    Size = TextMeasure (&Text [Index]);
    Font_ = FontFind ();
    if (Font_->Colour < 0)
      Font_->Colour = ColourTextFind ();
    if (RenderFlags & rRedrawPart)
      RenderFillRect ({Scroll.x, y, Scroll.x + Rect.Width, Font_->YAdvance/*Size.y*/}, ColourFind ());
    if (Index <= TextLen)
      {
        l = LineLength (Index);
        if (EditDrawLineCallBack)
          EditDrawLineCallBack (this, &Text [Index], l);
        TextCallBackCursorPosn.x = -1;
        TextRender (Form->Window, Font_, &Text [Index], {EditMargin, y});
        if (TextCallBackCursorPosn.x >= 0)   // we have a cursor
          {
            //DebugAddP ("Cursor Position", TextCallBackCursorPosn); //####
            l = Max ((Font_->YAdvance + 0) / 8, 2);
            RenderFillRect ({TextCallBackCursorPosn.x - l / 2, TextCallBackCursorPosn.y, l, Font_->YAdvance}, Font_->Colour);
          }
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
    if (Text == NULL)  //####
      DebugAdd ("Edit: Text == NULL");  // ####
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
        Index = LineNext (Index);
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
      {
        if (Size.x > SizeVirtual.x)
          {
            SizeVirtual.x = Size.x;
            Invalidate ();
          }
        if (Size.y > SizeVirtual.y)
          {
            SizeVirtual.y = Size.y;
            Invalidate ();
          }
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
    UndoAdd (opOvr, Txt - Text, Size);
    StrCharsRotate (Txt, Size, Shift);
    Changing ();
  }

void _EditCommon::BookmarkSwap (void)
  {
    int bm;
    //
    bm = Selected [1];
    MoveToIndex (IndexBookmark);
    ScrollToCursor ();
    Invalidate ();
    IndexBookmark = bm;
  }

bool _EditCommon::SelectWord (void)
  {
    _CharCategory Type;
    //
    if (Selected [0] == Selected [1])   // no selection so select a word
      {
        Type = Categorize (Text [Max (Selected [0] - 1, 0)]);
        if (Type != ccIdentifier)   // if next to an identifier, pick it
          Type = Categorize (Text [Selected [0]]);
        if (Type != ccEOL)
          {
            while (Categorize (Text [Selected [0]]) == Type)
              Selected [0]++;
            while (Selected [1] > 0 && Categorize (Text [Selected [1] - 1]) == Type)
              Selected [1]--;
          }
      }
    if (Selected [0] == Selected [1])
      return false;
    return true;
  }

void _EditCommon::SelectLine (void)
  {
    Selected [1] = IndexLine;
    if (Multiline)
      {
        Move (+1);
        Posn.x = 0;
        Selected [0] = IndexLine;
      }
    else   // Single line
      Selected [0] = LineLength (IndexLine);
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
    if (SelectWord ())
      {
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
              if (Pos == 0 || Categorize (Text [Pos - 1]) != ccIdentifier)
                {
                  // Find length of this identifier
                  LenB = 0;
                  while (Categorize (Text [Pos + LenB]) == ccIdentifier)
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
                          // Add completed identifier to PastResults
                          l = StrLen (PastResults);
                          if (l + LenB + 3 < SIZEARRAY (PastResults))
                            {
                              cp = &PastResults [l];
                              StrCat (&cp, s);
                              *cp = 0;
                              //DebugAdd (PastResults);//####
                            }
                          break;   // and exit
                        }
                    }
                }
            }
        Selected [0] = Selected [1];
      }
  }

const char SPACE = ' ';
const char TAB = '\t';

void _EditCommon::EditIndentOutdentLines (byte Key)
  {
    int a, b, j;
    //
    // Put Selected in order and go to start of top line
    if (Selected [1] < Selected [0])
      Swap (Selected [1], Selected [0]);
    b = Selected [1];
    MoveToIndex (Selected [0]);
    Selected [0] = IndexLine;
    Posn.x = 0;
    // Walk down [in/out]denting lines
    a = IndexLine;
    j = Posn.y;
    while (true)
      {
        if (Key == KeyRight)
          {
            if (Text [a] == '\t')
              Insert (a, 1, (char *) &TAB);
            else
              Insert (a, 1, (char *) &SPACE);
            b++;
          }
        else if (Text [a] == ' ' || Text [a] == '\t')
          {
            Delete (a, 1);
            b--;
          }
        RedrawAdd (j);
        a = LineNext (a);
        if (a == 0 || a >= b)
          break;
        j++;
      }
    // Make selection the block of complete lines
    Selected [1] = a;
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
        if (op == opDel || op == opOvr)
          {
            Undo->Text = (char *) malloc (Size);
            memcpy (Undo->Text, &Text [Index], Size);
          }
        ListAddStart (Undo, &UndoList);
      }
  }

void _EditCommon::Undo (void)
  {
    _Undo *u;
    //
    if (UndoList == NULL)
      Beep ();
    else
      {
        MoveToIndex (UndoList->Index);
        ScrollToCursor ();
        UndoInProgress++;
        if (UndoList->Op == opDel)
          Insert (UndoList->Index, UndoList->Size, UndoList->Text);
        else if (UndoList->Op == opIns)
          Delete (UndoList->Index, UndoList->Size);
        else if (UndoList->Op == opOvr)
          memcpy (&Text [UndoList->Index], UndoList->Text, UndoList->Size);
        free (UndoList->Text);
        u = UndoList;
        ListRemove (UndoList, &UndoList);
        free (u);
        UndoInProgress--;
        Invalidate ();
      }
  }

const char Space = ' ';

bool _EditCommon::ProcessKeyDown (_Event *Event)
  {
    bool Res;
    int Key;
    int i, j, k, l;
    _Font *Font_;
    char St [8], *s;
    //
    //DebugAddHex ("Edit:KeyEvent: 0x", Event->ShiftState << 8 | (Event->Key & KeyMax)); //####
    if (Event->Key == '\t' && !Multiline)
      return false;
    Font_ = FontFind ();
    Key = Event->Key & ~KeyShift;
    Res = true;
    if ((Key >= ' ' && Key < 0x80) || Key == '\t' || Key == KeyUNICODE)   // Insert visible character
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
    else if ((Key & KeyMax) == KeyEnter && Multiline)   // Insert line break
      {
        Clipboard (coDelete);   // replace selected text
        if (Key & KeyCntl)   // ^Enter inserts at end of current line
          Posn.x = LineLength (IndexLine);
        i = 0;   // measure indentation of current line
        while (Text [IndexLine + i] == ' ' || Text [IndexLine + i] == '\t')
          i++;
        Insert (IndexLine + Posn.x, EOLLen [EOL], (char *) EOLText [EOL]);   // break the line
        if (i <= Posn.x)   // If cursor is beyond the line indentation
          {
            Insert (IndexLine + Posn.x + EOLLen [EOL], i, &Text [IndexLine]);   // replicate the indentation
            Posn.x = i;   // place cursor beyond indent
          }
        else   // cursor was within the indentation
          Posn.x = 0;   // so go to the start of the new line
        Move (+1);
        Selected [0] = Selected [1] = IndexLine + Posn.x;
        RedrawAdd (IntMax);
      }
    else if (Key == Cntrl ('D') && Multiline)   // Duplicate
      {
        if (Selected [0] == Selected [1])   // no selection
          {
            i = LineNext (IndexLine);
            Insert (IndexLine, i - IndexLine, &Text [IndexLine]);
            RedrawAdd (IntMax);
          }
        else   // Selection exists
          Insert (Selected [0], Abs (Selected [0] - Selected [1]), &Text [Min (Selected [0], Selected [1])]);
      }
    else if (Key == (KeyRight | KeyAlt) || Key == (KeyLeft | KeyAlt))   // Indent/Outdent lines
      EditIndentOutdentLines (Key);
    else if (Key == Cntrl ('K'))   // change case
      {
        if (SelectWord ())
          {
            int a = Min (Selected [0], Selected [1]);
            int b = Max (Selected [0], Selected [1]);
            UndoAdd (opOvr, a, b - 1);
            if (StrChangeCase (&Text [a], b - a))
              Changing ();
          }
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
    else if (Key == Control ('W'))
      SelectWord ();
    else
      {
        i = IndexLine + Posn.x;
        if (IndexLine > 0 && (Event->Key == (KeyUp | KeyAlt) || Event->Key == (KeyUp | KeyCntl | KeyShift)))   // Move line up
          {
            j = IndexLine;   // Current
            Move (-1);   // Current - 1
            k = LineNext (j);   // Current + 1
            if (k)
              CharsRotate (&Text [IndexLine], k - IndexLine, IndexLine - j);
          }
        else if (Event->Key == (KeyDown | KeyAlt) || Event->Key == (KeyDown | KeyCntl | KeyShift))   // Move line down
          {
            j = IndexLine;   // Current
            k = LineNext (j);   // Current + 1
            l = LineNext (k);   // Current + 2
            if (l && k)
              {
                CharsRotate (&Text [j], l - j, j - k);
                Move (+1);
              }
          }
        // Cursor movement
        else if ((Key & KeyMax) == KeyLeft && Selected [0] > 0)   // Left
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
        else if ((Key & KeyMax) == KeyRight && Text [i])   // Right
          if (!StrLineEnd ((byte) Text [i]))  // >= ' ' || Text [i] == tab)
            if (Key & KeyCntl)
              Posn.x = NextWord (&Text [IndexLine], Posn.x);
            else
              Posn.x = UTF8Next (&Text [IndexLine], Posn.x);
          else
            {
              Move (+1);
              Posn.x = 0;
            }
        else if (Key == KeyUp && Multiline)
          Move (-1);
        else if (Key == KeyDown && Multiline)
          Move (+1);
        else if ((Key & KeyMax) == KeyHome)
          {
            Posn.x = 0;
            if (Key & KeyCntl)
              {
                Posn.y = 0;
                IndexLine = 0;
              }
          }
        else if ((Key & KeyMax) == KeyEnd)
          {
            if ((Key & KeyCntl) && Multiline)
              while (true)
                {
                  i = IndexLine;
                  j = LineNext (i);
                  if (j == 0)
                    break;
                  IndexLine = j;
                  Posn.y++;
                }
            Posn.x = LineLength (IndexLine);
          }
        else if (Key == KeyPageUp && Multiline)
          Move (-Rect.Height / Font_->YAdvance + 1);
        else if (Key == KeyPageDown && Multiline)
          Move (Rect.Height / Font_->YAdvance - 1);
        else if (Key == Cntrl ('B'))   // swap with Bookmark
          BookmarkSwap ();
        // Back space
        else if (Key == '\b')
          {
            if (Selected [0] != Selected [1])
              Clipboard (coDelete);
            else if (i)
              {
                if (Posn.x)
                  j = UTF8Prev (Text, i);
                else
                  j = i - EOLLen [EOL]; // was j = i - 1;
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
              else if (Event->Key & KeyShift)
                SelectLine ();
              else if (Event->Key & KeyAlt)
                while (Text [Selected [0]] >= ' ')   // Select to end of line
                  Selected [0]++;
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
                    if (Text [i] == '\r' && EOL == eolCRLF)
                      j++;
                    RedrawAdd (IntMax);
                  }
                // ... and remove it
                if (j != i)
                  Delete (i, j - i);
              }
          }
        else if ((Key & KeyMax)== KeyIns)
          Insert (IndexLine + Posn.x, 1, (char *) &Space);
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
    Invalidate (rRedrawPart);   // partial redraw
    //DebugAddP ("Edit RedrawZone", {RedrawFrom, RedrawTo});//####
  }

bool _EditCommon::IsWithinSelection (int Index)
  {
    if (Selected [0] < Selected [1])
      return (Index >= Selected [0]) && (Index < Selected [1]);
    return (Index >= Selected [1]) && (Index < Selected [0]);
  }

bool _EditCommon::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    _Point PosnOld;
    int Sel [2];
    int SelMin, SelMax, i;
    static int Clicks = 0;
    //
    if (!FontFind ())
      return true;
    Res = false;
    ChangedNew = false;
    //if (Event->Type != etKeyDown || Event->Key != '\t' || !Multiline)
    //  FocusCheck (Event, Offset);
    //if (IsEventMine (Event, Offset))
      {
        //DebugAddP ("Edit:Event", {Event->Type, Event->MouseKeys}); //####
        PosnOld = Posn;
        Sel [0] = Selected [0];
        Sel [1] = Selected [1];
        SelMin = Min (Sel [0], Sel [1]);
        SelMax = Max (Sel [0], Sel [1]);
        //RedrawFrom = RedrawTo = -1;
        Offset.x += EditMargin - Scroll.x;
        Offset.y += EditMargin - Scroll.y;
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)   // Mouse Left Click
          {
            Res = true;
            KeyPrev = 0;   // cancel potential erroneous double keys
            Clicks++;
            //DebugAddInt ("Clicks", Clicks); // ####
            FindChar (Event->X - Offset.x, Event->Y - Offset.y);
            MovingSelection = IsWithinSelection (IndexLine + Posn.x) && Clicks == 1;
            //if (MovingSelection) //####
            //  DebugAdd ("Moving");
            if (!MovingSelection)   // moving selection
              {
                Selected [0] = IndexLine + Posn.x;
                if (~Event->ShiftState & KeyShift) // || Event->Type == etMouseMove)
                  {
                    Selected [1] = Selected [0];
                    if (Clicks == 2)   // double click => select word
                      SelectWord ();
                    else if (Clicks == 3)   // triple click => select line
                      SelectLine ();
                    else if (Clicks >= 4)   // too many clicks
                      Clicks = 1;
                  }
              }
          }
        else if (Event->Type == etMouseMove)   // Mouse Move
          {
            Clicks = 0;
            if (Event->MouseKeys == Bit [KeyMouseLeft - 1])   // Mouse Drag
              {
                Res = true;
                FindChar (Event->X - Offset.x, Event->Y - Offset.y);
                if (!MovingSelection)
                  Selected [0] = IndexLine + Posn.x;
              }
          }
        else if (Event->Type == etMouseUp)   // Mouse Up
          {
            i = IndexLine + Posn.x;
            if (MovingSelection)
              {
                if (!IsWithinSelection (i))
                  {
                    if (Event->ShiftState & KeyCntl)   // Copy Selection
                      {
                        Insert (IndexLine + Posn.x, SelMax - SelMin, &Text [SelMin]);
                        RedrawAdd (Posn.y);
                        RedrawAdd (IntMax);
                      }
                    else   // Move Selection
                      {
                        if (i < SelMin)   // moving up
                          CharsRotate (&Text [i], SelMax - i, SelMax - SelMin);
                        else   // moving down
                          {
                            CharsRotate (&Text [SelMin], i - SelMin, SelMin - SelMax);
                            IndexLine = 0;   // Sort out position
                            Posn.y = 0;
                            MoveToIndex (i);
                          }
                      }
                  }
                Selected [0] = Selected [1] = IndexLine + Posn.x;
                Invalidate ();
              }
          }
        else if (Event->Type == etKeyDown)
          if (Event->Key & KeyMax)   // not just a shift key by itself
            Res = ProcessKeyDown (Event);
          else
            Res = false;
      }
    if (Res && ((Posn.x != PosnOld.x || Posn.y != PosnOld.y || Sel [0] != Selected [0] || Sel [1] != Selected [1]) || ChangedNew))
      {
        RedrawAdd (PosnOld.y);
        if (Multiline)
          {
            RedrawAdd (Posn.y);
            if (Selected [1] != Sel [1] && Sel [1] != Sel [0])   // unrelated new selection
              Invalidate ();   // full redraw
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
    int Len;
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
            // Redraw the lower portion of _Edit
            RedrawAdd (Posn.y);
            RedrawAdd (IntMax);
          }
      }
    if (ClipboardOp == coPaste)
      {
        if (ClipboardGet (Form->Window))
          if (ClipboardData && *ClipboardData)   // Insert ClipboardData
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
      }
    if (ClipboardOp == coSelectAll)
      {
        Selected [0] = 0;
        Selected [1] = TextLen;
        Invalidate ();
      }
  }

#define EditFileMax (IntMax-16)
#define EditFileHexMax (EditFileMax/3)

bool _EditCommon::FileLoad (char *Filename, bool Hex_)
  {
    char *Text_;
    //
    Hex = Hex_;
    Text_ = FileTextReadAll (Filename, EditFileMax, &Hex, &TextLen, &EOL, &UTF);
    StrAssign (&Text, Text_);
    Changed = false;
    return Text_ != NULL;
  }

bool _EditCommon::FileSave (char *Filename)
  {
    bool Res;
    int File;
    byte *Data;
    int DataSize;
    //
    Res = false;
    FileBackup (Filename);
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


//////////////////////////////////////////////////////////////////////////////
//
// Edit Text: with Popups

void ActionEditPopupMenu (_MenuPopup *Menu)
  {
    _EditPopup *Edit;
    //
    Edit = (_EditPopup *) Menu->Parent;
    switch (Menu->Selected)
      {
        case 0: Edit->Clipboard (coCut);   break;
        case 1: Edit->Clipboard (coCopy);  break;
        case 2: Edit->Clipboard (coPaste); break;
        case 3: Edit->TextSet (NULL);      break;
      }
  }

_EditPopup::_EditPopup (_Container *Parent, _Rect Rect_, char *Text_, _Action Action, bool Multi)
  : _EditCommon (Parent, Rect_, Text_, Action, Multi)
  {
    pmEdit = new _MenuPopup (this, {0, 0, 0, 0}, "Cut\tCopy\t_Paste\tClear", (_Action) ActionEditPopupMenu);
  }


//////////////////////////////////////////////////////////////////////////////
//
// Edit Text: with Popups and Drop List

void ActionEditDropListSelectionList (_SelectionList *SelectionList)
  {
    _EditPopupDropList *DropList;
    char *Item;
    //
    DropList = (_EditPopupDropList *) SelectionList->DataContainer;
    if (SelectionList->Selected >= 0)
      {
        if (DropList->SelectionList)
          {
            DropList->Selected = SelectionList->Selected;
            Item = StrGetItem (DropList->TextGet (), DropList->Selected);
            DropList->EditPopup->TextSet (Item);
            free (Item);
            if (DropList->Action)
              DropList->Action ((_Container *) DropList);
          }
      }
    SelectionList->Die ();
    DropList->SelectionList = NULL;
  }

void _EditPopupDropList::DropListAdd (char *Item, int MaxItems)
  {
    char *St;
    //
    St = NULL;
    StrAssignCopy (&St, TextGet ());
    StrAddItemStart (&St, Item, MaxItems);
    TextSet (St);
  }

_EditPopupDropList::_EditPopupDropList (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_)
  : _Container (Parent, Rect_, Text_)
  {
    Action = Action_;
    Selected = -1;
    SelectionList = NULL;
    EditPopup = new _EditPopup (this, {0, 0, -Rect.Height * 2/3, 0}, NULL, Action);
  };

void _EditPopupDropList::DrawCustom (void)
  {
    int i;
    //
    // Draw Arrow and bexel
    i = Rect.Height;
    DrawArrow ({Rect.Width - i * 2/3, 0, i/2, Rect.Height}, dDown);
    //DrawBezel (2);
  };

bool _EditPopupDropList::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    if (IsEventMine (Event, Offset))
      {
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)
          {
            SelectionList = new _SelectionList (Form->Container, {Offset.x, Offset.y + Rect.Height, 16, 16}, Text, (_Action) ActionEditDropListSelectionList);
            SelectionList->DataContainer = this;   // remember where we came from
            return true;
          }
      }
    return false;
  }

