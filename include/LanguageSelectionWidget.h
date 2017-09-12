#ifndef LANGUAGESELECTIONWIDGET_H
#define LANGUAGESELECTIONWIDGET_H

#include <QWidget>
#include <QMap>
#include <QString>

namespace Ui {
class LanguageSelectionWidget;
}

class LanguageSelectionWidget : public QWidget
{
  Q_OBJECT
public:
  explicit LanguageSelectionWidget(QWidget *parent = 0);
  ~LanguageSelectionWidget();
  QString selectedLanguageCode();

  static const QMap<QString,QString> & availableLanguages();
  static QString configuredTranslator();
  static QString systemDefaultAndAvailableLanguageCode();

public slots:
  void selectLanguage(const QString & code);

private:
  Ui::LanguageSelectionWidget *ui;
  const QMap<QString,QString> & _code2name;
  bool _systemDefaultIsAvailable;
};

#endif // LANGUAGESELECTIONWIDGET_H
