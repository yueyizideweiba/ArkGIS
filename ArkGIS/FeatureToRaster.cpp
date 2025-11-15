#include "FeatureToRaster.h"

FeatureToRaster::FeatureToRaster(QWidget* parent)
	: QDialog(parent), mMainWindow(mMainWindow)
{
	setWindowTitle(tr("要素转栅格"));
	setFixedSize(500, 500);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	// 输入要素选择
	QLabel* layerLabel = new QLabel(tr("输入要素:"));
	mLayerComboBox = new QComboBox();
	const auto layers = QgsProject::instance()->mapLayers();
	for (auto layer : layers) {
		if (layer->type() == Qgis::LayerType::Vector) {
			mLayerComboBox->addItem(layer->name(), QVariant::fromValue(layer));
		}
	}
	connect(mLayerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FeatureToRaster::updateFields);

	mainLayout->addWidget(layerLabel);
	mainLayout->addWidget(mLayerComboBox);

	// 字段选择
	QLabel* fieldLabel = new QLabel(tr("字段:"));
	mFieldComboBox = new QComboBox();
	mainLayout->addWidget(fieldLabel);
	mainLayout->addWidget(mFieldComboBox);

	// 输出栅格路径
	QLabel* outputLabel = new QLabel(tr("输出栅格:"));
	QHBoxLayout* outputLayout = new QHBoxLayout();
	mOutputFileLineEdit = new QLineEdit();
	mBrowseButton = new QPushButton(tr("浏览"));
	connect(mBrowseButton, &QPushButton::clicked, this, &FeatureToRaster::selectOutputFile);

	outputLayout->addWidget(mOutputFileLineEdit);
	outputLayout->addWidget(mBrowseButton);

	mainLayout->addWidget(outputLabel);
	mainLayout->addLayout(outputLayout);

	// 像元大小设置
	QLabel* cellSizeLabel = new QLabel(tr("像元大小:"));
	mCellSizeSpinBox = new QDoubleSpinBox();
	mCellSizeSpinBox->setRange(0.1, 1000.0);
	mCellSizeSpinBox->setValue(10.0); // 默认值
	mainLayout->addWidget(cellSizeLabel);
	mainLayout->addWidget(mCellSizeSpinBox);

	// 确定和取消按钮
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	mOkButton = new QPushButton(tr("确定"));
	mCancelButton = new QPushButton(tr("取消"));

	connect(mOkButton, &QPushButton::clicked, this, &FeatureToRaster::convertToRaster);
	connect(mCancelButton, &QPushButton::clicked, this, &QDialog::reject);

	buttonLayout->addWidget(mOkButton);
	buttonLayout->addWidget(mCancelButton);

	mainLayout->addLayout(buttonLayout);

	updateFields(); // 初始化字段列表
}

bool FeatureToRaster::execute()
{
	return exec() == QDialog::Accepted;
}

void FeatureToRaster::updateFields()
{
	mFieldComboBox->clear();
	QgsVectorLayer* layer = mLayerComboBox->currentData().value<QgsVectorLayer*>();
	if (layer) {
		for (const QgsField& field : layer->fields()) {
			mFieldComboBox->addItem(field.name());
		}
	}
}

void FeatureToRaster::selectOutputFile()
{
	QString filePath = QFileDialog::getSaveFileName(this, tr("选择输出栅格文件"), "", tr("TIFF 文件 (*.tif)"));
	if (!filePath.isEmpty()) {
		mOutputFileLineEdit->setText(filePath);
	}
}

void FeatureToRaster::convertToRaster()
{
	QgsVectorLayer* vectorLayer = mLayerComboBox->currentData().value<QgsVectorLayer*>();
	QString field = mFieldComboBox->currentText();
	QString outputFilePath = mOutputFileLineEdit->text();
	double resolution = mCellSizeSpinBox->value();

	if (!vectorLayer || field.isEmpty() || outputFilePath.isEmpty()) {
		QMessageBox::warning(this, tr("错误"), tr("请正确填写所有参数！"));
		return;
	}

	if (convertFeatureToRaster(vectorLayer, field, outputFilePath, resolution)) {
		QMessageBox::information(this, tr("成功"), tr("要素成功转换为栅格！"));
		accept();
	}
	else {
		QMessageBox::warning(this, tr("错误"), tr("要素转栅格失败！"));
	}
}

bool FeatureToRaster::convertFeatureToRaster(QgsVectorLayer* vectorLayer, const QString& fieldName, const QString& outputFilePath, double resolution) {

	return true;
}



QColor FeatureToRaster::getColorForValue(const QVariant& value) {
	if (value.canConvert<double>()) {
		double val = value.toDouble();
		int r = static_cast<int>(std::fmod(val * 50, 255));
		int g = static_cast<int>(std::fmod(val * 30, 255));
		int b = static_cast<int>(std::fmod(val * 10, 255));
		return QColor(r, g, b);
	}
	return QColor(Qt::black); // 默认黑色
}

void FeatureToRaster::drawLineOnRaster(QImage& raster, const QgsPointXY& start, const QgsPointXY& end,
	double xmin, double ymax, double resolution, const QColor& color) {
	// 将线段投影到栅格上
	int x1 = static_cast<int>((start.x() - xmin) / resolution);
	int y1 = static_cast<int>((ymax - start.y()) / resolution);
	int x2 = static_cast<int>((end.x() - xmin) / resolution);
	int y2 = static_cast<int>((ymax - end.y()) / resolution);

	int dx = std::abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
	int dy = -std::abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
	int err = dx + dy, e2;

	while (true) {
		raster.setPixel(x1, y1, color.rgb());
		if (x1 == x2 && y1 == y2) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x1 += sx; }
		if (e2 <= dx) { err += dx; y1 += sy; }
	}
}

void FeatureToRaster::drawPolygonOnRaster(QImage& raster, const QgsPolylineXY& ring,
	double xmin, double ymax, double resolution, const QColor& color) {
	// 使用扫描线算法填充多边形
	QVector<QPoint> points;
	for (const auto& vertex : ring) {
		int col = static_cast<int>((vertex.x() - xmin) / resolution);
		int row = static_cast<int>((ymax - vertex.y()) / resolution);
		points.append(QPoint(col, row));
	}

	QPainter painter(&raster);
	painter.setBrush(QBrush(color));
	painter.setPen(Qt::NoPen);
	painter.drawPolygon(points);
}


bool FeatureToRaster::validateLayer(QgsVectorLayer* layer)
{
	if (!layer || !layer->isValid() || layer->type() != Qgis::LayerType::Vector) {
		return false;
	}
	return true;
}
