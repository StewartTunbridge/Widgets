////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MESSAGE BOX: Create a form displaying a message
//
////////////////////////////////////////////////////////////////////////////////////////////////////


// Show a message in a box

extern void MessageBox (_Point Size, char *Message, int FontSize = 16);


// For adjustments --

class _FormMessageBox: public _Form
  {
    public:
      _FormMessageBox (_Rect Rect_, char *Message, int FontSize);
      ~_FormMessageBox (void);
      _Label *lMessage;
      _MenuPopup *mLabel;
  };

extern _FormMessageBox *fMessageBox;

