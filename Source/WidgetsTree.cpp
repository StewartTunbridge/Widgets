///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - TREEVIEW
//
///////////////////////////////////////////////////////////////////////////////


#include "Widgets.hpp"
#include "WidgetsDriver.hpp"
#include "WidgetsTree.hpp"

extern void DebugAddError (const char *Tag);

_TreeViewNode::_TreeViewNode (char *Text_, void *Data_)
  {
    Next = nullptr;
    Prev = nullptr;
    Parent = nullptr;
    Children = nullptr;
    Text = nullptr;
    StrAssignCopy (&Text, Text_);
    Data = Data_;
    Expanded = false;
  }

_TreeViewNode* _TreeViewNode::AddNode (bool Child, char *Text_, void *Data_)
  {
    _TreeViewNode *NewNode; //, *Sibling;
    //
    NewNode = new _TreeViewNode (Text_, Data_);
    if (Child | !Parent)   // No parent => no siblings
      {
        NewNode->Parent = this;
        ListAdd (NewNode, &Children);
      }
    else
      {
        NewNode->Parent = Parent;
        ListAdd (NewNode, &(Parent->Children));
      }
    return NewNode;
  }

_TreeViewNode::~_TreeViewNode ()
  {
    // Destroy children
    while (Children)
      delete Children;
    // Unlink from family
    if (Parent)
      ListRemove (this, &(Parent->Children));
    else
      ListRemove (this, (_TreeViewNode **) nullptr);
    // Free dynamic variable
    if (Text)
      free (Text);
    if (Data)
      free (Data);
  }

_TreeViewNode* _TreeViewNode::NodeNext (int *x)
  {
    _TreeViewNode *n;
    //
    if (Expanded && Children)
      {
        if (x)
          (*x)++;
        return Children;
      }
    if (Next)
      return Next;
    n = this;
    while (true)
      {
        if (!n->Parent)
          return nullptr;
        n = n->Parent;
        if (x)
          (*x)--;
        if (n->Next)
          return n->Next;
      }
  }

int _TreeViewNode::Depth (void)
  {
    int Res;
    _TreeViewNode *n;
    //
    Res = 0;
    n = this;
    while (n->Parent)
      {
        n = n->Parent;
        Res++;
      }
    return Res;
  }

class _TreeView;

_TreeView::_TreeView (_Container *Parent, _Rect Rect_) : _Container (Parent, Rect_, NULL)
  {
    _Point sz;
    //
    Nodes = new _TreeViewNode ("root", nullptr);
    Selected = Nodes; // nullptr;
    //Top = 0;
    sz = TextMeasure (Font, "Ag");
    NodeHeight = sz.y * 3 / 2;
  }

_TreeView::~_TreeView (void)
  {
    delete Nodes;
  }

void _TreeView::Check (void)
  {
    _TreeViewNode *Node;
    int x;
    //
    Node = Nodes;
    while (Node)
      {
        if (Node->Prev)
          if (Node->Prev->Next != Node)
            DebugAddError ("Prev/Next");
        if (Node->Next)
          if (Node->Next->Prev != Node)
            DebugAddError ("Next/Prev");
        x = 0;
        Node->Expanded = true;
        Node = Node->NodeNext (&x);
        if (x == 1)   // First child
          if (Node->Parent->Children != Node)
            DebugAddError ("Parent/Child");
      }
  }

int _TreeView::FindNode (_TreeViewNode *Target)
  {
    _TreeViewNode *Node;
    int Item;
    //
    Node = Nodes;
    Item = 0;
    while (true)
      {
        if (Node == nullptr)
          break;
        if (Node == Target)
          return Item;
        Item++;
        Node = Node->NodeNext (nullptr);
      }
    return -1;
  }

_TreeViewNode* _TreeView::FindItem (int Target)
  {
    _TreeViewNode *Node, *Node_;
    int Item;
    //
    Node = Nodes;
    Item = 0;
    while (true)
      {
        if (Node == nullptr)
          break;
        Node_ = Node->NodeNext (nullptr);
        if (Item == Target || Node_ == nullptr)
          return Node;
        Item++;
        Node = Node_;
      }
    return nullptr;
  }

int _TreeView::NodesDisplayable (void)
  {
    _TreeViewNode *Node;
    int n;
    //
    Node = Nodes;
    n = 0;
    while (Node)
      {
        Node = Node->NodeNext (nullptr);
        n++;
      }
    return n;
  }

