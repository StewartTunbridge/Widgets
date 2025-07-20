////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SELECT FILE
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "Widgets.hpp"
#include "WidgetsGrid.hpp"
#include "FileSelect.hpp"


bool FileSelectActive = false;

class _FormFileSelect: public _Form
  {
    public:
      _FormFileSelect (char *Title, _Rect Position, bool SaveFile, _ActionResultChars ActionFileSelected_);
      ~_FormFileSelect (void);
      int ColDir;
      //_ButtonArrow *bUp;
      _Label *lPath;
      _Container *cToolbar;
      _Edit *eNewFilename;
      _Button *bSave;
      _GridView *Grid;
      _ActionResultChars ActionFileSelected;
      void DirFill (void);
      void ActionSelection (void);
      //
      bool SaveFile;
      _DirItem *Dir;
      int CurrentIndex = -1;
      _DirItem *CurrentItem;
  };

_FormFileSelect *fFileSelect = NULL;
char *FileSelectFilter;

char* ColumnTitle [] = {"Size", "Filename"};
_Align ColumnAlign [] = {aRight, aLeft};

void _FormFileSelect::ActionSelection (void)
  {
    char *Res, *r;
    //
    Res = (char *) malloc (MaxPath); //(StrLen (fFileSelect->lPath->TextGet ()) + StrLen (fFileSelect->DirResult->Name) + 2);
    r = Res;
    StrCat (&r, lPath->TextGet ());
    StrCat (&r, PathDelimiter);
    if (SaveFile)
      StrCat (&r, eNewFilename->TextGet ());
    else
      StrCat (&r, CurrentItem->Name);
    *r = 0;
    if (FileSelectFilter)
      if (FileSelectFilter [0] == '.')
        StrAddExtension (Res, FileSelectFilter);
    if (ActionFileSelected)
      ActionFileSelected (Res, Container->DataContainer);
    free (Res);
    Die ();
  }

void FilesCellDraw (_GridView *Grid, int x, int y, _Rect Rect)
  {
    _FormFileSelect *Form;
    char St [32], *s;
    _Font *f;
    byte Style;
    //
    Form = (_FormFileSelect *) Grid->Form;
    Rect.x += 4;
    Rect.Width -= 8;
    Form->Grid->ColourText = -1;
    f = Grid->FontFind ();
    if (!f)
      return;
    Style = f->Style;
    if (y == 0)
      {
        f->Style = fsBold;
        Grid->TextOutWrap (Rect, ColumnTitle [x], ColumnAlign [x], aCenter);
      }
    else
      {
        if (ListGoto (Form->Dir, y - 1, &Form->CurrentIndex, &Form->CurrentItem))
          {
            if (Form->CurrentItem->Directory)
              Form->Grid->ColourText = ColourAdjust (Form->ColDir, 120);
            if (x == 0)
              {
                s = St;
                NumToStr (&s, Form->CurrentItem->Size);
                *s = 0;
                Grid->TextOutWrap (Rect, St, aRight, aCenter);
              }
            else
              Grid->TextOutWrap (Rect, Form->CurrentItem->Name, aLeft, aCenter);
          }
      }
    f->Style = Style;
  }

char* CellRead (_GridView *Grid, int x, int y)
  {
    _FormFileSelect *Form;
    //
    Form = (_FormFileSelect *) Grid->Form;
    if (ListGoto (Form->Dir, y - 1, &Form->CurrentIndex, &Form->CurrentItem))
      if (Form->CurrentItem->Directory)
        {
          chdir (Form->CurrentItem->Name);
          Form->DirFill ();
        }
      else
        if (Form->SaveFile)
          {
            Form->eNewFilename->TextSet (Form->CurrentItem->Name);
            Form->eNewFilename->FocusSet ();
          }
        else
          Form->ActionSelection ();
    return NULL;
  }

/*void ActionFileSelectUp (_Container *Button)
  {
    _FormFileSelect *Form;
    //
    Form = (_FormFileSelect *) Button->Form;
    chdir ("..");
    Form->DirFill ();
  }*/

void ActionFileSelectSave (_Container *Container)
  {
    if (StrLen (fFileSelect->eNewFilename->TextGet ()) > 0)
      fFileSelect->ActionSelection ();
  }

