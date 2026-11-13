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
        ListAddEnd (NewNode, &Children);
      }
    else
      {
        NewNode->Parent = Parent;
        ListAddEnd (NewNode, &(Parent->Children));
      }
    return NewNode;
  }

bool _TreeViewNode::NodeMoveUp (void)
  {
    if (Prev)
      return Prev->NodeMoveDown ();
    return false;
  }

bool _TreeViewNode::NodeMoveDown (void)
  {
    _TreeViewNode *nxt;
    //
    if (Next && Parent)
      {
        nxt = Next;
        ListRemove (this, &(Parent->Children));
        ListAdd (this, nxt);
        return true;
      }
    return false;
  }

bool _TreeViewNode::NodeMoveLeft (void)
  {
    if (Parent && Parent->Parent)
      {
        ListRemove (this, &(Parent->Children));
        ListAdd (this, Parent);
        Parent = Parent->Parent;
        return true;
      }
    return false;
  }

bool _TreeViewNode::NodeMoveRight (void)
  {
    _TreeViewNode *sib;
    //
    sib = NULL;   // Find sibling to descend from
    if (Prev)
      sib = Prev;
    else if (Next)
      sib = Next;
    if (sib && Parent)
      {
        ListRemove (this, &(Parent->Children));
        ListAddEnd (this, &(sib->Children));
        Parent = sib;
        sib->Expanded = true;
        return true;
      }
    return false;
  }

_TreeViewNode::~_TreeViewNode ()
  {
    // Destroy children
    while (Children)
      delete Children;
    // Unlink from family
    if (Parent)
      ListRemove (this, &(Parent->Children));
    // Free Data
    if (Data)
      {
        free (Data);   // Is this dangerous
        DebugAddS ("Deleting TreeViewNode with Data:", Text);
      }
    // Free dynamic Text
    if (Text)
      free (Text);
  }

_TreeViewNode* _TreeViewNode::NodeNext (int *x, bool Displayed)
  {
    _TreeViewNode *n;
    //
    if ((Expanded | !Displayed) && Children)
      {
        if (x)
          (*x)++;
        return Children;
      }
    // Next in line?
    if (Next)
      return Next;
    // Find a parent with a sibling
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

_TreeViewNode* _TreeViewNode::NodeNext (int *x)
  {
    return NodeNext (x, true);
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
    Action = Action_;
    Edit = NULL;
    Options = 0x00;
    TabIn = true;
    Nodes = new _TreeViewNode ("(root)", nullptr);
    Selected = Nodes;
    if (!Form->Focused)
      Form->Focused = this;
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
        Edit->Clipboard (coSelectAll);
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
      Invalidate ();
    return Pos;
  }

void _TreeView::DrawCustom (void)
  {
    _Font *Font_;
    int x;
    int x_, y_;
    _TreeViewNode *Node;
    _Rect Rec;
    int cText, cFocus, cLine;
    int dx, dy;
    _Point ts;
    int MaxX;
    int YByDepth [256];
    //
    MaxX = 0;
    cText = ColourTextFind ();
    cFocus = ColourAdjust (cText, 140);
    cLine = ColourAdjust (cText, 170);
    Font_ = FontFind ();
    if (Font_)
      NodeHeight = Font_->YAdvance * 4 / 3;
    Indent = NodeHeight * 3 / 5;
    x = 0;
    y_ = 0;
    Node = Nodes->Children;
    SizeVirtual.x = Rect.Width;
    while (true)
      {
        YByDepth [x] = y_;
        if (Node == nullptr)   // End of tree
          break;
        if (y_ - Scroll.y + NodeHeight >= 0 && y_ - Scroll.y < Rect.Height)   // on screen
          {
            ts = TextMeasure (Node->Text);
            x_ = x * Indent;
            Rec = {x_ + Indent, y_, ts.x + 2 * TreeMargin, NodeHeight};
            MaxX = Max (MaxX, x_ + Indent + TreeMargin + ts.x);
            if (Node == Selected)   // selected node
              if (Form->Focused == this)
                DrawRectangle (Rec, cFocus, cFocus, -1);   // selected focussed
              else
                DrawRectangle (Rec, -1, -1, cSelected);   // selected not focussed
            TextOutAligned (AddMargin (Rec, {TreeMargin, 0}), Node->Text, aLeft, aCenter);
            // Add line to parent
            dx = NodeHeight / 6;
            dy = NodeHeight / 2;
            if (x >= 1)
              {
                DrawLine (x_ + dx, y_ + dy, x_ - Indent + dx, y_ + dy, cLine);   // horizontal line
                DrawLine (x_ - Indent + dx, y_ + dy, x_ - Indent + dx, YByDepth [x - 1] + dy + dy / 2, cLine);   // vertical line to parent's y posn
                YByDepth [x - 1] = y_ - dy / 2;
              }
            // Draw Arrow
            dy = NodeHeight / 3;
            if (Node->Children)
              DrawArrow ({x_, y_ + dy, dy, dy}, Node->Expanded ? dDown : dRight);
          }
        // Next
        y_ += NodeHeight;
        Node = Node->NodeNext (&x);
      }
    if (MaxX > SizeVirtual.x)
      SizeVirtual.x = MaxX;
    SizeVirtual.y = y_;
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
            Invalidate ();
            return true;
          }
        return false;
      }
    //FocusCheck (Event, Offset);
    Res = false;
    if (NodeHeight == 0)
      return false;
    SelectedOld = Selected;
    //if (IsEventMine (Event, Offset))
      if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)
        {
          Res = true;
          StationaryClicks++;
          Offset.y -= Scroll.y;
          Selected = FindItem ((Event->Y - Offset.y) / NodeHeight);
          if (Selected)
            if ((Event->X - Offset.x) / Indent <= Selected->Depth () || StationaryClicks >= 2)
              Selected->Expanded = !Selected->Expanded;
          Invalidate ();
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
              Invalidate ();
            }
        }
    if (Selected != SelectedOld)
      {
        Invalidate ();
        if (Action)
          Action (this);
      }
    return Res;
  }

