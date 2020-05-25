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
#include "gmic_qt.h"

class FilterThread;

namespace cimg_library
{
template <typename T> struct CImgList;
}

class HeadlessProcessor : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Construct a headless processor a given command and arguments
   *        (e.g. GIMP script mode)
   *
   * @param parent
   */
  explicit HeadlessProcessor(QObject * parent, const char * command, GmicQt::InputMode inputMode, GmicQt::OutputMode outputMode);

  /**
   * @brief Construct a headless processor using last execution parameters
   *
   * @param parent
   */
  explicit HeadlessProcessor(QObject * parent = nullptr);

  ~HeadlessProcessor();
  QString command() const;
  QString filterName() const;
  void setProgressWindowFlag(bool);

public slots:
  void startProcessing();
  void onTimeout();
  void onProcessingFinished();
  void cancel();
signals:
  void singleShotTimeout();
  void done(QString errorMessage);
  void progression(float progress, int duration, unsigned long memory);

private:
  FilterThread * _filterThread;
  cimg_library::CImgList<float> * _gmicImages;
  QTimer _timer;
  QString _filterName;
  QString _lastCommand;
  QString _lastArguments;
  GmicQt::OutputMode _outputMode;
  GmicQt::OutputMessageMode _outputMessageMode;
  GmicQt::InputMode _inputMode;
  QString _lastEnvironment;
  bool _hasProgressWindow;
  QTimer _singleShotTimer;
  QString _gmicStatusQuotedParameters;
};

#endif // GMIC_QT_HEADLESSPROCESSOR_H
