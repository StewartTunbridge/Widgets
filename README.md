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
All Widgets are created inside a _Container.
Coordinates are relative to the Parent _Container.
All _Containers can optionally have: a Text string, a _Font, a _Bitmap and Colours.
These resources are available to Children _Containers.

Example, Create a window: 
  _Form *Main = new _Form ("Title", {10, 10, 600, 400}, waResizable);

Example, Add a Button to the _Form:
  _Button *bClose = new _Button (Main->Container, {4, 4, 64, 128}, "Close", AcnClose);
  The specified action function (AcnClose) must exist: Example
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
          sleep ();
        }
      // Finished - Delete the Forms
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


