///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - GRIDVIEW
//
///////////////////////////////////////////////////////////////////////////////


#include "Widgets.hpp"
#include "WidgetsDriver.hpp"

#include "WidgetsGrid.hpp"

/*int Limit_ (int Value, int Min, int Max)   // not used
  {
    int Value_;
    //
    Value_ = Limit (Value, Min, Max);
    if (Value_ != Value)
      Beep ();
    return Value_;
  }*/

void _GridView::SelectionCheck (void)
  {
    if ( (Selected1.x < Title.x || Selected1.x >= Columns) ||
         (Selected1.y < Title.y || Selected1.y >= Rows) )
      Beep ();
  }

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
    Invalidate (rRedrawPart);
  }

void _GridView::DrawCellsClear (void)
  {
    DrawCells0.x = -1;
  }

bool _GridView::DrawCellsIn (_Point p)
  {
    if (RenderFlags & rRedraw)
      return true;
    if (DrawCells0.x < 0)
      return true;
    if (p.x >= DrawCells0.x && p.x <= DrawCells1.x)
      if (p.y >= DrawCells0.y && p.y <= DrawCells1.y)
        return true;
    return false;
  }

void _GridView::CellSizeDefaultCalc (void)
  {
    CellSizeDefault.y = FontFind ()->YAdvance + 8;
    CellSizeDefault.x = CellSizeDefault.y * 4;
  }

_GridView::_GridView (_Container *Parent, _Rect Rect_, char *Text_, int Columns_, int Rows_, _ActionCellDraw ActionCellDraw_) : _Container (Parent, Rect_, Text_)
  {
    Columns = Columns_;
    Rows = Rows_;
    Options = goGridLines | goSelectMulti;
    TabIn = TabOut = true;
    //Selected = NULL;
    DrawCells0.x = -1;
    ActionCellDraw = ActionCellDraw_;
    ActionCellRead = NULL;
    ActionCellWrite = NULL;
    ActionCellSelect = NULL;
    ActionColumnHighlight = NULL;
    Edit = NULL;
    Locked = {0, 0};
    Title = {0, 1};
    ColumnHighlight = -1;
    Colour = cTextBgnd;
    ColourGrad = -1;
    Selected0 = Selected1 = {1, 1};
    CellOffset = {0, 0};
    CellSizeDefaultCalc ();
    CellWidths = NULL;
    CellHeights = NULL;
    ColumnAdjust = -1;
    if (!Form->Focused)
      Form->Focused = this;
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
    ArraySetInt (&CellWidths, Column, Width);
    Invalidate ();
  }

int _GridView::CellWidthGet (int Column)
  {
    return ArrayGetInt (&CellWidths, Column, CellSizeDefault.x);
  }

void _GridView::CellHeightSet (int Row, int Height)
  {
    ArraySetInt (&CellHeights, Row, Height);
    Invalidate ();
  }

