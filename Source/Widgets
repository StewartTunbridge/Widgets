WIDGETS
=======

Intro
-----
Widgets is a multi-platform library for rapidly creating GUI apps.
Targets currently include Linux (X), Windows and SDL.

Windows & Widgets
-----------------
To make a new window, create a _Form.
A _Form includes a _Container covering the entire _Form area.
Every _Container can contain children _Container(s) and will have a parent _Container
  (except the _Form's root _Container), making a tree.
_Containers look after themselves after being created, performing all drawing and user 
  interactions without help from the app.
All Widgets are descended (ultimately) from a _Container.
All Widgets are created inside a _Container and can overlap.
Coordinates are relative to the Parent _Container.
All _Containers can optionally have: a Text string, a _Font, a _Bitmap and Colours.
These resources are available to Children _Containers.

Example, Create a window, then add a Toolbar containing a Label and a Close button:
  _Form *Main = new _Form ("Title", {10, 10, 600, 400}, waResizable);
  _Container *Toolbar = new _Container (Main->Container, {0, 0, 0, 64});
  _Label *l = new _Label (Toolbar->Container, {0, 0, 64, 128}, "Some Text");
  _Button *bClose = new _Button (Toolbar->Container, {l->Rect.Width, 0, 64, 128}, "Close", AcnClose);

  Because an Action function was specified (AcnClose), it must be defined:
    void AcnClose (_Container *Container)
      {
        _Button *b = (_Button *) Container;
        if (!b->Down)
          Exit = true;
      }

_Containers can have many of their values changed or methods called after creation.
Example, change Colours:
  b->Colour = bBlue; b->ColourGrad = ColourAdjust (cBlue, 50);
  b->Invalidate (true);
Example, change Font, Text:
  b->FontSet ("../ARI.ttf", 24, fsOutline); b->TextSet ("Some Message");
Example, change Visibility:
  cTools1->VisibleSet (false); cTools2->VisibleSet (true);

Main Programme
--------------
The app must provide a main_ (arg, argv) function:
  int main_ (int argc, char *argv [])
    {
      <<initialize, make main _Form ...>>
      while (FormsUpdate ())   // this is the main loop
        {
          <<do stuff>>
          usleep (1000);
        }
      // Finished - Delete the Forms (only necessary if you break out of the main loop)
      while (FormsList)
        delete (FormsList);
    }

Main Modules
------------
WidgetsDriver*.cpp: Provide the targeted fundumentals
  This is all of the OS and hardware dependent code.
  To add a target, implement WidgetDriver.hpp in a new cpp file
Widgets.cpp: Implements _Form, _Container and all the basic Widgets
WidgetsText.cpp: Implements Font rendering, Sizing and Styles
  This relies on freetype
WidgetsImages.cpp: Loads and Saves various image file types in/from _Bitmap
  BMP support is native. Other formats rely on libpng and libjpeg
WidgetsEdit.cpp: Implements Single & Multi line Edit Boxes
WidgetGrid.cpp: Implements a versatile spreadsheet-like grid
WidgetTree.cpp: Implements a versatile tree structure
FileSlect.cpp: Create a _Form showing filtered local files. Aloows chaning directories and selcting an existing
  file or creating a new one.



WIDGETS REFERENCE
=================

_Form
-----
_Form (char *Title, _Rect Position, byte WindowAttribute = 0);   // Create Form
  #define waResizable 0x01
  #define waBorderless 0x02
  #define waAlwaysOnTop 0x04
  #define waFullScreen 0x08
  #define waModal 0x10
_Container *Container;
_Container *KeyFocus;
_Container *EventLock;
char KeyPress;   // Keyboard input from soft keyboards (0 = none)
byte Alpha;
byte WindowAttributes;
_FormEventPreview EventPreview;
void Clear (void);
bool SaveScreenShot (char *Filename);
void Die (void);  or just delete me


_Container
----------
_Container (_Container *Parent_, _Rect Rect_, char *Text_ = NULL);   // make me
  Rect_: x/y: Position relative to Parent. Width/Height
    To use Parent width/height, specify 0
    To use Parent width/height AND lock changes with the parent, specify 0
  void Die (void);
    This is how you delete yourself while still in your own method

Resources:
  char* TextGet (void);   // Get the Container's Text
  void TextSet (char *St);   // Set the Container's Text
  _Bitmap *Bitmap;
  _Bitmap* BitmapFind (void);
  int Colour, ColourGrad;   // Colours (-1 transparent). set Grad for graded colour
  int ColourFind (void);
  int ColourText;
  int ColourTextFind (void);
  bool FontSet (char *FontFile, int Size, byte Style = 0x00);
  _Font* FontFind (void);

Who/Where am I:
  _Rect Rect;   // Dimensions relative to Parent
  _Container *Parent;   // Owner
  _Container *Children;
  _Form *Form;   // _Form associated with me
  byte RectLock;   // Adjust container automagically on Parent resize. See rl*
    #define rlTop     0x01   // TRUE if Top is locked to Parent Bottom
    #define rlBottom  0x02   // TRUE if Bottom "
    #define rlLeft    0x04   // TRUE if Left is locked to Parent Right
    #define rlRight   0x08   // TRUE if Right "

General Data Places: just for you
  _Container *DataContainer;   // a pointer to _Container or anything else
  int DataInt;

Drawing:
  int BorderColourTL (int ColourReference);
  int BorderColourBR (int ColourReference);
  void DrawBorder (_Rect Rect, _Border Border, int ColourReference, int Indent = 0);
  void DrawBorder (_Border Border, int Indent);
  void DrawBezel (int Width);
  void DrawTab (_Rect Rec, int Colour, int ColourBG, bool Selected);
  void DrawLine (int x1, int y1, int x2, int y2, int Colour, int Width = 1);
  void DrawRectangle (_Rect Rect_, int ColourEdgeTL, int ColourEdgeBR, int ColourFill);
  void FillGradient (_Rect Rect, int Colour1, int Colour2);
  void DrawBitmap (_Bitmap *Bitmap, _Rect rSrc, _Rect rDest, bool Transparent);
  void DrawBackground (_Rect Rect);
  void DrawArrow (_Rect Rect_, _Direction Direction);   // Arrow: Direction: Left, Up, Right, Down
  void DrawCircle (int cx, int cy, int radius, int ColourEdge, int ColourFill = -1, int Width = 1, int Bevel = 0);
  void DrawRadius (int x, int y, int Rad1, int Rad2, int Angle, int Colour, int Width);
  void DrawScrollBar (bool Vert, int Bar1, int Bar2);
  void ScrollBarsDraw (byte Bars, _Point Value, _Point Content);
  bool ScrollRegionBarsDraw (void);

DrawText:
  If the _Container Font is NULL, it will inherit one
  Horizontal and Vertical Text Alignment
    aLeft, aRight, aCenter    (aLeft = aTop, aRight = aBottom)
  _Point TextMeasure (_Font *Font, char *Text);
  void TextOutAligned (_Font *Font, _Rect Rect, char *Text, _Align Align, _Align AlignVert);
  void TextOutWrap (_Rect Rect, char *Text, _Align Align, _Align AlignVert);
  void TextOutWrapReverse (_Rect Rect, char *St, _Align Align, _Point Margin);
    As above BUT the colours are "Inverted" for Menu Selections etc

State:
  void Invalidate (bool ReDraw);
    when you are finished drawing
  bool IsVisible (void);
  void VisibleSet (bool Value);
  bool IsEnabled (void);
  void EnabledSet (bool Enabled_);

When Making a new Widget:
  virtual void DrawCustom (void);   // Implemented by descendants
  virtual bool ProcessEventCustom (_Event *Event, _Point Offset);   // Implemented by descendants
    bool IsEventWithin (_Event *Event, _Point Offset);   // Is this event inside me
    bool IsEventMine (_Event *Event, _Point Offset);
  virtual void PollCustom (void);
  //
  bool ScrollBarsEvent (_Event *Event, _Point Offset, byte Bars, _Point Content, _Point *Scroll);
  _Point SizeVirtual;   // Size for scrollable Containers
  _Point Scroll;   // Displacement for scrolling regions
  bool ScrollRegionBarsEvent (_Event *Event, _Point Offset);

_Button
-------
_ButtonRadio
------------
_CheckBox
------------
_ButtonRound
------------
_ButtonArrow
------------
_ButtonSecret
-------------
_Label
------
_LabelNumber
------------
_LabelMoney
-----------
_LabelTime
----------
_LabelDateTime
--------------
_LabelStrip
-----------
_LabelScroll
------------
_EditNumber
-----------
_Slider
-------
_Knob
-----
_Wait
-----
_Image
------
_SelectionList
--------------
_Menu
-----
_MenuPopup
----------
_DropList
---------
_Tabs
-----
_SoftKeyboard
-------------

GRIDS

TREES

FILE SELECT

==============================================
