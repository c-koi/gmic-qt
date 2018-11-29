/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterTreeItem.cpp
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
#include "FilterTreeItem.h"
#include <QDebug>
#include "FilterSelector/FiltersView/FilterTreeFolder.h"
#include "HtmlTranslator.h"

FilterTreeItem::FilterTreeItem(const QString & text) : FilterTreeAbstractItem(text)
{
  _isWarning = false;
  _isFave = false;
  setEditable(false);
}

void FilterTreeItem::setHash(const QString & hash)
{
  _hash = hash;
}

void FilterTreeItem::setWarningFlag(bool flag)
{
  _isWarning = flag;
}

void FilterTreeItem::setFaveFlag(bool flag)
{
  _isFave = flag;
  setEditable(flag);
}

bool FilterTreeItem::isWarning() const
{
  return _isWarning;
}

bool FilterTreeItem::isFave() const
{
  return _isFave;
}

QString FilterTreeItem::hash() const
{
  return _hash;
}

bool FilterTreeItem::operator<(const QStandardItem & other) const
{
  auto otherFolder = dynamic_cast<const FilterTreeFolder *>(&other);
  auto otherItem = dynamic_cast<const FilterTreeItem *>(&other);
  Q_ASSERT_X(otherFolder || otherItem, "FilterTreeItem::operator<", "Wrong item types");
  bool otherIsWarning = (otherFolder && otherFolder->isWarning()) || (otherItem && otherItem->isWarning());
  bool otherIsFaveFolder = otherFolder && otherFolder->isFaveFolder();

  // Warnings first
  if (_isWarning && !otherIsWarning) {
    return true;
  }
  if (!_isWarning && otherIsWarning) {
    return false;
  }
  // Then fave folder
  if (otherIsFaveFolder) {
    return false;
  }
  // Then folders
  if (otherFolder) {
    return false;
  }
  // Other cases follow lexicographic order
  if (otherFolder) {
    return plainText().localeAwareCompare(otherFolder->plainText()) < 0;
  }
  return plainText().localeAwareCompare(otherItem->plainText()) < 0;
}
