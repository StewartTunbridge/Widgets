/////////////////////////////////////////////////////////////////////////
//
// Widget SDL Driver
//
/////////////////////////////////////////////////////////////////////////


#include <string.h>
#include <cmath>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "WidgetsDriver.hpp"

extern void DebugAdd (const char *St);   // defined in application
extern void DebugAddS (const char *s1, const char *s2);   // defined in Widgets.cpp
extern void DebugAddInt (const char *St, int n);
//extern void DebugAddR (const char *St, _Rect *r);
extern void DebugAddP (const char *St, _Point p);
extern void DebugAddHex (const char *St, int p);

typedef struct
  {
    SDL_Window *Window;
    SDL_Renderer *Renderer;
    SDL_Surface *Icon;
  } __Window;

char *ClipboardData = NULL;
_WindowID ClipboardWinID = NULL;

bool ForceSoftwareRendering = true;//false;

int TargetColour;

const char SDLError [] = "** SDL ERROR: ";

void DebugAddSDLError (char *St)
  {
    const char *em = SDL_GetError ();
    if (em)
      {
        char *Message = (char *) malloc (StrLen (St) + StrLen (SDLError) + StrLen (em) + 32);
        char *x = Message;
        StrCat (&x, SDLError);
        if (St)
          {
            StrCat (&x, St);
            StrCat (&x, ": ");
          }
        StrCat (&x, em);
        *x = 0;
        DebugAdd (Message);
        free (Message);
      }
  }

void RectToSDL_Rect (_Rect *Src, SDL_Rect *Dst)
  {
    Dst->x = Src->x;
    Dst->y = Src->y;
    Dst->w = Src->Width;
    Dst->h = Src->Height;
  }

Uint32 SurfaceGetPixel (SDL_Surface *Surface, int x, int y)
  {
    int bpp;   // Bytes / Pixel
    int Index;
    byte *pp;   // Pointer to a Pixel
    int Res;
    //
    Res = 0;
    if (x >= 0 && y >= 0 && x < Surface->w && y < Surface->h)
      {
        bpp = Surface->format->BytesPerPixel;
        Index = (y * Surface->pitch) + (x * bpp);
        pp = (Uint8*) Surface->pixels + Index;
        //Res = pp [0] | (pp [1] << 8) | (pp [2] << 16 | pp [3] << 24);
        Res = *((int *) pp);
      }
    //return *(Uint32 *)pp;
    return Res;
  }

void SurfaceSetPixel (SDL_Surface *Surface, int x, int y, int Colour)
  {
    int Index;
    //byte *pByte;
    //
    if (x >= 0 && y >= 0 && x < Surface->w && y < Surface->h)
      {
        Index = (y * Surface->pitch) + (x * Surface->format->BytesPerPixel);
        *((int *) (Surface->pixels + Index)) = Colour;//%%%%
        /*pByte = (byte *) (Surface->pixels + Index);
        *pByte++ = ColourR (Colour);
        *pByte++ = ColourG (Colour);
        *pByte++ = ColourB (Colour);
        *pByte++ = ColourA (Colour);*/
      }
  }

typedef struct
  {
    int SDLCode;
    int Key;
  } _SDLCodeToKey;

const _SDLCodeToKey SDLCodeToKeys [] =
  {
    {0x40000052, KeyUp},
    {0x40000051, KeyDown},
    {0x40000050, KeyLeft},
    {0x4000004F, KeyRight},
    {0x40000049, KeyIns},
    {0x0000007F, KeyDel},
    {0x4000004A, KeyHome},
    {0x4000004D, KeyEnd},
    {0x4000004B, KeyPageUp},
    {0x4000004E, KeyPageDown},
    {0x4000003A, KeyF1+0},
    {0x4000003B, KeyF1+1},
    {0x4000003C, KeyF1+2},
    {0x4000003D, KeyF1+3},
    {0x4000003E, KeyF1+4},
    {0x4000003F, KeyF1+5},
    {0x40000040, KeyF1+6},
    {0x40000041, KeyF1+7},
    {0x40000042, KeyF1+8},
    {0x40000043, KeyF1+9},
    {0x40000044, KeyF1+10},
    {0x40000045, KeyF1+11},

    {0x400000E0, KeyCntl},
    {0x400000E4, KeyCntl},
    {0x400000E1, KeyShift},
    {0x400000E5, KeyShift},
    {0x400000E2, KeyAlt},
    {0x400000E6, KeyAlt},

    {0x40000039, 0},   // Caps Lock - ignore 4 now - I hate caps lock

    {-1, 0}
  };

