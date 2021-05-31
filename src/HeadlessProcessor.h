/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file HeadlessProcessor.h
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
#ifndef GMIC_QT_HEADLESSPROCESSOR_H
#define GMIC_QT_HEADLESSPROCESSOR_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>
#include "GmicQt.h"

namespace cimg_library
{
template <typename T> struct CImgList;
}

namespace GmicQt
{
class FilterThread;
class ProgressInfoWindow;

class HeadlessProcessor : public QObject {
  Q_OBJECT

public:
  explicit HeadlessProcessor(QObject * parent);
  ~HeadlessProcessor() override;
  QString command() const;
  QString filterName() const;
  void setProgressWindow(ProgressInfoWindow *);
  bool processingCompletedProperly();
  bool setPluginParameters(const RunParameters & parameters);
  const QString & error() const;
public slots:
  void startProcessing();
  void sendProgressInformation();
  void onProcessingFinished();
  void cancel();
signals:
  void progressWindowShouldShow();
  void done(QString errorMessage);
  void progression(float progress, int duration, unsigned long memory);

private:
  void endApplication(const QString & errorMessage);
  FilterThread * _filterThread;
  cimg_library::CImgList<float> * _gmicImages;
  ProgressInfoWindow * _progressWindow;
  QTimer _timer;
  QString _filterName;
  QString _path;
  QString _command;
  QString _arguments;
  OutputMode _outputMode;
  OutputMessageMode _outputMessageMode;
  InputMode _inputMode;
  QTimer _singleShotTimer;
  bool _processingCompletedProperly;
  QString _errorMessage;
  QString _hash;
  QVector<bool> _gmicStatusQuotedParameters;
};

} // namespace GmicQt

#endif // GMIC_QT_HEADLESSPROCESSOR_H
