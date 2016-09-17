/* 
 * Copyright (c) 2004, 2005, 2006, 2007 Clemson University.
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

#ifndef __BLEPO_IMAGE_H__
#define __BLEPO_IMAGE_H__

#include "assert.h"
#include "Utilities/Reallocator.h"
#include "Utilities/PointSizeRect.h"

namespace blepo
{

/**
  @class Image

  Templated base class for an image.  Pixel data are stored contiguously in row-major
  format.

    This base class should work for most image types, such as:
    Image<unsigned char>    8-bit gray-level images
    Image<struct { unsigned b,g,r }>    24-bit BGR images
    Image<float>    single-precision floating-point images
    Image<int>    gray-level images with sizeof(int) bytes per pixel
    etc.
    (But note:  The constructor is not called on the pixels, so do not use a 
     class as the templated type if intialization of the class is required.)

  Packed binary images (one bit per pixel) require specialization because
  sizeof(bool)==1, i.e., a bool occupies a byte of memory even though it stores 
  only a bit.

  @author Stan Birchfield (STB)
*/

template <typename T>
class Image
{
public:
  /// @name Typedefs
  //@{
  typedef T Pixel;
  typedef T& PixelRef;
  typedef T* Iterator;
  typedef const T* ConstIterator;
  //@}

  /// @name Constants
  //@{
	static const int NBITS_PER_PIXEL;   ///< number of bits per pixel
	static const int NCHANNELS;        ///< number of channels
	static const Pixel MIN_VAL;     ///< minimum pixel value 
	static const Pixel MAX_VAL;     ///< maximum pixel value 
  //@}

public:
  /// Constructor / destructor / copy constructor
  //@{
  explicit Image()                       : m_width(0), m_height(0), m_data() {}
  explicit Image(int width, int height)  : m_width(0), m_height(0), m_data() { Reset(width, height); }
  Image(const Image& other)   : m_width(0), m_height(0), m_data() { *this = other; } 
  ~Image() {}
  //@}

  /// Assignment operator
  Image& operator=(const Image& other) 
  { 
    m_width = other.m_width;
    m_height = other.m_height;
    m_data = other.m_data;
    return *this;
  }

  /// @name Reinitialization
  /// After calling Reset(), the image will be in the exact same state as if you 
  /// were to instantiate a new object by calling the constructor with those same parameters.
  /// Notice that the parameters for Reset() are identical to those for the constructor.
  //@{
  void Reset(int width, int height)
  {
    m_width = width;
    m_height = height;
    m_data.Reset(width*height);
  }
  void Reset() { Reset(0,0); }
  //@}

  /// Changes the dimensions of the image without changing the elements.
  /// The number of elements (i.e., width*height) must be the same; 
  ///     otherwise this function has no effect.
  /// Returns true upon success, false otherwise
  bool Reshape(int width, int height)
  {
    if (m_width * m_height == width * height)
    {
      m_width = width;
      m_height = height;
      return true;
    }
    else
    {
      return false;
    }
  }

public:
  /// @name Image info
  //@{
  int Width()  const { return m_width;  }  ///< Number of pixels in a row
  int Height() const { return m_height; }  ///< Number of pixels in a column
  int NBytes() const { return m_width*m_height*sizeof(T); }  ///< Total number of bytes in the image
  int IsNull() const { return m_width==0 || m_height==0; }  ///< Whether image contains any pixels
  //@}

  /// @name Pixel accessing functions (inefficient but convenient)
  //@{
  const Pixel& operator()(int x, int y) const   { return *(Begin(x, y)); }
  Pixel& operator()(int x, int y)               { return *(Begin(x, y)); }
  const Pixel& operator()(int index) const      { return *(Begin(index)); }
  Pixel& operator()(int index)                  { return *(Begin(index)); }
  //@}

