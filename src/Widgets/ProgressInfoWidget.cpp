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
#include "Widgets/ProgressInfoWidget.h"
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include "GmicProcessor.h"
#include "IconLoader.h"
#include "Misc.h"
#include "ui_progressinfowidget.h"

#ifdef _IS_WINDOWS_
#include <windows.h>
#include <process.h>
#include <psapi.h>
#endif

namespace GmicQt
{

ProgressInfoWidget::ProgressInfoWidget(QWidget * parent) : QWidget(parent), ui(new Ui::ProgressInfoWidget), _gmicProcessor(nullptr)
{
  ui->setupUi(this);
  _mode = Mode::GmicProcessing;
  _canceled = false;
  _growing = true;
  setWindowTitle(tr("G'MIC-Qt Plug-in progression"));
  ui->progressBar->setRange(0, 100);
  ui->tbCancel->setIcon(LOAD_ICON("process-stop"));
  ui->tbCancel->setToolTip(tr("Abort"));
  connect(&_timer, &QTimer::timeout, this, &ProgressInfoWidget::onTimeOut);
  connect(ui->tbCancel, &QToolButton::clicked, this, &ProgressInfoWidget::onCancelClicked);
  if (!parent) {
    QRect position = frameGeometry();
    QList<QScreen *> screens = QGuiApplication::screens();
    if (!screens.isEmpty()) {
      position.moveCenter(screens.front()->geometry().center());
      move(position.topLeft());
    }
  }

  _showingTimer.setSingleShot(true);
  _showingTimer.setInterval(500);
  connect(&_showingTimer, &QTimer::timeout, this, &ProgressInfoWidget::onTimeOut);
  connect(&_showingTimer, &QTimer::timeout, &_timer, QOverload<>::of(&QTimer::start));
  connect(&_showingTimer, &QTimer::timeout, this, &ProgressInfoWidget::show);
}

ProgressInfoWidget::~ProgressInfoWidget()
{
  delete ui;
}

ProgressInfoWidget::Mode ProgressInfoWidget::mode() const
{
  return _mode;
}

bool ProgressInfoWidget::hasBeenCanceled() const
{
  return _canceled;
}

void ProgressInfoWidget::setGmicProcessor(const GmicProcessor * processor)
{
  _gmicProcessor = processor;
}

void ProgressInfoWidget::onTimeOut()
{
  if (_mode == Mode::GmicProcessing) {
    updateThreadInformation();
  } else if (_mode == Mode::FiltersUpdate) {
    updateUpdateProgression();
  }
}

void ProgressInfoWidget::onCancelClicked()
{
  _canceled = true;
  emit cancel();
}

void ProgressInfoWidget::stopAnimationAndHide()
{
  _timer.stop();
  _showingTimer.stop();
  hide();
}

void ProgressInfoWidget::startFilterThreadAnimationAndShow(bool showCancelButton)
{
  layout()->removeWidget(ui->tbCancel);
  layout()->removeWidget(ui->progressBar);
  layout()->removeWidget(ui->label);
  layout()->addWidget(ui->progressBar);
  layout()->addWidget(ui->tbCancel);
  layout()->addWidget(ui->label);

  _canceled = false;
  _mode = Mode::GmicProcessing;
  ui->progressBar->setRange(0, 100);
  ui->progressBar->setValue(0);
  ui->progressBar->setInvertedAppearance(false);
  onTimeOut();
  _timer.setInterval(250);
  _timer.start();
  ui->tbCancel->setVisible(showCancelButton);
  show();
}

void ProgressInfoWidget::startFiltersUpdateAnimationAndShow()
{

  layout()->removeWidget(ui->tbCancel);
  layout()->removeWidget(ui->progressBar);
  layout()->removeWidget(ui->label);
  layout()->addWidget(ui->label);
  layout()->addWidget(ui->tbCancel);
  layout()->addWidget(ui->progressBar);

  _mode = Mode::FiltersUpdate;
  _canceled = false;
  // ui->progressBar->setRange(0, 0);
  ui->progressBar->setValue(AnimationStep);
  ui->progressBar->setTextVisible(false);
  ui->progressBar->setInvertedAppearance(false);
  ui->label->setText(tr("Updating filters..."));
  _timer.setInterval(75);

  _growing = true;
  ui->tbCancel->setVisible(true);

  _showingTimer.start();
  // Connected to :
  // 1. onTimeOut();
  // 2. _timer.start();
  // 3. show();
}

void ProgressInfoWidget::showBusyIndicator()
{
  ui->progressBar->setRange(0, 0);
}

void ProgressInfoWidget::updateThreadInformation()
{
  int ms = _gmicProcessor->duration();
  float progress = _gmicProcessor->progress();
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
  QString durationStr = readableDuration(ms);
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

void ProgressInfoWidget::updateUpdateProgression()
{
  int value = ui->progressBar->value();
  if (_growing) {
    value += AnimationStep;
    if (value >= 100) {
      value = 100 - AnimationStep;
      ui->progressBar->setInvertedAppearance(!ui->progressBar->invertedAppearance());
      ui->progressBar->setValue(value);
      _growing = false;
    } else {
      ui->progressBar->setValue(value);
    }
  } else {
    value -= AnimationStep;
    if (value <= 0) {
      value = AnimationStep;
      ui->progressBar->setValue(value);
      _growing = true;
    } else {
      ui->progressBar->setValue(value);
    }
  }
}

} // namespace GmicQt
