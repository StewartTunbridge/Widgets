////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MESSAGE BOX
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <unistd.h>

#include "Widgets.hpp"
#include "MessageBox.hpp"


/*class _FormMessageBox: public _Form
  {
    public:
      _FormMessageBox (_Point Size, char *Message);
      ~_FormMessageBox (void);
      _Label *lMessage;
  };*/

_FormMessageBox *fMessageBox = NULL;

_FormMessageBox::_FormMessageBox (_Rect Rect_, char *Message, int FontSize)
                :_Form (NULL, Rect_, waAlwaysOnTop) // | waBorderless)
  {
    Container->FontSet (NULL, FontSize);
    lMessage = new _Label (Container, {0, 0, 0, 0}, Message, aCenter, bNone);
    lMessage->AlignVert = aCenter;
  }

_FormMessageBox::~_FormMessageBox (void)
  {
    fMessageBox = NULL;
  }

void MessageBox (_Point Size, char *Message, int FontSize)
  {
    _Point Mouse;
    //
    if (!fMessageBox)
      {
        Mouse = MousePos ();
        fMessageBox = new _FormMessageBox ({Mouse.x, Mouse.y, Size.x, Size.y}, Message, FontSize);
      }
  }
