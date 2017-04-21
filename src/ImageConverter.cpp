/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ImageConverter.cpp
 *
 *  Copyright 2017 Sebastien Fourey
 *
 *  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
 *  editors, offering hundreds of filters thanks to the underlying G'MIC
 *  image processing framework.
 *
 *  gmic_qt is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  gmic_qt is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gmic_qt.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <QImage>
#include <QDebug>
#include "Common.h"
#include "ImageConverter.h"
#include "gmic.h"

namespace {
inline bool archIsLittleEndian()
{
  const int x = 1;
  return (*reinterpret_cast<const unsigned char*>(&x));
}
}

// TODO : Optimize as mush as possible!
void ImageConverter::convert(const cimg_library::CImg<float> & in, QImage & out)
{
  Q_ASSERT_X(in.spectrum() <= 4,
             "ImageConverter::convert()",
             QString("bad input spectrum (%1)").arg(in.spectrum()).toLatin1() );;

  out = QImage(in.width(),in.height(),QImage::Format_RGB888);

  if ( in.spectrum() == 4 && out.format() != QImage::Format_ARGB32 ) {
    out = out.convertToFormat(QImage::Format_ARGB32);
  }

  if ( in.spectrum() == 3 && out.format() != QImage::Format_RGB888 ) {
    out = out.convertToFormat(QImage::Format_RGB888);
  }

  if ( in.spectrum() == 2 && out.format() != QImage::Format_ARGB32 ) {
    out = out.convertToFormat(QImage::Format_ARGB32);
  }

  // Format_Grayscale8 was added in Qt 5.5.
#if ((QT_VERSION_MAJOR == 5) && (QT_VERSION_MINOR>4)) || (QT_VERSION_MAJOR>=6)
  if ( in.spectrum() == 1 && out.format() != QImage::Format_Grayscale8 ) {
    out = out.convertToFormat(QImage::Format_Grayscale8);
  }
#else
  if ( in.spectrum() == 1  ) {
    out = out.convertToFormat(QImage::Format_RGB888);
  }
#endif

  if ( in.spectrum() == 3 ) {
    const float *srcR = in.data(0,0,0,0);
    const float *srcG = in.data(0,0,0,1);
    const float *srcB = in.data(0,0,0,2);
    int height = out.height();
    for ( int y = 0; y < height; ++y ) {
      int n = in.width();
      unsigned char * dst = out.scanLine(y);
      while (n--) {
        dst[0] = static_cast<unsigned char>(*srcR++);
        dst[1] = static_cast<unsigned char>(*srcG++);
        dst[2] = static_cast<unsigned char>(*srcB++);
        dst += 3;
      }
    }
  } else if ( in.spectrum() == 4 ) {
    const float *srcR = in.data(0,0,0,0);
    const float *srcG = in.data(0,0,0,1);
    const float *srcB = in.data(0,0,0,2);
    const float *srcA = in.data(0,0,0,3);
    int height = out.height();
    if (archIsLittleEndian()) {
      for ( int y = 0; y < height; ++y ) {
        int n = in.width();
        unsigned char * dst = out.scanLine(y);
        while (n--) {
          dst[0] = static_cast<unsigned char>(*srcB++);
          dst[1] = static_cast<unsigned char>(*srcG++);
          dst[2] = static_cast<unsigned char>(*srcR++);
          dst[3] = static_cast<unsigned char>(*srcA++);
          dst += 4;
        }
      }
    } else {
      for ( int y = 0; y < height; ++y ) {
        int n = in.width();
        unsigned char * dst = out.scanLine(y);
        while (n--) {
          dst[0] = static_cast<unsigned char>(*srcA++);
          dst[1] = static_cast<unsigned char>(*srcR++);
          dst[2] = static_cast<unsigned char>(*srcG++);
          dst[3] = static_cast<unsigned char>(*srcB++);
          dst += 4;
        }
      }
    }
  } else if ( in.spectrum() == 2 ) {
    //
    // Gray + Alpha
    //
    const float *src = in.data(0,0,0,0);
    const float *srcA = in.data(0,0,0,1);
    int height = out.height();
    if (archIsLittleEndian()) {
      for ( int y = 0; y < height; ++y ) {
        int n = in.width();
        unsigned char * dst = out.scanLine(y);
        while (n--) {
          dst[2] = dst[1] = dst[0] = static_cast<unsigned char>(*src++);
          dst[3] = static_cast<unsigned char>(*srcA++);
          dst+=4;
        }
      }
    } else {
      for ( int y = 0; y < height; ++y ) {
        int n = in.width();
        unsigned char * dst = out.scanLine(y);
        while (n--) {
          dst[1] = dst[2] = dst[3] = static_cast<unsigned char>(*src++);
          dst[0] = static_cast<unsigned char>(*srcA++);
          dst+=4;
        }
      }
    }
  } else {
    //
    // 8-bits Gray levels
    //
    const float *src = in.data(0,0,0,0);
    int height = out.height();
    for ( int y = 0; y < height; ++y ) {
      int n = in.width();
      unsigned char * dst = out.scanLine(y);
#if ((QT_VERSION_MAJOR == 5) && (QT_VERSION_MINOR>4)) || (QT_VERSION_MAJOR>=6)
      while (n--) {
        *dst++ = static_cast<unsigned char>(*src++);
      }
#else
      while (n--) {
        dst[0] = static_cast<unsigned char>(*src);
        dst[1] = static_cast<unsigned char>(*src);
        dst[2] = static_cast<unsigned char>(*src);
        ++src;
        dst+=3;
      }
#endif
    }
  }

}