const char *ShiftPairs = "`~1!2@3#4$5%6^7&8*9(0)-_=+[{]}\\|;:\'\",<.>/?";

void ProcessSDLEventKey (_Event *Event, SDL_KeyboardEvent *SDLEventKey, int *Shift)
  {
    int Res;
    SDL_Scancode sc;
    _SDLCodeToKey const *CodeKey;
    char *p;
    //
    Res = -1;
    sc = SDLEventKey->keysym.scancode;
    //DebugAddHex ("Key Code", SDLEventKey->keysym.sym); // ####
    if (*Shift == KeyCntl && sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
      Res = Cntrl ('A' + sc - SDL_SCANCODE_A);
    else
      {
        CodeKey = SDLCodeToKeys;
        while (true)
          {
            if (CodeKey->SDLCode < 0)   // End of table
              {
                if (SDLEventKey->keysym.sym < 0x80)
                  Res = SDLEventKey->keysym.sym;
                else
                  {
                    Res = KeyUNICODE;
                    Event->UTF = SDLEventKey->keysym.sym;
                  }
                break;
              }
            if (CodeKey->SDLCode == SDLEventKey->keysym.sym)
              {
                Res = CodeKey->Key;
                break;
              }
            CodeKey++;
          }
        *Shift = 0;
        if (SDLEventKey->keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT))
          *Shift |= KeyShift;
        if (SDLEventKey->keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
          *Shift |= KeyCntl;
        if (SDLEventKey->keysym.mod & (KMOD_LALT | KMOD_RALT))
          *Shift |= KeyAlt;
        if (Res < 0)
          Res = *Shift;
        else if (Res >= 0x80 && Res != KeyUNICODE)
          Res |= *Shift;
        else
          {
            if (*Shift & KeyShift)   // Special Shift chars
              {
                p = StrPos ((char *) ShiftPairs, Res);
                if (p)
                  {
                    Res = p [1];
                    *Shift &= ~KeyShift;
                  }
              }
            if (Res >= '@')   // "Alpha" keys
              {
                if (*Shift & KeyAlt)   // Alt alpha keys are always uppercase
                  Res = Res & ~0x20 | *Shift;
                else if (*Shift & KeyCntl)   // Control alphas
                  Res &= 0x1F;
                else if (*Shift & KeyShift)
                  Res &= ~0x20;
              }
            else
              Res |= *Shift;
          }
      }
    Event->Key = Res;
  }

void MouseCursor (_Window *Window, bool Show)
  {
    SDL_ShowCursor (Show ? SDL_ENABLE : SDL_DISABLE);
  }

_Point MousePos (void)
  {
    _Point Mouse;
    //
    SDL_GetGlobalMouseState (&Mouse.x,&Mouse.y);
    return Mouse;
  }

void WindowGetPosSize (_Window *Window, int *xPos, int *yPos, int *Width, int *Height)
  {
    int x, y, w, h;
    //
    if (xPos || yPos)
      SDL_GetWindowPosition (((__Window*) Window)->Window, &x, &y);
    if (Width || Height)
      SDL_GetWindowSize (((__Window*) Window)->Window, &w, &h);
    if (xPos)
      *xPos = x;
    if (yPos)
      *yPos = y;
    if (Width)
      *Width = w;
    if (Height)
      *Height = h;
  }

void SetAttribute (SDL_GLattr Attr, int Value)
  {
    int x;
    char s [80], *sp;
    //
    x = SDL_GL_SetAttribute (Attr, Value);
    if (x)
      {
        sp = s;
        StrCat (&sp, "SDL_GL_SetAttribute: [");
        NumToStr (&sp, Attr);
        StrCat (&sp, "]=");
        NumToStr (&sp, Value);
        StrCat (&sp, " Err=");
        NumToStr (&sp, x);
        *sp = 0;
        DebugAddSDLError (s);
      }
  }

bool WidgetsInit (void)
  {
    bool Res;
    //int f;
    char s [80], *sx;
    SDL_version Ver;
    //
    Res = false;
    if (SDL_Init (SDL_INIT_EVERYTHING) == 0)
      {
        //SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "1");?
        //if (TTF_Init ())
        //  DebugAddSDLError ("TTF_Init");
        /*f = IMG_Init (IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
        sx = s;
        StrCat (&sx, "IMG_Init: 0b");
        NumToStrBase (&sx, f, -8, 2);
        *sx = 0;
        DebugAdd (s);*/
        SDL_VERSION (&Ver); //SDL_GetVersion
        sx = s;
        StrCat (&sx, "SDL_VERSION: ");
        NumToStr(&sx, Ver.major);
        StrCat (&sx, '.');
        NumToStr(&sx, Ver.minor);
        StrCat (&sx, '.');
        NumToStr(&sx, Ver.patch);
        *sx = 0;
        DebugAdd (s);
        SDL_DisableScreenSaver ();
        SetAttribute (SDL_GL_RED_SIZE, 8);
        SetAttribute (SDL_GL_GREEN_SIZE, 8);
        SetAttribute (SDL_GL_BLUE_SIZE, 8);
        //SetAttribute (SDL_GL_ALPHA_SIZE, 8);
        //SetAttribute (SDL_GL_DEPTH_SIZE, 32);
        Res = true;
      }
    else
      DebugAddSDLError ("SDL_Init");
    return Res;
  }

bool WidgetsUninit (void)
  {
    //Quit SDL subsystems
    //TTF_Quit ();
    //IMG_Quit ();
    Mix_Quit ();
    SDL_Quit ();
    return true;
  }

_Window *WindowCreate (const char *Title, int x, int y, int SizeX, int SizeY, byte WindowAttribute)
  {
    __Window *Res;
    //int f;
    //int r, g, b, a, d;
    //char s [80], *sx;
    Uint32 flags;
    //
    Res = (__Window *) malloc (sizeof (__Window));
    Res->Window = NULL;
    Res->Renderer = NULL;
    if (x < 0)
      x = SDL_WINDOWPOS_UNDEFINED;
    if (y < 0)
      y = SDL_WINDOWPOS_UNDEFINED;
    flags = 0;
    if (SizeX * SizeY == 0 || (WindowAttribute & waFullScreen))   // full screen
      flags = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_FOCUS;
    else
      {
        if (WindowAttribute & waResizable)
          flags |= SDL_WINDOW_RESIZABLE;
        if (WindowAttribute & waBorderless)
          flags |= SDL_WINDOW_BORDERLESS;
      }
    if (WindowAttribute & waAlwaysOnTop)
      flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    Res->Window = SDL_CreateWindow (Title, x, y, SizeX, SizeY, flags);   //Create window
    //SDL_SetWindowPosition (Res->Window, x, y);
    //SDL_GetWindowPosition (Res->Window, &x, &y);
    //Res->Window = SDL_CreateWindow (Title, x, y, 640, 480, flags);   //Create window
    if (!Res->Window)
      DebugAddSDLError ("SDL_CreateWindow");
    else
      {
        //Create renderer for window
        if (!ForceSoftwareRendering)
          Res->Renderer = SDL_CreateRenderer (Res->Window, -1, SDL_RENDERER_ACCELERATED /*| SDL_RENDERER_TARGETTEXTURE*/);
        if (!Res->Renderer)
          Res->Renderer = SDL_CreateRenderer (Res->Window, -1, SDL_RENDERER_SOFTWARE /*| SDL_RENDERER_TARGETTEXTURE*/);
        if (!Res->Renderer)
          DebugAddSDLError ("SDL_CreateRenderer");
        //else
        //  SDL_SetRenderDrawBlendMode (Res->Renderer, SDL_BLENDMODE_BLEND);
        //SDL_GLContext context = SDL_GL_CreateContext (Res->Window);
      }
    //printf ("Red size: %d, Green size: %d, Blue size: %d\n", r, g, b);
    /*f = 0;
    f |= SDL_GL_GetAttribute (SDL_GL_RED_SIZE, &r);
    f |= SDL_GL_GetAttribute (SDL_GL_GREEN_SIZE, &g);
    f |= SDL_GL_GetAttribute (SDL_GL_BLUE_SIZE, &b);
    f |= SDL_GL_GetAttribute (SDL_GL_ALPHA_SIZE, &a);
    f |= SDL_GL_GetAttribute (SDL_GL_DEPTH_SIZE, &d);
    if (f)
      DebugAddSDLError ("SDL_GL_GetAttribute");
    else
      {
        sx = s;
        StrCat (&sx, "Colour Depth R G B A D: ");
        NumToStr (&sx, r, 3);
        NumToStr (&sx, g, 3);
        NumToStr (&sx, b, 3);
        NumToStr (&sx, a, 3);
        NumToStr (&sx, d, 3);
        *sx = 0;
        DebugAdd (s);
      }*/
    return Res;
  }

void WindowDestroy (_Window *Window)
  {
    SDL_DestroyRenderer (((__Window *) Window)->Renderer);
    SDL_DestroyWindow (((__Window *) Window)->Window);
    free (Window);
  }

bool WindowSetPosSize (_Window *Window, int x, int y, int Width, int Height)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    if (x >= 0 && y >= 0)
      SDL_SetWindowSize (Window_->Window, Width, Height);
    if (Width >= 0 && Height >= 0)
      {
        //SDL_SetWindowResizable (Window_->Window, SDL_TRUE);
        SDL_SetWindowPosition (Window_->Window, x, y);
      }
    return true;
  }

