WIDGETS - SIMPLE MULTI-PLATFORM GUI APPS
=======

Intro
-----
Widgets is a multi-platform library for rapidly creating GUI, Unicode apps.
Targets currently include Linux (X), Windows and SDL. More coming.

Example Form (Sample1)
-------
```c
// Example, Create a window, then add a Toolbar containing a Label and a Close button:

// 1) Include headers
#include <unistd.h>   // just for usleep()
#include "Widgets.hpp"

// 2) Action(s)
void AcnClose (_Container *Container)
  {
    _Button *b = (_Button *) Container;
    b->Form->Die ();
  }

// 3) Main Programme, Create Form, Create Widgets and main Loop, Finish
int main_ (int argc, char *argv [])
  {
    // Create a _Form.
    _Form *Main = new _Form ("Title", {10, 10, 300, 200}, waResizable);
    Main->Container->FontSet ("ARI.ttf", 16);

    // Create Widgets in Containers.
    _Container *Toolbar = new _Container (Main->Container, {0, 0, 0, 32});
    _Label *l = new _Label (Toolbar, {0, 0, 128, 0}, "Some Text");
    _Button *bClose = new _Button (Toolbar, {l->Rect.Width, 0, 64, 0}, "Close", AcnClose);

    // Main Loop
    while (!FormsUpdate ())   // this is the main loop
      {
        // <<do other stuff>>
        usleep (1000);
      }
    return 0;
  }
```

FORMS
=====

**Create Form (ie Window)**

```c
_Form (char *Title, _Rect Position, byte WindowAttribute = 0);
```

*Title* is the Window Bar Title

*Position:* {x, y, Width, Height}
- (x, y) - Place on the Screen
- Width, Height - Available Window Size

*WindowAttribute* Bitset:
- waResizable 0x01
- waBorderless 0x02
- waAlwaysOnTop 0x04
- waFullScreen 0x08
- waModal 0x10  

```c
_Form *MyForm = new _Form ("My Title", {0, 0, 500, 500}, waResizable);
```

WIDGETS
=======

- A \_Form includes a \_Container covering the entire \_Form area.
- Every \_Container can contain children \_Container(s) and a parent \_Container, 
  making a tree.
- \_Containers look after themselves after being created, performing all drawing and user 
  interactions without help from the app.
- All Widgets are descended (ultimately) from a \_Container.
- All Widgets are created inside a \_Container.
- All \_Containers can optionally have resources which are available to Children \_Containers.
- Interactions with the app are through callback functions. Things like Button click, Edit change ...
- \_Containers can perform their own background processing and effects.
- \_Containers can automatically cycle through their Children with transition effects.
- Change Values and call Methods after creation.

```c
// Example, change Colours:
b->Colour = bBlue;
b->ColourGrad = ColourAdjust (cBlue, 50);
b->Invalidate ();
//
// Example, change Font, Text:
b->FontSet ("../ARI.ttf", 24, fsOutline);
b->TextSet ("Some Message");
//
// Example, change Visibility:
cTools1->VisibleSet (false);
cTools2->VisibleSet (true);
```

**Common**

*Create:*

```c
new [Widget] (_Container *Parent_, _Rect Rect_, const char *Text_, _Action Action ...)
```

- Parent - What you are created in.
- Rect - where you are within Parent: {x, y, Width, Height}.
    - If x / y is < 0, position is relative to the Parent Width / Height.
    - If Width / Height is <= 0, size is relative to the Parent Width / Height.
    - Both above cases lock Rect to the Parent during resizing. See *RectLock*
    - Eg. Full Height, full width with a gap of 4 on both sides  *{4, 0, -4, 0}*
    - Eg. Bottom right corner, 24 x 24  *{-24, -24, 0, 0}*
- Text - An optional string to Display OR to control Format OR for a List (tab separated) OR a Filename.
    - Displayed Text can contain [escape] [ID] to insert a square icon from Bitmap (below).
