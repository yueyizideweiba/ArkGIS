#include "ProjectionTransformDialog.h"
#include "RectangleSelectTool.h"
#include "SingleClickSelectTool.h"
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QgsProjectionSelectionWidget.h>
#include <QgsProject.h>
#include <QgsVectorLayer.h>
#include <QMessageBox>
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "ogr_spatialref.h"
#include "ogrsf_frmts.h"

ProjectionTransformDialog::ProjectionTransformDialog(QgsMapCanvas* mapCanvas, QWidget* parent)
	: QDialog(parent), mpMapCanvas(mapCanvas)
{
	setModal(false);  // 设置对话框为非模态
	setWindowTitle(tr("投影变换"));

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	// 图层来源选择
	QHBoxLayout* layerSourceLayout = new QHBoxLayout();
	layerSourceLabel = new QLabel(tr("图层来源："), this);
	mLayerSourceComboBox = new QComboBox(this);
	mLayerSourceComboBox->addItem(tr("当前图层"));
	mLayerSourceComboBox->addItem(tr("文件"));
	layerSourceLayout->addWidget(layerSourceLabel);
	layerSourceLayout->addWidget(mLayerSourceComboBox);
	mainLayout->addLayout(layerSourceLayout);

	// 图层选择
	QHBoxLayout* layerLayout = new QHBoxLayout();
	QLabel* layerLabel = new QLabel(tr("选择图层："), this);
	mLayerComboBox = new QComboBox(this);

	// 获取当前项目中的图层
	const auto mapLayers = QgsProject::instance()->mapLayers();
	for (auto layer : mapLayers) {
		mLayers.append(layer);
		mLayerComboBox->addItem(layer->name());
	}

	layerLayout->addWidget(layerLabel);
	layerLayout->addWidget(mLayerComboBox);
	mainLayout->addLayout(layerLayout);

	// 文件选择
	QHBoxLayout* fileLayout = new QHBoxLayout();
	fileLabel = new QLabel(tr("选择文件："), this);
	mFileLineEdit = new QLineEdit(this);
	mSelectFileButton = new QPushButton(tr("浏览..."), this);
	fileLayout->addWidget(fileLabel);
	fileLayout->addWidget(mFileLineEdit);
	fileLayout->addWidget(mSelectFileButton);
	mainLayout->addLayout(fileLayout);

	// 目标坐标系选择
	QHBoxLayout* crsLayout = new QHBoxLayout();
	QLabel* crsLabel = new QLabel(tr("目标坐标系："), this);
	mCrsSelector = new QgsProjectionSelectionWidget(this);
	mCrsSelector->setMaximumHeight(40);
	crsLayout->addWidget(crsLabel);
	crsLayout->addWidget(mCrsSelector);
	mainLayout->addLayout(crsLayout);

	// 确定和取消按钮
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	okButton = new QPushButton(tr("确定"), this);
	cancelButton = new QPushButton(tr("取消"), this);
	buttonLayout->addStretch();
	buttonLayout->addWidget(okButton);
	buttonLayout->addWidget(cancelButton);
	mainLayout->addLayout(buttonLayout);

	// 默认状态
	mLayerComboBox->setEnabled(true);
	mFileLineEdit->setEnabled(false);
	mSelectFileButton->setEnabled(false);

	// 信号和槽
	connect(mLayerSourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &ProjectionTransformDialog::onLayerSourceChanged);
	connect(mSelectFileButton, &QPushButton::clicked, this, &ProjectionTransformDialog::selectFile);
	connect(okButton, &QPushButton::clicked, this, &ProjectionTransformDialog::accept);
	connect(cancelButton, &QPushButton::clicked, this, &ProjectionTransformDialog::reject);

	// 选择部分要素按钮
	mSelectFeaturesButton = new QPushButton(tr("选择部分要素"), this);
	mainLayout->addWidget(mSelectFeaturesButton);

	// 选择方式按钮
	QHBoxLayout* selectModeLayout = new QHBoxLayout();
	selectModeLabel = new QLabel(tr("选择方式："), this);
	mSingleClickButton = new QRadioButton(tr("点击选择"), this);
	mBoxSelectButton = new QRadioButton(tr("拉框框选"), this);
	selectModeLayout->addWidget(selectModeLabel);
	selectModeLayout->addWidget(mSingleClickButton);
	selectModeLayout->addWidget(mBoxSelectButton);
	mainLayout->addLayout(selectModeLayout);

	// 确认和取消按钮
	QHBoxLayout* selectbuttonLayout = new QHBoxLayout();
	mConfirmButton = new QPushButton(tr("确定"), this);
	selectcancelButton = new QPushButton(tr("取消"), this);
	selectbuttonLayout->addStretch();
	selectbuttonLayout->addWidget(mConfirmButton);
	selectbuttonLayout->addWidget(selectcancelButton);
	mainLayout->addLayout(selectbuttonLayout);

	// 默认隐藏选择方式和确定按钮
	selectModeLabel->setVisible(false);
	mSingleClickButton->setVisible(false);
	mBoxSelectButton->setVisible(false);
	mConfirmButton->setVisible(false);
	selectcancelButton->setVisible(false);

	connect(mSelectFeaturesButton, &QPushButton::clicked, this, &ProjectionTransformDialog::enableSelectionMode);
	connect(mSingleClickButton, &QRadioButton::clicked, this, &ProjectionTransformDialog::activateSingleSelectTool);
	connect(mBoxSelectButton, &QRadioButton::clicked, this, &ProjectionTransformDialog::activateBoxSelectTool);
	connect(mConfirmButton, &QPushButton::clicked, this, &ProjectionTransformDialog::handleSelectionComplete);
	connect(selectcancelButton, &QPushButton::clicked, this, &ProjectionTransformDialog::cancelSelectionMode);
}

