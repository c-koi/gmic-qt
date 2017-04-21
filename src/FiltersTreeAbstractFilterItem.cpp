/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersTreeAbstractFilterItem.cpp
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
#include "FiltersTreeAbstractFilterItem.h"
#include "gmic_qt.h"
#include <QCryptographicHash>

FiltersTreeAbstractFilterItem::FiltersTreeAbstractFilterItem( const QString & name,
                                                              const QString & command,
                                                              const QString & previewCommand,
                                                              float previewFactor,
                                                              bool accurateIfZoomed )
  : FiltersTreeAbstractItem(name),
    _command( command ),
    _previewCommand( previewCommand ),
    _previewFactor( previewFactor ),
    _isAccurateIfZoomed( accurateIfZoomed )
{
  _hash.clear();
}

FiltersTreeAbstractFilterItem::~FiltersTreeAbstractFilterItem()
{
}

QString FiltersTreeAbstractFilterItem::hash() const
{
  return _hash;
}

QString FiltersTreeAbstractFilterItem::parameters() const
{
  return _parameters;
}

void FiltersTreeAbstractFilterItem::setParameters(const QString & parameters)
{
  _parameters = parameters;
}

QString FiltersTreeAbstractFilterItem::command() const
{
  return _command;
}

QString FiltersTreeAbstractFilterItem::previewCommand() const
{
  return _previewCommand;
}

float FiltersTreeAbstractFilterItem::previewFactor() const
{
  return _previewFactor;
}

bool FiltersTreeAbstractFilterItem::hasPreviewFactor() const
{
  return _previewFactor != GmicQt::PreviewFactorAny;
}

bool FiltersTreeAbstractFilterItem::isAccurateIfZoomed() const
{
  return (_previewFactor == GmicQt::PreviewFactorAny) || _isAccurateIfZoomed;
}

QString FiltersTreeAbstractFilterItem::computeHash(const QString & name,
                                                   const QString & command,
                                                   const QString & previewCommand,
                                                   const QString & prefix)
{
   QCryptographicHash hash(QCryptographicHash::Md5);
   if ( ! prefix.isEmpty() ) {
     hash.addData(prefix.toLocal8Bit());
   }
   hash.addData(name.toLocal8Bit());
   hash.addData(command.toLocal8Bit());
   hash.addData(previewCommand.toLocal8Bit());
   return hash.result().toHex();
}
