#pragma warning(disable: 4996)
#include "bufferdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QgsVectorLayer.h>
#include <QgsGeometry.h>
#include <QgsFeature.h>
#include <QgsFeatureRequest.h>
#include <QgsFeatureIterator.h>
#include <QgsVectorFileWriter.h>
#include <QgsCoordinateReferenceSystem.h>
#include <QgsCoordinateTransformContext.h>
#include <QgsFeedback.h>
#include <QgsProject.h>
#include <QgsField.h>
#include <QgsCategorizedSymbolRenderer.h>
#include <QgsSymbol.h>
#include <QgsMapCanvas.h>
#include <QgsUnitTypes.h>
#include <QgsGeometryCollection.h>

BufferDialog::BufferDialog(QgsMapCanvas* canvas, QWidget* parent)
	: mpMapCanvas(canvas), QDialog(parent)
{
	// 创建布局
	QVBoxLayout* layout = new QVBoxLayout(this);

	// 创建和添加矢量文件路径选择的控件
	QLabel* labelVectorFile = new QLabel(tr("input file path: "), this);
	lineEditVectorFile = new QLineEdit(this);
	pushButtonSelectVectorFile = new QPushButton(tr("..."), this);
	QHBoxLayout* vectorFileLayout = new QHBoxLayout();
	vectorFileLayout->addWidget(labelVectorFile);
	vectorFileLayout->addWidget(lineEditVectorFile);
	vectorFileLayout->addWidget(pushButtonSelectVectorFile);
	layout->addLayout(vectorFileLayout);

	// 创建和添加输出文件路径选择的控件
	QLabel* labelOutputFile = new QLabel(tr("output file path:"), this);
	lineEditOutputFile = new QLineEdit(this);
	pushButtonSelectOutputFile = new QPushButton(tr("..."), this);
	QHBoxLayout* outputFileLayout = new QHBoxLayout();
	outputFileLayout->addWidget(labelOutputFile);
	outputFileLayout->addWidget(lineEditOutputFile);
	outputFileLayout->addWidget(pushButtonSelectOutputFile);
	layout->addLayout(outputFileLayout);

	// 创建和添加缓冲区大小的控件
	QLabel* labelBufferSize = new QLabel(tr("buffer size(m):"), this);
	doubleSpinBoxBufferSize = new QDoubleSpinBox(this);
	doubleSpinBoxBufferSize->setMinimum(-100000.0);
	doubleSpinBoxBufferSize->setMaximum(1000000.0);
	doubleSpinBoxBufferSize->setValue(10.0);
	QHBoxLayout* bufferSizeLayout = new QHBoxLayout();
	bufferSizeLayout->addWidget(labelBufferSize);
	bufferSizeLayout->addWidget(doubleSpinBoxBufferSize);
	layout->addLayout(bufferSizeLayout);

	// 创建和添加融合选择框
	checkBoxMergeBuffers = new QCheckBox(tr("merge overlapping buffers"), this);
	layout->addWidget(checkBoxMergeBuffers);

	// 创建和添加生成缓冲区按钮
	pushButtonGenerateBuffer = new QPushButton(tr("OK"), this);
	layout->addWidget(pushButtonGenerateBuffer);

	// 连接按钮的点击信号和槽函数
	connect(pushButtonSelectVectorFile, &QPushButton::clicked, this, &BufferDialog::on_pushButtonSelectVectorFile_clicked);
	connect(pushButtonSelectOutputFile, &QPushButton::clicked, this, &BufferDialog::on_pushButtonSelectOutputFile_clicked);
	connect(pushButtonGenerateBuffer, &QPushButton::clicked, this, &BufferDialog::on_pushButtonGenerateBuffer_clicked);

	// 设置对话框的布局
	setLayout(layout);
}

BufferDialog::~BufferDialog()
{
}

void BufferDialog::on_pushButtonSelectVectorFile_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("choose shp"), "", tr("Vector Files (*.shp *.geojson)"));
	if (!fileName.isEmpty()) {
		lineEditVectorFile->setText(fileName);
	}
}

void BufferDialog::on_pushButtonSelectOutputFile_clicked()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("choose output file"), "", tr("Vector Files (*.shp *.geojson)"));
	if (!fileName.isEmpty()) {
		lineEditOutputFile->setText(fileName);
	}
}

