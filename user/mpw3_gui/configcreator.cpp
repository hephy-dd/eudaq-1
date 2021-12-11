#include "configcreator.h"
#include "ui_configcreator.h"

#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QtDebug>
#include <memory>

ConfigCreatorView::ConfigCreatorView(QWidget *parent)
    : QWidget(parent), ui(new Ui::ConfigCreator) {
  ui->setupUi(this);

  ui->tvPower->setModel(&mModelPower);
  ui->tvMisc->setModel(&mModelMisc);

  initConfig();
  populateModels();
}

ConfigCreatorView::~ConfigCreatorView() { delete ui; }

void ConfigCreatorView::on_pbParse_clicked() {
  int fileSrc = 0;
  parseConfig(ui->leTemplateFile->text());
  //  setupView();
}

void ConfigCreatorView::parseConfig(const QString &pathToConfig) {
  QString filename;
  const QString tmpConfigFile = "/tmp/tmpconf.cfg";
  filename = pathToConfig;

  auto fileSrc = fileSrcFromInput(filename);

  if (fileSrc == FileSrc::Local) {
    // nothing particular to do, we are alrdy set up properly
  } else if (fileSrc == FileSrc::SSH) {
    QString scpCmd = "scp " + filename.trimmed() + " " + tmpConfigFile;
    if (system(scpCmd.toLocal8Bit().data()) !=
        0) { // calling external process scp to copy remote file to our drive
      ui->tbLog->append("Error copying config file to: " + tmpConfigFile);
      return;
    }
    filename = tmpConfigFile;
  }

  mItems.clear();
  QFile cfgFile(filename);
  QTextStream input(&cfgFile);
  if (!cfgFile.open(QIODevice::ReadOnly)) {
    ui->tbLog->append("Error opening input file");
    return;
  }
}

void ConfigCreatorView::saveConfig(const QString &fileName) {

  QFile file(fileName);
  const QString secComm = " Line needed for parsing, do not change!";
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream out(&file);
    out << "[RD50_MPW3]\n";
    out << "#SEC::MISC" << secComm << "\n";
    for (int i = 0; i < mModelMisc.rowCount(); i++) {
      out << mModelMisc.item(i, 0)->text() << " = "
          << mModelMisc.item(i, 1)->text() << "\n";
    }

    out << "\n\n#SEC::POWER" << secComm << "\n";
    out << "# voltages with suffix \"_v\", currents with "
           "\"_i\"\n";
    for (int i = 0; i < mModelPower.rowCount(); i++) {
      out << mModelPower.item(i, 0)->text() + "_v = "
          << mModelPower.item(i, 1)->text() << "\n";
      out << mModelPower.item(i, 0)->text() + "_i = "
          << mModelPower.item(i, 2)->text() << "\n";
    }

  } else {
    ui->tbLog->append("Error opening file " + fileName);
  }
}

void ConfigCreatorView::initConfig() {

  mConfigMisc.clear();
  mConfigPower.clear();

  mConfigMisc["i2c_dev"] = "\\dev\\i2c-13";
  mConfigMisc["i2c_addr"] = 0x1C;

  mConfigPower["vssa"] = ConfigPowerItem(1.3);
  mConfigPower["vdda"] = ConfigPowerItem(1.8);
  mConfigPower["vddc"] = ConfigPowerItem(1.8);
  mConfigPower["vddd"] = ConfigPowerItem(1.8);
  mConfigPower["vnwring"] = ConfigPowerItem(1.8);
  mConfigPower["vddio"] = ConfigPowerItem(1.8);
  mConfigPower["vsensbias"] = ConfigPowerItem(1.8);
  mConfigPower["vddbg"] = ConfigPowerItem(1.8);
  mConfigPower["th"] = ConfigPowerItem(.95);
  mConfigPower["bl"] = ConfigPowerItem(.9);
  mConfigPower["vn_ext"] = ConfigPowerItem(.616);
  mConfigPower["vncasc_ext"] = ConfigPowerItem(.926);
  mConfigPower["vnsf_ext"] = ConfigPowerItem(.418);
  mConfigPower["vptrim_ext"] = ConfigPowerItem(1.238);
  mConfigPower["vpcomp_ext"] = ConfigPowerItem(1.23);
  mConfigPower["vsensbias_ext"] = ConfigPowerItem(1.309);
  mConfigPower["vblr_ext"] = ConfigPowerItem(1.203);
  mConfigPower["vnfb_cont_ext"] = ConfigPowerItem(.584);
  mConfigPower["vpfb_sw_ext"] = ConfigPowerItem(1.279);
  mConfigPower["vpbias_ext"] = ConfigPowerItem(1.025);
  mConfigPower["sfoutbuff_thr_ext"] = ConfigPowerItem(.050);
}

void ConfigCreatorView::populateModels() {
  mModelMisc.clear();
  mModelPower.clear();
  auto miscKeys = mConfigMisc.keys();

  QStringList header;
  header << "key"
         << "value";
  mModelMisc.setHorizontalHeaderLabels(header);

  QList<QStandardItem *> row;
  foreach (auto key, miscKeys) {
    auto name = new QStandardItem(key);
    auto value = new QStandardItem(mConfigMisc.value(key).toString());
    row << name << value;

    mModelMisc.appendRow(row);
    row.clear();
  }

  auto powerKeys = mConfigPower.keys();
  header.clear();
  header << "power"
         << "U [V]"
         << "I_max [A]";
  mModelPower.setHorizontalHeaderLabels(header);

  foreach (auto key, powerKeys) {
    auto name = new QStandardItem(key);
    auto v =
        new QStandardItem(QString::number(mConfigPower.value(key).voltage));
    auto i =
        new QStandardItem(QString::number(mConfigPower.value(key).current));
    row << name << v << i;

    mModelPower.appendRow(row);
    row.clear();
  }
}

ConfigCreatorView::FileSrc ConfigCreatorView::fileSrcFromInput(QString &input) {
  if (input.startsWith("ssh://")) {
    input = input.remove("ssh://");
    return FileSrc::SSH;
  }

  return FileSrc::Local;
}

void ConfigCreatorView::on_pbDeploy_clicked() {

  QString outputFile;
  const QString tmpConfig = "/tmp/configout.cfg";
  QString target = ui->leOutputFile->text();
  auto fileSrc = fileSrcFromInput(target);
  if (fileSrc == FileSrc::Local) {
    outputFile = target;
  } else if (fileSrc == FileSrc::SSH) {
    outputFile = tmpConfig;
  }
  saveConfig(outputFile);

  if (fileSrc == FileSrc::SSH) {
    QString scpCmd = "scp " + tmpConfig + " " + target;
    if (system((scpCmd.toLocal8Bit().data())) != 0) {
      ui->tbLog->append("Error deploying file: " + tmpConfig +
                        "\n to: " + ui->leOutputFile->text());
    }
  }
}

void ConfigCreatorView::on_pbClearLog_clicked() { ui->tbLog->clear(); }

void ConfigCreatorView::on_pbPower_toggled(bool checked) {
  ui->tvPower->setVisible(checked);
}

void ConfigCreatorView::on_pbMisc_toggled(bool checked) {
  ui->tvMisc->setVisible(checked);
}

void ConfigCreatorView::on_pbInit_clicked() {
  if (QMessageBox::question(this, "Initializing",
                            "You are about to reset all values to default.\n"
                            "Possible changes will be lost!\n"
                            "Do you want to continue?") == QMessageBox::No) {
    return;
  }
  initConfig();
  populateModels();
}
