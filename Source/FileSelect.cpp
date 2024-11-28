////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SELECT FILE
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "../Widgets/Widgets.hpp"
#include "../Widgets/WidgetsGrid.hpp"


class _FormFileSelect: public _Form
  {
    public:
      _FormFileSelect (char *Title, _Rect Position, char *Filter, bool AllowNewFile);
      ~_FormFileSelect (void);
      _Label *lPath;
      _Container *cToolbar;
      _Edit *eNewFilename;
      _Button *bSave;
      _GridView *Grid;
      void DirFill (void);
      //
      bool AllowNewFile_;
      _DirItem *Dir;
      int Index = -1;
      _DirItem *di;
      char *Result;
  };

_FormFileSelect *fFileSelect = NULL;
char *FileSelectFilter;

char* ColumnTitle [] = {"Size", "Filename"};
_Align ColumnAlign [] = {aRight, aLeft};

void FilesCellDraw (_GridView *Grid, int x, int y, _Rect Rect)
  {
    char St [32], *s;
    _Font *f;
    //
    Rect.x += 4;
    Rect.Width -= 8;
    fFileSelect->Grid->ColourText = -1;
    f = Grid->FontFind ();
    if (!f)
      return;
    if (y == 0)
      {
        f->Style = fsBold;
        Grid->TextOutWrap (Rect, ColumnTitle [x], ColumnAlign [x], aCenter);
      }
    else
      {
        f->Style = 0;
        if (ListGoto (fFileSelect->Dir, y - 1, &fFileSelect->Index, &fFileSelect->di))
          {
            if (fFileSelect->di->Directory)
              fFileSelect->Grid->ColourText = ColourAdjust (cBlue, 120);
            if (x == 0)
              {
                s = St;
                NumToStr (&s, fFileSelect->di->Size);
                *s = 0;
                Grid->TextOutWrap (Rect, St, aRight, aCenter);
              }
            else
              Grid->TextOutWrap (Rect, fFileSelect->di->Name, aLeft, aCenter);
          }
      }
  }

char* FilesRead (_GridView *Grid, int x, int y)
  {
    char *p;
    //
    if (ListGoto (fFileSelect->Dir, y - 1, &fFileSelect->Index, &fFileSelect->di))
      if (fFileSelect->di->Directory)
        {
          chdir (fFileSelect->di->Name);
          fFileSelect->DirFill ();
        }
      else
        if (fFileSelect->AllowNewFile_)
          fFileSelect->eNewFilename->TextSet (fFileSelect->di->Name);
        else
          {
            fFileSelect->Result = (char *) malloc (MaxPath); //(StrLen (fFileSelect->lPath->TextGet ()) + StrLen (fFileSelect->DirResult->Name) + 2);
            p = fFileSelect->Result;
            StrCat (&p, fFileSelect->lPath->TextGet ());
            StrCat (&p, PathDelimiter);
            StrCat (&p, fFileSelect->di->Name);
            *p = 0;
          }
    return NULL;
  }

void ActionFileSelectSave (_Container *Container)
  {
    _Button *b;
    char *p;
    //
    b = (_Button *) Container;
    if (!b->Down)
      if (StrLen (fFileSelect->eNewFilename->TextGet ()) > 0)
        {
          fFileSelect->Result = (char *) malloc (MaxPath);
          p = fFileSelect->Result;
          StrCat (&p, fFileSelect->lPath->TextGet ());
          StrCat (&p, PathDelimiter);
          StrCat (&p, fFileSelect->eNewFilename->TextGet ());
          *p = 0;
        }
  }

_FormFileSelect::_FormFileSelect (char *Title, _Rect Position, char *Filter, bool AllowNewFile)
               : _Form (Title, Position, waAlwaysOnTop | waResizable)
  {
    int y;
    //
    Result = NULL;
    Dir = NULL;
    FileSelectFilter = Filter;
    AllowNewFile_ = AllowNewFile;
    Container->FontSet ("ARI.ttf", 14);
    y = 0;
    lPath = new _Label (Container, {4, y, 0, 24}, NULL); y += lPath->Rect.Height;
    if (AllowNewFile)
      {
        cToolbar = new _Container (Container, {0, y, 0, 32}); y += cToolbar->Rect.Height;
        eNewFilename = new _Edit (cToolbar, {4, 0, cToolbar->Rect.Width - 64 - 4, 0}, NULL);
        bSave = new _Button (cToolbar, {cToolbar->Rect.Width - 64, 0, 0, 0}, "Save", ActionFileSelectSave);
        bSave->RectLock = rlRight;
      }
    Grid = new _GridView (Container, {0, y, 0, 0}, NULL, 2, 2, FilesCellDraw);
    Grid->Locked = {0, 1};
    Grid->Title = {0, 1};
    Grid->Options = (Grid->Options | goSelectLine | goAdjustColumn) & ~goSelectMulti;
    Grid->ActionCellRead = FilesRead;
    DirFill ();
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
    Path = NULL;
    if (GetCurrentPath (&Path))
      {
        lPath->TextSet (Path);
        free (Path);
        DirFree (Dir);
        Index = -1;
        Dir = NULL;
        DirRead (&Dir, ReadDirCallBack);
        Dir = ListSort (Dir, DirItemsCompare);
        Grid->Rows = ListLength (Dir) + 1;
        Grid->CellOffset = {0, 0};
      }
    // breaks things Grid->Index0 = Grid->Index1 = Grid->Locked;
    Grid->Invalidate (true);
  }

_FormFileSelect::~_FormFileSelect (void)
  {
    DirFree (Dir);
    free (Result);
    fFileSelect = NULL;
  }

bool FileSelect (char **Name, char *Filter, bool AllowNewFile)   // caller must free Name
  {
    bool Res;
    //
    Res = false;
    *Name = NULL;
    if (!fFileSelect)
      {
        fFileSelect = new _FormFileSelect ("Select File", {50, 50, 600, 400}, Filter, AllowNewFile);
        while (true)
          {
            if (FormsUpdate ())
              break;
            if (fFileSelect == NULL)
              break;
            if (fFileSelect->Result)
              {
                StrForceExtension (fFileSelect->Result, Filter);
                StrAssignCopy (Name, fFileSelect->Result);
                Res = true;
                break;
              }
            usleep (1000);
          }
        if (fFileSelect)
          delete fFileSelect;
      }
    return Res;
  }
