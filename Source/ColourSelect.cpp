////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SELECT COLOUR
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <unistd.h>

#include "Widgets.hpp"
#include "ColourSelect.hpp"


///////////////////////////////////////////////////////////////////////////////////////////////////
//
// From https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion

float HueToRGB (float p, float q, float t)
  {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0/6.0)
      return p + (q - p) * 6.0 * t;
    if (t < 1.0/2.0)
      return q;
    if (t < 2.0/3.0)
      return p + (q - p) * (2.0/3.0 - t) * 6.0;
    return p;
  }

int HSLToRGB (float h, float s, float l)
  {
    //float h, s, l;
    float p, q, r, g, b;
    //
    if (s == 0.0)
      r = g = b = l; // achromatic
    else
      {
        q = (l < 0.5) ? l * (1 + s) : l + s - l * s;
        p = 2 * l - q;
        r = HueToRGB (p, q, h + 1.0/3.0);
        g = HueToRGB (p, q, h);
        b = HueToRGB (p, q, h - 1.0/3.0);
      }
    return Round (r * 255) | (Round (g * 255) << 8) | (Round (b * 255) << 16);
  }

void RGBToHSL (int RGB, float *Hue, float *Sat, float *Lum)
  {
    _Colour RGB_;
    float r, g, b;
    float vmax, vmin;
    float d;
    //
    RGB_.RGBA = RGB;
    r = RGB_.Prime [0] / 255.0;
    g = RGB_.Prime [1] / 255.0;
    b = RGB_.Prime [2] / 255.0;
    vmax = Max (r, g, b);
    vmin = Min (r, g, b);
    *Hue = *Sat = 0.0;
    *Lum = (vmax + vmin) / 2;
    if (vmax != vmin)   // not achromatic
      {
        d = vmax - vmin;
        *Sat = (*Lum > 0.5) ? d / (2 - vmax - vmin) : d / (vmax + vmin);
        if (vmax == r)
          *Hue = (g - b) / d + (g < b ? 6 : 0);
        if (vmax == g)
          *Hue = (b - r) / d + 2;
        if (vmax == b)
          *Hue = (r - g) / d + 4;
        *Hue /= 6;
      }
  }


//////////////////////////////////////////////////////////////////////////////////////////////////
//
// PALETTE WIDGET - Display Hue x Saturation

class _Palette: public _Container
  {
    public:
      _Palette (_Container *Parent_, _Rect Rect_, _Action Action_);
      virtual void DrawCustom (void);
      virtual bool ProcessEventCustom (_Event *Event, _Point Offset);
      //int mx, my;
      float Hue, Saturation;
      _Action Action;
  };

_Palette::_Palette (_Container *Parent_, _Rect Rect_, _Action Action_) : _Container (Parent_, Rect_)
  {
    Hue = 0;
    Saturation = 255;
    Action = Action_;
  }

void _Palette::DrawCustom (void)
  {
    int x, y;
    int Colour;
    //
    for (y = 0; y < Rect.Height; y++)
      for (x = 0; x < Rect.Width; x++)
        {
          Colour = HSLToRGB ((float) (Rect.Height - 1 - y) / Rect.Height, (float) (Rect.Width - x - 1) / Rect.Width, 0.5);
          //if (Colour < 0)
          //  DebugAddHex ("** ", Colour);//####
          DrawLine (x, y, x, y, Colour);
        }
    y = Rect.Height - 1 - Hue * Rect.Height;
    x = Rect.Width - 1 - Saturation * Rect.Width;
    //DrawRectangle ({x - 2, y - 2, 5, 5}, cBlack, cBlack, -1);
    DrawCircle (x, y, 3, cBlack, -1, 1);
    DrawCircle (x, y, 2, cWhite, -1, 1);
  };

bool _Palette::ProcessEventCustom (_Event *Event, _Point Offset)
  {
    int x, y;
    //
    //if (IsEventMine (Event, Offset))// && IsEventWithin (Event, Offset))
      if ((Event->Type == etMouseDown || Event->Type == etMouseMove) && Event->MouseKeys == Bit [KeyMouseLeft - 1])
        {
          x = Limit (Event->X - Offset.x, 0, Rect.Width - 1);
          y = Limit (Event->Y - Offset.y, 0, Rect.Height - 1);
          Hue = (float) (Rect.Height - y - 1) / Rect.Height;
          Saturation = (float) (Rect.Width - x - 1) / Rect.Width;
          Invalidate ();
          if (Action)
            Action (this);
          return true;
        }
    return false;
  }

class _FormColourSelect: public _Form
  {
    public:
      _FormColourSelect (char *Title, _Point Position, int InitialValue, _ActionResultInt ActionColour, void *Parameter);
      ~_FormColourSelect (void);
      _Slider *sRed, *sGreen, *sBlue;
      _Palette *Palette;
      _Slider *sLum;
      _Label *lSample;
      _MenuPopup *Menu;
      _Button *bOK, *bCancel;
      _ActionResultInt ActionColour;
      int ColourValue (void);
      void SampleFill (void);
  };

_FormColourSelect *fColourSelect = NULL;

void ActionColourUpdate (_Container *Container)
  {
    fColourSelect->SampleFill ();
  }

