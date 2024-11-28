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

#include "Widgets.hpp"
#include "WidgetsDriver.hpp"

// THEME
int cForm1 = 0xE8E8E8; //0xC0C0C0;
int cForm2 = 0xC0C0C0; //-1;
int cButton1 = 0xE0E0E0;
int cButton2 = 0xC0C0C0;
int cText = cBlack;
int cTextBgnd = cWhite;
int cSelected = ColourAdjust (cBlue, 190);
int cMenu = -1; //ColourAdjust (cBlue, 180); //-1; //cWhite;
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

char *CurrentPath = NULL;
char *Filename_ = NULL;

void HomeFilePath (char *Filename)
  {
    char *p;
    //
    if (Filename [0] == PathDelimiter)
      StrAssignCopy (&Filename_, Filename);
    else
      {
        free (Filename_);
        Filename_ = (char *) malloc (StrLen (CurrentPath) + StrLen (Filename) + 8);
        p = Filename_;
        StrCat (&p, CurrentPath);
        StrCat (&p, PathDelimiter);
        StrCat (&p, Filename);
        *p = 0;
      }
  }


_Point BottomRight (_Rect *Rect)
  {
    return {Rect->x + Rect->Width - 1, Rect->y + Rect->Height - 1};
  }

///////////////////////////////////////////////////////////////////////////////
//
// DEBUG

void StrCat_ (char **sp, char *St, int Len)
  {
    byte c;
    char *St_;
    //
    St_ = St;
    if (St)
      while (*St && St - St_ < Len)
        {
          c = *St++;
          if (c < ' ')
            {
              StrCat (sp, '^');
              StrCat (sp, c | '@');
            }
          else if (c < 0x80)
            StrCat (sp, c);
          else
            NumToHex (sp, c, 2);
        }
  }

void PointToStr (char **St, _Point Point)
  {
    StrCat (St, '(');
    NumToStr (St, Point.x);
    StrCat (St, ',');
    NumToStr (St, Point.y);
    StrCat (St, ')');
  }

void RectToStr (char **sp, _Rect *Rec)
  {
    StrCat (sp, '(');
    NumToStr (sp, Rec->x);
    StrCat (sp, ',');
    NumToStr (sp, Rec->y);
    StrCat (sp, ")-");
    NumToStr (sp, Rec->Width);
    StrCat (sp, 'x');
    NumToStr (sp, Rec->Height);
  }

void DebugAddS (const char *s1, const char *s2)
  {
    char *s = (char *) malloc (StrLen (s1) + StrLen (s2) + 1);
    char *p = s;
    if (s1)
      StrCat (&p, s1);
    if (s2)
      StrCat (&p, s2);
    *p = 0;
    DebugAdd (s);
    free (s);
  }

void DebugAdd (const char *St, int n)
  {
    char *s = (char *) malloc (StrLen (St) + 16);
    char *p = s;
    if (St)
      StrCat (&p, St);
    StrCat (&p, ' ');
    NumToStr (&p, n);
    *p = 0;
    DebugAdd (s);
    free (s);
  }

void DebugAddP (const char *St, _Point pnt)
  {
    char *s = (char *) malloc (StrLen (St) + 16);
    char *p = s;
    if (St)
      StrCat (&p, St);
    StrCat (&p, ' ');
    PointToStr (&p, pnt);
    *p = 0;
    DebugAdd (s);
    free (s);
  }

void DebugAddR (const char *St, _Rect *r)
  {
    char *s = (char *) malloc (StrLen (St) + 32);
    char *p = s;
    if (St)
      StrCat (&p, St);
    StrCat (&p, ' ');
    RectToStr (&p, r);
    *p = 0;
    DebugAdd (s);
    free (s);
  }


///////////////////////////////////////////////////////////////////////////////
//
// RENDER TEXTURE DEBUG

//#define DebugShowRenders

void RenderTexture_ (_Window *Window, _Texture *Texture, _Rect RecSource, int DestX, int DestY, byte AlphaOffset, char *Text)
  {
    RenderTexture (Window, Texture, RecSource, DestX, DestY, AlphaOffset);
    #ifdef DebugShowRenders
    char *s, *sp;
    s = (char *) malloc (100 + StrLen (Text));
    sp = s;
    StrCat (&sp, "    Rendering: ");
    RectToStr (&sp, &RecSource);
    StrCat (&sp, " --> ");
    PointToStr (&sp, {DestX, DestY});
    if (AlphaOffset != 0xFF)
      {
        StrCat (&sp, ' ');
        NumToHex (&sp, AlphaOffset);
      }
    /*RectOriginToStr (&sp, &RecDest);
    StrCat (&sp, ' ');
    RectSizeToStr (&sp, &RecSource);
    if (RecSource.Width != RecDest.Width || RecSource.Width != RecDest.Width)
      {
        StrCat (&sp, "==>");
        RectSizeToStr (&sp, &RecDest);
      }*/
    if (Text)
      {
        StrCat (&sp, "  \"");
        StrCat_ (&sp, Text);
        StrCat (&sp, '\"');
      }
    *sp = 0;
    DebugAdd (s);
    free (s);
    #endif // DebugShowRenders
  }


///////////////////////////////////////////////////////////////////////////////
//
// SUPPORT

char *TextCallBackCursor;
_Point CursorLocation;
char *TextCallBackSelection [2];
_Bitmap *TextCallBackBitmap;

int GetBitmapIndex (char **St)
  {
    int Res;
    char c;
    //
    if (*St [0] != '\e')
      return -1;
    c = UpCase ((*St) [1]);
    if (c >= '0' && c <= '9')
      Res = c - '0';
    else if (c >= 'A' && c <= 'Z')
      Res = c - 'A' + 10;
    else
      return -1;
    (*St) += 2;
    return Res;
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
                RenderBitmap (Window, TextCallBackBitmap, {Index * bh, 0, bh, bh}, {x, y, bh, bh}, true);
              }
            Res = bh;
          }
      }
    return Res;
  }

