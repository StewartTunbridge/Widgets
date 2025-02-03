////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SELECT COLOUR
//
////////////////////////////////////////////////////////////////////////////////////////////////////


// Create a form for user to select a colour
// If one is selected with OK, ActionColour will be called with the result
// On either OK or Cancel, form is deleted

typedef void (*_ActionResultInt) (int Result);

extern void ColourSelect (char *Title, int InitialValue, _ActionResultInt ActionColourSelected);
