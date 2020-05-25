/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ProgressInfoWindow.cpp
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
#include "ProgressInfoWindow.h"
#include <QApplication>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>
#include <QSettings>
#include <QStyleFactory>
#include "Common.h"
#include "DialogSettings.h"
#include "FilterThread.h"
#include "Globals.h"
#include "GmicStdlib.h"
#include "HeadlessProcessor.h"
#include "Updater.h"
#include "ui_progressinfowindow.h"
#include "gmic.h"

ProgressInfoWindow::ProgressInfoWindow(HeadlessProcessor * processor) : QMainWindow(nullptr), ui(new Ui::ProgressInfoWindow), _processor(processor)
{
  ui->setupUi(this);
  setWindowTitle("G'MIC-Qt Plug-in progression");
  processor->setProgressWindowFlag(true);

  ui->label->setText(QString("%1").arg(processor->filterName()));
  ui->progressBar->setRange(0, 100);
  ui->progressBar->setValue(100);
  ui->info->setText("");
  connect(processor, SIGNAL(singleShotTimeout()), this, SLOT(show()));
  connect(ui->pbCancel, SIGNAL(clicked(bool)), this, SLOT(onCancelClicked(bool)));
  connect(processor, SIGNAL(progression(float, int, ulong)), this, SLOT(onProgress(float, int, ulong)));
  connect(processor, SIGNAL(done(QString)), this, SLOT(onProcessingFinished(QString)));
  _isShown = false;

  if (DialogSettings::darkThemeEnabled()) {
    setDarkTheme();
  }
}

ProgressInfoWindow::~ProgressInfoWindow()
{
  delete ui;
}

void ProgressInfoWindow::showEvent(QShowEvent *)
{
  QRect position = frameGeometry();
  QList<QScreen *> screens = QGuiApplication::screens();
  if (!screens.isEmpty()) {
    position.moveCenter(screens.front()->geometry().center());
    move(position.topLeft());
  }
  _isShown = true;
}

void ProgressInfoWindow::closeEvent(QCloseEvent * event)
{
  event->accept();
}

void ProgressInfoWindow::setDarkTheme()
{
  qApp->setStyle(QStyleFactory::create("Fusion"));
  QPalette p = qApp->palette();
  p.setColor(QPalette::Window, QColor(53, 53, 53));
  p.setColor(QPalette::Button, QColor(73, 73, 73));
  p.setColor(QPalette::Highlight, QColor(110, 110, 110));
  p.setColor(QPalette::Text, QColor(255, 255, 255));
  p.setColor(QPalette::ButtonText, QColor(255, 255, 255));
  p.setColor(QPalette::WindowText, QColor(255, 255, 255));
  QColor linkColor(100, 100, 100);
  linkColor = linkColor.lighter();
  p.setColor(QPalette::Link, linkColor);
  p.setColor(QPalette::LinkVisited, linkColor);

  p.setColor(QPalette::Disabled, QPalette::Button, QColor(53, 53, 53));
  p.setColor(QPalette::Disabled, QPalette::Window, QColor(53, 53, 53));
  p.setColor(QPalette::Disabled, QPalette::Text, QColor(110, 110, 110));
  p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(110, 110, 110));
  p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(110, 110, 110));
  qApp->setPalette(p);
}

void ProgressInfoWindow::onCancelClicked(bool)
{
  ui->pbCancel->setEnabled(false);
  _processor->cancel();
}

void ProgressInfoWindow::onProgress(float progress, int duration, unsigned long memory)
{
  if (!_isShown) {
    return;
  }
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
  QString durationStr;
  if (duration >= 60000) {
    durationStr = QTime::fromMSecsSinceStartOfDay(duration).toString("HH:mm:ss");
  } else {
    durationStr = QString(tr("%1 seconds")).arg(duration / 1000);
  }
  QString memoryStr;
  unsigned long kiB = memory / 1024;
  if (kiB >= 1024) {
    memoryStr = QString("%1 MiB").arg(kiB / 1024);
  } else {
    memoryStr = QString("%1 KiB").arg(kiB);
  }
  if (kiB) {
    ui->info->setText(QString(tr("[Processing %1 | %2]")).arg(durationStr).arg(memoryStr));
  } else {
    ui->info->setText(QString(tr("[Processing %1]")).arg(durationStr));
  }
}

void ProgressInfoWindow::onProcessingFinished(const QString & errorMessage)
{
  if (!errorMessage.isEmpty()) {
    QMessageBox::warning(this, "Error", errorMessage, QMessageBox::Close);
  }
  close();
}
