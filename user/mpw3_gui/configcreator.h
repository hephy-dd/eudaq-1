#ifndef CONFIGCREATOR_H
#define CONFIGCREATOR_H

#include <QStandardItemModel>
#include <QWidget>

namespace Ui {
class ConfigCreator;
}

class ConfigCreatorView : public QWidget {
  Q_OBJECT

public:
  explicit ConfigCreatorView(QWidget *parent = nullptr);
  ~ConfigCreatorView();

private slots:
  void on_pbParse_clicked();
  void on_pbDeploy_clicked();
  void on_pbClearLog_clicked();

private:
  struct ConfigItem {
    QString key;
    QString value;
    bool enabled;
    bool isComment;
  };
  enum ColumnRole { Item, Comment };
  enum FileSrc { Local, SSH };

  Ui::ConfigCreator *ui;

  QStandardItemModel mModel;
  QList<ConfigItem> mItems;

  void parseConfig(const QString &pathToConfig);
  void saveConfig(const QString &fileName);
  void setupView();
  FileSrc fileSrcFromInput(QString &input);
};

#endif // CONFIGCREATOR_H