bool WindowSetTitle (_Window * Window, const char *Title)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    SDL_SetWindowTitle (Window_->Window, Title);
    return true;
  }

bool WindowSetIcon (_Window *Window, _Bitmap *Bitmap)
  {
    __Window *Window_;
    SDL_Surface *Surface;
    //
    Window_ = (__Window *) Window;
    if (Bitmap)
      {
        Surface = (SDL_Surface *) Bitmap;
        SDL_SetColorKey (Surface, SDL_TRUE, SurfaceGetPixel (Surface, 0, 0));
        SDL_SetWindowIcon (Window_->Window, Surface);
      }
    /*
    if (Window_->Icon)
      SDL_FreeSurface (Window_->Icon)
    Window_->Icon = NULL;
    */
    return true;
  }


////////////////////////////////////////////////////////////////////////////
//
// Render Functions

/*
bool RenderSetTarget (_Window *Window, _Texture *Texture, int Colour, _Rect Clip)
  {
    __Window *Window_;
    int Res;
    SDL_Rect SDLRect;
    //
    Window_ = (__Window *) Window;
    Res = SDL_SetRenderTarget (Window_->Renderer, (SDL_Texture *) Texture);
    if (Res)
      DebugAddSDLError ("SDL_SetRenderTarget");
    TargetColour = -1;
    if (Texture == NULL && Res == 0)
      {
        RectToSDL_Rect (&Clip, &SDLRect);
        Res = SDL_RenderSetClipRect (Window_->Renderer, &SDLRect);
      }
    return Res == 0;
  }
*/

