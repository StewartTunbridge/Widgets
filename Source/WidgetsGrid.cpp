///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - GRIDVIEW
//
///////////////////////////////////////////////////////////////////////////////


#include "Widgets.hpp"
#include "WidgetsDriver.hpp"

#include "WidgetsGrid.hpp"

// TODO:
// add Flags:
//  AllowMultiSelect
//  AllocColumnAdjust
//  ..
//  add Title (point)
//  split Locked funtionality with Title
//  Title effects appearance and editability
//  Locked effects scrolling (add a bolder line around Locked)

void _GridView::DrawCellsAdd (_Point p)
  {
    if (DrawCells0.x < 0)
      DrawCells0 = DrawCells1 = p;
    else
      {
        DrawCells0.x = Min (DrawCells0.x, p.x);
        DrawCells0.y = Min (DrawCells0.y, p.y);
        DrawCells1.x = Max (DrawCells1.x, p.x);
        DrawCells1.y = Max (DrawCells1.y, p.y);
      }
    Invalidate (true);
    RenderFlags |= rRedrawPart;
  }

void _GridView::DrawCellsClear (void)
  {
    DrawCells0.x = -1;
  }

bool _GridView::DrawCellsIn (_Point p)
  {
    if (DrawCells0.x < 0)
      return true;
    if (p.x >= DrawCells0.x && p.x <= DrawCells1.x)
      if (p.y >= DrawCells0.y && p.y <= DrawCells1.y)
        return true;
    return false;
  }

_GridView::_GridView (_Container *Parent, _Rect Rect_, char *Text_, int Columns_, int Rows_, _ActionCellDraw ActionCellDraw_) : _Container (Parent, Rect_, Text_)
  {
    Columns = Columns_;
    Rows = Rows_;
    Options = goGridLines | goSelectMulti;
    DrawCells0.x = -1;
    ActionCellDraw = ActionCellDraw_;
    ActionCellRead = NULL;
    ActionCellWrite = NULL;
    Edit = NULL;
    Locked = {1, 1};
    Title = {1, 1};
    Colour = cTextBgnd;
    ColourGrad = -1;
    Index0 = Index1 = {1, 1};
    CellOffset = {0, 0};
    CellSizeDefault = TextMeasure (Font, "AgAgAgAg");
    CellSizeDefault = {Max (CellSizeDefault.x, 16), Max (CellSizeDefault.y, 4)};
    CellSizeDefault.y = CellSizeDefault.y * 3 / 2;
    CellWidths = NULL;
    CellHeights = NULL;
    ColumnAdjust = -1;
    if (!Form->KeyFocus)
      Form->KeyFocus = this;
  }

_GridView::~_GridView (void)
  {
    if (CellWidths)
      free (CellWidths);
    if (CellHeights)
      free (CellHeights);
  }

void _GridView::CellWidthSet (int Column, int Width)
  {
    ArraySet (&CellWidths, Column, Width);
    Invalidate (true);
  }

int _GridView::CellWidthGet (int Column)
  {
    return ArrayGet (&CellWidths, Column, CellSizeDefault.x);
  }

void _GridView::CellHeightSet (int Row, int Height)
  {
    ArraySet (&CellHeights, Row, Height);
    Invalidate (true);
  }

int _GridView::CellHeightGet (int Row)
  {
    return ArrayGet (&CellHeights, Row, CellSizeDefault.y);
  }

_Point _GridView::CellGridPosnToIndex (_Point Position)
  {
    _Point Res;
    //
    if (Position.x < Locked.x)
      Res.x = Position.x;
    else
      Res.x = Position.x + CellOffset.x;
    if (Position.y < Locked.y)
      Res.y = Position.y;
    else
      Res.y = Position.y + CellOffset.y;
    return Res;
  }

_Point _GridView::CellIndexToGridPosn (_Point Index)
  {
    _Point Res;
    //
    if (Index.x < Locked.x)
      Res.x = Index.x;
    else
      Res.x = Index.x - CellOffset.x;
    if (Index.y < Locked.y)
      Res.y = Index.y;
    else
      Res.y = Index.y - CellOffset.y;
    return Res;
  }

