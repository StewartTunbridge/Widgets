////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MESSAGE BOX
//
////////////////////////////////////////////////////////////////////////////////////////////////////


// Create a form displaying a message

extern void MessageBox (_Point Size, char *Message);


// Optional Details

class _FormMessageBox: public _Form
  {
    public:
      _FormMessageBox (_Point Size, char *Message);
      ~_FormMessageBox (void);
      _Label *lMessage;
  };

extern _FormMessageBox *fMessageBox;
