/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterTreeFolder.cpp
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
#include "FilterTreeFolder.h"
#include <QDebug>
#include "FilterSelector/FiltersView/FilterTreeItem.h"
#include "HtmlTranslator.h"

FilterTreeFolder::FilterTreeFolder(QString text) : FilterTreeAbstractItem(text)
{
  setEditable(false);
  _isFaveFolder = false;
}

void FilterTreeFolder::setFaveFolderFlag(bool flag)
{
  _isFaveFolder = flag;
}

bool FilterTreeFolder::isFullyUnchecked()
{
  int count = rowCount();
  for (int row = 0; row < count; ++row) {
    FilterTreeAbstractItem * item = dynamic_cast<FilterTreeAbstractItem *>(child(row));
    if (item && item->isVisible()) {
      return false;
    }
    FilterTreeFolder * folder = dynamic_cast<FilterTreeFolder *>(child(row));
    if (folder && !folder->isFullyUnchecked()) {
      return false;
    }
  }
  return true;
}

void FilterTreeFolder::applyVisibilityStatusToFolderContents()
{
  if (_visibilityItem) {
    setItemsVisibility(_visibilityItem->checkState() == Qt::Checked);
  }
}

void FilterTreeFolder::setItemsVisibility(bool visible)
{
  int rows = rowCount();
  for (int row = 0; row < rows; ++row) {
    FilterTreeAbstractItem * item = dynamic_cast<FilterTreeAbstractItem *>(child(row));
    if (item) {
      item->setVisibility(visible);
    }
  }
}

bool FilterTreeFolder::isFaveFolder() const
{
  return _isFaveFolder;
}

bool FilterTreeFolder::operator<(const QStandardItem & other) const
{
  const FilterTreeFolder * otherFolder = dynamic_cast<const FilterTreeFolder *>(&other);
  const FilterTreeItem * otherItem = dynamic_cast<const FilterTreeItem *>(&other);
  Q_ASSERT_X(otherFolder || otherItem, "FilterTreeItem::operator<", "Wrong item types");
  bool otherIsWarning = (otherFolder && otherFolder->isWarning()) || (otherItem && otherItem->isWarning());
  bool otherIsFaveFolder = otherFolder && otherFolder->isFaveFolder();

  // Warnings first
  if (isWarning() && !otherIsWarning) {
    return true;
  }
  if (!isWarning() && otherIsWarning) {
    return false;
  }
  // Then fave folder
  if (_isFaveFolder && !otherIsFaveFolder) {
    return true;
  }
  if (!_isFaveFolder && otherIsFaveFolder) {
    return false;
  }
  // Then folders
  if (!otherFolder) {
    return true;
  }
  // Other cases follow lexicographic order
  if (otherFolder) {
    return plainText().localeAwareCompare(otherFolder->plainText()) < 0;
  } else {
    return plainText().localeAwareCompare(otherItem->plainText()) < 0;
  }
}
