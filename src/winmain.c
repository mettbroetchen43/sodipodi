#include <config.h>

#include <stdlib.h>
#include <glib.h>

#ifdef G_OS_WIN32

#undef DATADIR
#include <windows.h>

#include "modules/win32.h"

extern int main (int argc, char **argv);

/* In case we build this as a windowed application */

#ifdef __GNUC__
#  ifndef _stdcall
#    define _stdcall  __attribute__((stdcall))
#  endif
#endif

int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
	 struct HINSTANCE__ *hPrevInstance,
	 char               *lpszCmdLine,
	 int                 nCmdShow)
{
	int ret;
	sp_win32_init (0, NULL, "Sodipodi");
	ret = main (__argc, __argv);
	sp_win32_finish ();
	return ret;
}

#endif
