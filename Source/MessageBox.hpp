////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MESSAGE BOX
//
////////////////////////////////////////////////////////////////////////////////////////////////////


// Create a form displaying a message

extern void MessageBox (_Point Size, char *Message, int FontSize = 16);


// Optional Details

class _FormMessageBox: public _Form
  {
    public:
      _FormMessageBox (_Rect Rect_, char *Message, int FontSize);
      ~_FormMessageBox (void);
      _Label *lMessage;
  };

extern _FormMessageBox *fMessageBox;
