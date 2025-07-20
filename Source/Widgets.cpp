///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS - GUI Widget Library
// ============================
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
// Target independent expandable set of GUI controls
// Target dependent stuff entirely implemented externally defined by "WidgetsDriver.hpp"
//
///////////////////////////////////////////////////////////////////////////////


#include <string.h>
#include <unistd.h>

#include "Widgets.hpp"
#include "WidgetsDriver.hpp"

bool DebugDisplayContainers = false;
int Time1, Time2, Time3;   // Performance measurement

// THEME
int cForm1 = 0xE8E8E8; //0xC0C0C0;
int cForm2 = 0xC0C0C0; //-1;
int cButton1 = 0xE0E0E0;
int cButton2 = 0xC0C0C0;
int cText = cBlack;
int cTextBgnd = cWhite;
int cSelected = ColourAdjust (cBlue, 185);
//int cMenu = -1; //ColourAdjust (cBlue, 180); //-1; //cWhite;
//int cMenuText = cBlue;
//int cMenuText = cBlack;
//int cTextHighlight = cBlack; //cBlack;cWhite
//int cTextHighlightBgnd =

_ButtonStyle ButtonStyle = bsNormal;

char *FontFileDefault;

_Form *FormList = NULL;

int WindowCount = 0;
int CreateCount = 0;
int DelCount = 0;

word MouseKeys = 0x0000;

_Point BottomRight (_Rect *Rect)
  {
    return {Rect->x + Rect->Width - 1, Rect->y + Rect->Height - 1};
  }


///////////////////////////////////////////////////////////////////////////////
//
// DEBUG & SUPPORT

// debug string helpers
void StrCat_ (char **Dest, char *St, int Len)
  {
    byte c;
    char *Dest0;
    //
    Dest0 = *Dest;
    if (St)
      while (*St && (*Dest - Dest0 < Len))
        {
          c = *St++;
          if (c < ' ')
            {
              StrCat (Dest, '^');
              StrCat (Dest, c | '@');
            }
          else if (c < 0x80)
            StrCat (Dest, c);
          else
            NumToHex (Dest, c, 2);
        }
  }

void PointToStr (char **St, _Point Point)
  {
    StrCat (St, '(');
    IntToStr (St, Point.x);
    StrCat (St, ',');
    IntToStr (St, Point.y);
    StrCat (St, ')');
  }

void RectToStr (char **sp, _Rect Rec)
  {
    StrCat (sp, '(');
    IntToStr (sp, Rec.x);
    StrCat (sp, ',');
    IntToStr (sp, Rec.y);
    StrCat (sp, ") ");
    IntToStr (sp, Rec.Width);
    StrCat (sp, 'x');
    IntToStr (sp, Rec.Height);
  }

void DebugAddS (const char *s1, const char *s2)
  {
    char *s = (char *) malloc (StrLen (s1) + StrLen (s2) + 2);
    char *p = s;
    if (s1)
      StrCat (&p, s1);
    StrCat (&p, ' ');
    if (s2)
      StrCat (&p, s2);
    *p = 0;
    DebugAdd (s);
    free (s);
  }

void DebugAddIntDecHex (const char *St, int n, bool Hex)
  {
    char *s = (char *) malloc (StrLen (St) + 16);
    char *p = s;
    if (St)
      {
        StrCat (&p, St);
        StrCat (&p, ' ');
      }
    if (Hex)
      NumToStrBase (&p, n, 0, 16);
    else
      IntToStr (&p, n);
    *p = 0;
    DebugAdd (s);
    free (s);
  }

void DebugAddInt (const char *St, int n)
  {
    DebugAddIntDecHex (St, n, false);
  }

void DebugAddHex (const char *St, int n)
  {
    DebugAddIntDecHex (St, n, true);
  }

void DebugAddP (const char *St, _Point pnt)
  {
    char *s = (char *) malloc (StrLen (St) + 64);
    char *p = s;
    if (St)
      StrCat (&p, St);
    StrCat (&p, ' ');
    PointToStr (&p, pnt);
    *p = 0;
    DebugAdd (s);
    free (s);
  }

void DebugAddR (const char *St, _Rect r)
  {
    char *s = (char *) malloc (StrLen (St) + 128);
    char *p = s;
    if (St)
      StrCat (&p, St);
    StrCat (&p, ' ');
    RectToStr (&p, r);
    *p = 0;
    DebugAdd (s);
    free (s);
  }

void DebugAddError (const char* Message)
  {
    char *s = (char *) malloc (StrLen (Message) + 32);
    char *ps = s;
    //
    StrCat (&ps, "**** ERROR ");
    StrCat (&ps, Message);
    *ps = 0;
    //
    DebugAdd (s);
    free (s);
  }


///////////////////////////////////////////////////////////////////////////////
//
// RENDER / DRAW BASICS

_Bitmap *TargetBitmap = NULL;
_Point TargetBitmapScroll = {0, 0};   // Set by Draw() from _Container::Scroll
_Point TargetBitmapSize = {0, 0};   // Set by Draw() from _Container::Scroll

void RenderDrawPixel (int x, int y, int Colour)
  {
    x -= TargetBitmapScroll.x;
    y -= TargetBitmapScroll.y;
    //if (x >= 0 && y >= 0 && x < TargetBitmapSize.x && y < TargetBitmapSize.y)
      BitmapSetPixel (TargetBitmap, x, y, Colour | 0xFF000000);
  }

void RenderDrawPixelAlpha (int x, int y, int Colour, byte Alpha)
  {
    int Colour2;
    byte Alpha2;
    //
    x -= TargetBitmapScroll.x;
    y -= TargetBitmapScroll.y;
    //if (x >= 0 && y >= 0 && x < TargetBitmapSize.x && y < TargetBitmapSize.y)
      {
        Colour2 = BitmapGetPixel (TargetBitmap, x, y);
        Alpha2 = AlphaGet (&Colour2);
        if (Alpha2)
          {
            Colour = AlphaBlend (Colour, Colour2, Alpha);
            Alpha = 0xFF;
          }
        AlphaSet (&Colour, Alpha);
        BitmapSetPixel (TargetBitmap, x, y, Colour);
      }
  }

void RenderDrawLine (int Colour, int x1, int y1, int x2, int y2)
  {
    int dx, dy, sx, sy;
    int err, err2;
    //
    if (x1 == x2 && y1 == y2)   // Point
      RenderDrawPixel (x1, y1, Colour);
    else
      {
        if (x1 == x2)   // vertical line
          while (true)
            {
              RenderDrawPixel (x1, y1, Colour);
              if (y1 < y2)
                y1++;
              else if (y1 > y2)
                y1--;
              else
                break;
            }
        else if (y1 == y2)   // horizontal line
          while (true)
            {
              RenderDrawPixel (x1, y1, Colour);
              if (x1 < x2)
                x1++;
              else if (x1 > x2)
                x1--;
              else
                break;
            }
        else   // diagonal line - https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
          {
            dx =  Abs (x2 - x1);
            sx =  x1 < x2 ? 1 : -1;
            dy = -Abs (y2 - y1);
            sy = y1 < y2 ? 1 : -1;
            err = dx + dy;  // error value e_xy
            while (true)
              {
                RenderDrawPixel (x1, y1, Colour);
                if (x1 == x2 && y1 == y2)
                  break;
                err2 = err * 2;
                if (err2 >= dy)   // e_xy+e_x > 0
                  {
                    err += dy;
                    x1 += sx;
                  }
                if (err2 <= dx)   // e_xy+e_y < 0
                  {
                    err += dx;
                    y1 += sy;
                  }
              }
          }
      }
  }

void RenderFillRect (_Rect Rect, int Colour)
  {
    int y;
    //
    for (y = 0; y < Rect.Height; y++)
      RenderDrawLine (Colour, Rect.x, Rect.y + y, Rect.x + Rect.Width - 1, Rect.y + y);
  }

bool RenderBitmap (_Window *Window, _Bitmap *Bitmap, _Rect RecSource, _Point Dest, int ColTransparent, byte Alpha = 0xFF)   // same size
  {
    int x, y;
    int xd, yd;
    int Colour, Colour1, Colour2;
    byte Alpha1;
    //
    if (Alpha)   // Does this Bitmap have any influence
      for (x = 0; x < RecSource.Width; x++)
        for (y = 0; y < RecSource.Height; y++)
          {
            Colour1 = BitmapGetPixel (Bitmap, RecSource.x + x, RecSource.y + y);
            if (ColTransparent < 0 || ((Colour1 & 0xFFFFFF) != ColTransparent))
              {
                xd = x + Dest.x - TargetBitmapScroll.x;
                yd = y + Dest.y - TargetBitmapScroll.y;
                Alpha1 = AlphaGet (&Colour1);
                Colour2 = BitmapGetPixel (TargetBitmap, xd, yd);
                if (Alpha1)   // does this pixel have any influence
                  {
                    Colour = AlphaBlend (Colour1 & 0xFFFFFF, Colour2 & 0xFFFFFF, (int) Alpha1 * (int) Alpha / 0xFF);  //(int)  * (int) Alpha1 / 0xFF); //Alpha
                    AlphaSet (&Colour, Alpha1); //0xFF - (0xFF - Alpha1) * (0xFF - Alpha2) / 0xFF * Alpha / 0xFF);
                    BitmapSetPixel (TargetBitmap, xd, yd, Colour);
                  }
              }
          }
    return true;
  }

bool RenderBitmapResize (_Window *Window, _Bitmap *Bitmap, _Rect RecSource, _Rect RecDest, int ColTransparent, byte Alpha = 0xFF)   // resizing
  {
    _Bitmap *bm1, *bm2;
    int xs, ys, xd, yd;
    int a;
    bool get, put;
    int pixel;
    bool Res;
    //
    Res = false;
    bm1 = Bitmap;
    bm2 = NULL;
    if (Bitmap)
      if ((RecDest.Width > 0) && (RecDest.Height > 0))
        {
          if (RecSource.Width == RecDest.Width && RecSource.Height == RecDest.Height)   // Same size
            Res = RenderBitmap (Window, bm1, RecSource, {RecDest.x, RecDest.y}, ColTransparent, Alpha);
          else   // Bitmap needs resizing
            {
              // Generate Scaled Image
              bm2 = BitmapCreate (Window, RecDest.Width, RecDest.Height);
              xs = ys = 0;
              xd = yd = 0;
              get = put = true;
              while (true)
                {
                  if (get & put)
                    {
                      get = false;
                      pixel = BitmapGetPixel (bm1, RecSource.x + xs, RecSource.y + ys);
                    }
                  if (put)
                    {
                      put = false;
                      BitmapSetPixel (bm2, xd, yd, pixel);
                    }
                  a = (xs+1) * RecDest.Width - (xd+1) * RecSource.Width;
                  if (a <= 0)
                    {
                      xs++;
                      get = true;
                    }
                  if (a >= 0)
                    {
                      xd++;
                      put = true;
                    }
                  if (xs >= RecSource.Width && get)
                    {
                      get = put = true;
                      xs = 0;
                      xd = 0;
                      a = (ys+1) * RecDest.Height - (yd+1) * RecSource.Height;
                      if (a <= 0)
                        {
                          ys++;
                          get = true;
                        }
                      if (a >= 0)
                        {
                          yd++;
                          put = true;
                        }
                      if (ys >= RecSource.Height && get)
                        break;
                    }
                }
              // Now put on Target
              Res = RenderBitmap (Window, bm2, {0, 0, RecDest.Width, RecDest.Height}, {RecDest.x, RecDest.y}, ColTransparent, Alpha);
            }
          // Free stuff
          if (bm2)
            BitmapDestroy (bm2);
        }
    return Res;
  }


///////////////////////////////////////////////////////////////////////////////
//
// SUPPORT

char *TextCallBackCursor;
_Point CursorLocation;
char *TextCallBackSelected [2];
_Bitmap *TextCallBackBitmap;

int GetBitmapIndex (char Ch)
  {
    if (IsDigit (Ch))
      return Ch - '0';
    if (IsAlpha (Ch))
      return UpCase (Ch) - 'A' + 10;
    return -1;
  }

bool RenderBitmapTransparentTL (_Window *Window, _Bitmap *Bitmap, _Rect RecSource, _Rect RecDest, byte Alpha = 0xFF)
  {
    if (Bitmap)
      return RenderBitmapResize (Window, Bitmap, RecSource, RecDest, BitmapGetPixel (Bitmap, RecSource.x, RecSource.y) & 0xFFFFFF, Alpha);
    return false;
  }

int DrawTextBitmap (_Window *Window, int Index, int x, int y, int FontHeight)
  {
    int Res;
    int bw, bh;
    //
    Res = 0;
    if (TextCallBackBitmap)
      {
        BitmapGetSize (TextCallBackBitmap, &bw, &bh);
        if (bh * (Index + 1) <= bw)
          {
            if (Window)
              {
                if (FontHeight > bh)
                  y += (FontHeight - bh) / 2;
                RenderBitmapTransparentTL (Window, TextCallBackBitmap, {Index * bh, 0, bh, bh}, {x, y, bh, bh});
              }
            Res = bh;
          }
      }
    return Res;
  }

//int ColourTable [8] = {cBlack, cRed, cGreen, cRed|cGreen, cBlue, cBlue|cRed, cBlue|cGreen, cWhite};
int ColourTable [] = {cBlack, cRedDark, cGreenDark, cRedDark|cGreenDark, cBlueDark, cBlueDark|cRedDark, cBlueDark|cGreenDark, cWhite, cGray, cGrayDark};

bool LabelEditTextCallBack (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size)
  {
    int w;
    int bi;
    int bsz;
    bool Selected_;
    char c;
    //
    // Cursor?
    if (Posn)
      if (*Text == TextCallBackCursor)
        {
          w = Max (((Font->YAdvance + 7) / 15) * 2, 2);
          RenderFillRect ({Posn->x - w / 2, Posn->y, w, Font->YAdvance}, Font->Colour);
          CursorLocation = *Posn;
        }
    // Selection?
    if (Window && Posn)
      {
        if (TextCallBackSelected [0] == nullptr)
          Selected_ = false;
        else
          if (TextCallBackSelected [0] < TextCallBackSelected [1])
            Selected_ = (*Text >= TextCallBackSelected [0] && *Text < TextCallBackSelected [1]);
          else
            Selected_ = (*Text >= TextCallBackSelected [1] && *Text < TextCallBackSelected [0]);
        if (Selected_)
          Font->ColourBG = cSelected;
        else
          Font->ColourBG = -1;
      }
    // Bitmap command
    if (**Text == '\e')   // escape => draw Bitmap
      {
        (*Text)++;
        c = **Text;
        (*Text)++;
        bi = GetBitmapIndex (c);
        if (bi >= 0)
          {
            if (Window && Posn)
              Posn->x += DrawTextBitmap (Window, bi, Posn->x, Posn->y, Font->YAdvance);
            else if (Size)
              {
                bsz = DrawTextBitmap (0, bi, 0, 0, 0);
                Size->x += bsz;
                Size->y = Max (Size->y, bsz);
              }
          }
        return true;
      }
    // Font / Colour altering command
    if (**Text == '\a')   // ^G => Alter Text: Change Colour, Bold, Italic, Underline ...
      {
        (*Text)++;
        c = UpCase (**Text);
        (*Text)++;
        if (c >= '0' && c <= '9')   // Change colour
          Font->Colour = ColourTable [c - '0'];  //was ^=
        else if (c == 'B')
          Font->Style ^= fsBold;
        else if (c == 'I')
          Font->Style ^= fsItalic;
        else if (c == 'U')
          Font->Style ^= fsUnderline;
        else if (c == 'O')
          Font->Style ^= fsOutline;
        return true;
      }
    if (**Text == '\v')   // split command
      {
        (*Text)++;
        return true;
      }
    return false;
  }


///////////////////////////////////////////////////////////////////////////////
//
// FORM

void _Form::CreateContainer (char *Title)
  {
    int Width, Height;
    //
    if (!Container)
      if (Window)
        {
          WindowGetPosSize (Window, NULL, NULL, &Width, &Height);
          Container = new _Container (NULL, {0, 0, Width, Height}, Title);
          Container->Form = this;
          Container->Colour = cForm1;
          Container->ColourGrad = cForm2;
          Container->Invalidate (true);
        }
  }

void _Form::Clear (void)
  {
    char *Title;
    //
    Title = NULL;
    StrAssignCopy (&Title, Container->TextGet ());
    Focused = NULL;
    delete Container;
    Container = NULL;
    CreateContainer (Title);
    StrAssignCopy (&Title, NULL);
  }