ProjectionTransformDialog::~ProjectionTransformDialog() {
	// 确保退出选择要素模式
	if (mIsSelectingFeatures) {
		mpMapCanvas->unsetMapTool(mpMapCanvas->mapTool());  // 移除当前选择工具
		mIsSelectingFeatures = false;  // 重置选择模式标志
	}

	// 恢复为拖拽工具
	auto panTool = new QgsMapToolPan(mpMapCanvas);
	mpMapCanvas->setMapTool(panTool);
}


void ProjectionTransformDialog::onLayerSourceChanged(int index)
{
	if (index == 0) {
		// 当前图层
		mLayerComboBox->setEnabled(true);
		mFileLineEdit->setEnabled(false);
		mSelectFileButton->setEnabled(false);
	}
	else {
		// 文件
		mLayerComboBox->setEnabled(false);
		mFileLineEdit->setEnabled(true);
		mSelectFileButton->setEnabled(true);
	}
}

bool ProjectionTransformDialog::selectFile()
{
	QString layerName = QFileDialog::getOpenFileName(this, tr("选择文件"), "", tr("所有文件 (*.*);;QGIS项目文件 (*.qgs)"));
	if (!layerName.isEmpty()) {
		mFileLineEdit->setText(layerName);  // 更新文件路径到输入框

		// 检查文件类型
		if (layerName.endsWith(".qgs", Qt::CaseInsensitive)) {
			QgsProject tempProject;
			if (!tempProject.read(layerName)) {
				QMessageBox::warning(this, tr("错误"), tr("无法加载QGS项目文件。"));
				return 0;
			}
			QList<QgsMapLayer*> layers = tempProject.mapLayers().values();
			if (layers.isEmpty()) {
				QMessageBox::warning(this, tr("错误"), tr("QGS项目文件中没有图层。"));
				return 0;
			}

			// 这里可以选择返回第一个图层，或修改为其他逻辑以选择特定图层
			return layers.first();
		}
		else {
			// 尝试处理矢量图层
			QgsVectorLayer* vectorLayer = new QgsVectorLayer(layerName, QFileInfo(layerName).fileName(), "ogr");
			if (vectorLayer->isValid()) {
				return 1;
			}
			else {
				delete vectorLayer;  // 释放无效的图层

				// 如果矢量图层无效，则尝试处理栅格图层
				QgsRasterLayer* rasterLayer = new QgsRasterLayer(layerName, QFileInfo(layerName).fileName());
				if (rasterLayer->isValid()) {
					return 1;
				}
				else {
					delete rasterLayer;  // 释放无效的栅格图层
					QMessageBox::warning(this, tr("错误"), tr("无法加载文件，请检查文件格式。"));
					return 0;
				}
			}
		}
	}
	return 1;
}