  /// @name Iterator functions for fast pixel accessing
  //@{
  ConstIterator Begin() const             { return m_data.Begin(); }
  ConstIterator Begin(int x, int y) const { assert(x>=0 && x<m_width && y>=0 && y<m_height);  return m_data.Begin()+y*m_width+x; }
  ConstIterator Begin(int index) const    { assert(index>=0 && index<m_width*m_height);  return m_data.Begin()+index; }
  ConstIterator End() const               { return m_data.End(); }
  Iterator Begin()                        { return m_data.Begin(); }
  Iterator Begin(int x, int y)            { assert(x>=0 && x<m_width && y>=0 && y<m_height);  return m_data.Begin()+y*m_width+x; }
  Iterator Begin(int index)               { assert(index>=0 && index<m_width*m_height);  return m_data.Begin()+index; }
  Iterator End()                          { return m_data.End(); }
  //@}

  /// Get pointer to raw data, as unsigned char*
  //@{
  const unsigned char* BytePtr()    const { return reinterpret_cast<const unsigned char*>(m_data.Begin()); }
  unsigned char*       BytePtr()          { return reinterpret_cast<unsigned char*>      (m_data.Begin()); }
  const unsigned char* BytePtrEnd() const { return reinterpret_cast<const unsigned char*>(m_data.End());   }
  unsigned char*       BytePtrEnd()       { return reinterpret_cast<unsigned char*>      (m_data.End());   }
  //@}

  /// RectIterator for iterating through a rectangle of an image
  /// Example:
  ///    ImgBgr::RectIterator p = img.BeginRect();
  ///    while (!p.AtEnd())  *p++;
  /// Implementation note:  Be sure not to provide an automatic conversion to 
  /// an Iterator from this class, or someone might accidentally compare with
  /// the end of the image, as in
  ///    while (p != img.End())  *p++
  /// which would yield the wrong behavior with no compile-time warning or error.
  /// Currently, a compile-time error prevents users from inadvertently making
  /// this mistake.
  class RectIterator
  {
  public:
    RectIterator(Image<T>& img, const Rect& rect)
    {
      m_p = img.Begin(rect.left, rect.top);
      m_p_row = m_p + rect.Width();
      m_p_end = img.Begin(rect.left, rect.bottom-1) + img.Width();
      m_skip = img.Width() - rect.Width();
      m_img_width = img.Width();
    }
    Pixel& operator*() const { return *m_p; }
    /// prefix increment (++p)
    RectIterator& operator++()    
    { 
      m_p++;
      if (m_p == m_p_row)  { m_p += m_skip;  m_p_row += m_img_width; }
      return *this; 
    }
    /// postfix increment (p++)
    RectIterator  operator++(int) { RectIterator tmp=*this;  ++(*this);  return tmp; }  
    bool AtEnd() const { return m_p == m_p_end; }
  private:
    Iterator m_p, m_p_row, m_p_end;
    int m_skip, m_img_width;
  };

  class ConstRectIterator
  {
  public:
    ConstRectIterator(const Image<T>& img, const Rect& rect)
    {
      m_p = img.Begin(rect.left, rect.top);
      m_p_row = m_p + rect.Width();
      m_p_end = img.Begin(rect.left, rect.bottom-1) + img.Width();
      m_skip = img.Width() - rect.Width();
      m_img_width = img.Width();
    }
    const Pixel& operator*() const { return *m_p; }
    /// prefix increment (++p)
    ConstRectIterator& operator++()    
    { 
      m_p++;
      if (m_p == m_p_row)  { m_p += m_skip;  m_p_row += m_img_width; }
      return *this; 
    }
    /// postfix increment (p++)
    ConstRectIterator  operator++(int) { ConstRectIterator tmp=*this;  ++(*this);  return tmp; }  
    bool AtEnd() const { return m_p == m_p_end; }
  private:
    ConstIterator m_p, m_p_row, m_p_end;
    int m_skip, m_img_width;
  };

  RectIterator BeginRect(const Rect& rect) { return RectIterator(*this, rect); }
  ConstRectIterator BeginRect(const Rect& rect) const { return ConstRectIterator(*this, rect); }

private:
  int m_width, m_height;  ///< image dimensions
  Reallocator<T> m_data;  //< image data
};


