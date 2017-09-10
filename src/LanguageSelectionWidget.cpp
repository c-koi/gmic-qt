#include "LanguageSelectionWidget.h"
#include "ui_languageselectionwidget.h"
#include <QDebug>
#include <QLocale>
#include <QStringList>

LanguageSelectionWidget::LanguageSelectionWidget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::LanguageSelectionWidget)
{
  ui->setupUi(this);

  _code2name["en"] = QString("English");
  _code2name["cs"] = QString::fromUtf8("\xc4\x8c\x65\xc5\xa1\x74\x69\x6e\x61");
  _code2name["de"] = QString("Deutsch");
  _code2name["es"] = QString::fromUtf8("Espa\xc3\xb1ol");
  _code2name["fr"] = QString::fromUtf8("Fran\xc3\xa7""ais");
  _code2name["id"] = QString("bahasa Indonesia");
  _code2name["it"] = QString("Italiano");
  _code2name["ja"] = QString::fromUtf8("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e");
  _code2name["nl"] = QString("Dutch");
  _code2name["pl"] = QString::fromUtf8("J\xc4\x99zyk polski");
  _code2name["pt"] = QString::fromUtf8("Portugu\xc3\xaas");
  _code2name["ru"] = QString::fromUtf8("\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9");
  _code2name["ua"] = QString::fromUtf8("\xd0\xa3\xd0\xba\xd1\x80\xd0\xb0\xd1\x97\xd0\xbd\xd1\x81\xd1\x8c\xd0\xba\xd0\xb0");
  _code2name["zh"] = QString::fromUtf8("\xe7\xae\x80\xe5\x8c\x96\xe5\xad\x97");
  _code2name["zh_tw"] = QString::fromUtf8("\xe6\xad\xa3\xe9\xab\x94\xe5\xad\x97\x2f\xe7\xb9\x81\xe9\xab\x94\xe5\xad\x97");

  QMap<QString,QString>::iterator it = _code2name.begin();
  while (it != _code2name.end()) {
    qWarning() << it.key() << *it;
    ui->comboBox->addItem(*it,QVariant(it.key()));
    ++it;
  }

  QList<QString> languages = QLocale().uiLanguages();
  if ( languages.size() ) {
    QString lang = languages.front().split("-").front();
    if ( _code2name.find(lang) != _code2name.end() ) {
      // Check for traditional Chinese (following https://stackoverflow.com/a/4894634)
      if ( ( lang == "zh" ) && ( languages.front().endsWith("TW") || languages.front().endsWith("HK") ) ) {
        lang = "zh_tw";
      }
      ui->comboBox->insertItem(0,QString(tr("System default (%1)")).arg(_code2name[lang]),QVariant(lang));
    }
  }
}

LanguageSelectionWidget::~LanguageSelectionWidget()
{
  delete ui;
}
