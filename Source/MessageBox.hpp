////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MESSAGE BOX: Create a form displaying a message
//
////////////////////////////////////////////////////////////////////////////////////////////////////


// Show a message in a box:
//   Message: [text | heading \v text]

extern void MessageBox (_Point Size, const char *Message, int FontSize = 0);


// For adjustments --

class _FormMessageBox: public _Form
  {
    public:
      _FormMessageBox (_Rect Rect_, const char *Title, const char *Message, int FontSize);
      ~_FormMessageBox (void);
      _Label *lMessage;
      _MenuPopup *mLabel;
  };

extern _FormMessageBox *fMessageBox;

