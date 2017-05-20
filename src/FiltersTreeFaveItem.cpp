/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersTreeFaveItem.cpp
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
#include <QDebug>
#include <QCryptographicHash>
#include "FiltersTreeFaveItem.h"
#include "FiltersTreeFilterItem.h"
#include "Common.h"
#include "HtmlTranslator.h"
#include <QTextDocument>
#include "InOutPanel.h"
#include "DialogSettings.h"

FiltersTreeFaveItem::FiltersTreeFaveItem(const FiltersTreeAbstractFilterItem * filter,
                                         const QString & name,
                                         const QList<QString> & defaultValues )
  : FiltersTreeAbstractFilterItem(name,
                                  filter->command(),
                                  filter->previewCommand(),
                                  filter->previewFactor(),
                                  filter->isAccurateIfZoomed() ),
    _defaultValues(defaultValues)
{
  setParameters(filter->parameters());
  const FiltersTreeFaveItem * fave = dynamic_cast<const FiltersTreeFaveItem*>( filter );
  // Fave from a fave -> get the original name
  if ( fave ) {
    _originalName = fave->originalFilterName();
    _originalHash = fave->originalFilterHash();
  } else {
    _originalName = filter->name();
    _originalHash = filter->hash();
  }
  updateHash();
  setEditable(true);
}

FiltersTreeFaveItem::~FiltersTreeFaveItem()
{
}

void FiltersTreeFaveItem::rename(const QString & name)
{
  setName(name);
  updateHash();
}

FiltersTreeAbstractItem * FiltersTreeFaveItem::deepClone(const QStringList & selection) const
{
  if ( matchWordList(selection,text()) ) {
    return deepClone();
  }
  return 0;
}

FiltersTreeAbstractItem * FiltersTreeFaveItem::deepClone() const
{
  FiltersTreeFaveItem * item = new FiltersTreeFaveItem(this,text(),_defaultValues);
  item->setParameters(_parameters);
  return item;
}

QString FiltersTreeFaveItem::originalFilterHash() const
{
  return _originalHash;
}

QString FiltersTreeFaveItem::originalFilterName() const
{
  return _originalName;
}

QList<QString> FiltersTreeFaveItem::defaultValues() const
{
  return _defaultValues;
}

void FiltersTreeFaveItem::updateHash()
{
  _hash = FiltersTreeAbstractFilterItem::computeHash(text(),
                                                     command(),
                                                     previewCommand(),
                                                     QString("FAVE/"));
}