void BufferDialog::on_pushButtonGenerateBuffer_clicked()
{
	QString vectorFilePath = lineEditVectorFile->text();
	QString outputFilePath = lineEditOutputFile->text();
	double bufferSize = doubleSpinBoxBufferSize->value();
	bool mergeBuffers = checkBoxMergeBuffers->isChecked();

	if (vectorFilePath.isEmpty() || outputFilePath.isEmpty()) {
		QMessageBox::warning(this, tr("err"), tr("please write all"));
		return;
	}

	QgsVectorLayer* vectorLayer = new QgsVectorLayer(vectorFilePath, "vector_layer", "ogr");
	if (!vectorLayer->isValid()) {
		QMessageBox::warning(this, tr("err"), tr("invalid"));
		delete vectorLayer;
		return;
	}

	// 获取输入图层的 CRS
	QgsCoordinateReferenceSystem inputCRS = vectorLayer->crs();

	// 判断输入图层是否为地理坐标系
	QgsCoordinateReferenceSystem targetCRS = inputCRS.isGeographic() ?
		QgsCoordinateReferenceSystem("EPSG:3395") :  // 投影坐标系
		inputCRS;

	// 创建一个转换器，用于坐标系转换
	QgsCoordinateTransform coordinateTransform(inputCRS, targetCRS, QgsProject::instance());

	// 创建输出图层
	QgsVectorLayer* outputLayer = new QgsVectorLayer("Polygon?crs=" + targetCRS.authid(), "buffer_layer", "memory");
	outputLayer->setCrs(targetCRS);

	QgsFields fields = vectorLayer->fields(); // 复制字段
	outputLayer->dataProvider()->addAttributes(fields.toList());
	outputLayer->updateFields();

	QgsFeatureRequest request;
	QgsFeatureIterator features = vectorLayer->getFeatures(request);

	QgsFeature f;
	while (features.nextFeature(f)) {
		QgsGeometry geom = f.geometry();

		// 进行坐标转换
		geom.transform(coordinateTransform);

		// 计算缓冲区
		QgsGeometry bufferGeom = geom.buffer(bufferSize, 5);

		// 转换回原坐标系
		bufferGeom.transform(QgsCoordinateTransform(targetCRS, inputCRS, QgsProject::instance()));

		QgsFeature newFeature;
		newFeature.setGeometry(bufferGeom);
		newFeature.setAttributes(f.attributes());
		outputLayer->dataProvider()->addFeature(newFeature);
	}

	// 合并相交的缓冲区操作（如有需要）
	if (mergeBuffers) {
		QgsGeometry mergedGeometry;
		QgsFeatureIterator outputFeatures = outputLayer->getFeatures();
		QgsFeature outputFeature;
		while (outputFeatures.nextFeature(outputFeature)) {
			QgsGeometry geom = outputFeature.geometry();
			if (!geom.isNull()) {
				if (!mergedGeometry.isNull()) {
					mergedGeometry = mergedGeometry.combine(geom);
				}
				else {
					mergedGeometry = geom;
				}
			}
		}

		if (!mergedGeometry.isNull()) {
			QgsFeature mergedFeature;
			mergedFeature.setGeometry(mergedGeometry);
			outputLayer->dataProvider()->addFeature(mergedFeature);
			outputLayer->triggerRepaint();
		}
	}

	// 保存图层
	QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormat(
		outputLayer,
		outputFilePath,
		"UTF-8",
		outputLayer->crs(),
		"ESRI Shapefile"
	);

	if (error != QgsVectorFileWriter::NoError) {
		QMessageBox::critical(this, tr("err"), tr("write error"));
		delete vectorLayer;
		delete outputLayer;
		return;
	}

	// 加载保存的图层到当前工程
	QgsVectorLayer* savedLayer = new QgsVectorLayer(outputFilePath, "buffer_layer", "ogr");
	if (savedLayer->isValid()) {
		QgsProject::instance()->addMapLayer(savedLayer);
		mpMapCanvas->refresh();
	}
	else {
		QMessageBox::critical(this, tr("err"), tr("can not load layer"));
		delete savedLayer;
		delete vectorLayer;
		delete outputLayer;
		return;
	}

	// 清理
	delete vectorLayer;
	delete outputLayer;

	// 清空对话框内容
	lineEditVectorFile->clear();
	lineEditOutputFile->clear();
	doubleSpinBoxBufferSize->setValue(0.0);

	// 关闭对话框
	accept();

	QMessageBox::information(this, tr("success"), tr("success"));
}
