/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Tags.cpp
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
#include "Tags.h"
#include "Common.h"

#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QImage>
#include <QPainter>

namespace GmicQt
{

const char * TagColorNames[] = {"", "Red", "Green", "Blue"};

QString TagAssets::_markerHtml[static_cast<unsigned int>(TagColor::Count)];
unsigned int TagAssets::_markerSideSize[static_cast<unsigned int>(TagColor::Count)];
QColor TagAssets::_colors[static_cast<unsigned int>(TagColor::Count)] = {QColor(0, 0, 0, 0), Qt::red, Qt::green, Qt::blue};

const QString & TagAssets::markerHtml(const TagColor color, unsigned int sideSize)
{
  const int iColor = (int)color;
  if (!_markerHtml[iColor].isEmpty() && _markerSideSize[iColor] == sideSize) {
    return _markerHtml[iColor];
  }
  QImage image(sideSize, sideSize, QImage::Format_RGBA8888);
  image.fill(QColor(0, 0, 0, 0));
  if (color != TagColor::None) {
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen = painter.pen();
    pen.setWidth(2);
    pen.setColor(Qt::black);
    painter.setPen(pen);
    const int innerRadius = sideSize * 0.4;
    painter.setBrush(_colors[iColor]);
    painter.drawEllipse(image.rect().center(), innerRadius, innerRadius);
  }
  QByteArray ba;
  QBuffer buffer(&ba);
  image.save(&buffer, "png");
  _markerSideSize[iColor] = sideSize;
  _markerHtml[iColor] = QString("<img src=\"data:image/png;base64,%1\">").arg(QString(ba.toBase64()));
  return _markerHtml[iColor];
}

} // namespace GmicQt
