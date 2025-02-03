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
extern void DebugAddInt (const char *St, int n);   // "
extern void DebugAddHex (const char *St, int n);   // "
extern void DebugAddP (const char *St, _Point p);   // "
extern void DebugAddR (const char *St, _Rect *r);   // "

extern _Rect AddMargin (_Rect Rect, _Point Margin);

// FONT ALIGNMENT
enum _Align {aLeft, aRight, aCenter};   // Horizontal & Vertical Text Alignment
#define aTop aLeft
#define aBottom aRight

// BUTTON STYLE
typedef enum {bsNormal, bsFlat, bsSimple} _ButtonStyle;
extern _ButtonStyle ButtonStyle;

// COLOUR CONSTANTS
extern const int cBlack;
extern const int cWhite;
extern const int cRed;
extern const int cGreen;
extern const int cBlue;
extern const int cAqua;
extern const int cOrange;
extern const int cGray;
extern const int cYellow;
extern const int cPink;
extern const int cBrown;
extern const int cMaroon;

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

extern _ButtonStyle ButtonStyleDefault;

extern char *FontFileDefault;
extern int FontSizeDefault;
extern char *TextCallBackCursor;
extern _Point CursorLocation;
extern char *TextCallBackSelection [2];

enum _Direction {dLeft, dUp, dRight, dDown};
enum _Border {bNone, bRaised, bLowered, bMote};

class _Container;
class _Form;

typedef bool (*_FormEventPreview) (_Form *Form, _Event *Event);

class _Form //: public _List
  {
    private:
      //_Rect Cursor;
      //int CursorTick;
      void CreateContainer (char *Title);
    public:
      _Form *Next, *Prev;   // Next _Form in the list (see FormList)
      _Window *Window;   //The window we'll be rendering to
      _Container *Container;
      _Container *KeyFocus;
      _Container *EventLock;
      //_Event Event;   // Last Event this form
      _Texture *WindowTexture;
      bool ReRender;
      char KeyPress;   // Keyboard input from soft keyboards (0 = none)
      byte Alpha;
      byte WindowAttributes;
      _FormEventPreview EventPreview;
      bool DieFlag;
      _Form (char *Title, _Rect Position, byte WindowAttribute = 0);   // Create Form
      virtual ~_Form (void);   // Manually remove Form (happens on window close)
      void Clear (void);
      bool SaveScreenShot (char *Filename);
      void Die (void);
  };

// Look for events, processes everything, draw & render whats needed, deletes closing windows, returns true when last is gone
extern bool FormsUpdate (void);   // Looks for events, processes everything, returns true for close

extern _Form *FormList;   // List of existing forms

#include "WidgetsImages.hpp"

#define ScrollBarWidth 12

// RenderFlags:
#define rRedrawPart 0x01   // TRUE if Container needs to be partly redraw
#define rRedraw     0x02   // TRUE if Container needs to be redraw
#define rRender     0x04   // TRUE if this Container and its Children need to be rendered
#define rClear      0x08   // TRUE if container's region just became invisible
#define rInvalid    0x10   // TRUE if this Container was Invalidated
#define rDelete     0x20   // TRUE to schedule suicide  :(

// RectLock:
#define rlTop     0x01   // TRUE if Top is locked to Parent Bottom
#define rlBottom  0x02   // TRUE if Bottom "
#define rlLeft    0x04   // TRUE if Left is locked to Parent Right
#define rlRight   0x08   // TRUE if Right "

// Scroll Bars (when not automatic)
#define sbVert 0x01
#define sbHorz 0x02

typedef enum {ptmBlend, ptmSlide, ptmDrop, ptmRandom} _PagingTransitionMode;

//typedef bool (*_EventPreview) (_Container *Container, _Event *Event, int OffsetX, int OffsetY);