_Form::_Form (char *Title, _Rect Pos, byte WindowAttributes_)  // : _List (&FormList)
  {
    DebugAddS ("Creating Form ", Title); //####
    KeySeries = NULL;   // Soft Keyboard / Macro input
    EventPreview = NULL;
    Alpha = 0xFF;
    if (WindowCount++ == 0)
      {
        WidgetsInit ();
        TextCallBack = &LabelEditTextCallBack;
        StrAssign (&FontFileDefault, ResourcePathAdd ("ARI.ttf"));
      }
    Container = NULL;
    Focused = NULL;
    Window = WindowCreate (Title, Pos.x, Pos.y, Pos.Width, Pos.Height, WindowAttributes_);
    CreateContainer (Title);
    WindowAttributes = WindowAttributes_;
    Image = NULL; //BitmapCreate (Window, Pos.Width, Pos.Height);
    EventMouseLock = NULL;
    EventLock = NULL;
    RedrawZone [0] = {IntMax, IntMax};
    RedrawZone [1] = {-1, -1};
    ListAdd (this, &FormList);
    DieFlag = false;
    FloatError = 0.0001;   // Accuracy on sqrt/trig functions
  }

_Form::~_Form (void)
  {
    char s [64], *sp;
    //
    DebugAddS ("Deleting Form ", Container->TextGet ()); //####
    Focused = NULL;
    EventMouseLock = NULL;
    EventLock = NULL;
    delete Container;
    if (Image)
      BitmapDestroy (Image);
    WindowDestroy (Window);
    ListRemove (this, &FormList);   // Remove this from FormList
    if (--WindowCount == 0)
      {
        StrAssign (&FontFileDefault, NULL);
        WidgetsUninit ();
        sp = s;
        StrCat (&sp, "Containers: ");
        NumToStr (&sp, CreateCount, 0);
        StrCat (&sp, " Created, ");
        NumToStr (&sp, DelCount, 0);
        StrCat (&sp, " Deleted");
        *sp = 0;
        DebugAdd (s);
      }
    MouseKeys = 0x0000;
  }

inline void ZoneUp (int &x, int Val)
  {
    if (Val > x)
      x = Val;
  }

inline void ZoneDown (int &x, int Val)
  {
    if (Val < x)
      x = Val;
  }

void _Form::RedrawZoneAdd (_Container *Container, _Point Offset)
  {
    int x_, y_;
    //
    x_ = Container->Rect.x + Offset.x;
    y_ = Container->Rect.y + Offset.y;
    ZoneDown (RedrawZone [0].x, x_);
    ZoneDown (RedrawZone [0].y, y_);
    ZoneUp   (RedrawZone [1].x, x_ + Container->Rect.Width - 1);
    ZoneUp   (RedrawZone [1].y, y_ + Container->Rect.Height - 1);
  }

void _Form::Die (void)
  {
    DieFlag = true;
  }

//_Point Mouse;

bool ProcessEvent (void)   // Looks for and processes events for all Forms. Return TRUE if anything happened
  {
    bool Busy;
    _Form *f;
    _Event Event;
    //
    Busy = false;
    memset (&Event, 0, sizeof (Event));
    // Look for internal scheduled events
    f = FormList;
    while (f)
      {
        if (f->DieFlag)
          {
            delete f;
            return false;
          }
        if (f->KeySeries && *f->KeySeries)
          {
            Event.Type = etKeyDown;
            Event.Key = *f->KeySeries++;
            break;
          }
        f = (_Form *) f->Next;
      }
    // Look for external events
    if (f == NULL)
      {
        EventPoll (&Event);
        if (Event.Type != etNone)
          {
            f = FormList;
            while (f)
              {
                if (WindowIDMatch (Event.WindowID, f->Window))
                  break;
                f = (_Form *) f->Next;
              }
          }
      }
    if (f)
      {
        Busy = true;
        if (Event.Type == etMouseDown || Event.Type == etMouseUp)   // Update Mouse Keys
          if (Event.Key > 0 && Event.Key <= 16)
            {
              if (Event.Key != KeyMouseWheelUp && Event.Key != KeyMouseWheelDown)
                if (Event.Type == etMouseDown)
                  MouseKeys |= Bit [Event.Key - 1];
                else
                  MouseKeys &= ~Bit [Event.Key - 1];
              //#define DebugShowMouseKeys
              #ifdef DebugShowMouseKeys
              char s [32], *cp;
              cp = s;
              StrCat (&cp, "Mouse Keys: ");
              NumToStrBase (&cp, MouseKeys, 0, 2);
              *cp = 0;
              DebugAdd (s);
              #endif // DebugShowMouseKeys
            }
        if (f->EventPreview && f->EventPreview (f, &Event))
          ;
        else if (Event.Type == etWindowRedraw)
          f->Container->Invalidate (false);
        else if (Event.Type == etWindowResize)
          {
            if (f->Container->Resize (Event.X, Event.Y))
              {
                BitmapDestroy (f->Image);
                f->Image = NULL;
              }
          }
        else if (Event.Type == etWindowClose)
          {
            if (f == FormList)   // First window
              while (FormList)   // Remove all windows
                delete (FormList);
            else
              delete f;
            Busy = false;   // don't come back
          }
        else if (Event.Type != etNone)
          {
            Event.MouseKeys = MouseKeys;
            do
              f->Container->ProcessEvent (&Event, {0, 0});   // Give event to all components
            while (Event.Type == etNext);
          }
        if (Event.Type == etMouseUp)   // Release the Mouse Lock on Mouse Up
          f->EventMouseLock = NULL;
      }
    return Busy;
  }

_Rect MakeRect (_Point p1, _Point p2)
  {
    _Rect Res;
    //
    Res.x = Min (p1.x, p2.x);
    Res.y = Min (p1.y, p2.y);
    Res.Width = Abs (p1.x - p2.x) + 1;
    Res.Height = Abs (p1.y - p2.y) + 1;
    return Res;
  }

bool FormsUpdate (void)   // Looks for events, processes everything, returns true for close
  {
    _Form *f;
    _Rect RecUpdate;
    //
    do {} while (ProcessEvent ());
    // Perform Scheduled stuff
    f = FormList;
    while (f)
      {
        f->Container->Poll ();
        f->Container->Draw (false, {0, 0});   // Draw containers with ReDraw set
        if (f->ReRender)
          {
            f->ReRender = false;
            if (f->Container->Image)
              {
                if (f->Image == NULL)
                  f->Image = BitmapCreate (f->Window, f->Container->Rect.Width, f->Container->Rect.Height);
                TargetBitmap = f->Image;
                TargetBitmapSize = {f->Container->Rect.Width, f->Container->Rect.Height};
                f->Container->RenderContainers (false, {0, 0}, 0xFF);
                RecUpdate = MakeRect (f->RedrawZone [0], f->RedrawZone [1]);
                //RenderPresent (f->Window, f->Image, {0, 0, f->Container->Rect.Width, f->Container->Rect.Height}, {0, 0}, f->Alpha);
                ZoneDown (f->RedrawZone [1].x, f->Container->Rect.Width);
                ZoneDown (f->RedrawZone [1].y, f->Container->Rect.Height);
                //DebugAdd ("Render Present");
                //BitmapSave (f->Window, f->Image, "WinRender.bmp");
                if (f->RedrawZone [1].x >= 0 && f->RedrawZone [1].y >= 0)   // somthing to render
                  RenderPresent (f->Window, f->Image, RecUpdate, f->RedrawZone [0], f->Alpha);
                f->RedrawZone [0] = {IntMax, IntMax};
                f->RedrawZone [1] = {-1, -1};
              }
          }
        f = f->Next;
      }
    return FormList == NULL;
  }


///////////////////////////////////////////////////////////////////////////////
//
// CONTAINER

_Container::_Container (_Container *Parent_, _Rect Rect_, char *Text_, int PagingTime_, int PagingTransitionTime_, _PagingTransitionMode PagingTransitionMode_)
  {
    Next = NULL;
    Prev = NULL;
    CreateCount++;
    Visible = true;
    Enabled = true;
    if (Parent_)
      if (Parent_->PagingTime)   // We are to be paged
        Visible = false;   // So start invisible
    RenderFlags = 0;
    Form = NULL;
    Parent = NULL;
    Image = NULL;
    Colour = -1;   // Transparent
    ColourGrad = -1;
    ColourText = -1;
    Text = NULL;
    Bitmap = NULL;
    //EventPreview = NULL;
    StrAssignCopy (&Text, Text_);
    Alpha = 0xFF;
    Shift = {0, 0};
    Clip = {0, 0};
    PagingTime = PagingTime_;
    PagingTick = ClockMS ();
    PagingChild = NULL;
    PagingTransitionMode = PagingTransitionMode_;
    PagingTransitionTime = PagingTransitionTime_;
    PagingTransitionState = ptsIdle;
    //
    Children = NULL;
    Font = NULL;   // use ancestor
    RectLock = 0x00;
    if (Parent_)
      {
        if (Rect_.x < 0)
          {
            Rect_.x += Parent_->Rect.Width;
            RectLock |= rlLeft | rlRight;
          }
        if (Rect_.y < 0)
          {
            Rect_.y += Parent_->Rect.Height;
            RectLock |= rlTop | rlBottom;
          }
        if (Rect_.x + Rect_.Width > Parent_->Rect.Width)
          Rect_.Width = 0;
        if (Rect_.y + Rect_.Height> Parent_->Rect.Height)
          Rect_.Height = 0;
        if (Rect_.Width <= 0)// || Rect_.x + Rect_.Width > Parent_->Rect.Width)
          {
            Rect_.Width += Parent_->Rect.Width - Rect_.x;
            RectLock |= rlRight;
          }
        if (Rect_.Height <= 0)// || Rect_.y + Rect_.Height > Parent_->Rect.Height)
          {
            Rect_.Height += Parent_->Rect.Height - Rect_.y;
            RectLock |= rlBottom;
          }
        Parent = Parent_;
        Form = Parent_->Form;   // Every Container knows the Form
        ListAdd (this, &Parent_->Children);   // Add self to parent's children
      }
    Rect = Rect_;
    SizeVirtual = {0, 0};
    Scroll = {0, 0};
    DataContainer = NULL;
    DataInt = 0;
    Invalidate (true);
  }

_Container::~_Container (void)
  {
    DelCount++;
    // Unfocus and unlock
    if (Form)
      {
        if (Form->Focused == this)
          Form->Focused = NULL;
        if (Form->EventMouseLock == this)
          Form->EventMouseLock = NULL;
        if (Form->EventLock == this)
          Form->EventLock = NULL;
      }
    // remove children
    while (Children)
      delete Children;
    free (Text);
    Text = NULL;
    if (Bitmap)
      BitmapDestroy (Bitmap);
    Bitmap = NULL;
    if (Font)
      FontDestroy (Font);
    Font = NULL;
    // remove self from parent
    if (Parent)
      ListRemove (this, &Parent->Children);
    // Schedule Parent redisplay, now with a missing tooth
    if (Parent)
      Parent->Invalidate (false);
    // Delete Texture (if used)
    if (Image)
      BitmapDestroy (Image);
    Image = NULL;
  }

bool _Container::Resize (int Width, int Height)
  {
    int dX, dY;
    _Container *Child;
    int Width_, Height_;
    byte rl;
    //
    if (Width == Rect.Width && Height == Rect.Height)
      return false;
    //DebugAddP ("Resizing", {Width, Height});//####
    if (Image)   // Delete Texture (if used)
      BitmapDestroy (Image);
    Image = NULL;
    dX = Width - Rect.Width;
    dY = Height- Rect.Height;
    Rect.Width = Width;
    Rect.Height = Height;
    //Scroll.x = Limit (Scroll.x, 0, SizeVirtual.x - Rect.Width);
    if (Rect.Width >= SizeVirtual.x)
      Scroll.x = 0;//SizeVirtual.x - Rect.Width;
    if (Rect.Height >= SizeVirtual.y)
      Scroll.y = 0;
    Invalidate (true);   // Redraw
    //
    Child = Children;
    while (Child)
      {
        rl = Child->RectLock;
        if (rl & rlTop)
          rl &= ~rlBottom;
        if (rl & rlLeft)
          rl &= ~rlRight;
        if (rl & rlTop)
          Child->Rect.y = Child->Rect.y + dY; //Limit (Child->Rect.y + dY, 0, Rect.Height);
        if (rl & rlLeft)
          Child->Rect.x = Child->Rect.x + dX;//Limit (Child->Rect.x + dX, 0, Rect.Width);
        if (rl & (rlBottom | rlRight))   // Child is resized
          {
            Width_ = Child->Rect.Width;
            if (rl & rlRight)
              Width_ += dX;
            Height_ = Child->Rect.Height;
            if (rl & rlBottom)
              Height_ += dY;
            Child->Resize (Width_, Height_);
          }
        Child = Child->Next;
      }
    return true;   // Signal any double buffering must be resized
  }

char* _Container::TextGet (void)   // Get the Container's Text
  {
    return Text;
  }

void _Container::TextSet (char *St)   // Set the Container's Text
  {
    StrAssignCopy (&Text, St);
    Invalidate (true);
  }

void _Container::Invalidate (bool ReDraw, bool ReDrawPart)
  {
    _Container *c;
    //
    // Set Rendering flags as needed
    if (ReDraw)
      RenderFlags |= rRedraw;
    if (ReDrawPart)
      RenderFlags |= rRedrawPart;
    RenderFlags |= rRender | rInvalid;
    c = this;   // Rerender ancestors until we find a "solid"
    while (true)
      {
        if (c == NULL)
          break;
        if (c->Colour >= 0 && c->Alpha == 0xFF)   // c has a solid background
          break;   // so parent needs not be fully rerendered (just the children render bits)
        c->RenderFlags |= rRender;
        c = c->Parent;
      }
    if (IsVisible ())
      if (Form)
        Form->ReRender = true;
  }

void _Container::InvalidateAll (bool ReDraw)
  {
    _Container *Child;
    //
    Invalidate (ReDraw);
    Child = Children;
    while (Child)
      {
        Child->InvalidateAll (ReDraw);
        Child = Child->Next;
      }
  }

void _Container::EnabledSet (bool Enabled_)
  {
    if (Enabled_ != Enabled)
      {
        Enabled = Enabled_;
        if (!Enabled)
          {
            if (Form->EventMouseLock == this)
              Form->EventMouseLock = NULL;
            if (Form->EventLock == this)
              Form->EventLock = NULL;
          }
        InvalidateAll (true);
      }
  }

void _Container::RenderContainers (bool RenderThis, _Point Dest, byte AlphaOffset)
  {
    _Point Dest_;
    _Rect RecSource;
    _Rect RecSource_;
    _Container *Child;
    //
    if (IsVisible () && Alpha > 0 && Rect.Width > 0 && Rect.Height > 0)
      {
        // Work out the Source & Destination regions
        RecSource.x = 0;
        RecSource.y = 0;
        RecSource.Width = Rect.Width - Shift.x - Clip.x;
        RecSource.Height = Rect.Height - Shift.y - Clip.y;
        Dest_.x = Dest.x + Rect.x + Shift.x;
        Dest_.y = Dest.y + Rect.y + Shift.y;
        // Adjust Alpha
        AlphaOffset = (AlphaOffset * Alpha) / 0xFF;
        if (RenderFlags & rRender)
          RenderThis = true;
        if (RecSource.Width > 0 && RecSource.Height > 0)
          {
            if (RenderThis)
              {
                Form->RedrawZoneAdd (this, Dest);//{Dest.x, Dest.y, Rect.Width, Rect.Height});
                RenderBitmap (Form->Window, Image, RecSource, Dest_, -1, AlphaOffset);
                Child = Children;
                while (Child)
                  {
                    //if (Child->Rect.x < Rect.Width && Child->Rect.y < Rect.Height)
                      Child->RenderContainers (true, {Dest.x + Rect.x, Dest.y + Rect.y}, AlphaOffset);
                    Child = Child->Next;
                  }
              }
            else   // search down tree for render descendants
              {
                // Draw children backgrounds if needed
                Child = Children;
                while (Child)
                  {
                    //if (Child->Rect.x < Rect.Width && Child->Rect.y < Rect.Height)
                      if (((Child->RenderFlags & rRender) && Child->Visible && Child->Colour < 0) || Child->RenderFlags & rClear)// && Child->Colour < 0))// || !Child->Visible)
                        {
                          RecSource_ = {RecSource.x + Child->Rect.x, RecSource.y + Child->Rect.y, Child->Rect.Width, Child->Rect.Height};
                          RenderBitmap (Form->Window, Image, RecSource_, {Dest_.x + Child->Rect.x, Dest_.y + Child->Rect.y}, -1, Child->Alpha);
                          Form->RedrawZoneAdd (Child, {RecSource.x, RecSource.y});//RecSource_);
                          Child->RenderFlags &= ~rClear;
                        }
                    Child = Child->Next;
                  }
                Child = Children;
                // Now Render children
                while (Child)
                  {
                    //if (Child->Rect.x < Rect.Width && Child->Rect.y < Rect.Height)
                      Child->RenderContainers (RenderThis, Dest_, AlphaOffset);
                    Child = Child->Next;
                  }
              }
          }
      }
    RenderFlags &= ~(rRender | rClear);
  }