ID is *0-9* then *A-Z*. It assumes Bitmap is a horizontal strip, consisting of several adjacent square icons.
Eg *\e0 Exit*
    - Displayed Text can contain [\a] [Modifier] to change Font expression:
      - *\aB* for Bold. *\aI* for Italic. *\aU* for Underlined. *\aO* for Outlined. *\aS* for Shadow. *\a+* for Larger. *\a-* for Smaller.
      - *\a0* for Default Colour. *\a1* for RedDark. *\a2* for GreenDark. *\a3* for Brown. *\a4* for BlueDark. *\a5* for Purple. *\a6* for AquaDark. *\a7* for White. *\a8* for Gray. *\a9* for GrayDark.
      - Eg *\aBBold \aIBoldItalic  \aB\aI\a1NormalRed\a0*
- Action - What to do when the most basic Action happens. Define a function (\_Action) and
name it here.

```c
aButton = new _Button (aContainer, {0, 0, 120, 32}, "\e0New", ActionNew);
```

*Rendering and Paging*

- Alpha - the Transparency of the Widget.

- If you wish to display several objects sequentially with cool transitions, place these inside one Container and
set the following:
  - PagingTime - Display time each object (milliseconds).
  - PagingTransitionTime - Transition time between objects
  - PagingTransitionMode - ptmBlend, ptmSlide, ptmDrop, ptmRandom

*Resources*

- Bitmap - Load a Bitmap to provide icons-in-text. Or the image for _Image.
- Colour, ColourGrad - Background Colour (RGBA). If ColourGrad >= 0, a Graded Background with ColourGrad at the bottom.
- ColourText - Foreground Colour of Text.
- DataInt, DataContainer - An integer and a \_Container* for anything you want.

````c
widget->BitmapLoad (widget->Form->Window, "IconStrip.bmp");
widget->Colour = cBlue;
widget->ColourGrad = ColourAdjust (cBlue, 180);
widget->ColourText = cRed;
widget->Invalidate ();   // redraw widget
````

*Methods*
- TextSet() - Change the Text.
- TextGet() - Get the Text (read only)
- FontSet() - Set the Font for this Widget and its children (unless overriden).
- VisibleSet() - Make Visible or invisible (including the Children).
- IsVisible() - Is this Widget Visible?
- EnabledSet() - Enable Widget (and children).
- IsEnabled() - Is this Widget Enabled?
- Invalidate() - Notify that the Widget needs redrawing and/or re-rendering.

\_Container
-----------
The basis of all Widgets. Make this to contain a group of child Widgets as needed.

\_Button, \_ButtonRadio, \_CheckBox, \_ButtonRound, \_ButtonArrow, \_ButtonSecret
---------------------------------------------------------------------------

**Common:**
- Align, AlignVert: Text positioning: *aLeft, aRight, aCenter, aTop, aBottom*.
- Toggle: **true** if button toggles each click.
- Group == 0 for No Group: button is independent.
- Group > 0 for grouped Buttons: only 1 Button can be Down in a Group.
- Buttons in a Group must be Siblings.
  Many Groups can be siblings.
  If Toggle is **true**, all buttons can be Up.

*Note:* \_Form::ButtonStyle provides 3 different styles for many Widgets: *bsNormal, bsFlat, bsSimple*.

**\_CheckBox:**
- StyleCross: TRUE to draw a Cross , not a Tick.

**\_ButtonArrow:**
- Create: _ButtonArrow (Parent, Rect, Text, Action, Direction)
  - Direction specifies the arrow to draw: *dLeft, dUp, dRight, dDown*.

\_Label, \_LabelNumber, \_LabelMoney, \_LabelTime, \_LabelDateTime, \_LabelStrip
-------------------------------------------------------------

Text is displayed in an optional Border":

- Border - The surrounding box: *bNone, bRaised, bLowered, bMote*.
- Margin - Minimum gap either side of the Text.

**\_LabelNumber, \_LabelMoney**
- Value, ValueGet (), ValueSet () - Numerical value displayed.

**\_LabelDateTime:**
- Shows the current date / time
- Format determined by Text: $d $m $y $h $n $s. Eg *"The time is $2h$2n:$2s Date is $d $3m $4y"*

