#pragma once

#include "../shared/stdafx.h"
#include <math.h>
#include "DatabaseThread.h"

#if defined(DEBUG)
#	define DISABLE_PLAYER_BLINKING
#endif

extern bool g_bRunning;