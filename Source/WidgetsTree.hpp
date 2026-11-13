///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - TREEVIEW
//
///////////////////////////////////////////////////////////////////////////////


#include "WidgetsEdit.hpp"

class _TreeViewNode
  {
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
      _TreeViewNode *NodeAdd (bool Child, char *Text, void *Data = NULL);
      _TreeViewNode *NodeNext (int *x, bool Displayed);
      _TreeViewNode *NodeNext (int *x);
      _TreeViewNode *NodeFind (char *Txt);
      bool NodeMoveUp (void);
      bool NodeMoveDown (void);
      bool NodeMoveLeft (void);
      bool NodeMoveRight (void);
      int Depth (void);
  };

class _TreeView;

typedef void (*_ActionNodeEditWrite) (_TreeView *TreeView, char *Text, bool Abort);   // Here's your modified data. Abort on esc

class _TreeView: public _Container
  {
    protected:
      _Edit *Edit;   // Used for editing a Cell
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);   // returns true when actioned
    public:
      byte Options;
      int NodeHeight;
      int Indent;
      _TreeViewNode *Nodes;
      _TreeViewNode *Selected;
      _Action Action;
      _ActionNodeEditWrite ActionNodeEditWrite;
      _TreeView (_Container *Parent, _Rect Rect_, _Action Action_ = NULL);
     ~_TreeView ();
      void SetSize (int Width, int Height);
      _Point ScrollToSelected (void);   // adjust Scroll to show Selected Node
      void Check (void);
      int NodeFind (_TreeViewNode *Target);
      void NodeEdit (void);   // Create an edit box for Node->Name. Result via _ActionNodeEditWrite
      _TreeViewNode* FindItem (int Target);
  };