bool EditLinesTextCallBack (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size)
  {
    int w;
    int bi;
    int bsz;
    static bool Selected = false;
    static int ColBG;
    bool Selected_;
    //
    // Cursor?
    if (Posn)
      if (*Text == TextCallBackCursor)
        {
          w = Max (((Font->YAdvance + 7) / 15) * 2, 2);
          RenderFillRect (Window, {Posn->x - w / 2, Posn->y, w, Font->YAdvance}, Font->Colour);
          CursorLocation = *Posn;
        }
    // Bitmap?
    while (true)
      {
        bi = GetBitmapIndex (Text);
        if (bi < 0)
          break;
        if (Window && Posn)
          Posn->x += DrawTextBitmap (Window, bi, Posn->x, Posn->y, Font->YAdvance);
        else if (Size)
          {
            bsz = DrawTextBitmap (0, bi, 0, 0, 0);
            Size->x += bsz;
            Size->y = Max (Size->y, bsz);
          }
      }
    // Selection
    if (Window && Posn)
      {
        if (TextCallBackSelection [0] == nullptr)
          Selected_ = false;
        else
          if (TextCallBackSelection [0] < TextCallBackSelection [1])
            Selected_ = (*Text >= TextCallBackSelection [0] && *Text < TextCallBackSelection [1]);
          else
            Selected_ = (*Text >= TextCallBackSelection [1] && *Text < TextCallBackSelection [0]);
        if (Selected_ != Selected)
          {
            Selected = Selected_;
            if (Selected)
              {
                ColBG = Font->ColourBG;
                Font->ColourBG = cSelected;
                Font->Style |= fsFillBG ;
              }
            else
              {
                Font->ColourBG = ColBG;
                Font->Style &= ~fsFillBG;
              }
          }
      }
    return true;
  }


///////////////////////////////////////////////////////////////////////////////
//
// FORM

void _Form::CreateContainer (char *Title)
  {
    int Width;
    int Height;
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
    KeyFocus = NULL;
    delete Container;
    Container = NULL;
    CreateContainer (Title);
    StrAssignCopy (&Title, NULL);
  }

_Form::_Form (char *Title, _Rect Pos, byte WindowAttributes_)  // : _List (&FormList)
  {
    DebugAddS ("Creating Form ", Title); //####
    KeyPress = 0;   // Soft Keyboard input
    EventPreview = NULL;
    Alpha = 0xFF;
    if (WindowCount++ == 0)
      {
        WidgetsInit ();
        TextCallBack = &EditLinesTextCallBack;
        GetCurrentPath (&CurrentPath);
      }
    Container = NULL;
    KeyFocus = NULL;
    Window = WindowCreate (Title, Pos.x, Pos.y, Pos.Width, Pos.Height, WindowAttributes_);
    CreateContainer (Title);
    WindowAttributes = WindowAttributes_;
    WindowTexture = NULL;
    EventLock = NULL;
    ListAdd (this, &FormList);
    DieFlag = false;
    FloatError = 0.0001;   // Accuracy on sqrt/trig functions
  }

_Form::~_Form (void)
  {
    char s [64], *sp;
    //
    DebugAddS ("Deleting Form ", Container->TextGet ()); //####
    delete Container;
    if (WindowTexture)
      TextureDestroy (Window, WindowTexture);
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
        free (CurrentPath);
        free (Filename_);
      }
    //Unlink (&FormList);
  }

void _Form::Die (void)
  {
    DieFlag = true;
  }

bool ProcessEvent (void)   // Looks for and processes events for all Forms. Return TRUE if anything happened
  {
    bool Busy;
    _Form *f;
    static word MouseKeys = 0;
    _Event Event;
    //
    Busy = false;
    // Look for internal scheduled events
    f = FormList;
    while (f)
      {
        if (f->DieFlag)
          {
            delete f;
            return true;
          }
        if (f->KeyPress)
          {
            Event.Type = etKeyDown;
            Event.Key = f->KeyPress;
            f->KeyPress = 0;
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
        if (Event.Key > 0 && Event.Key <= 16)
          if (Event.Type == etMouseDown || Event.Type == etMouseUp)
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
                TextureDestroy (f->Window, f->WindowTexture);
                f->WindowTexture = NULL;
              }
          }
        else if (Event.Type == etWindowClose)
          {
            DebugAddS ("Event: Window Delete ", f->Container->TextGet ()); //####
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
      }
    return Busy;
  }