void ImageConverter::convert(const QImage & in, cimg_library::CImg<float> & out)
{
  Q_ASSERT_X(in.format() == QImage::Format_ARGB32 || in.format() == QImage::Format_RGB888,
             "convert","bad input format");

  if ( in.format() == QImage::Format_ARGB32 ) {
    const int w = in.width();
    const int h = in.height();
    out.assign(w,h,1,4);
    float *dstR = out.data(0,0,0,0);
    float *dstG = out.data(0,0,0,1);
    float *dstB = out.data(0,0,0,2);
    float *dstA = out.data(0,0,0,3);
    if (archIsLittleEndian()) {
      for ( int y = 0; y < h; ++y ) {
        const unsigned char * src = in.scanLine(y);
        int n = in.width();
        while (n--) {
          *dstB++ = static_cast<float>(src[0]);
          *dstG++ = static_cast<float>(src[1]);
          *dstR++ = static_cast<float>(src[2]);
          *dstA++ = static_cast<float>(src[3]);
          src += 4;
        }
      }
    } else {
      for ( int y = 0; y < h; ++y ) {
        const unsigned char * src = in.scanLine(y);
        int n = in.width();
        while (n--) {
          *dstA++ = static_cast<float>(src[0]);
          *dstR++ = static_cast<float>(src[1]);
          *dstG++ = static_cast<float>(src[2]);
          *dstB++ = static_cast<float>(src[3]);
          src += 4;
        }
      }
    }
    return;
  }

  if ( in.format() == QImage::Format_RGB888 ) {
    const int w = in.width();
    const int h = in.height();
    out.assign(w,h,1,3);
    float *dstR = out.data(0,0,0,0);
    float *dstG = out.data(0,0,0,1);
    float *dstB = out.data(0,0,0,2);
    for ( int y = 0; y < h; ++y ) {
      const unsigned char * src = in.scanLine(y);
      int n = in.width();
      while (n--) {
        *dstR++ = static_cast<float>(src[0]);
        *dstG++ = static_cast<float>(src[1]);
        *dstB++ = static_cast<float>(src[2]);
        src += 3;
      }
    }
    return;
  }
}
