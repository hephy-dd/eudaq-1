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
  void on_pbInit_clicked();
  void on_pbParse_clicked();
  void on_pbDeploy_clicked();
  void on_pbClearLog_clicked();
  void on_pbMisc_toggled(bool checked);
  void on_pbPower_toggled(bool checked);

private:
  struct ConfigItem {
    QString key;
    QString value;
    bool enabled;
    bool isComment;
  };

  struct ConfigPowerItem {
    ConfigPowerItem() {
      voltage = 0.0;
      current = 0.0;
    }
    ConfigPowerItem(double v, double i = 3.0) {
      voltage = v;
      current = i;
    }
    double voltage;
    double current;
  };

  enum ColumnRole { Item, Comment };
  enum FileSrc { Local, SSH };

  Ui::ConfigCreator *ui;

  QMap<QString, QVariant> mConfigMisc;
  QMap<QString, ConfigPowerItem> mConfigPower;
  QStandardItemModel mModelPower;
  QStandardItemModel mModelMisc;
  QList<ConfigItem> mItems;

  void parseConfig(const QString &pathToConfig);
  void saveConfig(const QString &fileName);
  void initConfig();
  void populateModels();
  FileSrc fileSrcFromInput(QString &input);
};

#endif // CONFIGCREATOR_H
