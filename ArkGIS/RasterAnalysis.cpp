#pragma warning(disable:4996)
#include "RasterAnalysis.h"
#include <QgsRasterLayer.h>
#include <QgsRasterDataProvider.h>
#include <QgsRectangle.h>
#include <QgsRasterBandStats.h>
#include <QDialogButtonBox.h>
#include <QVBoxLayout>
#include <QComboBox.h>
#include <QLabel.h>
#include <QLineEdit.h>
#include <QFileDialog.h>
#include <QMessageBox.h>
#include <QTextStream.h>
#include <QFile.h>
#include <QListWidget.h>
#include <QTreeWidget.h>
#include <QgsRasterCalculator.h>

RasterAnalysis::RasterAnalysis(QgsMapCanvas* canvas, QObject* parent)
	: QObject(parent), mpMapCanvas(canvas) {
}

// 栅格图层统计
void RasterAnalysis::performRasterLayerStatistics()
{
	QDialog dialog;
	dialog.resize(350, 150);
	dialog.setWindowTitle(tr("选择栅格图层进行统计"));

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	QLabel* rasterLabel = new QLabel(tr("请选择栅格图层："), &dialog);
	layout->addWidget(rasterLabel);

	// 创建栅格图层下拉框并添加到布局
	QComboBox* rasterComboBox = new QComboBox(&dialog);
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values())
	{
		if (l->type() == Qgis::LayerType::Raster)
		{
			rasterComboBox->addItem(l->name(), l->id());
		}
	}
	layout->addWidget(rasterComboBox);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	if (dialog.exec() == QDialog::Accepted)
	{
		// 获取选中的栅格图层
		QString rasterLayerId = rasterComboBox->currentData().toString();
		QgsRasterLayer* rasterLayer = dynamic_cast<QgsRasterLayer*>(QgsProject::instance()->mapLayer(rasterLayerId));

		if (!rasterLayer)
		{
			QMessageBox::warning(nullptr, tr("错误"), tr("无法找到选择的栅格图层。"));
			return;
		}

		QgsRasterDataProvider* provider = rasterLayer->dataProvider();
		int band = 1; // 波段1
		QgsRectangle extent = rasterLayer->extent(); // 获取栅格图层范围
		// 计算栅格图层的统计信息
		QgsRasterBandStats stats = provider->bandStatistics(band, QgsRasterBandStats::All, extent);

		if (stats.minimumValue == std::numeric_limits<double>::max())
		{
			QMessageBox::warning(nullptr, tr("错误"), tr("无法计算栅格图层的统计信息。"));
			return;
		}

		QString defaultPath = "../Results"; // 设置默认保存路径
		// 弹对话框获取保存路径
		QString fileName = QFileDialog::getSaveFileName(nullptr, tr("保存统计结果"), defaultPath, tr("HTML Files (*.html)"));

		if (fileName.isEmpty())
		{
			return;
		}

		// 打开并写文件
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QMessageBox::warning(nullptr, tr("错误"), tr("无法保存统计结果文件。"));
			return;
		}

		QTextStream out(&file);
		out.setCodec("UTF-8");
		out.setRealNumberNotation(QTextStream::FixedNotation);
		out.setRealNumberPrecision(4);

		// 写入HTML格式的统计结果
		out << tr("<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"UTF-8\">\n<title>栅格图层统计</title>\n</head>\n<body>\n");
		out << tr("<h1>栅格图层统计结果</h1>\n");
		out << tr("<p>分析的文件: ") << rasterLayer->source() << tr(" （波段 ") << band << tr("）</p>\n");
		out << tr("<p>最小值: ") << stats.minimumValue << tr("</p>\n");
		out << tr("<p>最大值: ") << stats.maximumValue << tr("</p>\n");
		out << tr("<p>范围: ") << stats.maximumValue - stats.minimumValue << tr("</p>\n");
		out << tr("<p>总和: ") << stats.sum << tr("</p>\n");
		out << tr("<p>平均值: ") << stats.mean << tr("</p>\n");
		out << tr("<p>标准差: ") << stats.stdDev << tr("</p>\n");
		out << tr("<p>平方和: ") << stats.sumOfSquares << tr("</p>\n");
		out << tr("<p>像元总数: ") << stats.elementCount << tr("</p>\n");
		out << tr("</body>\n</html>");

		file.close();

		QMessageBox::information(nullptr, tr("成功"), tr("统计结果已保存到文件。"));
	}
	
}

