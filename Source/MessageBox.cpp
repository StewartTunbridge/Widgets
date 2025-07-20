////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MESSAGE BOX
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <unistd.h>

#include "Widgets.hpp"
#include "MessageBox.hpp"


_FormMessageBox *fMessageBox = NULL;

void ActionMessageBoxPopup (_MenuPopup *Popup)
  {
    char *Text_, *t;
    char *a;
    int Size;
    char c;
    //
    Text_ = NULL;
    t = Text_;
    Size = 0;
    a = fMessageBox->lMessage->TextGet ();
    if (Popup->Selected == 0)   // Copy
      while (true)
        {
          if (*a == '\0')
            {
              if (Text_)
                {
                  *t = 0;
                  ClipboardSet (Popup->Form->Window, Text_);
                  free (Text_);
                  break;
                }
              else
                {
                  Text_ = (char *) malloc (Size);
                  t = Text_;
                }
              a = fMessageBox->lMessage->TextGet ();
            }
          c = *a;
          if (c == '\e')
            {
              Size += StrCat (&t, "<Img=");
              a++;
            }
          else if (c == '\a')
            {
              Size += StrCat (&t, "<Font=");
              a++;
            }
          Size += StrCat (&t, *a++);
          if (c == '\e' || c == '\a')
            Size += StrCat (&t, '>');
        }
  }

bool FormMessageBoxEventPreview (_Form *Form, _Event *Event)
  {
    if (Event->Type == etKeyDown)
      if (Event->Key == esc)
        {
          delete fMessageBox;
          fMessageBox = NULL;
          return true;
        }
    return false;
  }

_FormMessageBox::_FormMessageBox (_Rect Rect_, char *Message, int FontSize)
                :_Form (NULL, Rect_, waAlwaysOnTop | waResizable) // | waBorderless)
  {
    Container->FontSet (NULL, FontSize);
    lMessage = new _Label (Container, {0, 0, 0, 0}, Message, aCenter, bNone);
    lMessage->AlignVert = aCenter;
    mLabel = new _MenuPopup (lMessage, {0, 0, 0, 0}, "Copy", (_Action) ActionMessageBoxPopup);
    EventPreview = FormMessageBoxEventPreview;
  }

_FormMessageBox::~_FormMessageBox (void)
  {
    fMessageBox = NULL;
  }

void MessageBox (_Point Size, char *Message, int FontSize)
  {
    _Point Mouse;
    //
    if (fMessageBox)
      delete fMessageBox;
    Mouse = MousePos ();
    fMessageBox = new _FormMessageBox ({Mouse.x, Mouse.y, Size.x, Size.y}, Message, FontSize);
  }
