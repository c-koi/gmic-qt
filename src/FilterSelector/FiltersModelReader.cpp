/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersModelReader.cpp
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
#include "FilterSelector/FiltersModelReader.h"
#include <QBuffer>
#include <QDebug>
#include <QFileInfo>
#include <QList>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <cmath>
#include <cstring>
#include "Common.h"
#include "FilterSelector/FiltersModel.h"
#include "Globals.h"
#include "GmicQt.h"
#include "LanguageSettings.h"
#include "Logger.h"

namespace
{
const QChar CHAR_OPENING_PARENTHESIS('(');
const QChar CHAR_CLOSING_PARENTHESIS(')');
const QChar CHAR_SPACE(' ');
const QChar CHAR_TAB('\t');
const QChar CHAR_COLON(':');
const QChar CHAR_UNDERSCORE('_');
const QChar CHAR_CROSS_SIGN('#');
const QChar CHAR_NEWLINE('\n');
const QString AT_GUI("#@gui");

#ifdef __GNUC__
inline bool isSpace(const QChar & c) __attribute__((always_inline));
inline bool isSpace(const char c) __attribute__((always_inline));
inline void traverseSpaces(const QChar *& pc, const QChar * limit) __attribute__((always_inline));
inline void traverseSpaces(const char *& pc, const char * limit) __attribute__((always_inline));
inline bool traverseOneChar(const QChar *& pc, const QChar * limit, const QChar & c) __attribute__((always_inline));
inline bool traverseOneChar(const char *& pc, const char * limit, const char c) __attribute__((always_inline));
inline bool traverseOneCharDifferentFrom(const QChar *& pc, const QChar * limit, const QChar & c) __attribute__((always_inline));
inline void traverseCharSequenceDifferentFrom(const QChar *& pc, const QChar * limit, const QChar & c) __attribute__((always_inline));
inline bool equals(const QChar *& pc, const QChar * limit, const QString & text) __attribute__((always_inline));
#endif

inline bool isSpace(const QChar & c)
{
  return (c == CHAR_SPACE) || (c == CHAR_TAB);
}

inline bool isSpace(const char c)
{
  return (c == ' ') || (c == '\t');
}

inline void traverseSpaces(const QChar *& pc, const QChar * limit)
{
  while ((pc != limit) && isSpace(*pc)) {
    ++pc;
  }
}

inline void traverseSpaces(const char *& pc, const char * limit)
{
  while ((pc != limit) && isSpace(*pc)) {
    ++pc;
  }
}

inline bool traverseOneChar(const QChar *& pc, const QChar * limit, const QChar & c)
{
  if ((pc != limit) && (*pc == c)) {
    ++pc;
    return true;
  }
  return false;
}

inline bool traverseOneChar(const char *& pc, const char * limit, const char c)
{
  if ((pc != limit) && (*pc == c)) {
    ++pc;
    return true;
  }
  return false;
}

inline bool traverseOneCharDifferentFrom(const QChar *& pc, const QChar * limit, const QChar & c)
{
  if ((pc != limit) && (*pc != c)) {
    ++pc;
    return true;
  }
  return false;
}

inline void traverseCharSequenceDifferentFrom(const QChar *& pc, const QChar * limit, const QChar & c)
{
  while ((pc != limit) && (*pc != c)) {
    ++pc;
  }
}

inline bool traverseOneAlphabeticLetter(const QChar *& pc, const QChar * limit)
{
  if (pc != limit) {
    char c = pc->toLatin1();
    if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))) {
      ++pc;
      return true;
    }
  }
  return false;
}

inline bool equals(const QChar *& pc, const QChar * limit, const QString & text)
{
  const QChar * textPc = text.constData();
  const QChar * textLimit = textPc + text.size();
  while ((pc != limit) && (textPc != textLimit) && (*pc == *textPc)) {
    ++pc;
    ++textPc;
  }
  return (textPc == textLimit);
}

// "^\\s*#@gui"
bool containsGuiComment(const QString & text)
{
  const QChar * pc = text.constData();
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  return equals(pc, limit, AT_GUI);
}