void ActionParamUpdate (_Container *Container)
  {
    float Lum;
    _Colour Colour;
    //
    Lum = (float) fColourSelect->sLum->ValueGet () / fColourSelect->sLum->ValueMax;
    //Lum = Max ((float) 0.1, Lum);   // Restrict Luminance to allow defined Hue and Saturat
    //Lum = Min ((float) 0.9, Lum);
    Colour.RGBA = HSLToRGB (fColourSelect->Palette->Hue, fColourSelect->Palette->Saturation, Lum);
    //
    fColourSelect->sRed->ValueSet (Colour.Prime [0]);
    fColourSelect->sGreen->ValueSet (Colour.Prime [1]);
    fColourSelect->sBlue->ValueSet (Colour.Prime [2]);
    fColourSelect->SampleFill ();
  }

void ActionColourOK (_Container *Container)
  {
    if (fColourSelect->ActionColour)
      fColourSelect->ActionColour (fColourSelect->lSample->Colour, fColourSelect->Container->DataContainer);
    fColourSelect->Die ();
  }

void ActionColourCancel (_Container *Container)
  {
    fColourSelect->Die ();
  }
void ActionColourMenu (_MenuPopup *mp)
  {
    char Val [10], *v;
    char *c;
    int Col;
    byte R, G, B;
    //
    if (mp->Selected == 0)   // Copy
      {
        v = Val;
        StrCat (&v, "0x");
        NumToHex (&v, fColourSelect->ColourValue ());
        *v = 0;
        ClipboardSet (mp->Form->Window, Val);
      }
    else if (mp->Selected == 1)   // Paste
      {
        if (ClipboardGet (mp->Form->Window))
          {
            c = ClipboardData;
            if (StrMatch (&c, "0x"))
              {
                Col = StrGetHex (&c);
                ColourToRGB (Col, &R, &G, &B);
                fColourSelect->sRed->ValueSet (R);
                fColourSelect->sGreen->ValueSet (G);
                fColourSelect->sBlue->ValueSet (B);
                fColourSelect->SampleFill ();
              }
          }
      }
  }

bool FormColourPreview (_Form *Form, _Event *Event)
  {
    bool Res;
    //
    Res = false;
    if (Event->Type == etKeyDown)
      {
        Res = true;
        if (Event->Key == KeyEnter)
          ActionColourOK (NULL);
        else if (Event->Key == esc)
          Form->Die ();
        else
          Res = false;
      }
    return Res;
  }

_FormColourSelect::_FormColourSelect (char *Title, _Point Position, int InitialValue, _ActionResultInt ActionColour_, void *Parameter)
                 : _Form (Title, {Position.x, Position.y, 300, 172}, waAlwaysOnTop)// | waBorderless)
  {
    const int sX = 20;
    const int sY = 128;
    const int bW = 64;
    const int bH = 32;
    //
    int x, y;
    byte r, g, b;
    float h, s, l;
    //
    ColourToRGB (InitialValue, &r, &g, &b);
    ActionColour = ActionColour_;
    Container->DataContainer = (_Container *) Parameter;
    Container->FontSet (NULL, 14); //NULL
    x = 4;
    y = 4;
    sRed = new _Slider (Container, {x, y, sX, sY}, NULL, ActionColourUpdate, 0, 255); x += sX + 4;
    sRed->ColourKnob = cRedLight;
    sRed->ValueSet (r);
    sGreen = new _Slider (Container, {x, y, sX, sY}, NULL, ActionColourUpdate, 0, 255); x += sX + 4;
    sGreen->ColourKnob = cGreenLight;
    sGreen->ValueSet (g);
    sBlue = new _Slider (Container, {x, y, sX, sY}, NULL, ActionColourUpdate, 0, 255); x += sX + 4;
    sBlue->ColourKnob = cBlueLight;
    sBlue->ValueSet (b);
    //
    RGBToHSL (InitialValue, &h, &s, &l);
    Palette = new _Palette (Container, {x, y, sX * 5, sY}, ActionParamUpdate); x += 5 * sX;
    sLum = new _Slider (Container, {x, y, sX, sY}, NULL, ActionParamUpdate, 0, 255); x += sX + 4;
    Palette->Hue = h;
    Palette->Saturation = s;
    sLum->ValueSet (l * sLum->ValueMax);
    //
    lSample = new _Label (Container, {x, y, Container->Rect.Width - x - 4, sY}, NULL, aCenter, bMote);
    SampleFill ();
    Menu = new _MenuPopup (lSample, {0, 0, 0, 0}, "Copy\tPaste", (_Action) ActionColourMenu);
    y += sY + 4;
    //
    bOK = new _Button (Container, {4, -bH - 4, bW, bH}, "OK", ActionColourOK);
    bCancel = new _Button (Container, {-bW - 4, -bH - 4, bW, bH}, "Cancel", ActionColourCancel);
    EventPreview = FormColourPreview;
  }

int _FormColourSelect::ColourValue (void)
  {
    return RGBToColour (sRed->ValueGet (), sGreen->ValueGet (), sBlue->ValueGet ());
  }

void _FormColourSelect::SampleFill (void)
  {
    float Lum;
    //
    lSample->Colour = ColourValue ();
    lSample->Invalidate ();
    RGBToHSL (lSample->Colour, &Palette->Hue, &Palette->Saturation, &Lum);
    Palette->Invalidate ();
    sLum->ValueSet (Lum * sLum->ValueMax);
    //DebugAddHex ("Colour", lSample->Colour); //####
  }

_FormColourSelect::~_FormColourSelect (void)
  {
    fColourSelect = NULL;
  }

void ColourSelect (char *Title, int InitialValue, _ActionResultInt ActionColour_, void *Parameter)
  {
    if (!fColourSelect)
      fColourSelect = new _FormColourSelect (Title, MousePos (), InitialValue, ActionColour_, Parameter);
  }
