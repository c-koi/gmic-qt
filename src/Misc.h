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

#include <QList>
#include <QString>
#include "gmic_qt.h"
class QStringList;

QString commandFromOutputMessageMode(GmicQt::OutputMessageMode mode);

void appendWithSpace(QString & str, const QString & other);

QString mergedWithSpace(const QString & prefix, const QString & suffix);

void downcaseCommandTitle(QString & title);

bool parseGmicUniqueFilterCommand(const char * text, QString & command_name, QStringList & args);

QString filterFullPathWithoutTags(const QList<QString> & path, const QString & name);

QString filterFullPathBasename(const QString & path);

QString flattenGmicParameterList(const QList<QString> & list);

#endif // GMIC_QT_MISC_H
