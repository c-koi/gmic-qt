/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file StoredFave.h
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
#ifndef _GMIC_QT_STOREDFAVE_H_
#define _GMIC_QT_STOREDFAVE_H_

#include <QString>
#include <QList>
#include <QStringList>
#include <QTextStream>

class FiltersTreeFaveItem;

class StoredFave {

public:
  StoredFave(QString name,
             QString originalName,
             QString command,
             QString previewCommand,
             QStringList defaultParameters);
  StoredFave(const FiltersTreeFaveItem *);
  QString originalFilterHash();
  inline QString name() const;
  inline QString originalName() const;
  inline QString command() const;
  inline QString previewCommand() const;
  inline const QStringList & defaultParameters() const;
  inline QTextStream & flush(QTextStream & stream) const;

  static QList<StoredFave> importFaves();
  static QList<StoredFave> readFaves();

private:
  QString _name;
  QString _originalName;
  QString _command;
  QString _previewCommand;
  QStringList _defaultParameters;
};

QTextStream & operator<<(QTextStream & stream, const StoredFave & fave);

QString StoredFave::name() const { return _name; }
QString StoredFave::originalName() const { return _originalName; }
QString StoredFave::command() const  { return _command; }
QString StoredFave::previewCommand() const { return _previewCommand; }
const QStringList & StoredFave::defaultParameters() const { return _defaultParameters; }

#endif // _GMIC_QT_STOREDFAVE_H_
