#include "configcreator.h"
#include "ui_configcreator.h"

#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QtDebug>
#include <memory>

ConfigCreatorView::ConfigCreatorView(QWidget *parent)
    : QWidget(parent), ui(new Ui::ConfigCreator), nCol(64), nRow(64) {
  ui->setupUi(this);

  ui->tvPower->setModel(&mModelPower);
  ui->tvMisc->setModel(&mModelMisc);

  initConfig();
  populateModels();

  ui->sbCol->setMaximum(nCol - 1);
  ui->sbRow->setMaximum(nRow - 1); // start counting from 0
  ui->sbTdac->setMaximum(15);
  ui->sbTdac->setMinimum(0);
}

ConfigCreatorView::~ConfigCreatorView() { delete ui; }

void ConfigCreatorView::on_pbParse_clicked() {
  mConfigMisc.clear();
  mConfigPower.clear();
  parseConfig(ui->leTemplateFile->text());
  populateModels();
}

void ConfigCreatorView::parseConfig(const QString &pathToConfig) {
  QString pearyConfigFile, server, srcPath, srcFile;
  const QString tmpPearyConfigFile = "/tmp/tmpconf.cfg";
  const QString tmpMatrixConfig = "/tmp/matrix.cfg";

  auto fileSrc = fileSrcFromInput(pathToConfig, &server, &srcPath, &srcFile);

  if (fileSrc == FileSrc::Local) {
    // nothing particular to do, we are alrdy set up properly
    pearyConfigFile = srcFile;
  } else if (fileSrc == FileSrc::SSH) {
    if (!fetchViaSsh(tmpPearyConfigFile, server, srcPath, srcFile)) {
      ui->tbLog->append("Error copying file using scp from " + server +
                        srcPath + srcFile);
      return;
    }
    pearyConfigFile = tmpPearyConfigFile;
  }

  mItems.clear();
  QFile cfgFile(pearyConfigFile);
  QTextStream input(&cfgFile);
  if (!cfgFile.open(QIODevice::ReadOnly)) {
    ui->tbLog->append("Error opening input file");
    return;
  }

  int stage = -1;

  while (!input.atEnd()) {
    auto line = input.readLine();
    if (line.startsWith("#SEC::MISC")) {
      stage = 0;
      continue;
    }
    if (line.startsWith("#SEC::POWER")) {
      stage = 1;
      continue;
    }

    if (line.startsWith("#") || line.startsWith("[") || line.isEmpty()) {
      /* is either comment line, or device specifier, or an empty line
       */
      continue;
    }

    auto splitted = line.split("=");
    if (splitted.length() != 2) {
      ui->tbLog->append("invalid config line :\"" + line + "\"");
      continue;
    }
    splitted[0] = splitted[0].trimmed();
    splitted[1] = splitted[1].trimmed(); // remove white space
    if (stage == -1) {
      continue;
    } else if (stage == 0) {
      // currently we are working in the MISC section

      mConfigMisc[splitted[0]] = splitted[1];
    } else if (stage == 1) {

      bool ok = true;

      if (splitted[0].endsWith("__u")) {
        auto withoutSuffix = splitted[0].remove("__u");
        auto val = mConfigPower.value(withoutSuffix);

        val.voltage = splitted[1].toDouble(&ok);
        // keep possible already parsed __i for curent power item
        // only set the voltage
        mConfigPower[withoutSuffix] =
            val; // item will be overwritten if alrdy in config

      } else if (splitted[0].endsWith("__i")) {
        auto withoutSuffix = splitted[0].remove("__i");
        auto val = mConfigPower.value(withoutSuffix);

        val.current = splitted[1].toDouble(&ok);
        // keep possible already parsed __v for curent power item
        // only set the current
        mConfigPower[withoutSuffix] =
            val; // item will be overwritten if alrdy in config
      } else {
        ui->tbLog->append("Invalid power item suffix in item\"" + splitted[0] +
                          "\"");
      }
      if (!ok) {
        ui->tbLog->append("Error converting value: " + splitted[1]);
      }
    }
  }
  srcFile = mConfigMisc.value("matrix_config").toString();
  if (fileSrc == FileSrc::Local) {
    // nothing particular to do, we are alrdy set up properly
    loadMatrixConfig(srcFile);
  } else if (fileSrc == FileSrc::SSH) {
    if (!fetchViaSsh(tmpMatrixConfig, server, srcPath, srcFile)) {
      ui->tbLog->append("Error copying file using scp from " + server +
                        srcPath + srcFile);
      return;
    }
    loadMatrixConfig(tmpMatrixConfig);
  }
}

