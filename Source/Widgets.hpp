///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS: MULTI TARGET GUI DEVELOPMENT
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////


#ifndef _Widgets
#define _Widgets

#include "lib.hpp"
#include "WidgetsDriver.hpp"
#include "WidgetsText.hpp"

extern bool DebugDisplayContainers;
extern int Time1, Time2, Time3;   // Performance measurement

extern void PointToStr (char **St, _Point Point);
extern void DebugAdd (const char *St);   // defined in application
extern void DebugAddS (const char *s1, const char *s2);   // defined in Widgets.cpp
extern void DebugAddSS (const char *s1, const char *s2, const char *s3);   // "
extern void DebugAddInt (const char *St, int n);   // "
extern void DebugAddHex (const char *St, int n);   // "
extern void DebugAddP (const char *St, _Point p);   // "
//extern void DebugAddR (const char *St, _Rect *r);   // "
extern void DebugAddError (const char* Message);   // "

extern _Rect AddMargin (_Rect Rect, _Point Margin);

// FONT ALIGNMENT
enum _Align {aLeft, aRight, aCenter};   // Horizontal & Vertical Text Alignment
#define aTop aLeft
#define aBottom aRight

// BUTTON STYLE
typedef enum {bsNormal, bsFlat, bsSimple} _ButtonStyle;
extern _ButtonStyle ButtonStyleDefault;

// THEME
extern int cForm1;
extern int cForm2;
extern int cButton1;
extern int cButton2;
extern int cText;
extern int cTextBgnd;
extern int cSelected;
extern int cMenu;
//extern int cMenuText;

//extern int cTextHighlight;
//extern int cTextHighlightBgnd;

extern char *FontFileDefault;
extern int FontSizeDefault;
extern char *TextCallBackCursor;
extern _Point CursorLocation;
extern char *TextCallBackSelected [2];
extern int ColourTable [];   // Colours used by escape code in Label.Text

enum _Direction {dLeft, dUp, dRight, dDown};
enum _Border {bNone, bRaised, bLowered, bMote};

class _Container;
class _Form;

typedef bool (*_FormEventPreview) (_Form *Form, _Event *Event);

class _Form //: public _List
  {
    private:
      byte WindowAttributes;
      void CreateContainer (const char *Title);
    public:
      _Form *Next, *Prev;   // Next _Form in the list (see FormList)
      _ButtonStyle ButtonStyle;
      _Window *Window;   //The window we'll be rendering to
      _Container *Container;
      _Container *Focused;   // Container with keyboard focus
      _Container *EventMouseLock;   // Containers lock focus when mouse key pressed
      _Container *EventLock;   // Containers lock focus when mouse key pressed
      _Bitmap *Image;
      _Point RedrawZone [2];
      bool ReRender;
      int *KeySeries;   // Keyboard input from soft keyboards or Macros (NULL = none)
      byte Alpha;
      _FormEventPreview EventPreview;   // Specify a function to preview, act on, cancel all Form's events.
      bool DieFlag;
      void Clear (void);   // Delete everything, start fresh
      // bool SaveScreenShot (char *Filename); depricated, use BitmapSave (Form->Image)
      void Die (void);   // Schedule a self delete
      void RedrawZoneAdd (_Container *Container, _Point Offset);
      _Form (const char *Title, _Rect Position, byte WindowAttribute = 0);   // Create Form
      virtual ~_Form (void);   // Manually remove Form (happens on window close)
  };

// Look for events, processes everything, draw & render whats needed, deletes closing windows, returns true when last is gone
extern bool FormsUpdate (void);   // Acts on Events, updates Widgets, returns true for close app

extern _Form *FormList;   // List of existing forms

#include "WidgetsImages.hpp"

#define ScrollBarWidth 12

// RenderFlags:
#define rRedrawPart  0x01   // TRUE if Container needs to be partly redraw
#define rRedraw      0x02   // TRUE if Container needs to be redraw
#define rRender      0x04   // TRUE if this Container and its Children need to be rendered
#define rClear       0x08   // TRUE if container's region just became invisible
#define rDelete      0x80   // TRUE to schedule suicide  :(
#define rDrawing     0x10   // TRUE if currently being drawn
#define rRedrawAgain 0x20   // TRUE to schedula rRedraw after drawing (something changed)

// RectLock:
#define rlTop     0x01   // TRUE if Top is locked to Parent Bottom
#define rlBottom  0x02   // TRUE if Bottom "
#define rlLeft    0x04   // TRUE if Left is locked to Parent Right
#define rlRight   0x08   // TRUE if Right "

// Scroll Bars (when not automatic)
#define sbVert 0x01
#define sbHorz 0x02

typedef enum {ptmBlend, ptmSlide, ptmDrop, ptmRandom} _PagingTransitionMode;

typedef void (*_Action) (_Container *Container);

