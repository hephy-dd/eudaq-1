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
  void on_cbSfout_stateChanged(int arg1);
  void on_cbHb_stateChanged(int arg1);
  void on_cbInj_stateChanged(int arg1);
  void on_sbTdac_valueChanged(int arg1);
  void on_cbManOverride_stateChanged(int arg1);
  void on_cbMasked_stateChanged(int arg1);
  void on_pbInit_clicked();
  void on_pbParse_clicked();
  void on_pbDeploy_clicked();
  void on_pbClearLog_clicked();

  void pixelChosen(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                   const QVector<int> &roles = QVector<int>());

private:
  struct ConfigPowerItem {
    ConfigPowerItem() {
      this->voltage = 0.0;
      this->current = 0.0;
    }
    ConfigPowerItem(double v, double i = 3.0) {
      this->voltage = v;
      this->current = i;
    }

    double voltage;
    double current;
  };

  struct ConfigPixel {
    bool masked = false;
    bool inj = false;
    bool hb = false;
    bool sfout = false;
    int tdac = -1; // -1 means read from calibration file
  };
  struct Pixel {
    int row = -1;
    int col = -1;
    // is needed by QList::remove
    inline bool operator==(const Pixel &other) {
      return this->col == other.col && this->row == other.row;
    };
  };

  enum FileSrc { Local, SSH };

  Ui::ConfigCreator *ui;

  const int nCol;
  const int nRow;

  QMap<QString, QVariant> mConfigMisc;
  QMap<QString, ConfigPowerItem> mConfigPower;
  QStandardItemModel mModelPower;
  QStandardItemModel mModelMisc;
  QStandardItemModel mModelMatrix;
  QList<QList<ConfigPixel *>> mConfigMatrix;
  QList<Pixel> mPixelToModify;

  void parseConfig(const QString &pathToConfig);
  void saveConfig(const QString &fileName);
  void initConfig();
  void initPixelMatrix();
  void populateModels();
  void updatePixelInputs();
  void saveMatrixConfig(const QString &fileName);
  void loadMatrixConfig(const QString &fileName);
  bool deployViaSsh(const QString &localFile, const QString &server,
                    const QString &targetPath, const QString &targetFile);
  bool fetchViaSsh(const QString &localFile, const QString &server,
                   const QString &srcPath, const QString &srcFile);
  ConfigPixel *pixelConfig(int row, int col);
  ConfigPixel *pixelConfig(const Pixel &pix);
  FileSrc fileSrcFromInput(QString input, QString *server, QString *filepath,
                           QString *filename);
};

#endif // CONFIGCREATOR_H
