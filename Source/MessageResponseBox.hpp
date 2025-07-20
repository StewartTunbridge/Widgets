////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MESSAGE RESPONSE BOX
//
////////////////////////////////////////////////////////////////////////////////////////////////////


// Create a form with a massage and a set of response buttons
// Any response will call the ActionResultInt and delete the form

typedef void (*_ActionResultInt) (int Result, void *Parameter);

extern bool MessageResponseBox (_Point Size, char *Message, char *Responses, _ActionResultInt ActionResponse, void *Parameter = NULL);


// For adjustments

class _FormMessageResponseBox: public _Form
  {
    public:
      char Shortcuts [64];
      _FormMessageResponseBox (_Rect Pos, char *Message, char *Responses, _ActionResultInt ActionRes, void *Parameter);
     ~_FormMessageResponseBox (void);
      _Label *lMessage;
      _ActionResultInt ActionResult;
  };

extern _FormMessageResponseBox *fMessageResponseBox;

