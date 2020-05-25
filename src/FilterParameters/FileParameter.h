/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FileParameter.h
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
#ifndef GMIC_QT_FILEPARAMETER_H
#define GMIC_QT_FILEPARAMETER_H

#include <QString>
#include "AbstractParameter.h"
class QLabel;
class QPushButton;

class FileParameter : public AbstractParameter {
  Q_OBJECT
public:
  FileParameter(QObject * parent = nullptr);
  ~FileParameter() override;
  bool addTo(QWidget *, int row) override;
  QString textValue() const override;
  QString unquotedTextValue() const override;
  void setValue(const QString & value) override;
  void reset() override;
  bool initFromText(const char * text, int & textLength) override;
  bool isQuoted() const override;
public slots:
  void onButtonPressed();

private:
  enum DialogMode
  {
    InputMode,
    OutputMode,
    InputOutputMode
  };
  QString _name;
  QString _default;
  QString _value;
  QLabel * _label;
  QPushButton * _button;
  DialogMode _dialogMode;
};

#endif // GMIC_QT_FILEPARAMETER_H