void _Container::Die (void)
  {
    RenderFlags |= rDelete;
  }

void _Container::DrawBackground (_Rect Rect)
  {
    if (Colour >= 0)   // Colour defined
      if (ColourGrad < 0)   // Solid Colour
        DrawRectangle (Rect, -1, -1, Colour);
      else
        FillGradient (Rect, Colour, ColourGrad);
    else   // no colour defined. Transparent
      BitmapClear (Image);
  }

void _Container::Draw (bool Force, _Point Offset)
  {
    _Container *Child;
    _Font *Font_;
    //
    // Is this even visible
    if (IsVisible () && Rect.Width > 0 && Rect.Height > 0)
      {
        if ((RenderFlags & (rRedraw | rRedrawPart)) || Force)
          {
            //DebugAddS ("Draw Text", Text);//####
            //DebugAddR ("Draw Rect", Rect);//####
            if (Image == NULL)
              Image = BitmapCreate (Form->Window, Rect.Width, Rect.Height);
            TargetBitmap = Image;
            TargetBitmapSize = {Rect.Width, Rect.Height};
            // Draw the background
            if (RenderFlags & rRedraw)
              DrawBackground ({0, 0, Rect.Width, Rect.Height});
            // Draw stuff specific to this type of Container
            TargetBitmapScroll = Scroll;
            Font_ = FontFind ();
            if (Font_)
              Font_->Colour = -1; //ColourTextFind ();
            DrawCustom ();
            TargetBitmapScroll = {0, 0};
            ScrollRegionBarsDraw ();
          }
        // Draw Children
        Child = Children;
        while (Child)
          {
            //if (Child->FitsParent () || Texture)
              Child->Draw (Force, {Offset.x + Child->Rect.x, Offset.y + Child->Rect.y});
            Child = Child->Next;
          }
        RenderFlags &= ~(rRedraw | rRedrawPart);
      }
  }

void _Container::DrawCustom (void)
  {
    if (DebugDisplayContainers)
      DrawRectangle ({0, 0, Rect.Width, Rect.Height}, cRed, cRed, -1);
  }

bool _Container::FontSet (char *FontFile, int Size, byte Style)
  {
    _Font *Font_;
    char *s;
    //
    s = NULL;
    Font_ = NULL;
    if (FontFile && *FontFile)   // Font specified
      {
        s = ResourcePathAdd (FontFile);
        Font_ = FontCreate (s, Size, Style);
        if (Font_)
          if (FontFileDefault == NULL)
            StrAssignCopy (&FontFileDefault, s);   // Set Default Font
      }
    if (!Font_)   // it failed, try default
      Font_ = FontCreate (FontFileDefault, Size, Style);
    if (Font_)   // it worked
      {
        if (Font)   // Free old Font
          FontDestroy (Font);
        Font = Font_;
      }
    if (!Font)
      DebugAddS ("FontSet FAIL: ", FontFile);
    if (s)
      free (s);
    return Font != NULL;
  }

bool _Container::FocusCheck (_Event *Event, _Point Offset)   // Take or lose keyboard Focus
  {
    _Container *c;
    bool Res;
    //
    Res = false;
    // Take focus if next in line
    if (Event->Type == etNext)
      {
        Event->Type = etNone;
        Form->Focused = this;
        Invalidate (true);
        Res = true;
      }
    if (!Form->EventMouseLock)
      {
        // Take focus if clicked on
        if (IsEventMine (Event, Offset))
          if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)
            if (Form->Focused != this)
              {
                c = Form->Focused;
                Form->Focused = this;
                if (c)
                  c->Invalidate (true);
                Invalidate (true);
                Res = true;
              }
        // Lose focus if tab pressed
        if (Event->Type == etKeyDown)
          if (Form->Focused == this)
            if (Event->Key == tab)
              {
                Event->Type = etNext;
                Form->Focused = NULL;
                Invalidate (true);
                Res = true;
              }
      }
    return Res;
  }

void _Container::FocusSet (void)   // Give Focus to a _Container
  {
    _Container *c;
    //
    c = Form->Focused;
    Form->Focused = this;
    if (c)
      c->Invalidate (true);
    Invalidate (true);
  }

void DebugShowMouseEvent (_Event *Event, _Container *Container)
  {
    char s [80], *ps;
    //
    if (Event->Type >= etMouseDown && Event->Type <= etMouseMove)
      {
        ps = s;
        StrCat (&ps, "Event ");
        NumToStr (&ps, Event->Type);
        StrCat (&ps, " at ");
        PointToStr (&ps, {Event->X, Event->Y});
        StrCat (&ps, " on thing at ");
        RectToStr (&ps, Container->Rect);
        *ps = 0;
        DebugAdd (s);
      }
  }

bool _Container::ProcessEvent (_Event *Event, _Point Offset)
  {
    _Container *Child;
    _Point ScrollOld;
    bool Res;
    //
    Res = false;
    if (!IsVisible () || !Enabled)
      return false;
    // Check the Children
    Child = Children;
    while (Child)
      {
        if (Child->ProcessEvent (Event, {Offset.x + Child->Rect.x, Offset.y + Child->Rect.y}))
          return true;
        Child = Child->Next;
      }
    ScrollOld = Scroll;
    if (ScrollRegionBarsEvent (Event, Offset))
      Res = true;
    else
      Res = ProcessEventCustom (Event, Offset);
    if (Scroll.x < 0)
      Scroll.x = 0;
    if (Scroll.y < 0)
      Scroll.y = 0;
    if (Scroll.x != ScrollOld.x || Scroll.y != ScrollOld.y)
      Invalidate (true);
    if (Res)
      if (Event->Type == etMouseDown)   // Lock to this Container on Mouse Down
        Form->EventMouseLock = this;
    return Res;
  }

bool _Container::ProcessEventCustom (_Event *Event, _Point Offset)   // Implemented by descendants
  {
    return false;
  }

bool _Container::IsVisible (void)
  {
    _Container *c;
    //
    c = this;
    while (true)
      {
        if (!c ->Visible)
          return false;
        if (c->Alpha == 0)
          return false;
        if (c->Parent == NULL)
          return true;
        c = c->Parent;
      }
  }

void _Container::VisibleSet (bool Value)
  {
    Visible = Value;
    if (!Value)
      RenderFlags |= rClear;
    Invalidate (false);
  }

bool _Container::IsEnabled (void)
  {
    _Container *c;
    //
    c = this;
    while (true)
      {
        if (!c ->Enabled)
          return false;
        if (c->Parent == NULL)
          return Enabled;
        c = c->Parent;
      }
  }

void _Container::Poll (void)
  {
    _Container *Child, *Child_;
    //
    // Check the Children
    Child = Children;
    while (Child)
      {
        Child->Poll ();
        Child_ = Child->Next;
        if (Child->RenderFlags & rDelete)
          delete Child;
        Child = Child_;
      }
    // Now yourself
    PollCustom ();
  }

void _Container::PollCustom (void)
  {
    int Tick;
    _Container *PagingChildNew;
    int a;
    //
    if (PagingTime && Children)
      {
        Tick = ClockMS ();
        if (PagingChild == NULL)
          PagingChild = Children;
        PagingChildNew = PagingChild->Next;
        if (PagingChildNew == NULL)
          PagingChildNew = Children;
        if (PagingChild != PagingChildNew)
          if (PagingTransitionState == ptsIdle)
            {
              if ((int) (Tick - PagingTick) >= 0)   // Time to change
                if (PagingTransitionTime == 0)   // no transitions, just flip over
                  {
                    PagingChild->VisibleSet (false);
                    PagingChildNew->VisibleSet (true);
                    PagingChild = PagingChildNew;
                    PagingTick += PagingTime;
                  }
                else   // Transition specified
                  {
                    if (PagingTransitionMode == ptmBlend)
                      PagingChildNew->Alpha = 0;
                    else if (PagingTransitionMode == ptmDrop)
                      PagingChildNew->Clip.y = Rect.Height;
                    else if (PagingTransitionMode == ptmSlide)
                      PagingChildNew->Clip.x = Rect.Width;
                    PagingChildNew->VisibleSet (true);
                    PagingTransitionState = ptsBlending;
                    Invalidate (false);
                  }
            }
          else if (PagingTransitionState == ptsBlending)
            {
              if ((int) (Tick - PagingTick) >= PagingTransitionTime)   // Transition finished
                {
                  PagingChild->VisibleSet (false);
                  PagingChild->Shift = {0, 0};
                  PagingChild = PagingChildNew;
                  PagingChild->Alpha = 0xFF;
                  PagingChild->Clip = {0, 0};
                  PagingTransitionState = ptsIdle;
                  PagingTick += PagingTime;
                  Invalidate (false);
                }
              else
                {
                  if (PagingTransitionMode == ptmBlend)
                    {
                      a = ((Tick - PagingTick) * 0xFF) / PagingTransitionTime;   // Portion of transition ito Alpha
                      PagingChild->Alpha = 0xFF - a;
                      PagingChildNew->Alpha = a;
                    }
                  else if (PagingTransitionMode == ptmDrop)
                    {
                      //a = ((Tick - PagingTick) * Rect.Height) / PagingTransitionTime;   // Portion of transition ito Height
                      a = Rect.Height * Sqr (Tick - PagingTick) / Sqr (PagingTransitionTime);   // Physics: d ~ t^2
                      PagingChild->Shift.y = a;
                      PagingChildNew->Clip.y = Rect.Height - a;
                    }
                  else if (PagingTransitionMode == ptmSlide)
                    {
                      a = ((Tick - PagingTick) * Rect.Width) / PagingTransitionTime;   // Portion of transition ito Width
                      PagingChild->Shift.x = a;
                      PagingChildNew->Clip.x = Rect.Width - a;
                    }
                  Invalidate (false);
                }
            }
      }
  }


///////////////////////////////////////////////////////////////////////////////
//
// COMPONENT RESOURCES

bool _Container::IsEventWithin (_Event *Event, _Point Offset)
  {
    if (Event->X >= Offset.x)
      if (Event->X <= Offset.x + Rect.Width)
        if (Event->Y >= Offset.y)
          if (Event->Y <= Offset.y + Rect.Height)
            return true;
    return false;
  }

bool _Container::IsEventMine (_Event *Event, _Point Offset)
  {
    if (IsVisible () && IsEnabled ())   // might be redundant
      {
        if (Event->Type == etKeyDown || Event->Type == etKeyUp)
          return Form->Focused == this;
        if (Form->EventLock)
          return Form->EventLock == this;
        if (Form->EventMouseLock)
          return Form->EventMouseLock == this;
        if ((Event->Type >= etMouseDown && Event->Type <= etMouseMove) && IsEventWithin (Event, Offset))
          return true;
      }
    return false;
  }

int _Container::ColourFind (void)
  {
    _Container *c;
    //
    c = this;
    while (true)
      {
        if (c->Colour >= 0)
          if (c->ColourGrad < 0)
            return c->Colour;
          else
            return ColourAverage (c->Colour, c->ColourGrad);
        if (c->Parent == NULL)
          return cForm1;
        c = c->Parent;
      }
  }

int _Container::ColourTextFind (void)
  {
    _Container *c;
    //
    c = this;
    while (true)
      {
        if (c->ColourText >= 0)
          return c->ColourText;
        if (c->Parent == NULL)
          return cText;
        c = c->Parent;
      }
  }

_Font* _Container::FontFind (void)
  {
    _Container *c;
    //
    c = this;
    while (true)
      {
        if (c->Font)
          return c->Font;
        if (c->Parent == NULL)
          return NULL;   // Just in case
        c = c->Parent;
      }
  }

_Bitmap* _Container::BitmapFind (void)
  {
    _Container *c;
    //
    c = this;
    while (true)
      {
        if (c->Bitmap)
          return c->Bitmap;
        if (c->Parent == NULL)
          return NULL;
        c = c->Parent;
      }
  }

_Rect AddMargin (_Rect Rect, _Point Margin)
  {
    return {Rect.x + Margin.x, Rect.y + Margin.y, Rect.Width - 2 * Margin.x, Rect.Height - 2 * Margin.y};
  }

_Point _Container::TextMeasure (char *Text)
  {
    _Point Size;
    _Font *Font_, Font__;
    //
    Font_ = FontFind ();
    if (Font_ == NULL)
      Size = {0, 0};
    else
      {
        Font__ = *Font_;
        TextCallBackBitmap = BitmapFind ();
        Size = TextSize (&Font__, Text);
      }
    return Size;
  }

extern int puts (char *St);

void _Container::TextOutAligned (_Rect Rect_, char *Text, _Align Align, _Align AlignVert)
  {
    static int ColourTextOld = -1;
    int Col;
    _Font *Font_;
    _Point Size;
    int dX, dY;
    char *Split;
    //
    if (Text == NULL)
      return;
    Font_ = FontFind ();
    if (Font_ == NULL)
      return;
    // Change colour if requested, otherwise keep current colour
    Col = ColourTextFind ();
    if (Col != ColourTextOld || Font_->Colour < 0)
      Font_->Colour = Col;
    ColourTextOld = Col;
    TextCallBackBitmap = BitmapFind ();
    if (!IsEnabled ())
      Font_->Colour = ColourAverage (Font_->Colour, ColourFind ());
    Split = StrPos (Text, '\v');   // vertical tab => Split line, left & right justified
    if (Split)
      {
        *Split = 0;
        TextOutAligned ({Rect_.x, Rect_.y, Rect_.Width, Rect_.Height}, Text, aLeft, AlignVert);
        TextOutAligned ({Rect_.x, Rect_.y, Rect_.Width, Rect_.Height}, Split + 1, aRight, AlignVert);
        *Split = '\v';
      }
    else
      {
        Size = TextMeasure (Text);
        if (Align != aLeft)
          {
            dX = Rect_.Width - Size.x;
            if (Align == aRight)
              Rect_.x += dX;
            else if (Align == aCenter)
              Rect_.x += dX / 2;
          }
        dY = Rect_.Height - Font_->YAdvance;
        if (AlignVert == aCenter)
          Rect_.y += dY / 2;
        else if (AlignVert == aRight)
          Rect_.y += dY;
        if (Font_->Style & fsRotate)
          TextRender (Form->Window, Font_, Text, {Rect_.x, Rect_.y + Size.x - 1});
        else if (Rect_.y - Scroll.y < Rect.Height && Rect_.y - Scroll.y + Font_->YAdvance > 0)
          TextRender (Form->Window, Font_, Text, {Rect_.x, Rect_.y});
      }
  }

// Word wrap text out.  !Write => measure only
// PosY is starting & finishing position

int StrBlank (char *St, int Index, int Dir)
  {
    char c;
    //
    while (true)
      {
        if (Index < 0)
          return -1;
        c = St [Index];
        if (c == ' ' || c == '\0')
          return Index;
        Index += Dir;
      }
  }

void _Container::TextOutAlignedN (_Rect Rect_, char *St, _Align Align, int n)
  {
    char *St_;
    //
    St_ = StrMake (St, n);
    TextOutAligned (Rect_, St_, Align, aTop);
    free (St_);
    /*
    char c;
    //
    c = St [n];
    St [n] = 0;  DOESN'T LIKE THIS !!!!!!! %%%%
    TextOutAligned (Rect_, St, Align, aTop);
    if (c)
      St [n] = c;
    */
  }

void _Container::TextOutWrap_ (_Rect Rect_, char *St, _Align Align, int *PosY, bool Write)
  {
    _Font *Font_;
    int x, a;
    //
    if (St == NULL)
      return;
    Font_ = FontFind ();
    while (true)
      {
        if (*St == '\0')
          break;
        x = TextIndex (Font_, St, Rect_.Width);
        if (St [x] == ' ' || St [x] == '\n' || St [x] == '\0')
          a = x;
        else
          {
            a = StrBlank (St, x, -1);
            if (a < 0)
              a = StrBlank (St, x, +1);
          }
        if (Write)
          //if ((*PosY - Scroll.y) + Font_->YAdvance >= Rect_.y && (*PosY - Scroll.y) < Rect_.y + Rect_.Height)
            TextOutAlignedN ({Rect_.x, *PosY, Rect_.Width, 99999}, St, Align, a);
        if (St [a] == ' ' || St [a] == '\n')
          a++;
        St = &St [a];
        *PosY += Font_->YAdvance;
      }
  }

