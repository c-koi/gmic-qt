/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersPresenter.h
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
#ifndef GMIC_QT_FILTERSPRESENTER_H
#define GMIC_QT_FILTERSPRESENTER_H
#include <QObject>
#include "FilterSelector/FavesModel.h"
#include "FilterSelector/FiltersModel.h"
#include "FilterSelector/FiltersView/FiltersView.h"
#include "InputOutputState.h"

class QSettings;

class FiltersPresenter : public QObject {
  Q_OBJECT
public:
  struct Filter {
    QString name;
    QString plainTextName;
    QString command;
    QString previewCommand;
    QString parameters;
    QList<QString> defaultParameterValues;
    QList<int> defaultVisibilityStates;
    QString hash;
    bool isAccurateIfZoomed;
    float previewFactor;
    bool isAFave;
    void clear();
    void setInvalid();
    bool isInvalid() const;
    bool isNoApplyFilter() const;
    bool isNoPreviewFilter() const;
    const char * previewFactorString() const;
  };

  FiltersPresenter(QObject * parent);
  ~FiltersPresenter();
  void setFiltersView(FiltersView * filtersView);
  void rebuildFilterView();
  void rebuildFilterViewWithSelection(const QList<QString> & keywords);

  void clear();
  void readFilters();
  void readFaves();

  bool allFavesAreValid() const;
  bool danglingFaveIsSelected() const;

  /**
   * @brief restoreFaveHashLinksRelease236
   * Starting with release 240 of gmic, filter name capitalization has been normalized.
   * For example : "Add grain" became "Add Grain"
   * As a consequence, links between faves and filters based on hashes (computed in part
   * from the name) were broken.
   * This method tries to restore the links in the case when 4 faves or more are broken.
   */
  void restoreFaveHashLinksAfterCaseChange();
  void importGmicGTKFaves();
  void saveFaves();
  void addSelectedFilterAsNewFave(const QList<QString> & defaultValues, const QList<int> & visibilityStates, GmicQt::InputOutputState inOutState);

  void applySearchCriterion(const QString & text);
  void selectFilterFromHash(QString hash, bool notify);
  const Filter & currentFilter() const;

  void loadSettings(const QSettings & settings);
  void saveSettings(QSettings & settings);

  void setInvalidFilter();
  bool isInvalidFilter() const;

  void adjustViewSize();
  void expandFaveFolder();
  void expandPreviousSessionExpandedFolders();

  void expandAll();
  void collapseAll();

  const QString & errorMessage() const;

signals:
  void filterSelectionChanged();
  void faveAdditionRequested(QString);

public slots:
  void removeSelectedFave();
  void editSelectedFaveName();
  void onFaveRenamed(const QString & hash, const QString & name);
  void toggleSelectionMode(bool on);

private slots:
  void onFilterChanged(const QString & hash);
  void removeFave(const QString & hash);

private:
  void setCurrentFilter(const QString & hash);

  FiltersModel _filtersModel;
  FavesModel _favesModel;
  FiltersView * _filtersView;
  Filter _currentFilter;
  QString _errorMessage;
};

#endif // GMIC_QT_FILTERSPRESENTER_H