class _Container
  {
    public:
      _Container *Next, *Prev;
      _Bitmap *Bitmap;   // Resource
      bool TabIn, TabOut;
    protected:
      byte RenderFlags;   // ####See bRender*
      char *Text;   // ####Used by descendants for ...
      _Point Shift, Clip;   // for transitions
      int Hover;   // >= 0 for Container Showing a Hover graphic
    private:
      bool Visible;   // True if this (and children) are displayed
      bool Enabled;   // True if events are allowed here and within
      _Container *PagingChild;
      int PagingTick;
      enum {ptsIdle, ptsBlending} PagingTransitionState;
      void TextOutWrap_ (_Rect Rect, char *Text, _Align Align, int *PosY, bool Write);   // TextOut multiline wordwrapped. '\n'=NewLine '\t'=Split
    public:
      _Font *Font;
      _Bitmap* Image;
      byte Alpha;
      _TextCallBack *TextCallBack;
      _Point TextMeasure (char *Text);
      _Form *Form;   // _Form associated with this.
      _Container *Parent;   // Owner
      _Rect Rect;   // Dimensions
      _Point SizeVirtual;   // Size for scrollable Containers
      _Point Scroll;   // Displacement for scrolling regions
      byte RectLock;   // Adjust container automagically on Parent resize. See rl*
      _Container *Children;   // Descendents List
      bool NoDraw;
      int Colour, ColourGrad;   // Colours (-1 transparent). set Grad for graded colour
      int ColourText;
      _Action Action;   // Callback function when something important happens
      //
      int PagingTime;
      int PagingTransitionTime;
      _PagingTransitionMode PagingTransitionMode;
      //
      // General data for whatever
      _Container *DataContainer;   // a pointer to _Container or anything else
      int DataInt;
      //
      _Container (_Container *Parent_, _Rect Rect_, const char *Text_ = NULL, _Action Action_ = NULL, int PagingTime_ = 0, int PagingTransitionTime_ = 0, _PagingTransitionMode PagingTransitionMode_ = ptmBlend);
        // Rect: if x / y < 0, posn is relative to Parent Width / Height
        //       if Width / Height <= 0, size is relative to Parent Width / Height AND suitable RectLock bits will be set
      virtual ~_Container (void);
      bool Resize (int Width, int Height);
      char* TextGet (void);   // Get the Container's Text
      void TextSet (char *St);   // Set the Container's Text
      void Invalidate (byte RenderFlags = rRedraw);
      void InvalidateAll (bool Redraw);
      bool IsVisible (void);
      void VisibleSet (bool Value);
      bool IsEnabled (void);
      void EnabledSet (bool Enabled_);
      //
      bool FontSet (char *FontFile, int Size = 0, byte Style = 0x00);
      //
      int ColourFind (void);
      int ColourTextFind (void);
      _Font* FontFind (void);
      _Bitmap* BitmapFind (void);
      int TextOutAligned (_Rect Rect, char *Text, _Align Align, _Align AlignVert);   // Find Font, Draw Text
      int TextOutWrap (_Rect Rect, char *Text, _Align Align, _Align AlignVert);
      void TextOutInverse (_Rect Rect, char *St, int Margin);   // Inverse for Menus etc
      int BorderColourTL (int ColourReference);
      int BorderColourBR (int ColourReference);
      void DrawBorder (_Rect Rect, _Border Border, int ColourReference, int Indent = 0);
      void DrawBorder (_Border Border, int Indent);
      void DrawBezel (int Width);
      void DrawTab (_Rect Rec, int Colour, int ColourBG, bool Selected, bool Hover);
      void DrawTick (_Rect Pos);
      //void DrawCursor (int x, int y, int Height);
      void DrawLine (int x1, int y1, int x2, int y2, int Colour, int Width = 1);
      void DrawLineTo (int x, int y);
      void DrawRectangle (_Rect Rect_, int ColourEdgeTL, int ColourEdgeBR, int ColourFill = -1);
      void FillGradient (_Rect Rect, int Colour1, int Colour2);
      void DrawBitmap (_Bitmap *Bitmap, _Rect rSrc, _Rect rDest, bool Transparent);
      void DrawBackground (_Rect Rect);
      void DrawArrow (_Rect Rect_, _Direction Direction);   // Arrow: Direction: Left, Up, Right, Down
      void DrawCircle (int cx, int cy, int radius, int ColourEdge, int ColourFill = -1, int Width = 1, int Bevel = 0);
      void DrawRadius (int x, int y, int Rad1, int Rad2, int Angle, int Colour, int Width);
      void DrawKnerl (_Rect Rect, bool Vert);
      void DrawScrollBar (bool Vert, int Bar1, int Bar2);
      void ScrollBarsDraw (byte Bars, _Point Value, _Point Content);
      bool ScrollBarsEvent (_Event *Event, _Point Offset, byte Bars, _Point Content, _Point *Scroll);
      bool ScrollRegionBarsDraw (void);
      bool ScrollRegionBarsEvent (_Event *Event, _Point Offset);
      bool IsEventWithin (_Event *Event, _Point Offset);   // Is this event inside me
      bool IsEventMine (_Event *Event, _Point Offset);
      //
      void Draw (bool Force, _Point Offset);
      void RenderContainers (bool RenderAll, _Point Dest, byte AlphaOffset);
      void Die (void);
      //
      bool FocusCheck (_Event *Event, _Point Offset);
      void FocusSet (void);   // Give Focus to a _Container
      bool ProcessEvent (_Event *Event, _Point Offset);   // returns true when actioned
      void Poll (void);
    protected:
      virtual void DrawCustom (void);   // Implemented by descendants
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);   // Implemented by descendants
      virtual void PollCustom (void);
  };

