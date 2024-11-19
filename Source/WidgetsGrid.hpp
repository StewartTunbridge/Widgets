///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - GRIDVIEW
//
///////////////////////////////////////////////////////////////////////////////


#include "WidgetsEdit.hpp"

#define goGridLines    0x01
#define goSelectMulti  0x02
#define goSelectLine   0x04
#define goAdjustColumn 0x08

class _GridView;

typedef void (*_ActionCellDraw) (_GridView *Grid, int x, int y, _Rect Rect);
typedef char* (*_ActionCellRead) (_GridView *Grid, int x, int y);   // Result will be freed by Grid
typedef void (*_ActionCellWrite) (_GridView *Grid, int x, int y, char *Data);

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
      _Point Index0, Index1;   // Current Selection
      _Point Locked;   // Scrolling: Index of top left corner of Scrolling region
      _Point CellOffset;   // Scrolling: Index of Cell at 'Locked'
      _Point Title;   // Emboss any Cells above or left of this
      _Point CellSizeDefault;
      _Point DrawCells0, DrawCells1;
      int *CellWidths;
      int *CellHeights;
      _ActionCellDraw ActionCellDraw;   // Draws the specific cell contents
      _ActionCellRead ActionCellRead;   // Define to allow Cell Editing
      _ActionCellWrite ActionCellWrite;   // After Editing, returns the edited result
      //
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
    private:
      int ColumnAdjust;
      int ColumnAdjustX;
  };