/**
  @class Image<bool>
  Specialization for a packed binary image, with one bit per pixel.  The first pixel 
  is stored in the most significant bit of the first byte, the second pixel in the 
  next bit, and so on.  If Width()*Height() is not divisible by 8, then the final 
  byte will contain some unused bits (in its least significant bits).  

  The interface of this class is exactly the same as the other image classes.  The
  only exception is that sizeof(Image<bool>::Pixel) returns sizeof(bool)=1 byte, 
  which is incorrect because only one bit per pixel is actually used.  It is this
  inconsistency in the native bool datatype that causes the code to be so complicated.
  Keep in mind that the packed bytes can be accessed directly using the BytePtr() 
  functions.
  
  Allow me to say a word about the ordering of the pixels in the packed bytes.  The 
  ordering described above seems to be the most natural representation because 
     (1) pixels with smaller coordinates are stored in lower memory addresses,
         i.e., the first byte of the allocated block of memory contains the first
         eight pixels of the image; and
     (2) a bit shift right causes pixels to be shifted to the right.

  Unfortunately, this choice has a drawback.  Intel processors use the little-endian 
  format, which means that when a word of memory is loaded into a register, the byte at
  the low memory address is stored in the low byte of the word.  For example, suppose
  the first two bytes of our image look like this:
       A7-A6-A5-A4-A3-A2-A1-A0-B7-B6-B5-B4-B3-B2-B1-B0,
  where A0 is the least significant bit of the first byte, and B0 the least 
  significant bit of the second byte.  Loading the word (i.e., the 16 bits containing 
  A and B) into a register yields the following contents for the register:
       B7-B6-B5-B4-B3-B2-B1-B0-A7-A6-A5-A4-A3-A2-A1-A0
  because byte A is stored at the lower memory address.  The problem here is that B0 
  and A7 are now adjacent in the register, even though they are not adjacent in the 
  image.  As a result, a bit shift in the register will cause artifacts at the byte 
  boundary.  The same problem occurs at all the byte boundaries after loading 
  a double-word (32 bits), quad-word (64 bits), or double-quad-word (128 bits).
    
  The only way to fix this problem is to discard one of the two goals above.  For
  example, we could store the image backward, with the last eight pixels of the image 
  in the first allocated block.  Incrementing the pointer would cause the image to be
  traversed from right to left and from bottom to top.  This seems unnatural.  
  Alternatively, we could store the first pixel in the least significant bit of the 
  first pixel, the second pixel in the bit to its left, and so forth.  One appealing
  property of this approach is that (in our example above) A0 contains img(0,0), while
  A1 contains img(1,0), i.e., the indices of the image and the bit line up.  On the
  other hand, a terrible consequence is that a bit shift right would actually cause
  the image pixels to shift left, which seems very unnatural indeed.  Although either
  of these choices could be implemented easily without significantly affecting the
  user (who would be largely insulated from the underlying representation via the 
  Iterator class), it is not clear which is the best choice.  

  For clarity, let us define the four choices according to whether traversing the image
  pixels from the top left to the bottom right either
     - increases or decreases the bit number (0 to 7)
     - increases or decreases the byte number (memory address)
  The four choices are
    (1) increasing bits, increasing bytes (IBIB)
        Good:  works for Little Endian machines
        Bad:   bit shift left actually shifts pixels right
    (2) increasing bits, decreasing bytes (IBDB)
        Good:  works for Little Endian machines
        Bad:   first byte is at end of allocated block
    (3) decreasing bits, increasing bytes (DBIB)
        Good:  natural choice
        Bad:   works for Big Endian machines
    (4) decreasing bits, decreasing bytes (DBDB)
        Bad:   first byte is at end of allocated block
        Bad:   works for Big Endian machines
        (I.e., DBDB is not a viable option b/c it is altogether bad)

  Examples of the four choices are shown below.  A and B are two 
  consecutive bytes, and the ellipsis (...) indicates the rest of
  the pixels in the image.  The values in the four rows above A and
  B are the indices of the first 16 pixels in the image.

     DBDB: ... 08 09 10 11 12 13 14 15 00 01 02 03 04 05 06 07    
     IBDB: ... 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00    
     IBIB:     07 06 05 04 03 02 01 00 15 14 13 12 11 10 09 08 ...
     DBIB:     00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 ...
               A7-A6-A5-A4-A3-A2-A1-A0-B7-B6-B5-B4-B3-B2-B1-B0
        
  Note that this is an unsolved problem, because it really becomes an
  issue only when one wishes to perform bit shifting for the entire image.  
  In Intel's IPP Library, bit shifting does not extend past byte boundaries,
  so these issues are not addressed.

  @author Stan Birchfield (STB)
*/