// "^\\s*#@gui[ ][^:]+$"
bool isFolderNoLanguage(const QString & text)
{
  const QChar * pc = text.constData();
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, QString("#@gui "))) {
    return false;
  }
  if (!traverseOneCharDifferentFrom(pc, limit, CHAR_COLON)) {
    return false;
  }
  traverseCharSequenceDifferentFrom(pc, limit, CHAR_COLON);
  return (pc == limit);
}

// QString("^\\s*#@gui_%1[ ][^:]+$").arg(language);
bool isFolderLanguage(const QString & text, const QString & language)
{
  const QChar * pc = text.constData();
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, QString("#@gui_"))) {
    return false;
  }
  if (!equals(pc, limit, language)) {
    return false;
  }
  if (!traverseOneChar(pc, limit, CHAR_SPACE)) {
    return false;
  }
  if (!traverseOneCharDifferentFrom(pc, limit, CHAR_COLON)) {
    return false;
  }
  traverseCharSequenceDifferentFrom(pc, limit, CHAR_COLON);
  return (pc == limit);
}

// "^\\s*#@gui[ ][^:]+[ ]*:.*"
// Replaced here by "^\\s*#@gui[ ][^:]+:.*"
bool isFilterNoLanguage(const QString & text)
{
  const QChar * pc = text.constData();
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, QString("#@gui "))) {
    return false;
  }
  if (!traverseOneCharDifferentFrom(pc, limit, CHAR_COLON)) {
    return false;
  }
  traverseCharSequenceDifferentFrom(pc, limit, CHAR_COLON);
  return traverseOneChar(pc, limit, CHAR_COLON);
}

// QString("^\\s*#@gui_%1[ ][^:]+[ ]*:.*").arg(language);
// Replaced here by "^\\s*#@gui_%1[ ][^:]+:".arg(language);
bool isFilterLanguage(const QString & text, const QString & language)
{
  const QChar * pc = text.constData();
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, QString("#@gui_"))) {
    return false;
  }
  if (!equals(pc, limit, language)) {
    return false;
  }
  if (!traverseOneChar(pc, limit, CHAR_SPACE)) {
    return false;
  }
  if (!traverseOneCharDifferentFrom(pc, limit, CHAR_COLON)) {
    return false;
  }
  traverseCharSequenceDifferentFrom(pc, limit, CHAR_COLON);
  return traverseOneChar(pc, limit, CHAR_COLON);
}

// "\\s*:\\s*([xX.*+vViI-])\\s*$"
bool containsInputMode(const QString & text, QString & inputMode)
{
  const QChar * pc = text.constData();
  const QChar * limit = pc + text.size();
  traverseCharSequenceDifferentFrom(pc, limit, CHAR_COLON);
  if (!traverseOneChar(pc, limit, CHAR_COLON)) {
    return false;
  }
  traverseSpaces(pc, limit);
  if (pc != limit) {
    char c = pc->toLatin1();
    if (strchr("xX.*+vViI-", c)) {
      inputMode = *pc;
      return true;
    }
  }
  return false;
}

// QString("^\\s*#@gui_%1[ ]+hide\\((.*)\\)").arg(language)); // Capture 'path'
bool containsHidePath(const QString & text, const QString & language, QString & path)
{
  const QChar * begin = text.constData();
  const QChar * pc = begin;
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, AT_GUI)) {
    return false;
  }
  if (!traverseOneChar(pc, limit, CHAR_UNDERSCORE)) {
    return false;
  }
  if (!equals(pc, limit, language)) {
    return false;
  }
  if (!traverseOneChar(pc, limit, CHAR_SPACE)) {
    return false;
  }
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, QString("hide("))) {
    return false;
  }
  const QChar * captureBegin = pc;
  traverseCharSequenceDifferentFrom(pc, limit, CHAR_CLOSING_PARENTHESIS);
  if ((pc == limit) || (*pc != CHAR_CLOSING_PARENTHESIS)) {
    return false;
  }
  const QChar * captureEnd = pc;
  path = QString(captureBegin, captureEnd - captureBegin);
  return true;
}

// "^\\s*#"
bool containsLeadingSpaceAndCrossSign(const char * text, const char * limit)
{
  traverseSpaces(text, limit);
  return traverseOneChar(text, limit, '#');
}

