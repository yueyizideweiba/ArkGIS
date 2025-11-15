#include <QFileDialog>
#include <QMessageBox>
#include <QgsVectorLayer.h>
#include <QgsRasterLayer.h>
#include <QgsProject.h>
#include <QgsMapCanvas.h>
#include <QgsCoordinateTransform.h>
#include <QgsRectangle.h>
#include <QgsField.h>
#include <QgsFields.h>
#include <QProcess>
#include <QCoreApplication>
#include "mainwindow.h"

VectorToRasterConverter::VectorToRasterConverter(QgsMapCanvas* mapCanvas, MainWindow* mainWindow)
	: mpMapCanvas(mapCanvas), mMainWindow(mainWindow) {
}

void VectorToRasterConverter::startConversion() {
	selectVectorLayer();
}

void VectorToRasterConverter::selectVectorLayer() {
	QString fileName = QFileDialog::getOpenFileName(nullptr, QCoreApplication::translate("VectorToRasterConverter", "choose vector file"), "", QCoreApplication::translate("VectorToRasterConverter", "shp (*.shp *.gpkg)"));
	if (!fileName.isEmpty()) {
		QgsVectorLayer* vectorLayer = new QgsVectorLayer(fileName, QFileInfo(fileName).baseName(), "ogr");

		if (vectorLayer->isValid()) {
			FieldSelectionDialog dialog(vectorLayer);
			if (dialog.exec() == QDialog::Accepted) {
				int fieldIndex = dialog.getFieldIndex();
				QString outputPath = dialog.getOutputPath();
				convertToRaster(vectorLayer, fieldIndex, outputPath);
			}
			delete vectorLayer;
		}
		else {
			QMessageBox::critical(nullptr, QCoreApplication::translate("VectorToRasterConverter", "err"), QCoreApplication::translate("VectorToRasterConverter", "cannot add vector file"));
			delete vectorLayer;
		}
	}
}

void VectorToRasterConverter::convertToRaster(QgsVectorLayer* vectorLayer, int fieldIndex, const QString& outputPath) {
	QgsFields fields = vectorLayer->fields();

	// 获取矢量图层的范围
	QgsRectangle extent = vectorLayer->extent();
	double xMin = extent.xMinimum();
	double xMax = extent.xMaximum();
	double yMin = extent.yMinimum();
	double yMax = extent.yMaximum();
	int width = 1000;  // 栅格图层的宽度
	int height = 1000; // 栅格图层的高度

	// 构建 gdal_rasterize 命令的参数
	QStringList arguments;
	arguments << "-a" << fields.at(fieldIndex).name()
		<< "-tr" << QString::number((xMax - xMin) / width)
		<< QString::number((yMax - yMin) / height)
		<< "-te" << QString::number(xMin)
		<< QString::number(yMin)
		<< QString::number(xMax)
		<< QString::number(yMax)
		<< "-ot" << "Byte"
		<< "-of" << "GTiff"
		<< vectorLayer->source()
		<< outputPath;

	// 使用 QProcess 执行命令
	QProcess process;
	process.start("gdal_rasterize", arguments);  // Updated to use arguments as a list
	if (!process.waitForFinished()) {
		QMessageBox::critical(nullptr, QCoreApplication::translate("VectorToRasterConverter", "err"), QCoreApplication::translate("VectorToRasterConverter", "tiff fail"));
		return;
	}

	// 加载生成的栅格文件
	addRasterLayer(outputPath);
}


void VectorToRasterConverter::addRasterLayer(const QString& rasterPath) {
	if (!rasterPath.isEmpty()) {
		QgsRasterLayer* rasterLayer = new QgsRasterLayer(rasterPath, QFileInfo(rasterPath).baseName(), "gdal");

		if (rasterLayer->isValid()) {
			// 检查是否已设置数据框坐标系
			if (!mMainWindow->dataFrameCrs().isValid()) {
				// 未设置数据框坐标系，则将其设置为当前图层的坐标系
				mMainWindow->setDataFrameCrs(rasterLayer->crs());
			}
			else if (rasterLayer->crs() != mMainWindow->dataFrameCrs()) {
				// 如果图层坐标系与数据框坐标系不同，进行临时投影转换
				QgsCoordinateTransform transform(rasterLayer->crs(), mMainWindow->dataFrameCrs(), QgsProject::instance());

				// 设置其CRS进行坐标系转换
				rasterLayer->setCrs(mMainWindow->dataFrameCrs());
			}

			// 将图层添加到项目并更新地图画布
			QgsProject::instance()->addMapLayer(rasterLayer);
			mpMapCanvas->refresh();
		}
		else {
			QMessageBox::critical(nullptr, QCoreApplication::translate("VectorToRasterConverter", "err"), QCoreApplication::translate("VectorToRasterConverter", "cannot addtiff"));
			delete rasterLayer;
		}
	}
}

VectorToRasterConverter::FieldSelectionDialog::FieldSelectionDialog(QgsVectorLayer* vectorLayer, QWidget* parent)
	: QDialog(parent), mFieldIndex(-1), mOutputPath() {
	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	// 字段选择部分
	QLabel* fieldLabel = new QLabel("choose tiff field:");
	mainLayout->addWidget(fieldLabel);

	mFieldListWidget = new QListWidget(this);
	for (const QgsField& field : vectorLayer->fields()) {
		mFieldListWidget->addItem(field.name());
	}
	mainLayout->addWidget(mFieldListWidget);

	// 输出路径选择部分
	QHBoxLayout* outputPathLayout = new QHBoxLayout();
	QLabel* outputPathLabel = new QLabel("output path:");
	outputPathLayout->addWidget(outputPathLabel);

	mOutputPathEdit = new QLineEdit(this);
	outputPathLayout->addWidget(mOutputPathEdit);

	mBrowseButton = new QPushButton("...", this);
	QObject::connect(mBrowseButton, &QPushButton::clicked, this, [this]() {
		QString outputPath = QFileDialog::getSaveFileName(this, "save file", "", "TIFF (*.tiff)");
		if (!outputPath.isEmpty()) {
			mOutputPathEdit->setText(outputPath);
		}
		});
	outputPathLayout->addWidget(mBrowseButton);

	mainLayout->addLayout(outputPathLayout);

	// 按钮部分
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	mConvertButton = new QPushButton("OK", this);
	mCancelButton = new QPushButton("NO", this);
	buttonLayout->addWidget(mConvertButton);
	buttonLayout->addWidget(mCancelButton);

	mainLayout->addLayout(buttonLayout);

	// 连接按钮信号和槽
	QObject::connect(mConvertButton, &QPushButton::clicked, this, &QDialog::accept);
	QObject::connect(mCancelButton, &QPushButton::clicked, this, &QDialog::reject);

	// 设置默认字段选择
	if (mFieldListWidget->count() > 0) {
		mFieldListWidget->setCurrentRow(0);
	}
}

QString VectorToRasterConverter::FieldSelectionDialog::getOutputPath() const {
	return mOutputPathEdit->text();
}

int VectorToRasterConverter::FieldSelectionDialog::getFieldIndex() const {
	return mFieldListWidget->currentRow();
}