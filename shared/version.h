#pragma once

#define __VERSION 1886

// Throw these in here while MFC still lingers
#define MAX_USER			3000

#define MAX_ID_SIZE			20
#if __VERSION >= 1453
#define MAX_PW_SIZE			28
#else
#define MAX_PW_SIZE			12
#endif