/*
void _Container::TextOutWrap_ (_Rect Rect_, char *St, _Align Align, int *PosY, bool Write)
  {
    int BOL, BOW, EOW, x;
    char *StSub;   // Copy of St
    _Point Size;
    char *Split;
    char c;
    //
    if (St)
      {
        StSub = NULL;
        StrAssignCopy (&StSub, St);
        BOL = 0;
        BOW = 0;
        while (St [BOL])
          {
            EOW = BOW;
            while (true)
              {
                c = St [EOW];
                if (c == 0 || c == ' ' || c == '\n')   // End St, Word Brk, Line Brk
                  break;
                EOW++;
              }
            memcpy (StSub, St, EOW);
            StSub [EOW] = 0;//####
            Size = TextMeasure (&StSub [BOL]);
            x = 0;
            if (Size.x > Rect_.Width)   // too wide
              if (BOW == BOL)   // Only 1 word this line?
                x = EOW;   // So show it all
              else
                x = BOW - 1;   // otherwise Show words that fit
            else   // not too wide
              if (St [EOW] != ' ')   // but out of string OR Specified End of Line
                x = EOW;   // so finish off
            if (x || (St [EOW] != ' '))   // display a line
              {
                StSub [x] = 0;
                if (Write)// && *PosY < Rect_.Height)
                  {
                    Split = StrPos (&StSub [BOL], '\v');   // vertical tab => Split line, left & right justified
                    if (Split)
                      {
                        *Split = 0;
                        TextOutAligned ({Rect_.x, *PosY, Rect_.Width, Size.y}, &StSub [BOL], aLeft, aLeft);
                        TextOutAligned ({Rect_.x, *PosY, Rect_.Width, Size.y}, Split + 1, aRight, aLeft);
                      }
                    else
                      TextOutAligned ({Rect_.x, *PosY, Rect_.Width, Size.y}, &StSub [BOL], Align, aLeft);
                  }
                if (St [x])   // Step past Break char
                  x++;
                BOL = x;
                BOW = x;
                *PosY += Size.y;
              }
            else
              {
                if (St [EOW])
                  EOW++;
                BOW = EOW;
              }
          }
        free (StSub);
      }
  }
*/

// Text out wrapping at words. Force wrap at \n. Split line full justify at \t
int _Container::TextOutWrap (_Rect Rect, char *Text, _Align Align, _Align AlignVert)
  {
    _Font *Font_;
    _Point Size;
    int x, y, gapX, gapY;
    //
    Font_ = FontFind ();
    if (Font_)
      {
        if (Font_->Style & fsRotate)
          {
            Size = TextMeasure (Text);
            gapX = (Rect.Width - Size.y);
            gapY = (Rect.Height - Size.x);
            x = Rect.x;
            y = Rect.y;
            if (Align == aCenter)
              y += gapY / 2;
            else if (Align == aLeft)
              y += gapY;
            if (AlignVert == aCenter)
              x += gapX / 2;
            else if (AlignVert == aRight)
              x += gapX;
            TextOutAligned ({x, y, 0, 0}, Text, aLeft, aLeft);
          }
        else
          {
            y = Rect.y;
            if (AlignVert == aLeft)   // Top
              TextOutWrap_ (Rect, Text, Align, &y, true);
            else
              {
                y = 0;
                //Font__ = *Font_;   // save
                TextOutWrap_ (Rect, Text, Align, &y, false);
                //*Font_ = Font__;   // restore
                if (AlignVert == aCenter)
                  y = Rect.y + (Rect.Height - y) / 2;
                else   // Bottom
                  y = Rect.y + Rect.Height - y;
                TextOutWrap_ (Rect, Text, Align, &y, true);
              }
          }
      }
    return y;
  }

void _Container::TextOutInverse (_Rect Rect, char *St, int Margin)
  {
    int cText;
    _Font *Font_;
    int Colour;
    _Point Sz;
    //
    cText = ColourTextFind ();
    DrawRectangle ({0, Rect.y, Rect.Width, Rect.Height}, -1, -1, cText);
    Font_ = FontFind ();
    if (Font_)
      {
        Sz = TextMeasure (St);
        Colour = Font_->Colour;
        Font_->Colour = cText ^ cWhite;   // inverted colour
        TextRender (Form->Window, Font_, St, {Rect.x + Margin, Rect.y + (Rect.Height - Sz.y) / 2});
        //TextOutAligned (AddMargin (Rect, {Margin, 0}), St, aLeft, aCenter);
        Font_->Colour = Colour;   // Restore Colour
      }
  }

void _Container::DrawLine (int x1, int y1, int x2, int y2, int Colour, int Width)
  {
    int i;
    //
    if (Colour >= 0)   // non-transparent
      {
        for (i = 0; i < Width; i++)
          if (i == 0)
            RenderDrawLine (Colour, x1, y1, x2, y2);
          else
            {
              RenderDrawLine (Colour, x1 + i, y1, x2 + i, y2);
              RenderDrawLine (Colour, x1 - i, y1, x2 - i, y2);
              RenderDrawLine (Colour, x1, y1 + i, x2, y2 + i);
              RenderDrawLine (Colour, x1, y1 - i, x2, y2 - i);
            }
      }
  }

void _Container::DrawRectangle (_Rect Rect_, int ColourEdgeTL, int ColourEdgeBR, int ColourFill)
  {
    _Point BR;
    //
    if (ColourFill >= 0)
      RenderFillRect (Rect_, ColourFill);
    BR = BottomRight (&Rect_);
    if (ColourEdgeTL >= 0)
      {
        if (Rect_.Height)
          RenderDrawLine (ColourEdgeTL, Rect_.x, BR.y, Rect_.x, Rect_.y);
        if (Rect_.Width)
          RenderDrawLine (ColourEdgeTL, Rect_.x, Rect_.y, BR.x, Rect_.y);
      }
    if (ColourEdgeBR >= 0)
      {
        if (Rect_.Height)
          RenderDrawLine (ColourEdgeBR, BR.x, Rect_.y, BR.x, BR.y);
        if (Rect_.Width)
          RenderDrawLine (ColourEdgeBR, BR.x, BR.y, Rect_.x, BR.y);
      }
  }

int _Container::BorderColourTL (int ColourReference)
  {
    return ColourAdjust (ColourReference, 180);
  }

int _Container::BorderColourBR (int ColourReference)
  {
    return ColourAdjust (ColourReference, 50);
  }

void _Container::DrawBorder (_Rect Rect, _Border Border, int ColourReference, int Indent)
  {
    int ColTL, ColBR;
    _Point p1, p2;
    _Point tbs;
    //
    //
    tbs = TargetBitmapScroll;
    TargetBitmapScroll = {0};
    ColTL = BorderColourTL (ColourReference);
    ColBR = BorderColourBR (ColourReference);
    if (Border != bNone)
      if (Border == bMote)
        {
          DrawBorder (Rect, bLowered, ColourReference, 0);
          DrawBorder (Rect, bRaised, ColourReference, 1);
        }
      else
        {
          if (Border == bLowered)
            Swap (ColTL, ColBR);
          if (Rect.Width == 0)
            {
              DrawLine (Rect.x, Rect.y, Rect.x, Rect.y + Rect.Height, ColBR);
              DrawLine (Rect.x+1, Rect.y, Rect.x+1, Rect.y + Rect.Height, ColTL);
            }
          else if (Rect.Height == 0)
            {
              DrawLine (Rect.x, Rect.y, Rect.x + Rect.Width, Rect.y, ColBR);
              DrawLine (Rect.x, Rect.y+1, Rect.x + Rect.Width, Rect.y+1, ColTL);
            }
          else
            {
              p1 = {Rect.x + Indent, Rect.y + Indent};
              p2 = {Rect.x + Rect.Width - 1 - Indent, Rect.y + Rect.Height - 1 - Indent};
              DrawLine (p1.x, p1.y, p2.x , p1.y, ColTL);   // Top
              DrawLine (p2.x, p1.y, p2.x, p2.y, ColBR);   // Right
              DrawLine (p1.x, p2.y, p2.x, p2.y, ColBR);   // Bottom
              DrawLine (p1.x, p1.y, p1.x, p2.y, ColTL);   // Left
            }
        }
    TargetBitmapScroll = tbs;
  }

void _Container::DrawBorder (_Border Border, int Indent)
  {
    int Colour;
    //
    Colour = ColourFind ();
    DrawBorder ({0, 0, Rect.Width, Rect.Height}, Border, Colour, Indent);
  }

void _Container::DrawBezel (int Width)
  {
    int i, c;
    _Point tbs;
    //
    tbs = TargetBitmapScroll;
    TargetBitmapScroll = {0, 0};
    for (i = 0; i < Width; i++)
      DrawBorder (bLowered, i);
    if (Form->Focused == this)
      {
        c = ColourAdjust (ColourTextFind (), 150);
        DrawRectangle ({i, i, Rect.Width - i - i, Rect.Height - i - i}, c, c, -1);
        //i++;
        //DrawRectangle ({i, i, Rect.Width - i - i, Rect.Height - i - i}, c, c, -1);
      }
    TargetBitmapScroll = tbs;
  }

#define TabCorners 2

void _Container::DrawTab (_Rect Rec, int ColourReference, int ColourBG, bool Selected)
  {
    _Point BR;
    int ColTL, ColBR;
    int i;
    //
    BR = BottomRight (&Rec);
    ColTL = BorderColourTL (ColourReference);
    ColBR = BorderColourBR (ColourReference);
    if (ButtonStyle == bsNormal)
      {
        // Straights
        for (i = 0; i < 2; i++)
          {
            DrawLine (Rec.x + i, BR.y - i, Rec.x + i, Rec.y + i, ColTL);   // Left
            DrawLine (Rec.x + i, Rec.y + i, BR.x - i, Rec.y + i, ColTL);   // Top
            DrawLine (BR.x - i, Rec.y + i, BR.x - i, BR.y - i, ColBR);   // Right
          }
        // Corners
        #if TabCorners == 1
        DrawLine (Rec.x, Rec.y, Rec.x, Rec.y, ColourBG);
        DrawLine (BR.x, Rec.y, BR.x, Rec.y, ColourBG);
        DrawLine (Rec.x + 2, Rec.y + 2, Rec.x + 2, Rec.y + 2, ColTL);
        #elif TabCorners == 2
        DrawLine (Rec.x, Rec.y + 1, Rec.x, Rec.y, ColourBG);
        DrawLine (Rec.x, Rec.y, Rec.x + 1, Rec.y, ColourBG);
        DrawLine (BR.x, Rec.y + 1, BR.x, Rec.y, ColourBG);
        DrawLine (BR.x, Rec.y, BR.x - 1, Rec.y, ColourBG);
        DrawLine (Rec.x + 2, Rec.y + 2, Rec.x + 2, Rec.y + 2, ColTL);
        #elif TabCorners == 3
        DrawLine (Rec.x, Rec.y + 2, Rec.x, Rec.y, ColourBG);
        DrawLine (Rec.x, Rec.y, Rec.x + 2, Rec.y, ColourBG);
        DrawLine (BR.x, Rec.y + 2, BR.x, Rec.y, ColourBG);
        DrawLine (BR.x, Rec.y, BR.x - 2, Rec.y, ColourBG);
        DrawLine (Rec.x + 2, Rec.y + 2, Rec.x + 2, Rec.y + 2, ColTL);
        #endif
      }
    else   // bsFlat OR bsSimple
      {
        //ColBR = ColourAdjust (ColourTextFind (), 140);
        if ((ButtonStyle == bsFlat && Rec.x == 0) || (ButtonStyle == bsSimple && Selected))
          DrawLine (Rec.x, BR.y, Rec.x, Rec.y, ColBR);   // Left
        if (ButtonStyle == bsFlat || Selected)
          {
            DrawLine (Rec.x, Rec.y, BR.x, Rec.y, ColBR);   // Top
            DrawLine (BR.x, Rec.y, BR.x, BR.y, ColBR);   // Right
          }
      }
  }

/*
void _Container::DrawCursor (int x, int y, int Height)
  {
    int Col;
    //
    if (Form->KeyFocus == this)
      {
        Col = ColourTextFind ();
        DrawRectangle ({x, y, Max (1, Height / 7), Height}, -1, -1, Col);
      }
  }

// Draw Circle: Center (cx, cy), optionally filled, with each 1/8 masked by Octant bits:
//  2 1
// 3   0
// 4   7
//  5 6
void _Container::DrawCircle (int cx, int cy, int radius, int ColourEdge, int ColourFill, byte Octant, int Width)
  {
    float r;
    int dx, dxOld, dy, dyOld;
    //
    r = (double) radius;
    if (ColourFill >= 0)
      for (dy = 0; dy <= r; dy++)
        {
          //dx = floor (sqrt (r * r - dy * dy));
          dx = Sqrt ((float) (r * r - dy * dy)) + 0.5;
          DrawLine (Form->Window, ColourFill, cx - dx, cy - dy, cx + dx, cy - dy);
          if (dy)
            DrawLine (Form->Window, ColourFill, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    if (ColourEdge >= 0)
      {
        dxOld = r;
        dyOld = 0;
        for (dy = 0; dy <= r; dy++)
          {
            //dx = floor (sqrt (r * r - dy * dy) + 0.5);
            dx = (float) Sqrt (r * r - dy * dy) + 0.5;
            if (dy < dx && Octant & Bit [3] || dy >= dx && Octant & Bit [2])
              DrawLine (cx - dxOld, cy - dyOld, cx - dx, cy - dy, ColourEdge, Width);   // Top Left
            if (dy < dx && Octant & Bit [0] || dy >= dx && Octant & Bit [1])
              DrawLine (cx + dxOld, cy - dyOld, cx + dx, cy - dy, ColourEdge, Width);   // Top Right
            if (dy < dx && Octant & Bit [4] || dy >= dx && Octant & Bit [5])
              DrawLine (cx - dxOld, cy + dyOld, cx - dx, cy + dy, ColourEdge, Width);   // Bottom Left
            if (dy < dx && Octant & Bit [7] || dy >= dx && Octant & Bit [6])
              DrawLine (cx + dxOld, cy + dyOld, cx + dx, cy + dy, ColourEdge, Width);   // Bottom Right
            dxOld = dx;
            dyOld = dy;
          }
      }
  }
*/

const float SinCosPi_4 = 0.70710678;   // 1 / Sqrt (2)

// Calculates the colour of an angled bevel
int BevelColour (int dx, int dy, int Rad, int Colour, int Pcnt)
  {
    float SinA, CosA, SinA_;
    //
    // Bevel colouring is proportional to Sin (Angle - pi/4)
    SinA = - (float) dy / (float) Rad;   // Screen co-ordinates use reverse y
    CosA = (float) dx / (float) Rad;
    // Subtract pi/4 from angle: Sin (a-b) = Sin A * Cos B - Cos A * Sin B
    // Sin pi/4 = Cos pi/4 = 1 / Sqrt (2)
    SinA_ = SinA * SinCosPi_4 - CosA * SinCosPi_4;
    // Adjust Colour proportionalely
    return ColourAdjust (Colour, 100 + SinA_ * (float) Pcnt);
  }

void _Container::DrawCircle (int cx, int cy, int radius, int ColourEdge, int ColourFill, int Width, int Bevel)
  {
    float r;
    int dx, dxOld, dy, dyOld;
    int c;
    //
    r = (double) radius;
    dxOld = r;
    dyOld = 0;
    for (dy = 0; dy <= r; dy++)
      {
        //dx = floor (sqrt (r * r - dy * dy));
        dx = Sqrt ((float) (r * r - dy * dy)) + 0.5;
        if (ColourFill >= 0)
          {
            DrawLine (cx - dx, cy - dy, cx + dx, cy - dy, ColourFill);
            if (dy)
              DrawLine (cx - dx, cy + dy, cx + dx, cy + dy, ColourFill);
          }
        if (ColourEdge >= 0)
          {
            if (Bevel)
              {
                c = BevelColour (-dx, -dy, radius, ColourEdge, Bevel);
                DrawLine (cx - dxOld, cy - dyOld, cx - dx, cy - dy, c, Width);   // Top Left
                c = BevelColour (+dx, -dy, radius, ColourEdge, Bevel);
                DrawLine (cx + dxOld, cy - dyOld, cx + dx, cy - dy, c, Width);   // Top Right
                c = BevelColour (-dx, +dy, radius, ColourEdge, Bevel);
                DrawLine (cx - dxOld, cy + dyOld, cx - dx, cy + dy, c, Width);   // Bottom Left
                c = BevelColour (+dx, +dy, radius, ColourEdge, Bevel);
                DrawLine (cx + dxOld, cy + dyOld, cx + dx, cy + dy, c, Width);   // Bottom Right
              }
            else
              {
                DrawLine (cx - dxOld, cy - dyOld, cx - dx, cy - dy, ColourEdge, Width);   // Top Left
                DrawLine (cx + dxOld, cy - dyOld, cx + dx, cy - dy, ColourEdge, Width);   // Top Right
                DrawLine (cx - dxOld, cy + dyOld, cx - dx, cy + dy, ColourEdge, Width);   // Bottom Left
                DrawLine (cx + dxOld, cy + dyOld, cx + dx, cy + dy, ColourEdge, Width);   // Bottom Right
              }
            dxOld = dx;
            dyOld = dy;
          }
      }
  }

