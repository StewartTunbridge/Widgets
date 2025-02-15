////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SELECT COLOUR
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <unistd.h>

#include "Widgets.hpp"
#include "ColourSelect.hpp"


class _FormColourSelect: public _Form
  {
    public:
      _FormColourSelect (char *Title, _Point Position, int InitialValue, _ActionResultInt ActionColour);
      ~_FormColourSelect (void);
      _Slider *sRed, *sGreen, *sBlue;
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

void ActionColourOK (_Container *Container)
  {
    if (fColourSelect->ActionColour)
      fColourSelect->ActionColour (fColourSelect->lSample->Colour);
    fColourSelect->Die ();
  }

void ActionColourCancel (_Container *Container)
  {
    fColourSelect->Die ();
  }
void ActionColourMenu (_MenuPopup *mp)
  {
    char Val [10], *v;
    char *cb;
    int Col;
    byte R, G, B;
    //
    if (mp->Selected == 0)   // Copy
      {
        v = Val;
        StrCat (&v, "0x");
        NumToHex (&v, fColourSelect->ColourValue ());
        *v = 0;
        ClipboardSet (Val);
      }
    else if (mp->Selected == 1)   // Paste
      {
        cb = ClipboardGet ();
        if (cb)
          {
            v = cb;
            if (StrMatch (&v, "0x"))
              {
                Col = StrGetHex (&v);
                ColourToRGB (Col, &R, &G, &B);
                fColourSelect->sRed->ValueSet (R);
                fColourSelect->sGreen->ValueSet (G);
                fColourSelect->sBlue->ValueSet (B);
                fColourSelect->SampleFill ();
              }
            free (cb);
          }
      }
  }

bool FormColourPreview (_Form *Form, _Event *Event)
  {
    if (Event->Type == etKeyDown)
      if (Event->Key == KeyEnter)
        ActionColourOK (NULL);
      else if (Event->Key == esc)
        Form->Die ();
    return false;
  }

_FormColourSelect::_FormColourSelect (char *Title, _Point Position, int InitialValue, _ActionResultInt ActionColour_)
                 : _Form (Title, {Position.x, Position.y, 150, 110}, waAlwaysOnTop)// | waBorderless)
  {
    const int sX = 16;
    const int sY = 64;
    const int bW = 64;
    const int bH = 32;
    //
    int x, y;
    byte r, g, b;
    //
    ColourToRGB (InitialValue, &r, &g, &b);
    ActionColour = ActionColour_;
    Container->FontSet (NULL, 14); //NULL
    x = 4;
    y = 4;
    sRed = new _Slider (Container, {x, y, sX, sY}, 0, 255, ActionColourUpdate); x += sX + 4;
    sRed->ColourKnob = cRed;
    sRed->ValueSet (r);
    sGreen = new _Slider (Container, {x, y, sX, sY}, 0, 255, ActionColourUpdate); x += sX + 4;
    sGreen->ColourKnob = cGreen;
    sGreen->ValueSet (g);
    sBlue = new _Slider (Container, {x, y, sX, sY}, 0, 255, ActionColourUpdate); x += sX + 4;
    sBlue->ColourKnob = cBlue;
    sBlue->ValueSet (b);
    lSample = new _Label (Container, {x, y, Container->Rect.Width - x - 4, sY}, NULL, aCenter, bMote);
    SampleFill ();
    Menu = new _MenuPopup (lSample, {0, 0, 0, 0}, "Copy\tPaste", (_Action) ActionColourMenu);
    x = 4;
    y += sY + 4;
    bOK = new _Button (Container, {x, y, bW, bH}, "OK", ActionColourOK);
    bCancel = new _Button (Container, {Container->Rect.Width - bW - 4, y, bW, bH}, "Cancel", ActionColourCancel);
    EventPreview = FormColourPreview;
  }

int _FormColourSelect::ColourValue (void)
  {
    return RGBToColour (sRed->ValueGet (), sGreen->ValueGet (), sBlue->ValueGet ());
  }

void _FormColourSelect::SampleFill (void)
  {
    if (lSample)
      {
        lSample->Colour = ColourValue ();
        lSample->Invalidate (true);
      }
  }

_FormColourSelect::~_FormColourSelect (void)
  {
    fColourSelect = NULL;
  }

void ColourSelect (char *Title, int InitialValue, _ActionResultInt ActionColour_)
  {
    if (!fColourSelect)
      fColourSelect = new _FormColourSelect (Title, MousePos (), InitialValue, ActionColour_);
  }