// 栅格计算器功能
void RasterAnalysis::rasterCalculator() {
	QDialog dialog;
	dialog.resize(600, 500);
	dialog.setWindowTitle(tr("栅格计算器"));
	QVBoxLayout* layout1 = new QVBoxLayout(&dialog);
	QHBoxLayout* layout2 = new QHBoxLayout(&dialog);
	QVBoxLayout* layout3 = new QVBoxLayout(&dialog);
	QVBoxLayout* layout4 = new QVBoxLayout(&dialog);

	// 栅格图层列表
	QLabel* rasterLabel = new QLabel(tr("栅格图层列表："), &dialog);
	layout3->addWidget(rasterLabel);
	QTreeWidget* rasterTreeWidget = new QTreeWidget(&dialog);
	rasterTreeWidget->setHeaderLabels({ "Name", "bandCount"});
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values())
	{
		if (l->type() == Qgis::LayerType::Raster)
		{
			QgsRasterLayer* rasterLayer = dynamic_cast<QgsRasterLayer*>(l);
			QTreeWidgetItem* layerItem = new QTreeWidgetItem(rasterTreeWidget); // 显示图层名称
			layerItem->setText(0, rasterLayer->name());
			layerItem->setText(1, QString::number(rasterLayer->bandCount()));
			layerItem->setData(0, Qt::UserRole, rasterLayer->id()); // 将图层 ID 存储到用户数据中
			// 获取波段信息
			QgsRasterDataProvider* provider = rasterLayer->dataProvider();
			if (provider) {
				for (int band = 1; band <= provider->bandCount(); band++) {
					QTreeWidgetItem* bandItem = new QTreeWidgetItem(layerItem);
					bandItem->setText(0, QString("band %1").arg(band));
                    bandItem->setData(0, Qt::UserRole, band);
				}
			}

		}
	}
	layout3->addWidget(rasterTreeWidget);

	// 运算符及函数列表
	QLabel* operatorLabel = new QLabel(tr("运算符及函数列表："), &dialog);
	layout4->addWidget(operatorLabel);
	QListWidget* operatorListWidget = new QListWidget(&dialog);
	operatorListWidget->addItem("+");
	operatorListWidget->addItem("-");
	operatorListWidget->addItem("*");
	operatorListWidget->addItem("/");
	operatorListWidget->addItem("^");
	operatorListWidget->addItem("(");
	operatorListWidget->addItem(")");
	operatorListWidget->addItem("sqrt()");
	operatorListWidget->addItem("log()");
	layout4->addWidget(operatorListWidget);
	layout2->addLayout(layout3);
	layout2->addLayout(layout4);
	layout1->addLayout(layout2);

	QLabel* formulaLabel = new QLabel(tr("栅格计算表达式："), &dialog);
	QLineEdit* formulaLineEdit = new QLineEdit(&dialog);
	layout1->addWidget(formulaLabel);
	layout1->addWidget(formulaLineEdit);

	// 表达式中的栅格图层
	QVector<QPair<QgsRasterLayer*, int>> rasterLayers;
	// 双击栅格图层列表中的项，将其输入到表达式
	auto inputRasterLayer = [&](QTreeWidgetItem* item) {
		if (item->parent() != nullptr) {
			QTreeWidgetItem* parentItem = item->parent();
			QgsRasterLayer* rasterLayer = dynamic_cast<QgsRasterLayer*>(QgsProject::instance()->mapLayer(parentItem->data(0, Qt::UserRole).toString()));
			if (rasterLayer != nullptr) {
				formulaLineEdit->insert(parentItem->text(0) + "@" + item->data(0, Qt::UserRole).toString() + " ");
				rasterLayers.append(QPair(rasterLayer, item->data(0, Qt::UserRole).toInt()));
			}
		}
	};
	// 双击运算符及函数列表中的项，将其输入到表达式
	auto inputOperator = [&](QListWidgetItem* item) {
		formulaLineEdit->insert(item->text() + " ");
		// 如果输入为函数，则将光标移动到括号中
		if (item->text() == "sqrt()" || item->text() == "log()") {
			formulaLineEdit->setCursorPosition(formulaLineEdit->cursorPosition() - 2);
		}

	};
	connect(rasterTreeWidget, &QTreeWidget::itemDoubleClicked, inputRasterLayer);
	connect(operatorListWidget, &QListWidget::itemDoubleClicked, inputOperator);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout1->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	if (dialog.exec() == QDialog::Accepted)
	{
		// 栅格列表不能为空
		if (rasterLayers.isEmpty() || rasterLayers[0].first == nullptr) {
			qDebug() << "No rasterLayer selected.";
			return;
		}
		
		// 定义输出路径
		QString outputRasterPath = QFileDialog::getSaveFileName(nullptr, tr("选择输出栅格位置"), "", tr("Raster Files (*.tiff)"));
		if (outputRasterPath.isEmpty()) {
			qDebug() << "Output raster file path is empty.";
			return;
		}

		// 创建计算表达式（以像素值相加为例）
		QString formula = formulaLineEdit->text();

		// 配置计算器参数
		QVector<QgsRasterCalculatorEntry> entries;
		// 遍历表达式中的栅格图层
		for (int i = 0; i < rasterLayers.size(); i++) {
            QgsRasterCalculatorEntry entry;
            entry.ref = rasterLayers[i].first->name() + "@" + QString::number(rasterLayers[i].second);
            entry.raster = rasterLayers[i].first;
			entry.bandNumber = rasterLayers[i].second;
			entries << entry;
		}

		// 执行栅格计算
		QgsRasterCalculator calculator(
			formula,
			outputRasterPath,
			"GTiff",
			rasterLayers[0].first->extent(),
			rasterLayers[0].first->width(),
			rasterLayers[0].first->height(),
			entries);

		if (calculator.processCalculation() == QgsRasterCalculator::Success) {
			// 加载输出栅格图层到 QGIS
			QgsRasterLayer* outputRasterLayer = new QgsRasterLayer(outputRasterPath, outputRasterPath.split('/').last());
			QgsProject::instance()->addMapLayer(outputRasterLayer);
		}
		else {
			qWarning() << "Raster calculation failed with error:" << calculator.lastError();
		}
	}
}