class _Container
  {
    public:
      _Container *Next, *Prev;
      _Bitmap *Bitmap;
    protected:
      byte RenderFlags;   // ####See bRender*
      char *Text;   // ####Used by descendants for ...
    private:
      bool Visible;   // True if this (and children) are displayed
      bool Enabled;   // True if events are allowed here and within
      _Container *PagingChild;
      int PagingTick;
      enum {ptsIdle, ptsBlending} PagingTransitionState;
      void TextOutWrap_ (_Rect Rect, char *Text, _Align Align, int *PosY, bool Write);   // TextOut multiline wordwrapped. '\n'=NewLine '\t'=Split
    protected:
      _Point Shift, Clip;   // for transitions
      _Font *Font;
    public:
      _Texture* Texture;
      byte Alpha;
      _Point TextMeasure (_Font *Font, char *Text);
      _Form *Form;   // _Form associated with this. (ultimate Parent)
      _Container *Parent;   // Owner
      _Rect Rect;   // Dimensions
      _Point SizeVirtual;   // Size for scrollable Containers
      _Point Scroll;   // Displacement for scrolling regions
      //bool DrawBackgroundInhibit;   // Special case when partial drawing required
      byte RectLock;   // Adjust container automagically on Parent resize. See rl*
      _Container *Children;
      int Colour, ColourGrad;   // Colours (-1 transparent). set Grad for graded colour
      int ColourText;
      //
      int PagingTime;
      int PagingTransitionTime;
      _PagingTransitionMode PagingTransitionMode;
      //
      // General data for whatever
      _Container *DataContainer;   // a pointer to _Container or anything else
      int DataInt;
      //
      _Container (_Container *Parent_, _Rect Rect_, char *Text_ = NULL, int PagingTime_ = 0, int PagingTransitionTime_ = 0, _PagingTransitionMode PagingTransitionMode_ = ptmBlend);
        // Rect_: To use Parent Width/Height specify 0. This also sets bits RectLock
      virtual ~_Container (void);
      bool Resize (int Width, int Height);
      char* TextGet (void);   // Get the Container's Text
      void TextSet (char *St);   // Set the Container's Text
      void Invalidate (bool ReDraw);
      void InvalidateAll (bool Redraw);
      bool IsVisible (void);
      void VisibleSet (bool Value);
      bool IsEnabled (void);
      void EnabledSet (bool Enabled_);
      //
      bool FontSet (char *FontFile, int Size, byte Style = 0x00);
      //
    //protected:
      int ColourFind (void);
      int ColourTextFind (void);
      _Font* FontFind (void);
      _Bitmap* BitmapFind (void);
      bool RenderTarget_ (_Texture *Texture, _Point Offset, int Colour, _Rect Clip);
      void TextOutAligned (_Font *Font, _Rect Rect, char *Text, _Align Align, _Align AlignVert);   // Find Font, Draw Text
      void TextOutWrap (_Rect Rect, char *Text, _Align Align, _Align AlignVert);
      void TextOutWrapInverse (_Rect Rect, char *St, _Align Align, _Point Margin);   // Inverse for Menus etc
      int BorderColourTL (int ColourReference);
      int BorderColourBR (int ColourReference);
      void DrawBorder (_Rect Rect, _Border Border, int ColourReference, int Indent = 0);
      void DrawBorder (_Border Border, int Indent);
      void DrawBezel (int Width);
      void DrawTab (_Rect Rec, int Colour, int ColourBG, bool Selected);
      //void DrawCursor (int x, int y, int Height);
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
      bool ScrollBarsEvent (_Event *Event, _Point Offset, byte Bars, _Point Content, _Point *Scroll);
      bool ScrollRegionBarsDraw (void);
      bool ScrollRegionBarsEvent (_Event *Event, _Point Offset);
      //bool ScrollBarClick (_Event *Event, _Point Offset, int Val1, int Val2, int *Value);//
      //bool ScrollBarHorizontalClick (_Event *Event, _Point Offset, int Val1, int Val2, int *Value);//
      bool IsEventWithin (_Event *Event, _Point Offset);   // Is this event inside me
      bool IsEventMine (_Event *Event, _Point Offset);
      //
      //_EventPreview EventPreview;
      void Draw (bool Force, _Point Offset);
      virtual void DrawCustom (void);   // Implemented by descendants
      void Render1 (bool RenderAll, int DestX, int DestY, byte AlphaOffset);
      void Render2 (int DestX, int DestY);
      void Die (void);
      //
      bool CheckFocus (_Event *Event, _Point Offset);
      bool ProcessEvent (_Event *Event, _Point Offset);   // returns true when actioned
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);   // Implemented by descendants
      void Poll (void);
      virtual void PollCustom (void);
  };

