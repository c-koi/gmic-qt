/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterTreeAbstractItem.cpp
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
#include "FilterTreeAbstractItem.h"
#include "FilterTextTranslator.h"
#include "Globals.h"
#include "HtmlTranslator.h"

FilterTreeAbstractItem::FilterTreeAbstractItem(QString text)
{
  _visibilityItem = nullptr;
  if (text.startsWith(GmicQt::WarningPrefix)) {
    text.remove(0, 1);
    _isWarning = true;
  } else {
    _isWarning = false;
  }
  setText(FilterTextTranslator::translate(text));
  _plainText = HtmlTranslator::html2txt(FilterTextTranslator::translate(text), true);
}

FilterTreeAbstractItem::~FilterTreeAbstractItem() {}

void FilterTreeAbstractItem::setVisibilityItem(QStandardItem * item)
{
  _visibilityItem = item;
}

const QString & FilterTreeAbstractItem::plainText() const
{
  return _plainText;
}

bool FilterTreeAbstractItem::isWarning() const
{
  return _isWarning;
}

bool FilterTreeAbstractItem::isVisible() const
{
  if (_visibilityItem) {
    return _visibilityItem->checkState() == Qt::Checked;
  }
  return true;
}

void FilterTreeAbstractItem::setVisibility(bool flag)
{
  if (_visibilityItem) {
    _visibilityItem->setCheckState(flag ? Qt::Checked : Qt::Unchecked);
  }
}

QStringList FilterTreeAbstractItem::path() const
{
  QStringList result;
  result.push_back(text());
  const FilterTreeAbstractItem * parentFolder = dynamic_cast<FilterTreeAbstractItem *>(parent());
  while (parentFolder) {
    result.push_front(parentFolder->text());
    parentFolder = dynamic_cast<FilterTreeAbstractItem *>(parentFolder->parent());
  }
  return result;
}

QString FilterTreeAbstractItem::removeWarningPrefix(QString folderName)
{
  if (folderName.startsWith(GmicQt::WarningPrefix)) {
    folderName.remove(0, 1);
  }
  return folderName;
}
