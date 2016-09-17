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

#include "PnmFile.h"
#include "Utilities/Exception.h"  // BLEPO_ERROR
#include <stdio.h>		// FILE 
#include <stdlib.h>		// malloc(), atoi()

// -------------------- all includes must go before these lines ------------------
#if defined(DEBUG) && defined(WIN32) && !defined(NO_MFC)
#include <afxwin.h>
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------- all code must go after these lines -----------------------

using namespace blepo;  // for BLEPO_ERROR

//////////////////////////////////////////////////////////////////////
// _getNextString
//
// Helper function to get the next string.
// 'length' is the number of characters in 'line'
static void _getNextString(
   FILE *fp,
   char *line,
   int length)
{
   int i;
   
   line[0] = '\0';
   
   while (line[0] == '\0')  {
     char fmt[100];
     sprintf(fmt, "%%%ds", length);  // e.g., fscanf(fmt, "%80s", length)
      fscanf(fp, fmt, line);
// printf("String before removing comment: %s\n", line);
      i = -1;
      do  {
         i++;
         if (line[i] == '#')  {
            line[i] = '\0';
            while (fgetc(fp) != '\n') ;
         }
      }  while (line[i] != '\0');
// printf("                         After: %s\n", line);
   }
// printf("Final string: %s\n", line);
}


//////////////////////////////////////////////////////////////////////
// pnmReadHeader
//
// Reads a pnm header from an open stream.

void pnmReadHeader(
   FILE *fp, 
   int *magic, 
   int *ncols, int *nrows, 
   int *maxval)
{
  const int length = 80;
  char line[length];
   
   // Read magic number
   _getNextString(fp, line, length);
   if (line[0] != 'P')
      BLEPO_ERROR(StringEx("(pnmReadHeader) Magic number does not begin with 'P', "
            "but with a '%c'", line[0]))
   sscanf(line, "P%d", magic);
   
   // Read size, skipping comments
   _getNextString(fp, line, length);
   *ncols = atoi(line);
   _getNextString(fp, line, length);
   *nrows = atoi(line);
   if (*ncols < 0 || *nrows < 0 || *ncols > 10000 || *nrows > 10000)
      BLEPO_ERROR(StringEx("(pnmReadHeader) The dimensions %d x %d are unacceptable",
            *ncols, *nrows))
   
   // Read maxval, skipping comments
   _getNextString(fp, line, length);
   *maxval = atoi(line);
   fread(line, 1, 1, fp); // Read newline which follows maxval

   if (*maxval != 255)
   {
    BLEPO_ERROR(StringEx("(pnmReadHeader) Maxval is %d, not 255", *maxval));  // But does this ever happen?
   }
}


//////////////////////////////////////////////////////////////////////
// pgmReadHeader
//
// Reads a pgm header from an open stream.

void pgmReadHeader(
   FILE *fp, 
   int *magic, 
   int *ncols, int *nrows, 
   int *maxval)
{
   pnmReadHeader(fp, magic, ncols, nrows, maxval);
   if (*magic != 5)
   {
      BLEPO_ERROR(StringEx("(pgmReadHeader) Magic number is not 'P5', but 'P%d'", *magic));
   }
}


//////////////////////////////////////////////////////////////////////
// ppmReadHeader
//
// Reads a ppm header from an open stream.

void ppmReadHeader(
   FILE *fp, 
   int *magic, 
   int *ncols, int *nrows, 
   int *maxval)
{
   pnmReadHeader(fp, magic, ncols, nrows, maxval);
   if (*magic != 6)
      BLEPO_ERROR(StringEx("(ppmReadHeader) Magic number is not 'P6', but 'P%d'", *magic))
}


//////////////////////////////////////////////////////////////////////
// pgmReadHeaderFile
//
// Reads the header of a pgm file.