void RenderPresent (_Window *Window, _Bitmap *Bitmap_, _Rect Source, _Point Dest, byte Alpha)
  {
    SDL_Surface *Bitmap;
    SDL_Surface *screenSurface;
    int x, y, IndexIn, IndexOut;
    byte *PixelsIn;
    _Colour Pixel;
    //
    SDL_SetWindowOpacity (((__Window *) Window)->Window, (float) Alpha / 255.0);
    //
    Bitmap = (SDL_Surface*) Bitmap_;
    screenSurface = SDL_GetWindowSurface (((__Window *) Window)->Window);
    if ( (Dest.x + Source.Width > screenSurface->w) ||
         (Dest.y + Source.Height> screenSurface->h) ||
         (Dest.x < 0 || Dest.y < 0) )
      DebugAdd ("SDL_SetWindowOpacity: range error");
    else {
    //memcpy (screenSurface->pixels, ((SDL_Surface *) Bitmap)->pixels, screenSurface->pitch * screenSurface->h);
    //SDL_Rect srcRect = {0, 0, ((SDL_Surface *) Bitmap)->w, ((SDL_Surface *) Bitmap)->h}; // Full source surface
    //SDL_Rect dstRect = {0, 0, screenSurface->w, screenSurface->h}; // Full destination surface
    //SDL_BlitSurface ((SDL_Surface *) Bitmap, &srcRect, screenSurface, &dstRect);
    //SDL_FillRect (screenSurface, NULL, SDL_MapRGB (screenSurface->format, 0x8F, 0xFF, 0xFF));
    PixelsIn = (byte *) (Bitmap->pixels);
    for (y = 0; y < Source.Height; y++)
      {
        IndexIn = (Source.y + y) * Bitmap->pitch + Source.x * Bitmap->format->BytesPerPixel;
        IndexOut = (Dest.y + y) * screenSurface->pitch + Dest.x * screenSurface->format->BytesPerPixel;
        for (x = 0; x < Source.Width; x++)
          {
            Pixel.RGBA = ((_Colour *) (&PixelsIn [IndexIn]))->RGBA;
            ((byte *) screenSurface->pixels) [IndexOut + 0] = Pixel.Prime [2];
            ((byte *) screenSurface->pixels) [IndexOut + 1] = Pixel.Prime [1];
            ((byte *) screenSurface->pixels) [IndexOut + 2] = Pixel.Prime [0];
            if (screenSurface->format->BytesPerPixel == 4)
              ((byte *) screenSurface->pixels) [IndexOut + 3] = Pixel.Prime [3];
            IndexIn += ((SDL_Surface *) Bitmap)->format->BytesPerPixel;
            IndexOut += screenSurface->format->BytesPerPixel;
          }
      }
    SDL_UpdateWindowSurface (((__Window *) Window)->Window); }
    //SDL_RenderPresent (((__Window *) Window)->Renderer);
  }