bool FormsUpdate (void)   // Looks for events, processes everything, returns true for close
  {
    _Form *f;
    bool RenderAll;
    //
    while (ProcessEvent ())
      ;
    //
    // Perform Scheduled stuff
    f = FormList;
    while (f)
      {
        f->Container->Poll ();
        f->Container->Draw (false, {0, 0});   // Draw containers with ReDraw set
        if (f->ReRender)
          {
            f->ReRender = false;
            RenderAll = false;
            if (f->Container->Texture)
              {
                #ifdef DebugShowRenders
                DebugAdd ("Page Update:");
                #endif // DebugShowRenders
                #define DoubleBuffer
                #ifdef DoubleBuffer
                if (f->WindowTexture == NULL)
                  {
                    f->WindowTexture = TextureCreate (f->Window, f->Container->Rect.Width, f->Container->Rect.Height);
                    RenderAll = true;
                  }
                f->Container->RenderTarget_ (f->WindowTexture, {0, 0}, cBlack, f->Container->Rect);
                f->Container->Render1 (RenderAll, 0, 0, 0xFF);
                f->Container->RenderTarget_ (NULL, {0, 0}, cBlack, f->Container->Rect);
                f->Container->Render2 (0, 0);
                #else
                f->Container->RenderTarget_ (f->WindowTexture, {0, 0}, cBlack, {0, 0, f->Container->Rect.Width, f->Container->Rect.Height});
                f->Container->Render1 (RenderAll, 0, 0, 0xFF);
                #endif // DoubleBuffer
              }
            RenderPresent (f->Window, f->Alpha);
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
    Texture = NULL;
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
        if (Rect_.Width <= 0)// || Rect_.x + Rect_.Width > Parent_->Rect.Width)
          {
            Rect_.Width = Parent_->Rect.Width - Rect_.x;
            RectLock |= rlRight;
          }
        if (Rect_.Height <= 0)// || Rect_.y + Rect_.Height > Parent_->Rect.Height)
          {
            Rect_.Height = Parent_->Rect.Height - Rect_.y;
            RectLock |= rlBottom;
          }
        //if (Rect_.x + Rect_.Width == Parent_->Rect.Width)
        //  RectLock |= rlRight;
        //if (Rect_.y + Rect_.Height == Parent_->Rect.Height)
        //  RectLock |= rlBottom;
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
    // Unfocus
    if (Form)
      {
        if (Form->KeyFocus == this)
          Form->KeyFocus = NULL;
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
    if (Texture)
      TextureDestroy (Form->Window, Texture);
    Texture = NULL;
  }

_Point RenderTargetOffset;
_Rect RenderTargetClip;

bool _Container::RenderTarget_ (_Texture *Texture, _Point Offset, int Colour, _Rect Clip)
  {
    #ifdef DebugShowRenders
    char s [80], *sp;
    //
    sp = s;
    StrCat (&sp, "  Render Target ");
    NumToHex (&sp, (long int) Texture, 0);
    *sp = 0;
    DebugAdd (s);
    #endif // DebugShowRenders
    // Assume writing directly to Window
    RenderTargetOffset = {0, 0};
    // Unless writing to the Texture
    if (Texture == NULL)
      RenderTargetOffset = Offset;
    RenderTargetOffset.x -= Scroll.x;
    RenderTargetOffset.y -= Scroll.y;
    RenderTargetClip = Clip;
    return RenderTarget (Form->Window, Texture, Colour, Clip);
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
    if (Texture)   // Delete Texture (if used)
      TextureDestroy (Form->Window, Texture);
    Texture = NULL;
    dX = Width - Rect.Width;
    dY = Height- Rect.Height;
    Rect.Width = Width;
    Rect.Height = Height;
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

//#define DebugShowRendersShowInvalidates

#ifdef DebugShowRendersShowInvalidates
void DebugInvalidate (_Container *c)
  {
    char *s = (char *) malloc (1000);
    char *p = s;
    StrCat (&p, "RenderFlags 0x");
    NumToHex (&p, c->RenderFlags, -2);
    StrCat (&p, ' ');
    if (c->Text)
      StrCat (&p, c->Text);
    *p = 0;
    DebugAdd (s);
    free (s);
  }
#endif

void _Container::Invalidate (bool ReDraw)
  {
    _Container *c;
    //
    // Set Rendering flags as needed
    if (Texture == NULL)
      ReDraw = true;
    if (ReDraw)
      RenderFlags |= rRedraw;
    RenderFlags = (RenderFlags & ~rRedrawPart) | rRender | rInvalid;
    #ifdef DebugShowRendersShowInvalidates
    DebugInvalidate (this);
    #endif
    c = this; //Parent
    while (true)
      {
        if (c == NULL)
          break;
        if (c->Colour >= 0 && c->Alpha == 0xFF)   // c has a solid background
          break;   // so parent needs not be fully rerendered (just the children render bits)
        c->RenderFlags |= rRender;
        #ifdef DebugShowRendersShowInvalidates
        DebugInvalidate (c);
        #endif
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
        InvalidateAll (true);
      }
  }

void _Container::Render1 (bool RenderThis, int DestX, int DestY, byte AlphaOffset)
  {
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
        DestX += Rect.x + Shift.x;
        DestY += Rect.y + Shift.y;
        // Adjust Alpha
        AlphaOffset = (AlphaOffset * Alpha) / 0xFF;
        if (RenderFlags & rRender)
          RenderThis = true;
        if (RecSource.Width > 0 && RecSource.Height > 0)
          {
            if (RenderThis)
              {
                RenderTexture_ (Form->Window, Texture, RecSource, DestX, DestY, AlphaOffset, Text);
                Child = Children;
                while (Child)
                  {
                    //if (Child->Rect.x < Rect.Width && Child->Rect.y < Rect.Height)
                      Child->Render1 (true, DestX, DestY, AlphaOffset);
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
                          RenderTexture_ (Form->Window, Texture, RecSource_, DestX + Child->Rect.x, DestY + Child->Rect.y, Child->Alpha, Text);
                          Child->RenderFlags &= ~rClear;
                        }
                    Child = Child->Next;
                  }
                Child = Children;
                // Now Render children
                while (Child)
                  {
                    //if (Child->Rect.x < Rect.Width && Child->Rect.y < Rect.Height)
                      Child->Render1 (RenderThis, DestX, DestY, AlphaOffset);
                    Child = Child->Next;
                  }
              }
          }
      }
    RenderFlags &= ~(rRender | rClear);
  }

void _Container::Render2 (int DestX, int DestY)
  {
    _Rect Rec;
    _Container *Child;
    //
    if (Rect.Width > 0 && Rect.Height > 0)
      {
        DestX += Rect.x;
        DestY += Rect.y;
        Rec = {DestX, DestY, Rect.Width, Rect.Height};
        if (RenderFlags & rInvalid)
          RenderTexture_ (Form->Window, Form->WindowTexture, Rec, DestX, DestY, 0xFF, Text);
        else
          {
            Child = Children;
            while (Child)
              {
                //if (Child->Rect.x < Rect.Width && Child->Rect.y < Rect.Height)
                  Child->Render2 (DestX, DestY);
                Child = Child->Next;
              }
          }
        RenderFlags &= ~rInvalid;
      }
  }

void _Container::Die (void)
  {
    RenderFlags |= rDelete;
  }

void _Container::DrawBackground (_Rect Rect)
  {
    RenderTargetOffset.x += Scroll.x;
    RenderTargetOffset.y += Scroll.y;
    if (Colour >= 0)   // Colour defined
      if (ColourGrad < 0)   // Solid Colour
        DrawRectangle (Rect, -1, -1, Colour);
      else
        FillGradient (Rect, Colour, ColourGrad);
    else   // no colour defined. Transparent
      if (Texture)   // no rendering so no background
        RenderFillTransparent (Form->Window);
    RenderTargetOffset.x -= Scroll.x;
    RenderTargetOffset.y -= Scroll.y;
  }

void _Container::Draw (bool Force, _Point Offset)
  {
    _Container *Child;
    //
    // Is this even visible
    if (IsVisible () && Rect.Width > 0 && Rect.Height > 0)
      {
        // Create the Texture if absent
        if (Texture == NULL)
          Texture = TextureCreate (Form->Window, Rect.Width, Rect.Height);
        if (RenderFlags & (rRedraw) || Force)
          {
            RenderTarget_ (Texture, Offset, ColourFind (), {Offset.x, Offset.y, Rect.Width, Rect.Height});
            // Draw the background
            if (~RenderFlags & rRedrawPart)
              DrawBackground ({0, 0, Rect.Width, Rect.Height});
            // Draw stuff specific to this type of Container
            DrawCustom ();
            ScrollRegionBarsDraw ();
            if (Texture == NULL)   // no texture ...
              {
                Force = true;   // ... so impose redraw on children
                RenderFlags &= ~rRender;   // ... and nothing to render
              }
          }
        else if (Texture == NULL)   // no texture so set children background if rerendering
          {
            Child = Children;
            while (Child)
              {
                if (Child->RenderFlags & rRender)   // Child needs redrawing
                  {
                    //Child->RenderFlags |= rRedraw;   // so schedule redraw
                    Child->RenderFlags = (Child->RenderFlags &~rRender) | rRedraw;   // so schedule redraw, forget rerender
                    if (~Child->RenderFlags & rRedrawPart)
                      if (Child->Colour < 0 || !Child->Visible)   // and prepare child background if it is transparent
                        {
                          Child->RenderTarget_ (Texture, Offset, ColourFind (), {Child->Rect.x + Offset.x, Child->Rect.y + Offset.y, Child->Rect.Width, Child->Rect.Height});
                          DrawBackground (Child->Rect);
                        }
                  }
                Child = Child->Next;
              }
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
    //DrawRectangle ({0, 0, Rect.Width, Rect.Height}, cRed, cRed, -1);//####
  }

bool _Container::FontSet (char *FontFile, int Size, byte Style)
  {
    _Font *Font_;
    //
    Font_ = NULL;
    if (FontFile && *FontFile)   // Open New Font
      {
        HomeFilePath (FontFile);
        Font_ = FontCreate (Filename_, Size, Style, -1);
        if (Font_)
          StrAssignCopy (&FontFileDefault, FontFile);   // Set Default Font
      }
    else if (FontFileDefault && *FontFileDefault)
      {
        HomeFilePath (FontFileDefault);
        Font_ = FontCreate (Filename_, Size, Style, -1);
      }
    if (Font_)   // it worked
      {
        if (Font)   // Free old Font
          FontDestroy (Font);
        Font = Font_;
      }
    //Invalidate (true);
    if (!Font)
      DebugAddS ("FontSet FAIL: ", FontFile);
    return Font != NULL;
  }

bool _Container::CheckFocus (_Event *Event, _Point Offset)   // Take or lose keyboard Focus
  {
    _Container *c;
    bool Res;
    //
    Res = false;
    // Take focus if next in line
    if (Event->Type == etNext)
      {
        Event->Type = etNone;
        Form->KeyFocus = this;
        Invalidate (true);
        Res = true;
      }
    if (!Form->EventLock)
      {
        // Take focus if clicked on
        if (IsEventMine (Event, Offset))
          if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)
            if (Form->KeyFocus != this)
              {
                c = Form->KeyFocus;
                Form->KeyFocus = this;
                if (c)
                  c->Invalidate (true);
                Invalidate (true);
                Res = true;
              }
        // Lose focus if tab pressed
        if (Event->Type == etKeyDown)
          if (Form->KeyFocus == this)
            if (Event->Key == tab)
              {
                Event->Type = etNext;
                Form->KeyFocus = NULL;
                Invalidate (true);
              }
      }
    return Res;
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
        RectToStr (&ps, &Container->Rect);
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
    if (!IsVisible () || !Enabled)
      return false;
    /*if (Event->Type != etMouseMove)   //####
      {
        char s [120], *ps;
        ps = s;
        StrCat (&ps, "Event ");
        NumToStr (&ps, Event->Type);
        StrCat (&ps, " - ");
        RectToStr (&ps, &Rect);
        StrCat (&ps, ": ");
        StrCat_ (&ps, Text, 80);
        *ps = 0;
        DebugAdd (s);
      }*/
    //if (EventPreview)
    //  if (EventPreview (this, Event, OffsetX, OffsetY))
    //    return true;
    //DebugShowMouseEvent (Event, this);
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
      {
        Invalidate (true);
        RenderFlags &= ~rRedrawPart;
      }
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
                if (PagingTransitionTime == 0 || Texture == NULL)   // no transitions, just flip over
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
    bool Res;
    //
    if (Form->EventLock)
      return Form->EventLock == this;
    Res = false;
    if (IsEventMouse (Event) && IsEventWithin (Event, Offset))
      Res = true;
    else if (Event->Type == etKeyDown || Event->Type == etKeyUp)
      {
        if (Form->KeyFocus == this)
          Res = true;
      }
    //else   // non mouse kbd events
    //  Res = true;
    return Res;
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

_Point _Container::TextMeasure (_Font *Font_, char *Text)
  {
    if (Font_ == NULL)
      Font_ = FontFind ();
    if (Font_ == NULL)
      return {0, 0};
    TextCallBackBitmap = BitmapFind ();
    return TextSize (Font_, Text);
  }

const int OutlineX [] = {-1,  1, -1, 1,  -1, 1,  0, 0}; //{0, 2, 0, 2};
const int OutlineY [] = {-1, -1,  1, 1,   0, 0, -1, 1}; //{0, 0, 2, 2};

void _Container::TextOutAligned (_Font *Font_, _Rect Rect, char *Text, _Align Align, _Align AlignVert)
  {
    _Point Size;
    int dX, dY;
    int cFG, cBG;
    byte stl;
    //
    if (Text == NULL)
      return;
    if (Font_ == NULL)
      Font_ = FontFind ();
    if (Font_ == NULL)
      return;
    TextCallBackBitmap = BitmapFind ();
    cFG = Font_->Colour;
    cBG = Font_->ColourBG;
    stl = Font_->Style;
    if (Font_->Colour < 0)
      Font_->Colour = ColourTextFind ();
    if (!IsEnabled ())
      {
        Font_->Colour = ColourAverage (Font_->Colour, ColourFind ());
        Font_->Style &= ~(fsShadow | fsOutline);
      }
    if (Align != aLeft)
      {
        Size = TextMeasure (Font_, Text);
        dX = Rect.Width - Size.x;
        if (Align == aRight)
          Rect.x += dX;
        else if (Align == aCenter)
          Rect.x += dX / 2;
      }
    dY = Rect.Height - Font_->YAdvance;
    if (AlignVert == aCenter)
      Rect.y += dY / 2;
    else if (AlignVert == aRight)
      Rect.y += dY;
    if (cBG < 0)
      Font_->ColourBG = ColourFind ();
    TextRender (Form->Window, Font_, Text, {Rect.x + RenderTargetOffset.x, Rect.y + RenderTargetOffset.y});
    Font_->Colour = cFG;
    Font_->ColourBG = cBG;
    Font_->Style |= stl & (fsShadow | fsOutline);
  }

// Word wrap text out.  !Write => measure only
// PosY is starting & finishing position
void _Container::TextOutWrap_ (_Rect Rect_, char *St, _Align Align, int *PosY, bool Write)
  {
    int BOL, BOW, EOW, x;
    char *StSub;   // Copy of St
    _Point Size;
    char *Split;
    //char *Param;
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
            /*if (BOW == BOL)   // Start of line
              if (St [BOL] == '~')   // Change Colour
                {
                  Param = &St [BOL + 1];
                  Colour_ = StrGetNumBaseFixed (&Param, 6, 16);
                  BOL = Param - St;
                  BOW = BOL;
                }*/
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
            Size = TextMeasure (Font, &StSub [BOL]);
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
                        TextOutAligned (NULL, {Rect_.x, *PosY, Rect_.Width, Size.y}, &StSub [BOL], aLeft, aLeft);
                        TextOutAligned (NULL, {Rect_.x, *PosY, Rect_.Width, Size.y}, Split + 1, aRight, aLeft);
                      }
                    else
                      TextOutAligned (NULL, {Rect_.x, *PosY, Rect_.Width, Size.y}, &StSub [BOL], Align, aLeft);
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

// Text out wrapping at words. Force wrap at \n. Split line full justify at \t
void _Container::TextOutWrap (_Rect Rect, char *Text, _Align Align, _Align AlignVert)
  {
    _Font *Font;
    _Point Size;
    int x, y;
    //
    Font = FontFind ();
    if (Font)
      if (Font->Style & fsRotate)
        {
          Size = TextMeasure (Font, Text);
          x = (Rect.Width - Size.y) / 2;
          y = (Rect.Height - Size.x) / 2;
          if (Align == aCenter)
            TextOutAligned (Font, {Rect.x + x, Rect.y + y + Size.x, 0, 0}, Text, aLeft, aLeft);
          else if (Align == aLeft)
            TextOutAligned (Font, {Rect.x + x, Rect.y + Rect.Height, 0, 0}, Text, aLeft, aLeft);
          else if (Align == aRight)
            TextOutAligned (Font, {Rect.x + x, Rect.y + Size.x, 0, 0}, Text, aLeft, aLeft);
        }
      else
        {
          y = Rect.y;
          if (AlignVert == aLeft)   // Top
            TextOutWrap_ (Rect, Text, Align, &y, true);
          else
            {
              y = 0;
              TextOutWrap_ (Rect, Text, Align, &y, false);
              if (AlignVert == aCenter)
                y = Rect.y + (Rect.Height - y) / 2;
              else   // Bottom
                y = Rect.y + Rect.Height - y;
              TextOutWrap_ (Rect, Text, Align, &y, true);
            }
        }
  }

void _Container::TextOutWrapReverse (_Rect Rect, char *St, _Align Align, _Point Margin)
  {
    int cFG, cText;
    _Font *Font_;
    //
    cText = ColourTextFind ();
    DrawRectangle ({0, Rect.y, Rect.Width, Rect.Height}, -1, -1, cText);//ColFont cSelected
    Font_ = FontFind ();
    cFG = Font_->Colour;
    Font_->Colour = cText ^ cWhite;//cTextBgnd cTextHighlight
    Font_->ColourBG = cText;//ColFont cSelected
    TextOutWrap (AddMargin (Rect, Margin), St, Align, aCenter);
    Font_->Colour = cFG;
    Font_->ColourBG = -1;
  }

void _Container::DrawLine (int x1, int y1, int x2, int y2, int Colour, int Width)
  {
    int i;
    //
    x1 += RenderTargetOffset.x;
    y1 += RenderTargetOffset.y;
    x2 += RenderTargetOffset.x;
    y2 += RenderTargetOffset.y;
    if (Colour >= 0)
      for (i = 0; i < Width; i++)
        if (i == 0)
          RenderDrawLine (Form->Window, Colour, x1, y1, x2, y2);
        else
          {
            RenderDrawLine (Form->Window, Colour, x1 + i, y1, x2 + i, y2);
            RenderDrawLine (Form->Window, Colour, x1 - i, y1, x2 - i, y2);
            RenderDrawLine (Form->Window, Colour, x1, y1 + i, x2, y2 + i);
            RenderDrawLine (Form->Window, Colour, x1, y1 - i, x2, y2 - i);
          }
  }

void _Container::DrawRectangle (_Rect Rect_, int ColourEdgeTL, int ColourEdgeBR, int ColourFill)
  {
    _Point BR;
    //
    Rect_.x += RenderTargetOffset.x;
    Rect_.y += RenderTargetOffset.y;
    if (ColourFill >= 0)
      RenderFillRect (Form->Window, Rect_, ColourFill);
    //####
    BR = BottomRight (&Rect_);
    if (ColourEdgeTL >= 0)
      {
        if (Rect_.Height)
          RenderDrawLine (Form->Window, ColourEdgeTL, Rect_.x, BR.y, Rect_.x, Rect_.y);
        if (Rect_.Width)
          RenderDrawLine (Form->Window, ColourEdgeTL, Rect_.x, Rect_.y, BR.x, Rect_.y);
      }
    if (ColourEdgeBR >= 0)
      {
        if (Rect_.Height)
          RenderDrawLine (Form->Window, ColourEdgeBR, BR.x, Rect_.y, BR.x, BR.y);
        if (Rect_.Width)
          RenderDrawLine (Form->Window, ColourEdgeBR, BR.x, BR.y, Rect_.x, BR.y);
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
    //
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
  }

void _Container::DrawBorder (_Border Border, int Indent)
  {
    int Colour;
    //
    Colour = Parent->ColourFind ();
    DrawBorder ({0, 0, Rect.Width, Rect.Height}, Border, Colour, Indent);
  }

void _Container::DrawBezel (int Width)
  {
    int i, c;
    //
    RenderTargetOffset.x += Scroll.x;
    RenderTargetOffset.y += Scroll.y;
    for (i = 0; i < Width; i++)
      DrawBorder (bLowered, i);
    if (Form->KeyFocus == this)
      {
        c = ColourAdjust (ColourTextFind (), 150);
        DrawRectangle ({i, i, Rect.Width - i - i, Rect.Height - i - i}, c, c, -1);
        //i++;
        //DrawRectangle ({i, i, Rect.Width - i - i, Rect.Height - i - i}, c, c, -1);
      }
    RenderTargetOffset.x -= Scroll.x;
    RenderTargetOffset.y -= Scroll.y;
  }

#define TabCorners 2

void _Container::DrawTab (_Rect Rec, int ColourReference, int ColourBG, bool Selected)
  {
    _Point BR;
    int ColTL, ColBR;
    int i;
    //
    BR = BottomRight (&Rec);
    if (ButtonStyle == bsNormal)
      {
        ColTL = BorderColourTL (ColourReference);
        ColBR = BorderColourBR (ColourReference);
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
        ColTL = ColourAdjust (ColourTextFind (), 140);
        if ((ButtonStyle == bsFlat && Rec.x == 0) || (ButtonStyle == bsSimple && Selected))
          DrawLine (Rec.x, BR.y, Rec.x, Rec.y, ColTL);   // Left
        if (ButtonStyle == bsFlat || Selected)
          {
            DrawLine (Rec.x, Rec.y, BR.x, Rec.y, ColTL);   // Top
            DrawLine (BR.x, Rec.y, BR.x, BR.y, ColTL);   // Right
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
    sin = Sin (Angle);
    cos = Cos (Angle);
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

void _Container::DrawScrollBar (bool Vert, int Bar1, int Bar2)
  {
    int Col [3];   // ColBG, ColBorder, ColBar
    int i, j;
    //
    Col [0] = ColourAdjust (cForm1, 150);   // ColBG
    Col [1] = ColourAdjust (cForm1, 70);   // ColBorder
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
    for (i = 0; i < 8; i++)
      if (Vert)
        DrawLine (Rect.Width - ScrollBarWidth + 3, j + i, Rect.Width - 4, j + i, Col [i % 3], 1);
      else
        DrawLine (j + i, Rect.Height - ScrollBarWidth + 3, j + i, Rect.Height - 4, Col [i % 3], 1);
    // corner
    DrawRectangle ({Rect.Width - ScrollBarWidth, Rect.Height - ScrollBarWidth, ScrollBarWidth, ScrollBarWidth}, -1, -1, ColourFind ());
  }

void _Container::ScrollBarsDraw (byte Bars, _Point Value, _Point Content)
  {
    int Bar1, Bar2;
    //
    if (Bars & sbHorz)
      {
        Bar1 = Value.x * (Rect.Width - 3 * ScrollBarWidth) / (Content.x - 1);
        Bar2 = Bar1 + 2 * ScrollBarWidth - 1;
        DrawScrollBar (false, Bar1, Bar2);
      }
    if (Bars & sbVert)
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
          {
            Drag = {-1, -1};
            Form->EventLock = NULL;
          }
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
                      Form->EventLock = this;
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
                  Form->EventLock = this;
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
          {
            Drag = {-1, -1};
            Form->EventLock = NULL;
          }
        ScrollOld = Scroll;
        if (SizeVirtual.x > Rect.Width)   // we will have a horizontal scroll bar
          {
            Res = true;
            if (Event->Type == etMouseMove && Event->MouseKeys == Bit [KeyMouseLeft - 1] && Drag.x >= 0)   // Drag with left mouse button
              Scroll.x = DragVal.x + (Event->X - Drag.x) * SizeVirtual.x / Rect.Width;
            else
              if (Event->Y - Offset.y >= Rect.Height - ScrollBarWidth)   // Click is potentially on the hor scroll bar
                if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft)   // My left click
                  {
                    Drag.x = Event->X;
                    DragVal.x = Scroll.x;
                    Form->EventLock = this;
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
        if (!Res && SizeVirtual.y > Rect.Height)   // we will have a scroll bar
          {
            Res = true;
            if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelDown)
              Scroll.y += 16;
            else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelUp)
              Scroll.y -= 16;
            else if (Event->Type == etMouseDown && Event->Key == KeyMouseLeft &&   // My left click
                Event->X - Offset.x >= Rect.Width - ScrollBarWidth)   // Click is on the vert scroll bar
              {
                Drag.y = Event->Y;
                DragVal.y = Scroll.y;
                Form->EventLock = this;
              }
            else if (Event->Type == etMouseMove && Event->MouseKeys == Bit [KeyMouseLeft - 1] && Drag.y >= 0)   // Drag with left mouse button
              Scroll.y = DragVal.y + (Event->Y - Drag.y) * SizeVirtual.y / Rect.Height;
            else
              Res = false;
          }
        if (Res)
          {
            Limit (&Scroll.y, 0, SizeVirtual.y - Rect.Height + ScrollBarWidth);
            Limit (&Scroll.x, 0, SizeVirtual.x - Rect.Width + ScrollBarWidth);
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
    //
    RenderTargetOffset.x += Scroll.x;
    RenderTargetOffset.y += Scroll.y;
    Res = false;
    if (SizeVirtual.y > Rect.Height)
      {
        Res = true;
        h = Rect.Height - ScrollBarWidth;
        Bar1 = h * Scroll.y / SizeVirtual.y;
        Bar2 = h * h / SizeVirtual.y;
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
    RenderTargetOffset.x -= Scroll.x;
    RenderTargetOffset.y -= Scroll.y;
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
    rDest.x += RenderTargetOffset.x;
    rDest.y += RenderTargetOffset.y;
    RenderBitmap (Form->Window, Bitmap, rSrc, rDest, Transparent);
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
    bool DownOld, HoverOld;;
    _Container *Sibling;
    _Button *SiblingButton;
    bool Res;
    //
    Res = false;
    DownOld = Down;
    HoverOld = Hover;
    if (IsEventMine (Event, Offset))   // Either by location OR Forced
      {
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
            Form->EventLock = this;
          }
        else if (Event->Type == etMouseUp && Event->Key == KeyMouseLeft)   // Mouse Up
          {
            Res = true;
            if (Toggle)
              Down = !Flag;
            else   // not Toggle
              Down = Grouped;
          }
        if (Down != DownOld)   // Changed so Redraw and Action
          {
            Invalidate (true);
            if (Action)   // Call Handler
              Action (this);
          }
        if (ButtonStyle != bsNormal)
          {
            Hover = IsEventWithin (Event, Offset);
            if (Hover != HoverOld)
              Invalidate (true);
          }
        if (Hover || Event->MouseKeys)
          Form->EventLock = this;
        else
          Form->EventLock = NULL;
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
    int r;
    int cText, cCircle;
    _Point Size;
    int xCircle, xText;
    //
    cText = ColourTextFind ();
    Size = TextMeasure (Font, "Ag");
    cCircle = ColourAdjust (cText, 140);
    r = Size.y * 3 / 8;
    xText = 4 * r;
    xCircle = 2 * r;
    if (Align == aRight)
      {
        xText = TextMargin;
        xCircle = Rect.Width - 2 * r;
      }
    TextOutWrap ({xText, 0, Rect.Width - 4 * r, Rect.Height}, Text, Align, aCenter);
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
    Size = TextMeasure (Font, "Ag");
    h = Size.y * 3 / 8;
    x0 = h;
    y0 = Rect.Height / 2 - h;
    xText = 4 * h;
    if (Align == aRight)
      {
        x0 = Rect.Width - 3 * h;
        xText = 0;
      }
    TextOutWrap ({xText, 0, Rect.Width - 4 * h, Rect.Height}, Text, Align, aCenter);
    DrawRectangle ({x0, y0, h * 2, h * 2}, cBox, cBox, -1);
    if (Hover)
      DrawRectangle ({x0 + 1, y0 + 1, h * 2 - 2, h * 2 - 2}, cBox, cBox, -1);
    if (Down)
      {
        // Tick
        x0 += h / 2;
        y0 += h / 3;
        h = h * 2 / 3;
        DrawLine (x0, y0 + h, x0 + h, y0 + 2*h, cText, 2);
        DrawLine (x0 + h, y0 + 2*h, x0 + 3*h, y0, cText, 2);
        /*/ Cross
        DrawLine (x0 + 1, y0 + 1, x0 + h - 2, y0 + h - 2, c1, 2);
        DrawLine (x0 + h - 2, y0 + 1, x0 + 1, y0 + h - 2, c1, 2);*/
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
  }

void _Label::DrawCustom (void)
  {
    int Margin;
    //
    Margin = 0;
    if (Border != bNone)
      Margin = TextMargin;
    TextOutWrap ({Margin, Margin, Rect.Width - 2 * Margin, Rect.Height - 2 * Margin}, Text, Align, AlignVert);
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
    NumToStrDecimals (&x, abs (Value), 2);
    if (Value < 0)
      *x++ = ')';
    StrCat (&x, Text);
    *x = 0;
    TextOutAligned (NULL, {Margin, 0, Rect.Width - 2 * Margin, Rect.Height}, St, aRight, aCenter);
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
    TextOutAligned (NULL, {Margin, 0, Rect.Width - 2 * Margin, Rect.Height}, St, aRight, aCenter);
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
    TextOutAligned (NULL, {Margin, 0, Rect.Width - 2 * Margin, Rect.Height}, Res, Align, aCenter);
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
    FontSet (FontFileDefault, Abs (Height), Height < 0 ? fsBold : fsNone);
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
    //
    TextOutAligned (NULL, {SlidX, 0, Rect.Width, Rect.Height}, Text, aLeft, aCenter);
    Size = TextMeasure (Font, Text);
    if (-SlidX >= Size.x)
      {
        SlidX = Rect.Width;
        Tick = ClockMS ();
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

_EditNumber::_EditNumber (_Container *Parent, _Rect Rect, char *Text, int Max_, _Action Action_) : _Container (Parent, Rect, Text)
  {
    Max = Max_;
    Value = 0;
    Action = Action_;
    Colour = cTextBgnd;
    ColourGrad = -1;
    if (!Form->KeyFocus)
      Form->KeyFocus = this;
  }

bool _EditNumber::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    int Ch;
    int ValueOld;
    bool Res;
    //
    Res = false;
    ValueOld = Value;
    if (CheckFocus (Event, Offset))
      NewFocus = true;
    if (IsEventMine (Event, Offset))
      {
        if (Event->Type == etMouseDown)
          {
            Res = true;
            if (Event->Key == KeyMouseWheelUp)
              Value++;
            else if (Event->Key == KeyMouseWheelDown)
              Value--;
          }
        // Process Key presses
        else if (Event->Type == etKeyDown && Form->KeyFocus == this)
          {
            Res = true;
            Ch = Event->Key;
            if (Ch == bs)
              Value /= 10;
            else if ((Ch >= '0') && (Ch <= '9'))
              {
                if (NewFocus)
                  Value = 0;
                Value = Value * 10 + (Ch - '0');
              }
            else if (Ch == KeyUp)
              Value++;
            else if (Ch == KeyDown)
              Value--;
            else
              Res = false;
            NewFocus = false;
          }
      }
    Limit (&Value, 0, Max);
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
    //_Point Size;
    //
    St = (char *) malloc (StrLen (Text) + 16);
    x = St;
    if (Text)
      StrCat (&x, Text);
    NumToStr (&x, Value);
    *x = 0;
    //Size = TextMeasure (Font, St);
    //TextOutAligned (NULL, {0, (Rect.Height - Size.y) / 2, Rect.Width - 4, Rect.Height}, St, aRight, aCenter);
    TextOutWrap (AddMargin ({0, 0, Rect.Width, Rect.Height}, {TextMargin, 0}), St, aRight, aCenter);
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
    ValueMin = Min;
    ValueMax = Max;
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
            Form->EventLock = this;
          }
        else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelUp)   // wheel up
          Value++;
        else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelDown)   // wheel down
          Value--;
        else if (Event->Type == etMouseUp)
          {
            MouseDown = false;
            Form->EventLock = NULL;
          }
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
    int Pos;
    _Rect Button;
    int Col;
    //
    hx = Rect.Width / 2;
    hy = Rect.Height / 2;
    Col = ColourFind ();
    if (Vertical)
      {
        DrawBorder ({hx, SliderMargin, 0, Rect.Height - 2 * SliderMargin}, bMote, Col, 0);
        Pos = (Rect.Height - 2 * SliderMargin) * (ValueMax - Value) / (ValueMax - ValueMin);
        Button = {hx - 2 * SliderMargin, Pos, 4 * SliderMargin, 2 * SliderMargin};
      }
    else
      {
        DrawBorder ({SliderMargin, hy, Rect.Width - 2 * SliderMargin, 0}, bMote, Col, 0);
        Pos = (Rect.Width - 2 * SliderMargin) * (Value - ValueMin) / (ValueMax - ValueMin);
        Button = {Pos, hy - 2 * SliderMargin, 2 * SliderMargin, 4 * SliderMargin};
      }
    DrawRectangle (Button, -1, -1, Col);
    DrawBorder (Button, bRaised, Col, 0);
    DrawBorder (Button, bRaised, Col, 1);
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
            Form->EventLock = this;
          }
        else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelUp)
          Value++;
        else if (Event->Type == etMouseDown && Event->Key == KeyMouseWheelDown)
          Value--;
        else if (Event->Type == etMouseUp)
          {
            MouseX = MouseY = -1;
            Form->EventLock = NULL;
          }
        else if (Event->Type == etMouseMove)
          {
            if (MouseX >= 0)
              Value = MouseValue + ((MouseY - Event->Y) + (Event->X - MouseX)) * (ValueMax - ValueMin) / Rect.Width;
          }
        else
          Res = false;
        Limit (&Value, ValueMin, ValueMax);
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
    int c0, cf;
    int angle, i;
    int Margin;
    //
    Rad = Min (Rect.Width, Rect.Height) / 3;
    c0 = ColourFind ();
    cf = ColourTextFind ();
    for (i = 0; i < Markers; i++)
      {
        angle = (180 + 45) - (i * (360 - 90) + (Markers / 2)) / (Markers - 1);
        DrawRadius (Rect.Width / 2, Rect.Width / 2, Rad + 4, Rad + 4, angle, cf, 1);
      }
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
        DrawCircle (x + Cos (((i + Counter) % 8) * 45) * (float) R, y + Sin (((i + Counter) % 8) * 45) * (float) R, r, -1, c);
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
    //Bitmap = NULL;
    Stretch = sNormal;
    Align = aCenter;
    AlignVert = aCenter;
  }

_Image::~_Image (void)
  {
    //if (Bitmap)
    //  BitmapDestroy (Bitmap);
  }

void _Image::DrawCustom (void)
  {
    _Rect rSrc, rDest;
    _Point Size, Size_;
    //
    if (Bitmap == NULL && Text)
      {
        HomeFilePath (Text);
        Bitmap = BitmapLoad (Form->Window, Filename_);
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

#define ListMargin 4

_SelectionList::_SelectionList (_Container *Parent, _Rect Rect_, char *Text_, _Action Action_) : _Container (Parent, Rect_, Text_)
  {
    _Point Size;
    int n;
    char *Item, *ps;
    //
    Hovering = -1;
    Selected = -1;
    Align = aLeft;
    Action = Action_;
    Colour = cForm1;
    ColourGrad = cForm2;
    if (cMenu >= 0)
      {
        Colour = cMenu;
        ColourGrad = -1;
      }
    ItemSize = {0, 0};
    ps = Text;
    n = 0;
    while (true)
      {
        Item = StrGetItem (&ps, '\t');
        if (!Item)
          break;
        Size = TextMeasure (Font, Item);
        ItemSize.x = Max (ItemSize.x, Size.x);
        ItemSize.y = Max (ItemSize.y, Size.y);
        free (Item);
        n++;
      }
    ItemSize.y += 2 * TextMargin;
    Rect.Width = ItemSize.x + 2 * ListMargin;
    Rect.Height = ItemSize.y * n + 2 * ListMargin;
    if (Rect.y + Rect.Height > Parent->Rect.Height)
      Rect.y = Parent->Rect.Height - Rect.Height;
    Form->EventLock = this;
  }

void _SelectionList::DrawCustom (void)
  {
    int i, y;
    char *Item, *ps;
    _Rect Rec;
   //
    if (!Text)
      return;
    i = 0;
    y = ListMargin;
    ps = Text;
    while (true)
      {
        Item = StrGetItem (&ps, '\t');
        if (!Item)
          break;
        Rec = {0, y, Rect.Width, ItemSize.y};
        if (i == Hovering)
          TextOutWrapReverse (Rec, Item, Align, {ListMargin, 0});
        else
          TextOutWrap (AddMargin (Rec, {ListMargin, 0}), Item, Align, aCenter);
        free (Item);
        y += ItemSize.y;
        i++;
      }
    DrawBorder (bRaised, 0);
  };

bool _SelectionList::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    bool Res;
    int h;
    //
    Res = false;
    if (IsEventMine (Event, Offset))
      if (Event->Type == etMouseMove)
        {
          h = Hovering;
          if (ItemSize.y > 0 && Event->Y >= Offset.y + ListMargin && Event->Y < Offset.y + Rect.Height - ListMargin)
            Hovering = (Event->Y - Offset.y - ListMargin) / ItemSize.y;
          else
            Hovering = -1;
          if (Hovering != h)
            Invalidate (true);
          Res = true;
        }
      else if (Event->Type == etMouseDown || Event->Type == etMouseUp)   // Selected
        {
          if (IsEventMine (Event, Offset))
            Selected = Hovering;
          if (Action)
            Action (this);
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
        StepSpace (&ps);
        StrAssignCopy (&List, ps);
        Size = TextMeasure (Font, Name);
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
      TextOutWrapReverse (Rec, Name, aCenter, {ListMargin, 0});
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
        if (Event->Type == etMouseDown)   // Activate
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
  }

void ActionMenuPopupSelectionList (_SelectionList *SelectionList)
  {
    _MenuPopup *MenuPopup;
    //
    MenuPopup = (_MenuPopup *) SelectionList->DataContainer;
    if (MenuPopup->SelectionList)
      {
        MenuPopup->Selected = SelectionList->Selected;
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
    if (DropList->SelectionList)
      {
        DropList->Selected = SelectionList->Selected;
        if (DropList->Action)
          DropList->Action ((_Container *) DropList);
        DropList->SelectionList->Die ();
        DropList->SelectionList = NULL;
        DropList->Invalidate (true);
      }
  }

/*
void ActionDropList (_ButtonArrow *Button)
  {
    _DropList *DropList;
    //
    DropList = (_DropList *) Button->DataContainer;
    if (Button->Down)
      {
        DropList->SelectionList = new _SelectionList (DropList->Parent, {DropList->Rect.x, DropList->Rect.y + DropList->Rect.Height, 16, 16}, DropList->Text, (_Action) ActionDropListSelectionList);
        DropList->SelectionList->DataContainer = DropList;   // remember where we came from
      }
  }
*/

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
        Size = TextMeasure (Font, Item);
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
        TextOutAligned (NULL, Rec, Item, aCenter, aLeft);
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
                Size = TextMeasure (Font, Item);
                free (Item);
                x += Size.x + 2 * TabMargin;
                if (Event->X + Offset.x < x)   // this tab selected
                  break;
                i++;
              }
            if (i != Selected)
              {
                Selected = i;
                if (Form->KeyFocus)
                  {
                    c = Form->KeyFocus;
                    Form->KeyFocus = NULL;
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
// SOFT KEYBOARD

char *Keyboard1 [] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "  ", NULL};
char *Keyboard2 [] = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", NULL};
char *Keyboard3 [] = {"A", "S", "D", "F", "G", "H", "J", "K", "L", NULL};
char *Keyboard4 [] = {"Z", "X", "C", "V", "B", "N", "M", NULL};

void _ButtonAction (_Container *Container)
  {
    char *s;
    _Button *Button;
    //
    Button = (_Button *) Container;
    if (Button)
      if (Button->Down)
        {
          s = Button->TextGet ();
          if (s && s [0])
            Button->Form->KeyPress = s [0];
        }
  }

_SoftKeyboard::_SoftKeyboard (_Container *Parent, _Rect Rect_) : _Container (Parent, Rect_, NULL)
  {
    int y;
    int Gap = 8;
    //
    ButtonSpacing.x = (Rect.Width + Gap) / 12;
    ButtonSpacing.y = (Rect.Height + Gap) / 4;
    ButtonSize.x = ButtonSpacing.x - Gap;
    ButtonSize.y = ButtonSpacing.y - Gap;
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
    while (*Keys)
      {
        if (StrLen (*Keys) == 1)
          new _Button (this, {Start.x, Start.y, ButtonSize.x, ButtonSize.y}, *Keys, _ButtonAction);
        else
          new _ButtonArrow (this, {Start.x, Start.y, ButtonSize.x + ButtonSpacing.x, ButtonSize.y}, *Keys, _ButtonAction, dLeft);
        Start.x += ButtonSpacing.x;
        Keys++;
      }
  }
