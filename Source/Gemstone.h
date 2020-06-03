#pragma once

#include "resource.h"
#include <dwrite.h>
#include <vector>

using namespace std;

typedef struct _FONT_
{
	char name[LOCALE_NAME_MAX_LENGTH];
}FONT, *PFONT;