template <>
class Image<bool>
{
public:
  typedef bool Pixel;

  /// @name Helper classes, because the bool datatype is not packed.
  //@{
  class PixelRef;
  class ConstIterator;

  /// An iterator pointing to a binary pixel, allowing getting and setting of
  /// values, as well as incrementing and decrementing the iterator.
  class Iterator 
  {
  public:
    /// You should generally let Image<bool>::Begin() call this constructor
    /// rather than calling it directly.
    /// @param index The zero-based one-dimensional index of the pixel to which
    ///              this iterator points, by treating image as a 1D array using
    ///              row-major order (i.e., 0 is the first pixel, 1 is the 2nd pixel, etc.)
    /// @param data Pointer to the first byte of the image
    Iterator(int index, unsigned char* data)
    {
      assert(sizeof(unsigned char)==1);  // This assumption seems pretty safe
      m_ptr = data + (index>>3);  // index>>3 <==> indx / 8
      m_bit = 0x80 >> (index & 0x07);  // index & 0x07 <==> index % 8
    }
    /// Okay to call these constructors
    Iterator() : m_ptr(0), m_bit(0) {}
    Iterator(const Iterator& other) : m_ptr(other.m_ptr), m_bit(other.m_bit) {}
    ~Iterator() {}
    Iterator& operator=(const Iterator& other) { m_ptr=other.m_ptr;  m_bit=other.m_bit;  return *this; }
    Pixel operator*() const { return GetValue(); }
    PixelRef operator*() { return PixelRef(*this); }
    bool operator==(const Iterator& other) { return (m_ptr==other.m_ptr && m_bit==other.m_bit); }
    bool operator!=(const Iterator& other) { return (m_ptr!=other.m_ptr || m_bit!=other.m_bit); }
    Iterator& operator++()    { if (m_bit == 0x01)  { m_ptr++;  m_bit = 0x80; } else m_bit >>= 1;  return *this; }  ///< prefix increment (++p)
    Iterator  operator++(int) { Iterator tmp=*this;  ++(*this);  return tmp; }  ///< postfix increment (p++)
    Iterator& operator--()    { if (m_bit == 0x80)  { m_ptr--;  m_bit=0x01; } else m_bit <<= 1;  return *this; }  ///< prefix decrement (--p)
    Iterator  operator--(int) { Iterator tmp=*this;  --(*this);  return tmp; }  ///< postfix decrement (p--)
    Iterator& operator+=(int offset) { m_ptr += (offset>>3);  for (int i=(offset&0x07) ; i>0 ; i--)  ++(*this);  return *this; }
    Iterator& operator-=(int offset) { m_ptr -= (offset>>3);  for (int i=(offset&0x07) ; i>0 ; i--)  --(*this);  return *this; }
    Iterator operator+(int offset) const { Iterator tmp = *this;  tmp += offset;  return tmp; }
    Iterator operator-(int offset) const { Iterator tmp = *this;  tmp -= offset;  return tmp; }
  private:
    // Only allow PixelRef to call GetValue or SetValue
    friend PixelRef;
    Pixel GetValue() const { return ((*m_ptr) & m_bit) != 0; }
    void SetValue(Pixel val) { if (val) *m_ptr = *m_ptr | m_bit;  else *m_ptr = *m_ptr & (~m_bit); }
  private:
    friend class ConstIterator;  // allow ConstIterator to copy these
    unsigned char* m_ptr;
    int m_bit;
  };

