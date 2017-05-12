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
#ifndef _GMIC_QT_FILTERSTREEABSTRACTITEM_H_
#define _GMIC_QT_FILTERSTREEABSTRACTITEM_H_

#include <QString>
#include <QStandardItem>

class FiltersTreeFaveItem;
class FiltersTreeFilterItem;

class FiltersTreeAbstractItem : public QStandardItem
{
public:
  FiltersTreeAbstractItem(const QString & name);
  ~FiltersTreeAbstractItem();
  QString name() const;
  QString plainText() const;
  virtual FiltersTreeAbstractItem * deepClone(const QStringList &) const = 0;
  virtual FiltersTreeAbstractItem * deepClone() const = 0;
  virtual bool isWarning() const;

  QList<QString> path() const;
  static int countLeaves( QStandardItem * item );
  static FiltersTreeFaveItem * findFave( QStandardItem * folder, QString hash );
  static FiltersTreeFilterItem * findFilter( QStandardItem * folder, QString hash );
  bool operator<(const QStandardItem & other ) const;
  static bool matchWordList(const QStringList & words, const QString & str);

  void setVisibilityItem( QStandardItem * );
  bool isVisible() const;
  void setVisibility(bool visibility);

protected:
  void setName(QString name);
  QStandardItem * visibilityItem();

private:
  QString _plainText;
  QStandardItem * _visibilityItem;
};

#endif // _GMIC_QT_FILTERSTREEABSTRACTITEM_H_
