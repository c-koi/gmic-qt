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
#include <QByteArray>
#include <QChar>
#include <QDebug>
#include <QMap>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <algorithm>
#include <cctype>
#include "Common.h"
#include "Globals.h"
#include "HtmlTranslator.h"
#include "Logger.h"
#include "CImg.h"
#include "gmic.h"

namespace
{
inline void skipSpaces(const char *& pc)
{
  while (isspace(*pc)) {
    ++pc;
  }
}
inline bool isEmptyOrSpaceSequence(const char * pc)
{
  while (*pc) {
    if (!isspace(*pc)) {
      return false;
    }
    ++pc;
  }
  return true;
}
} // namespace

namespace GmicQt
{

QString commandFromOutputMessageMode(OutputMessageMode mode)
{
  switch (mode) {
  case OutputMessageMode::Quiet:
  case OutputMessageMode::VerboseConsole:
  case OutputMessageMode::VerboseLogFile:
  case OutputMessageMode::Unspecified:
    return "";
  case OutputMessageMode::VeryVerboseConsole:
  case OutputMessageMode::VeryVerboseLogFile:
    return "v 3";
  case OutputMessageMode::DebugConsole:
  case OutputMessageMode::DebugLogFile:
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

bool parseGmicFilterParameters(const QString & text, QStringList & args)
{
  return parseGmicFilterParameters(text.toUtf8().constData(), args);
}

bool parseGmicFilterParameters(const char * text, QStringList & args)
{
  args.clear();
  if (!text) {
    return false;
  }
  skipSpaces(text);
  const char * pc = text;
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
      args.push_back(QString::fromUtf8(buffer));
      output = buffer;
      ++pc;
    } else {
      *output++ = *pc++;
    }
  }
  const bool endsWidthComma = (output == buffer) && (pc > text) && (!quoted && !escaped && (pc[-1] == ','));
  if ((output != buffer) || endsWidthComma) {
    *output = '\0';
    args.push_back(QString::fromUtf8(buffer));
  }
  delete[] buffer;
  if (quoted || (meaningfulSpaceFound && !isEmptyOrSpaceSequence(pc))) {
    args.clear();
    return false;
  }
  return true;
}

bool parseGmicUniqueFilterCommand(const char * text, QString & command, QString & arguments)
{
  arguments.clear();
  command.clear();
  if (!text) {
    return false;
  }
  const char * commandBegin = text;
  skipSpaces(commandBegin);
  if (*commandBegin == '\0') {
    return false;
  }
  const char * pc = commandBegin;
  while (isalnum(*pc) || (*pc == '_')) {
    ++pc;
  }
  if ((*pc != '\0') && !isspace(*pc)) {
    return false;
  }
  const char * const commandEnd = pc;
  skipSpaces(pc);

  bool quoted = false;
  bool escaped = false;
  bool meaningfulSpaceFound = false;
  const char * argumentStart = pc;
  while (*pc && !((meaningfulSpaceFound = (!quoted && !escaped && isspace(*pc))))) {
    if (escaped) {
      escaped = false;
    } else if (*pc == '\\') {
      escaped = true;
    } else if (*pc == '"') {
      quoted = !quoted;
    }
    ++pc;
  }
  if (quoted || (meaningfulSpaceFound && !isEmptyOrSpaceSequence(pc))) {
    return false;
  }
  command = QString::fromUtf8(commandBegin, static_cast<int>(commandEnd - commandBegin));
  arguments = QString::fromUtf8(argumentStart, static_cast<int>(pc - argumentStart));
  return true;
}

QString escapeUnescapedQuotes(const QString & text)
{
  std::string source_str = text.toStdString();
  const char * pc = source_str.c_str();
  std::vector<char> output_str(2 * source_str.size() + 1, static_cast<char>(0));
  char * out = output_str.data();
  bool escaped = false;
  while (*pc) {
    if (escaped) {
      escaped = false;
    } else if (*pc == '\\') {
      escaped = true;
    } else if (*pc == '"') {
      *out++ = '\\';
    }
    *out++ = *pc++;
  }
  QString result = QString::fromUtf8(output_str.data());
  return result;
}

QString filterFullPathWithoutTags(const QList<QString> & path, const QString & name)
{
  QStringList noTags = {QString()};
  for (QString str : path) {
    if (str.startsWith(WarningPrefix)) {
      str.remove(0, 1);
    }
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

QString flattenGmicParameterList(const QList<QString> & list, const QVector<bool> & quotedParameters)
{
  QString result;
  if (list.isEmpty()) {
    return result;
  }
  QList<QString>::const_iterator itList = list.constBegin();
  QVector<bool>::const_iterator itQuoting = quotedParameters.constBegin();
  result += (*itQuoting++) ? quotedString(*itList++) : (*itList++);
  while (itList != list.end()) {
    result += QString(",%1").arg((*itQuoting++) ? quotedString(*itList++) : (*itList++));
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

QString elided(const QString & text, int width)
{
  if (text.length() <= width) {
    return text;
  }
  return text.left(std::max(0, width - 3)) + "...";
}

QVector<bool> quotedParameters(const QList<QString> & parameters)
{
  QVector<bool> result;
  for (const auto & str : parameters) {
    result.push_back(str.startsWith("\""));
  }
  return result;
}

QStringList mergeSubsequences(const QStringList & sequence, const QVector<int> & subSequenceLengths)
{
  QStringList result;
  QVector<int> lengths = subSequenceLengths;
  QStringList::const_iterator itInput = sequence.constBegin();
  QVector<int>::iterator itLength = lengths.begin();
  while ((itInput != sequence.constEnd()) && (itLength != lengths.end())) {
    if (*itLength <= 0) {
      ++itLength;
      continue;
    }
    QString text = *itInput++;
    --(*itLength);
    while (*itLength > 0) {
      text += QString(",%1").arg(*itInput++);
      --(*itLength);
    }
    result.push_back(text);
    ++itLength;
  }
  if ((itInput != sequence.constEnd()) || (itLength != lengths.end())) {
    Logger::warning(QObject::tr("List %1 cannot be merged considering these runs: %2").arg(stringify(sequence)).arg(stringify(subSequenceLengths)));
    return QStringList();
  }
  return result;
}

QStringList completePrefixFromFullList(const QStringList & prefix, const QStringList & fullList)
{
  if (fullList.size() <= prefix.size()) {
    return prefix;
  }
  QStringList result = prefix;
  QStringList::const_iterator it = fullList.constBegin();
  it += prefix.size();
  while (it != fullList.constEnd()) {
    result.push_back(*it++);
  }
  return result;
}

QString quotedString(QString text)
{
  return QString("\"%1\"").arg(escapeUnescapedQuotes(text));
}

QStringList quotedStringList(const QStringList & stringList)
{
  QStringList result;
  for (const auto & text : stringList) {
    result.push_back(quotedString(text));
  }
  return result;
}

QString unescaped(const QString & text)
{
  QByteArray ba = text.toUtf8();
  cimg_library::cimg::strunescape(ba.data());
  return QString::fromUtf8(ba.data());
}

QString unquoted(const QString & text)
{
  QRegularExpression re("^\\s*\"(.*)\"\\s*$");
  auto match = re.match(text);
  if (match.hasMatch()) {
    return match.captured(1);
  } else {
    return text.trimmed();
  }
}

QStringList expandParameterList(const QStringList & parameters, const QVector<int> & sizes)
{
  // FIXME : Handle errors here
  QStringList result;
  Q_ASSERT_X(parameters.size() == sizes.size(), __PRETTY_FUNCTION__, QString("Sizes are different (parameters: %1, sizes: %2)").arg(parameters.size()).arg(sizes.size()).toStdString().c_str());
  QStringList::const_iterator itParam = parameters.constBegin();
  auto itSize = sizes.constBegin();
  while (itParam != parameters.constEnd() && itSize != sizes.constEnd()) {
    if (*itSize > 1) {
      result.append(itParam->split(","));
    } else if (*itSize == 1) {
      result.push_back(*itParam);
    } else {
      Q_ASSERT_X((*itSize >= 1), __PRETTY_FUNCTION__, QString("Parameter size should be at least 1 (it is %1)").arg(*itSize).toStdString().c_str());
    }
    ++itParam;
    ++itSize;
  }
  return result;
}

} // namespace GmicQt
