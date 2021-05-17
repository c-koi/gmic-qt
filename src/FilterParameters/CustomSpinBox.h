/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file CustomSpinBox.h
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
#ifndef GMIC_QT_CUSTOMSPINBOX_H
#define GMIC_QT_CUSTOMSPINBOX_H

#include <QSize>
#include <QSpinBox>
class QShowEvent;
class QResizeEvent;

namespace GmicQt
{

class CustomSpinBox : public QSpinBox {
  Q_OBJECT
public:
  CustomSpinBox(QWidget * parent, int min, int max);
  ~CustomSpinBox() override;
  QString textFromValue(int value) const override;
  inline bool unfinishedKeyboardEditing() const;

protected:
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;
  void keyPressEvent(QKeyEvent *) override;
signals:
  void keyboardNumericalInputOngoing();

private:
  QSize _sizeHint;
  QSize _minimumSizeHint;
  bool _unfinishedKeyboardEditing = false;
  static const int MAX_DIGITS = 5;
  static int integerPartDigitCount(float value);
};

bool CustomSpinBox::unfinishedKeyboardEditing() const
{
  return _unfinishedKeyboardEditing;
}

} // namespace GmicQt

#endif // GMIC_QT_CUSTOMSPINBOX_H