#define TreeMargin 4

void _TreeView::SetSize ()
  {
    SizeVirtual = {Rect.Width * 2, NodeHeight * NodesDisplayable ()};
  }

void _TreeView::DrawCustom (void)
  {
    int x, y;
    int x_;
    _TreeViewNode *Node;
    _Rect Rec1, Rec2;
    _Font *Font;
    int cText, cFocus;
    int h;
    //
    Font = FontFind ();
    cText = ColourTextFind ();
    cFocus = ColourAdjust (cText, 140);
    x = 0;
    y = 0;
    Node = Nodes;
    SetSize ();
    while (true)
      {
        if (Node == nullptr)
          break;
        if (y - Scroll.y > Rect.Height)
          break;
        if (y - Scroll.y + NodeHeight >= 0)
          {
            x_ = x * NodeHeight;
            Rec1 = {x_ + NodeHeight, y, Rect.Width, NodeHeight};
            Rec2.x = x_ + NodeHeight + TreeMargin;
            Rec2.y = y + NodeHeight / 6;
            Rec2.Width = Rect.Width - Rec2.x;
            Rec2.Height = NodeHeight;
            if (Node == Selected)
              if (Form->KeyFocus == this)
                DrawRectangle (Rec1, cFocus, cFocus, -1);
              else
                {
                  DrawRectangle (Rec1, -1, -1, cSelected);
                  Font->ColourBG = cSelected;
                }
            TextOutAligned (NULL, Rec2, Node->Text, aLeft, aLeft);
            Font->ColourBG = -1;
            h = NodeHeight / 3;
            if (Node->Children)
              DrawArrow ({x_ + h, y + h, h, h}, Node->Expanded ? dDown : dRight);
          }
        y += NodeHeight;
        Node = Node->NodeNext (&x);
      }
  }

bool _TreeView::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    int Move;
    static int StationaryClicks = 0;
    _TreeViewNode *SelectedOld;
    int Item;
    //
    CheckFocus (Event, Offset);
    Res = false;
    SelectedOld = Selected;
    if (IsEventMine (Event, Offset))
      {
        Res = true;
        if (Event->Type == etMouseDown)
          {
            StationaryClicks++;
            Offset.y -= Scroll.y;
            Res = true;
            if (Event->Key == KeyMouseLeft)
              {
                Selected = FindItem ((Event->Y - Offset.y) / NodeHeight);
                if ((Event->X - Offset.x) / NodeHeight <= Selected->Depth () || StationaryClicks >= 2)
                  Selected->Expanded = !Selected->Expanded;
                SetSize ();
                Invalidate (true);
              }
          }
        else if (Event->Type == etMouseMove)
          StationaryClicks = 0;
        else if (Event->Type == etKeyDown)
          {
            Move = 0;
            switch ((Event->Key & KeyMax))
              {
                case KeyUp:       Move = -1; break;
                case KeyDown:     Move = +1; break;
                case KeyLeft:     if (Selected->Expanded)
                                    Selected->Expanded = false;
                                  else
                                    Selected = Selected->Parent;
                                  break;
                case KeyRight:    Selected->Expanded = true; break;
                case KeyPageUp:   Move = - Rect.Height / NodeHeight; break;
                case KeyPageDown: Move = Rect.Height / NodeHeight; break;
                case KeyHome:     Selected = Nodes; break;
                case KeyEnd:      Move = IntMax/2; break;
                case KeyEnter:
                case ' ':         Selected->Expanded = !Selected->Expanded; //DoubleClick = true;
                                  break;
              }
            Item = FindNode (Selected);
            Item += Move;
            Selected = FindItem (Max (Item, 0));
            if (!Selected)
              Selected = Nodes;
            Item = FindNode (Selected);
            // Out of sight?
            if (Item * NodeHeight < Scroll.y)
              Scroll.y = Item * NodeHeight;
            if ((Item + 1) * NodeHeight - Scroll.y > Rect.Height - ScrollBarWidth)
              Scroll.y = (Item + 1) * NodeHeight - Rect.Height + ScrollBarWidth;
            SetSize ();
            Invalidate (true);
          }
      }
    if (Selected != SelectedOld)
      if (Action)
        Action (this);
    return Res;
  }

