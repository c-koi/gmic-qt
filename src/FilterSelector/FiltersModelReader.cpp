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
#include <QBuffer>
#include <QList>
#include <QString>
#include <QLocale>
#include <QFileInfo>
#include <QSettings>
#include <QRegularExpression>
#include <QDebug>
#include "gmic_qt.h"
#include "Common.h"
#include "FilterSelector/FiltersModelReader.h"
#include "FilterSelector/FiltersModel.h"
#include "gmic.h"

FiltersModelReader::FiltersModelReader(FiltersModel & model)
  :_model(model)
{
}

void FiltersModelReader::parseFiltersDefinitions(QByteArray & stdlibArray)
{
  // TODO : Move this
  //  if ( GmicStdlib.isEmpty() ) {
  //    loadStdLib();
  //  }
  QBuffer stdlib(&stdlibArray);
  stdlib.open(QBuffer::ReadOnly);
  QList<QString> filterPath;

  QString language;
  QList<QString> languages = QLocale().uiLanguages();
  if ( languages.size() ) {
    language = languages.front().split("-").front();
  } else {
    language = "void";
  }
  if ( ! stdlibArray.contains(QString("#@gui_%1").arg(language).toLocal8Bit()) ) {
    language = "en";
  }

  // Use _en locale if not localization for the language is found.

  QString buffer = stdlib.readLine(4096);
  QString line;

  QRegExp folderRegexpNoLanguage("^..gui[ ][^:]+$");
  QRegExp folderRegexpLanguage( QString("^..gui_%1[ ][^:]+$").arg(language));

  QRegExp filterRegexpNoLanguage("^..gui[ ][^:]+[ ]*:.*");
  QRegExp filterRegexpLanguage( QString("^..gui_%1[ ][^:]+[ ]*:.*").arg(language));

  const QChar WarningPrefix('!');
  do {
    line = buffer.trimmed();
    if ( line.startsWith("#@gui") ) {
      if ( folderRegexpNoLanguage.exactMatch(line) || folderRegexpLanguage.exactMatch(line) ) {
        //
        // A folder
        //
        QString folderName = line;
        folderName.replace(QRegExp("^..gui[_a-zA-Z]{0,3}[ ]"), "" );

        while ( folderName.startsWith("_") && filterPath.size() ) {
          folderName.remove(0,1);
          filterPath.pop_back();
        }
        while ( folderName.startsWith("_") ) {
          folderName.remove(0,1);
        }
        // TODO : A folder name may be a warning
        if ( ! folderName.isEmpty() ) {
          filterPath.push_back(folderName);
        }
        buffer = stdlib.readLine(4096);
      } else if ( filterRegexpNoLanguage.exactMatch(line) || filterRegexpLanguage.exactMatch(line) ) {
        //
        // A filter
        //
        QString filterName = line;
        filterName.replace( QRegExp("[ ]*:.*$"), "" );
        filterName.replace( QRegExp("^..gui[_a-zA-Z]{0,3}[ ]"), "" );

        const bool warning = filterName.startsWith(WarningPrefix);
        if ( warning ) {
          filterName.remove(0,1);
        }

        QString filterCommands = line;
        filterCommands.replace(QRegExp("^..gui[_a-zA-Z]{0,3}[ ][^:]+[ ]*:[ ]*"),"");

        QList<QString> commands = filterCommands.split(",");

        QString filterCommand = commands[0].trimmed();
        if ( commands.size() == 0) {
          commands.push_back("_none_");
        }
        if ( commands.size() == 1) {
          commands.push_back(commands.front());
        }
        QList<QString> preview = commands[1].trimmed().split("(");
        float previewFactor = GmicQt::PreviewFactorAny;
        bool accurateIfZoomed = true;
        if ( preview.size() >= 2 ) {
          if (preview[1].endsWith("+")) {
            accurateIfZoomed = true;
            preview[1].chop(1);
          } else {
            accurateIfZoomed = false;
          }
          previewFactor = preview[1].replace(QRegExp("\\).*"),"").toFloat();
        }
        QString filterPreviewCommand = preview[0].trimmed();

        //        FiltersTreeFilterItem * filterItem = new FiltersTreeFilterItem(filterName,
        //                                                                       filterCommand,
        //                                                                       filterPreviewCommand,
        //                                                                       previewFactor,
        //                                                                       accurateIfZoomed);
        // filterItem->setWarningFlag(warning);

        QString start = line;
        start.replace(QRegExp(" .*")," :");

        // Read parameters
        QString parameters;
        do {
          buffer = stdlib.readLine(4096);
          if ( buffer.startsWith(start) ) {
            QString parameterLine = buffer;
            parameterLine.replace(QRegExp("^..gui[_a-zA-Z]{0,3}[ ]*:[ ]*"), "");
            parameters += parameterLine;
          }
        } while ( (buffer.startsWith(start)
                   || buffer.startsWith("#")
                   || (buffer.trimmed().isEmpty() && ! stdlib.atEnd()))
                  && !folderRegexpNoLanguage.exactMatch(buffer)
                  && !folderRegexpLanguage.exactMatch(buffer)
                  && !filterRegexpNoLanguage.exactMatch(buffer)
                  && !filterRegexpLanguage.exactMatch(buffer));

        FiltersModel::Filter filter;
        filter.setName(filterName);
        filter.setCommand(filterCommand);
        filter.setPreviewCommand(filterPreviewCommand);
        filter.setPreviewFactor(previewFactor);
        filter.setAccurateIfZoomed(accurateIfZoomed);
        filter.setParameters(parameters);
        filter.setPath(filterPath);
        filter.setWarningFlag(warning);
        filter.build();
        _model.addFilter(filter);

      } else {
        buffer = stdlib.readLine(4096);
      }
    } else {
      buffer = stdlib.readLine(4096);
    }
  } while ( ! buffer.isEmpty() );

}
