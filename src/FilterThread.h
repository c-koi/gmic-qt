/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FilterThread.h
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
#ifndef GMIC_QT__FILTERTHREAD_H
#define GMIC_QT__FILTERTHREAD_H

#include <QElapsedTimer>
#include <QString>
#include <QThread>
#include "Common.h"
#include "GmicQt.h"
#include "Host/GmicQtHost.h"

namespace gmic_library
{
template <typename T> struct gmic_list;
}

namespace GmicQt
{

class FilterThread : public QThread {
  Q_OBJECT

public:
  FilterThread(QObject * parent, const QString & command, const QString & arguments, const QString & environment);

  ~FilterThread() override;
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
  int duration() const;
  float progress() const;
  QString fullCommand() const;
  void setLogSuffix(const QString & text);

  static QStringList status2StringList(QString);
  static QList<int> status2Visibilities(const QString &);

public slots:
  void abortGmic();

signals:
  void done();

protected:
  void run() override;

private:
  QString _command;
  const QString _arguments;
  QString _environment;
  gmic_library::gmic_list<float> * _images;
  gmic_library::gmic_list<char> * _imageNames;
  gmic_library::gmic_image<char> * _persistentMemoryOutput;
  bool _gmicAbort;
  bool _failed;
  QString _gmicStatus;
  float _gmicProgress;
  QString _errorMessage;
  QString _name;
  QString _logSuffix;
  QElapsedTimer _startTime;
};

} // namespace GmicQt

#endif // GMIC_QT__FILTERTHREAD_H
