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
      _Button *bOK, *bCancel;
      _ActionResultInt ActionColour;
      void SampleFill (void);
  };

_FormColourSelect *fColourSelect = NULL;

void ActionColourUpdate (_Container *Container)
  {
    fColourSelect->SampleFill ();
  }

void ActionColourOK (_Container *Container)
  {
    _FormColourSelect *Form;
    _Button *b;
    //
    Form = (_FormColourSelect *) Container->Form;
    b = (_Button *) Container;
    if (!b->Down)
      {
        if (Form->ActionColour)
          Form->ActionColour (Form->lSample->Colour);
        Form->Die ();
      }
  }

void ActionColourCancel (_Container *Container)
  {
    _FormColourSelect *Form;
    _Button *b;
    //
    Form = (_FormColourSelect *) Container->Form;
    b = (_Button *) Container;
    if (!b->Down)
      Form->Die ();
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
    sRed->Value = r;
    sGreen = new _Slider (Container, {x, y, sX, sY}, 0, 255, ActionColourUpdate); x += sX + 4;
    sGreen->ColourKnob = cGreen;
    sGreen->Value = g;
    sBlue = new _Slider (Container, {x, y, sX, sY}, 0, 255, ActionColourUpdate); x += sX + 4;
    sBlue->ColourKnob = cBlue;
    sBlue->Value = b;
    lSample = new _Label (Container, {x, y, Container->Rect.Width - x - 4, sY}, NULL, aCenter, bMote);
    SampleFill ();
    x = 4;
    y += sY + 4;
    bOK = new _Button (Container, {x, y, bW, bH}, "OK", ActionColourOK);
    bCancel = new _Button (Container, {Container->Rect.Width - bW - 4, y, bW, bH}, "Cancel", ActionColourCancel);
  }

void _FormColourSelect::SampleFill (void)
  {
    int Res;
    //
    Res = RGBToColour (sRed->Value, sGreen->Value, sBlue->Value);
    lSample->Colour = Res;
    lSample->Invalidate (true);
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
