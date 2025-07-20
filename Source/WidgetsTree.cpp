///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - TREEVIEW
//
///////////////////////////////////////////////////////////////////////////////


#include "Widgets.hpp"
#include "WidgetsDriver.hpp"
#include "WidgetsTree.hpp"

#define TreeMargin 4

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

_TreeViewNode* _TreeViewNode::NodeAdd (bool Child, char *Text_, void *Data_)
  {
    _TreeViewNode *NewNode; //, *Sibling;
    //
    NewNode = new _TreeViewNode (Text_, Data_);
    if (Child || !Parent)   // No parent => no siblings
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
    // Warn about Data
    if (Data)
      DebugAddS ("Deleting TreeViewNode with Data:", Text);
    // Free dynamic Text
    if (Text)
      free (Text);
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

_TreeViewNode* _TreeViewNode::NodeFind (char *Txt)
  {
    _TreeViewNode *n;
    //
    n = Children;
    while (n)
      {
        if (StrSame (n->Text, Txt))
          return (n);
        n = n->Next;
      }
    return NULL;
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

_TreeView::_TreeView (_Container *Parent, _Rect Rect_, _Action Action_) : _Container (Parent, Rect_, NULL)
  {
    _Point sz;
    //
    Action = Action_;
    Edit = NULL;
    Options = 0x00;
    Nodes = new _TreeViewNode ("(root)", nullptr);
    Selected = Nodes; // nullptr;
    sz = TextMeasure ("Ag");
    NodeHeight = sz.y * 4 / 3;
    Indent = NodeHeight / 2;
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

void _TreeView::NodeEdit (void)
  {
    _Rect Rec;
    _Point Pos;
    //
    if (Selected)
      {
        Pos = ScrollToSelected ();
        Rec = {Pos.x - Scroll.x + Indent, Pos.y - Scroll.y, - ScrollBarWidth, NodeHeight};
        Edit = new _Edit (this, Rec, Selected->Text);
        Form->Focused = Edit;
        Form->EventLock = Edit;
      }
  }

int _TreeView::NodeFind (_TreeViewNode *Target)
  {
    _TreeViewNode *Node;
    int Item;
    //
    Node = Nodes->Children;
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
    Node = Nodes->Children;
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

_Point _TreeView::ScrollToSelected (void)   // adjust Scroll to show Selected Node
  {
    _Point Pos;
    _TreeViewNode *Node;
    _Point ScrollOld;
    //
    Pos = {0, 0};
    Node = Nodes->Children;
    while (true)
      {
        if (Node == nullptr)
          break;
        if (Node == Selected)
          break;
        Pos.y++;
        Node = Node->NodeNext (&Pos.x);
      }
    Pos.x *= Indent;
    Pos.y *= NodeHeight;
    ScrollOld = Scroll;
    if (Pos.y < Scroll.y)
      Scroll.y = Pos.y;
    if (Pos.y + NodeHeight - Scroll.y > Rect.Height - ScrollBarWidth)
      Scroll.y = Pos.y + NodeHeight - Rect.Height + ScrollBarWidth;
    if (Pos.x < Scroll.x)
      Scroll.x = Pos.x;
    if (Pos.x + Indent + 64 - Scroll.x > Rect.Width - ScrollBarWidth)
      Scroll.x = Pos.x + Indent + 64 - Rect.Width + ScrollBarWidth;
    if (Scroll.x != ScrollOld.x || Scroll.y != ScrollOld.y)
      Invalidate (true);
    return Pos;
  }

void _TreeView::DrawCustom (void)
  {
    int x, y;
    int x_;
    _TreeViewNode *Node;
    _Rect Rec;
    int cText, cFocus;
    int h;
    _Point ts;
    int MaxX;
    int Col;
    //
    MaxX = 0;
    cText = ColourTextFind ();
    cFocus = ColourAdjust (cText, 140);
    x = 0;
    y = 0;
    Col = Colour;
    Node = Nodes->Children;
    SizeVirtual.x = Rect.Width;
    while (true)
      {
        if (Node == nullptr)
          break;
        if (y - Scroll.y + NodeHeight >= 0 && y - Scroll.y < Rect.Height)   // on screen
          {
            ts = TextMeasure (Node->Text);
            x_ = x * Indent;
            Rec = {x_ + Indent, y, ts.x + 2 * TreeMargin, NodeHeight};
            MaxX = Max (MaxX, x_ + Indent + TreeMargin + ts.x);
            if (Node == Selected)
              if (Form->Focused == this)
                DrawRectangle (Rec, cFocus, cFocus, -1);
              else
                {
                  DrawRectangle (Rec, -1, -1, cSelected);
                  Colour = cSelected;
                }
            TextOutAligned (AddMargin (Rec, {TreeMargin, 0}), Node->Text, aLeft, aCenter);
            Colour = Col;
            h = NodeHeight / 3;
            if (Node->Children)
              DrawArrow ({x_, y + h, h, h}, Node->Expanded ? dDown : dRight);
          }
        y += NodeHeight;
        Node = Node->NodeNext (&x);
      }
    if (MaxX > SizeVirtual.x)
      SizeVirtual.x = MaxX;
    SizeVirtual.y = y;
  }

bool _TreeView::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    int Move;
    static int StationaryClicks = 0;
    _TreeViewNode *SelectedOld;
    int Item;
    //
    if (Edit)
      {
        if (Event->Type == etKeyDown && (Event->Key == KeyEnter || Event->Key == esc))
          {
            if (ActionNodeEditWrite)
              ActionNodeEditWrite (this, Edit->TextGet (), Event->Key == esc);
            delete Edit;
            Edit = NULL;
            Form->Focused = this;
            Invalidate (true);
          }
        return true;
      }
    FocusCheck (Event, Offset);
    Res = false;
    if (NodeHeight == 0)
      return false;
    SelectedOld = Selected;
    if (IsEventMine (Event, Offset))
      if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)
        {
          Res = true;
          StationaryClicks++;
          Offset.y -= Scroll.y;
          Selected = FindItem ((Event->Y - Offset.y) / NodeHeight);
          if ((Event->X - Offset.x) / Indent <= Selected->Depth () || StationaryClicks >= 2)
            Selected->Expanded = !Selected->Expanded;
          Invalidate (true);
        }
      else if (Event->Type == etMouseMove)
        {
          Res = true;
          StationaryClicks = 0;
          Offset.y -= Scroll.y;
          if (Event->MouseKeys & KeyMouseLeft)
            Selected = FindItem ((Event->Y - Offset.y) / NodeHeight);
        }
      else if (Event->Type == etKeyDown)
        {
          Res = true;
          Move = 0;
          switch ((Event->Key & KeyMax))
            {
              case KeyUp:       Move = -1; break;
              case KeyDown:     Move = +1; break;
              case KeyLeft:     if (Selected->Expanded)
                                  Selected->Expanded = false;
                                else if (Selected->Parent != Nodes)
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
              default:          Res = false;
            }
          if (Res)
            {
              Item = NodeFind (Selected);
              Item += Move;
              Selected = FindItem (Max (Item, 0));
              if (!Selected)
                Selected = Nodes;
              ScrollToSelected ();   // Out of sight?
              Invalidate (true);
            }
        }
    if (Selected != SelectedOld)
      {
        Invalidate (true);
        if (Action)
          Action (this);
      }
    return Res;
  }

