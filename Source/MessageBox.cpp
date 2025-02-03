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

_FormMessageBox::_FormMessageBox (_Point Size, char *Message)
                 : _Form (NULL, {MousePos ().x, MousePos ().y, Size.x, Size.y}, waAlwaysOnTop) // | waBorderless)
  {
    Container->FontSet (NULL, 20);
    lMessage = new _Label (Container, {0, 0, 0, 0}, Message, aCenter, bNone);
    lMessage->AlignVert = aCenter;
  }

_FormMessageBox::~_FormMessageBox (void)
  {
    fMessageBox = NULL;
  }

void MessageBox (_Point Size, char *Message)
  {
    if (!fMessageBox)
      fMessageBox = new _FormMessageBox (Size, Message);
  }
