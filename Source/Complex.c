////////////////////////////////////////////////////////////////////////////
//
// COMPLEX
// =======
//
// Define Complex structure and related maths function
//
// 13 Feb 2020 Genesis
//
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
//
// COMPLEX STRUCTURE & CONSTANTS

typedef struct
  {
    double Real;
    double Imaginary;
    bool ModArgValid;
    double Mod;
    double Arg;
  } Complex;


////////////////////////////////////////////////////////////////////////////
//
// CONSTANTS

const Complex C_1 = {1, 0, true, 1, 0};
const Complex C_2 = {2, 0, true, 2, 0};
const Complex C_3 = {3, 0, true, 3, 0};
const Complex C_4 = {4, 0, true, 4, 0};
const Complex C_8 = {8, 0, true, 8, 0};
const Complex C_12 = {12, 0, true, 12, 0};
const Complex C_Pi = {M_PI, 0.0, true, M_PI, 0.0};
const Complex C_i = {0, 1, true, 1, M_PI / 2.0};
const Complex C_2i = {0, 2, true, 2, M_PI / 2.0};
const Complex C_N1 = {-1, 0, true, 1, M_PI};


////////////////////////////////////////////////////////////////////////////
//
// BASIC FUNCTIONS

double frac (double Val)
  {
    return Val - floor (Val);
  }

void ComplexAdd (const Complex *a, const Complex *b, Complex *Result)
  {
    double r, i;
    //
    r = a->Real + b->Real;
    i = a->Imaginary + b->Imaginary;
    Result->Real = r;
    Result->Imaginary = i;
    Result->ModArgValid = false;
  }

void ComplexSub (const Complex *a, const Complex *b, Complex *Result)
  {
    double r, i;
    //
    r = a->Real - b->Real;
    i = a->Imaginary - b->Imaginary;
    Result->Real = r;
    Result->Imaginary = i;
    Result->ModArgValid = false;
  }

void ComplexMul (const Complex *a, const Complex *b, Complex *Result)
  {
    double r, i;
    //
    r = (a->Real * b->Real) - (a->Imaginary * b->Imaginary);
    i = (a->Real * b->Imaginary) + (a->Imaginary * b->Real);
    Result->Real = r;
    Result->Imaginary = i;
    Result->ModArgValid = false;
  }

bool ComplexInvert (const Complex *Val, Complex *Result)
  {
    double Divisor;
    double r, i;
    //
    Divisor = (Val->Real * Val->Real) + (Val->Imaginary * Val->Imaginary);
    if (Divisor == 0.0)
      return false;
    r = Val->Real / Divisor;
    i = -Val->Imaginary / Divisor;
    Result->Real = r;
    Result->Imaginary = i;
    Result->ModArgValid = false;
    return true;
  }

bool ComplexDiv (const Complex *a, const Complex *b, Complex *Result)
  {
    Complex InvB;
    //
    if (a->Imaginary == 0.0 && b->Imaginary == 0.0)
      {
        Result->Real = a->Real / b->Real;
        Result->Imaginary = 0.0;
      }
    else
      {
        if (!ComplexInvert (b, &InvB))
          return false;
        ComplexMul (a, &InvB, Result);
      }
    return true;
  }

bool ComplexMod (const Complex *a, const Complex *b, Complex *Result)
  {
    Complex InvB;
    //
    if (!ComplexInvert (b, &InvB))   // InvB = 1 / b
      return false;
    ComplexMul (a, &InvB, Result);   // Result = frac (a * InvB)
    Result->Real = frac (Result->Real);
    Result->Imaginary = frac (Result->Imaginary);
    ComplexMul (b, Result, Result);   // Result = b * Result
    return true;
  }

double Sqr (double x)
  {
    return x * x;
  }

double Mod2 (Complex *a)
  {
    return Sqr (a->Real) + Sqr (a->Imaginary);
  }

