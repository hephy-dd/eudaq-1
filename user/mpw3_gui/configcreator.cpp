#include "configcreator.h"
#include "ui_configcreator.h"

#include <QFile>
#include <QTextStream>
#include <memory>

ConfigCreatorView::ConfigCreatorView(QWidget *parent)
    : QWidget(parent), ui(new Ui::ConfigCreator) {
  ui->setupUi(this);

  ui->tvConfig->setModel(&mModel);
}

ConfigCreatorView::~ConfigCreatorView() { delete ui; }

void ConfigCreatorView::on_pbParse_clicked() {
  mModel.clear();
  parseConfig(ui->leTemplateFile->text());
  setupView();
}

void ConfigCreatorView::parseConfig(const QString &pathToCfg) {
  QString filename;
  const QString tmpConfigFile = "/tmp/tmpconf.cfg";

  if (ui->rbLocal->isChecked()) {
    filename = pathToCfg; // we work locally, this means we can simply open the
                          // wanted file from our drive
  } else if (ui->rbSsh->isChecked()) {
    QString scpCmd = "scp " + pathToCfg.trimmed() + " " + tmpConfigFile;
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

  while (!input.atEnd()) {
    ConfigItem item;
    QString line = input.readLine();
    line = line.trimmed(); // trim unnecessary white space
    if (line.isEmpty() ||
        line.startsWith("[")) //"[" is the start of the device specifier
      continue;

    if (line.startsWith("#") ||
        line.startsWith("[")) { // might signal a comment OR a deactivated item
      auto split = line.split("=");
      if (split.length() ==
          2) { // we found a "=" which indicates a deactivated item
        item.enabled = false;
        item.isComment = false;
        item.key = split[0].replace("#", "");
        item.value = split[1];
      } else {
        item.isComment = true;
        item.key = line;
      }

      mItems.append(item);

    } else {
      auto split = line.split("=");
      if (split.length() != 2) {
        ui->tbLog->append("Invalid config line:\n" + line + "\n");
      } else {
        item.enabled = true;
        item.isComment = false;
        item.key = split[0];
        item.value = split[1];
        mItems.append(item);
      }
    }
  }
}

void ConfigCreatorView::saveConfig(const QString &fileName) {
  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream out(&file);
    out << "[RD50_MPW2]\n"; // device specifier needed by peary
    for (int row = 0; row < mModel.rowCount(); row++) {

      // data of 1st item tells us how to treat this row
      if (mModel.item(row, 0)->data() == ColumnRole::Comment) {
        out << "\n";
        out << mModel.item(row, 0)->text();
        out << "\n";
        continue;
      } else if (mModel.item(row, 0)->data() == ColumnRole::Item) {
        ui->tbLog->append(
            "data " + QString::number(row) + " = " +
            QString::number(
                mModel.item(row, 2)->data(Qt::CheckStateRole).toInt()));
        if (mModel.item(row, 2)->data(Qt::CheckStateRole) == Qt::Unchecked) {
          out << "#"; // this item is disabled, indicated by "#"
        }
        out << mModel.item(row, 0)->text();
        out << "=";
        out << mModel.item(row, 1)->text();
        out << "\n";
      }
    }
  } else {
    ui->tbLog->append("Error opening output file");
  }
}

void ConfigCreatorView::setupView() {
  mModel.clear();

  foreach (const ConfigItem &item, mItems) {

    if (item.isComment) {
      auto commentLine = new QStandardItem(item.key);
      commentLine->setForeground(QBrush(Qt::blue)); // color comment line
      commentLine->setData(ColumnRole::Comment);
      QList<QStandardItem *> row;
      row << commentLine;
      mModel.appendRow(row);

      continue;
    }
    // so we are dealing with actual data (no comment)
    auto key = new QStandardItem(item.key);
    key->setData(ColumnRole::Item);
    auto val = new QStandardItem(item.value);
    auto enabled = new QStandardItem();

    QVariant data;
    if (item.enabled) {
      data = Qt::Checked;
    } else {
      data = Qt::Unchecked;
    }

    enabled->setData(data, Qt::CheckStateRole); // this item is a checkbox
    enabled->setFlags(enabled->flags() | Qt::ItemIsUserCheckable);
    QList<QStandardItem *> row;
    row.append(key);
    row.append(val);
    row.append(enabled);
    mModel.appendRow(row);
  }
  QStringList labels;
  labels << "key"
         << "value"
         << "enabled";
  mModel.setHorizontalHeaderLabels(labels);
}

void ConfigCreatorView::on_pbDeploy_clicked() {

  QString outputFile;
  const QString tmpConfig = "/tmp/configout.cfg";
  if (ui->rbLocal->isChecked()) {
    outputFile = ui->leOutputFile->text();
  } else if (ui->rbSsh->isChecked()) {
    outputFile = tmpConfig;
  }
  saveConfig(outputFile);

  if (ui->rbSsh->isChecked()) {
    QString scpCmd = "scp " + tmpConfig + " " + ui->leOutputFile->text();
    if (system((scpCmd.toLocal8Bit().data())) != 0) {
      ui->tbLog->append("Error deploying file: " + tmpConfig +
                        "\n to: " + ui->leOutputFile->text());
    }
  }
}

void ConfigCreatorView::on_pbAddItem_clicked() {

  QList<QStandardItem *> row;
  for (int i = 0; i < 2; i++) {
    auto item = new QStandardItem();
    if (i == 0) {
      item->setText("new item");
    } else {
      item->setText("xxx");
    }

    row.append(item);
  }
  mModel.appendRow(row);
}

void ConfigCreatorView::on_pbClearLog_clicked() { ui->tbLog->clear(); }