// Remove "\\s*:.*$"
void removeColonAndText(QString & text)
{
  int i = text.indexOf(':');
  while ((i > 0) && isSpace(text[i - 1])) {
    --i;
  }
  text.remove(i, text.size() - i);
}

// Remove "^\\s*#@gui[_a-zA-Z]{0,3}[ ]"
void removeAtGuiLangPrefix(QString & text)
{
  const QChar * begin = text.constData();
  const QChar * pc = begin;
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, AT_GUI)) {
    return;
  }
  if (traverseOneChar(pc, limit, CHAR_UNDERSCORE)) {
    traverseOneAlphabeticLetter(pc, limit);
    traverseOneAlphabeticLetter(pc, limit);
  }
  if (!traverseOneChar(pc, limit, CHAR_SPACE)) {
    return;
  }
  text.remove(0, pc - begin);
}

// "^\\s*#@gui[_a-zA-Z]{0,3}[ ][^:]+:[ ]*"
void removeAtGuiTextAndColon(QString & text)
{
  const QChar * begin = text.constData();
  const QChar * pc = begin;
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, AT_GUI)) {
    return;
  }
  if (traverseOneChar(pc, limit, CHAR_UNDERSCORE)) {
    traverseOneAlphabeticLetter(pc, limit);
    traverseOneAlphabeticLetter(pc, limit);
  }
  if (!traverseOneChar(pc, limit, CHAR_SPACE)) {
    return;
  }
  if (!traverseOneCharDifferentFrom(pc, limit, CHAR_COLON)) {
    return;
  }
  traverseCharSequenceDifferentFrom(pc, limit, CHAR_COLON);
  if (!traverseOneChar(pc, limit, CHAR_COLON)) {
    return;
  }
  traverseSpaces(pc, limit);
  text.remove(0, pc - begin);
}

// "^\\s*#@gui[_a-zA-Z]{0,3}[ ]*:[ ]*"
void removeAtGuiSpacesAndColon(QString & text)
{
  const QChar * begin = text.constData();
  const QChar * pc = begin;
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, AT_GUI)) {
    return;
  }
  if (traverseOneChar(pc, limit, CHAR_UNDERSCORE)) {
    traverseOneAlphabeticLetter(pc, limit);
    traverseOneAlphabeticLetter(pc, limit);
  }
  traverseSpaces(pc, limit);
  if (!traverseOneChar(pc, limit, CHAR_COLON)) {
    return;
  }
  traverseSpaces(pc, limit);
  text.remove(0, pc - begin);
}

// "^\\s*"
void removeLeadingSpaces(QString & text)
{
  const QChar * begin = text.constData();
  const QChar * pc = begin;
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (pc != begin) {
    text.remove(0, pc - begin);
  }
}

// " .*"
void removeSpaceAndText(QString & text)
{
  int index = text.indexOf(CHAR_SPACE);
  if (index != -1) {
    text.remove(index, text.size() - index);
  }
}

// "\\s*:\\s*([xX.*+vViI-])\\s*$"  // Capture
void removeInputMode(QString & text)
{
  int index = text.indexOf(CHAR_COLON);
  if (index != -1) {
    while ((index > 0) && (text[index - 1] == CHAR_SPACE)) {
      --index;
    }
    text.remove(index, text.size() - index);
  }
}

// Replace QRegExp(" .*") by "[ ]?:" to obtain #@gui[ ]?: or #@gui_fr[ ]?:
// then check the regexp "^\s*#@gui[ ]?:" or "^\s*#@gui_fr[ ]?:"
// Check \s*PREFIX[ ]?:    (PREFIX is e.g. #@gui or #@gui_fr)
bool isPrefixAndColon(const QString & text, const QString & prefix)
{
  const QChar * begin = text.constData();
  const QChar * pc = begin;
  const QChar * limit = pc + text.size();
  traverseSpaces(pc, limit);
  if (!equals(pc, limit, prefix)) {
    return false;
  }
  traverseOneChar(pc, limit, CHAR_SPACE);
  return traverseOneChar(pc, limit, CHAR_COLON);
}

} // namespace

