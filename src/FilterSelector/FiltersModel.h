/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersModel.h
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
#ifndef _GMIC_QT_FILTERSMODEL_H_
#define _GMIC_QT_FILTERSMODEL_H_
#include <QList>
#include <QMap>
#include <QString>
#include <cstddef>
#include <vector>

class FiltersModel {
public:
  class Filter {
  public:
    Filter();
    Filter & setName(QString name);
    Filter & setCommand(QString command);
    Filter & setPreviewCommand(QString previewCommand);
    Filter & setParameters(QString parameters);
    Filter & setPreviewFactor(float factor);
    Filter & setAccurateIfZoomed(bool accurate);
    Filter & setPath(const QList<QString> & path);
    Filter & setWarningFlag(bool flag);
    Filter & build();

    QString name() const;
    QString plainText() const;
    const QList<QString> & path() const;
    QString hash() const;
    QString command() const;
    QString previewCommand() const;
    QString parameters() const;
    float previewFactor() const;
    bool isAccurateIfZoomed() const;
    bool isWarning() const;

    bool matchKeywords(const QList<QString> & keywords) const;

  private:
    QString _name;
    QString _plainText;
    QList<QString> _path;
    QList<QString> _plainPath;
    QString _command;
    QString _previewCommand;
    QString _parameters;
    float _previewFactor;
    bool _isAccurateIfZoomed;
    QString _hash;
    bool _isWarning;
  };

  FiltersModel();
  ~FiltersModel();

public:
  void clear();
  void addFilter(const Filter & filter);
  void flush();
  size_t filterCount() const;
  size_t notTestingFilterCount() const;
  const Filter & getFilter(size_t index) const;
  size_t getFilterIndexFromHash(const QString & hash);
  const Filter & getFilterFromHash(const QString & hash);
  bool contains(const QString & hash) const;
  static const size_t NoIndex;

private:
  std::vector<Filter> _filters;
  QMap<QString, size_t> _hash2filterIndex;
};

#endif // _GMIC_QT_FILTERSMODEL_H_
