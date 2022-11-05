#pragma once

#include "resource.h"

#define PROB_LEN 4
#define NUM_LEN 8
#define SPIN_LEN 4
#define _STR(x) #x
#define STR(x) _STR(x)
#define MAX_DECIMAL(x) (int)(1e ## x-1)

extern HINSTANCE hInst;
extern HFONT hNormalFont;
extern HFONT hLargeFont;