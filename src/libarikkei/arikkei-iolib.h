#ifndef __ARIKKEI_IOLIB_H__
#define __ARIKKEI_IOLIB_H__

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

const unsigned char *arikkei_mmap (const unsigned char *filename, unsigned int *size, const unsigned char *name);
void arikkei_munmap (const unsigned char *buffer, unsigned int size);

#endif