void _Container::DrawRadius (int x, int y, int Rad1, int Rad2, int Angle, int Colour, int Width)
  {
    float sin, cos;
    //
    sin = SinDeg (Angle);
    cos = CosDeg (Angle);
    DrawLine (x + Round (cos * Rad1), y - Round (sin * Rad1), x + Round (cos * Rad2), y - Round (sin * Rad2), Colour, Width);
  }

void _Container::DrawArrow (_Rect Rect_, _Direction Direction)   // Arrow: Direction: Left, Up, Right, Down
  {
    int x, y;
    int a, i;
    int dx, dy, Col;
    //
    Col = ColourTextFind ();
    if (Direction == dLeft)
      {
        a = Rect_.Height / 2;
        x = Rect_.x + (Rect_.Width - a) / 2;
        y = Rect_.y + a;
        dx = 1;
        dy = 0;
      }
    else if (Direction == dRight)
      {
        a = Rect_.Height / 2;
        x = Rect_.x + (Rect_.Width - a) / 2 + a;
        y = Rect_.y + a;
        dx = -1;
        dy = 0;
      }
    else if (Direction == dUp)
      {
        a = Rect_.Width / 2;
        x = Rect_.x + a;
        y = Rect_.y + (Rect_.Height - a) / 2;
        dx = 0;
        dy = 1;
      }
    else //if (Direction == dDown)
      {
        a = Rect_.Width / 2;
        x = Rect_.x + a;
        y = Rect_.y + (Rect_.Height - a) / 2 + a;
        dx = 0;
        dy = -1;
      }
    for (i = 0; i <= a; i++)
      {
        if (dx)
          DrawLine (x, y - i, x, y + i, Col);
        else
          DrawLine (x - i, y, x + i, y, Col);
        x += dx;
        y += dy;
      }
  }

void _Container::DrawKnerl (_Rect Rect, bool Vert)
  {
    int Col [3];   // ColBG, ColBorder, ColBar
    int i;
    //
    Col [2] = Parent->ColourFind ();
    Col [0] = ColourAdjust (Col [2], 150);
    Col [1] = ColourAdjust (Col [2], 60);
    i = 0;
    while (true)
      {
        if (Vert)
          DrawLine (Rect.x + i, Rect.y, Rect.x + i, Rect.y + Rect.Height, Col [i % 3], 1);
        else
          DrawLine (Rect.x, Rect.y + i, Rect.x + Rect.Width, Rect.y + i, Col [i % 3], 1);
        i++;
        if (i >= (Vert ? Rect.Width : Rect.Height))
          break;
      }
  }

void _Container::DrawScrollBar (bool Vert, int Bar1, int Bar2)
  {
    int Col [3];   // ColBG, ColBorder, ColBar
    int i, j;
    //
    Col [0] = ColourAdjust (cForm1, 150);   // ColBG
    Col [1] = ColourAdjust (cForm1, 60);   // ColBorder
    Col [2] = ColourAdjust (cForm1, 90);   // ColBar
    //
    if (Vert)
      {
        DrawRectangle ({Rect.Width - ScrollBarWidth, 0, ScrollBarWidth, Rect.Height - ScrollBarWidth}, -1, -1, Col [0]);   // Background
        DrawRectangle ({Rect.Width - ScrollBarWidth, Bar1, ScrollBarWidth, Bar2 - Bar1 + 1}, Col [1], Col [1], Col [2]);   // Bar
      }
    else
      {
        DrawRectangle ({0, Rect.Height - ScrollBarWidth, Rect.Width - ScrollBarWidth, ScrollBarWidth}, -1, -1, Col [0]);   // Background
        DrawRectangle ({Bar1, Rect.Height - ScrollBarWidth, Bar2 - Bar1 + 1, ScrollBarWidth}, Col [1], Col [1], Col [2]);   // Bar
      }
    // knerl
    j = (Bar1 + Bar2) / 2 - 3;
    if (Bar2 - Bar1 > 8)
      for (i = 0; i < 8; i++)
        if (Vert)
          DrawLine (Rect.Width - ScrollBarWidth + 3, j + i, Rect.Width - 4, j + i, Col [i % 3], 1);
        else
          DrawLine (j + i, Rect.Height - ScrollBarWidth + 3, j + i, Rect.Height - 4, Col [i % 3], 1);
    // corner
    DrawRectangle ({Rect.Width - ScrollBarWidth, Rect.Height - ScrollBarWidth, ScrollBarWidth, ScrollBarWidth}, Col [1], Col [1], Col [2]); //ColourFind ());
  }

void _Container::ScrollBarsDraw (byte Bars, _Point Value, _Point Content)
  {
    longint Bar1, Bar2;
    //
    if ((Bars & sbHorz) && Content.x > 1)
      {
        Bar1 = Value.x * (Rect.Width - 3 * ScrollBarWidth) / (Content.x - 1);
        Bar2 = Bar1 + 2 * ScrollBarWidth - 1;
        DrawScrollBar (false, Bar1, Bar2);
      }
    if ((Bars & sbVert) && Content.y > 1)
      {
        Bar1 = Value.y * (Rect.Height - 3 * ScrollBarWidth) / (Content.y - 1);
        Bar2 = Bar1 + 2 * ScrollBarWidth - 1;
        DrawScrollBar (true, Bar1, Bar2);
      }
  }

bool _Container::ScrollBarsEvent (_Event *Event, _Point Offset, byte Bars, _Point Content, _Point *Scroll)
  {
    static _Point Drag = {-1, -1};
    static _Point DragVal;
    bool Res;
    _Point ScrollOld;
    //
    Res = false;
    //BP Event->Type != etMouseMove
    if (IsEventMine (Event, Offset))
      {
        if ((Drag.x >= 0 || Drag.y >= 0) && Event->MouseKeys == 0)
          Drag = {-1, -1};
        ScrollOld = *Scroll;
        if (Bars & sbHorz)   // we will have a horizontal scroll bar
          {
            if (Event->Type == etMouseMove && Event->MouseKeys == Bit [KeyMouseLeft - 1] && Drag.x >= 0)   // Drag with left mouse button
              {
                Scroll->x = DragVal.x + (Content.x - 1) * (Event->X - Drag.x) / (Rect.Width - 3 * ScrollBarWidth);
                Res = true;
              }
            else if (Event->Type == etMouseDown)
              if (Event->Y - Offset.y >= Rect.Height - ScrollBarWidth)
                {
                  Res = true;
                  if (Event->Key == KeyMouseLeft)   // Click is on the hor scroll bar
                    {
                      Res = true;
                      Drag.x = Event->X;
                      DragVal.x = Scroll->x;
                    }
                  else if (Event->Key == KeyMouseWheelDown)
                    Scroll->x ++;
                  else if (Event->Key == KeyMouseWheelUp)
                    Scroll->x --;
                }
          }
        if (!Res && Bars & sbVert)   // we will have a vertical scroll bar
          {
            if (Event->Type == etMouseMove && Event->MouseKeys == Bit [KeyMouseLeft - 1] && Drag.y >= 0)   // Drag with left mouse button
              {
                Scroll->y = DragVal.y + (Content.y - 1) * (Event->Y - Drag.y) / (Rect.Height - 3 * ScrollBarWidth);
                Res = true;
              }
            else if (Event->Type == etMouseDown)
              if (Event->Key == KeyMouseLeft && Event->X - Offset.x >= Rect.Width - ScrollBarWidth)   // Click is on the vert scroll bar
                {
                  Res = true;
                  Drag.y = Event->Y;
                  DragVal.y = Scroll->y;
                }
              else
                {
                  Res = true;
                  if (Event->Key == KeyMouseWheelDown)
                    Scroll->y ++;
                  else if (Event->Key == KeyMouseWheelUp)
                    Scroll->y --;
                  else
                    Res = false;
                }
          }
        if (Scroll->x != ScrollOld.x || Scroll->y != ScrollOld.y)
          Invalidate (true);
      }
    return Res;
  }

/*
bool _Container::ScrollBarClick (_Event *Event, _Point Offset, int Val1, int Val2, int *Value)
  {
    static int Drag = -1;
    static int DragVal;
    //
    if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft && IsEventMine (Event, Offset))
      if (Event->X - Offset.x >= Rect.Width - ScrollBarWidth)   // Click on Scroll
        {
          Drag = Event->Y;
          if (Value)
            DragVal = *Value;
          Form->EventLock = this;
          return true;
        }
    if (Event->Type == etMouseMove && Event->MouseKeys == Bit [KeyMouseLeft - 1] && Drag >= 0)   // Drag
      {
        if (Value)
          *Value = DragVal + (Val2 - Val1) * (Event->Y - Drag) / Rect.Height;
        return true;
      }
    else if (Event->Type == etMouseUp && Event->Key == KeyMouseLeft && Drag >= 0)
      {
        Drag = -1;
        Form->EventLock = NULL;
        return true;
      }
    return false;
  }

bool _Container::ScrollBarHorizontalClick (_Event *Event, _Point Offset, int Val1, int Val2, int *Value)
  {
    static int Drag = -1;
    static int DragVal;
    //
    if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft && Event->Y - Offset.y >= Rect.Height - ScrollBarWidth)   // Click on Scroll
      {
        Drag = Event->X;
        if (Value)
          DragVal = *Value;
        Form->EventLock = this;
        return true;
      }
    if (Event->Type == etMouseMove && Event->MouseKeys == Bit [KeyMouseLeft - 1] && Drag >= 0)   // Drag
      {
        if (Value)
          *Value = DragVal + (Val2 - Val1) * (Event->X - Drag) / Rect.Width;
        return true;
      }
    else if (Event->Type == etMouseUp && Event->Key == KeyMouseLeft && Drag >= 0)
      {
        Drag = -1;
        Form->EventLock = NULL;
        return true;
      }
    return false;
  }
*/

// Scroll Region

void DebugShowScroll (_Container *Container)
  {
    char s [80], *ps;
    //
    ps = s;
    StrCat (&ps, "Scroll: ");
    PointToStr (&ps, Container->Scroll);
    *ps = 0;
    DebugAdd (s);
  }

bool _Container::ScrollRegionBarsEvent (_Event *Event, _Point Offset)
  {
    const int MouseWheelDistance = 64;
    static _Point Drag = {-1, -1};
    static _Point DragVal;
    bool Res;
    _Point ScrollOld;
    //
    Res = false;
    //BP Event->Type != etMouseMove
    if (IsEventMine (Event, Offset))
      {
        if ((Drag.x >= 0 || Drag.y >= 0) && Event->MouseKeys == 0)
          Drag = {-1, -1};
        ScrollOld = Scroll;
        if (SizeVirtual.x > Rect.Width)   // we will have a horizontal scroll bar
          {
            Res = true;
            if (Event->Type == etMouseMove && Event->MouseKeys == Bit [KeyMouseLeft - 1] && Drag.x >= 0)   // Drag with left mouse button
              Scroll.x = DragVal.x + (longint) (Event->X - Drag.x) * (longint) SizeVirtual.x / (longint) Rect.Width;
            else
              if (Event->Y - Offset.y >= Rect.Height - ScrollBarWidth)   // Click is potentially on the hor scroll bar
                if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)   // My left click
                  {
                    Drag.x = Event->X;
                    DragVal.x = Scroll.x;
                  }
                else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelDown)
                  Scroll.x += 16;
                else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelUp)
                  Scroll.x -= 16;
                else
                  Res = false;
              else
                Res = false;
          }
        if (!Res && SizeVirtual.y > Rect.Height)   // we will have a vertical scroll bar
          {
            Res = true;
            if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelDown)
              Scroll.y += MouseWheelDistance;
            else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelUp)
              Scroll.y -= MouseWheelDistance;
            else if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft &&   // My left click
                Event->X - Offset.x >= Rect.Width - ScrollBarWidth)   // Click is on the vert scroll bar
              {
                Drag.y = Event->Y;
                DragVal.y = Scroll.y;
              }
            else if (Event->Type == etMouseMove && Event->MouseKeys == Bit [KeyMouseLeft - 1] && Drag.y >= 0)   // Drag with left mouse button
              Scroll.y = DragVal.y + (longint) (Event->Y - Drag.y) * (longint) SizeVirtual.y / (longint) Rect.Height;
            else
              Res = false;
          }
        if (Res)
          {
            //DebugAddP ("ScrollBars: Scroll", Scroll);//####
            Scroll.y = Limit (Scroll.y, 0, SizeVirtual.y - Rect.Height + ScrollBarWidth);
            Scroll.x = Limit (Scroll.x, 0, SizeVirtual.x - Rect.Width + ScrollBarWidth);
            //if (Scroll.x != ScrollOld.x || Scroll.y != ScrollOld.y)
            //  Invalidate (true);
          }
      }
    return Res;
  }

bool _Container::ScrollRegionBarsDraw (void)
  {
    bool Res;
    int Bar1, Bar2;
    int h;
    //_Point tbs;
    //
    //tbs = TargetBitmapScroll;
    TargetBitmapScroll = {0, 0};
    Res = false;
    if (SizeVirtual.y > Rect.Height)
      {
        Res = true;
        h = Rect.Height - ScrollBarWidth;
        Bar1 = (longint) h * (longint) Scroll.y / (longint) SizeVirtual.y;
        Bar2 = (longint) h * (longint) h / (longint) SizeVirtual.y;
        //DebugAddP ("  DrawScrollBar: Bars", {Bar1, Bar2});
        DrawScrollBar (true, Bar1, Bar1 + Bar2);
      }
    if (SizeVirtual.x > Rect.Width)
      {
        Res = true;
        h = Rect.Width - ScrollBarWidth;
        Bar1 = h * Scroll.x / SizeVirtual.x;
        Bar2 = h * h / SizeVirtual.x;
        DrawScrollBar (false, Bar1, Bar1 + Bar2);
      }
    //TargetBitmapScroll = tbs;
    return Res;
  }

void _Container::FillGradient (_Rect Rect, int Colour1, int Colour2)
  {
    int x2, y;
    //
    if (Colour1 < 0)
      Colour1 = cBlack;
    if (Colour2 < 0)
      Colour2 = Colour1;
    x2 = Rect.x + Rect.Width - 1;
    for (y = Rect.y; y < Rect.y + Rect.Height; y++)
      DrawLine (Rect.x, y, x2, y, ColourGraded (Colour1, Colour2, y, _Container::Rect.Height));
  }

void _Container::DrawBitmap (_Bitmap *Bitmap, _Rect rSrc, _Rect rDest, bool Transparent)
  {
    if (Transparent)
      RenderBitmapTransparentTL (Form->Window, Bitmap, rSrc, rDest);
    else
      RenderBitmapResize (Form->Window, Bitmap, rSrc, rDest, -1);
  }

/*
void _Container::FillGradientRound (int Colour1, int Colour2, int ColourEdge)
  {
    int Edge;
    int x1, x2;
    int y;
    int r, delta;
    int R, G, B;
    int Col;
    _Rect Rect_;
    //
    Edge = Rect.Height / 30;
    if (Edge == 0)
      Edge = 1;
    Rect_ = {0, 0, Rect.Width, Rect.Height};
    if (Colour2 < 0)
      Colour2 = Colour1;
    for (y = 0; y < Rect_.Height; y++)
      {
        delta = 0;
        r = Rect.Height / 4;
        if (y < r)
          delta = r - (sqrt (sqr (r) - sqr (r - y)) + 0.5);  // r - y;
        else if (y > Rect.Height - r)
          delta = r - (sqrt (sqr (r) - sqr (r - (Rect_.Height - y))) + 0.5); // y - Rect.Height + r;
        x1 = delta;
        x2 = Rect.Width - 1 - delta;
        if (ColourEdge >= 0)
          DrawLine (Form->Window, ColourEdge, Rect_.x + x1, Rect_.y + y, Rect_.x + x2, Rect_.y + y);
        if (Colour1)
          if ((y >= Edge) && (y < Rect_.Height - Edge))
            {
              x1 += Edge;
              x2 -= Edge;
              R = Grad (ColourR (Colour1), ColourR (Colour2), y, Rect.Height);
              G = Grad (ColourG (Colour1), ColourG (Colour2), y, Rect.Height);
              B = Grad (ColourB (Colour1), ColourB (Colour2), y, Rect.Height);
              Col = RGBToColour (R,G, B);
              DrawLine (Form->Window, Col, Rect_.x + x1, Rect_.y + y, Rect_.x + x2, Rect_.y + y);
            }
      }
  }
*/


