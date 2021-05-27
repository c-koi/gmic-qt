/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Utils.h
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
#ifndef GMIC_QT_MISC_H
#define GMIC_QT_MISC_H

#include <QDebug>
#include <QList>
#include <QString>
#include <QStringList>
#include "gmic_qt.h"
class QStringList;

namespace GmicQt
{

QString commandFromOutputMessageMode(OutputMessageMode mode);

void appendWithSpace(QString & str, const QString & other);

QString mergedWithSpace(const QString & prefix, const QString & suffix);

void downcaseCommandTitle(QString & title);

bool parseGmicFilterParameters(const char * text, QStringList & args);

bool parseGmicFilterParameters(const QString & text, QStringList & args);

bool parseGmicUniqueFilterCommand(const char * text, QString & command, QString & arguments);

QString escapeUnescapedQuotes(const QString & text);

QString filterFullPathWithoutTags(const QList<QString> & path, const QString & name);

QString filterFullPathBasename(const QString & path);

QString flattenGmicParameterList(const QList<QString> & list, const QVector<bool> & quotedParameters);

QStringList expandParameterList(const QStringList & parameters, QVector<int> sizes);

QString elided(const QString & text, int width);

QVector<bool> quotedParameters(const QList<QString> & parameters);

QString quotedString(QString text);

QStringList quotedStringList(const QStringList & stringList);

QString unescaped(const QString & text);

QStringList mergeSubsequences(const QStringList & sequence, const QVector<int> & subSequenceLengths);

QStringList completePrefixFromFullList(const QStringList & prefix, const QStringList & fullList);

QString unquoted(const QString & text);

inline QString elided80(const std::string & text)
{
  return elided(QString::fromStdString(text), 80);
}

template <typename T> //
QString stringify(const T & value)
{
  QString result;
  QDebug(&result) << value;
  return result;
}

template <typename T> //
inline void setValueIfNotNullPointer(T * pointer, const T & value)
{
  if (pointer) {
    *pointer = value;
  }
}

} // namespace GmicQt

#endif // GMIC_QT_MISC_H
