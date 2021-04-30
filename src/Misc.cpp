/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file Misc.cpp
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

#include "Misc.h"
#include <QMap>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <cctype>
#include "HtmlTranslator.h"

namespace
{
inline void skipSpaces(const char *& pc)
{
  while (isspace(*pc)) {
    ++pc;
  }
}
} // namespace

QString commandFromOutputMessageMode(GmicQt::OutputMessageMode mode)
{
  switch (mode) {
  case GmicQt::Quiet:
  case GmicQt::VerboseConsole:
  case GmicQt::VerboseLogFile:
  case GmicQt::UnspecifiedOutputMessageMode:
    return "";
  case GmicQt::VeryVerboseConsole:
  case GmicQt::VeryVerboseLogFile:
    return "v 3";
  case GmicQt::DebugConsole:
  case GmicQt::DebugLogFile:
    return "debug";
  }
  return "";
}

void appendWithSpace(QString & str, const QString & other)
{
  if (str.isEmpty() || other.isEmpty()) {
    str += other;
    return;
  }
  str += QChar(' ');
  str += other;
}

void downcaseCommandTitle(QString & title)
{
  QMap<int, QString> acronyms;
  // Acronyms
  QRegExp re("([A-Z0-9]{2,255})");
  int index = 0;
  while ((index = re.indexIn(title, index)) != -1) {
    QString pattern = re.cap(0);
    acronyms[index] = pattern;
    index += pattern.length();
  }

  // 3D
  re.setPattern("([1-9])[dD] ");
  if ((index = re.indexIn(title, 0)) != -1) {
    acronyms[index] = re.cap(1) + "d ";
  }

  // B&amp;W
  re.setPattern("(B&amp;W|[ \\[]Lab|[ \\[]YCbCr)");
  index = 0;
  while ((index = re.indexIn(title, index)) != -1) {
    acronyms[index] = re.cap(1);
    index += re.cap(1).length();
  }

  // Uppercase letter in last position, after a space
  re.setPattern(" ([A-Z])$");
  if ((index = re.indexIn(title, 0)) != -1) {
    acronyms[index] = re.cap(0);
  }
  title = title.toLower();
  QMap<int, QString>::const_iterator it = acronyms.cbegin();
  while (it != acronyms.cend()) {
    title.replace(it.key(), it.value().length(), it.value());
    ++it;
  }
  title[0] = title[0].toUpper();
}

bool parseGmicUniqueFilterCommand(const char * text, QString & command_name, QStringList & args)
{
  args.clear();
  command_name.clear();
  if (!text) {
    return false;
  }
  skipSpaces(text);
  if (*text == '\0') {
    return false;
  }
  const char * pc = text;
  while (isalnum(*pc) || (*pc == '_')) {
    ++pc;
  }
  if ((*pc != '\0') && !isspace(*pc)) {
    return false;
  }
  command_name = QString::fromLatin1(text, static_cast<int>(pc - text));
  skipSpaces(pc);

  bool quoted = false;
  bool escaped = false;
  bool meaningfulSpaceFound = false;
  char * buffer = new char[strlen(pc)]();
  char * output = buffer;
  while (*pc && !((meaningfulSpaceFound = (!quoted && !escaped && isspace(*pc))))) {
    if (escaped) {
      *output++ = *pc++;
      escaped = false;
    } else if (*pc == '\\') {
      escaped = true;
      ++pc;
    } else if (*pc == '"') {
      quoted = !quoted;
      ++pc;
    } else if (!quoted && !escaped && (*pc == ',')) {
      *output = '\0';
      args.push_back(QString::fromLatin1(buffer));
      output = buffer;
      ++pc;
    } else {
      *output++ = *pc++;
    }
  }
  if (output != buffer) {
    *output = '\0';
    args.push_back(QString::fromLatin1(buffer));
  }
  delete[] buffer;
  if (quoted || meaningfulSpaceFound) {
    command_name.clear();
    args.clear();
    return false;
  }
  return true;
}

QString filterFullPathWithoutTags(const QList<QString> & path, const QString & name)
{
  QStringList noTags;
  for (const QString & str : path) {
    noTags.push_back(HtmlTranslator::removeTags(str));
  }
  noTags.push_back(HtmlTranslator::removeTags(name));
  return noTags.join('/');
}

QString filterFullPathBasename(const QString & path)
{
  QString result = path;
  result.remove(QRegularExpression("^.*/"));
  return result;
}

// FIXME : test status with "

QString flattenGmicParameterList(const QList<QString> & list)
{
  QString result;
  if (list.isEmpty()) {
    return result;
  }
  QList<QString>::const_iterator itList = list.begin();
  result += QString("\"%1\"").arg(*itList++).replace(QChar('"'), QString("\\\""));
  while (itList != list.end()) {
    result += QString(",\"%1\"").arg(*itList++).replace(QChar('"'), QString("\\\""));
  }
  return result;
}

QString mergedWithSpace(const QString & prefix, const QString & suffix)
{
  if (prefix.isEmpty() || suffix.isEmpty()) {
    return prefix + suffix;
  }
  return prefix + QChar(' ') + suffix;
}
