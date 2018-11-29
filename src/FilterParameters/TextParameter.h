/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file TextParameter.h
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
#ifndef _GMIC_QT_TEXTPARAMETER_H_
#define _GMIC_QT_TEXTPARAMETER_H_

#include <QString>
#include "AbstractParameter.h"

class QLineEdit;
class QLabel;
class QTextEdit;
class QAction;
class MultilineTextParameterWidget;

class TextParameter : public AbstractParameter {
  Q_OBJECT
public:
  TextParameter(QObject * parent = nullptr);
  ~TextParameter();
  void addTo(QWidget *, int row) override;
  QString textValue() const override;
  QString unquotedTextValue() const override;
  void setValue(const QString & value) override;
  void reset() override;
  bool initFromText(const char * text, int & textLength) override;
  bool isQuoted() const override;

private slots:
  void onValueChanged();

private:
  void connectEditor();
  void disconnectEditor();
  QString _name;
  QString _default;
  QString _value;
  QLabel * _label;
  QLineEdit * _lineEdit;
  MultilineTextParameterWidget * _textEdit;
  QAction * _updateAction;
  bool _multiline;
  bool _connected;
};

#endif // _GMIC_QT_TEXTPARAMETER_H_