void SetRenderDrawColor (_Window *Window, int Colour, byte Alpha = 0xFF)
  {
    if (Colour != TargetColour)
      {
        SDL_SetRenderDrawColor (((__Window *) Window)->Renderer, ColourR (Colour), ColourG (Colour), ColourB (Colour), Alpha);
        TargetColour = Colour;
      }
  }

void RenderDrawPoint (_Window *Window, int Colour, int x, int y, byte Alpha)
  {
    SetRenderDrawColor (Window, Colour, Alpha);
    SDL_RenderDrawPoint (((__Window *) Window)->Renderer, x, y);
  }

void RenderDrawLine (_Window *Window, int Colour, int x1, int y1, int x2, int y2)
  {
    SetRenderDrawColor (Window, Colour);
    if (x1 == x2 && y1 == y2)
      SDL_RenderDrawPoint (((__Window *) Window)->Renderer, x1, y1);
    else
      SDL_RenderDrawLine (((__Window *) Window)->Renderer, x1, y1, x2, y2);
  }

void RenderFillRect (_Window *Window, _Rect Rect, int Colour)
  {
    SDL_Rect FillRect;
    //
    SetRenderDrawColor (Window, Colour);
    RectToSDL_Rect (&Rect, &FillRect);   // FillRect = {x1, y1, Width, Height};
    SDL_RenderFillRect (((__Window *) Window)->Renderer, &FillRect);
  }

/*
_Texture *TextureFromText (_Window *Window, char *Text, _Typeface *Font, int Colour);

void RenderText (_Window *Window, int Colour, char *Text, _Typeface *Font, int x, int y)
  {
    _Texture *Texture;
    int Width, Height;
    //
    if (TextSize (Font, Text, &Width, &Height))
      {
        Texture = TextureFromText (Window, Text, Font, Colour);
        if (Texture)
          RenderTexture (Window, Texture, {0, 0, Width, Height}, x, y, 0xFF);
      }
  }
*/

