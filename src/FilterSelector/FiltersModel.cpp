/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersModel.cpp
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
#include "FilterSelector/FiltersModel.h"
#include <QCryptographicHash>
#include <QDebug>
#include <limits>
#include "HtmlTranslator.h"
#include "gmic_qt.h"

const size_t FiltersModel::NoIndex = std::numeric_limits<size_t>::max();

FiltersModel::FiltersModel()
{
}

FiltersModel::~FiltersModel()
{
}

void FiltersModel::clear()
{
  _filters.clear();
}

void FiltersModel::addFilter(const FiltersModel::Filter & filter)
{
  _filters.push_back(filter);
  _hash2filterIndex[filter.hash()] = _filters.size() - 1;
}

void FiltersModel::flush()
{
  qDebug() << "Filters\n=======";
  for (const Filter & filter : _filters) {
    qDebug() << "[" << filter.path() << "]" << filter.name();
  }
}

size_t FiltersModel::filterCount() const
{
  return _filters.size();
}

size_t FiltersModel::notTestingFilterCount() const
{
  std::vector<Filter>::const_iterator it = _filters.cbegin();
  size_t result = 0;
  while (it != _filters.cend()) {
    const QList<QString> & path = it->path();
    if (!path.startsWith("<b>Testing</b>")) {
      ++result;
    }
    ++it;
  }
  return result;
}

const FiltersModel::Filter & FiltersModel::getFilter(size_t index) const
{
  return _filters[index];
}

size_t FiltersModel::getFilterIndexFromHash(const QString & hash)
{
  QMap<QString, size_t>::iterator it = _hash2filterIndex.find(hash);
  if (it == _hash2filterIndex.end()) {
    return NoIndex;
  }
  return it.value();
}

const FiltersModel::Filter & FiltersModel::getFilterFromHash(const QString & hash)
{
  Q_ASSERT_X(_hash2filterIndex.contains(hash), "FiltersModel::getFilterFromHash()", "Hash not found");
  size_t index = _hash2filterIndex.find(hash).value();
  return _filters[index];
}

bool FiltersModel::contains(const QString & hash) const
{
  return (_hash2filterIndex.find(hash) != _hash2filterIndex.cend());
}

FiltersModel::Filter::Filter()
{
  _previewFactor = GmicQt::PreviewFactorAny;
  _isAccurateIfZoomed = false;
  _isWarning = false;
}

FiltersModel::Filter & FiltersModel::Filter::setName(QString name)
{
  _name = name;
  _plainText = HtmlTranslator::html2txt(name, true);
  return *this;
}

FiltersModel::Filter & FiltersModel::Filter::setCommand(QString command)
{
  _command = command;
  return *this;
}

FiltersModel::Filter & FiltersModel::Filter::setPreviewCommand(QString previewCommand)
{
  _previewCommand = previewCommand;
  return *this;
}

FiltersModel::Filter & FiltersModel::Filter::setParameters(QString parameters)
{
  _parameters = parameters;
  return *this;
}

FiltersModel::Filter & FiltersModel::Filter::setPreviewFactor(float factor)
{
  _previewFactor = factor;
  return *this;
}

FiltersModel::Filter & FiltersModel::Filter::setAccurateIfZoomed(bool accurate)
{
  _isAccurateIfZoomed = accurate;
  return *this;
}

FiltersModel::Filter & FiltersModel::Filter::setPath(const QList<QString> & path)
{
  _path = path;
  return *this;
}

FiltersModel::Filter & FiltersModel::Filter::setWarningFlag(bool flag)
{
  _isWarning = flag;
  return *this;
}

FiltersModel::Filter & FiltersModel::Filter::build()
{
  //
  // Caution : This code is duplicated in FavesModel::Fave::build() to
  //           compute the originalHash of a Fave.
  //
  QCryptographicHash hash(QCryptographicHash::Md5);
  hash.addData(_name.toLocal8Bit());
  hash.addData(_command.toLocal8Bit());
  hash.addData(_previewCommand.toLocal8Bit());
  _hash = hash.result().toHex();
  return *this;
}

QString FiltersModel::Filter::name() const
{
  return _name;
}

QString FiltersModel::Filter::plainText() const
{
  return _plainText;
}

const QList<QString> & FiltersModel::Filter::path() const
{
  return _path;
}

QString FiltersModel::Filter::hash() const
{
  return _hash;
}

QString FiltersModel::Filter::command() const
{
  return _command;
}

QString FiltersModel::Filter::previewCommand() const
{
  return _previewCommand;
}

QString FiltersModel::Filter::parameters() const
{
  return _parameters;
}

float FiltersModel::Filter::previewFactor() const
{
  return _previewFactor;
}

bool FiltersModel::Filter::isAccurateIfZoomed() const
{
  return _isAccurateIfZoomed;
}

bool FiltersModel::Filter::isWarning() const
{
  return _isWarning;
}

bool FiltersModel::Filter::matchKeywords(const QList<QString> & keywords) const
{
  QList<QString>::const_iterator itKeyword = keywords.cbegin();
  while (itKeyword != keywords.cend()) {
    // Check that this keyword is present, either in filter name or in its path
    const QString & keyword = *itKeyword;
    bool keywordInPath = false;
    QList<QString>::const_iterator itPath = _path.cbegin();
    while (itPath != _path.cend() && !keywordInPath) {
      keywordInPath = itPath->contains(keyword, Qt::CaseInsensitive);
      ++itPath;
    }
    if (!keywordInPath && !_plainText.contains(keyword, Qt::CaseInsensitive)) {
      return false;
    }
    ++itKeyword;
  }
  return true;
}