**\_LabelScroll:** Shows Text as a horizontal scrolling string.

\_EditNumber, \_Slider, \_Knob
-----------
Three ways of displaying / editing an integer.

**Common:**
- Create - [Widget] (Parent, Rect, Text, Action, int Min, int Max)
- int Value: the number
- ValueGet (), ValueSet (int): Get and Set Value
- Range limited: Min, Max

**\_EditNumber:**
- An edit box for a number:
- Responds to digits and backspace.

**\_Slider:**
- A mixer style slider control.
- Horizontal and Vertical orientation (depending on its proportions).

**\_Knob**
- A round dial you turn. Text is displayes under.

\_Section
--------
A coloured and titled rectangle to help divide a Form into sections in a more colourful way than just using Containers. Colour code different sections for clarity with subtlety OR use many different bright colours to irritate the chronophobic.
- Create - _Section (Parent, Rect, Text, ColourTitle)

\_Wait
-----
A dynamic rotating circle of dimming circles to show progress, or just busy

\_Image
------
- A holder for any Image.
- Text is a filename
- Alternatively set Bitmap with LoadBitmap()


\_SelectionList
--------------
- A transient popup list showing a set of Items.
- These Items are defined in Text (Tab separated)
- When an Item is selected from this it calls Action with Selected set accordingly or -1 if none selected

\_Menu
-----
- A permanent Menu Title is displayed
- Text is also a set of Tab separated Items, BUT the first is the Title, the rest will go in a new SelectionList when Title is clicked.
- On clicking this SelectionList, Selected is set and Action is called and then the SelectionList is destroyed
- This Widget is self-sizing. Use its Rect.Width to position subsequent Menu Widgets to make a complete Menu set

\_MenuPopup
----------
- An invisible Widget but a right click creates a SelectionList with Items defined in Text. (As above)
- On subsequently clicking (or releasing) an Item, Action is called with Selected set and then the SelectionList is destroyed
- Typically this is placed on top of a related Widget or even the Form->Container

\_DropList
---------
- A text box with a drop arrow
- Clicking will create a SelectionList defined by Text (as above)
- After a SelectionList click (or release), Selected is set and Action is called and then SelectionList is destroyed

\_Tabs
-----
- A set of adjacent Tabs defined by Text (tab separated) allowing users to pick between pages or anything else
- When a tab is clicked, Selected is set and Action is called
- Typically used to change the data set for a constant set of Widgets OR change to a totally different set of Widgets
    - In the 1st case, just reload the Widget values to match the new selection
    - In the 2nd case, it is efficient to place each page of widgets in their own Container and use VisibleSet() to swap what is seen

\_Splitter
---------
- A movable bar between 2 Containers to allow resizing of both
- Vertical or Horizontal (depending on its proportions)

\_SoftKeyboard
-------------
- Screen Keyboard. Very basic

\_GridView
---------
- Versatile Custom Square Grid. I built a full Spreadsheet with this.
- Transient in-grid Edit box
- Define an Action for:
    - CellDraw - Draw the Cell with your data
    - CellSelect - Act on a Cell being Selected
    - CellRead - Allow a Cell to be Changed (provide data for an in-grid edit)
    - CellWrite - Apply modified Cell data to your data
    - ColumnHighlight - A new column heading has been selected, for say, sorting
- Options for:
    - GridLines - on / off
    - SelectMulti - Select multiple Cells
    - SelectLine - Only select Whole Rows
    - AdjustColumn - Allow adjustable column widths
    - ColumnHighlight - Allow Column Highlight by clicking (for Sorting etc)

\_TreeView
---------
- Collapsible Tree. I built the FileBrowser with this.
- Add Nodes as Siblings OR Children
- Transient in-tree Edit box
- Action called on Node selection

