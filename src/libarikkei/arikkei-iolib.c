#define __ARIKKEI_IOLIB_C__

/*
 * Arikkei
 *
 * Basic datatypes and code snippets
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 *
 */

#ifndef WIN32
#include <unistd.h>
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include "arikkei-strlib.h"

#include "arikkei-iolib.h"

#ifndef S_ISREG
#define S_ISREG(st) 1
#endif

const unsigned char *
arikkei_mmap (const unsigned char *filename, int *size, const unsigned char *name)
{
#ifdef WIN32
	// nr_w32_mmap (const TCHAR *filename, int size, LPCTSTR name)
	TCHAR *tfilename, *tname;
	unsigned char *cdata;
	struct _stat st;
	HANDLE fh, hMapObject;

	if (!filename) return NULL;
#ifdef _UNICODE
	tfilename = arikkei_utf8_ucs2_strdup (filename);
#else
	tfilename = strdup (filename);
#endif

	if (name) {
#ifdef _UNICODE
		tname = arikkei_utf8_ucs2_strdup (name);
#else
		tname = strdup (name);
#endif
	} else {
		static int rval = 0;
		TCHAR tbuf[32];
		_stprintf (tbuf, TEXT ("Object-%d"), rval++);
		tname = _tcsdup (tbuf);
	}

	/* Load file into mmaped memory buffer */
	if (_tstat (tfilename, &st) /* || !S_ISREG (st.st_mode)*/) {
		/* No such file */
		/* fprintf (stderr, "File %s not found or not regular file\n", filename); */
		free (tfilename);
		free (tname);
		return NULL;
	}

	fh = CreateFile (tfilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE) {
		/* No cannot open */
		/* fprintf (stderr, "File %s cannot be opened for reading\n", filename); */
		free (tfilename);
		free (tname);
		return NULL;
	}

	hMapObject = CreateFileMapping (fh, NULL, PAGE_READONLY, 0, 0, tname);

    if (hMapObject != NULL) {
        /* Get a pointer to the file-mapped shared memory. */
        cdata = (char *) MapViewOfFile( 
                hMapObject,     /* object to map view of */
                FILE_MAP_READ, /* read/write access */
                0,              /* high offset:  map from */
                0,              /* low offset:   beginning */
                0);             /* default: map entire file */

        /* if (cdata == NULL) { */
            CloseHandle(hMapObject);
        /* } */
    } else {
        cdata = NULL;
    }

	CloseHandle (fh);

	free (tfilename);
	free (tname);

	*size = st.st_size;

	return cdata;
#else
	unsigned char *cdata;
	struct stat st;
	cdata = NULL;
	if (!stat (filename, &st) && S_ISREG (st.st_mode) && (st.st_size > 8)) {
		int fd;
		fd = open (filename, O_RDONLY);
		if (!fd) return NULL;
		cdata = mmap (NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
		close (fd);
		if ((!cdata) || (cdata == (unsigned char *) -1)) return NULL;
	}

	*size = st.st_size;

	return cdata;
#endif
}

void
arikkei_munmap (const unsigned char *cdata, int size)
{
#ifdef WIN32
	/* Release data */
	UnmapViewOfFile (cdata);
#else
	munmap ((void *) cdata, size);
#endif
}
