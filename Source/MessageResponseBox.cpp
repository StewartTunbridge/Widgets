////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MESSAGE RESPONSE BOX
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <unistd.h>

#include "Widgets.hpp"
#include "MessageResponseBox.hpp"


_FormMessageResponseBox *fMessageResponseBox = NULL;

void ActionResponse (_Button *Button)
  {
    if (fMessageResponseBox->ActionResult)
      fMessageResponseBox->ActionResult (Button->DataInt, Button->Form->Container->DataContainer);
    fMessageResponseBox->Die ();
  }

bool FormMessageResponseBoxPreview (_Form *Form, _Event *Event)
  {
    _FormMessageResponseBox *fMessageResponseBox;
    int i;
    //
    fMessageResponseBox = (_FormMessageResponseBox *) Form;
    if ( (Event->Type == etKeyDown && Event->Key == esc) ||
         (Event->Type == etWindowClose) )
      {
        if (fMessageResponseBox->ActionResult)
          fMessageResponseBox->ActionResult (-1, Form->Container->DataContainer);
        delete fMessageResponseBox;
        fMessageResponseBox = NULL;
        return true;
      }
    if (Event->Type == etKeyDown)
      {
        i = 0;
        while (fMessageResponseBox->Shortcuts [i])
          {
            if (UpCase (fMessageResponseBox->Shortcuts [i]) == UpCase (Event->Key))
              if (fMessageResponseBox->ActionResult)
                {
                  fMessageResponseBox->ActionResult (i, fMessageResponseBox->Container->DataContainer);
                  fMessageResponseBox->Die ();
                  return true;
                }
            i++;
          }
        Beep ();
      }
    return false;
  }

_FormMessageResponseBox::_FormMessageResponseBox (_Rect Pos, char *Message, char *Responses, _ActionResultInt ActionRes, void *Parameter)
  : _Form (NULL, Pos, waAlwaysOnTop)// | waBorderless)
  {
    _Button *Button;
    const int bW = 64;
    const int bH = 32;
    int n, x, y;
    char BtnTxt [128], *bt;
    char *r;
    _Point Size;
    //
    //
    ActionResult = ActionRes;
    Container->FontSet (NULL, 16);
    lMessage = new _Label (Container, {0}, Message, aCenter, bRaised);
    lMessage->AlignVert = aTop;
    for (n = 0; n < SIZEARRAY (Shortcuts); n++)
      Shortcuts [n] = '\0';
    n = 0;
    x = 4;
    y = Pos.Height - bH - 4;
    r = Responses;
    while (true)
      {
        bt = BtnTxt;
        while (true)
          {
            if (*r == tab || *r == '\0')
              break;
            if (IsUpper (*r) && Shortcuts [n] == '\0')
              {
                Shortcuts [n] = *r;
                StrCat (&bt, "\aU");
                StrCat (&bt, *r++);
                StrCat (&bt, "\aU");
              }
            else if (*r == '\e' || *r == '\a')   // text modifier
              {
                StrCat (&bt, *r++);
                StrCat (&bt, *r++);
              }
            else
              StrCat (&bt, *r++);
          }
        *bt = 0;
        //DebugAddS ("BtnTxt", BtnTxt); //####
        Size = Container->TextMeasure (BtnTxt);
        Size.x = Max (Size.x + 16, bW);
        Button = new _Button (Container, {x, y, Size.x, bH}, BtnTxt, (_Action) ActionResponse);
        Button->DataInt = n++;
        x += Size.x;
        if (*r == '\0')   // end reached
          break;
        r++;   // step past tab
      }
    Container->DataContainer = (_Container *) Parameter;
    EventPreview = FormMessageResponseBoxPreview;
  }

_FormMessageResponseBox::~_FormMessageResponseBox (void)
  {
    fMessageResponseBox = NULL;
  }

bool MessageResponseBox (_Point Size, char *Message, char *Responces, _ActionResultInt ActionResponce, void *Parameter)
  {
    _Point Pos;
    //
    if (fMessageResponseBox)
      return false;
    Pos = MousePos ();
    fMessageResponseBox = new _FormMessageResponseBox ({Pos.x, Pos.y, Size.x, Size.y}, Message, Responces, ActionResponce, Parameter);
    return true;
  }