bool _GridView::CellInSelection (_Point Index)
  {
    if (Index.x >= Min (Index0.x, Index1.x))
      if (Index.x <= Max (Index0.x, Index1.x))
        if (Index.y >= Min (Index0.y, Index1.y))
          if (Index.y <= Max (Index0.y, Index1.y))
            return true;
    return false;
  }

_Rect _GridView::CellRect (_Point Posn)
  {
    _Rect Res;
    _Point p, Index;
    int x, y;
    //
    p = {0, 0};
    x = y = 0;
    while (true)
      {
        Index = CellGridPosnToIndex (p);
        if (p.x >= Posn.x)
          break;
        x += CellWidthGet (Index.x);
        p.x++;
      }
    while (true)
      {
        Index = CellGridPosnToIndex (p);
        if (p.y >= Posn.y)
          break;
        y += CellHeightGet (Index.y);
        p.y++;
      }
    Res = {x, y, CellWidthGet (Index.x), CellHeightGet (Index.y)};
    if (Index.x + 1 == Columns && CellOffset.x == 0)   // Widen the right-most cells
      Res.Width = Rect.Width - Res.x;
    return Res;
  }

_Point _GridView::CellAtXY (int x, int y, bool *MidColumn)
  {
    _Point Posn;
    _Point Index;
    int x_, y_;
    bool mc;
    //
    mc = false;
    Posn = {0, 0};
    x_ = y_ = 0;
    while (true)
      {
        Index = CellGridPosnToIndex (Posn);
        x_ += CellWidthGet (Index.x);
        if (MidColumn)
          if (Abs (x_ - x) < 4)   // within 4 pixels of column gap
            {
              mc = true;
              break;
            }
        if (x_ >= x)
          break;
        Posn.x++;
      }
    while (true)
      {
        Index = CellGridPosnToIndex (Posn);
        y_ += CellHeightGet (Index.y);
        if (y_ >= y)
          break;
        Posn.y++;
      }
    if (MidColumn)
      *MidColumn = mc;
    return Posn;
  }

void _GridView::DrawCustom (void)
  {
    _Point Posn;
    _Point Ind;
    int ColGrid, ColGridFocus, ColLockBG, ColSelBG;
    _Rect RectCell;
    int c1, c2;
    int x2;
    _Font *Font;
    //
    /* char s [80], *ps;
    if (DrawCells0.x >= 0)
      {
        ps = s;
        StrCat (&ps, "DrawCells: ");
        PointToStr (&ps, DrawCells0);
        StrCat (&ps, " - ");
        PointToStr (&ps, DrawCells1);
        *ps = 0;
        DebugAdd (s);
      } */
    //
    ColLockBG = Parent->ColourFind ();
    ColGrid = ColourAdjust (cForm1, 150);
    ColGridFocus = ColourAdjust (ColourTextFind (), 140);
    ColSelBG = cSelected;
    Font = FontFind ();
    Posn = {0, 0};
    ScrollBars = 0;
    if (CellOffset.x)
      ScrollBars |= sbHorz;
    if (CellOffset.y)
      ScrollBars |= sbVert;
    while (true)
      {
        Font->ColourBG = ColourFind ();
        Ind = CellGridPosnToIndex (Posn);
        RectCell = CellRect (Posn);
        if (Ind.x >= Columns || RectCell.x > Rect.Width)
          {
            if (Ind.x < Columns)
              ScrollBars |= sbHorz;
            Posn.x = 0;
            Posn.y++;
            Ind = CellGridPosnToIndex (Posn);
            RectCell = CellRect (Posn);
          }
        if (Ind.y >= Rows)
          break;
        if (RectCell.y >= Rect.Height)
          {
            ScrollBars |= sbVert;
            break;
          }
        if (DrawCellsIn (Ind))
          {
            // Draw cell ornaments
            if (Ind.x < Title.x || Ind.y < Title.y)   // Draw Title Cell Background
              {
                DrawRectangle (RectCell, -1, -1, ColLockBG);
                DrawBorder (RectCell, bRaised, ColLockBG, 0);
                Font->ColourBG = ColLockBG;
              }
            else
              {
                //if ((~Options & goSelectLine) && Ind.x == Index1.x && Ind.y == Index1.y && Form->KeyFocus == this)   // Current selection
                if (Ind.x == Index1.x && Ind.y == Index1.y && Form->KeyFocus == this)
                  DrawRectangle (RectCell, ColGridFocus, ColGridFocus, ColourFind ());
                else
                  {
                    if (CellInSelection (Ind))   // Current multiple selection
                      {
                        DrawRectangle (RectCell, -1, -1, ColSelBG);
                        Font->ColourBG = ColSelBG;
                      }
                    else if (RenderFlags & rRedrawPart)
                      DrawRectangle (RectCell, -1, -1, ColourFind ());
                    // Grid lines
                    c1 = c2 = -1;
                    if (Options & goGridLines)
                      c1 = c2 = ColGrid;
                    if (Ind.x + 1 == Locked.x && Ind.y >= Locked.y)
                      c1 = ColGridFocus;
                    if (Ind.y + 1 == Locked.y && Ind.x >= Locked.x)
                      c2 = ColGridFocus;
                    x2 = RectCell.x + RectCell.Width - 1;
                    if (x2 >= Rect.Width)
                      x2 = Rect.Width - 1;
                    if (Ind.x < Columns)
                      if (c2 >= 0)
                        DrawLine (RectCell.x, RectCell.y + RectCell.Height - 1, x2, RectCell.y + RectCell.Height - 1, c2);
                    if (c1 >= 0)
                      DrawLine (RectCell.x + RectCell.Width - 1, RectCell.y, RectCell.x + RectCell.Width - 1, RectCell.y + RectCell.Height - 1, c1);
                  }
              }
            // Draw Cell user contents
            if (ActionCellDraw)
              ActionCellDraw (this, Ind.x, Ind.y, RectCell);
          }
        Posn.x++;
      }
    ScrollBarsDraw (ScrollBars, CellOffset, {Columns - Locked.x, Rows - Locked.y});
    //Posn = CellAtXY (Rect.Width - 1, Rect.Height - 1);
    //ScrollBarsDraw ({CellOffset.x + Locked.x, CellOffset.y + Locked.y}, CellGridPosnToIndex (Posn), Locked, {Columns - 1, Rows - 1});
    DrawCellsClear ();
  }