  /// A const iterator that disallows setting of values.
  class ConstIterator 
  {
  public:
    /// You should generally let Image<bool>::Begin() call this constructor
    /// rather than calling it directly.
    ConstIterator(int index, const unsigned char* data)
    {
      assert(sizeof(unsigned char)==1);  // This assumption seems pretty safe
      m_ptr = data + (index>>3);  // index>>3 <==> indx / 8
      m_bit = 0x80 >> (index & 0x07);  // index & 0x07 <==> index % 8
    }
    /// Okay to call these constructors
    ConstIterator() : m_ptr(0), m_bit(0) {}
    ConstIterator(const ConstIterator& other) : m_ptr(other.m_ptr), m_bit(other.m_bit) {}
    ConstIterator(const Iterator& other) : m_ptr(other.m_ptr), m_bit(other.m_bit) {}
    ~ConstIterator() {}
    ConstIterator& operator=(const ConstIterator& other) { m_ptr=other.m_ptr;  m_bit=other.m_bit;  return *this; }
    Pixel operator*() const { return GetValue(); }
    bool operator==(const ConstIterator& other) { return (m_ptr==other.m_ptr && m_bit==other.m_bit); }
    bool operator!=(const ConstIterator& other) { return (m_ptr!=other.m_ptr || m_bit!=other.m_bit); }
    ConstIterator& operator++()    { if (m_bit == 0x01)  { m_ptr++;  m_bit = 0x80; } else m_bit >>= 1;  return *this; }  ///< prefix increment (++p)
    ConstIterator  operator++(int) { ConstIterator tmp=*this;  ++(*this);  return tmp; }  ///< postfix increment (p++)
    ConstIterator& operator--()    { if (m_bit == 0x80)  { m_ptr--;  m_bit=0x01; } else m_bit <<= 1;  return *this; }  ///< prefix decrement (--p)
    ConstIterator  operator--(int) { ConstIterator tmp=*this;  --(*this);  return tmp; }  ///< postfix decrement (p--)
    ConstIterator& operator+=(int offset) { m_ptr += (offset>>3);  for (int i=(offset&0x07) ; i>0 ; i--)  ++(*this);  return *this; }
    ConstIterator& operator-=(int offset) { m_ptr -= (offset>>3);  for (int i=(offset&0x07) ; i>0 ; i--)  --(*this);  return *this; }
    ConstIterator  operator+(int offset) const { ConstIterator tmp = *this;  tmp += offset;  return tmp; }
    ConstIterator  operator-(int offset) const { ConstIterator tmp = *this;  tmp -= offset;  return tmp; }
  private:
    // Only allow PixelRef to call GetValue
    friend PixelRef;
    Pixel GetValue() const { return ((*m_ptr) & m_bit) != 0; }
  private:
    const unsigned char* m_ptr;
    int m_bit;
  };

  /// A reference to a binary pixel, allowing getting and setting of values.
  /// (The implementation of the functions is at the bottom of the file, which
  /// is necessary because they use the Iterator class, which is not defined 
  /// until below.)
  class PixelRef
  {
  public:
    PixelRef(const Iterator& it);
    PixelRef& operator=(const Pixel& p);
    operator Pixel() const;
  private:
    Iterator m_it;
  };
  //@}

public:

  /// @name Constants
  //@{
	static const int NBITS_PER_PIXEL;   ///< number of bits per pixel
	static const int NCHANNELS;        ///< number of channels
	static const Pixel MIN_VAL;     ///< minimum pixel value 
	static const Pixel MAX_VAL;     ///< maximum pixel value 
  //@}

  /// returns the number of bytes used to hold the image data of a binary image.
  static int GetNBytes(int width, int height)
  {
    int npix = width * height;
    // Note:  npix>>3 <==> npix/8, and index&0x07 <==> index%8
    return (npix>>3) + ((npix&0x07)==0 ? 0 : 1);
  }

public:
  /// Constructor / destructor / copy constructor
  //@{
  Image<bool>() : m_data(), m_width(0), m_height(0) { Reset(0, 0); }
  Image<bool>(int width, int height) : m_data(), m_width(0), m_height(0) { Reset(width, height); }
  Image<bool>(const Image<bool>& other) : m_data(), m_width(0), m_height(0) { *this = other; }
  virtual ~Image<bool>() {}
  //@}