namespace GmicQt
{

FiltersModelReader::FiltersModelReader(FiltersModel & model) : _model(model) {}

void FiltersModelReader::parseFiltersDefinitions(const QByteArray & stdlibArray)
{
  TIMING;
  const char * stdlib = stdlibArray.constData();
  const char * stdLibLimit = stdlib + stdlibArray.size();
  QList<QString> filterPath;

  QString language = LanguageSettings::configuredTranslator();
  if (language.isEmpty()) {
    language = "void";
  }

  // Use _en locale if no localization for the language is found.
  QByteArray localePrefix = QString("#@gui_%1").arg(language).toLocal8Bit();
  if (!textIsPrecededBySpacesInSomeLineOfArray(localePrefix, stdlibArray)) {
    language = "en";
  }

  QString buffer = readBufferLine(stdlib, stdLibLimit);
  QString line;
  QVector<QString> hiddenPaths;

  const QChar WarningPrefix('!');
  do {
    line = buffer.trimmed();
    if (containsGuiComment(line)) {
      QString path;
      if (containsHidePath(line, language, path)) {
        hiddenPaths.push_back(path);
        buffer = readBufferLine(stdlib, stdLibLimit);
      } else if (isFolderNoLanguage(line) || isFolderLanguage(line, language)) {
        //
        // A folder
        //
        QString folderName = line;
        removeAtGuiLangPrefix(folderName);

        while (folderName.startsWith("_") && !filterPath.isEmpty()) {
          folderName.remove(0, 1);
          filterPath.pop_back();
        }
        while (folderName.startsWith("_")) {
          folderName.remove(0, 1);
        }
        if (!folderName.isEmpty()) {
          filterPath.push_back(folderName);
        }
        buffer = readBufferLine(stdlib, stdLibLimit);
      } else if (isFilterNoLanguage(line) || isFilterLanguage(line, language)) {
        //
        // A filter
        //
        QString filterName = line;
        removeColonAndText(filterName);
        removeAtGuiLangPrefix(filterName);
        const bool warning = filterName.startsWith(WarningPrefix);
        if (warning) {
          filterName.remove(0, 1);
        }

        QString filterCommands = line;
        removeAtGuiTextAndColon(filterCommands);

        // Extract default input mode
        InputMode defaultInputMode = InputMode::Unspecified;
        QString inputMode;
        if (containsInputMode(filterCommands, inputMode)) {
          removeInputMode(filterCommands);
          defaultInputMode = symbolToInputMode(inputMode);
        }

        QList<QString> commands = filterCommands.split(",");
        QString filterCommand = commands[0].trimmed();
        if (commands.isEmpty()) {
          commands.push_back("_none_");
        }
        if (commands.size() == 1) {
          commands.push_back(commands.front());
        }
        QList<QString> preview = commands[1].trimmed().split("(");
        float previewFactor = PreviewFactorAny;
        bool accurateIfZoomed = true;
        bool previewFromFullImage = false;
        if (preview.size() >= 2) {
          if (preview[1].endsWith("+")) {
            accurateIfZoomed = true;
            preview[1].chop(1);
          } else if (preview[1].endsWith("*")) {
            accurateIfZoomed = true;
            previewFromFullImage = true;
            preview[1].chop(1);
          } else {
            accurateIfZoomed = false;
          }
          bool ok = false;
          const int closingParenthesisIndex = preview[1].indexOf(QChar(')'));
          if (closingParenthesisIndex != -1) {
            preview[1].remove(closingParenthesisIndex, preview[1].size() - closingParenthesisIndex);
          }
          previewFactor = preview[1].toFloat(&ok);
          if (!ok) {
            Logger::error(QString("Cannot parse zoom factor for filter [%1]:\n%2").arg(filterName).arg(line));
            previewFactor = PreviewFactorAny;
          }
          previewFactor = std::abs(previewFactor);
        }

        QString filterPreviewCommand = preview[0].trimmed();
        QString start = line;
        removeLeadingSpaces(start);
        removeSpaceAndText(start); // #@gui or #@gui_fr

        // Read parameters
        QString parameters;
        do {
          buffer = readBufferLine(stdlib, stdLibLimit);
          if (isPrefixAndColon(buffer, start)) { //
            QString parameterLine = buffer;
            removeAtGuiSpacesAndColon(parameterLine);
            parameters += parameterLine;
          }
        } while ((stdlib != stdLibLimit)                //
                 && !isFolderNoLanguage(buffer)         //
                 && !isFolderLanguage(buffer, language) //
                 && !isFilterNoLanguage(buffer)         //
                 && !isFilterLanguage(buffer, language));
        FiltersModel::Filter filter;
        filter.setName(filterName);
        filter.setCommand(filterCommand);
        filter.setPreviewCommand(filterPreviewCommand);
        filter.setDefaultInputMode(defaultInputMode);
        filter.setPreviewFactor(previewFactor);
        filter.setAccurateIfZoomed(accurateIfZoomed);
        filter.setPreviewFromFullImage(previewFromFullImage);
        filter.setParameters(parameters);
        filter.setPath(filterPath);
        filter.setWarningFlag(warning);
        filter.build();
        _model.addFilter(filter);
      } else {
        buffer = readBufferLine(stdlib, stdLibLimit);
      }
    } else {
      buffer = readBufferLine(stdlib, stdLibLimit);
    }
  } while (!buffer.isEmpty());

  // Remove hidden filters from the model
  for (const QString & path : hiddenPaths) {
    const size_t count = _model.filterCount();
    QList<QString> pathList = path.split("/", QT_SKIP_EMPTY_PARTS);
    _model.removePath(pathList);
    if (_model.filterCount() == count) {
      Logger::warning(QString("While hiding filter, name or path not found: \"%1\"").arg(path));
    }
  }
  TIMING;
}

bool FiltersModelReader::textIsPrecededBySpacesInSomeLineOfArray(const QByteArray & text, const QByteArray & array)
{
  if (text.isEmpty()) {
    return false;
  }
  int from = 0;
  int position;
  const char * data = array.constData();
  while ((position = array.indexOf(text, from)) != -1) {
    int index = position - 1;
    while ((index >= 0) && (data[index] != '\n') && (data[index] <= ' ')) {
      --index;
    }
    if ((index < 0) || (data[index] == '\n')) {
      return true;
    }
    from = position + 1;
  }
  return false;
}

InputMode FiltersModelReader::symbolToInputMode(const QString & str)
{
  if (str.length() != 1) {
    Logger::warning(QString("'%1' is not recognized as a default input mode (should be a single symbol/letter)").arg(str));
    return InputMode::Unspecified;
  }
  switch (str.toLocal8Bit()[0]) {
  case 'x':
  case 'X':
    return InputMode::NoInput;
  case '.':
    return InputMode::Active;
  case '*':
    return InputMode::All;
  case '-':
    return InputMode::ActiveAndAbove;
  case '+':
    return InputMode::ActiveAndBelow;
  case 'V':
  case 'v':
    return InputMode::AllVisible;
  case 'I':
  case 'i':
    return InputMode::AllInvisible;
  default:
    Logger::warning(QString("'%1' is not recognized as a default input mode").arg(str));
    return InputMode::Unspecified;
  }
}

// QString FiltersModelReader::readBufferLine(const char * & pc, const char * limit)

QString FiltersModelReader::readBufferLine(const char *& ptr, const char * limit)
{
  if (ptr == limit) {
    return QString();
  }
  QString line;
  const char * eol = strchr(ptr, '\n');
  const char * start = ptr;
  ptr = eol ? (eol + 1) : limit;
  const int lineSize = int(ptr - start);
  line = QString::fromUtf8(start, lineSize);

  if (containsLeadingSpaceAndCrossSign(start, start + lineSize)) {
    while (line.endsWith("\\\n")) {
      line.chop(2);
      if (!containsLeadingSpaceAndCrossSign(ptr, limit)) {
        line.append(CHAR_NEWLINE);
        break;
      }
      while (isSpace(*ptr)) { // Skip spaces
        ++ptr;
      }
      ++ptr; // Skip '#'
      eol = strchr(ptr, '\n');
      start = ptr;
      ptr = eol ? (eol + 1) : limit;
      line.append(QString::fromUtf8(start, int(ptr - start)));
    }
  }
  return line;
}

} // namespace GmicQt
