/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersTreeFolderItem.cpp
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
#include "Common.h"
#include "FiltersTreeFolderItem.h"
#include "FiltersTreeAbstractFilterItem.h"
#include "HtmlTranslator.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QTextDocument>

FiltersTreeFolderItem::FiltersTreeFolderItem(const QString & name, FolderType folderType)
  : FiltersTreeAbstractItem(name),
    _isFaveFolder(folderType == FaveFolder),
    _isWarning( false )
{
}

FiltersTreeFolderItem::~FiltersTreeFolderItem()
{
}

bool FiltersTreeFolderItem::isFaveFolder() const
{
  return _isFaveFolder;
}

void FiltersTreeFolderItem::setWarningFlag(bool on)
{
  _isWarning = on;
}

bool FiltersTreeFolderItem::isWarning() const
{
  return _isWarning;
}

FiltersTreeAbstractItem * FiltersTreeFolderItem::deepClone(const QStringList & words) const
{
  if ( matchWordList(words,plainText()) ) {
    return deepClone();
  }
  int count = rowCount();
  QList<QStandardItem*> list;
  for (int row = 0; row < count; ++row ) {
    QStandardItem * standardItem = child(row);
    FiltersTreeAbstractItem * item = dynamic_cast<FiltersTreeAbstractItem*>(standardItem);
    Q_ASSERT(item);
    FiltersTreeAbstractItem * clone = item->deepClone(words);
    if (clone) {
      list.push_back(clone);
    }
  }
  if (!list.isEmpty()) {
    FiltersTreeFolderItem * clone = new FiltersTreeFolderItem(text(),
                                                              _isFaveFolder ? FaveFolder : NormalFolder);
    clone->appendRows(list);
    return clone;
  }
  return 0;
}

FiltersTreeAbstractItem * FiltersTreeFolderItem::deepClone() const
{
  int count = rowCount();
  QList<QStandardItem*> list;
  for (int row = 0; row < count; ++row ) {
    QStandardItem * standardItem = child(row);
    FiltersTreeAbstractItem * item = dynamic_cast<FiltersTreeAbstractItem*>(standardItem);
    Q_ASSERT(item);
    list.push_back(item->deepClone());
  }
  if (!list.isEmpty()) {
    FiltersTreeFolderItem * clone = new FiltersTreeFolderItem(text(),
                                                              _isFaveFolder ? FaveFolder : NormalFolder);
    clone->appendRows(list);
    return clone;
  }
  return 0;
}

void FiltersTreeFolderItem::setItemsVisibility(bool visible)
{
  int rows = rowCount();
  for ( int row = 0; row < rows; ++row ) {
    FiltersTreeAbstractItem * item = dynamic_cast<FiltersTreeAbstractItem*>(child(row));
    if ( item ) {
      item->setVisibility(visible);
    }
  }
}

bool FiltersTreeFolderItem::isFullyUnchecked()
{
  int count = rowCount();
  for ( int row = 0; row < count; ++row ) {
    FiltersTreeAbstractFilterItem * item = dynamic_cast<FiltersTreeAbstractFilterItem*>(child(row));
    if ( item && item->isVisible() ) {
      return false;
    }
    FiltersTreeFolderItem * folder = dynamic_cast<FiltersTreeFolderItem*>(child(row));
    if ( folder && !folder->isFullyUnchecked() ) {
      return false;
    }
  }
  return true;
}

void FiltersTreeFolderItem::applyVisibilityStatusToFolderContents()
{
  QStandardItem * check = visibilityItem();
  if ( check ) {
    setItemsVisibility(check->checkState() == Qt::Checked);
  }
}
