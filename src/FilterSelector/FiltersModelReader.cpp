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
#include "Common.h"
#include "FilterSelector/FiltersModel.h"
#include "Globals.h"
#include "GmicQt.h"
#include "LanguageSettings.h"
#include "Logger.h"

namespace GmicQt
{

FiltersModelReader::FiltersModelReader(FiltersModel & model) : _model(model) {}

void FiltersModelReader::parseFiltersDefinitions(QByteArray & stdlibArray)
{
  TIMING;
  QBuffer stdlib(&stdlibArray);
  stdlib.open(QBuffer::ReadOnly | QBuffer::Text);
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

  QString buffer = readBufferLine(stdlib);
  QString line;

  QRegularExpression folderRegexpNoLanguage("^\\s*#@gui[ ][^:]+$");
  QRegularExpression folderRegexpLanguage(QString("^\\s*#@gui_%1[ ][^:]+$").arg(language));

  QRegularExpression filterRegexpNoLanguage("^\\s*#@gui[ ][^:]+[ ]*:.*");
  QRegularExpression filterRegexpLanguage(QString("^\\s*#@gui_%1[ ][^:]+[ ]*:.*").arg(language));

  QRegularExpression hideCommandRegExp(QString("^\\s*#@gui_%1[ ]+hide\\((.*)\\)").arg(language));
  QRegularExpression guiComment("^\\s*#@gui");
  QRegularExpression atGuiLangPrefix("^\\s*#@gui[_a-zA-Z]{0,3}[ ]");
  QRegularExpression colonAndText("\\s*:.*$");
  QRegularExpression atGuiToColon("^\\s*#@gui[_a-zA-Z]{0,3}[ ][^:]+[ ]*:[ ]*");
  QRegularExpression reInputMode("\\s*:\\s*([xX.*+vViI-])\\s*$");
  QRegularExpression closingParenthesisAndText("\\).*");
  QRegularExpression atGuiColonSpaces("^\\s*#@gui[_a-zA-Z]{0,3}[ ]*:[ ]*");
  QRegularExpression leadingSpaces("^\\s*");
  QRegularExpression spaceAndText(" .*");
  QVector<QString> hiddenPaths;

  const QChar WarningPrefix('!');
  do {
    line = buffer.trimmed();
    if (line.contains(guiComment)) {
      QRegularExpressionMatch match = hideCommandRegExp.match(line);
      if (match.hasMatch()) {
        QString path = match.captured(1);
        hiddenPaths.push_back(path);
        buffer = readBufferLine(stdlib);
      } else if (folderRegexpNoLanguage.match(line).hasMatch() || folderRegexpLanguage.match(line).hasMatch()) {
        //
        // A folder
        //
        QString folderName = line;
        folderName.remove(atGuiLangPrefix);

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
        buffer = readBufferLine(stdlib);
      } else if (filterRegexpNoLanguage.match(line).hasMatch() || filterRegexpLanguage.match(line).hasMatch()) {
        //
        // A filter
        //
        QString filterName = line;
        filterName.remove(colonAndText);
        filterName.remove(atGuiLangPrefix);
        const bool warning = filterName.startsWith(WarningPrefix);
        if (warning) {
          filterName.remove(0, 1);
        }

        QString filterCommands = line;
        filterCommands.remove(atGuiToColon);

        // Extract default input mode
        InputMode defaultInputMode = InputMode::Unspecified;
        match = reInputMode.match(filterCommands);
        if (match.hasMatch()) {
          QString mode = match.captured(1);
          filterCommands.remove(reInputMode);
          defaultInputMode = symbolToInputMode(mode);
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
          preview[1].remove(closingParenthesisAndText);
          previewFactor = preview[1].toFloat(&ok);
          if (!ok) {
            Logger::error(QString("Cannot parse zoom factor for filter [%1]:\n%2").arg(filterName).arg(line));
            previewFactor = PreviewFactorAny;
          }
          previewFactor = std::abs(previewFactor);
        }

        QString filterPreviewCommand = preview[0].trimmed();
        QString start = line;
        start.remove(leadingSpaces);
        start.replace(spaceAndText, "[ ]?:");
        QRegularExpression startRegexp(QString("^\\s*%1").arg(start));

        // Read parameters
        QString parameters;
        do {
          buffer = readBufferLine(stdlib);
          if (startRegexp.match(buffer).hasMatch()) {
            QString parameterLine = buffer;
            parameterLine.remove(atGuiColonSpaces);
            parameters += parameterLine;
          }
        } while (!stdlib.atEnd()                                     //
                 && !folderRegexpNoLanguage.match(buffer).hasMatch() //
                 && !folderRegexpLanguage.match(buffer).hasMatch()   //
                 && !filterRegexpNoLanguage.match(buffer).hasMatch() //
                 && !filterRegexpLanguage.match(buffer).hasMatch());
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
        buffer = readBufferLine(stdlib);
      }
    } else {
      buffer = readBufferLine(stdlib);
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

QString FiltersModelReader::readBufferLine(QBuffer & buffer)
{
  // QBuffer::readline(max_size) may be very slow, in debug mode, when max_size
  // is too big (e.g. 1MB). We read large lines in multiple calls.
  QString result;
  QString text;
  QRegularExpression commentStart("^\\s*#");
  do {
    text = buffer.readLine(1024);
    result.append(text);
  } while (!text.isEmpty() && !text.endsWith("\n"));

  // Merge comment lines ending with '\'
  if (commentStart.match(result).hasMatch()) {
    while (result.endsWith("\\\n")) {
      QString nextLinePeek = buffer.peek(1024);
      QRegularExpressionMatch match = commentStart.match(nextLinePeek);
      if (!match.hasMatch()) {
        return result;
      }
      const QString nextCommentPrefix = match.captured(0);
      result.chop(2);
      QString nextLine;
      do {
        text = buffer.readLine(1024);
        nextLine.append(text);
      } while (!text.isEmpty() && !text.endsWith("\n"));
      int ignoreCount = nextCommentPrefix.length();
      const int limit = nextLine.length() - nextLine.endsWith("\n");
      while (ignoreCount < limit && nextLine[ignoreCount] <= ' ') {
        ++ignoreCount;
      }
      result.append(nextLine.right(nextLine.length() - ignoreCount));
    }
  }
  return result;
}

} // namespace GmicQt
