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
#include <QString>
#include "Common.h"
#include "FilterSelector/FavesModelReader.h"
#include "FilterSelector/FavesModelWriter.h"
#include "FilterSelector/FiltersModelReader.h"
#include "FilterTextTranslator.h"
#include "FiltersVisibilityMap.h"
#include "Globals.h"
#include "GmicStdlib.h"
#include "HtmlTranslator.h"
#include "Logger.h"
#include "ParametersCache.h"
#include "Utils.h"
#include "Widgets/InOutPanel.h"

namespace GmicQt
{

FiltersPresenter::FiltersPresenter(QObject * parent) : QObject(parent)
{
  _filtersView = nullptr;
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
  connect(_filtersView, &FiltersView::filterSelected, this, &FiltersPresenter::onFilterChanged);
  connect(_filtersView, &FiltersView::faveRenamed, this, &FiltersPresenter::onFaveRenamed);
  connect(_filtersView, &FiltersView::faveRemovalRequested, this, &FiltersPresenter::removeFave);
  connect(_filtersView, &FiltersView::faveAdditionRequested, this, &FiltersPresenter::faveAdditionRequested);
}

void FiltersPresenter::rebuildFilterView()
{
  rebuildFilterViewWithSelection(QList<QString>());
}

void FiltersPresenter::rebuildFilterViewWithSelection(const QList<QString> & keywords)
{
  if (!_filtersView) {
    return;
  }
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
        QString message = QString("Fave '%1' has been relinked to filter '%2'").arg(fave.name()).arg(itFilter->name());
        Logger::log(message, "information", true);
        someFavesHaveBeenRelinked = true;
      } else {
        QString message = QString("Could not associate Fave '%1' to an existing filter").arg(fave.name());
        Logger::warning(message, true);
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

void FiltersPresenter::addSelectedFilterAsNewFave(const QList<QString> & defaultValues, const QList<int> & visibilityStates, InputOutputState inOutState)
{
  if (_currentFilter.hash.isEmpty() || (!_filtersModel.contains(_currentFilter.hash) && !_favesModel.contains(_currentFilter.hash))) {
    return;
  }
  FavesModel::Fave fave;
  fave.setDefaultValues(defaultValues);
  fave.setDefaultVisibilities(visibilityStates);

  bool filterAlreadyHasAFave = false;
  if (_filtersModel.contains(_currentFilter.hash)) {
    const FiltersModel::Filter & filter = _filtersModel.getFilterFromHash(_currentFilter.hash);
    fave.setName(_favesModel.uniqueName(FilterTextTranslator::translate(filter.name()), QString()));
    fave.setCommand(filter.command());
    fave.setPreviewCommand(filter.previewCommand());
    fave.setOriginalHash(filter.hash());
    fave.setOriginalName(filter.name());
    filterAlreadyHasAFave = filterExistsAsFave(filter.hash());
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
    filterAlreadyHasAFave = true;
  }

  fave.build();
  FiltersVisibilityMap::setVisibility(fave.hash(), true);
  _favesModel.addFave(fave);
  ParametersCache::setValues(fave.hash(), defaultValues);
  ParametersCache::setVisibilityStates(fave.hash(), visibilityStates);
  ParametersCache::setInputOutputState(fave.hash(), inOutState, _currentFilter.defaultInputMode);
  if (_filtersView) {
    _filtersView->addFave(fave.name(), fave.hash());
    _filtersView->sortFaves();
    _filtersView->selectFave(fave.hash());
  }
  saveFaves();
  onFilterChanged(fave.hash());
  if (filterAlreadyHasAFave) {
    editSelectedFaveName();
  }
}

void FiltersPresenter::applySearchCriterion(const QString & text)
{
  if (!_filtersView) {
    return;
  }
  static QString previousText;
  if ((!text.isEmpty() && previousText.isEmpty()) || (text.isEmpty() && previousText.isEmpty())) {
    _filtersView->preserveExpandedFolders();
  }
  QList<QString> keywords = text.split(QChar(' '), QT_SKIP_EMPTY_PARTS);

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
  bool hashExists = true;
  if (_filtersView) {
    if (_favesModel.contains(hash)) {
      _filtersView->selectFave(hash);
    } else if (_filtersModel.contains(hash)) {
      const FiltersModel::Filter & filter = _filtersModel.getFilterFromHash(hash);
      _filtersView->selectActualFilter(hash, filter.path());
    } else {
      hashExists = false;
    }
  }
  if (not hashExists) {
    hash.clear();
  }
  setCurrentFilter(hash);
  if (notify) {
    emit filterSelectionChanged();
  }
}

void FiltersPresenter::selectFilterFromPlainName(const QString & name)
{
  QString faveHash;
  auto itFave = _favesModel.findFaveFromPlainText(name);
  if (itFave != _favesModel.cend()) {
    faveHash = itFave->hash();
  }
  QStringList filterHashes;
  for (const FiltersModel::Filter & filter : _filtersModel) {
    if (filter.plainText() == name) {
      filterHashes.push_back(filter.hash());
    }
  }

  QString hash;
  if (((!faveHash.isEmpty()) + filterHashes.size()) == 1) {
    if (!faveHash.isEmpty()) {
      hash = faveHash;
      if (_filtersView) {
        _filtersView->selectFave(hash);
      }
    } else { // filterHashes.size() == 1
      hash = filterHashes.front();
      if (_filtersView) {
        _filtersView->selectFave(hash);
      }
    }
  }
  setCurrentFilter(hash);
}

void FiltersPresenter::selectFilterFromCommand(const QString & command)
{
  // We consider only the first matching filter
  for (const FiltersModel::Filter & filter : _filtersModel) {
    if (filter.command() == command) {
      setCurrentFilter(filter.hash());
      return;
    }
  }
  setCurrentFilter(QString());
}

void FiltersPresenter::selectFilterFromAbsolutePath(QString path)
{
  QString hash;
  if (path.startsWith("/")) {
    static const QString FaveFolderPrefix = "/" + HtmlTranslator::html2txt(FAVE_FOLDER_TEXT) + "/";
    if (path.startsWith(FaveFolderPrefix)) {
      path.remove(0, FaveFolderPrefix.length());
      auto it = _favesModel.findFaveFromPlainText(path);
      if (it != _favesModel.cend()) {
        hash = it->hash();
        if (_filtersView) {
          _filtersView->selectFave(hash);
        }
      }
    } else {
      auto it = _filtersModel.findFilterFromAbsolutePath(path);
      if (it != _filtersModel.cend()) {
        hash = it->hash();
        if (_filtersView) {
          _filtersView->selectActualFilter(hash, it->path());
        }
      }
    }
  }
  setCurrentFilter(hash);
}

void FiltersPresenter::selectFilterFromAbsolutePathOrPlainName(const QString & path)
{
  if (path.startsWith("/")) {
    selectFilterFromAbsolutePath(path);
  } else {
    selectFilterFromPlainName(path);
  }
}

const FiltersPresenter::Filter & FiltersPresenter::currentFilter() const
{
  return _currentFilter;
}

void FiltersPresenter::loadSettings(const QSettings & settings)
{
  if (_filtersView) {
    _filtersView->loadSettings(settings);
  }
}

void FiltersPresenter::saveSettings(QSettings & settings)
{
  if (_filtersView) {
    _filtersView->saveSettings(settings);
  }
}

void FiltersPresenter::setInvalidFilter()
{
  _currentFilter.setInvalid();
}

bool FiltersPresenter::isInvalidFilter() const
{
  return _currentFilter.isInvalid();
}

void FiltersPresenter::adjustViewSize()
{
  if (_filtersView) {
    _filtersView->adjustTreeSize();
  }
}

void FiltersPresenter::expandFaveFolder()
{
  if (_filtersView) {
    _filtersView->expandFaveFolder();
  }
}

void FiltersPresenter::expandPreviousSessionExpandedFolders()
{
  if (_filtersView) {
    QList<QString> expandedFolderPaths = QSettings().value("Config/ExpandedFolders", QStringList()).toStringList();
    _filtersView->expandFolders(expandedFolderPaths);
  }
}

void FiltersPresenter::expandAll()
{
  if (_filtersView) {
    _filtersView->expandAll();
  }
}

void FiltersPresenter::collapseAll()
{
  if (_filtersView) {
    _filtersView->collapseAll();
  }
}

const QString & FiltersPresenter::errorMessage() const
{
  return _errorMessage;
}

FiltersPresenter::Filter FiltersPresenter::findFilterFromAbsolutePathOrNameInStdlib(const QString & path)
{
  FiltersPresenter presenter(nullptr);
  presenter.readFaves();
  presenter.readFilters();
  if (path.startsWith("/")) {
    presenter.selectFilterFromAbsolutePath(path);
  } else {
    presenter.selectFilterFromPlainName(path);
  }
  return presenter.currentFilter();
}

FiltersPresenter::Filter FiltersPresenter::findFilterFromCommandInStdlib(const QString & command)
{
  FiltersPresenter presenter(nullptr);
  // presenter.readFaves();
  presenter.readFilters();
  presenter.selectFilterFromCommand(command);
  return presenter.currentFilter();
}

void FiltersPresenter::removeSelectedFave()
{
  if (_filtersView) {
    QString hash = _filtersView->selectedFilterHash();
    removeFave(hash);
  }
}

void FiltersPresenter::editSelectedFaveName()
{
  if (_filtersView) {
    _filtersView->editSelectedFaveName();
  }
}

void FiltersPresenter::onFaveRenamed(const QString & hash, const QString & name)
{
  Q_ASSERT_X(_favesModel.contains(hash), "onFaveRenamed()", "Hash not found");
  FavesModel::Fave fave = _favesModel.getFaveFromHash(hash);
  _favesModel.removeFave(hash);

  InputMode defaultInputMode = InputMode::Unspecified;
  if (_filtersModel.contains(fave.originalHash())) {
    const FiltersModel::Filter & originalFilter = _filtersModel.getFilterFromHash(fave.originalHash());
    defaultInputMode = originalFilter.defaultInputMode();
  }

  QString newName = name;
  if (newName.isEmpty()) {
    if (_filtersModel.contains(fave.originalHash())) {
      const FiltersModel::Filter & originalFilter = _filtersModel.getFilterFromHash(fave.originalHash());
      newName = _favesModel.uniqueName(FilterTextTranslator::translate(originalFilter.name()), QString());
    } else {
      newName = _favesModel.uniqueName(tr("Unknown filter"), QString());
    }
  } else {
    newName = _favesModel.uniqueName(newName, QString());
  }
  fave.setName(newName);
  fave.build();

  // Move parameters
  QList<QString> values = ParametersCache::getValues(hash);
  QList<int> visibilityStates = ParametersCache::getVisibilityStates(hash);
  InputOutputState inOutState = ParametersCache::getInputOutputState(hash);
  ParametersCache::remove(hash);
  ParametersCache::setValues(fave.hash(), values);
  ParametersCache::setVisibilityStates(fave.hash(), visibilityStates);
  ParametersCache::setInputOutputState(fave.hash(), inOutState, defaultInputMode);

  _favesModel.addFave(fave);
  if (_filtersView) {
    _filtersView->updateFaveItem(hash, fave.hash(), fave.name());
    _filtersView->sortFaves();
  }
  saveFaves();
  setCurrentFilter(fave.hash());
  emit faveNameChanged(newName);
}

void FiltersPresenter::toggleSelectionMode(bool on)
{
  if (_filtersView) {
    if (on) {
      _filtersView->enableSelectionMode();
    } else {
      _filtersView->disableSelectionMode();
    }
  }
}

void FiltersPresenter::onFilterChanged(const QString & hash)
{
  setCurrentFilter(hash);
  emit filterSelectionChanged();
}

void FiltersPresenter::removeFave(const QString & hash)
{
  if (hash.isEmpty() || !_favesModel.contains(hash)) {
    return;
  }
  ParametersCache::remove(hash);
  _favesModel.removeFave(hash);
  if (_filtersView) {
    _filtersView->removeFave(hash);
  }
  saveFaves();
  if (_filtersView) {
    onFilterChanged(_filtersView->selectedFilterHash());
  }
}

bool FiltersPresenter::danglingFaveIsSelected() const
{
  if (not _filtersView || not _filtersView->aFaveIsSelected()) {
    return false;
  }
  QString hash = _filtersView->selectedFilterHash();
  if (_favesModel.contains(hash)) {
    return not _filtersModel.contains(_favesModel.getFaveFromHash(hash).originalHash());
  }
  return false;
}

void FiltersPresenter::setCurrentFilter(const QString & hash)
{
  _errorMessage.clear();
  if (hash.isEmpty()) {
    _currentFilter.setInvalid();
  } else if (_favesModel.contains(hash)) {
    const FavesModel::Fave & fave = _favesModel.getFaveFromHash(hash);
    const QString & originalHash = fave.originalHash();
    if (_filtersModel.contains(originalHash)) {
      const FiltersModel::Filter & filter = _filtersModel.getFilterFromHash(originalHash);
      _currentFilter.command = fave.command();
      _currentFilter.defaultParameterValues = fave.defaultValues();
      _currentFilter.defaultVisibilityStates = fave.defaultVisibilityStates();
      _currentFilter.defaultInputMode = filter.defaultInputMode();
      _currentFilter.hash = hash;
      _currentFilter.isAFave = true;
      _currentFilter.name = fave.name();
      _currentFilter.plainTextName = fave.plainText();
      _currentFilter.fullPath = fave.absolutePath();
      _currentFilter.parameters = filter.parameters();
      _currentFilter.previewCommand = fave.previewCommand();
      _currentFilter.isAccurateIfZoomed = filter.isAccurateIfZoomed();
      _currentFilter.previewFactor = filter.previewFactor();
    } else {
      setInvalidFilter();
      _errorMessage = tr("Cannot find this fave's original filter\n");
    }
  } else if (_filtersModel.contains(hash)) {
    const FiltersModel::Filter & filter = _filtersModel.getFilterFromHash(hash);
    _currentFilter.command = filter.command();
    _currentFilter.defaultParameterValues = ParametersCache::getValues(hash); // FIXME : Unused unless it's a fave. Should be renamed.
    _currentFilter.defaultVisibilityStates = ParametersCache::getVisibilityStates(hash);
    _currentFilter.defaultInputMode = filter.defaultInputMode();
    _currentFilter.hash = hash;
    _currentFilter.isAFave = false;
    _currentFilter.name = filter.name();
    _currentFilter.plainTextName = filter.plainText();
    _currentFilter.fullPath = filter.absolutePathNoTags();
    _currentFilter.parameters = filter.parameters();
    _currentFilter.previewCommand = filter.previewCommand();
    _currentFilter.isAccurateIfZoomed = filter.isAccurateIfZoomed();
    _currentFilter.previewFactor = filter.previewFactor();
  } else {
    _currentFilter.setInvalid();
  }
}

bool FiltersPresenter::filterExistsAsFave(const QString filterHash)
{
  for (const FavesModel::Fave & fave : _favesModel) {
    if (fave.originalHash() == filterHash) {
      return true;
    }
  }
  return false;
}

void FiltersPresenter::Filter::clear()
{
  name.clear();
  command.clear();
  previewCommand.clear();
  parameters.clear();
  defaultParameterValues.clear();
  fullPath.clear();
  hash.clear();
  plainTextName.clear();
  previewFactor = PreviewFactorAny;
  defaultInputMode = InputMode::Unspecified;
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

bool FiltersPresenter::Filter::isValid() const
{
  return !isInvalid();
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
  if (previewFactor == PreviewFactorActualSize) {
    return "ActualSize";
  }
  if (previewFactor == PreviewFactorAny) {
    return "Any";
  }
  if (previewFactor == PreviewFactorFullImage) {
    return "FullImage";
  }
  return "float value";
}

} // namespace GmicQt
