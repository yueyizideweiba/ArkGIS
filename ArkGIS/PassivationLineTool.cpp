#pragma warning(disable: 4996)
#include "passivationlinetool.h"
#include <QgsProject.h>
#include <QgsVectorLayer.h>
#include <QgsFeature.h>
#include <QgsGeometry.h>
#include <QgsRubberBand.h>
#include <QgsWkbTypes.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QgsVectorFileWriter.h>
#include <QgsMapMouseEvent.h>
#include <QCoreApplication>

PassivationLineTool::PassivationLineTool(QgsMapCanvas* canvas)
	: QgsMapTool(canvas), mCanvas(canvas), mLayer(nullptr), mRubberBand(nullptr), mIsPassivating(false), mDialog(new QDialog())
{
	// 创建布局
	QVBoxLayout* layout = new QVBoxLayout(mDialog);

	// 创建和添加矢量文件路径选择的控件
	QLabel* labelVectorFile = new QLabel(tr("input file path: "), mDialog);
	lineEditVectorFile = new QLineEdit(mDialog);
	pushButtonSelectVectorFile = new QPushButton(tr("..."), mDialog);
	QHBoxLayout* vectorFileLayout = new QHBoxLayout();
	vectorFileLayout->addWidget(labelVectorFile);
	vectorFileLayout->addWidget(lineEditVectorFile);
	vectorFileLayout->addWidget(pushButtonSelectVectorFile);
	layout->addLayout(vectorFileLayout);

	// 创建和添加输出文件路径选择的控件
	QLabel* labelOutputFile = new QLabel(tr("output file path:"), mDialog);
	lineEditOutputFile = new QLineEdit(mDialog);
	pushButtonSelectOutputFile = new QPushButton(tr("..."), mDialog);
	QHBoxLayout* outputFileLayout = new QHBoxLayout();
	outputFileLayout->addWidget(labelOutputFile);
	outputFileLayout->addWidget(lineEditOutputFile);
	outputFileLayout->addWidget(pushButtonSelectOutputFile);
	layout->addLayout(outputFileLayout);

	// 创建和添加确认按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, mDialog);
	layout->addWidget(buttonBox);
	connect(buttonBox, &QDialogButtonBox::accepted, mDialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &PassivationLineTool::onFileSelected);

	// 设置对话框的布局
	mDialog->setLayout(layout);

	connect(pushButtonSelectVectorFile, &QPushButton::clicked, this, &PassivationLineTool::selectVectorFile);
	connect(pushButtonSelectOutputFile, &QPushButton::clicked, this, &PassivationLineTool::selectOutputFile);
	// 显示对话框
	mDialog->show();
}

PassivationLineTool::~PassivationLineTool()
{
	if (mRubberBand) {
		mCanvas->scene()->removeItem(mRubberBand);
		delete mRubberBand;
	}
	delete mDialog;
}

void PassivationLineTool::selectVectorFile()
{
	QString inputFilePath = QFileDialog::getOpenFileName(mDialog, tr("Select Vector File"), "", tr("Vector Files (*.shp *.geojson)"));
	if (!inputFilePath.isEmpty()) {
		lineEditVectorFile->setText(inputFilePath);
	}
}

void PassivationLineTool::selectOutputFile()
{
	QString outputFilePath = QFileDialog::getSaveFileName(mDialog, tr("Select Output File"), "", tr("Shapefile (*.shp)"));
	if (!outputFilePath.isEmpty()) {
		lineEditOutputFile->setText(outputFilePath);
	}
}

void PassivationLineTool::onFileSelected()
{
	QString inputFilePath = lineEditVectorFile->text();
	QString outputFilePath = lineEditOutputFile->text();

	if (inputFilePath.isEmpty()) {
		qCritical("No input file selected");
		QMessageBox::critical(mDialog, tr("Error"), tr("Input file not selected"));
		return;
	}

	if (outputFilePath.isEmpty()) {
		qCritical("No output path selected");
		QMessageBox::critical(mDialog, tr("Error"), tr("Output path not selected"));
		return;
	}

	mLayer = new QgsVectorLayer(inputFilePath, "layer", "ogr");
	if (!mLayer->isValid()) {
		qCritical("Failed to load layer: %s", qPrintable(inputFilePath));
		QMessageBox::critical(mDialog, tr("Error"), tr("Failed to load layer: %1").arg(inputFilePath));
		delete mLayer;
		return;
	}

	if (mLayer->geometryType() != Qgis::GeometryType::Line) {
		qCritical("Selected layer is not a line layer");
		QMessageBox::critical(mDialog, tr("Error"), tr("Selected layer is not a line layer"));
		delete mLayer;
		return;
	}

	qInfo("Line layer loaded successfully: %s", qPrintable(mLayer->name()));

	// 显示选择的图层信息
	mDialog->hide();
	mDialog = new QDialog();
	mDialog->setWindowTitle(tr("Layer"));
	QVBoxLayout* layout = new QVBoxLayout(mDialog);
	QLabel* layerLabel = new QLabel(tr("Layer: %1").arg(mLayer->name()), mDialog);
	layout->addWidget(layerLabel);
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, mDialog);
	layout->addWidget(buttonBox);
	connect(buttonBox, &QDialogButtonBox::accepted, mDialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &PassivationLineTool::processPassivationLine);
	mDialog->show();
}

void PassivationLineTool::processPassivationLine()
{
	if (mLayer) {
		QgsFeatureIterator fit = mLayer->getFeatures();
		QgsFeature feature;
		QgsFeature newFeature;
		QgsFields newFields = mLayer->fields();
		QgsVectorLayer* newLayer = new QgsVectorLayer("Linestring?crs=" + mLayer->crs().toWkt(), "passivation_layer", "memory");
		newLayer->startEditing();
		newLayer->dataProvider()->addAttributes(newFields.toList());
		newLayer->updateFields();

		while (fit.nextFeature(feature)) {
			QgsGeometry geom = feature.geometry();

			// 平滑处理线
			QgsGeometry smoothedGeom = geom.smooth(4);
			newFeature.setGeometry(smoothedGeom);
			newFeature.setFields(newFields);
			newFeature.setAttributes(feature.attributes());
			newLayer->addFeature(newFeature);

		}

		newLayer->commitChanges();

		// 保存新图层
		QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormat(
			newLayer, lineEditOutputFile->text(), "UTF-8", mLayer->crs(), "ESRI Shapefile"
		);

		// 检查保存是否成功
		if (error == QgsVectorFileWriter::NoError) {
			qInfo("Layer saved successfully: %s", qPrintable(lineEditOutputFile->text()));
			QMessageBox::information(mDialog, tr("Success"), tr("Layer saved successfully: %1").arg(lineEditOutputFile->text()));

			// 加载新图层
			QgsVectorLayer* savedLayer = new QgsVectorLayer(lineEditOutputFile->text(), "passivation_layer", "ogr");
			if (savedLayer->isValid()) {
				QgsProject::instance()->addMapLayer(savedLayer);
				mCanvas->refresh();
			}
			else {
				qCritical("Failed to load saved layer: %s", qPrintable(lineEditOutputFile->text()));
				QMessageBox::critical(mDialog, tr("Error"), tr("Failed to load saved layer: %1").arg(lineEditOutputFile->text()));
			}
		}
		else {
			qCritical("Failed to write layer: %s", qPrintable(lineEditOutputFile->text()));
			QMessageBox::critical(mDialog, tr("Error"), tr("Failed to write layer: %1").arg(lineEditOutputFile->text()));
		}
		// 发射处理完成信号
		emit finished();
	}
}