void ComplexSqrt (const Complex *a, Complex *Result)
  {
    double m;
    double r, i;
    //
    m = sqrt (Sqr (a->Real) + Sqr (a->Imaginary));
    r = sqrt ((m + a->Real) / 2.0);
    i = sqrt ((m - a->Real) / 2.0);
    if (a->Imaginary < 0.0)   // copy the original imaginary sign
      i = -i;
    Result->Real = r;
    Result->Imaginary = i;
    Result->ModArgValid = false;
  }

int ComplexCompare (const Complex *a, const Complex *b)
  {
    if (a->Real > b->Real)
      return +1;
    if (a->Real < b->Real)
      return -1;
    return 0;
  }


////////////////////////////////////////////////////////////////////////////
//
// ADVANCED FUNCTIONS - Convert between polar and rectangular

double Atan2 (double dy, double dx)
  {
    double Res;
    //
    if (dx == 0.0)
      if (dy >= 0.0)
        Res = M_PI / 2.0;
      else
        Res = -M_PI / 2.0;
    else
      {
        Res = atan (Abs (dy / dx));
        if (dx < 0.0)   // left quadrants
          if (dy >= 0.0)   // top left quadrant
            Res = M_PI - Res;
          else   // lower left quadrant
            Res = -M_PI + Res;
        else   // right quadrants
          if (dy < 0.0)   // lower right quadrants
            Res = -Res;
      }
    return Res;
  }

void ComplexCalcModArg (Complex *a)
  {
    if (!a->ModArgValid)
      {
        a->Mod = sqrt (Mod2 (a));
        a->Arg = Atan2 (a->Imaginary, a->Real);
        a->ModArgValid = true;
      }
  }

bool ComplexCalcRealImaginary (Complex *a)
  {
    if (!a->ModArgValid)
      return false;
    a->Real = a->Mod * cos (a->Arg);
    a->Imaginary = a->Mod * sin (a->Arg);
    return true;
  }


////////////////////////////////////////////////////////////////////////////
//
// CONVERSION BETWEEN COMPLEX AND REALS

void ComplexSet  (double Real, double Imaginary, Complex *Result)
  {
    Result->Real = Real;
    Result->Imaginary = Imaginary;
    Result->ModArgValid = false;
  }

void ComplexGet (Complex *a, double *Real, double *Imaginary)
  {
    *Real = a->Real;
    *Imaginary = a->Imaginary;
  }

void ComplexSetModArg  (double Mod, double Arg, Complex *Result)
  {
    Result->Mod = Mod;
    Result->Arg = Arg;
    Result->ModArgValid = true;
    ComplexCalcRealImaginary (Result);
  }

void ComplexGetModArg (Complex *a, double *Mod, double *Arg)
  {
    ComplexCalcModArg (a);
    *Mod = a->Mod;
    *Arg = a->Arg;
  }

////////////////////////////////////////////////////////////////////////////
//
// ADVANCED FUNCTIONS - Log, Exponential, Power ...

void ComplexExp (const Complex *a, Complex *Result)
  {
    Result->Mod = exp (a->Real);
    Result->Arg = a->Imaginary;
    Result->ModArgValid = true;
    ComplexCalcRealImaginary (Result);
  }

bool ComplexLn (Complex *a, Complex *Result)
  {
    ComplexCalcModArg (a);
    if (a->Mod <= 0.0)
      return false;
    Result->Real = log (a->Mod);
    Result->Imaginary = a->Arg;
    Result->ModArgValid = false;
    return true;
  }

bool ComplexPower (Complex *a, Complex *b, Complex *Result)
  {
    Complex temp1, temp2;
    //
    if (Mod2 (a) == 0.0)
      {
        ComplexSet (0.0, 0.0, Result);
        return true;
      }
    if (!ComplexLn (a, &temp1))
      return false;
    ComplexMul (&temp1, b, &temp2);
    ComplexExp (&temp2, Result);
    return true;
  }

