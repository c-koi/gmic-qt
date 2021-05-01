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
#ifndef GMIC_QT_FAVESMODEL_H
#define GMIC_QT_FAVESMODEL_H
#include <QList>
#include <QMap>
#include <QString>
#include <cstddef>

class FavesModel {
public:
  class Fave {
  public:
    Fave & setName(const QString & name);
    Fave & setOriginalName(const QString & name);
    Fave & setCommand(const QString & command);
    Fave & setPreviewCommand(const QString & command);
    Fave & setOriginalHash(const QString & hash);
    Fave & setDefaultValues(const QList<QString> & defaultValues);
    Fave & setDefaultVisibilities(const QList<int> & defaultVisibilityStates);
    Fave & build();

    const QString & name() const;
    const QString & plainText() const;
    const QString absolutePath() const;
    const QString & originalName() const;
    const QString & originalHash() const;
    const QString & command() const;
    const QString & previewCommand() const;
    const QString & hash() const;
    const QList<QString> & defaultValues() const;
    const QList<int> & defaultVisibilityStates() const;
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
    QList<int> _defaultVisibilityStates;
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
  const_iterator findFaveFromHash(const QString &) const;
  const_iterator findFaveFromPlainText(const QString &) const;
  const Fave & getFaveFromHash(const QString & hash) const;
  QString uniqueName(const QString & name, const QString & faveHashToIgnore);
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
#endif // GMIC_QT_FAVESMODEL_H
