/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file MultilineTextParameterWidget.cpp
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
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include "Common.h"
#include "FilterParameters/MultilineTextParameterWidget.h"
#include "ui_multilinetextparameterwidget.h"

MultilineTextParameterWidget::MultilineTextParameterWidget(QString name, QString value, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MultilineTextParameterWidget)
{
  ui->setupUi(this);
  ui->textEdit->document()->setPlainText(value);
  ui->textEdit->installEventFilter(this);
  ui->label->setText(name);
  ui->pbUpdate->setToolTip(tr("Ctrl+Return"));
  connect(ui->pbUpdate,SIGNAL(clicked(bool)),
          this,SLOT(onUpdate(bool)));
}

MultilineTextParameterWidget::~MultilineTextParameterWidget()
{
  delete ui;
}

QString MultilineTextParameterWidget::text() const
{
  return ui->textEdit->document()->toPlainText();
}

void MultilineTextParameterWidget::setText(const QString & text)
{
  ui->textEdit->document()->setPlainText(text);
}

void MultilineTextParameterWidget::onUpdate(bool)
{
  emit valueChanged();
}

bool MultilineTextParameterWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
    if ( keyEvent->modifiers() & Qt::ControlModifier
         && ( keyEvent->key() == Qt::Key_Enter
              || keyEvent->key() == Qt::Key_Return ) ) {
      onUpdate(true);
      return true;
    }
  }
  return QObject::eventFilter(obj, event);
}