\_Edit, \_EditMultiline, \_EditPopup, \_EditPopupDropList
-----
- Edit text: Single line, Multi line
- Drag / Drop
- Infinite Undo
- Move Line Up / Down
- Change Case word
- Auto Complete word
- Auto Indent
- Indent / Outdent Lines
- Callbacks for line and character effects.
- _EditPopup: An \_Edit with a popup menu for Cut, Copy, Paste, Clear
- _EditPopupDropList: An \_EditPopup with a drop down list
  - To add / move an item to the top of the list - Call DropListAdd (char *Item, int MaxItems)

Making a new Widget
-------------------
- Inheret from \_Container OR Similar Widget.
- Implement:
    - virtual void DrawCustom (void);
        - use Container::Draw*()
        - use Container::TextOutAligned() / TextOutWrap()
    - virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
        - use bool IsEventWithin (_Event *Event, _Point Offset);   // Is this event inside me
        - use bool IsEventMine (_Event *Event, _Point Offset);   // My Event?
    - virtual void PollCustom (void);   // Optional. For any background activity

PRE-MADE FORMS
=============

FileSelect
----------
- Creates a form listing local files and directories.
- User can Select a file and explore other directories.
- If SaveFile is TRUE, an Edit box for a new Filename and a Save button is provided.
- If a file is selected or Save is clicked, ActionFileSelected () will be called with the filepath in Result.
- Filter:
    - If not NULL, only filenames including this will be listed.
    - If it starts with a dot (.) and SaveFile is TRUE, this will provide a default extension.

```c
FileSelect ("Save Spreadsheet", ".ss", true, ActionSave_);
```

ColourSelect
------------
- Creates a form for user to select / modify a Colour.
- This provides a palette as well as sliders for Red, Green, Blue & Saturation.
- If OK is clicked, ActionColour will be called with the Result.
- A Parameter is added and passed to ActionColour to help differentiate items.

```c
ColourSelect ("Adjust Colour", Button->Colour, ActionColourSelected, Button);
```

MessageBox
----------
- Creates an waAlwaysOnTop _Form with a specified Title and Label.

```c
MessageBox ({300, 80}, "Title\vMessage to display");
```

MessageResponseBox
----------
- Creates a MessageBox with the addition of a row of buttons.
- When a Button is clicked (or the Form closed) Action will be called with the Button (or none) specified.

```c
void ActionNowWhat (int Result, void *Parameter)
  {
    if (Result == 0)
      LabelX->TextSet ("He said YES");
  }

...

MessageResponseBox ({300, 80}, "Title\vMessage to display", "Yes\tNo\tMaybe", ActionNowWhat, Parameter);
```

MODULES
=======
- WidgetsDriver*.cpp: Provide the target fundamentals.
  This is all of the OS and hardware dependent code.
  To add a target, implement WidgetDriver.hpp in a new cpp file.