bool _GridView::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    _Point Index0Old, Index1Old;
    _Point CellOffsetOld;
    _Point Posn;
    _Point Index;
    _Rect Rec;
    int y;
    bool MidColumn;
    bool Res;
    char *Data;
    bool OK;
    static _Point LastClick = {-1, -1};
    bool DoubleClick;
    //
    if (Edit)
      {
        if (Event->Type == etKeyDown && (Event->Key == KeyEnter || Event->Key == esc))
          {
            if (Event->Key == KeyEnter && ActionCellWrite)
              ActionCellWrite (this, Index1.x, Index1.y, Edit->TextGet ());
            delete Edit;
            Edit = NULL;
            Form->KeyFocus = this;
            Form->EventLock = NULL;
            DrawCellsAdd (Index1);
          }
        return true;
      }
    CheckFocus (Event, Offset);
    Res = false;
    Index1Old = Index1;
    Index0Old = Index0;
    CellOffsetOld = CellOffset;
    DoubleClick = false;
    if (IsEventMine (Event, Offset))   // Either by location OR Forced
      {
        Res = true;
        Posn = CellAtXY (Rect.Width - 1, Rect.Height - 1);
        if (!ScrollBarsEvent (Event, Offset, ScrollBars, {Columns, Rows}, &CellOffset))
          {
            if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)   // Left Mouse Down
              {
                DoubleClick = (Event->X == LastClick.x && Event->Y == LastClick.y);
                LastClick = {Event->X, Event->Y};
                Posn = CellAtXY (Event->X - Offset.x, Event->Y - Offset.y, &MidColumn);
                Index = CellGridPosnToIndex (Posn);
                if (Index.y < Locked.y && MidColumn && (Options & goAdjustColumn))   // Adjusting Column Width?
                  {
                    ColumnAdjust = Index.x;   // Adjusting a Column
                    ColumnAdjustX = Event->X;
                    Form->EventLock = this;
                  }
                else
                  {
                    ColumnAdjust = -1;
                    if (Index.x < Title.x)
                      {
                        if (Options & goSelectMulti)   // Select whole rows
                          {
                            Index0 = {Columns - 1, Index.y};
                            Index1 = {0, Index.y};
                          }
                      }
                    else if (Index.y < Title.y)
                      {
                        if (Options & goSelectMulti)   // Select whole columns
                          {
                            Index0 = {Index.x, Rows - 1};
                            Index1 = {Index.x, 0};
                          }
                      }
                    else
                      {
                        Index1 = Index;
                        if (~Event->ShiftState & KeyShift)
                          Index0 = Index1;
                      }
                    Form->EventLock = this;
                  }
              }
            else if (Event->Type == etMouseMove)
              {
                Posn = CellAtXY (Event->X - Offset.x, Event->Y - Offset.y, &MidColumn);
                Index = CellGridPosnToIndex (Posn);
                if (Event->MouseKeys == Bit [KeyMouseLeft - 1])
                  if (ColumnAdjust >= 0)   // Adjusting a column
                    {
                      CellWidthSet (ColumnAdjust, CellWidthGet (ColumnAdjust) + Event->X - ColumnAdjustX);
                      ColumnAdjustX = Event->X;
                    }
                  else   // Stretching the selection
                    Index1 = Index;
                LastClick.x = -1;   // not part of a double click
              }
            else if (Event->Type == etMouseUp && Event->Key == KeyMouseLeft)   // Mouse up, stop drag operations
              {
                ColumnAdjust = -1;
                Form->EventLock = NULL;
              }
            else if (Event->Type == etKeyDown)
              {
                OK = true;
                switch ((Event->Key & KeyMax))
                  {
                    case KeyUp:       Index1.y--; break;
                    case KeyDown:     Index1.y++; break;
                    case KeyLeft:     Index1.x--; break;
                    case KeyRight:    Index1.x++; break;
                    case KeyPageUp:   y = 0;
                                      while (y + 2 * CellHeightGet (Index1.y) < Rect.Height - CellRect (Locked).y)
                                        {
                                          y += CellHeightGet (Index1.y);
                                          Index1.y--;
                                        }
                                      break;
                    case KeyPageDown: y = 0;
                                      while (y + 2 * CellHeightGet (Index1.y) < Rect.Height - CellRect (Locked).y)
                                        {
                                          y += CellHeightGet (Index1.y);
                                          Index1.y++;
                                        }
                                      break;
                    case KeyHome:     Index1.y = 0; break;
                    case KeyEnd:      Index1.y = Rows; break;
                    case KeyEnter:
                    case ' ':         DoubleClick = true;
                                      break;
                    default: OK = false;
                  }
                if (OK)
                  if (~Event->ShiftState & KeyShift)
                    Index0 = Index1;
                while (Index1.x - CellOffset.x < Locked.x)
                  CellOffset.x--;
                while (CellRect (CellIndexToGridPosn ({Index1.x + 1, Index1.y})).x > Rect.Width)
                  CellOffset.x++;
                while (Index1.y - CellOffset.y < Locked.y)
                  CellOffset.y--;
                while (CellRect (CellIndexToGridPosn ({Index1.x, Index1.y + 1})).y > Rect.Height)
                  CellOffset.y++;
              }
          }
      }
    Limit (&Index1.x, Title.x, Columns - 1);
    Limit (&Index1.y, Title.y, Rows - 1);
    if (~Options & goSelectMulti)
      Index0 = Index1;
    if (Options & goSelectLine)
      {
        Index0.x = Columns - 1;
        Index1.x = Title.x;
      }
    if (DoubleClick)
      if (ActionCellRead)
        {
          Posn = CellIndexToGridPosn (Index1);
          Data = ActionCellRead (this, Index1.x, Index1.y);
          if (Data)
            {
              Rec = CellRect (Posn);
              Edit = new _Edit (this, Rec, Data);
              free (Data);
              Form->KeyFocus = Edit;
              Form->EventLock = Edit;
            }
        }
    Limit (&CellOffset.x, 0, Columns - Locked.x - 1);
    Limit (&CellOffset.y, 0, Rows - Locked.y - 1);
    if (CellOffset.x != CellOffsetOld.x || CellOffset.y != CellOffsetOld.y)
      Invalidate (true);
    else if ((Index1.x != Index1Old.x || Index1.y != Index1Old.y) || (Index0.x != Index0Old.x || Index0.y != Index0Old.y))
      {
        DrawCellsAdd (Index0);
        DrawCellsAdd (Index1);
        DrawCellsAdd (Index0Old);
        DrawCellsAdd (Index1Old);
      }
    return Res;
  }

