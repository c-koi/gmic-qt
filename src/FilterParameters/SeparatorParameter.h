/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file SeparatorParameter.h
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
#ifndef GMIC_QT_SEPARATORPARAMETER_H
#define GMIC_QT_SEPARATORPARAMETER_H

#include "AbstractParameter.h"
class QFrame;

class SeparatorParameter : public AbstractParameter {
  Q_OBJECT
public:
  SeparatorParameter(QObject * parent = nullptr);
  ~SeparatorParameter()  override;
  bool addTo(QWidget *, int row) override;
  QString textValue() const override;
  void setValue(const QString & value) override;
  void reset() override;
  bool initFromText(const char * text, int & textLength) override;

private:
  QFrame * _frame;
};

#endif // GMIC_QT_SEPARATORPARAMETER_H
