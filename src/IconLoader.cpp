/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file IconLoader.cpp
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

#include "IconLoader.h"
#include <QString>

namespace GmicQt
{

QIcon IconLoader::getForDarkTheme(const QString & name)
{
  QPixmap pixmap(QString(":/icons/dark/%1.png").arg(name));
  QIcon icon(pixmap);
  icon.addPixmap(darkerPixmap(pixmap), QIcon::Disabled, QIcon::Off);
  return icon;
}

QPixmap IconLoader::darkerPixmap(const QPixmap & pixmap)
{
  QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
  for (int row = 0; row < image.height(); ++row) {
    auto pixel = reinterpret_cast<QRgb *>(image.scanLine(row));
    const QRgb * limit = pixel + image.width();
    while (pixel != limit) {
      QRgb grayed;
      if (qAlpha(*pixel) != 0) {
        grayed = qRgba((int)(0.4 * qRed(*pixel)), (int)(0.4 * qGreen(*pixel)), (int)(0.4 * qBlue(*pixel)), (int)(0.4 * qAlpha((*pixel))));
      } else {
        grayed = qRgba(0, 0, 0, 0);
      }
      *pixel++ = grayed;
    }
  }
  return QPixmap::fromImage(image);
}

} // namespace GmicQt