  /// @name Image reinitialization
  /// After calling Reset(), class object will be in the exact same state as if you 
  /// were to instantiate a new object by calling the constructor with those parameters.
  /// Notice that the parameters for Reset() are identical to those for the constructor.
  //@{
  void Reset(int width, int height)
  {
    m_width = width;
    m_height = height;
    int nbytes = GetNBytes(width, height);
    m_data.Reset(nbytes);
    m_end = Iterator(Width()*Height(), m_data.Begin()); 
    m_end_const = ConstIterator(m_end); 
  }
  void Reset() { Reset(0,0); }
  //@}

  /// Assignment operator
  Image<bool>& operator=(const Image<bool>& other)
  {
    m_width = other.m_width;
    m_height = other.m_height;
    m_data = other.m_data;
    m_end = Iterator(Width()*Height(), m_data.Begin()); 
    m_end_const = ConstIterator(m_end); 
    return *this;
  }

public:
  /// @name Image info
  //@{
  int Width()  const { return m_width;  }  ///< Number of pixels in a row
  int Height() const { return m_height; }  ///< Number of pixels in a column
  int NBytes() const { return GetNBytes(m_width, m_height); }  ///< Total number of bytes in the image
  int IsNull() const { return m_width==0 || m_height==0; }  ///< Whether image contains any pixels
  //@}

  /// @name Pixel accessing functions (inefficient but convenient)
  //@{
  Pixel    operator()(int x, int y) const   { return *(Begin(x, y)); }
  PixelRef operator()(int x, int y)         { return *(Begin(x, y)); }
  Pixel    operator()(int index) const      { return *(Begin(index)); }
  PixelRef operator()(int index)            { return *(Begin(index)); }
  //@}

  /// @name Iterator functions for fast pixel accessing
  /// Note:  End returns a reference, to avoid expensive copy constructor inside loop;
  ///        No need to do this for Begin b/c it does not get called often.
  //@{
  ConstIterator  Begin() const             { return ConstIterator(0, m_data.Begin()); }  // do not call Begin(0,0) or assert will fire if image is NULL
  ConstIterator& End()   const             { return const_cast<ConstIterator&>(m_end_const); }
  ConstIterator  Begin(int x, int y) const { assert(x>=0 && x<m_width && y>=0 && y<m_height);  return ConstIterator(Width()*y+x, m_data.Begin()); }
  ConstIterator  Begin(int index) const    { assert(index>=0 && index<m_width*m_height);  return ConstIterator(index, m_data.Begin()); }
  Iterator       Begin()                   { return Iterator(0, m_data.Begin()); }  // do not call Begin(0,0) or assert will fire if image is NULL
  Iterator&      End()                     { return m_end; } 
  Iterator       Begin(int x, int y)       { assert(x>=0 && x<m_width && y>=0 && y<m_height);  return Iterator(Width()*y+x, m_data.Begin()); }
  Iterator       Begin(int index)          { assert(index>=0 && index<m_width*m_height);  return Iterator(index, m_data.Begin()); }
  //@}

  /// Get pointer to raw data, without convenience of iterator classes
  //@{
  const unsigned char* BytePtr()    const { return m_data.Begin(); }
  unsigned char*       BytePtr()          { return m_data.Begin(); }
  const unsigned char* BytePtrEnd() const { return m_data.End();   }
  unsigned char*       BytePtrEnd()       { return m_data.End();   }
  //@}

private:
  int m_width, m_height;  //< image dimensions
  Reallocator<unsigned char> m_data;  //< image data
  Iterator m_end;  //< points to end (precomputes value for End())
  ConstIterator m_end_const;  //< points to end (precomputes value for End())
};