typedef void (*_Action) (_Container *Container);

class _Button: public _Container
  {
    public:
      //_ButtonStyle ButtonStyle;
      _Align Align;
      bool Toggle;     // TRUE => Button toggles
      bool Grouped;    // TRUE => Only 1 button down in parent container (if Toggle then allow all up)
      bool Down;       // Current state of button
      bool Flag;       // State of Button on mouse down
      bool Hover;      // Mouse is over button
      _Action Action;  // Callback function when Down changes
      //
      _Button (_Container *Parent, _Rect Rect, char *Text_ = NULL, _Action Action = NULL);
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
      //
      _Label (_Container *Parent, _Rect Rect, char *Text_, _Align Align_ = aLeft, _Border Border_ = bNone);
      virtual void DrawCustom (void);
      //virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

class _LabelNumber: public _Container
  {
    public:
      _Border Border;
      int Value;
      //
      _LabelNumber (_Container *Parent, _Rect Rect, char *Text_ = NULL, _Border Border_ = bNone);
      virtual void DrawCustom (void);
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
      int Max;
      _Action Action;
      bool NewFocus;
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
    public:
      _EditNumber (_Container *Parent, _Rect Rect, char *Text, int Max, _Action Action = NULL);
      virtual void DrawCustom (void);
      void ValueSet (int Val);
      int Value;
  };

class _Slider: public _Container
  {
    protected:
      bool Vertical;
      int ValueMin, ValueMax;
      _Action Action;
      bool MouseDown;
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
    public:
      int ColourKnob;
      _Slider (_Container *Parent, _Rect Rect, int Min, int Max, _Action Action = NULL);
      virtual void DrawCustom (void);
      int Value;
  };

class _Knob: public _Container
  {
    protected:
      _Border Border;
      int ValueMin, ValueMax;
      _Action Action;
      int MouseX, MouseY;
      int MouseValue;
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
    public:
      int Markers;
      _Knob (_Container *Parent, _Rect Rect, char *Text, int Min, int Max, _Border Border = bNone, _Action Action = nullptr);
      virtual void DrawCustom (void);
      int Value;
  };

class _Wait: public _Container
  {
    protected:
      int Counter;
      int Tick;
      virtual void DrawCustom (void);
    public:
      _Wait (_Container *Parent, _Rect Rect);
      void PollCustom (void);
  };

typedef enum {sNone, sNormal, sFill, sTile} _Stretch;

class _Image: public _Container
  {
    protected:
      virtual void DrawCustom (void);
    public:
      _Image (_Container *Parent, _Rect Rect, char *Filename, bool Transparent = true);
      ~_Image (void);
      //_Bitmap *Bitmap;
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
    public:
      char *List;
      _Align Align;
      int Selected;
      _Action Action;  // Callback function when Value changes
      //
      _SelectionList (_Container *Parent, _Rect Rect, char *Text_, _Action Action);
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

class _Menu: public _Container
  {
    public:
      _Align Align;
      int Selected;
      _Action Action;
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
      _Action Action;
      _Point Mouse;
      _SelectionList *SelectionList;
      _MenuPopup (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_);
    protected:
      bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

class _DropList : public _Container
  {
    public:
      _Align Align;
      int Selected;
      _Action Action;
      _SelectionList *SelectionList;
      _DropList (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_ = nullptr);
      ~_DropList (void);
    protected:
      void DrawCustom (void);
      bool ProcessEventCustom (_Event *Event, _Point Offset);
  };

class _Tabs: public _Container
  {
    public:
      int Selected;
      _Action Action;  // Callback function when Down changes
      //
      _Tabs (_Container *Parent, _Rect Rect, char *Text_, _Action Action = nullptr);
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
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