void ConfigCreatorView::saveConfig(const QString &fileName) {

  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream out(&file);

    out << "#This config file was generated with the \"MPW3_gui - Config "
           "Creator\"\n"
        << "#Do not change comment lines like \"SEC::xxx\" !\n"
        << "#They are needed for parsing\n\n";

    out << "[RD50_MPW3]\n"; // needed by peary to identify correct device
    out << "#SEC::MISC\n";
    for (int i = 0; i < mModelMisc.rowCount(); i++) {
      out << mModelMisc.item(i, 0)->text() << " = "
          << mModelMisc.item(i, 1)->text() << "\n";
    }

    out << "\n\n#SEC::POWER\n";
    out << "# voltages with suffix \"__u\", currents with \"__i\"\n";
    for (int i = 0; i < mModelPower.rowCount(); i++) {
      out << mModelPower.item(i, 0)->text() + "__u = "
          << mModelPower.item(i, 1)->text() << "\n";
      out << mModelPower.item(i, 0)->text() + "__i = "
          << mModelPower.item(i, 2)->text() << "\n";
    }

  } else {
    ui->tbLog->append("Error opening file " + fileName);
  }
}

void ConfigCreatorView::initConfig() {

  mConfigMisc.clear();
  mConfigPower.clear();

  mConfigMisc["i2c_dev"] = "/dev/i2c-13";
  mConfigMisc["i2c_addr"] = 0x1C;
  mConfigMisc["clock_config"] = "clock_config.txt";
  mConfigMisc["matrix_config"] = "matrix_config.txt";

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

  initPixelMatrix();
}

void ConfigCreatorView::initPixelMatrix() {
  mConfigMatrix.clear();
  for (int i = 0; i < nRow; i++) {
    QList<ConfigPixel *> row;
    for (int j = 0; j < nCol; j++) {
      row.append(new ConfigPixel());
    }
    mConfigMatrix.append(row);
  }

  setPixelToModify();
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
    name->setEditable(false);
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
    name->setEditable(false);
    auto v =
        new QStandardItem(QString::number(mConfigPower.value(key).voltage));
    auto i =
        new QStandardItem(QString::number(mConfigPower.value(key).current));
    row << name << v << i;

    mModelPower.appendRow(row);
    row.clear();
  }
}

void ConfigCreatorView::updatePixelInputs() {
  auto pix = pixelConfig(mPixelToModify[0]);

  ui->cbMasked->setChecked(pix->masked);
  ui->cbInj->setChecked(pix->inj);
  ui->cbHb->setChecked(pix->hb);
  ui->cbSfout->setChecked(pix->sfout);
  if (pix->tdac != -1) {
    // someone insisted on specifying a manual TDAC value
    ui->cbManOverride->setChecked(true);
    ui->sbTdac->setValue(pix->tdac);
  } else {
    ui->sbTdac->clear();
  }
}

void ConfigCreatorView::setPixelToModify() {
  Pixel pix;
  pix.col = ui->sbCol->value();
  pix.row = ui->sbRow->value();
  mPixelToModify.clear();
  mPixelToModify << pix;
}

