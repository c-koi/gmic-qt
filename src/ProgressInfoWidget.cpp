/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ProgressInfoWidget.cpp
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
#include "ProgressInfoWidget.h"
#include <QDesktopWidget>
#include <QFile>
#include "DialogSettings.h"
#include "FilterThread.h"
#include "ui_progressinfowidget.h"

#ifdef _IS_WINDOWS_
#include <process.h>
#include <psapi.h>
#include <windows.h>
#endif

ProgressInfoWidget::ProgressInfoWidget(QWidget * parent) : QWidget(parent), ui(new Ui::ProgressInfoWidget), _filterThread(0)
{
  ui->setupUi(this);
  setWindowTitle(tr("G'MIC-Qt Plug-in progression"));
  ui->progressBar->setRange(0, 100);
  ui->tbCancel->setIcon(LOAD_ICON("process-stop"));
  _timer.setInterval(250);
  connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimeOut()));
  connect(ui->tbCancel, SIGNAL(clicked(bool)), this, SLOT(onCancelClicked(bool)));
  if (!parent) {
    QRect position = frameGeometry();
    position.moveCenter(QDesktopWidget().availableGeometry().center());
    move(position.topLeft());
  }
}

ProgressInfoWidget::~ProgressInfoWidget()
{
  delete ui;
}

void ProgressInfoWidget::onTimeOut()
{
  if (!_filterThread) {
    return;
  }
  int ms = _filterThread->duration();
  float progress = _filterThread->progress();
  if (progress >= 0) {
    ui->progressBar->setInvertedAppearance(false);
    ui->progressBar->setTextVisible(true);
    ui->progressBar->setValue((int)progress);
  } else {
    ui->progressBar->setTextVisible(false);
    int value = ui->progressBar->value();
    value += 20;
    if (value > 100) {
      ui->progressBar->setValue(value - 100);
      ui->progressBar->setInvertedAppearance(!ui->progressBar->invertedAppearance());
    } else {
      ui->progressBar->setValue(value);
    }
  }
  QTime duration = QTime::fromMSecsSinceStartOfDay(ms);
  QString durationStr = (ms >= 60000) ? duration.toString("HH:mm:ss") : QString("%1 seconds").arg(ms / 1000);
#ifdef _IS_LINUX_
  // Get memory usage
  QString memoryStr("? KiB");
  QFile status("/proc/self/status");
  if (status.open(QFile::ReadOnly)) {
    QByteArray text = status.readAll();
    const char * str = strstr(text.constData(), "VmRSS:");
    unsigned int kiB;
    if (str && sscanf(str + 7, "%u", &kiB)) {
      if (kiB >= 1024) {
        memoryStr = QString("%1 MiB").arg(kiB / 1024);
      } else {
        memoryStr = QString("%1 KiB").arg(kiB);
      }
    }
  }
  ui->label->setText(QString(tr("[Processing %1 | %2]")).arg(durationStr).arg(memoryStr));
#elif defined(_IS_WINDOWS_)
  PROCESS_MEMORY_COUNTERS counters;
  unsigned long kiB = 0;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
    kiB = static_cast<unsigned long>(counters.WorkingSetSize / 1024);
  }
  QString memoryStr;
  if (kiB >= 1024) {
    memoryStr = QString("%1 MiB").arg(kiB / 1024);
  } else {
    memoryStr = QString("%1 KiB").arg(kiB);
  }
  ui->label->setText(QString(tr("[Processing %1 | %2]")).arg(durationStr).arg(memoryStr));
#else
  ui->label->setText(QString(tr("[Processing %1]")).arg(durationStr));
#endif
}

void ProgressInfoWidget::onCancelClicked(bool)
{
  emit cancel();
}

void ProgressInfoWidget::stopAnimationAndHide()
{
  _filterThread = 0;
  _timer.stop();
  hide();
}

void ProgressInfoWidget::startAnimationAndShow(FilterThread * thread, bool showCancelButton)
{
  _filterThread = thread;
  ui->progressBar->setValue(0);
  onTimeOut();
  _timer.start();
  ui->tbCancel->setVisible(showCancelButton);
  show();
}