///////////////////////////////////////////////////////////////////////////////
//
// COMPONENTS


#define TextMargin 4


///////////////////////////////////////////////////////////////////////////////
//
// BUTTON

_Button::_Button (_Container *Parent, _Rect Rect, char *Text_, _Action Action) : _Container (Parent, Rect, Text_)
  {
    Colour = cButton1;
    ColourGrad = cButton2;
    //Alpha = 64;//%%%%
    if (ButtonStyle == bsSimple)
      Colour = -1;
    Align = aCenter;
    Toggle = false;
    Grouped = false;
    Down = false;
    Flag = false;
    Hover = false;
    _Button::Action = Action;
  }

_Border ButtonDownToBorder (bool Down)
  {
    if (Down)
      return bLowered;
    return bRaised;
  }

void _Button::DrawCustom (void)
  {
    int cT, cBG, cFill, cLine;
    int TextOffset;
    //
    cT = ColourTextFind ();
    cBG = ColourFind ();
    TextOffset = 0;
    if (Down)
      TextOffset = 1;
    if (ButtonStyle == bsNormal)
      {
        DrawBorder (ButtonDownToBorder (Down), 0);
        DrawBorder (ButtonDownToBorder (Down), 1);
        //DrawBorder (ButtonDownToBorder (Down), 2);
      }
    else if (ButtonStyle == bsFlat)
      {
        cFill = -1;
        if (Down)
          cFill = ColourAdjust (ColourFind (), 80);
        cLine = ColourAdjust (cT, 140);
        if (Down || ButtonStyle == bsFlat)
          DrawRectangle ({0, 0, Rect.Width, Rect.Height}, cLine, cLine, cFill);
        if (Hover)
          DrawRectangle ({1, 1, Rect.Width - 2, Rect.Height - 2}, cLine, cLine, -1);
      }
    else if (ButtonStyle == bsSimple)
      {
        cLine = cFill = -1;
        if (Down)
          cFill = ColourAdjust (cBG, 80);
        if (Hover || Down)
          cLine = ColourAdjust (cT, 140);
        DrawRectangle ({0, 0, Rect.Width, Rect.Height}, cLine, cLine, cFill);
      }
    TextOutWrap ({TextMargin + TextOffset, TextOffset, Rect.Width - 2 * TextMargin, Rect.Height}, Text, Align, aCenter);
  }

bool _Button::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    bool DownOld, HoverOld;;
    _Container *Sibling;
    _Button *SiblingButton;
    //
    Res = false;
    if (IsEventMine (Event, Offset))   // Either by mouse location OR Locked to me
      {
        DownOld = Down;
        HoverOld = Hover;
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)   // Left Mouse Down
          {
            Res = true;
            Flag = Down;
            Down = true;
            if (Grouped)   // Check for grouped
              if (Parent && Parent->Children)
                {
                  Sibling = Parent->Children;
                  while (Sibling)
                    {
                      if (Sibling != this)
                        {
                          SiblingButton = dynamic_cast <_Button *> (Sibling);
                          if (SiblingButton)
                            if (SiblingButton->Down && SiblingButton->Grouped)
                              {
                                SiblingButton->Down = false;
                                SiblingButton->Flag = false;
                                SiblingButton->Invalidate (true);
                                if (SiblingButton->Action)
                                  SiblingButton->Action (Sibling);
                              }
                        }
                      Sibling = Sibling->Next;
                    }
                }
          }
        else if (Event->Type == etMouseUp && Event->Key == KeyMouseLeft)   // Left Mouse Up
          {
            Res = true;
            if (Toggle)
              Down = !Flag;
            else   // not Toggle
              Down = Grouped;
          }
        //else if (Event->Type == etMouseMove && ButtonStyle != bsNormal)
          if (IsEventWithin (Event, Offset))   // Still over the button
            {
              Hover = true;
              Form->EventLock = this;
            }
          else   // but moved off the button
            {
              Hover = false;
              Form->EventLock = NULL;
              if (!Toggle && !Grouped)   // ie non toggle switch
                Down = DownOld = false;   // so cancel the action
            }
        if (Hover != HoverOld)
          Invalidate (true);
        if (Down != DownOld)   // Changed so Redraw and Action
          {
            Res = true;
            Invalidate (true);
            if (!Down || Toggle || Grouped)
              if (Action)   // Call Handler
                Action (this);
          }
      }
    return Res;
  }


///////////////////////////////////////////////////////////////////////////////
//
// BUTTON RADIO

_ButtonRadio::_ButtonRadio (_Container *Parent, _Rect Rect, char *Text_, _Action Action) : _Button (Parent, Rect, Text_, Action)
  {
    Align = aLeft;
    Grouped = true;
    Colour = -1;
    ColourGrad = -1;
  }

void _ButtonRadio::DrawCustom (void)
  {
    int r;   // Radius
    int cText, cCircle;   // Colours
    _Point Size;
    int xCircle, xText;
    //
    cText = ColourTextFind ();
    Size = TextMeasure ("Ag");
    cCircle = ColourAdjust (cText, 140);
    r = Size.y * 3 / 8;
    xText = 3 * r;
    xCircle = r;
    if (Align == aRight)
      {
        xText = 0;
        xCircle = Rect.Width - r - 1;
      }
    TextOutWrap ({xText, 0, Rect.Width - 3 * r, Rect.Height}, Text, Align, aCenter);
    DrawCircle (xCircle, Rect.Height / 2, r, cCircle, -1);
    if (Hover)
      DrawCircle (xCircle, Rect.Height / 2, r - 1, cCircle, -1);
    if (Down)
      DrawCircle (xCircle, Rect.Height / 2, r * 2 / 4, -1, cText);
  }


///////////////////////////////////////////////////////////////////////////////
//
// CHECK BOX

_CheckBox::_CheckBox (_Container *Parent, _Rect Rect, char *Text_, _Action Action) : _Button (Parent, Rect, Text_, Action)
  {
    StyleCross = false;
    Align = aLeft;
    Toggle = true;
    Colour = -1;
    ColourGrad = -1;
  }

void _CheckBox::DrawCustom (void)
  {
    int h;
    int x0, y0, xText;
    int cText, cBox;
    _Point Size;
    //
    cText = ColourTextFind ();
    cBox = ColourAdjust (cText, 140);
    Size = TextMeasure ("Ag");
    h = Size.y * 3 / 8;
    x0 = 0;
    y0 = Rect.Height / 2 - h;
    xText = 3 * h;
    if (Align == aRight)
      {
        x0 = Rect.Width - 2 * h;
        xText = 0;
      }
    TextOutWrap ({xText, 0, Rect.Width - 3 * h, Rect.Height}, Text, Align, aCenter);
    DrawRectangle ({x0, y0, h * 2, h * 2}, cBox, cBox, -1);
    if (Hover)
      DrawRectangle ({x0 + 1, y0 + 1, h * 2 - 2, h * 2 - 2}, cBox, cBox, -1);
    if (Down)
      if (StyleCross)   // Cross
        {
          DrawLine (x0 + 2, y0 + 2, x0 + h * 2 - 3, y0 + h * 2 - 3, cBlue, 2);//cText
          DrawLine (x0 + h * 2 - 3, y0 + 2, x0 + 2, y0 + h * 2 - 3, cBlue, 2);//cText
        }
      else   // Tick
        {
          x0 += h / 2;
          y0 += h / 3;
          h = h * 2 / 3;
          DrawLine (x0, y0 + h, x0 + h, y0 + 2*h, ColourAdjust (cGreen, 30), 2);//cText
          DrawLine (x0 + h, y0 + 2*h, x0 + 5*h/2, y0 - h, ColourAdjust (cGreen, 30), 2);//cText
        }
  }


///////////////////////////////////////////////////////////////////////////////
//
// BUTTON ROUND

_ButtonRound::_ButtonRound (_Container *Parent, _Rect Rect, char *Text_, _Action Action) : _Button (Parent, Rect, Text_, Action)
  {
    Colour = -1;
    ColourGrad = -1;
  }

void _ButtonRound::DrawCustom (void)
  {
    int Radius;
    //
    Radius = Min (Rect.Width, Rect.Height) / 2 - 2;
    DrawCircle (Rect.Width / 2, Rect.Height / 2, Radius, ColourFind (), -1, 2, Down ? -60 : 60);
    TextOutWrap ({TextMargin + (int) Down, (int) Down, Rect.Width - 2 * TextMargin, Rect.Height}, Text, Align, aCenter);
  }


///////////////////////////////////////////////////////////////////////////////
//
// BUTTON ARROW

_ButtonArrow::_ButtonArrow (_Container *Parent, _Rect Rect, char *Text_, _Action Action, _Direction Direction_) : _Button (Parent, Rect, Text_, Action)
  {
    Direction = Direction_;
  }

void _ButtonArrow::DrawCustom (void)
  {
    _Rect Rect_;
    //
    _Button::DrawCustom ();
    Rect_ = {Rect.Width / 4, Rect.Height / 4, Rect.Width / 2, Rect.Height / 2};
    if (Down)
      {
        Rect_.x++;
        Rect_.y++;
      }
    DrawArrow (Rect_, Direction);
  }


///////////////////////////////////////////////////////////////////////////////
//
// BUTTON SECRET

_ButtonSecret::_ButtonSecret (_Container *Parent, _Rect Rect, _Action Action) : _Button (Parent, Rect, NULL, Action)
  {
    Colour = -1;   // transparent
  }

void _ButtonSecret::DrawCustom  (void)
  {
    // do nothing
  }


///////////////////////////////////////////////////////////////////////////////
//
// LABEL

_Label::_Label (_Container *Parent, _Rect Rect, char *Text_, _Align Align_, _Border Border_) : _Container (Parent, Rect, Text_)
  {
    Align = Align_;
    AlignVert = aCenter;
    Border = Border_;
    Margin = 0;
  }

void _Label::DrawCustom (void)
  {
    int m, SizeY;
    //
    m = Margin;
    if (Border != bNone)
      m += TextMargin;
    SizeY = TextOutWrap ({m, m, Rect.Width - 2 * m, Rect.Height - 2 * m}, Text, Align, AlignVert);
    if (AlignVert == aTop)
      if (SizeY > Rect.Height)
        SizeVirtual = {Rect.Width, SizeY};
    DrawBorder (Border, 0);
  }


///////////////////////////////////////////////////////////////////////////////
//
// LABEL NUMBER

_LabelNumber::_LabelNumber (_Container *Parent, _Rect Rect, char *Text_, _Border Border_) : _Container (Parent, Rect, Text_)
  {
    Border = Border_;
    Value = 0;
  }

void _LabelNumber::DrawCustom (void)
  {
    char *St;
    char *x;
    _Align Align;
    //
    Align = aRight;
    if (Border != bNone)
      Align = aCenter;
    St = (char *) malloc (StrLen (Text) + 10);
    x = St;
    NumToStr (&x, Value);
    StrCat (&x, Text);
    *x = 0;
    TextOutWrap ({0, 0, Rect.Width, Rect.Height}, St, Align, aCenter);
    free (St);
    DrawBorder (Border, 0);
  }

int _LabelNumber::ValueGet (void)
  {
    return Value;
  }

void _LabelNumber::ValueSet (int Value_)
  {
    Value = Value_;
    Invalidate (true);
  }


///////////////////////////////////////////////////////////////////////////////
//
// LABEL MONEY

_LabelMoney::_LabelMoney (_Container *Parent, _Rect Rect, char *Text_, _Border Border_) : _Container (Parent, Rect, Text_)
  {
    Border = Border_;
    Value = 0;
  }

void _LabelMoney::DrawCustom (void)
  {
    char *St;
    char *x;
    int Margin;
    //
    DrawBorder (Border, 0);
    Margin = 0;
    if (Border != bNone)
      Margin = TextMargin;
    St = (char *) malloc (StrLen (Text) + 16);
    x = St;
    if (Value < 0)
      *x++ = '(';
    *x++ = '$';
    IntToStrDecimals (&x, abs (Value), 2);
    if (Value < 0)
      *x++ = ')';
    StrCat (&x, Text);
    *x = 0;
    TextOutWrap ({Margin, 0, Rect.Width - 2 * Margin, Rect.Height}, St, aRight, aCenter);
    free (St);
  }


///////////////////////////////////////////////////////////////////////////////
//
// LABEL TIME

_LabelTime::_LabelTime (_Container *Parent, _Rect Rect, char *Text_, _Border Border_) : _Container (Parent, Rect, Text_)
  {
    Border = Border_;
    Value = 0;
  }

void _LabelTime::DrawCustom (void)
  {
    int Val;
    char *St;
    char *x;
    int d;
    int Margin;
    //
    DrawBorder (Border, 0);
    Margin = 0;
    if (Border != bNone)
      Margin = TextMargin;
    St = (char *) malloc (StrLen (Text) + 32);
    x = St;
    Val = Value;
    d = Val / (60 * 24);
    if (d)   // Has Days
      {
        NumToStr (&x, d);
        StrCat (&x, " day");
        if (d > 1)
          StrCat (&x, 's');
        Val %= 60 * 24;   // remove whole days
      }
    if (x != St)
      StrCat (&x, ' ');
    NumToStr (&x, Val / 60);
    StrCat (&x, "h ");
    NumToStr (&x, Val % 60);
    StrCat (&x, 'm');
//    if ((Val >= 60) || (x == St))   // Has Hours OR text not empty
//      {
//        if (x != St)
//          StrCat (&x, ' ');
//        NumToStr (&x, Val / 60);
//        StrCat (&x, 'h');
//        Val %= 60;   // remove whole days
//      }
//    if ((Val > 0) || (x == St))   // has minutes OR text not empty
//      {
//        if (x != St)
//          StrCat (&x, ' ');
//        NumToStr (&x, Val);
//        StrCat (&x, 'm');
//      }
    StrCat (&x, Text);
    *x = 0;
    TextOutWrap ({Margin, 0, Rect.Width - 2 * Margin, Rect.Height}, St, aRight, aCenter);
    free (St);
  }


///////////////////////////////////////////////////////////////////////////////
//
// LABEL DATETIME

_LabelDateTime::_LabelDateTime (_Container *Parent, _Rect Rect, char *Text_, _Align Align_, _Border Border_) : _Container (Parent, Rect, Text_)
  {
    Align = Align_;
    Border = Border_;
    Tick = ClockMS ();
  }

void _LabelDateTime::DrawCustom (void)
  {
    tm DateTime;
    char Res [128], *xRes;
    int Margin;
    //
    GetDateTime (&DateTime);   // Read Time
    //DateTime.tm_year += 1900;
    // Format date time as per Text
    xRes = Res;
    StrFormatDateTime (&xRes, &DateTime, Text);
    *xRes = 0;
    //
    Margin = 0;
    if (Border != bNone)
      Margin = TextMargin;
    TextOutWrap ({Margin, 0, Rect.Width - 2 * Margin, Rect.Height}, Res, Align, aCenter);
    DrawBorder (Border, 0);
  }

void _LabelDateTime::PollCustom (void)
  {
    int t;
    //
    t = ClockMS ();
    if (t - Tick >= 1000)   // Redraw every second
      {
        Tick += 1000;
        if (IsVisible ())
          Invalidate (true);
      }
  }


///////////////////////////////////////////////////////////////////////////////
//
// LABEL STRIP

_LabelStrip::_LabelStrip (_Container *Parent, int *YPos, int Height, char *Text_) : _Label (Parent, {0, *YPos, Parent->Rect.Width, abs (Height)}, Text_, aCenter, bNone) //bRaised
  {
    *YPos += abs (Height);
    FontSet (NULL, Abs (Height), Height < 0 ? fsBold : fsNone);
  }


///////////////////////////////////////////////////////////////////////////////
//
// LABEL SCROLL

_LabelScroll::_LabelScroll (_Container *Parent, _Rect Rect, char *Text_, int SlideX_, int SlideY_) : _Container (Parent, Rect, Text_)
  {
    SlideX = SlideX_;
    SlideY = SlideY_;
    SlidX = 0;
    SlidY = 0;
    Tick = ClockMS ();
    if (SlideX_ || SlideY_)
      SlidX = Rect.Width;
  }

void _LabelScroll::DrawCustom (void)
  {
    _Point Size;
    _Font *Font_;
    //
    Font_ = FontFind ();
    if (Font_)
      {
        TextOutAligned ({SlidX, 0, Rect.Width, Rect.Height}, Text, aLeft, aCenter);
        Size = TextMeasure (Text);
        if (-SlidX >= Size.x)
          {
            SlidX = Rect.Width;
            Tick = ClockMS ();
          }
      }
  }

