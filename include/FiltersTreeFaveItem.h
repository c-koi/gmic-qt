/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersTreeFaveItem.h
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
#ifndef _GMIC_QT_FILTERSTREEFAVEITEM_H_
#define _GMIC_QT_FILTERSTREEFAVEITEM_H_

#include <QList>
#include <QString>
#include <QStandardItem>
#include <QStringList>
#include "gmic_qt.h"

#include "FiltersTreeAbstractFilterItem.h"

class InOutPanel;

class FiltersTreeFaveItem : public FiltersTreeAbstractFilterItem {

public:
  FiltersTreeFaveItem(const FiltersTreeAbstractFilterItem * filter,
                      const QString & name,
                      const QList<QString> & defaultValues );
  ~FiltersTreeFaveItem();
  void rename(const QString &);
  FiltersTreeAbstractItem * deepClone(const QStringList & selection) const override;
  FiltersTreeAbstractItem * deepClone() const override;
  QString originalFilterHash() const;
  QString originalFilterName() const;
  QList<QString> defaultValues() const;

protected:
  void updateHash() override;

private:
  QString _originalName;
  QString _originalHash;
  QList<QString> _defaultValues;
};

#endif // _GMIC_QT_FILTERSTREEFAVEITEM_H_
