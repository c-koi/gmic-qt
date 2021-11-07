/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file GmicProcessor.h
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
#ifndef GMIC_QT_GMICPROCESSOR_H
#define GMIC_QT_GMICPROCESSOR_H

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QSettings>
#include <QSignalMapper>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <deque>
#include "GmicQt.h"
#include "InputOutputState.h"

namespace cimg_library
{
template <typename T> struct CImgList;
template <typename T> struct CImg;
} // namespace cimg_library

namespace GmicQt
{
class FilterThread;
class FilterSyncRunner;

class GmicProcessor : public QObject {
  Q_OBJECT
public:
  struct FilterContext {
    enum class RequestType
    {
      SynchronousPreview,
      Preview,
      FullImage
    };
    struct VisibleRect {
      double x, y, w, h;
    };
    struct PositionStringCorrection {
      double xFactor;
      double yFactor;
    };
    RequestType requestType;
    VisibleRect visibleRect;
    InputOutputState inputOutputState;
    OutputMessageMode outputMessageMode;
    PositionStringCorrection positionStringCorrection;
    double zoomFactor;
    int previewWidth;
    int previewHeight;
    int previewTimeout;
    QString filterName;
    QString filterCommand;
    QString filterFullPath;
    QString filterArguments;
    QString filterHash;
  };

  GmicProcessor(QObject * parent = nullptr);
  ~GmicProcessor() override;
  void init();
  void setContext(const FilterContext & context);
  void execute();

  bool isProcessingFullImage() const;

  bool isProcessing() const;
  bool isIdle() const;
  bool hasUnfinishedAbortedThreads() const;

  const cimg_library::CImg<float> & previewImage() const;
  const QStringList & gmicStatus() const;
  const QList<int> & parametersVisibilityStates() const;
  void setGmicStatusQuotedParameters(const QVector<bool> & quotedParameters);

  void saveSettings(QSettings & settings);

  int duration() const;
  float progress() const;
  int lastPreviewFilterExecutionDurationMS() const;
  void resetLastPreviewFilterExecutionDurations();
  void recordPreviewFilterExecutionDurationMS(int duration);
  int averagePreviewFilterExecutionDuration() const;
  int completedFullImageProcessingCount() const;

public slots:
  void cancel();

signals:
  void previewCommandFailed(QString errorMessage);
  void fullImageProcessingFailed(QString errorMessage);
  void previewImageAvailable();
  void fullImageProcessingDone();
  void noMoreUnfinishedJobs();
  void aboutToSendImagesToHost();

private slots:
  void onPreviewThreadFinished();
  void onApplyThreadFinished();
  void onAbortedThreadFinished();
  void showWaitingCursor();
  void hideWaitingCursor();

private:
  void updateImageNames(cimg_library::CImgList<char> & imageNames);
  void abortCurrentFilterThread();
  void manageSynchonousRunner(FilterSyncRunner & runner);

  FilterThread * _filterThread;
  FilterContext _filterContext;
  cimg_library::CImgList<float> * _gmicImages;
  cimg_library::CImg<float> * _previewImage;
  QList<FilterThread *> _unfinishedAbortedThreads;

  unsigned int _previewRandomSeed;
  QStringList _gmicStatus;
  QList<int> _parametersVisibilityStates;
  QTimer _waitingCursorTimer;
  static const int WAITING_CURSOR_DELAY = 200;

  QString _lastAppliedFilterPath;
  QString _lastAppliedFilterHash;
  QString _lastAppliedCommand;
  QString _lastAppliedCommandArguments;
  QStringList _lastAppliedCommandGmicStatus;
  InputOutputState _lastAppliedCommandInOutState;
  QElapsedTimer _filterExecutionTime;
  std::deque<int> _lastFilterPreviewExecutionDurations;
  int _completeFullImageProcessingCount;
  QVector<bool> _gmicStatusQuotedParameters;
};

} // namespace GmicQt

#endif // GMIC_QT_GMICPROCESSOR_H