void _LabelScroll::PollCustom (void)
  {
    int t;
    int SlidX_;
    //
    t = ClockMS ();
    SlidX_ = SlidX;
    SlidX = Rect.Width - ((t - Tick) * SlideX) / 1000;
    if (SlidX != SlidX_)   // Redraw if text has moved
      if (IsVisible ())
        Invalidate (true);
  }


///////////////////////////////////////////////////////////////////////////////
//
// EDIT NUMBER BOX (from _Container)

_EditNumber::_EditNumber (_Container *Parent, _Rect Rect, char *Text, int Min_, int Max_, _Action Action_) : _Container (Parent, Rect, Text)
  {
    Min = Min_;
    Max = Max_;
    Value = Min_;
    Action = Action_;
    Colour = cTextBgnd;
    ColourGrad = -1;
    if (!Form->Focused)
      Form->Focused = this;
  }

bool _EditNumber::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    int Key;
    int ValueOld;
    bool Res;
    bool Check;
    //
    Res = false;
    Check = false;
    ValueOld = Value;
    if (FocusCheck (Event, Offset))
      {
        NewFocus = true;
        Check = true;
      }
    if (IsEventMine (Event, Offset))
      {
        if (Event->Type == etMouseDown)
          {
            Res = true;
            Check = true;
            if (Event->Key == KeyMouseWheelUp)
              Value++;
            else if (Event->Key == KeyMouseWheelDown)
              Value--;
          }
        // Process Key presses
        else if (Event->Type == etKeyDown && Form->Focused == this)
          {
            Res = true;
            Key = Event->Key;
            if (Key == bs)
              Value /= 10;
            else if (IsDigit (Key))
              {
                if (NewFocus)
                  Value = 0;
                Value = Value * 10 + (Key - '0');
              }
            else
              {
                Check = true;
                if (Key == KeyUp)
                  Value++;
                else if (Key == KeyDown)
                  Value--;
              }
            NewFocus = false;
          }
        else if (Event->Type == etMouseMove)   // on move (away), check minimum
          {
            NewFocus = true;
            Check = true;
          }
      }
    if (Check)
      if (Value < Min)
        Value = Min;
    if (Value > Max)
      Value = Max;
    if (Value != ValueOld)
      {
        Invalidate (true);
        if (Action)
          Action (this);
      }
    return Res;
  }

void _EditNumber::DrawCustom (void)
  {
    char *St;
    char *x;
    //
    St = (char *) malloc (StrLen (Text) + 16);
    x = St;
    if (Text)
      StrCat (&x, Text);
    NumToStr (&x, Value);
    *x = 0;
    if (Form->Focused == this)
      TextCallBackCursor = StrPos (St, '\0');
    TextOutAligned (AddMargin ({0, 0, Rect.Width, Rect.Height}, {TextMargin, 0}), St, aRight, aCenter);
    TextCallBackCursor = NULL;
    free (St);
    DrawBezel (2);
  }

void _EditNumber::ValueSet (int Val)
  {
    Value = Val;
    Invalidate (true);
  }


///////////////////////////////////////////////////////////////////////////////
//
// SLIDER

_Slider::_Slider (_Container *Parent, _Rect Rect_, int Min, int Max, _Action Action_) : _Container (Parent, Rect_)
  {
    ColourKnob = Colour;
    ValueMin = Min;
    ValueMax = Max;
    Sections = 0;
    if (Max - Min < 16)
      Sections = Max - Min;   // Number of sections to be Marked. Num Marks = Sections + 1
    Value = ValueMin;
    Action = Action_;
    Vertical = Rect.Height > Rect.Width;
    MouseDown = false;
  }

#define SliderMargin 4

bool _Slider::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool CalcValue;
    int Value_;
    bool Res;
    //
    Res = false;
    if (IsEventMine (Event, Offset))
      {
        Res = true;
        CalcValue = false;
        Value_ = Value;
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)
          {
            MouseDown = true;
            CalcValue = true;
          }
        else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelUp)   // wheel up
          Value++;
        else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelDown)   // wheel down
          Value--;
        else if (Event->Type == etMouseUp)
          MouseDown = false;
        else if (Event->Type == etMouseMove)
          {
            if (MouseDown)
              CalcValue = true;
          }
        else
          Res = false;
        if (CalcValue)
          if (Vertical)
            Value = ValueMax - (Event->Y - Offset.y - SliderMargin) * (ValueMax - ValueMin) / (Rect.Height - 2 * SliderMargin);
          else   // Horizontal
            Value = ValueMin + (Event->X - Offset.x - SliderMargin) * (ValueMax - ValueMin) / (Rect.Width - 2 * SliderMargin);
        if (Value < ValueMin)
          Value = ValueMin;
        else if (Value > ValueMax)
          Value = ValueMax;
        if (Value != Value_)   // changed
          {
            Invalidate (true);
            if (Action)
              Action (this);
          }
      }
    return Res;
  }

void _Slider::DrawCustom (void)
  {
    int hx, hy;
    int ColSlot, ColKnob, ColTick;
    int x, y, s;
    int Pos;
    _Rect Knob;
    //
    hx = Rect.Width / 2;
    hy = Rect.Height / 2;
    ColSlot = ColourFind ();
    ColKnob = ColourKnob;
    ColTick = ColourAverage (ColourTextFind (), ColSlot);
    if (ColKnob < 0)
      ColKnob = ColSlot;
    // Draw Markers
    if (Sections > 0)
      {
        for (s = 1; s < Sections; s++)
          if (Vertical)
            {
              y = s * (Rect.Height - 2 * SliderMargin) / Sections + SliderMargin;
              DrawLine (hx - SliderMargin, y, hx - 3 * SliderMargin / 4, y, ColTick, 1);
              DrawLine (hx + SliderMargin, y, hx + 3 * SliderMargin / 4, y, ColTick, 1);
            }
          else   // Horizontal
            {
              x = s * (Rect.Width - 2 * SliderMargin) / Sections + SliderMargin;
              DrawLine (x, hy - SliderMargin, x, hy - 3 * SliderMargin / 4, ColTick, 1);
              DrawLine (x, hy + SliderMargin, x, hy + 3 * SliderMargin / 4, ColTick, 1);
            }
      }
    // Draw Slot, setup Knob
    if (Vertical)
      {
        DrawBorder ({hx, SliderMargin, 0, Rect.Height - 2 * SliderMargin}, bMote, ColSlot, 0);
        Pos = (Rect.Height - 2 * SliderMargin) * (ValueMax - Value) / (ValueMax - ValueMin);
        Knob = {hx - 2 * SliderMargin, Pos, 4 * SliderMargin, 2 * SliderMargin};
      }
    else
      {
        DrawBorder ({SliderMargin, hy, Rect.Width - 2 * SliderMargin, 0}, bMote, ColSlot, 0);
        Pos = (Rect.Width - 2 * SliderMargin) * (Value - ValueMin) / (ValueMax - ValueMin);
        Knob = {Pos, hy - 2 * SliderMargin, 2 * SliderMargin, 4 * SliderMargin};
      }
    // Draw Knob  <===3
    DrawRectangle (Knob, -1, -1, ColKnob);
    DrawBorder (Knob, bRaised, ColKnob, 0);
    DrawBorder (Knob, bRaised, ColKnob, 1);
  }

int _Slider::ValueGet (void)
  {
    return Value;
  }

void _Slider::ValueSet (int Value_)
  {
    Value = Value_;
    Invalidate (true);
  }


///////////////////////////////////////////////////////////////////////////////
//
// KNOB - ROTARY Slider

_Knob::_Knob (_Container *Parent, _Rect Rect, char *Text, int Min_, int Max_, _Border Border_, _Action Action_) : _Container (Parent, Rect, Text)
  {
    ValueMin = Min_;
    ValueMax = Max_;
    Border = Border_;
    Value = Min_;
    Action = Action_;
    Markers = Min (11, Max_ - Min_ + 1);
    MouseX = MouseY = -1;
  }

bool _Knob::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    int Value_;
    bool Res;
    //
    Res = false;
    if (IsEventMine (Event, Offset))
      {
        Res = true;
        Value_ = Value;
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)
          {
            MouseX = Event->X;
            MouseY = Event->Y;
            MouseValue = Value;
          }
        else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelUp)
          Value++;
        else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelDown)
          Value--;
        else if (Event->Type == etMouseUp)
          MouseX = MouseY = -1;
        else if (Event->Type == etMouseMove)
          {
            if (MouseX >= 0)
              Value = MouseValue + ((MouseY - Event->Y) + (Event->X - MouseX)) * (ValueMax - ValueMin) / Rect.Width;
          }
        else
          Res = false;
        Value = Limit (Value, ValueMin, ValueMax);
        if (Value != Value_)
          {
            Invalidate (true);
            if (Action)
              Action (this);
          }
      }
    return Res;
  }

void _Knob::DrawCustom (void)
  {
    int Rad;
    int c0, cf, cm;
    int angle, i;
    int Margin;
    //
    Rad = Min (Rect.Width, Rect.Height) / 3;
    c0 = ColourFind ();
    cf = ColourTextFind ();
    cm = cf;
    // Draw MArkers
    for (i = 0; i < Markers; i++)
      {
        angle = (180 + 45) - (i * (360 - 90) + (Markers / 2)) / (Markers - 1);
        DrawRadius (Rect.Width / 2, Rect.Width / 2, Rad + 4, Rad + 4, angle, cm, 1);
      }
    // Draw Knob
    DrawCircle (Rect.Width / 2, Rect.Width / 2, Rad, c0, -1, 2, 60); //ColourAdjust (c0, 120)
    angle = 180 + 45 - (Value - ValueMin) * (360 - 90) / (ValueMax - ValueMin);
    DrawRadius (Rect.Width / 2, Rect.Width / 2, Rad / 4, Rad - 3, angle, cf, 2);
    //
    Margin = 0;
    if (Border != bNone)
      Margin = TextMargin;
    TextOutWrap ({Margin, 0, Rect.Width - 2 * Margin, Rect.Height - Margin}, Text, aCenter, aRight /*Bottom*/);
    DrawBorder (Border, 0);
  }

int _Knob::ValueGet (void)
  {
    return Value;
  }

void _Knob::ValueSet (int Value_)
  {
    Value = Value_;
    Invalidate (true);
  }


///////////////////////////////////////////////////////////////////////////////
//
// PLEASE WAIT BOX

//float Sin [] = {0.000000, 0.707107, 1.000000, 0.707107, 0.000000, -0.707107, -1.000000, -0.707107};
//float Cos [] = {1.000000, 0.707107, 0.000000, -0.707107, -1.000000, -0.707107, 0.000000, 0.707107};

_Wait::_Wait (_Container *Parent, _Rect Rect): _Container (Parent, Rect)
  {
    Counter = 0;
    Tick = ClockMS ();
  }

void _Wait::DrawCustom (void)
  {
    int cb, cf, c;
    int r, d, x, y;
    int R;
    int i;
    //
    cb = ColourFind ();
    // Calculate parameters
    cf = ColourTextFind ();
    d = Min (Rect.Width, Rect.Height);
    r = d / 8;
    R = d / 2 - r - 1;
    x = Rect.Width / 2;
    y = Rect.Height / 2;
    for (i = 7; i > 0; i--)
      {
        c = ColourGraded (cb, cf, i, 7);
        DrawCircle (x + CosDeg (((i + Counter) % 8) * 45) * (float) R, y + SinDeg (((i + Counter) % 8) * 45) * (float) R, r, -1, c);
        //r = r - 1;
      }
  }

void _Wait::PollCustom (void)
  {
    int t;
    //
    t = ClockMS ();
    if (t - Tick >= 250)   // 250mS. Time to change
      {
        Tick += 250;
        Counter++;
        if (IsVisible ())
          Invalidate (true);
      }
  }


///////////////////////////////////////////////////////////////////////////////
//
// IMAGE BOX

_Image::_Image (_Container *Parent, _Rect Rect, char *Filename, bool Transparent_) : _Container (Parent, Rect, Filename)
  {
    Transparent = Transparent_;
    Stretch = sNormal;
    Align = aCenter;
    AlignVert = aCenter;
  }

void _Image::DrawCustom (void)
  {
    _Rect rSrc, rDest;
    _Point Size, Size_;
    //
    if (Bitmap == NULL && Text)
      {
        Bitmap = BitmapLoadResource (Form->Window, Text);
        if (Bitmap == NULL)
          DebugAddS ("Image: File not loaded: ", Text);
      }
    if (Bitmap)
      {
        BitmapGetSize (Bitmap, &Size.x, &Size.y);
        if (Size.x && Size.y)
          {
            rSrc = {0, 0, Size.x, Size.y};
            if (Stretch == sTile)
              {
                rDest = {0, 0, Size.x, Size.y};
                while (true)
                  {
                    if (rDest.x >= Rect.Width)
                      {
                        rDest.x = 0;
                        rDest.y += Size.y;
                        if (rDest.y >= Rect.Height)
                          break;
                      }
                    DrawBitmap (Bitmap, rSrc, rDest, Transparent);
                    rDest.x += Size.x;
                  }
              }
            else
              {
                switch (Stretch)
                  {
                    case sNone:   // preserve size
                                  Size_ = Size;
                                  if (Size.x <= Rect.Width && Size.y <= Rect.Height)   // too big => sNormal
                                    break;
                    case sNormal: // Stretch keeping aspect ratio
                                  Size_ =  SizeStretch (Size, {Rect.Width, Rect.Height});
                                  break;
                    case sFill:   // Fill entire Container
                                  Size_ = {Rect.Width, Rect.Height};
                                  break;
                  }
                // Align Image & Draw
                rDest = {0, 0, Size_.x, Size_.y};   // Assume Top & Left Justified
                if (Align == aRight)
                  rDest.x = Rect.Width - Size_.x;
                else if (Align == aCenter)
                  rDest.x = (Rect.Width - Size_.x) / 2;
                if (AlignVert == aRight)   // bottom
                  rDest.y = Rect.Height - Size_.y;
                else if (AlignVert == aCenter)   // center vertically
                  rDest.y = (Rect.Height - Size_.y) / 2;
                DrawBitmap (Bitmap, rSrc, rDest, Transparent);
              }
          }
      }
  }


///////////////////////////////////////////////////////////////////////////////
//
// SELECTION LIST, MENU, POPUP MENU

#define ListMargin 6

_SelectionList::_SelectionList (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_) : _Container (Parent, Rect_, Text_)
  {
    _Point Size;
    char *Item, *ps;
    //
    Hovering = -1;
    ListOffset = 0;
    Selected = -1;
    Action = Action_;
    Colour = cForm1;
    ColourGrad = cForm2;
    /*if (cMenu >= 0)
      {
        Colour = cMenu;
        ColourGrad = -1;
      }*/
    ItemSize = {0, 0};
    ps = Text;
    ListSize = 0;
    while (true)
      {
        Item = StrGetItem (&ps, '\t');
        if (!Item)
          break;
        Size = TextMeasure (Item);
        ItemSize.x = Max (ItemSize.x, Size.x);
        ItemSize.y = Max (ItemSize.y, Size.y);
        free (Item);
        ListSize++;
      }
    ListDisplayed = ListSize;
    ItemSize.y += 2 * TextMargin;
    Rect.Width = ItemSize.x + 2 * ListMargin;
    Rect.Height = ItemSize.y * ListSize + 2 * ListMargin;
    while (Rect.Height > Parent->Rect.Height)
      {
        Rect.Height -= ItemSize.y;
        ListDisplayed--;
      }
    if (Rect.x + Rect.Width > Parent->Rect.Width)
      Rect.x = Parent->Rect.Width- Rect.Width;
    if (Rect.y + Rect.Height > Parent->Rect.Height)
      Rect.y = Parent->Rect.Height - Rect.Height;
    Form->EventLock = this;
  }

_SelectionList::~_SelectionList (void)
  {
    //if (Form->EventLock == this)   // check this has not been taken over %%%%
    //  Form->EventLock = NULL;
  }

