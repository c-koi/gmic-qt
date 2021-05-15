/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file ChoiceParameter.h
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
#ifndef GMIC_QT_CHOICEPARAMETER_H
#define GMIC_QT_CHOICEPARAMETER_H

#include <QList>
#include <QString>
#include "AbstractParameter.h"
class QComboBox;
class QLabel;

class ChoiceParameter : public AbstractParameter {
  Q_OBJECT
public:
  ChoiceParameter(QObject * parent = nullptr);
  ~ChoiceParameter() override;
  virtual int size() const override;
  bool addTo(QWidget *, int row) override;
  QString value() const override;
  QString defaultValue() const override;
  void setValue(const QString &) override;
  void reset() override;
  bool initFromText(const char * text, int & textLength) override;
public slots:
  void onComboBoxIndexChanged(int);

private:
  void connectComboBox();
  void disconnectComboBox();
  QString _name;
  int _default;
  int _value;
  QLabel * _label;
  QComboBox * _comboBox;
  QList<QString> _choices;
  bool _connected;
};

#endif // GMIC_QT_CHOICEPARAMETER_H