QgsMapLayer* ProjectionTransformDialog::selectedLayer() const
{
	int index = mLayerComboBox->currentIndex();
	if (index >= 0 && index < mLayers.size()) {
		return mLayers.at(index);
	}
	return nullptr;
}

QString ProjectionTransformDialog::selectedFile() const
{
	return mFileLineEdit->text();
}


QgsCoordinateReferenceSystem ProjectionTransformDialog::targetCrs() const
{
	return mCrsSelector->crs();
}

// 启用部分要素选择模式
void ProjectionTransformDialog::enableSelectionMode()
{
	// 切换到选择模式
	mIsSelectingFeatures = true;

	// 隐藏选择部分要素按钮并显示选择方式选项
	mSelectFeaturesButton->setVisible(false);
	selectModeLabel->setVisible(true);
	mSingleClickButton->setVisible(true);
	mBoxSelectButton->setVisible(true);
	mConfirmButton->setVisible(true);
	selectcancelButton->setVisible(true);
	okButton->setVisible(false);
	cancelButton->setVisible(false);
	fileLabel->setVisible(false);
	mFileLineEdit->setVisible(false);
	mSelectFileButton->setVisible(false);
	mLayerSourceComboBox->setVisible(false);
	layerSourceLabel->setVisible(false);

	// 默认选中点击选择方式
	mSingleClickButton->setChecked(true);
	activateSingleSelectTool();
}

// 激活点击选择工具
void ProjectionTransformDialog::activateSingleSelectTool()
{
	auto layer = qobject_cast<QgsVectorLayer*>(selectedLayer());
	if (!layer) return;

	auto singleClickTool = new SingleClickSelectTool(mpMapCanvas, layer);
	mpMapCanvas->setMapTool(singleClickTool);

	connect(singleClickTool, &SingleClickSelectTool::featureSelected, this, [this](const QgsFeature& feature) {
		QList<QgsFeature> features;
		features.append(feature);
		onFeaturesSelected(features);
		});
}

// 激活框选工具
void ProjectionTransformDialog::activateBoxSelectTool()
{
	auto layer = qobject_cast<QgsVectorLayer*>(selectedLayer());
	if (!layer) return;

	auto boxSelectTool = new RectangleSelectTool(mpMapCanvas, layer, this);
	mpMapCanvas->setMapTool(boxSelectTool);

	connect(boxSelectTool, &RectangleSelectTool::featuresSelected, this, &ProjectionTransformDialog::onFeaturesSelected);
}


