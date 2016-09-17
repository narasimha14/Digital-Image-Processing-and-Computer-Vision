The files in the folder are version 6.2b of the JPEG library from the Independent JPEG Group http:://www.ijg.org .  The only change I made was to comment out an exit() call in jerror.c .  Someone from Smaller Animals Software who put together the JpegFile class (in ../) had commented out this line in their code, but they were using version 6.1 of the library.  I have not touched the Smaller Animals code, but it seems to work fine with the new library.

For more info on the JPEG library, see
  http://www.smalleranimals.com/jpegfile.htm

Note that Neeraj Kanhere has now added two files to handle reading/writing of JPEG data from/to memory, rather than from/to files:
  jmemsrc.c
  jmemdst.c
He also added prototypes to these functions in JPEGLIB.H.

Thus, when need to be sure to keep these files and prototypes whenever we upgrade to a new version
of the JPEG library.

-- Stan Birchfield, 10/25/05; 11/3/09