void _SelectionList::DrawCustom (void)
  {
    int i, x, y;
    char *Item, *Item_;
    char *ps;
    _Rect Rec;
    int cLine;
   //
    if (!Text)
      return;
    // Draw List
    cLine = ColourAverage (ColourFind (), ColourTextFind ());
    i = 0;
    ps = Text;
    y = ListMargin;
    while (true)
      {
        Item = StrGetItem (&ps, '\t');
        if (!Item)
          break;
        if (i >= ListOffset)   // Skip past the Offset (if List scrolled up)
          {
            //DebugAddS ("List Draw Item: ", Item);  //####
            Rec = {0, y, Rect.Width, ItemSize.y};
            Item_ = Item;
            if (*Item_ == '_')
              Item_++;
            if (i == Hovering)
              TextOutInverse (Rec, Item_, ListMargin);
            else
              TextOutAligned (AddMargin (Rec, {ListMargin, 0}), Item_, aLeft, aCenter);
            y += ItemSize.y;
            if (*Item == '_')
              DrawLine (0, y, Rect.Width - 1, y, cLine, 1);
          }
        free (Item);
        i++;
        if (i - ListOffset >= ListDisplayed)
          break;
      }
    // Extender arrows
    x = Rect.Width / 2;
    if (ListOffset)
      DrawArrow ({x - 6, 1, 12, ListMargin - 2}, dUp);
    if (ListSize - ListOffset > ListDisplayed)
      DrawArrow ({x - 6, Rect.Height - ListMargin + 1, 12, ListMargin - 2}, dDown);
    DrawBorder (bRaised, 0);
  };

bool _SelectionList::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    int h, o;
    //
    Res = false;
    h = Hovering;
    o = ListOffset;
    if (IsEventMine (Event, Offset))
      if (Event->Type == etMouseMove)
        {
          if (ItemSize.y > 0 && Event->Y >= Offset.y + ListMargin && Event->Y < Offset.y + Rect.Height - ListMargin)
            Hovering = ListOffset + (Event->Y - Offset.y - ListMargin) / ItemSize.y;
          else
            Hovering = -1;
          if (Event->Y - Offset.y >= Rect.Height - ListMargin)
            if (ListSize - ListOffset > ListDisplayed)
              ListOffset++;
          if (Event->Y - Offset.y <= ListMargin)
            if (ListOffset > 0)
              ListOffset--;
          Res = true;
        }
      else if (/*Event->Type == etMouseDown ||*/ Event->Type == etMouseUp)   // Selected
        {
          if (IsEventMine (Event, Offset))
            if (Event->Key == KeyMouseLeft || Event->Key == KeyMouseRight)
              {
                Selected = Hovering;
                if (Action)
                  Action (this);
              }
            else if (Event->Key == KeyMouseWheelUp && ListOffset > 0)
              ListOffset--;
            else if (Event->Key == KeyMouseWheelDown && ListSize - ListOffset > ListDisplayed)
              ListOffset++;
          Res = true;
        }
      else if (Event->Type == etKeyDown)
        {
          if (Event->Key == KeyEnter)
            Selected = Hovering;
          if (Action)
            Action (this);
          Res = true;
        }
    if (Hovering != h || ListOffset != o)
      Invalidate (true);
    return Res;
  }

_Menu::_Menu (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_) : _Container (Parent, Rect_, Text_)
  {
    char *ps;
    _Point Size;
    //
    Action = Action_;
    SelectionList = NULL;
    Name = NULL;
    List = NULL;
    if (Text && *Text)
      {
        ps = Text;
        Name = StrGetItem (&ps, '\t');
        //StepSpace (&ps);
        StrAssignCopy (&List, ps);
        Size = TextMeasure (Name);
        Rect.Width = Size.x + 2 * ListMargin;
      }
    RectLock = 0;
  }

_Menu::~_Menu (void)
  {
    free (Name);
    free (List);
  }

void _Menu::DrawCustom (void)
  {
    _Rect Rec;
    //
    if (!Text || !*Text)
      return;
    Rec = {0, 0, Rect.Width, Rect.Height};
    if (SelectionList)
      TextOutInverse (Rec, Name, ListMargin);
    else
      TextOutWrap (Rec, Name, aCenter, aCenter);
  }

void ActionMenuSelectionList (_SelectionList *SelectionList)
  {
    _Menu *Menu;
    //
    Menu = (_Menu *) SelectionList->DataContainer;
    if (Menu->SelectionList)
      {
        Menu->Selected = SelectionList->Selected;
        if (Menu->Selected >= 0)
          if (Menu->Action)
            Menu->Action ((_Container *) Menu);
        Menu->SelectionList->Die ();
        Menu->SelectionList = NULL;
        Menu->Invalidate (true);
      }
  }

bool _Menu::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    //
    Res = false;
    if (List)
      if (IsEventMine (Event, Offset))
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)   // Activate
          {
            SelectionList = new _SelectionList (Form->Container, {Offset.x, Offset.y + Rect.Height, 0, 0}, List, (_Action) ActionMenuSelectionList);
            SelectionList->DataContainer = this;   // remember where we came from
            Invalidate (true);
            Res = true;
          }
    return Res;
  }

_MenuPopup::_MenuPopup (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_) : _Container (Parent, Rect_, Text_)
  {
    Action = Action_;
    SelectionList = NULL;
    FontForm = NULL;
  }

void ActionMenuPopupSelectionList (_SelectionList *SelectionList)
  {
    _MenuPopup *MenuPopup;
    //
    MenuPopup = (_MenuPopup *) SelectionList->DataContainer;
    if (MenuPopup->FontForm)   // Restore Font
      MenuPopup->Form->Container->Font = MenuPopup->FontForm;
    if (MenuPopup->SelectionList)
      {
        MenuPopup->Selected = SelectionList->Selected;
        if (MenuPopup->Selected >= 0)
          if (MenuPopup->Action)
            MenuPopup->Action ((_Container *) MenuPopup);
        MenuPopup->SelectionList->Die ();
        MenuPopup->SelectionList = NULL;
      }
  }

bool _MenuPopup::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    //
    Res = false;
    if (Text)
      if (IsEventMine (Event, Offset))
        if (Event->Type == etMouseDown && Event->Key == KeyMouseRight)   // Activate
          {
            Mouse = {Event->X - Offset.x, Event->Y - Offset.y};
            if (Font)
              {
                FontForm = Form->Container->Font;   // Impose Font
                Form->Container->Font = Font;
              }
            SelectionList = new _SelectionList (Form->Container, {Event->X, Event->Y, 0, 0}, Text, (_Action) ActionMenuPopupSelectionList);
            SelectionList->DataContainer = this;   // remember where we came from
            Res = true;
          }
    return Res;
  }


//////////////////////////////////////////////////////////////
//
// DROP LIST

void ActionDropListSelectionList (_SelectionList *SelectionList)
  {
    _DropList *DropList;
    //
    DropList = (_DropList *) SelectionList->DataContainer;
    if (SelectionList->Selected >= 0)
      {
        if (DropList->SelectionList)
          {
            DropList->Selected = SelectionList->Selected;
            if (DropList->Action)
              DropList->Action ((_Container *) DropList);
            DropList->Invalidate (true);
          }
      }
    SelectionList->Die ();
    DropList->SelectionList = NULL;
  }

_DropList::_DropList (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_) : _Container (Parent, Rect_, Text_)
  {
    Action = Action_;
    Colour = cTextBgnd;
    Selected = -1;
    SelectionList = NULL;
  };

_DropList::~_DropList (void)
  {
  };

#define DropMargin 4

void _DropList::DrawCustom (void)
  {
    char *Item;
    char *ps;
    int i;
    _Rect Rec;
    //
    if (!Text || !*Text)
      return;
    Rec = {DropMargin, 0, Rect.Width - Rect.Height - 2 * DropMargin, Rect.Height};
    // Draw Selected Value
    if (Selected >= 0)
      {
        ps = Text;
        i = 0;;
        while (true)
          {
            Item = StrGetItem (&ps, '\t');
            if (Item == nullptr)
              break;
            if (i == Selected)
              {
                TextOutWrap (Rec, Item, aLeft, aCenter);
                free (Item);
                break;
              }
            free (Item);
            i++;
          }
      }
    // Draw Arrow and bexel
    i = Rect.Height / 4;
    DrawArrow ({Rect.Width - 3*i, 0, 2*i, Rect.Height}, dDown);
    DrawBezel (2);
  }

bool _DropList::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    if (IsEventMine (Event, Offset))
      {
        if (Event->Type == etMouseDown)
          {
            SelectionList = new _SelectionList (Parent, {Rect.x, Rect.y + Rect.Height, 16, 16}, Text, (_Action) ActionDropListSelectionList);
            SelectionList->DataContainer = this;   // remember where we came from
          }
        return true;
      }
    return false;
  }


//////////////////////////////////////////////////////////////
//
// TABBED CONTROL

#define TabMargin 4
#define TabOffset 2

_Tabs::_Tabs (_Container *Parent, _Rect Rect, char *Text_, _Action Action) : _Container (Parent, Rect, Text_)
  {
    Colour = cForm1;
    ColourGrad = -1;
    Selected = 0;
    _Tabs::Action = Action;
  }

void _Tabs::DrawCustom (void)
  {
    int Offset;
    int Col;
    int ColBG, ColBR;
    //
    int i, x, x1, x2;
    char *Item, *ps;
    _Point Size;
    int Width;
    _Rect Rec;
    //
    if (!Text)
      return;
    Col = ColourFind ();
    ColBG = ColourAdjust (Col, 80);
    x = 0;
    i = 0;
    ps = Text;
    while (true)
      {
        Item = StrGetItem (&ps, '\t');
        if (!Item)
          break;
        Size = TextMeasure (Item);
        Width = Size.x + 2 * TabMargin;
        Offset = TabOffset;
        if (i == Selected)
          {
            Offset = 0;
            x1 = x;
            x2 = x + Width;
          }
        else
          DrawRectangle ({x, 0, Width, TabOffset}, -1, -1, ColBG);   // Dark area above offset tabs
        Rec = {x, 0 + Offset, Width, Rect.Height - Offset};
        DrawTab (Rec, Col, ColBG, i == Selected);
        Rec.y += TabMargin;
        TextOutWrap (Rec, Item, aCenter, aLeft);
        free (Item);
        x += Rec.Width;
        i++;
      }
    // Separation line
    ColBR = BorderColourBR (Col);
    if (x1)
      DrawLine (0, Rect.Height - 1, x1 - 1, Rect.Height - 1, ColBR);
    DrawLine (x2, Rect.Height - 1, Rect.Width - 1, Rect.Height - 1, ColBR);
    // Remaining background region, right of tabs
    DrawRectangle ({x, 0, Rect.Width - x, Rect.Height}, -1, -1, ColBG);
  };

bool _Tabs::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    char *ps;
    char *Item;
    _Point Size;
    int i, x;
    _Container *c;
    //
    Res = false;
    if (IsEventMine (Event, Offset))
      {
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)   // Left Mouse Down
          {
            Res = true;
            i = 0;
            x = 0;
            ps = Text;
            while (true)
              {
                Item = StrGetItem (&ps, '\t');
                if (!Item)
                  {
                    i = Selected;
                    break;
                  }
                Size = TextMeasure (Item);
                free (Item);
                x += Size.x + 2 * TabMargin;
                if (Event->X + Offset.x < x)   // this tab selected
                  break;
                i++;
              }
            if (i != Selected)
              {
                Selected = i;
                if (Form->Focused)
                  {
                    c = Form->Focused;
                    Form->Focused = NULL;
                    c->Invalidate (true);
                  }
                Invalidate (true);
                if (Action)
                  Action (this);
              }
          }
      }
    return Res;
  };


///////////////////////////////////////////////////////////////////////////////
//
// SPLITTER

_Splitter::_Splitter (_Container *Parent, _Rect Rect) : _Container (Parent, Rect)
  {
    Reference = {-1, -1};
    Colour = ColourAdjust (cForm1, 90);
  }

void _Splitter::DrawCustom (void)
  {
    int x, y, s;
    //
    // Draw ??
    x = Rect.Width / 2;
    y = Rect.Height / 2;
    s = Min (Rect.Width, Rect.Height) / 2;
    if (Rect.Height > Rect.Width)   // Vertical Bar
      DrawKnerl ({x - s + 1, y - s, 2 * s - 2, 2 * s}, true);
    else
      DrawKnerl ({x - s, y - s + 1, 2 * s, 2 * s - 2}, false);
    //DrawRectangle ({x - s, y - s, s+s, s+s}, -1, -1, ColourTextFind ());
  }

bool _Splitter::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    _Point p;
    int dx, dy;
    _Container *c;
    //
    Res = false;
    if (IsEventMine (Event, Offset))
      {
        p = {Event->X - Parent->Rect.x, Event->Y - Parent->Rect.y};   // Mouse Relative to Parent
        //DebugAddR ("Splitter:p psz", {p.x, p.y, Parent->Rect.Width, Parent->Rect.Height});
        //DebugAddP ("Splitter:XY", {Event->X, Event->Y});
        //DebugAddP ("Splitter:Offset", {Offset});
        Res = true;
        if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)   // Left Mouse Button Down
          Reference = {Event->X, Event->Y};
        else if (Event->Type == etMouseMove && (Event->MouseKeys & Bit [KeyMouseLeft - 1]))   // Mouse Move with Left Button
          if (p.x >= 8 && p.x < Parent->Rect.Width - 8)   // Don't move outside of Parent
            if (p.y >= 8 && p.y < Parent->Rect.Height - 8)
              {
                // Define Splitter movement
                dx = dy = 0;
                if (Rect.Width > Rect.Height)   // Horizontal Splitter
                  dy = Event->Y - Reference.y;
                else   // Vertical Splitter
                  dx = Event->X - Reference.x;
                // Find effected siblings
                if (dx || dy)
                  {
                    c = Parent->Children;
                    while (c)
                      {
                        if ((c->Rect.x + c->Rect.Width == Rect.x) && dx)   // Left sibling
                          c->Resize (c->Rect.Width + dx, c->Rect.Height);
                        else if ((Rect.x + Rect.Width == c->Rect.x) && dx)   // Right sibling
                          {
                            c->Resize (c->Rect.Width - dx, c->Rect.Height);
                            c->Rect.x += dx;
                          }
                        if ((c->Rect.y + c->Rect.Height == Rect.y) && dy)   // Top sibling
                          c->Resize (c->Rect.Width, c->Rect.Height + dy);
                        else if ((Rect.y + Rect.Height == c->Rect.y) && dy)   // Bottom sibling
                          {
                            c->Resize (c->Rect.Width, c->Rect.Height - dy);
                            c->Rect.y += dy;
                          }
                        c = c->Next;
                      }
                    Rect.x += dx;   // Move the Splitter itself
                    Rect.y += dy;
                    Invalidate (true);
                    Reference = {Event->X, Event->Y};
                  }
              }
      }
    return Res;
  };


///////////////////////////////////////////////////////////////////////////////
//
// SOFT KEYBOARD

char *Keyboard1 [] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\b", NULL};
char *Keyboard2 [] = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", NULL};
char *Keyboard3 [] = {"A", "S", "D", "F", "G", "H", "J", "K", "L", NULL};
char *Keyboard4 [] = {"Z", "X", "C", "V", "B", "N", "M", " ", NULL};

int KeySeries [2];

void _ButtonAction (_Container *Container)
  {
    char *s;
    _Button *Button;
    //
    Button = (_Button *) Container;
    if (Button)
      //if (Button->Down)
        {
          s = Button->TextGet ();
          if (s && s [0])
            {
              KeySeries [0] = s [0];
              KeySeries [1] = 0;
              Button->Form->KeySeries = KeySeries;
              DebugAdd ("SoftKeyboard press");
            }
        }
  }

#define SoftKeyboardGap 4

_SoftKeyboard::_SoftKeyboard (_Container *Parent, _Rect Rect_) : _Container (Parent, Rect_, NULL)
  {
    int y;
    //
    ButtonSpacing.x = (Rect.Width + SoftKeyboardGap) / 12;
    ButtonSpacing.y = (Rect.Height + SoftKeyboardGap) / 4;
    ButtonSize.x = ButtonSpacing.x - SoftKeyboardGap;
    ButtonSize.y = ButtonSpacing.y - SoftKeyboardGap;
    y = 0;
    CreateButtons (Keyboard1, {0, y});
    y += ButtonSpacing.y;
    CreateButtons (Keyboard2, {ButtonSize.x / 2, y});
    y += ButtonSpacing.y;
    CreateButtons (Keyboard3, {ButtonSize.x, y});
    y += ButtonSpacing.y;
    CreateButtons (Keyboard4, {ButtonSize.x * 3 / 2, y});
  }

void _SoftKeyboard::CreateButtons (char *Keys [], _Point Start)
  {
    _Button *b;
    //
    while (*Keys)
      {
        if (*Keys [0] <= ' ')
          Start.x += ButtonSize.x / 2;
        if (*Keys [0] < ' ')   // backspace
          b = new _ButtonArrow (this, {Start.x, Start.y, ButtonSize.x + ButtonSpacing.x, ButtonSize.y}, *Keys, _ButtonAction, dLeft);
        else
          b = new _Button (this, {Start.x, Start.y, *Keys [0] == ' ' ? ButtonSize.x * 4: ButtonSize.x, ButtonSize.y}, *Keys, _ButtonAction);
        Start.x += b->Rect.Width + SoftKeyboardGap;
        Keys++;
      }
  }