void pgmReadHeaderFile(
   const char *fname, 
   int *magic, 
   int *ncols, int *nrows, 
   int *maxval)
{
   FILE *fp;
   
   // Open file
   if ( (fp = fopen(fname, "rb")) == NULL)
      BLEPO_ERROR(StringEx("(pgmReadHeaderFile) Can't open file named '%s' for reading\n", fname))
   
   // Read header
   pgmReadHeader(fp, magic, ncols, nrows, maxval);
   
   // Close file
   fclose(fp);
}


//////////////////////////////////////////////////////////////////////
// ppmReadHeaderFile
//
// Reads the header of a ppm file.

void ppmReadHeaderFile(
   const char *fname, 
   int *magic, 
   int *ncols, int *nrows, 
   int *maxval)
{
   FILE *fp;
   
   // Open file
   if ( (fp = fopen(fname, "rb")) == NULL)
      BLEPO_ERROR(StringEx("(ppmReadHeaderFile) Can't open file named '%s' for reading\n", fname))
   
   // Read header
   ppmReadHeader(fp, magic, ncols, nrows, maxval);
   
   // Close file
   fclose(fp);
}


//////////////////////////////////////////////////////////////////////
// pgmRead
//
// Reads a pgm image from an open stream.  Allocates memory if img==NULL.

unsigned char* pgmRead(
   FILE *fp,
   unsigned char *img,
   int *ncols, int *nrows)
{
   unsigned char *ptr;
   int magic, maxval;
   
   // Read header
   pgmReadHeader(fp, &magic, ncols, nrows, &maxval);
   
   // Allocate memory, if necessary, and set pointer
   if (img == NULL)  {
      ptr = (unsigned char*) malloc(*ncols * *nrows * sizeof(char));
      if (ptr == NULL)  
         BLEPO_ERROR(StringEx("(pgmRead) Memory not allocated"))
   }
   else
      ptr = img;
   
   // Read binary image data
   fread(ptr, *ncols * *nrows, 1, fp);
   
   return ptr;
}


//////////////////////////////////////////////////////////////////////
// pgmReadFile
//
// Reads an image from a pgm file.  Allocates memory if img==NULL.

unsigned char* pgmReadFile(
   const char *fname,
   unsigned char *img,
   int *ncols, int *nrows)
{
   unsigned char *ptr;
   FILE *fp;
   
   // Open file
   if ( (fp = fopen(fname, "rb")) == NULL)
      BLEPO_ERROR(StringEx("(pgmReadFile) Can't open file named '%s' for reading\n", fname))
   
   // Read file
   ptr = pgmRead(fp, img, ncols, nrows);
   
   // Close file
   fclose(fp);
   
   return ptr;
}


//////////////////////////////////////////////////////////////////////
// ppmRead
//
// Reads a ppm image from an open stream.  Allocates memory if img==NULL.

unsigned char* ppmRead(
   FILE *fp,
   unsigned char *img,
   int *ncols, int *nrows)
{
   unsigned char *ptr;
   int magic, maxval;
   
   // Read header
   ppmReadHeader(fp, &magic, ncols, nrows, &maxval);
   
   // Allocate memory, if necessary, and set pointers
   if (img == NULL)  {
      ptr = (unsigned char*) malloc(*ncols * *nrows * 3 * sizeof(char));
      if (ptr == NULL)  
         BLEPO_ERROR(StringEx("(ppmRead) Memory not allocated"))
   }
   else 
      ptr = img;
   
   // Read binary image data
   fread(ptr, *ncols * *nrows * 3, 1, fp);
   
   return ptr;
}


//////////////////////////////////////////////////////////////////////
// ppmReadFile
//
// Reads an image from a ppm file.  Allocates memory if img==NULL. 

unsigned char* ppmReadFile(
   const char *fname,
   unsigned char *img,
   int *ncols, int *nrows)
{
   unsigned char *ptr;
   FILE *fp;
   
   // Open file
   if ( (fp = fopen(fname, "rb")) == NULL)
      BLEPO_ERROR(StringEx("(ppmReadFile) Can't open file named '%s' for reading\n", fname))
   
   // Read file
   ptr = ppmRead(fp, img, ncols, nrows);
   
   // Close file
   fclose(fp);
   
   return ptr;
}


