/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ImageTools.cpp
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
#include "ImageTools.h"
#include <QDebug>
#include <QImage>
#include <QPainter>
#include "GmicStdlib.h"
#include "ImageConverter.h"
#include "gmic.h"

/*
 * Part of this code is much inspired by the original source code
 * of the GTK version of the gmic plug-in for GIMP by David Tschumperl\'e.
 */

namespace GmicQt
{

void buildPreviewImage(const cimg_library::CImgList<float> & images, cimg_library::CImg<float> & result, GmicQt::PreviewMode previewMode, int previewWidth, int previewHeight)
{
  cimg_library::CImgList<gmic_pixel_type> preview_input_images;
  switch (previewMode) {
  case GmicQt::FirstOutput:
    if (images && images.size() > 0) {
      preview_input_images.push_back(images[0]);
    }
    break;
  case GmicQt::SecondOutput:
    if (images && images.size() > 1) {
      preview_input_images.push_back(images[1]);
    }
    break;
  case GmicQt::ThirdOutput:
    if (images && images.size() > 2) {
      preview_input_images.push_back(images[2]);
    }
    break;
  case GmicQt::FourthOutput:
    if (images && images.size() > 3) {
      preview_input_images.push_back(images[3]);
    }
    break;
  case GmicQt::First2SecondOutput: {
    preview_input_images.push_back(images[0]);
    preview_input_images.push_back(images[1]);
  } break;
  case GmicQt::First2ThirdOutput: {
    for (int i = 0; i < 3; ++i) {
      preview_input_images.push_back(images[i]);
    }
  } break;
  case GmicQt::First2FourthOutput: {
    for (int i = 0; i < 4; ++i) {
      preview_input_images.push_back(images[i]);
    }
  } break;
  case GmicQt::AllOutputs:
  default:
    preview_input_images = images;
  }

  int spectrum = 0;
  cimglist_for(preview_input_images, l) { spectrum = std::max(spectrum, preview_input_images[l].spectrum()); }
  spectrum += (spectrum == 1 || spectrum == 3);
  cimglist_for(preview_input_images, l) { GmicQt::calibrate_image(preview_input_images[l], spectrum, true); }
  if (preview_input_images.size() == 1) {
    result.swap(preview_input_images.front());
    return;
  }
  if (preview_input_images.size() > 1) {
    try {
      cimg_library::CImgList<char> preview_images_names;
      gmic("gui_preview", preview_input_images, preview_images_names, GmicStdLib::Array.constData(), true);
      if (preview_input_images.size() >= 1) {
        result.swap(preview_input_images.front());
        return;
      }
    } catch (...) {
      QImage qimage(QSize(previewWidth, previewHeight), QImage::Format_ARGB32);
      QPainter painter(&qimage);
      painter.fillRect(qimage.rect(), QColor(40, 40, 40, 200));
      painter.setPen(Qt::green);
      painter.drawText(qimage.rect(), Qt::AlignCenter | Qt::TextWordWrap, "Preview error (handling preview mode)");
      painter.end();
      ImageConverter::convert(qimage, result);
      return;
    }
  }
  result.assign();
}

template <typename T> void image2uchar(cimg_library::CImg<T> & img)
{
  unsigned int len = img.width() * img.height();
  auto dst = reinterpret_cast<unsigned char *>(img.data());
  switch (img.spectrum()) {
  case 1: {
    const T * src = img.data(0, 0, 0, 0);
    while (len--) {
      *dst++ = static_cast<unsigned char>(*src++);
    }
  } break;
  case 2: {
    const T * srcG = img.data(0, 0, 0, 0);
    const T * srcA = img.data(0, 0, 0, 1);
    while (len--) {
      dst[0] = static_cast<unsigned char>(*srcG++);
      dst[1] = static_cast<unsigned char>(*srcA++);
      dst += 2;
    }
  } break;
  case 3: {
    const T * srcR = img.data(0, 0, 0, 0);
    const T * srcG = img.data(0, 0, 0, 1);
    const T * srcB = img.data(0, 0, 0, 2);
    while (len--) {
      dst[0] = static_cast<unsigned char>(*srcR++);
      dst[1] = static_cast<unsigned char>(*srcG++);
      dst[2] = static_cast<unsigned char>(*srcB++);
      dst += 3;
    }
  } break;
  case 4: {
    const T * srcR = img.data(0, 0, 0, 0);
    const T * srcG = img.data(0, 0, 0, 1);
    const T * srcB = img.data(0, 0, 0, 2);
    const T * srcA = img.data(0, 0, 0, 3);
    while (len--) {
      dst[0] = static_cast<unsigned char>(*srcR++);
      dst[1] = static_cast<unsigned char>(*srcG++);
      dst[2] = static_cast<unsigned char>(*srcB++);
      dst[3] = static_cast<unsigned char>(*srcA++);
      dst += 4;
    }
  } break;
  default:
    return;
  }
}

// Calibrate any image to fit the required number of channels (GRAY,GRAYA, RGB or RGBA).
//---------------------------------------------------------------------------------------
template <typename T> void calibrate_image(cimg_library::CImg<T> & img, const int spectrum, const bool is_preview)
{
  if (!img || !spectrum)
    return;
  switch (spectrum) {
  case 1: // To GRAY
    switch (img.spectrum()) {
    case 1: // from GRAY
      break;
    case 2: // from GRAYA
      if (is_preview) {
        T *ptr_r = img.data(0, 0, 0, 0), *ptr_a = img.data(0, 0, 0, 1);
        cimg_forXY(img, x, y)
        {
          const unsigned int a = (unsigned int)*(ptr_a++), i = 96 + (((x ^ y) & 8) << 3);
          *ptr_r = (T)((a * (unsigned int)*ptr_r + (255 - a) * i) >> 8);
          ++ptr_r;
        }
      }
      img.channel(0);
      break;
    case 3: // from RGB
      (img.get_shared_channel(0) += img.get_shared_channel(1) += img.get_shared_channel(2)) /= 3;
      img.channel(0);
      break;
    case 4: // from RGBA
      (img.get_shared_channel(0) += img.get_shared_channel(1) += img.get_shared_channel(2)) /= 3;
      if (is_preview) {
        T *ptr_r = img.data(0, 0, 0, 0), *ptr_a = img.data(0, 0, 0, 3);
        cimg_forXY(img, x, y)
        {
          const unsigned int a = (unsigned int)*(ptr_a++), i = 96 + (((x ^ y) & 8) << 3);
          *ptr_r = (T)((a * (unsigned int)*ptr_r + (255 - a) * i) >> 8);
          ++ptr_r;
        }
      }
      img.channel(0);
      break;
    default: // from multi-channel (>4)
      img.channel(0);
    }
    break;

  case 2: // To GRAYA
    switch (img.spectrum()) {
    case 1: // from GRAY
      img.resize(-100, -100, 1, 2, 0).get_shared_channel(1).fill(255);
      break;
    case 2: // from GRAYA
      break;
    case 3: // from RGB
      (img.get_shared_channel(0) += img.get_shared_channel(1) += img.get_shared_channel(2)) /= 3;
      img.channels(0, 1).get_shared_channel(1).fill(255);
      break;
    case 4: // from RGBA
      (img.get_shared_channel(0) += img.get_shared_channel(1) += img.get_shared_channel(2)) /= 3;
      img.get_shared_channel(1) = img.get_shared_channel(3);
      img.channels(0, 1);
      break;
    default: // from multi-channel (>4)
      img.channels(0, 1);
    }
    break;

  case 3: // to RGB
    switch (img.spectrum()) {
    case 1: // from GRAY
      img.resize(-100, -100, 1, 3);
      break;
    case 2: // from GRAYA
      if (is_preview) {
        T *ptr_r = img.data(0, 0, 0, 0), *ptr_a = img.data(0, 0, 0, 1);
        cimg_forXY(img, x, y)
        {
          const unsigned int a = (unsigned int)*(ptr_a++), i = 96 + (((x ^ y) & 8) << 3);
          *ptr_r = (T)((a * (unsigned int)*ptr_r + (255 - a) * i) >> 8);
          ++ptr_r;
        }
      }
      img.channel(0).resize(-100, -100, 1, 3);
      break;
    case 3: // from RGB
      break;
    case 4: // from RGBA
      if (is_preview) {
        T *ptr_r = img.data(0, 0, 0, 0), *ptr_g = img.data(0, 0, 0, 1), *ptr_b = img.data(0, 0, 0, 2), *ptr_a = img.data(0, 0, 0, 3);
        cimg_forXY(img, x, y)
        {
          const unsigned int a = (unsigned int)*(ptr_a++), i = 96 + (((x ^ y) & 8) << 3);
          *ptr_r = (T)((a * (unsigned int)*ptr_r + (255 - a) * i) >> 8);
          *ptr_g = (T)((a * (unsigned int)*ptr_g + (255 - a) * i) >> 8);
          *ptr_b = (T)((a * (unsigned int)*ptr_b + (255 - a) * i) >> 8);
          ++ptr_r;
          ++ptr_g;
          ++ptr_b;
        }
      }
      img.channels(0, 2);
      break;
    default: // from multi-channel (>4)
      img.channels(0, 2);
    }
    break;

  case 4: // to RGBA
    switch (img.spectrum()) {
    case 1: // from GRAY
      img.resize(-100, -100, 1, 4).get_shared_channel(3).fill(255);
      break;
    case 2: // from GRAYA
      img.resize(-100, -100, 1, 4, 0);
      img.get_shared_channel(3) = img.get_shared_channel(1);
      img.get_shared_channel(1) = img.get_shared_channel(0);
      img.get_shared_channel(2) = img.get_shared_channel(0);
      break;
    case 3: // from RGB
      img.resize(-100, -100, 1, 4, 0).get_shared_channel(3).fill(255);
      break;
    case 4: // from RGBA
      break;
    default: // from multi-channel (>4)
      img.channels(0, 3);
    }
    break;
  }
}

template void image2uchar(cimg_library::CImg<gmic_pixel_type> & img);
template void image2uchar(cimg_library::CImg<unsigned char> & img);
template void calibrate_image(cimg_library::CImg<gmic_pixel_type> & img, const int spectrum, const bool is_preview);
template void calibrate_image(cimg_library::CImg<unsigned char> & img, const int spectrum, const bool is_preview);
} // namespace GmicQt

template <typename T> bool hasAlphaChannel(const cimg_library::CImg<T> & image)
{
  return image.spectrum() == 2 || image.spectrum() == 4;
}

template bool hasAlphaChannel(const cimg_library::CImg<float> &);
template bool hasAlphaChannel(const cimg_library::CImg<unsigned char> &);

QPixmap darkerPixmap(const QPixmap & pixmap)
{
  static int i = 0;
  QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
  image.save(QString("/tmp/icon%1.png").arg(i++));
  for (int row = 0; row < image.height(); ++row) {
    auto pixel = reinterpret_cast<QRgb *>(image.scanLine(row));
    const QRgb * limit = pixel + image.width();
    while (pixel != limit) {
      QColor color;
      if (qAlpha(*pixel) != 0) {
        color.setRed((int)(0.4 * qRed(*pixel)));
        color.setGreen((int)(0.4 * qGreen(*pixel)));
        color.setBlue((int)(0.4 * qBlue(*pixel)));
      } else {
        color.setRgb(0, 0, 0, 0);
      }
      *pixel++ = color.rgba();
    }
  }
  return QPixmap::fromImage(image);
}
