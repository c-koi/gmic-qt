/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FavesModel.h
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
#ifndef _GMIC_QT_FAVESMODEL_H_
#define _GMIC_QT_FAVESMODEL_H_
#include <QList>
#include <QMap>
#include <QString>
#include <cstddef>

class FavesModel {
public:
  class Fave {
  public:
    Fave & setName(QString name);
    Fave & setOriginalName(QString name);
    Fave & setCommand(QString command);
    Fave & setPreviewCommand(QString command);
    Fave & setOriginalHash(QString hash);
    Fave & setDefaultValues(QList<QString> defaultValues);
    Fave & build();

    QString name() const;
    QString plainText() const;
    QString originalName() const;
    QString originalHash() const;
    QString command() const;
    QString previewCommand() const;
    QString hash() const;
    QList<QString> defaultValues() const;
    QString toString() const;
    bool matchKeywords(const QList<QString> & keywords) const;

  private:
    QString _name;
    QString _plainText;
    QString _originalName;
    QString _command;
    QString _previewCommand;
    QString _hash;
    QString _originalHash;
    QList<QString> _defaultValues;
  };

  class const_iterator {
  public:
    const_iterator(const QMap<QString, Fave>::const_iterator & iterator);
    const Fave & operator*() const;
    const_iterator & operator++();
    const_iterator operator++(int);
    const Fave * operator->() const;
    bool operator!=(const FavesModel::const_iterator & other) const;
    bool operator==(const FavesModel::const_iterator & other) const;

  private:
    QMap<QString, Fave>::const_iterator _mapIterator;
  };

  FavesModel();
  ~FavesModel();
  inline const_iterator begin() const;
  inline const_iterator end() const;
  inline const_iterator cbegin() const;
  inline const_iterator cend() const;
  void clear();
  void addFave(const Fave &);
  void removeFave(const QString & hash);
  bool contains(const QString & hash) const;
  void flush() const;
  size_t faveCount() const;
  const_iterator findFaveFromHash(const QString &);
  const Fave & getFaveFromHash(const QString & hash);
  QString uniqueName(QString name, QString faveHashToIgnore);
  static const size_t NoIndex;

private:
  QMap<QString, Fave> _faves;
};

/*
 * Inline methods
 */

FavesModel::const_iterator FavesModel::cbegin() const
{
  return FavesModel::const_iterator(_faves.cbegin());
}

FavesModel::const_iterator FavesModel::cend() const
{
  return FavesModel::const_iterator(_faves.end());
}
FavesModel::const_iterator FavesModel::begin() const
{
  return FavesModel::const_iterator(_faves.cbegin());
}

FavesModel::const_iterator FavesModel::end() const
{
  return FavesModel::const_iterator(_faves.end());
}
#endif // _GMIC_QT_FAVESMODEL_H_
