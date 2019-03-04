/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FavesModel.cpp
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
#include "FilterSelector/FavesModel.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QString>
#include <limits>
#include "Common.h"
#include "Globals.h"
#include "HtmlTranslator.h"
#include "gmic_qt.h"

const size_t FavesModel::NoIndex = std::numeric_limits<size_t>::max();

FavesModel::FavesModel() = default;

FavesModel::~FavesModel() = default;

void FavesModel::clear()
{
  _faves.clear();
}

void FavesModel::addFave(const FavesModel::Fave & fave)
{
  _faves[fave.hash()] = fave;
}

void FavesModel::removeFave(const QString & hash)
{
  _faves.remove(hash);
}

bool FavesModel::contains(const QString & hash) const
{
  return _faves.find(hash) != _faves.cend();
}

void FavesModel::flush() const
{
  qDebug() << "Faves\n=======";
  for (const Fave & fave : _faves) {
    qDebug() << fave.name();
  }
}

size_t FavesModel::faveCount() const
{
  return _faves.size();
}

FavesModel::const_iterator FavesModel::findFaveFromHash(const QString & hash)
{
  return {_faves.find(hash)};
}

const FavesModel::Fave & FavesModel::getFaveFromHash(const QString & hash)
{
  Q_ASSERT_X(_faves.contains(hash), "getFaveFromHash", "Hash not found");
  return _faves.find(hash).value();
}

QString FavesModel::uniqueName(const QString & name, const QString & faveHashToIgnore)
{
  QString basename(name);
  basename.replace(QRegExp(R"~( *\(\d+\)$)~"), QString());
  int iMax = -1;
  bool nameIsUnique = true;
  QMap<QString, Fave>::const_iterator it = _faves.cbegin();
  while (it != _faves.cend()) {
    if (it.key() != faveHashToIgnore) {
      QString faveName = it.value().name();
      if (faveName == name) {
        nameIsUnique = false;
      }
      QRegExp re(R"~( *\((\d+)\)$)~");
      if (re.indexIn(faveName) != -1) {
        faveName.replace(re, QString());
        if (faveName == basename) {
          iMax = std::max(iMax, re.cap(1).toInt());
        }
      } else if ((basename == faveName) && (iMax == -1)) {
        iMax = 1;
      }
    }
    ++it;
  }
  if (nameIsUnique || (iMax == -1)) {
    return name;
  }
  return QString("%1 (%2)").arg(basename).arg(iMax + 1);
}

FavesModel::Fave & FavesModel::Fave::setName(const QString & name)
{
  _name = name;
  _plainText = HtmlTranslator::html2txt(_name, true);
  return *this;
}

FavesModel::Fave & FavesModel::Fave::setOriginalName(const QString & name)
{
  _originalName = name;
  return *this;
}

FavesModel::Fave & FavesModel::Fave::setCommand(const QString & command)
{
  _command = command;
  return *this;
}

FavesModel::Fave & FavesModel::Fave::setPreviewCommand(const QString & command)
{
  _previewCommand = command;
  return *this;
}

FavesModel::Fave & FavesModel::Fave::setOriginalHash(const QString & hash)
{
  _originalHash = hash;
  return *this;
}

FavesModel::Fave & FavesModel::Fave::setDefaultValues(const QList<QString> & defaultValues)
{
  _defaultValues = defaultValues;
  return *this;
}

FavesModel::Fave & FavesModel::Fave::setDefaultVisibilities(const QList<int> & defaultVisibilities)
{
  _defaultVisibilityStates = defaultVisibilities;
}

FavesModel::Fave & FavesModel::Fave::build()
{
  QCryptographicHash hash(QCryptographicHash::Md5);
  hash.addData("FAVE/");
  hash.addData(_name.toLocal8Bit());
  hash.addData(_command.toLocal8Bit());
  hash.addData(_previewCommand.toLocal8Bit());
  _hash = hash.result().toHex();

  QCryptographicHash originalHash(QCryptographicHash::Md5);
  originalHash.addData(_originalName.toLocal8Bit());
  originalHash.addData(_command.toLocal8Bit());
  originalHash.addData(_previewCommand.toLocal8Bit());
  _originalHash = originalHash.result().toHex();
  // TODO : use raw hashes in memory, hex when stored as text
  return *this;
}

const QString & FavesModel::Fave::name() const
{
  return _name;
}

const QString & FavesModel::Fave::plainText() const
{
  return _plainText;
}

const QString & FavesModel::Fave::originalName() const
{
  return _originalName;
}

const QString & FavesModel::Fave::originalHash() const
{
  return _originalHash;
}

const QString & FavesModel::Fave::command() const
{
  return _command;
}

const QString & FavesModel::Fave::previewCommand() const
{
  return _previewCommand;
}

const QString & FavesModel::Fave::hash() const
{
  return _hash;
}

const QList<QString> & FavesModel::Fave::defaultValues() const
{
  return _defaultValues;
}

const QList<int> & FavesModel::Fave::defaultVisibilityStates() const
{
  return _defaultVisibilityStates;
}

QString FavesModel::Fave::toString() const
{
  return QString("(name='%1', command='%2', previewCommand='%3',"
                 " hash='%4', originalHash='%5')")
      .arg(_name)
      .arg(_command)
      .arg(_previewCommand)
      .arg(_hash)
      .arg(_originalHash);
}

bool FavesModel::Fave::matchKeywords(const QList<QString> & keywords) const
{
  static const QString faveFolderPlainText = HtmlTranslator::html2txt(QObject::tr(FAVE_FOLDER_TEXT));
  QList<QString>::const_iterator itKeyword = keywords.cbegin();
  while (itKeyword != keywords.cend()) {
    const QString & keyword = *itKeyword;
    if (!faveFolderPlainText.contains(keyword, Qt::CaseInsensitive) && !_plainText.contains(keyword, Qt::CaseInsensitive)) {
      return false;
    }
    ++itKeyword;
  }
  return true;
}

FavesModel::const_iterator::const_iterator(const QMap<QString, FavesModel::Fave>::const_iterator & iterator)
{
  _mapIterator = iterator;
}

const FavesModel::Fave & FavesModel::const_iterator::operator*() const
{
  return _mapIterator.value();
}

FavesModel::const_iterator & FavesModel::const_iterator::operator++()
{
  ++_mapIterator;
  return *this;
}

FavesModel::const_iterator FavesModel::const_iterator::operator++(int)
{
  FavesModel::const_iterator current = *this;
  ++(*this);
  return current;
}

const FavesModel::Fave * FavesModel::const_iterator::operator->() const
{
  return &(_mapIterator.value());
}

bool FavesModel::const_iterator::operator!=(const FavesModel::const_iterator & other) const
{
  return _mapIterator != other._mapIterator;
}

bool FavesModel::const_iterator::operator==(const FavesModel::const_iterator & other) const
{
  return _mapIterator == other._mapIterator;
}
