/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterSyncRunner.h
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
#ifndef GMIC_QT_FILTERSYNCRUNNER_H
#define GMIC_QT_FILTERSYNCRUNNER_H

#include <QObject>
#include <QString>
#include <QTime>

#include "Common.h"
#include "GmicQt.h"
#include "Host/GmicQtHost.h"
class QObject;

namespace gmic_library
{
template <typename T> struct gmic_list;
}

namespace GmicQt
{

class FilterSyncRunner : public QObject {
  Q_OBJECT
public:
  FilterSyncRunner(QObject * parent, const QString & command, const QString & arguments, const QString & environment);

  virtual ~FilterSyncRunner();
  void setArguments(const QString &);
  void setInputImages(const gmic_library::gmic_list<float> & list);
  void setImageNames(const gmic_library::gmic_list<char> & imageNames);
  void swapImages(gmic_library::gmic_list<float> & images);
  const gmic_library::gmic_list<float> & images() const;
  const gmic_library::gmic_list<char> & imageNames() const;
  gmic_library::gmic_image<char> & persistentMemoryOutput();
  QStringList gmicStatus() const;
  QList<int> parametersVisibilityStates() const;
  QString errorMessage() const;
  bool failed() const;
  bool aborted() const;
  float progress() const;
  QString fullCommand() const;
  void setLogSuffix(const QString & text);
  void run();
  void abortGmic();

private:
  QString _command;
  QString _arguments;
  QString _environment;
  gmic_library::gmic_list<float> * _images;
  gmic_library::gmic_list<char> * _imageNames;
  gmic_library::gmic_image<char> * _persistentMemoryOuptut;
  bool _gmicAbort;
  bool _failed;
  QString _gmicStatus;
  float _gmicProgress;
  QString _errorMessage;
  QString _name;
  QString _logSuffix;
};

} // namespace GmicQt

#endif // GMIC_QT_FILTERSYNCRUNNER_H