//////////////////////////////////////////////////////////////////////
// pgmWrite
//
// Writes a pgm image to an open stream.

void pgmWrite(
   FILE *fp,
   unsigned char *img, 
   int ncols, 
   int nrows)
{
   int i;
   
   // Write header
   fprintf(fp, "P5\n");
   fprintf(fp, "%d %d\n", ncols, nrows);
   fprintf(fp, "255\n");
   
   // Write binary image data
   for (i = 0 ; i < nrows ; i++)  {
      fwrite(img, ncols, 1, fp);
      img += ncols;
   }
}


//////////////////////////////////////////////////////////////////////
// pgmWriteFile
//
// Writes an image to a pgm file.

void pgmWriteFile(
   const char *fname, 
   unsigned char *img, 
   int ncols, 
   int nrows)
{
   FILE *fp;
   
   // Open file
   if ( (fp = fopen(fname, "wb")) == NULL)
      BLEPO_ERROR(StringEx("(pgmWriteFile) Can't open file named '%s' for writing\n", fname))
   
   // Write to file
   pgmWrite(fp, img, ncols, nrows);
   
   // Close file
   fclose(fp);
}


//////////////////////////////////////////////////////////////////////
// ppmWrite
//
// Writes a ppm image to an open stream.

void ppmWrite(
   FILE *fp,
   unsigned char *img,
   int ncols, 
   int nrows)
{
   // Write header
   fprintf(fp, "P6\n");
   fprintf(fp, "%d %d\n", ncols, nrows);
   fprintf(fp, "255\n");
   
   // Write binary image data
   fwrite(img, ncols * nrows * 3, 1, fp);  
}


//////////////////////////////////////////////////////////////////////
// ppmWriteFile
//
// Write an image to a ppm file.

void ppmWriteFile(
   const char *fname, 
   unsigned char *img,
   int ncols, 
   int nrows)
{
   FILE *fp;
   
   // Open file
   if ( (fp = fopen(fname, "wb")) == NULL)
      BLEPO_ERROR(StringEx("(ppmWriteFile) Can't open file named '%s' for writing\n", fname));
   
   // Write to file
   ppmWrite(fp, img, ncols, nrows);
   
   // Close file
   fclose(fp);
}


//////////////////////////////////////////////////////////////////////
// ppmWriteRGB
//
// Writes R,G,B images to an open stream.

void ppmWriteRGB(
   FILE *fp,
   unsigned char *redimg,
   unsigned char *greenimg,
   unsigned char *blueimg,
   int ncols, 
   int nrows)
{
   int i, j;
   
   // Write header
   fprintf(fp, "P6\n");
   fprintf(fp, "%d %d\n", ncols, nrows);
   fprintf(fp, "255\n");
   
   // Write binary image data
   for (j = 0 ; j < nrows ; j++)  {
      for (i = 0 ; i < ncols ; i++)  {
         fwrite(redimg, 1, 1, fp); 
         fwrite(greenimg, 1, 1, fp);
         fwrite(blueimg, 1, 1, fp);
         redimg++;  greenimg++;  blueimg++;
      }
   }
}


//////////////////////////////////////////////////////////////////////
// ppmWriteFileRGB
//
// Writes R,G,B images to a ppm file.

void ppmWriteFileRGB(
   const char *fname, 
   unsigned char *redimg,
   unsigned char *greenimg,
   unsigned char *blueimg,
   int ncols, 
   int nrows)
{
   FILE *fp;
   
   // Open file
   if ( (fp = fopen(fname, "wb")) == NULL)
      BLEPO_ERROR(StringEx("(ppmWriteFileRGB) Can't open file named '%s' for writing\n", fname))
   
   // Write to file
   ppmWriteRGB(fp, redimg, greenimg, blueimg, ncols, nrows);
   
   // Close file
   fclose(fp);
}