void ConfigCreatorView::saveMatrixConfig(const QString &fileName) {
  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream out(&file);
    out << "# Config-File of the Pixel-Matrix; generated by "
           "MPW3-Config-Creator\n"
        << "# each line represents the configuration of an individual pixel\n"
        << "# format: {col} {row} {mask} {en_inj} {hb_en} {en_sfout} {TDAC}\n\n"
        << "# default values (which are not printed) are:\n"
        << "# {col} {row} 0 0 0 0 -1\n";
    for (int i = 0; i < nCol; i++) {
      for (int j = 0; j < nRow; j++) {
        auto pixel = pixelConfig(j, i);
        if (pixel->hb || pixel->inj || pixel->masked || pixel->tdac != -1) {

          // this pixel is not configured the default way, we need to store it

          out << QStringLiteral("%1").arg(i, 2, 10, QLatin1Char('0')) << " "
              << QStringLiteral("%1").arg(j, 2, 10, QLatin1Char('0')) << " "
              << pixel->masked << " " << pixel->inj << " " << pixel->hb << " "
              << pixel->sfout << " "
              << QStringLiteral("%1").arg(pixel->tdac, 2, 10, QLatin1Char('0'))
              << "\n";
          // QStringLiteral stuff for filling with leading zeros
        }
      }
    }

  } else {
    ui->tbLog->append("ERROR opening file " + fileName);
  }
}

void ConfigCreatorView::loadMatrixConfig(const QString &fileName) {

  QFile file(fileName);
  if (file.open(QIODevice::ReadOnly)) {
    initPixelMatrix();

    while (!file.atEnd()) {
      bool ok = true;
      auto line = file.readLine();
      line = line.trimmed(); // remove whitespace
      if (line.startsWith("#") || line.isEmpty()) {
        continue; // is a comment or an empty line
      }
      auto splitted = line.split(' ');
      if (splitted.length() != 7) {
        ui->tbLog->append(
            "ERROR: invalid number of items in matrix config line: \n" + line);
        continue;
      }
      ConfigPixel pixel;
      int col = splitted[0].toInt(&ok);
      int row = splitted[1].toUInt(&ok);
      pixel.masked = splitted[2].toInt(&ok);
      pixel.inj = splitted[3].toInt(&ok);
      pixel.hb = splitted[4].toInt(&ok);
      pixel.sfout = splitted[5].toInt(&ok);
      pixel.tdac = splitted[6].toInt(&ok);
      if (!ok) {
        ui->tbLog->append(
            "ERROR: converting strings to number ins matrix config line: \n" +
            line);
        continue;
      }
      *pixelConfig(row, col) = pixel;
    }
  }
}

bool ConfigCreatorView::deployViaSsh(const QString &localFile,
                                     const QString &server,
                                     const QString &targetPath,
                                     const QString &targetFile) {
  QString scpCmd = "scp " + localFile.trimmed() + " " + server.trimmed() +
                   targetPath.trimmed() + targetFile.trimmed();

  return system((scpCmd.toLocal8Bit().data())) == 0;
}

bool ConfigCreatorView::fetchViaSsh(const QString &localFile,
                                    const QString &server,
                                    const QString &srcPath,
                                    const QString &srcFile) {
  QString scpCmd = "scp " + server.trimmed() + srcPath.trimmed() +
                   srcFile.trimmed() + " " + localFile.trimmed();

  return system((scpCmd.toLocal8Bit().data())) == 0;
}

ConfigCreatorView::ConfigPixel *ConfigCreatorView::pixelConfig(int row,
                                                               int col) {
  Pixel pix;
  pix.col = col;
  pix.row = row;
  return pixelConfig(pix);
}

ConfigCreatorView::ConfigPixel *
ConfigCreatorView::pixelConfig(const Pixel &pix) {
  return mConfigMatrix[pix.row][pix.col];
}
/**
 * @brief extracts different path layers from user input
 * @param input user input from UI
 * @param server the remote server to deploy to, is an output
 * @param target the filename to which stuff should be stored to, is an output
 * @return deployment target: local file or remote host
 */
