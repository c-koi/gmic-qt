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

private:
  Ui::LanguageSelectionWidget *ui;
  QMap<QString,QString> _code2name;
};

#endif // LANGUAGESELECTIONWIDGET_H
