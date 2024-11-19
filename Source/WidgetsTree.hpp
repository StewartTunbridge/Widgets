///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - TREEVIEW
//
///////////////////////////////////////////////////////////////////////////////

/*
#define toGridLines    0x01
#define toSelectMulti  0x02
#define toSelectLine   0x04
#define toAdjustColumn 0x08
*/

class _TreeViewNode
  {
    //private:
    public:
      _TreeViewNode *Next, *Prev;
      _TreeViewNode *Parent;
      _TreeViewNode *Children;
      char *Text;
      void *Data;
      bool Expanded;
      //
      _TreeViewNode (char *Text, void *Data = NULL);
      ~_TreeViewNode ();
      //
      _TreeViewNode *AddNode (bool Child, char *Text, void *Data = NULL);
      _TreeViewNode* NodeNext (int *x);
      int Depth (void);
  };

class _TreeView;

//typedef void (*_ActionCellDraw) (_Grid *Grid, int x, int y, _Rect Rect);
//typedef char* (*_ActionCellRead) (_Grid *Grid, int x, int y);   // Result will be freed by Grid
//typedef void (*_ActionCellWrite) (_Grid *Grid, int x, int y, char *Data);

class _TreeView: public _Container
  {
    protected:
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);   // returns true when actioned
    public:
      byte Options;
      int NodeHeight;
      _TreeViewNode *Nodes;
      _TreeViewNode *Selected;
      //int Top;
      _Action Action;
      _TreeView (_Container *Parent, _Rect Rect_);
      ~_TreeView ();
      void SetSize (void);
      void Check (void);
      int FindNode (_TreeViewNode *Target);
      _TreeViewNode* FindItem (int Target);
      int NodesDisplayable (void);
      //int DrawFrom;
      //_ActionCellDraw ActionCellDraw;   // Draws the specific cell contents
      //_ActionCellRead ActionCellRead;   // Define to allow Cell Editing
      //_ActionCellWrite ActionCellWrite;   // After Editing, returns the edited result
  };

