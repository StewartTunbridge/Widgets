///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - GRIDVIEW
//
///////////////////////////////////////////////////////////////////////////////


#include "WidgetsEdit.hpp"

#define goGridLines            0x01
#define goSelectMulti          0x02
#define goSelectLine           0x04
#define goAdjustColumn         0x08
#define goColumnTitleHighlight 0x10

class _GridView;

typedef void (*_ActionCellDraw) (_GridView *Grid, int x, int y, _Rect Rect);   // Draw Cell contents in Rect
typedef void (*_ActionCellSelect) (_GridView *Grid, int x, int y);   // Cell selected
typedef char* (*_ActionCellRead) (_GridView *Grid, int x, int y);   // Give me data to edit or NULL. Result not altered, nor freed
typedef void (*_ActionCellWrite) (_GridView *Grid, int x, int y, char *Data, bool Abort);   // Here's your modified data. Abort on esc
typedef void (*_ActionTitleColumnHighlight) (_GridView *Grid, int x);   // Highlighted Column selected

class _GridView: public _Container
  {
    protected:
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);   // returns true when actioned
      _Edit *Edit;   // Used for editing a Cell
      byte ScrollBars;
    public:
      _GridView (_Container *Parent, _Rect Rect_, char *Text_, int Columns_, int Rows_, _ActionCellDraw ActionCellDraw_ = NULL);
      ~_GridView (void);
      int Columns;
      int Rows;
      byte Options;
      //bool *Selected;   // Array of selected lines (goSelectLine only)
      _Point Selected0, Selected1;   // Current Selection
      _Point Locked;   // Scrolling: Index of top left corner of Scrolling region
      _Point CellOffset;   // Scrolling: Index of Cell at 'Locked'
      _Point Title;   // Emboss any Cells above or left of this
      int ColumnTitleHighlight;   // Title: Column to be Lowered instead or Raised
      _Point CellSizeDefault;
      _Point DrawCells0, DrawCells1;
      int *CellWidths;
      int *CellHeights;
      _ActionCellDraw ActionCellDraw;   // Draws the specific cell contents
      _ActionCellSelect ActionCellSelect;   // Cell Selected
      _ActionCellRead ActionCellRead;   // Define to allow Cell Editing OR an action when cell hard selected
      _ActionCellWrite ActionCellWrite;   // After Editing, returns the edited result
      _ActionTitleColumnHighlight ActionTitleColumnHighlight;
      //
      void ScrollToSelected (void);
      int CellWidthGet (int Column);
      void CellWidthSet (int Col, int Width);
      int CellHeightGet (int Row);
      void CellHeightSet (int Col, int Width);
      _Point CellGridPosnToIndex (_Point Position);
      _Point CellIndexToGridPosn (_Point Index);
      _Rect CellRect (_Point Posn);
      _Point CellAtXY (int x, int y, bool *MidColumn = NULL);
      bool CellInSelection (_Point Index);
      void DrawCellsAdd (_Point p);
      void DrawCellsClear (void);
      bool DrawCellsIn (_Point p);
      void EditCellInitiate (void);
    private:
      int ColumnAdjust;
      int ColumnAdjustX;
  };