// Implementation of Image<bool>::PixelRef methods
// (These cannot be defined in the class declaration itself because of circular dependencies.)
inline Image<bool>::PixelRef::PixelRef(const Iterator& it) : m_it(it) {}
inline Image<bool>::PixelRef& Image<bool>::PixelRef::operator=(const Pixel& val) { m_it.SetValue(val);  return *this; }
inline Image<bool>::PixelRef::operator Image<bool>::Pixel() const { return m_it.GetValue(); }

/// A struct to hold three bytes for red, green, and blue color values.
struct Bgr
{
  typedef enum { BLEPO_BGR_XBGR, BLEPO_BGR_XRGB, BLEPO_BGR_GRAY } IntType;

  unsigned char b, g, r;
  Bgr() : b(0), g(0), r(0) {}
  /// Constructor from an int
  /// 'format' governs the interpretation of 'bgr':
  ///    BLEPO_BGR_XBGR => contains BGR values as 0x00BBGGRR (same as COLORREF)
  ///    BLEPO_BGR_XRGB => contains BGR values as 0x00RRGGBB
  ///    BLEPO_BGR_GRAY => contains grayscale values as 0x000000gg (default)
  explicit Bgr(unsigned int bgr, IntType format) 
  { 
    if (format == BLEPO_BGR_XBGR)
    {  // 0x00BBGGRR
      r=bgr&0xFF;  bgr>>=8;  g=bgr&0xFF;  bgr>>=8;  b=bgr&0xFF;
    } 
    else if (format == BLEPO_BGR_XRGB)
    {  // 0x00RRGGBB
      b=bgr&0xFF;  bgr>>=8;  g=bgr&0xFF;  bgr>>=8;  r=bgr&0xFF;
    }
    else // BLEPO_BGR_GRAY (default)
    {  // 0x000000gg (grayscale)
      b=bgr&0xFF;  g=b;  r=b;
    } 
  }
  explicit Bgr(unsigned char blue, unsigned char green, unsigned char red) : b(blue), g(green), r(red) {}
  bool operator==(const Bgr& other) const { return b==other.b && g==other.g && r==other.r; }
  bool operator!=(const Bgr& other) const { return b!=other.b || g!=other.g || r!=other.r; }
  void FromInt(unsigned int bgr, IntType format)
  {
    *this = Bgr(bgr, format);
  }
  int ToInt(IntType format) const
  {
    if (format == BLEPO_BGR_XBGR)
    {  // 0x00BBGGRR
      return (b<<16) | (g<<8) | (r);
    }
    else if (format == BLEPO_BGR_XRGB)
    {  // 0x00RRGGBB
      return (r<<16) | (g<<8) | (b);
    }
    else // BLEPO_BGR_GRAY (default)
    {  // 0x000000gg (grayscale)
      return ToGray();
    } 
  }
  unsigned char ToGray() const 
  { 
    int val = (1*b + 6*g + 3*r) / 10;
    if (val < 0)    val = 0;
    if (val > 255)  val = 255;
    return val;
  }

  // built-in colors
  static Bgr BLUE, GREEN, RED, YELLOW, CYAN, MAGENTA, WHITE, BLACK;
};


/**
  @class ImgBgr
    A color image in the blue-green-red (BGR) color space.  Each pixel occupies 3 bytes, 
    with colors interleaved, i.e., B0G0R0B1G1R1...

  @class ImgBinary
    A packed binary image with one bit per pixel.

  @class ImgFloat
    A single-precision floating-point image, with each pixel occupying four bytes.

  @class ImgGray
    A grayscale image, with each pixel occupying a single byte (unsigned char).

  @class ImgInt
    An integer image, with each pixel occupying a single integer.  Because integers
    are machine-dependent, the pixel size is not guaranteed, but it will generally
    be four bytes on a 32-bit machine.

  @author Stan Birchfield (STB)
*/

typedef Image<Bgr> ImgBgr;
typedef Image<float> ImgFloat;
typedef Image<unsigned char> ImgGray;
typedef Image<signed int> ImgInt;
typedef Image<bool> ImgBinary;

};  // end namespace blepo

#endif //__BLEPO_IMAGE_H__