void RenderFillTransparent (_Window *Window)
  {
    SDL_SetRenderDrawColor (((__Window *) Window)->Renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear (((__Window *) Window)->Renderer);
  }

bool WindowIDMatch (_WindowID WindowID, _Window *Window)
  {
    return WindowID == (_WindowID) SDL_GetWindowID (((__Window *) Window)->Window);
  }

int MouseX = 0, MouseY = 0;   // Keep for Mouse Wheel events
int ShiftState = 0;

void EventPoll (_Event *Event)
  {
    SDL_Event SDLEvent;
    //
    Event->Type = etNone;
    /*if (ClipboardReady)
      {
        ClipboardReady = false;
       x Event->Type = etClipboard;
        Event->WindowID = ClipboardWinID;
        return;
      }*/
    if (SDL_PollEvent (&SDLEvent))
      {
        Event->WindowID = (_WindowID) SDLEvent.window.windowID;
        if (SDLEvent.type == SDL_KEYUP)
          {
            ProcessSDLEventKey (Event, &SDLEvent.key, &ShiftState);
            //Event->Key = SDLkeysymToChar (&SDLEvent.key.keysym, &ShiftState);
            if (Event->Key >= 0)
              Event->Type = etKeyUp;
            if ((SDLEvent.key.keysym.sym == SDLK_F4) || (SDLEvent.key.keysym.sym == SDLK_x))   // Quit application
              if (SDLEvent.key.keysym.mod & (KMOD_LALT | KMOD_RALT | KMOD_LCTRL | KMOD_RCTRL))
                Event->Type = etQuit;
          }
        else if (SDLEvent.type == SDL_KEYDOWN)
          {
            ProcessSDLEventKey (Event, &SDLEvent.key, &ShiftState);
            //Event->Key = SDLkeysymToChar (&SDLEvent.key.keysym, &ShiftState);
            if (Event->Key >= 0)
              Event->Type = etKeyDown;
            //DebugAddHex ("SDL keysym.sym", {SDLEvent.key.keysym.sym}); //####
          }
        else if (SDLEvent.type == SDL_MOUSEBUTTONDOWN || SDLEvent.type == SDL_MOUSEBUTTONUP)
          {
            if (SDLEvent.type == SDL_MOUSEBUTTONDOWN)
              Event->Type = etMouseDown;
            else
              Event->Type = etMouseUp;
            Event->Key = SDLEvent.button.button;
            Event->X = SDLEvent.button.x;
            Event->Y = SDLEvent.button.y;
          }
        else if (SDLEvent.type == SDL_MOUSEWHEEL)
          {
            Event->Type = etMouseDown;
            if (SDLEvent.wheel.y > 0)
              Event->Key = KeyMouseWheelUp;
            else if (SDLEvent.wheel.y < 0)
              Event->Key = KeyMouseWheelDown;
            else if (SDLEvent.wheel.x > 0)
              Event->Key = KeyMouseWheelRight;
            else if (SDLEvent.wheel.x < 0)
              Event->Key = KeyMouseWheelLeft;
            Event->X = SDLEvent.wheel.mouseX; // MouseX;
            Event->Y = SDLEvent.wheel.mouseY; // MouseY;
            DebugAddInt ("MouseWheel", SDLEvent.button.button); //####
          }
        else if (SDLEvent.type == SDL_MOUSEMOTION)
          {
            Event->Type = etMouseMove;
            Event->Key = 0;
            Event->X = SDLEvent.motion.x;
            Event->Y = SDLEvent.motion.y;
            MouseX = SDLEvent.motion.x;
            MouseY = SDLEvent.motion.y;
          }
        else if (SDLEvent.type == SDL_QUIT)
          Event->Type = etQuit;
        else if (SDLEvent.type == SDL_WINDOWEVENT)
          {
            //#define DebugShowSDLWindowEvents
            #ifdef DebugShowSDLWindowEvents
            char s [64], *sp;
            sp = s;
            StrCat (&sp, "SDL: WindowEvent ");
            NumToStr (&sp, SDLEvent.window.event);
            *sp = 0;
            DebugAdd (s);
            #endif // DebugShowSDLWindowEvents
            if (SDLEvent.window.event == SDL_WINDOWEVENT_EXPOSED)
              Event->Type = etWindowRedraw;
            else if (SDLEvent.window.event == SDL_WINDOWEVENT_RESIZED ||
                     SDLEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
              {
                Event->Type = etWindowResize;
                Event->X = SDLEvent.window.data1;
                Event->Y = SDLEvent.window.data2;
              }
            else if (SDLEvent.window.event == SDL_WINDOWEVENT_CLOSE)
              Event->Type = etWindowClose;
            #ifdef __MINGW32__
            else if (Event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
              if (WindowCount == 1)   // If multiple windows this would causes a runaway
                SDL_RaiseWindow (Window);
            #endif
          }
      }
    Event->ShiftState = ShiftState;
  }

/*
bool RenderTexture (_Window *Window, _Texture *Texture, _Rect RecSource, int DestX, int DestY, byte AlphaOffset)
  {
    bool Res;
    SDL_Rect s, d;
    //
    Res = false;
    if (Texture)
      {
        if ((RecSource.Width > 0) && (RecSource.Height > 0))
          {
            RectToSDL_Rect (&RecSource, &s);
            d.x = DestX;
            d.y = DestY;
            d.w = RecSource.Width;
            d.h = RecSource.Height;
            if (SDL_SetTextureAlphaMod ((SDL_Texture *) Texture, AlphaOffset))
              DebugAddSDLError ("SDL_SetTextureAlphaMod");
            if (SDL_RenderCopy (((__Window *) Window)->Renderer, (SDL_Texture *) Texture, &s, &d))  //NULL, &d))
              DebugAddSDLError ("SDL_RenderCopy");
            Res = true;
          }
      }
    return Res;
  }


////////////////////////////////////////////////////////////////////////////
//
// Texture Functions

_Texture *TextureCreate (_Window *Window, int Width, int Height)
  {
    SDL_Texture *Res;
    //
    //return NULL;
    Res = SDL_CreateTexture (((__Window *) Window)->Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, Width, Height);
    if (!Res)
      DebugAddSDLError ("SDL_CreateTexture");
    else
      if (SDL_SetTextureBlendMode (Res, SDL_BLENDMODE_BLEND))
        DebugAddSDLError ("SDL_SetTextureBlendMode");
    return Res;
  }

void TextureDestroy (_Window *Window, _Texture *Texture)
  {
    if (Texture)
      SDL_DestroyTexture ((SDL_Texture *) Texture);
  }

void TextureGetSize (_Texture *Texture, int *Width, int *Height)
  {
    Uint32 Format;
    int Access;
    //
    *Width = 0;
    *Height = 0;
    if (Texture)
      SDL_QueryTexture ((SDL_Texture *) Texture, &Format, &Access, Width, Height);
  }

extern _Texture *TextureFromText (_Window *Window, char *Text, _Typeface *Font, int Colour)
  {
    SDL_Surface *Surface;
    SDL_Texture *Texture;
    //
    Texture = NULL;
    if (Text && *Text && Font)
      {
        #ifdef TextSmooth
          Surface = TTF_RenderUTF8_Blended ((TTF_Font *) Font, Text, {ColourR (Colour), ColourG (Colour), ColourB (Colour), 0xFF});
        #else
          Surface = TTF_RenderUTF8_Solid ((TTF_Font *) Font, Text, {ColourR (Colour), ColourG (Colour), ColourB (Colour), 0xFF});
        #endif // TextSmooth
        if (Surface)
          {
            //char s [80], *sp;
            //sp = s; StrCat (&sp, "Surface: "); NumToStr (&sp, Surface->w, 0); StrCat (&sp, 'x'); NumToStr (&sp, Surface->h, 0); *sp = 0;
            //DebugAdd (s);
            Texture = SDL_CreateTextureFromSurface (((__Window *) Window)->Renderer, Surface);
            if (!Texture)
              DebugAddSDLError ("SDL_CreateTextureFromSurface");
            SDL_FreeSurface (Surface);
          }
        else
          DebugAddSDLError ("TTF_RenderText_*");
      }
    return Texture;
  }
*/

_Bitmap *BitmapCreate (_Window *Window, int Width, int Height)
  {
    return SDL_CreateRGBSurface (0, Width, Height, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
  }

void BitmapDestroy (_Bitmap *Bitmap)
  {
    SDL_FreeSurface ((SDL_Surface *) Bitmap);
  }

int BitmapGetPixel (_Bitmap *Bitmap, int x, int y)
  {
    return SurfaceGetPixel (((SDL_Surface *) Bitmap), x, y);
  }

bool BitmapSetPixel (_Bitmap *Bitmap, int x, int y, int Colour)
  {
    if (Bitmap)
      {
        SurfaceSetPixel (((SDL_Surface *) Bitmap), x, y, Colour);
        return true;
      }
    return false;
  }

bool BitmapGetSize (_Bitmap *Bitmap, int *Width, int *Height)
  {
    if (Bitmap)
      {
        *Width = ((SDL_Surface *) Bitmap)->w;
        *Height = ((SDL_Surface *) Bitmap)->h;
        return true;
      }
    return false;
  }

void BitmapClear (_Bitmap *Bitmap)
  {
    int w, h;
    int x, y;
    //
    BitmapGetSize (Bitmap, &w, &h);
    for (x = 0; x < w; x++)
      for (y = 0; y < h; y++)
        BitmapSetPixel (Bitmap, x, y, 0);
  }


//////////////////////////////////////////////////////////////////////////////////
/*

bool RenderBitmap (_Window *Window, _Bitmap *Bitmap, _Rect RecSource, _Rect RecDest, int ColTransparent)   // Allow resizing
  {
    __Window *Window_;
    SDL_Surface *Surface;
    SDL_Texture *Texture;
    SDL_Rect s, d;
    //
    Texture = NULL;
    Window_ = (__Window *) Window;
    Surface = (SDL_Surface *) Bitmap;
    if (ColTransparent >= 0)
      SDL_SetColorKey (Surface, SDL_TRUE, ColTransparent);
    Texture = SDL_CreateTextureFromSurface (Window_->Renderer, Surface);
    //
    RectToSDL_Rect(&RecSource, &s);
    RectToSDL_Rect (&RecDest, &d);
    //if (SDL_SetTextureAlphaMod ((SDL_Texture *) Texture, 0xFF);//AlphaOffset))
    //  DebugAddSDLError ("SDL_SetTextureAlphaMod");
    if (SDL_RenderCopy (((__Window *) Window)->Renderer, (SDL_Texture *) Texture, &s, &d))  //NULL, &d))
      DebugAddSDLError ("SDL_RenderCopy");
    SDL_DestroyTexture (Texture);
    return true;
  }

_Bitmap* BitmapFromTexture (_Window *Window, _Texture *Texture, int Width, int Height)
  {
    __Window *Window_;
    SDL_Surface *Surface;
    //
    Surface = NULL;
    Window_ = (__Window *) Window;
    Surface = (SDL_Surface *) BitmapCreate (Window, Width, Height);
    SDL_RenderReadPixels (Window_->Renderer, &Surface->clip_rect, Surface->format->format, Surface->pixels, Surface->pitch);
    return Surface;
  }
*/

////////////////////////////////////////////////////////////////////////////
//
// Clipboard

bool ClipboardGet (_Window *Window_)
  {
    StrAssign (&ClipboardData, SDL_GetClipboardText ());
    ClipboardWinID = (void *) SDL_GetWindowID (((__Window *) Window_)->Window);
    return true;
  }

bool ClipboardSet (_Window *Window_, char *Text)
  {
    return SDL_SetClipboardText (Text) == 0;
  }


////////////////////////////////////////////////////////////////////////////
//
// Start Thread

//#include <pthread.h>

/*int SDLThreadFunction (void *data)
  {
    int i, j;
    //
    j = 0;
    for (i = 0; i < 1000; i++)
      j += i;
    return j;
  }*/

bool StartThread (_ThreadFunction ThreadFunction, void *Param)
  {
    SDL_Thread* threadID;
    //
    //threadID = SDL_CreateThread ((ThreadFunction) ThreadFunction, NULL, Param);
    threadID = SDL_CreateThread (ThreadFunction, "Thread", Param);
    if (threadID == 0)
      DebugAdd ("** StartThread ERROR ");
    else
      DebugAdd ("New Thread ");//, (int) threadID);
    return threadID != 0;
    /*
    pthread_t Thread;
    //pthread_attr_t ThreadAttr;
    int err;
    char St [64], *s;
    //
    Thread = 0;
    err = pthread_create (&Thread, NULL, ThreadFunction, Param);
    s = St;
    if (err)
      {
        StrCat (&s, "** StartThread ERROR ");
        NumToStr (&s, err);
      }
    else
      {
        StrCat (&s, "New Thread 0x");
        NumToHex (&s, Thread);
      }
    *s = 0;
    DebugAdd (St);
    return err == 0;
    */
  }

void Beep (void)   // Make a beep, why not?
  {
    puts ("\a");
  }


////////////////////////////////////////////////////////////////////////////
//
// Sound Support

int SoundDevice = -1;
int SoundsPlaying = 0;

void SoundInit (void)
  {
    SoundDevice = Mix_OpenAudio (44100, MIX_DEFAULT_FORMAT, 2, 2048);
  }

int SoundPlayThread (void *Param)
  {
    Mix_Chunk *SoundChunk;
    //
    SoundChunk = (Mix_Chunk *) Param;
    while (Mix_Playing (-1))
      SDL_Delay (100);
    DebugAdd ("SDL Sound Finished");
    SoundsPlaying--;
    free (SoundChunk->abuf);
    if (SoundChunk)
      Mix_FreeChunk (SoundChunk);
    return 0;
  }

bool SoundPlay (short *Buffer, int BufferSize, bool Free)
  {
    Mix_Chunk *SoundChunk;
    int i;
    //
    if (SoundDevice  < 0)
      SoundInit ();
    //
    SoundChunk = new Mix_Chunk;
    SoundChunk->abuf = (Uint8 *) Buffer;
    SoundChunk->alen = 2 * BufferSize;
    SoundChunk->allocated = 0;
    SoundChunk->volume = 128;
    //
    SoundsPlaying++;
    i = Mix_PlayChannel (-1, SoundChunk, 0);
    if (i >= 0)
      {
        StartThread (SoundPlayThread, Free ? SoundChunk : NULL);
        return true;
      }
    DebugAdd ("*** Play Sound Fail");
    if (Free)
      free (Buffer);
    return false;
  }

void SoundStop (void)
  {
    Mix_HaltChannel (-1);
  }


////////////////////////////////////////////////////////////////////////////
//
// Main Programme

int main (int argc, char* argv [])
  {
    PathStart = GetCurrentPath ();
    char *p = StrFindFilename (argv [0]);
    PathExe = StrMake (argv [0], p - argv [0]);
    int Res = main_ (argc, argv);
    free (PathStart);
    free (PathExe);
    return Res;
  }