bool FormFileSelectPreview (_Form *Form, _Event *Event)
  {
    if (Form->Focused == fFileSelect->eNewFilename)
      if (Event->Type == etKeyDown)
        if (Event->Key == KeyEnter)
          ActionFileSelectSave (NULL);
        else if (Event->Key == esc)
          Form->Die ();
    return false;
  }

_FormFileSelect::_FormFileSelect (char *Title, _Rect Position, bool SaveFile_, _ActionResultChars ActionFileSelected_)
               : _Form (Title, Position, waAlwaysOnTop | waResizable)
  {
    const int lh = 24;
    //const int aw = 18;
    const int th = 32;
    int x, y;
    //
    ActionFileSelected = ActionFileSelected_;
    Container->FontSet (NULL, 16);
    Dir = NULL;
    SaveFile = SaveFile_;
    ColDir = cBlue;
    Container->FontSet (NULL, 14);
    x = 0;
    y = 0;
    //bUp = new _ButtonArrow (Container, {x, y, aw, lh}, NULL, ActionFileSelectUp, dUp); x += aw;
    lPath = new _Label (Container, {x, y, 0, lh}, NULL);
    lPath->Colour = ColourAdjust (ColDir, 180);
    lPath->Margin = 4;
    y += lh;
    x = 0;
    cToolbar = NULL;
    eNewFilename = NULL;
    bSave = NULL;
    if (SaveFile)
      {
        cToolbar = new _Container (Container, {0, y, 0, th}); y += th;
        eNewFilename = new _Edit (cToolbar, {x, 0, -64, 0}, NULL);
        bSave = new _Button (cToolbar, {-64, 0, 0, 0}, "Save", ActionFileSelectSave);
        //bSave->RectLock |= rlRight;
      }
    Grid = new _GridView (Container, {0, y, 0, 0}, NULL, 2, 2, FilesCellDraw);
    //Grid->Locked = {0, 1};
    Grid->Title = {0, 1};
    //Grid->Selected0 = Grid->Title;
    Grid->Options = (Grid->Options | goSelectLine | goAdjustColumn) & ~goSelectMulti;
    Grid->ActionCellRead = CellRead;
    DirFill ();
    FileSelectActive = true;
    EventPreview = FormFileSelectPreview;
  }

int DirItemsCompare (_DirItem *Data1, _DirItem *Data2)
  {
    _DirItem *i1, *i2;
    int delta;
    //
    i1 = (_DirItem *) Data1;
    i2 = (_DirItem *) Data2;
    delta = i2->Directory - i1->Directory;
    if (delta == 0)
      delta = StrCmp (i1->Name, i2->Name);
    return delta;
  }

bool ReadDirCallBack (_DirItem *Item)
  {
    if (Item->Directory)
      return true;
    if (FileSelectFilter == NULL)
      return true;
    if (StrPos (Item->Name, FileSelectFilter))
      return true;
    return false;
  }

void _FormFileSelect::DirFill (void)
  {
    char *Path;
    //
    if (GetCurrentPath (&Path))
      {
        lPath->TextSet (Path);
        free (Path);
        DirFree (Dir);
        CurrentIndex = -1;
        Dir = NULL;
        DirRead (NULL, &Dir, ReadDirCallBack);
        Dir = ListSort (Dir, DirItemsCompare);
        Grid->Rows = Max (1, ListLength (Dir) + 1);
        Grid->CellOffset = {0, 0};
      }
    // breaks things Grid->Index0 = Grid->Index1 = Grid->Locked;
    Grid->Invalidate (true);
  }

_FormFileSelect::~_FormFileSelect (void)
  {
    DirFree (Dir);
    fFileSelect = NULL;
    FileSelectActive = false;
  }

void FileSelect (char *Title, char *Filter, bool SaveFile, _ActionResultChars ActionFileSelected, void *Parameter)   // caller must free Name
  {
    _Point p;
    //
    if (!fFileSelect)
      {
        FileSelectFilter = Filter;
        p = MousePos ();
        fFileSelect = new _FormFileSelect (Title, {p.x, p.y, 300, 400}, SaveFile, ActionFileSelected);
        fFileSelect->Container->DataContainer = (_Container *) Parameter;
      }
  }