void ComplexSin (const Complex *a, Complex *Result)
  {
    Complex temp1;
    Complex temp2;
    //
    ComplexMul (&C_i, a, &temp1);   // temp1 = ia
    ComplexExp (&temp1, &temp1);   // temp1 = e^(ia)
    ComplexInvert (&temp1, &temp2);   // temp2 = e^(-ia)
    // Result = (e^(ia) - e^(-ia)) / 2i
    ComplexSub (&temp1, &temp2, Result);
    ComplexDiv (Result, &C_2i, Result);
  }

void ComplexCos (const Complex *a, Complex *Result)
  {
    Complex temp1;
    Complex temp2;
    //
    ComplexMul (&C_i, a, &temp1);   // temp1 = ia
    ComplexExp (&temp1, &temp1);   // temp1 = e^(ia)
    ComplexInvert (&temp1, &temp2);   // temp2 = e^(-ia)
    // Result = (e^(ia) + e^(-ia)) / 2
    ComplexAdd (&temp1, &temp2, Result);
    ComplexDiv (Result, &C_2, Result);
  }

void ComplexTan (const Complex *a, Complex *Result)
  {
    Complex temp1;   // sin a
    Complex temp2;   // cos a
    //
    ComplexSin (a, &temp1);
    ComplexCos (a, &temp2);
    ComplexDiv (&temp1, &temp2, Result);
  }

void ComplexArcSinCos (Complex *a, Complex *Result, bool Cos)
  {
    Complex temp1;   // 1 - a^2
    Complex temp2;
    Complex temp3;   // i.a
    //
    // temp1 = (1 - a*2) as r/theta
    ComplexMul (a, a, &temp1);
    ComplexSub (&C_1, &temp1, &temp1);
    ComplexCalcModArg (&temp1);
    // temp2 = Sqrt (Mod (temp1)) X Exp (i/2 X Arg (temp1))
    ComplexSet (temp1.Mod, 0, &temp2);   // temp2 = sqrt mod temp1
    ComplexSqrt (&temp2, &temp2);
    ComplexSet (0, temp1.Arg, &temp3);   // temp3 = i X Arg (temp1)
    ComplexDiv (&temp3, &C_2, &temp3);   //   / 2
    ComplexExp (&temp3, &temp3);         //   e^
    ComplexMul (&temp2, &temp3, &temp2);
    //
    if (Cos)
      {
        ComplexMul (&temp2, &C_i, &temp3);   // temp3 = a + i X temp2
        ComplexAdd (a, &temp3, &temp3);
      }
    else   // Sin
      {
        ComplexMul (&C_i, a, &temp3);   // temp3 = i.a + temp2
        ComplexAdd (&temp3, &temp2, &temp3);
      }
    ComplexLn (&temp3, Result);
    ComplexDiv (Result, &C_i, Result);
  }

void ComplexArcSin (Complex *a, Complex *Result)
  {
    ComplexArcSinCos (a, Result, false);
  }

void ComplexArcCos (Complex *a, Complex *Result)
  {
    ComplexArcSinCos (a, Result, true);
  }

bool ComplexArcTan (Complex *a, Complex *Result)
  {
    Complex temp1;
    Complex temp2;
    //
    ComplexSub (&C_i, a, &temp1);   // temp1 = i - a
    ComplexAdd (&C_i, a, &temp2);   // temp2 = i + a
    if (!ComplexDiv (&temp1, &temp2, Result))   // Result = temp1 / temp2
      return false;
    ComplexLn (Result, Result);   // Result = ln (Result) / 2i
    ComplexDiv (Result, &C_2i, Result);
    return true;
  }

bool ComplexOK (Complex *a)
  {
    if (isnan (a->Real))
      return false;
    if (isnan (a->Imaginary))
      return false;
    if (isinf (a->Real))
      return false;
    if (isinf (a->Imaginary))
      return false;
    return true;
  }
