/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file SearchFieldWidget.h
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
#ifndef _GMIC_QT_SEARCHFIELDWIDGET_H_
#define _GMIC_QT_SEARCHFIELDWIDGET_H_
#include <QIcon>
#include <QWidget>

namespace Ui
{
class SearchFieldWidget;
}

class QAction;
class QLineEdit;
class QToolButton;

class SearchFieldWidget : public QWidget {
  Q_OBJECT
public:
  explicit SearchFieldWidget(QWidget * parent = nullptr);
  ~SearchFieldWidget();
  QString text() const;
public slots:
  void clear();
signals:
  void textChanged(QString);
private slots:
  void onTextChanged(const QString &);

private:
  Ui::SearchFieldWidget * ui;
  bool _empty;
  QIcon _clearIcon;
  QIcon _findIcon;
  QLineEdit * _lineEdit;
#if QT_VERSION >= 0x050200
  QAction * _action;
#else
  QToolButton * _button;
#endif
};

#endif // _GMIC_QT_SEARCHFIELDWIDGET_H_