ConfigCreatorView::FileSrc
ConfigCreatorView::fileSrcFromInput(QString input, QString *server,
                                    QString *filepath, QString *filename) {

  QRegularExpression regexp("(.+//)(.+:)(.+)");
  /*
   * 1st group matches something like "ssh://"
   * 2nd one "192.168.130.7:"
   * 3rd one "peary/config.cfg"
   */
  auto match = regexp.match(input);
  if (match.hasMatch()) {
    // captured(0) is the global match
    // captured(i) accesses the different groups
    if (match.captured(1) == "ssh://") {

      *server = match.captured(2);
      *filepath = match.captured(3);
      regexp.setPattern(".+/"); // now seperate path and filename
      // would match "/tmp/" in "/tmp/test.cfg"
      match = regexp.match(*filepath);
      *filename = filepath->remove(regexp);
      *filepath = match.captured();

      return FileSrc::SSH;
    }
  }
  regexp.setPattern(".+/"); // now seperate path and filename
  // would match "/tmp/" in "/tmp/test.cfg"
  match = regexp.match(input);
  *filepath = match.captured();
  *filename = input.remove(regexp);
  return FileSrc::Local;
}

void ConfigCreatorView::on_pbDeploy_clicked() {

  QString server, targetPath, targetFile;
  const QString tmpConfigPeary = "/tmp/configout.cfg";
  const QString tmpMatrixConfig = "/tmp/matrix.cfg";
  auto fileSrc = fileSrcFromInput(ui->leOutputFile->text(), &server,
                                  &targetPath, &targetFile);
  if (fileSrc == FileSrc::Local) {
    saveConfig(targetPath + targetFile);
    saveMatrixConfig(targetPath +
                     mConfigMisc.value("matrix_config").toString());
  } else if (fileSrc == FileSrc::SSH) {
    saveConfig(tmpConfigPeary);
    if (!deployViaSsh(tmpConfigPeary, server, targetPath, targetFile)) {
      ui->tbLog->append("Error deploying peary config file " + tmpConfigPeary +
                        " to " + server + ":" + targetPath + targetFile);
    }

    saveMatrixConfig(tmpMatrixConfig);

    // we store the matrix config in the same path as the peary config but use
    // the "matrix_config" item in the peary config for the filename on the
    // remote host

    targetFile = mConfigMisc.value("matrix_config").toString();
    if (!deployViaSsh(tmpMatrixConfig, server, targetPath, targetFile)) {
      ui->tbLog->append("Error deploying peary config file " +
                        mConfigMisc.value("matrix_config").toString() + " to " +
                        server + ":" + targetPath + targetFile);
    }
  }
}

void ConfigCreatorView::on_pbClearLog_clicked() { ui->tbLog->clear(); }

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

void ConfigCreatorView::on_cbMasked_stateChanged(int arg1) {
  foreach (auto pix, mPixelToModify) { pixelConfig(pix)->masked = bool(arg1); }
}

void ConfigCreatorView::on_sbRow_valueChanged(int arg1) {

  setPixelToModify();
  updatePixelInputs();
}

void ConfigCreatorView::on_sbCol_valueChanged(int arg1) {

  setPixelToModify();
  updatePixelInputs();
}

void ConfigCreatorView::on_cbManOverride_stateChanged(int arg1) {
  ui->cbInj->setEnabled(arg1);
  ui->cbSfout->setEnabled(arg1);
  ui->cbHb->setEnabled(arg1);
  ui->sbTdac->setEnabled(arg1);
}

void ConfigCreatorView::on_sbTdac_valueChanged(int arg1) {
  foreach (auto pix, mPixelToModify) { pixelConfig(pix)->tdac = arg1; }
}

void ConfigCreatorView::on_cbInj_stateChanged(int arg1) {
  foreach (auto pix, mPixelToModify) { pixelConfig(pix)->inj = arg1; }
}

void ConfigCreatorView::on_cbHb_stateChanged(int arg1) {
  foreach (auto pix, mPixelToModify) { pixelConfig(pix)->hb = arg1; }
}

void ConfigCreatorView::on_cbSfout_stateChanged(int arg1) {
  foreach (auto pix, mPixelToModify) { pixelConfig(pix)->sfout = arg1; }
}