class _Button: public _Container
  {
    public:
      //_ButtonStyle ButtonStyle;
      _Align Align;
      bool Toggle;     // TRUE => Button toggles
      int Group;       // 0=>NoGroup ELSE Specifies group ID. Only 1 button down in parent container (if Toggle then allow all up)
      bool Down;       // Current state of button
      bool Flag;       // State of Button on mouse down
      //
      _Button (_Container *Parent, _Rect Rect, char *Text_ = NULL, _Action Action = NULL);
      void DownSet (bool Down_);   // Set Down state, invalidate and call the Action if it has changed
      //virtual ~_Button (void); // Use if we add an image
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);   // returns true when actioned
  };

class _ButtonRadio: public _Button
  {
    public:
      _ButtonRadio (_Container *Parent, _Rect Rect, char *Text_, _Action Action = NULL);
      virtual void DrawCustom (void);
  };

class _CheckBox: public _Button
  {
    public:
      _CheckBox (_Container *Parent, _Rect Rect, char *Text_, _Action Action = NULL);
      virtual void DrawCustom (void);
      bool StyleCross;
  };

class _ButtonRound: public _Button
  {
    public:
      _ButtonRound (_Container *Parent, _Rect Rect, char *Text_, _Action Action);
      virtual void DrawCustom (void);
  };

class  _ButtonArrow : public _Button
  {
    public:
      _Direction Direction;
      _ButtonArrow (_Container *Parent, _Rect Rect, char *Text_, _Action Action, _Direction Direction_);
      virtual void DrawCustom (void);
  };

class _ButtonSecret: public _Button
  {
    private:
      virtual void DrawCustom (void);
    public:
      _ButtonSecret (_Container *Parent, _Rect Rect, _Action Action);
  };

