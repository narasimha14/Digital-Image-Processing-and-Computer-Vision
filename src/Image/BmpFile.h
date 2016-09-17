#ifndef __BLEPO_BMPFILE_H__
#define __BLEPO_BMPFILE_H__

// For info on BMP files (and source code by Michael Sweet), 
// see http://local.wasp.uwa.edu.au/~pbourke/dataformats/bmp/  -- STB

///////////////////////////////////////////////////////
// BMP file reading and writing, from 
// http://www.smalleranimals.com/jpegfile.htm

#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)
#define BMP_HEADERSIZE (3 * 2 + 4 * 12)
#define BMP_BYTESPERLINE (width, bits) ((((width) * (bits) + 31) / 32) * 4)
#define BMP_PIXELSIZE(width, height, bits) (((((width) * (bits) + 31) / 32) * 4) * height)


class BMPFile
{
//public:
	// parameters
	//CString m_errorText;
	//DWORD m_bytesRead;

public:

	// operations

	BMPFile();
  ~BMPFile();

	static BYTE * LoadBMP(CString fileName, UINT *width, UINT *height);

	static BOOL SaveBMP(CString fileName,		// output path
							BYTE * buf,				// RGB buffer
							UINT width,				// size
							UINT height);

	static BOOL SaveBMP(CString fileName, 			// output path
							 BYTE * colormappedbuffer,	// one BYTE per pixel colomapped image
							 UINT width,
							 UINT height,
 							 int bitsperpixel,			// 1, 4, 8
							 int colors,				// number of colors (number of RGBQUADs)
							 RGBQUAD *colormap);			// array of RGBQUADs 

};

#endif //__BLEPO_BMPFILE_H__