void ProjectionTransformDialog::handleSelectionComplete() {
	QString outputPath = QFileDialog::getSaveFileName(this, tr("保存转换后的文件"), "", tr("*.shp;;*.geojson"));
	if (outputPath.isEmpty()) {
		return;  // 用户取消保存
	}

	// 打开 GDAL 数据集
	const char* driverName = "ESRI Shapefile";
	GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(driverName);
	if (poDriver == nullptr) {
		QMessageBox::warning(this, tr("错误"), tr("无法找到指定的驱动程序。"));
		return;
	}

	// 创建目标数据集
	GDALDataset* poDstDS = poDriver->Create(outputPath.toUtf8().constData(), 0, 0, 0, GDT_Unknown, nullptr);
	if (poDstDS == nullptr) {
		QMessageBox::warning(this, tr("错误"), tr("无法创建目标数据集。"));
		return;
	}

	// 获取目标坐标系
	OGRSpatialReference oTargetSRS;
	oTargetSRS.importFromWkt(targetCrs().toWkt().toUtf8().constData());

	// 创建图层
	OGRLayer* poDstLayer = poDstDS->CreateLayer("TransformedLayer", &oTargetSRS, wkbUnknown, nullptr);
	if (poDstLayer == nullptr) {
		GDALClose(poDstDS);
		QMessageBox::warning(this, tr("错误"), tr("无法创建目标图层。"));
		return;
	}

	// 遍历选中的要素并进行坐标转换
	for (const QgsFeature& feature : mSelectedFeatures) {
		OGRFeature* poDstFeature = OGRFeature::CreateFeature(poDstLayer->GetLayerDefn());

		// 设置属性字段
		for (int i = 0; i < feature.fields().size(); ++i) {
			poDstFeature->SetField(i, feature.attribute(i).toString().toUtf8().constData());
		}

		// 设置几何信息并转换坐标
		OGRGeometry* poGeometry = nullptr;
		const char* wkt = feature.geometry().asWkt().toUtf8().constData();
		OGRGeometryFactory::createFromWkt(&wkt, nullptr, &poGeometry);
		if (poGeometry != nullptr) {
			poGeometry->transformTo(&oTargetSRS);  // 转换几何坐标
			poDstFeature->SetGeometry(poGeometry);
			OGRGeometryFactory::destroyGeometry(poGeometry);
		}

		// 添加要素到目标图层
		if (poDstLayer->CreateFeature(poDstFeature) != OGRERR_NONE) {
			qDebug() << "创建要素失败。";
		}
		OGRFeature::DestroyFeature(poDstFeature);
	}

	// 关闭数据集
	GDALClose(poDstDS);

	// 加载转换后的图层
	QgsVectorLayer* transformedLayer = new QgsVectorLayer(outputPath, QFileInfo(outputPath).baseName(), "ogr");
	if (transformedLayer->isValid()) {
		QgsProject::instance()->addMapLayer(transformedLayer);
		QMessageBox::information(this, tr("投影转换成功"), tr("选中要素已成功转换并加载。"));
	}
	else {
		QMessageBox::warning(this, tr("投影转换失败"), tr("转换后的文件无效，请检查格式和路径。"));
	}

	if (mIsSelectingFeatures) {
		mpMapCanvas->unsetMapTool(mpMapCanvas->mapTool());  // 移除当前选择工具
		mIsSelectingFeatures = false;  // 重置选择模式标志
	}

	// 恢复为拖拽工具
	auto panTool = new QgsMapToolPan(mpMapCanvas);
	mpMapCanvas->setMapTool(panTool);
}

void ProjectionTransformDialog::closeEvent(QCloseEvent* event) {
	// 确保退出选择要素模式
	if (mIsSelectingFeatures) {
		mpMapCanvas->unsetMapTool(mpMapCanvas->mapTool());  // 移除当前选择工具
		mIsSelectingFeatures = false;  // 重置选择模式标志
	}

	QDialog::closeEvent(event);  // 继续执行默认关闭事件
}

void ProjectionTransformDialog::cancelSelectionMode() {
	if (mIsSelectingFeatures) {
		mpMapCanvas->unsetMapTool(mpMapCanvas->mapTool());  // 移除当前选择工具
		mIsSelectingFeatures = false;  // 重置选择模式标志
	}

	// 恢复为拖拽工具
	auto panTool = new QgsMapToolPan(mpMapCanvas);
	mpMapCanvas->setMapTool(panTool);

	// 关闭对话框
	reject();  // 关闭对话框
}

void ProjectionTransformDialog::onFeaturesSelected(const QList<QgsFeature>& features)
{
	if (features.isEmpty()) {
		QMessageBox::information(this, tr("选择要素"), tr("未选择任何要素。"));
		return;
	}

	mSelectedFeatures.append(features);
	QMessageBox::information(this, tr("选择要素"), tr("已选中 %1 个要素").arg(features.size()));
	handleSelectionComplete();
}