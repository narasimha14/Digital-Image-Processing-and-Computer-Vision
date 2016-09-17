/* 
 * Copyright (c) 2004 Stan Birchfield.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __BLEPO_PNMFILE_H__
#define __BLEPO_PNMFILE_H__

//////////////////////////////////////////////////////////////////////
// Code to read and write binary PGM and PPM images.

#include <stdio.h>  // FILE

///////////
// For the reading functions, setting img to NULL causes memory
// to be allocated using malloc.

// Functions for reading from/writing to files
unsigned char* pgmReadFile(
     const char *fname,
     unsigned char *img,
     int *ncols, 
	int *nrows);
void pgmReadHeaderFile(
     const char *fname,
     int *magic,
     int *ncols, int *nrows,
     int *maxval);
unsigned char* ppmReadFile(
     const char *fname,
     unsigned char *img,
     int *ncols, int *nrows);
void ppmReadHeaderFile(
     const char *fname,
     int *magic,
     int *ncols, int *nrows,
     int *maxval);
void pgmWriteFile(
     const char *fname,
     unsigned char *img,
     int ncols,
     int nrows);
void ppmWriteFile(
     const char *fname,
     unsigned char *img,
     int ncols,
     int nrows);
void ppmWriteFileRGB(
     const char *fname,
     unsigned char *redimg,
     unsigned char *greenimg,
     unsigned char *blueimg,
     int ncols,
     int nrows);

// Functions for reading from/writing to streams
unsigned char* pgmRead(
     FILE *fp,
     unsigned char *img,
     int *ncols, int *nrows);
void pgmWrite(
     FILE *fp,
     unsigned char *img,
     int ncols,
     int nrows);
unsigned char* ppmRead(
     FILE *fp,
     unsigned char *img,
     int *ncols, int *nrows);
void ppmWrite(
     FILE *fp,
     unsigned char *img,
     int ncols,
     int nrows);
void ppmWriteRGB(
     FILE *fp,
     unsigned char *redimg,
     unsigned char *greenimg,
     unsigned char *blueimg,
     int ncols,
     int nrows);

// Read the header of a file and determine the type and size of the image
void pnmReadHeader(
     FILE *fp,
     int *magic,
     int *ncols, int *nrows,
     int *maxval);

#endif //__BLEPO_PNMFILE_H__