class _Label: public _Container
  {
    public:
      _Align Align;   // Horizontal Text Alignment
      _Align AlignVert;   // Vertical Text Alignment. aLeft == Top, aRight == Bottom
      _Border Border;
      int Margin;
      //
      _Label (_Container *Parent, _Rect Rect, const char *Text_, _Align Align_ = aLeft, _Border Border_ = bNone);
      virtual void DrawCustom (void);
      //virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

class _LabelNumber: public _Container
  {
    protected:
      int Value;
      virtual void DrawCustom (void);
    public:
      _Border Border;
      int ValueGet (void);
      void ValueSet (int Val);
      _LabelNumber (_Container *Parent, _Rect Rect, char *Text_ = NULL, _Border Border_ = bNone);
  };

class _LabelMoney: public _Container
  {
    public:
      _Border Border;
      int Value;   // Cents
      //
      _LabelMoney (_Container *Parent, _Rect Rect, char *Text_, _Border Border_);
      virtual void DrawCustom (void);
  };

class _LabelTime: public _Container
  {
    public:
      _Align Align;   // Horizontal Text Alignment
      _Border Border;
      int Value;   // Minutes
      //
      _LabelTime (_Container *Parent, _Rect Rect, char *Text_, _Border Border_);
      virtual void DrawCustom (void);
  };

// Text_: Controls display containing: $d $m $y $h $n $s eg "The time is $2h$2n:$2s Date is $d $3m $4y"
class _LabelDateTime: public _Container
  {
    public:
      _Align Align;   // Horizontal Text Alignment
      _Border Border;
      int Tick;
      //
      _LabelDateTime (_Container *Parent, _Rect Rect, char *Text_, _Align Align_ = aLeft, _Border Border_ = bNone);
      virtual void DrawCustom (void);
      void PollCustom (void);
  };

class _LabelStrip: public _Label
  {
    protected:
    public:
      _LabelStrip (_Container *Parent, int *YPos, int Height, char *Text_);
  };

class _LabelScroll: public _Container
  {
    public:
      int SlideX, SlideY;
      int SlidX, SlidY;
      int Tick;
      //
      _LabelScroll (_Container *Parent, _Rect Rect, char *Text_, int SlideX = 0, int SlideY = 0);
      virtual void DrawCustom (void);
      void PollCustom (void);
  };

class _EditNumber: public _Container
  {
    protected:
      int ColourFontBgnd;
      int Min, Max;
      bool NewFocus;
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
    public:
      _EditNumber (_Container *Parent, _Rect Rect, char *Text, _Action Action, int Min, int Max);
      virtual void DrawCustom (void);
      void ValueSet (int Val);
      int Value;
  };

class _Slider: public _Container
  {
    protected:
      int TextHeight;
      bool Vertical;
      int Value;
      bool MouseDown;
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
    public:
      int ValueMin, ValueMax;
      int ColourKnob;
      int Sections;
      int Decimals;
      _Slider (_Container *Parent, _Rect Rect, char *Text, _Action Action, int Min, int Max);
      int ValueGet (void);
      void ValueSet (int Value);
      virtual void DrawCustom (void);
  };

class _Knob: public _Container
  {
    protected:
      _Border Border;
      int Value;
      int ValueMin, ValueMax;
      int MouseX, MouseY;
      int MouseValue;
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
    public:
      int Markers;
      int Decimals;
      int ValueGet (void);
      void ValueSet (int Val);
      _Knob (_Container *Parent, _Rect Rect, char *Text, _Action Action, int Min, int Max, _Border Border = bNone);
  };

class _Section: public _Container
  {
    protected:
      virtual void DrawCustom (void);
    public:
      int ColourTitle;
      _Section (_Container *Parent, _Rect Rect, char *Title, int ColourTitle);
  };

class _Wait: public _Container
  {
    private:
      void PollCustom (void);
    protected:
      int Counter;
      int Tick;
      virtual void DrawCustom (void);
    public:
      int Period;   // Time between animations, mS
      bool Clicked;
      _Wait (_Container *Parent, _Rect Rect);
      bool ProcessEventCustom (_Event *Event, _Point Offset);
      void Advance (void);
  };

typedef enum {sNone, sNormal, sFill, sTile} _Stretch;

class _Image: public _Container
  {
    protected:
      virtual void DrawCustom (void);
    public:
      _Image (_Container *Parent, _Rect Rect, char *Filename, bool Transparent = true);
      _Stretch Stretch;
      _Align Align;
      _Align AlignVert;
      bool Transparent;
  };

class _SelectionList: public _Container
  {
    protected:
      _Point ItemSize;
      int Hovering;
      int ListOffset;   // scroll long lists
    public:
      char *List;
      int ListSize;
      int ListDisplayed;
      int Selected;
      int Boxed;    // Mask of Squares
      int Ticked;   // Mask of Ticks
      int Dotted;   // Mask of Dots
      //
      _SelectionList (_Container *Parent, _Rect Rect, char *Text_, _Action Action);
    //~_SelectionList (void);
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

class _Menu: public _Container
  {
    public:
      _Align Align;
      int Selected;
      int Boxed, Ticked, Dotted;
      _SelectionList *SelectionList;
      _Menu (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_);
    protected:
      char *Name;
      char *List;
      //
      ~_Menu (void);
      void DrawCustom (void);
      bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

class _MenuPopup : public _Container
  {
    public:
      _Align Align;
      int Selected;
      int Boxed, Ticked, Dotted;
      _Point Mouse;
      _Font *FontForm;   // Fudge fix later
      _SelectionList *SelectionList;
      _MenuPopup (_Container *Parent, _Rect Rect_, const char *Text_, _Action Action_);
    protected:
      bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

class _DropList : public _Container
  {
    public:
      _Align Align;
      int Selected;
      _SelectionList *SelectionList;
      char *SelectedStr (void);
      _DropList (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_ = nullptr);
    protected:
      void DrawCustom (void);
      bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

typedef void (*_ActionDrag) (int From, int To);

class _Tabs: public _Container
  {
    public:
      int Selected;   // Current selection
      bool TabScrollAllowed, TabDrag;
      int TabScroll;
      _ActionDrag ActionDrag;
      //
      _Tabs (_Container *Parent, _Rect Rect, char *Text_, _Action Action = nullptr, _ActionDrag ActionDrag = nullptr);
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

class _Splitter: public _Container
  {
    protected:
      _Point Reference;
      void DrawCustom (void);
      bool ProcessEventCustom (_Event *Event, _Point Offset);
    public:
      _Splitter (_Container *Parent, _Rect Rect);
  };

//////////////////////////////////////////////////////////////////////////////

class _SoftKeyboard: public _Container
  {
    private:
      _Point ButtonSize;
      _Point ButtonSpacing;
      void CreateButtons (char *Keys [], _Point Start);
    public:
      _SoftKeyboard (_Container *Parent, _Rect Rect_);
  };

#endif
