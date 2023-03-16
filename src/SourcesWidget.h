/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file SourcesWidget.h
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
#ifndef GMIC_QT_SOURCESWIDGET_H
#define GMIC_QT_SOURCESWIDGET_H

#include <QString>
#include <QStringList>
#include <QWidget>

namespace Ui
{
class SourcesWidget;
}

class QSettings;

namespace GmicQt
{

class SourcesWidget : public QWidget {
  Q_OBJECT

public:
  enum class OfficialFilters
  {
    Disabled,
    EnabledWithoutUpdates,
    EnabledWithUpdates
  };

  explicit SourcesWidget(QWidget * parent = nullptr);
  ~SourcesWidget() override;

  QStringList list() const;
  static QStringList defaultList();
  void saveSettings();
  bool sourcesModified(bool & internetUpdateRequired);

private slots:
  void onOpenFile();
  void onAddNew();
  void setToDefault();
  void enableButtons();
  void removeCurrentSource();
  void onMoveDown();
  void onMoveUp();

private:
  Ui::SourcesWidget * ui;
  QString _newItemText;
  QStringList _sourcesAtOpening;
  OfficialFilters _officialFiltersAtOpening;
};

} // namespace GmicQt
#endif // GMIC_QT_SOURCESWIDGET_H
