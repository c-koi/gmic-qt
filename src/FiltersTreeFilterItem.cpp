/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersTreeFilterItem.cpp
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
#include "FiltersTreeFilterItem.h"
#include <QCryptographicHash>
#include <QDebug>
#include "Common.h"
#include <QTextDocument>

FiltersTreeFilterItem::FiltersTreeFilterItem(const QString & name,
                                             const QString & command,
                                             const QString & previewCommand,
                                             float previewFactor,
                                             bool accurateIfZoomed)
  :  FiltersTreeAbstractFilterItem(name,command,previewCommand,previewFactor,accurateIfZoomed),
    _isWarning( false )
{
  updateHash();
#ifdef _GMIC_QT_DEBUG_
  QString str;
  if ( previewFactor == GmicQt::PreviewFactorFullImage ) {
    str = "(Full)";
  } else if ( previewFactor == GmicQt::PreviewFactorActualSize ) {
    str = "(1:1)";
  } else {
    str = "(?)";
  }
  setToolTip(QString("Preview factor: %1 %2\nAccurateIfZoomed: %3").arg(previewFactor).arg(str).arg(accurateIfZoomed));
#endif
}

FiltersTreeFilterItem::~FiltersTreeFilterItem()
{
}

void FiltersTreeFilterItem::setWarningFlag(bool on)
{
  _isWarning = on;
}

bool FiltersTreeFilterItem::isWarning() const
{
  return _isWarning;
}

FiltersTreeAbstractItem * FiltersTreeFilterItem::deepClone(const QStringList & words) const
{
  if ( matchWordList(words,text()) ) {
    return deepClone();
  }
  return 0;
}

FiltersTreeAbstractItem * FiltersTreeFilterItem::deepClone() const
{
  FiltersTreeFilterItem * item = new FiltersTreeFilterItem(text(),
                                                           command(),
                                                           previewCommand(),
                                                           previewFactor(),
                                                           isAccurateIfZoomed());
  item->setParameters(_parameters);
  return item;
}

void FiltersTreeFilterItem::updateHash()
{
  _hash = FiltersTreeAbstractFilterItem::computeHash(text(),command(),previewCommand());
}