int _GridView::CellHeightGet (int Row)
  {
    return ArrayGetInt (&CellHeights, Row, CellSizeDefault.y);
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
    //if (Selected)
    //  return (ArrayGetBool (&Selected, Index.y, false));
    if (Index.x >= Min (Selected0.x, Selected1.x))
      if (Index.x <= Max (Selected0.x, Selected1.x))
        if (Index.y >= Min (Selected0.y, Selected1.y))
          if (Index.y <= Max (Selected0.y, Selected1.y))
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
      Res.Width = Rect.Width - Res.x - ScrollBarWidth;
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
        Index = CellGridPosnToIndex (Posn);   // Convert to Screen Cells
        x_ += CellWidthGet (Index.x);
        if (MidColumn)
          if (Abs (x_ - x) < 8)   // within 8 pixels of column gap
            {
              mc = true;
              break;
            }
        if (x_ >= x || Posn.x == Columns - 1)   // We have found our column
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
    int Col;
    int c1, c2;
    int x2;
    bool ttl;
    //
    /*char s [80], *ps; //####
    ps = s;
    StrCat (&ps, "DrawCells: ");
    if (DrawCells0.x < 0)
      StrCat (&ps, "ALL");
    else
      {
        PointToStr (&ps, DrawCells0);
        StrCat (&ps, " - ");
        PointToStr (&ps, DrawCells1);
      }
    *ps = 0;
    DebugAdd (s);*/
    //DebugAddHex ("Grid Draw", RenderFlags); // ####
    //
    //int tick = ClockMS (); //####
    CellSizeDefaultCalc ();
    Col = ColourFind ();
    //ColLockBG = ColourAverage (Parent->ColourFind (), Col);
    ColLockBG = ColourAdjust (Col, 90); //ColourAverage (cForm1, Col);
    ColGrid = ColLockBG;  //ColourAdjust (cForm1, 150);
    ColGridFocus = ColourAdjust (ColourTextFind (), 140);
    ColSelBG = cSelected;
    Posn = {0, 0};
    ScrollBars = 0;
    if (CellOffset.x)
      ScrollBars |= sbHorz;
    if (CellOffset.y)
      ScrollBars |= sbVert;
    while (true)
      {
        Ind = CellGridPosnToIndex (Posn);
        RectCell = CellRect (Posn);
        if (Ind.x >= Columns || RectCell.x >= Rect.Width)
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
        ttl = Ind.x < Title.x || Ind.y < Title.y;
        if (DrawCellsIn (Ind) || ttl)
          {
            // Draw cell ornaments
            if (ttl)   // Draw Title Cell Background
              {
                c1 = ColLockBG;
                if (~Options & goSelectLine)
                  if (Ind.x == Selected1.x || Ind.y == Selected1.y)
                    c1 = ColourAdjust (c1, 90);
                DrawRectangle (RectCell, -1, -1, c1);//ColLockBG);
                if (ColumnHighlight == Ind.x)
                  DrawBorder (RectCell, bLowered, ColLockBG, 0);
                else
                  DrawBorder (RectCell, bRaised, ColLockBG, 0);
              }
            else
              {
                //if ((~Options & goSelectLine) && Ind.x == Selected1.x && Ind.y == Selected1.y && Form->KeyFocus == this)   // Current selection
                if (Ind.x == Selected1.x && Ind.y == Selected1.y && Form->Focused == this)
                  DrawRectangle (RectCell, ColGridFocus, ColGridFocus, Col);
                else
                  {
                    if (CellInSelection (Ind))   // Current multiple selection
                      DrawRectangle (RectCell, -1, -1, ColSelBG);
                    else if (~RenderFlags & rRedraw)
                      DrawRectangle (RectCell, -1, -1, Col);
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
            RectCell.x += 1;
            RectCell.Width -= 2;
            if (ActionCellDraw)
              ActionCellDraw (this, Ind.x, Ind.y, RectCell);
          }
        Posn.x++;
      }
    ScrollBarsDraw (ScrollBars, CellOffset, {Columns - Locked.x, Rows - Locked.y});
    DrawCellsClear ();
    //DebugAddInt ("Grid Draw time (mS) ", ClockMS () - tick); //####
  }

void _GridView::EditCellInitiate (void)
  {
    _Point Posn;
    char *Data;
    _Rect Rec;
    //
    if (ActionCellRead)
      {
        Posn = CellIndexToGridPosn (Selected1);
        Data = ActionCellRead (this, Selected1.x, Selected1.y);
        if (Data)
          {
            Rec = CellRect (Posn);
            Edit = new _Edit (this, Rec, Data);
            Edit->Clipboard (coSelectAll);
            Edit->FocusSet ();
            Form->EventLock = Edit;
          }
      }
  }

void _GridView::ScrollToSelected (void)
  {
    _Point CellOffsetOld;
    //
    CellOffsetOld = CellOffset;
    while (Selected1.x - CellOffset.x < Locked.x && CellOffset.x > 0)
      CellOffset.x--;
    while (CellRect (CellIndexToGridPosn ({Selected1.x + 1, Selected1.y})).x > Rect.Width)
      CellOffset.x++;
    while (Selected1.y - CellOffset.y < Locked.y && CellOffset.y > 0)
      CellOffset.y--;
    while (CellRect (CellIndexToGridPosn ({Selected1.x, Selected1.y + 1})).y > Rect.Height)
      CellOffset.y++;
    if (CellOffset.x != CellOffsetOld.x || CellOffset.y != CellOffsetOld.y)
      Invalidate ();
  }

bool _GridView::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    _Point Selected0Old, Selected1Old;
    _Point CellOffsetOld;
    _Point Posn;
    _Point Index;
    int y, w;
    bool MidColumn;
    bool Res;
    static _Point LastClick = {-1, -1};
    bool DoubleClick;
    bool SelAll;
    //int i;
    //
    if (Edit)
      {
        if (Event->Type == etKeyDown && (Event->Key == KeyEnter || Event->Key == esc))
          {
            if (ActionCellWrite)
              ActionCellWrite (this, Selected1.x, Selected1.y, Edit->TextGet (), Event->Key == esc);
            delete Edit;
            Edit = NULL;
            Form->Focused = this;
            DrawCellsAdd (Selected1);
            return true;
          }
        return false;
      }
    //FocusCheck (Event, Offset);
    Res = false;
    Selected1Old = Selected1;
    Selected0Old = Selected0;
    CellOffsetOld = CellOffset;
    DoubleClick = false;
    SelAll = false;
    //if (IsEventMine (Event, Offset))   // Either by location OR Forced
      {
        Res = true;
        Posn = CellAtXY (Rect.Width - 1, Rect.Height - 1);
        if (!ScrollBarsEvent (Event, Offset, ScrollBars, {Columns, Rows}, &CellOffset))
          {
            if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)   // Left Mouse Down
              {
                Posn = CellAtXY (Event->X - Offset.x, Event->Y - Offset.y, &MidColumn);
                Index = CellGridPosnToIndex (Posn);
                if (Index.x >= Title.x && Index.y >= Title.y)
                  DoubleClick = (Event->X == LastClick.x && Event->Y == LastClick.y);
                LastClick = {Event->X, Event->Y};
                if (Index.y < Title.y && Index.x >= Title.x && MidColumn && (Options & goAdjustColumn))   // Adjusting Column Width?
                  {
                    ColumnAdjust = Index.x;   // Adjusting a Column
                    ColumnAdjustX = Event->X;
                  }
                else
                  {
                    ColumnAdjust = -1;
                    if ((Index.x < Title.x) && (Index.y < Title.y))   // top left corner => select all
                      {
                        Selected1 = {0, 0};
                        Selected0 = {Columns, Rows};
                      }
                    else if (Index.x < Title.x)
                      {
                        if (Options & goSelectMulti)   // Select whole rows
                          {
                            Selected0 = {Columns - 1, Index.y};
                            Selected1 = {0, Index.y};
                          }
                      }
                    else if (Index.y < Title.y)   // Top Title Line(s)
                      {
                        if ((Options & (goSelectMulti | goSelectLine)) == goSelectMulti)   // Select whole columns
                          {
                            Selected0 = {Index.x, Rows - 1};
                            Selected1 = {Index.x, 0};
                          }
                        else if (Options & goColumnHighlight)   // change Highlight Column
                          {
                            ColumnHighlight = Index.x;
                            if (ActionColumnHighlight)
                              ActionColumnHighlight (this, ColumnHighlight);
                            Invalidate ();
                          }
                      }
                    else
                      {
                        Selected1 = Index;
                        if (~Event->ShiftState & KeyShift)
                          Selected0 = Selected1;
                      }
                  }
              }
            else if (Event->Type == etMouseMove)
              {
                Posn = CellAtXY (Event->X - Offset.x, Event->Y - Offset.y, &MidColumn);
                Index = CellGridPosnToIndex (Posn);
                if (Event->MouseKeys == Bit [KeyMouseLeft - 1])
                  if (ColumnAdjust >= 0)   // Adjusting a column
                    {
                      w = Max (4, CellWidthGet (ColumnAdjust) + Event->X - ColumnAdjustX);
                      CellWidthSet (ColumnAdjust, w);
                      ColumnAdjustX = Event->X;
                    }
                  else if ((~Options & goSelectLine) || Index.y >= Title.y) // was if (Posn.x >= Title.x && Posn.y >= Title.y)   // Stretching the selection
                    Selected1 = Index;
                LastClick.x = -1;   // not part of a double click
              }
            else if (Event->Type == etMouseUp && Event->Key == KeyMouseLeft)   // Mouse up, stop drag operations
              ColumnAdjust = -1;
            else if (Event->Type == etKeyDown)
              {
                switch ((Event->Key & KeyMax))
                  {
                    case KeyUp:       Selected1.y--;
                                      SelectionCheck ();
                                      break;
                    case KeyDown:     Selected1.y++;
                                      SelectionCheck ();
                                      break;
                    case KeyLeft:     if (~Options & goSelectLine)
                                        Selected1.x--;
                                      SelectionCheck ();
                                      break;
                    case KeyRight:    if (~Options & goSelectLine)
                                      Selected1.x++;
                                      SelectionCheck ();
                                      break;
                    case KeyPageUp:   y = 0;
                                      while (y + 2 * CellHeightGet (Selected1.y) < Rect.Height - CellRect (Locked).y)
                                        {
                                          y += CellHeightGet (Selected1.y);
                                          Selected1.y--;
                                        }
                                      SelectionCheck ();
                                      break;
                    case KeyPageDown: y = 0;
                                      while (y + 2 * CellHeightGet (Selected1.y) < Rect.Height - CellRect (Locked).y)
                                        {
                                          y += CellHeightGet (Selected1.y);
                                          Selected1.y++;
                                        }
                                      SelectionCheck ();
                                      break;
                    case KeyHome:     if ((Event->ShiftState & KeyCntl) || (Options & goSelectLine))
                                        Selected1.y = Title.y;
                                      else
                                        Selected1.x = Title.x;
                                      break;
                    case KeyEnd:      if ((Event->ShiftState & KeyCntl) || (Options & goSelectLine))
                                        Selected1.y = Rows - 1;
                                      else
                                        Selected1.x = Columns - 1;
                                      break;
                    case KeyEnter:
                    case ' ':         DoubleClick = true; break;
                    case Cntrl ('A'): SelAll = true; break;
                    //
                    default:          Res = false;
                  }
                if (Res)
                  {
                    if (SelAll)
                      {
                        Selected0 = Title;
                        Selected1 = {Columns - 1, Rows - 1};
                      }
                    else
                      {
                        if (~Event->ShiftState & KeyShift)
                          Selected0 = Selected1;
                        while (Selected1.x - CellOffset.x < Locked.x && CellOffset.x > 0)
                          CellOffset.x--;
                        while (CellRect (CellIndexToGridPosn ({Selected1.x + 1, Selected1.y})).x > Rect.Width)
                          CellOffset.x++;
                        while (Selected1.y - CellOffset.y < Locked.y && CellOffset.y > 0)
                          CellOffset.y--;
                        while (CellRect (CellIndexToGridPosn ({Selected1.x, Selected1.y + 1})).y > Rect.Height)
                          CellOffset.y++;
                      }
                  }
              }
          }
        Selected1.x = Limit (Selected1.x, Title.x, Columns - 1);
        Selected1.y = Limit (Selected1.y, Title.y, Rows - 1);
        if (~Options & goSelectMulti)
          Selected0 = Selected1;
        if (Options & goSelectLine)
          {
            Selected0.x = Columns - 1;
            Selected1.x = Title.x;
          }
        if (DoubleClick)
          EditCellInitiate ();
        if (Columns <= Title.x)
          Columns = Title.x + 1;
        if (Rows <= Title.y)
          Rows = Title.y + 1;
        CellOffset.x = Limit (CellOffset.x, 0, Columns - Locked.x - 1);
        CellOffset.y = Limit (CellOffset.y, 0, Rows - Locked.y - 1);
        if (Selected1.x != Selected1Old.x || Selected1.y != Selected1Old.y)
          if (ActionCellSelect)
            ActionCellSelect (this, Selected1.x, Selected1.y);
        if (CellOffset.x != CellOffsetOld.x || CellOffset.y != CellOffsetOld.y)   // scrolling whole grid
          Invalidate ();
        else   // redrawing part ofthe grid
          if ((Selected1.x != Selected1Old.x || Selected1.y != Selected1Old.y) || (Selected0.x != Selected0Old.x || Selected0.y != Selected0Old.y))   // Selection has changed
            /*
              {
                DrawCellsAdd (Selected1Old);
                if ((Options & (goSelectMulti | goSelectLine) == (goSelectMulti | goSelectLine)) && (Event->ShiftState & KeyCntl))   // multi select + whole lines AND Cntrl pressed
                  {
                    bool SelNew = !ArrayGetBool (&Selected, Selected0.y, false);   // so toggle the selected items
                    for (i = Min (Selected0.y, Selected1.y); i <= Max (Selected0.y, Selected1.y); i++)
                      {
                        ArraySetBool (&Selected, i, SelNew);
                        DrawCellsAdd ({0, i});
                        DrawCellsAdd ({Columns - 1, i});
                      }
                  }
                else   // no Cntrl pressed
                  if (Selected)   // using individual selected lines
                    {
                      for (i = 0; i < Rows; i++)
                        if (ArrayGetBool (&Selected, i, false) != ValueWithin (i, Selected0.y, Selected1.y))
                          {
                            ArraySetBool (&Selected, i, ValueWithin (i, Selected0.y, Selected1.y));
                            DrawCellsAdd ({0, i});
                            DrawCellsAdd ({Columns - 1, i});
                          }
                    }
                  else*/
                      {
                        DrawCellsAdd (Selected0);
                        DrawCellsAdd (Selected1);
                        DrawCellsAdd (Selected0Old);
                        DrawCellsAdd (Selected1Old);
                      }
              //}
      }
    return Res;
  }