- Widgets.cpp: Implements \_Form, \_Container and all the basic Widgets.
- WidgetsText.cpp: Implements Font rendering, Sizing and Styles.
  This relies on stb\_truetype.h (https://github.com/nothings/stb/tree/master).
- WidgetsImagesSTB.cpp: Loads and Saves various image file types in/from \_Bitmap.
  This relies on stb\_image.h and stb_image_write.h (").
- WidgetsImagesBMP.cpp: Loads and Saves BMP image files in/from \_Bitmap natively.
- WidgetsEdit.cpp: Implements Single & Multi line Edit Boxes.
- WidgetGrid.cpp: Implements a versatile spreadsheet-like grid.
- WidgetTree.cpp: Implements a versatile tree structure.
- ColourSelect.cpp: Select a Colour from a Palette and Sliders.
- FileSelect.cpp: Select a file for reading / writing.
- MessageBox.cpp: Post a message in a window.
- MessageResponseBox.cpp: Post a message with buttons in a window.
- Complex.c: Double precision, Complex number arithmetic and scientific functions.
- TCP.cpp: Opens TCP client and server sockets.
- WAV.cpp: Save a waveform in RAM to a wav file.
- lib.cpp: Many useful functions, for many things.


WIDGETS REFERENCE
=================

\_Form class
-----

**Items and Methods**

```c
typedef bool (*_FormEventPreview) (_Form *Form, _Event *Event);

_Container *Container;   // Root Container
_Container *KeyFocus;   // Container with Keyboard focus
_Container *EventLock;   // Set to lock  all events to one Container
int *KeySeries;   // Keyboard input from soft keyboards or Macros (NULL = none)
byte Alpha;   // Form Transparency
_FormEventPreview EventPreview;   // Specify a function to preview, act on, cancel all Form's events.

void Clear (void);   // Delete everything, start fresh
void Die (void);   // Schedule a self delete
```

**Other Functions**

````c
extern bool FormsUpdate (void);   // Acts on Events, updates Widgets, returns true for close app

extern _Form *FormList;   // List of existing forms
````

\_Container class
----------

**Items and Methods**

```c
_Container (_Container *Parent, _Rect Rect, char *Text = NULL, _Action Action = NULL);   // make me
char* TextGet (void);   // Get the Container's Text
void TextSet (char *St);   // Set the Container's Text
_Bitmap *Bitmap;   // Bitmap Resource for Images within Text etc
_Bitmap* BitmapFind (void);   // Find my nearest Bitmap
int Colour, ColourGrad;   // Background Colours (-1 transparent). set *Grad for graded Background
int ColourFind (void);   // Find my nearest Colour
int ColourText;   // Default Colour of Text
int ColourTextFind (void);   // Find my nearest ColourText
bool FontSet (char *FontFile, int Size, byte Style = 0x00);   // Set Container Font: TTF file, size, attributes
_Font* FontFind (void);   // Find my nearest Font

// Who/Where am I:
_Rect Rect;   // Dimensions relative to Parent
_Point SizeVirtual;   // Size for scrollable Containers
_Container *Parent;   // Owner
_Container *Children;   // Descendents List
_Form *Form;   // _Form associated with me
byte RectLock;   // Adjust container automagically on Parent resize, bitest:
  // rlTop     0x01   // TRUE if Top is locked to Parent Bottom
  // rlBottom  0x02   // TRUE if Bottom "
  // rlLeft    0x04   // TRUE if Left is locked to Parent Right
  // rlRight   0x08   // TRUE if Right "

// General Data: for whatever
_Container *DataContainer;   // a pointer to _Container or anything else
int DataInt;   // an integer for anything

// Drawing:
int BorderColourTL (int ColourReference);   // Brighten ColourReference
int BorderColourBR (int ColourReference);   // Darken ColourReference
void DrawBorder (_Rect Rect, _Border Border, int ColourReference, int Indent = 0);
void DrawBorder (_Border Border, int Indent);
void DrawBezel (int Width);
void DrawTab (_Rect Rec, int Colour, int ColourBG, bool Selected, bool Hover);
void DrawTick (_Rect Pos);
void DrawLine (int x1, int y1, int x2, int y2, int Colour, int Width = 1);
void DrawLineTo (int x, int y);
void DrawRectangle (_Rect Rect_, int ColourEdgeTL, int ColourEdgeBR, int ColourFill);
void FillGradient (_Rect Rect, int Colour1, int Colour2);
void DrawBitmap (_Bitmap *Bitmap, _Rect rSrc, _Rect rDest, bool Transparent);
void DrawBackground (_Rect Rect);
void DrawArrow (_Rect Rect_, _Direction Direction);   // Arrow: Direction: Left, Up, Right, Down
void DrawCircle (int cx, int cy, int radius, int ColourEdge, int ColourFill = -1, int Width = 1, int Bevel = 0);
void DrawRadius (int x, int y, int Rad1, int Rad2, int Angle, int Colour, int Width);
void DrawScrollBar (bool Vert, int Bar1, int Bar2);
void ScrollBarsDraw (byte Bars, _Point Value, _Point Content);   // Or use SizeVirtual

// DrawText:
//   If the _Container Font is NULL, it will inherit one
//   Horizontal and Vertical Text Alignment:
//     aLeft, aRight, aCenter, aTop, aBottom
_Point TextMeasure (char *Text);
void TextOutAligned (_Rect Rect, char *Text, _Align Align, _Align AlignVert);
void TextOutWrap (_Rect Rect, char *Text, _Align Align, _Align AlignVert);
void TextOutInverse (_Rect Rect, char *St, int Margin);   // Inverted for Menus etc

// State:
void Invalidate ();
  // Schedule Container redraw. Optional render parameter
  //  rRedrawPart  0x01   // TRUE if Container needs to be partly redraw
  //  rRedraw      0x02   // TRUE if Container needs to be redraw
  //  rRender      0x04   // TRUE if this Container and its Children need to be rendered
  //  rClear       0x08   // TRUE if container's region just became invisible
  //  rDelete      0x80   // TRUE to schedule suicide  :(
void InvalidateAll (bool Redraw);   // Invalidate whole family
bool IsVisible (void);
void VisibleSet (bool Value);
bool IsEnabled (void);
void EnabledSet (bool Enabled_);

void Die (void);   // Schedule a self delete
```

GridView
--------

```c
typedef void (*_ActionCellDraw) (_GridView *Grid, int x, int y, _Rect Rect);   // Draw Cell contents in Rect
typedef void (*_ActionCellSelect) (_GridView *Grid, int x, int y);   // Cell selected
typedef char* (*_ActionCellRead) (_GridView *Grid, int x, int y);   // Give me data to edit or NULL. Result not altered, nor freed
typedef void (*_ActionCellWrite) (_GridView *Grid, int x, int y, char *Data, bool Abort);   // Here's your modified data. Abort on esc
typedef void (*_ActionColumnHighlight) (_GridView *Grid, int x);   // Highlighted Column selected

_ActionCellDraw ActionCellDraw;   // Draws the specific cell contents
_ActionCellSelect ActionCellSelect;   // Cell Selected
_ActionCellRead ActionCellRead;   // Define to allow Cell Editing OR an action when cell hard selected
_ActionCellWrite ActionCellWrite;   // After Editing, returns the edited result
_ActionColumnHighlight ActionColumnHighlight;   // React to Column Title selected
````

TreeView
----

TreeView is a tree of TreeViewNode

````c
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
````

TreeViewClass

````c
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
````

Edit, EditMultiline
-------------------

**Create**

```c
new _Edit (_Container *Parent, _Rect Rect, char *Text = NULL, _Action Action_ = NULL);
new _EditMultiline (_Container *Parent, _Rect Rect, char *Text = NULL, _Action Action_ = NULL);
````

**Items and Methods**

```c
int TextLen;   // Length of Text
_Point Posn;   // Cursor position rel to top of file
int IndexLine;   // Index of Start of Current Line
int Selected [2];   // Index of current Selection
void Insert (int Pos, int Size, char *St);   // Insert St into Text
void Delete (int Pos, int Size);   // Delete Size from Text
void MoveToIndex (int Index);   // Move to specified Index
void MoveToPosn (_Point Posn);   // Move to specified Cursor position
void ScrollToCursor (void);   // adjust Scroll to show cursor
_Action Action;   // Call when Text changes
_EditDrawLineCallBack EditDrawLineCallBack;   // Callback prior to Drawing each line
bool Changed, ChangedNew;   // Text Changed flag
_EditCommon (_Container *Parent, _Rect Rect, char *Text = NULL, _Action Action_ = NULL, bool Multi = false);
~_EditCommon (void);
void TextSet (void);
void TextSet (char *Text);
bool SelectWord (void);   // Select word at cursor: True if successful
void SelectLine (void);   // Select line at cursor
void BookmarkSwap (void);   // Swap cursor with Bookmark
void Clipboard (_ClipboardOp ClipboardOp);   // All Clipboard operations
void Undo (void);   // Perform an Undo if possible (all Text changes go thru Insert/Delete/TextSet
void UndoClearList (void);   // Clear Undo list
bool UndoAvailable (void);   // Anything to Undo?
bool FileLoad (char *Filename, bool Hex = false);   // Loads Text from file. Force Hex format.
bool FileSave (char *Filename);   // Saves Text to File
bool Hex;   // Set by FileLoad. Used in FileSave
_EOL EOL;   // "
_UTF UTF;   // "
````
