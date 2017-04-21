/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersTreeAbstractItem.h
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
#include "FiltersTreeAbstractItem.h"
#include "FiltersTreeFolderItem.h"
#include "FiltersTreeAbstractFilterItem.h"
#include "FiltersTreeFilterItem.h"
#include "FiltersTreeFaveItem.h"
#include <QList>
#include <QTextDocument>
#include <QCryptographicHash>
#include <QDebug>
#include "HtmlTranslator.h"
#include "Common.h"

FiltersTreeAbstractItem::FiltersTreeAbstractItem(const QString & name)
  : QStandardItem(name)
{
  setEditable(false);
  _plainText = HtmlTranslator::html2txt(name,true);
}

FiltersTreeAbstractItem::~FiltersTreeAbstractItem()
{
}

QString
FiltersTreeAbstractItem::name() const
{
  return text();
}

QString FiltersTreeAbstractItem::plainText() const
{
  return _plainText;
}

void FiltersTreeAbstractItem::setName(QString name)
{
  setText(name);
  _plainText = HtmlTranslator::html2txt(name,true);
}

bool FiltersTreeAbstractItem::isWarning() const
{
  return false;
}

QList<QString>
FiltersTreeAbstractItem::path() const
{
  QList<QString> result;
  result.push_back(text());
  const FiltersTreeFolderItem * folder = dynamic_cast<FiltersTreeFolderItem*>(parent());
  while ( folder ) {
    result.push_front(folder->text());
    folder = dynamic_cast<FiltersTreeFolderItem*>(folder->parent());
  }
  return result;
}

int FiltersTreeAbstractItem::countLeaves(QStandardItem *item)
{
  if ( dynamic_cast<FiltersTreeFaveItem*>(item) ) {
    return 0;
  }
  if ( dynamic_cast<FiltersTreeFilterItem*>(item) ) {
    return 1;
  }
  FiltersTreeFolderItem * folder = dynamic_cast<FiltersTreeFolderItem*>(item);
  if ( folder && folder->plainText() == QString("Testing") ) {
    return 0;
  }
  int c = 0;
  int rows = item->rowCount();
  for (int row = 0; row < rows; ++row ) {
    c += countLeaves(item->child(row));
  }
  return c;
}

FiltersTreeFaveItem *
FiltersTreeAbstractItem::findFave(QStandardItem * folder, QString hash)
{
  int count = folder->rowCount();
  for (int row = 0; row < count; ++row) {
    FiltersTreeFaveItem * item = dynamic_cast<FiltersTreeFaveItem*>(folder->child(row));
    if ( item && item->hash() == hash ) {
      return item;
    }
  }
  return 0;
}

FiltersTreeFilterItem *
FiltersTreeAbstractItem::findFilter(QStandardItem * folder, QString hash)
{
  int rows = folder->rowCount();
  for (int row = 0; row < rows; ++row ) {
    QStandardItem * item = folder->child(row);
    FiltersTreeFilterItem * filter = static_cast<FiltersTreeFilterItem*>(item);
    FiltersTreeFolderItem * subFolder = static_cast<FiltersTreeFolderItem*>(item);
    if ( filter && filter->hash() == hash ) {
      return filter;
    } else if ( subFolder ) {
      FiltersTreeFilterItem * found = findFilter(subFolder,hash);
      if ( found ) {
        return found;
      }
    }
  }
  return 0;
}

bool FiltersTreeAbstractItem::operator<(const QStandardItem & other) const
{
  const FiltersTreeAbstractItem & o = dynamic_cast<const FiltersTreeAbstractItem &>(other);
  const FiltersTreeFolderItem * folder = dynamic_cast<const FiltersTreeFolderItem *>(this);
  const FiltersTreeFolderItem * other_folder = dynamic_cast<const FiltersTreeFolderItem *>(&other);
  const bool fave_folder = folder && folder->isFaveFolder();
  const bool other_fave_folder = other_folder && other_folder->isFaveFolder();

  // Warnings first
  if ( isWarning() && !o.isWarning() ) {
    return true;
  }
  if ( !isWarning() && o.isWarning() ) {
    return false;
  }
  // Then fave folder
  if ( fave_folder && ! other_fave_folder ) {
    return true;
  }
  if ( ! fave_folder && other_fave_folder ) {
    return false;
  }
  // Then folders
  if ( folder && ! other_folder ) {
    return true;
  }
  if ( ! folder && other_folder ) {
    return false;
  }
  // Other cases follow lexicographic order
  return plainText().localeAwareCompare(o.plainText()) < 0;
}

bool FiltersTreeAbstractItem::matchWordList(const QStringList & words, const QString & str)
{
  for ( const QString & word : words ) {
    if (!str.contains(word,Qt::CaseInsensitive)) {
      return false;
    }
  }
  return true;
}
