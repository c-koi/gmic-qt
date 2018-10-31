/** -*- mode: c++ ; c-basic-offset: 2 -*-
 *
 *  @file FiltersPresenter.cpp
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
#include "FilterSelector/FiltersPresenter.h"
#include <QDebug>
#include <QSettings>
#include "Common.h"
#include "FilterSelector/FavesModelReader.h"
#include "FilterSelector/FavesModelWriter.h"
#include "FilterSelector/FiltersModelReader.h"
#include "FiltersVisibilityMap.h"
#include "Globals.h"
#include "GmicStdlib.h"
#include "Logger.h"
#include "ParametersCache.h"
#include "Utils.h"
#include "Widgets/InOutPanel.h"

FiltersPresenter::FiltersPresenter(QObject * parent) : QObject(parent)
{
  _filtersView = 0;
}

FiltersPresenter::~FiltersPresenter()
{
  saveFaves();
}

void FiltersPresenter::setFiltersView(FiltersView * filtersView)
{
  if (_filtersView) {
    _filtersView->disconnect(this);
  }
  _filtersView = filtersView;
  connect(_filtersView, SIGNAL(filterSelected(QString)), this, SLOT(onFilterChanged(QString)));
  connect(_filtersView, SIGNAL(faveRenamed(QString, QString)), this, SLOT(onFaveRenamed(QString, QString)));
  connect(_filtersView, SIGNAL(faveRemovalRequested(QString)), this, SLOT(removeFave(QString)));
  connect(_filtersView, SIGNAL(faveAdditionRequested(QString)), this, SIGNAL(faveAdditionRequested(QString)));
}

void FiltersPresenter::rebuildFilterView()
{
  rebuildFilterViewWithSelection(QList<QString>());
}

void FiltersPresenter::rebuildFilterViewWithSelection(QList<QString> keywords)
{
  _filtersView->clear();
  _filtersView->disableModel();
  for (const FiltersModel::Filter & filter : _filtersModel) {
    if (filter.matchKeywords(keywords)) {
      _filtersView->addFilter(filter.name(), filter.hash(), filter.path(), filter.isWarning());
    }
  }
  FavesModel::const_iterator itFave = _favesModel.cbegin();
  while (itFave != _favesModel.cend()) {
    if (itFave->matchKeywords(keywords)) {
      _filtersView->addFave(itFave->name(), itFave->hash());
    }
    ++itFave;
  }
  _filtersView->sort();

  QString header = QObject::tr("Available filters (%1)").arg(_filtersModel.notTestingFilterCount());
  _filtersView->setHeader(header);
  _filtersView->enableModel();
}

void FiltersPresenter::clear()
{
  _favesModel.clear();
  _filtersModel.clear();
}

void FiltersPresenter::readFilters()
{
  _filtersModel.clear();
  if (GmicStdLib::Array.isEmpty()) {
    GmicStdLib::loadStdLib();
  }
  FiltersModelReader filterModelReader(_filtersModel);
  filterModelReader.parseFiltersDefinitions(GmicStdLib::Array);
}

void FiltersPresenter::readFaves()
{
  FavesModelReader favesModelReader(_favesModel);
  favesModelReader.loadFaves();
}

bool FiltersPresenter::allFavesAreValid() const
{
  for (const FavesModel::Fave & fave : _favesModel) {
    if (!_filtersModel.contains(fave.originalHash())) {
      return false;
    }
  }
  return true;
}

void FiltersPresenter::restoreFaveHashLinksAfterCaseChange()
{
  if (allFavesAreValid()) {
    return;
  }
  FavesModel formerFaveModel = _favesModel;
  FavesModel::const_iterator itFormerFave = formerFaveModel.cbegin();
  bool someFavesHaveBeenRelinked = false;
  while (itFormerFave != formerFaveModel.cend()) {
    const FavesModel::Fave & fave = *itFormerFave;
    if (!_filtersModel.contains(fave.originalHash())) {
      FiltersModel::const_iterator itFilter = _filtersModel.cbegin();
      while ((itFilter != _filtersModel.cend()) && (itFilter->hash236() != fave.originalHash())) {
        ++itFilter;
      }
      if (itFilter != _filtersModel.cend()) {
        _favesModel.removeFave(fave.hash());
        FavesModel::Fave newFave = fave;
        newFave.setOriginalHash(itFilter->hash());
        newFave.setOriginalName(itFilter->name());
        _favesModel.addFave(newFave);
        QString message = QString("\n[%1]./information/ Fave '%2' has been relinked to filter '%3'\n").arg(GmicQt::pluginCodeName()).arg(fave.name()).arg(itFilter->name());
        Logger::log(message);
        someFavesHaveBeenRelinked = true;
      } else {
        QString message = QString("\n[%1]./warning/ Could not associate Fave '%2' to an existing filter\n").arg(GmicQt::pluginCodeName()).arg(fave.name());
        Logger::log(message);
      }
    }
    ++itFormerFave;
  }
  if (someFavesHaveBeenRelinked) {
    saveFaves();
  }
}

void FiltersPresenter::importGmicGTKFaves()
{
  FavesModelReader favesModelReader(_favesModel);
  favesModelReader.importFavesFromGmicGTK();
}

void FiltersPresenter::saveFaves()
{
  FavesModelWriter favesModelWriter(_favesModel);
  favesModelWriter.writeFaves();
}

void FiltersPresenter::addSelectedFilterAsNewFave(QList<QString> defaultValues, GmicQt::InputOutputState inOutState)
{
  if (_currentFilter.hash.isEmpty() || (!_filtersModel.contains(_currentFilter.hash) && !_favesModel.contains(_currentFilter.hash))) {
    return;
  }

  FavesModel::Fave fave;
  fave.setDefaultValues(defaultValues);

  if (_filtersModel.contains(_currentFilter.hash)) {
    const FiltersModel::Filter & filter = _filtersModel.getFilterFromHash(_currentFilter.hash);
    fave.setName(_favesModel.uniqueName(filter.name(), QString()));
    fave.setCommand(filter.command());
    fave.setPreviewCommand(filter.previewCommand());
    fave.setOriginalHash(filter.hash());
    fave.setOriginalName(filter.name());
  } else {
    FavesModel::const_iterator faveIterator = _favesModel.findFaveFromHash(_currentFilter.hash);
    if (faveIterator != _favesModel.cend()) {
      const FavesModel::Fave & originalFave = *faveIterator;
      fave.setName(_favesModel.uniqueName(originalFave.name(), QString()));
      fave.setCommand(originalFave.command());
      fave.setPreviewCommand(originalFave.previewCommand());
      fave.setOriginalHash(originalFave.originalHash());
      fave.setOriginalName(originalFave.originalName());
    }
  }

  fave.build();
  FiltersVisibilityMap::setVisibility(fave.hash(), true);
  _favesModel.addFave(fave);
  ParametersCache::setValues(fave.hash(), defaultValues);
  ParametersCache::setInputOutputState(fave.hash(), inOutState);
  _filtersView->addFave(fave.name(), fave.hash());
  _filtersView->sortFaves();
  _filtersView->selectFave(fave.hash());
  onFilterChanged(fave.hash());
  saveFaves();
}

void FiltersPresenter::applySearchCriterion(const QString & text)
{
  static QString previousText;
  if ((!text.isEmpty() && previousText.isEmpty()) || (text.isEmpty() && previousText.isEmpty())) {
    _filtersView->preserveExpandedFolders();
  }
  QList<QString> keywords = text.split(QChar(' '), QString::SkipEmptyParts);
  rebuildFilterViewWithSelection(keywords);
  if (text.isEmpty()) {
    _filtersView->restoreExpandedFolders();
  } else {
    _filtersView->expandAll();
  }
  if (!_currentFilter.hash.isEmpty()) {
    selectFilterFromHash(_currentFilter.hash, false);
  }
  previousText = text;
}

void FiltersPresenter::selectFilterFromHash(QString hash, bool notify)
{
  if (_favesModel.contains(hash)) {
    _filtersView->selectFave(hash);
  } else if (_filtersModel.contains(hash)) {
    const FiltersModel::Filter & filter = _filtersModel.getFilterFromHash(hash);
    _filtersView->selectActualFilter(hash, filter.path());
  } else {
    hash.clear();
  }
  setCurrentFilter(hash);
  if (notify) {
    emit filterSelectionChanged();
  }
}

const FiltersPresenter::Filter & FiltersPresenter::currentFilter() const
{
  return _currentFilter;
}

void FiltersPresenter::loadSettings(const QSettings & settings)
{
  _filtersView->loadSettings(settings);
}

void FiltersPresenter::saveSettings(QSettings & settings)
{
  _filtersView->saveSettings(settings);
}

void FiltersPresenter::setInvalidFilter()
{
  _currentFilter.clear();
  _currentFilter.command = "skip";
  _currentFilter.previewCommand = "skip";
}

bool FiltersPresenter::isInvalidFilter() const
{
  return _currentFilter.isInvalid();
}

void FiltersPresenter::adjustViewSize()
{
  _filtersView->adjustTreeSize();
}

void FiltersPresenter::expandFaveFolder()
{
  _filtersView->expandFaveFolder();
}

void FiltersPresenter::expandPreviousSessionExpandedFolders()
{
  QList<QString> expandedFolderPaths = QSettings().value("Config/ExpandedFolders", QStringList()).toStringList();
  _filtersView->expandFolders(expandedFolderPaths);
}

void FiltersPresenter::expandAll()
{
  _filtersView->expandAll();
}

void FiltersPresenter::collapseAll()
{
  _filtersView->collapseAll();
}

void FiltersPresenter::removeSelectedFave()
{
  QString hash = _filtersView->selectedFilterHash();
  removeFave(hash);
}

void FiltersPresenter::editSelectedFaveName()
{
  _filtersView->editSelectedFaveName();
}

void FiltersPresenter::onFaveRenamed(QString hash, QString newName)
{
  Q_ASSERT_X(_favesModel.contains(hash), "onFaveRenamed()", "Hash not found");
  FavesModel::Fave fave = _favesModel.getFaveFromHash(hash);
  _favesModel.removeFave(hash);
  if (newName.isEmpty()) {
    if (_filtersModel.contains(fave.originalHash())) {
      const FiltersModel::Filter & originalFilter = _filtersModel.getFilterFromHash(fave.originalHash());
      newName = _favesModel.uniqueName(originalFilter.name(), QString());
    } else {
      newName = _favesModel.uniqueName("Unknown filter", QString());
    }
  } else {
    newName = _favesModel.uniqueName(newName, QString());
  }

  fave.setName(newName);
  fave.build();

  // Move parameters
  QList<QString> values = ParametersCache::getValues(hash);
  GmicQt::InputOutputState inOutState = ParametersCache::getInputOutputState(hash);
  ParametersCache::remove(hash);
  ParametersCache::setValues(fave.hash(), values);
  ParametersCache::setInputOutputState(fave.hash(), inOutState);

  _favesModel.addFave(fave);
  _filtersView->updateFaveItem(hash, fave.hash(), fave.name());
  _filtersView->sortFaves();
  saveFaves();
}

void FiltersPresenter::toggleSelectionMode(bool on)
{
  if (on) {
    _filtersView->enableSelectionMode();
  } else {
    _filtersView->disableSelectionMode();
  }
}

void FiltersPresenter::onFilterChanged(QString hash)
{
  setCurrentFilter(hash);
  emit filterSelectionChanged();
}

void FiltersPresenter::removeFave(QString hash)
{
  if (hash.isEmpty() || !_favesModel.contains(hash)) {
    return;
  }
  ParametersCache::remove(hash);
  _favesModel.removeFave(hash);
  _filtersView->removeFave(hash);
  saveFaves();
  onFilterChanged(_filtersView->selectedFilterHash());
}

void FiltersPresenter::setCurrentFilter(QString hash)
{
  if (hash.isEmpty()) {
    _currentFilter.clear();
  } else if (_favesModel.contains(hash)) {
    const FavesModel::Fave & fave = _favesModel.getFaveFromHash(hash);
    const QString & originalHash = fave.originalHash();
    if (_filtersModel.contains(originalHash)) {
      const FiltersModel::Filter & filter = _filtersModel.getFilterFromHash(originalHash);
      _currentFilter.command = fave.command();
      _currentFilter.defaultParameterValues = fave.defaultValues();
      _currentFilter.hash = hash;
      _currentFilter.isAFave = true;
      _currentFilter.name = fave.name();
      _currentFilter.plainTextName = fave.plainText();
      _currentFilter.parameters = filter.parameters();
      _currentFilter.previewCommand = fave.previewCommand();
      _currentFilter.isAccurateIfZoomed = filter.isAccurateIfZoomed();
      _currentFilter.previewFactor = filter.previewFactor();
    }
  } else if (_filtersModel.contains(hash)) {
    const FiltersModel::Filter & filter = _filtersModel.getFilterFromHash(hash);
    _currentFilter.command = filter.command();
    _currentFilter.defaultParameterValues = ParametersCache::getValues(hash);
    _currentFilter.hash = hash;
    _currentFilter.isAFave = false;
    _currentFilter.name = filter.name();
    _currentFilter.plainTextName = filter.plainText();
    _currentFilter.parameters = filter.parameters();
    _currentFilter.previewCommand = filter.previewCommand();
    _currentFilter.isAccurateIfZoomed = filter.isAccurateIfZoomed();
    _currentFilter.previewFactor = filter.previewFactor();
  } else {
    _currentFilter.clear();
  }
}

void FiltersPresenter::Filter::clear()
{
  name.clear();
  command.clear();
  previewCommand.clear();
  parameters.clear();
  defaultParameterValues.clear();
  hash.clear();
  plainTextName.clear();
  previewFactor = GmicQt::PreviewFactorAny;
  isAFave = false;
}

void FiltersPresenter::Filter::setInvalid()
{
  clear();
  command = "skip";
  previewCommand = "skip";
}

bool FiltersPresenter::Filter::isInvalid() const
{
  return hash.isEmpty() && (command == "skip") && (previewCommand == "skip");
}

bool FiltersPresenter::Filter::isNoApplyFilter() const
{
  return hash.isEmpty() || command.isEmpty() || (command == "_none_");
}

bool FiltersPresenter::Filter::isNoPreviewFilter() const
{
  return hash.isEmpty() || previewCommand.isEmpty() || (previewCommand == "_none_");
}

const char * FiltersPresenter::Filter::previewFactorString() const
{
  if (previewFactor == GmicQt::PreviewFactorActualSize) {
    return "ActualSize";
  }
  if (previewFactor == GmicQt::PreviewFactorAny) {
    return "Any";
  }
  if (previewFactor == GmicQt::PreviewFactorFullImage) {
    return "FullImage";
  }
  return "float value";
}
