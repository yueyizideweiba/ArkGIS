#pragma warning(disable:4996)
#include "VectorAnalysis.h"
#include <QgsVectorLayer.h>
#include <QgsFeature.h>
#include <QgsGeometry.h>
#include <QgsSpatialIndex.h>
#include <QDialogButtonBox.h>
#include <QVBoxLayout>
#include <QComboBox.h>
#include <QLabel.h>
#include <QSpinBox.h>
#include <QFileDialog.h>
#include <QMessageBox.h>
#include <QProgressDialog.h>
#include <random>
#include <QgsProject.h>
#include <QgsField.h>
#include <QgsCategorizedSymbolRenderer.h>
#include <QgsVectorFileWriter.h>
#include <QtMath>
#include <QgsSymbol.h>
//-----------------------------------------裁剪-------------------------------------------------------------------------------------
#include"myQgsMapTool.h"
#include <QgsGeometry.h>
#include <QgsPolygon.h>
#include <qgscircle.h>
#include <QgsWkbTypes.h>
#include <QgsGeometry.h>
#include <QgsPointXY.h>
#include <cmath>
#include <QgsProject.h>
#include <QgsVectorLayer.h>
#include <QMessageBox>
#include <QVector>
#include <QgsFeature.h>
#include <QgsFeatureIterator.h>
#include <QgsVectorFileWriter.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QgsProject.h>
#include <QgsMapLayer.h>
#include <qgsmessagelog.h>   // 用于日志输出
//-----------------------------------------裁剪-------------------------------------------------------------------------------------
VectorAnalysis::VectorAnalysis(QgsMapCanvas* canvas, QObject* parent)
	: QObject(parent), mpMapCanvas(canvas) {
}

// 进行K值聚类分析
void VectorAnalysis::performKMeansClustering()
{
	QDialog dialog;
	dialog.resize(350, 150);
	dialog.setWindowTitle(tr("选择图层和类别数"));
	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	QLabel* label = new QLabel(tr("请选择点图层："), &dialog);
	layout->addWidget(label);

	// 创建图层下拉框并获取点图层
	QComboBox* comboBox = new QComboBox(&dialog);
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values())
	{
		if (l->type() == Qgis::LayerType::Vector && static_cast<QgsVectorLayer*>(l)->geometryType() == Qgis::GeometryType::Point)
		{
			comboBox->addItem(l->name(), l->id());
		}
	}
	layout->addWidget(comboBox);

	// 创建类别数输入框并
	QLabel* kLabel = new QLabel(tr("请输入聚类类别数："), &dialog);
	layout->addWidget(kLabel);

	QSpinBox* kSpinBox = new QSpinBox(&dialog);
	// 设置聚类类别数的范围和默认值
	kSpinBox->setRange(2, 20);
	kSpinBox->setValue(5);
	layout->addWidget(kSpinBox);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 如果选择确认
	if (dialog.exec() == QDialog::Accepted)
	{
		QString layerId = comboBox->currentData().toString();
		int k = kSpinBox->value();

		// 获取选中的矢量图层
		QgsVectorLayer* layer = dynamic_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayer(layerId));
		if (!layer)
		{
			QMessageBox::warning(nullptr, tr("错误"), tr("无法找到选择的图层。"));
			return;
		}

		// 调用K均值聚类函数
		applyKMeansClustering(layer, k);
	}
}

// 实现K均值聚类算法
void VectorAnalysis::applyKMeansClustering(QgsVectorLayer* layer, int k)
{
	// 创建进度条对话框
	QProgressDialog* progressDialog = new QProgressDialog(tr("正在进行聚类分析..."), tr("取消"), 0, 100);
	progressDialog->setWindowModality(Qt::WindowModal);
	progressDialog->resize(300, 150);
	progressDialog->setFixedSize(progressDialog->size());
	progressDialog->show();

	// 获取所有要素
	QVector<QgsFeature> features;
	QgsFeatureIterator it = layer->getFeatures();
	QgsFeature feature;
	while (it.nextFeature(feature))
	{
		features.append(feature);
	}

	if (features.size() < k)
	{
		QMessageBox::warning(nullptr, tr("错误"), tr("点的数量少于聚类的类别数。"));
		return;
	}

	// 初始化聚类中心
	QVector<QgsPointXY> centroids;
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, features.size() - 1);
	for (int i = 0; i < k; ++i)
	{
		centroids.append(features[distribution(generator)].geometry().asPoint());
	}

	QVector<int> labels(features.size(), -1);
	bool changed;

	do
	{
		changed = false;

		// 重新分配每个点的标签
		for (int i = 0; i < features.size(); ++i)
		{
			double minDist = std::numeric_limits<double>::max();
			int label = -1;

			for (int j = 0; j < k; ++j)
			{
				double dist = qSqrt(qPow(features[i].geometry().asPoint().x() - centroids[j].x(), 2) +
					qPow(features[i].geometry().asPoint().y() - centroids[j].y(), 2));
				if (dist < minDist)
				{
					minDist = dist;
					label = j;
				}
			}

			if (labels[i] != label)
			{
				labels[i] = label;
				changed = true;
			}
		}

		// 如果标签有变化，更新聚类中心
		if (changed)
		{
			QVector<int> counts(k, 0);
			centroids.fill(QgsPointXY());

			for (int i = 0; i < features.size(); ++i)
			{
				centroids[labels[i]].setX(centroids[labels[i]].x() + features[i].geometry().asPoint().x());
				centroids[labels[i]].setY(centroids[labels[i]].y() + features[i].geometry().asPoint().y());
				counts[labels[i]] += 1;
			}

			for (int j = 0; j < k; ++j)
			{
				if (counts[j] != 0)
				{
					centroids[j].setX(centroids[j].x() / counts[j]);
					centroids[j].setY(centroids[j].y() / counts[j]);
				}
			}
		}

		// 更新进度条
		progressDialog->setValue(progressDialog->value() + (changed ? 1 : 100));
		qApp->processEvents();

		if (progressDialog->wasCanceled())
		{
			return;
		}

	} while (changed);

	progressDialog->setValue(100);
	progressDialog->close();
	delete progressDialog;

	// 创建聚类结果图层
	QString newLayerName = layer->name() + "_clustered";
	QMessageBox::StandardButton reply;
	// 询问是否需要保存对话框
	reply = QMessageBox::question(nullptr, tr("保存聚类结果"), tr("是否需要保存聚类结果到文件？"),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

	// 如果需要保存结果到文件则获取路径并进行保存
	if (reply == QMessageBox::Yes)
	{
		QString defaultPath = "../Results"; // 设置默认保存路径
		QString fileName = QFileDialog::getSaveFileName(nullptr, tr("保存聚类结果"), defaultPath, tr("Shapefile (*.shp)"));

		if (fileName.isEmpty())
		{
			return;
		}

		if (!fileName.endsWith(".shp"))
		{
			fileName += ".shp";
		}

		// 添加CLUSTER_ID字段
		QgsFields fields = layer->fields();
		fields.append(QgsField("CLUSTER_ID", QVariant::Int));

		// 创建新矢量图层
		QgsVectorLayer* newLayer = new QgsVectorLayer("Point?crs=" + layer->crs().authid(), newLayerName, "memory");
		newLayer->dataProvider()->addAttributes(fields.toList());  // 添加字段
		newLayer->updateFields();  // 更新字段

		// 为每个要素设置几何和属性
		QgsFeature newFeature;
		QgsAttributes attributes;
		for (int i = 0; i < features.size(); ++i)
		{
			newFeature.setGeometry(features[i].geometry());
			attributes = features[i].attributes();
			attributes.append(labels[i]);  // 添加聚类标签
			newFeature.setAttributes(attributes);
			newLayer->dataProvider()->addFeature(newFeature);  // 添加要素到新图层
		}

		// 将新矢量图层保存为shp格式
		QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormat(
			newLayer,
			fileName,
			"UTF-8",
			newLayer->crs(),
			"ESRI Shapefile"
		);

		if (error != QgsVectorFileWriter::NoError)
		{
			QMessageBox::critical(nullptr, tr("错误"), tr("无法保存聚类结果图层"));
			return;
		}

		// 加载保存的图层到当前工程
		QgsVectorLayer* savedLayer = new QgsVectorLayer(fileName, newLayerName, "ogr");
		if (savedLayer->isValid())
		{
			QgsProject::instance()->addMapLayer(savedLayer);
			mpMapCanvas->refresh();

			// 创建分类符号化渲染器
			QList<QgsRendererCategory> categories;
			for (int i = 0; i < k; ++i)
			{
				QgsSymbol* symbol = QgsSymbol::defaultSymbol(savedLayer->geometryType());
				symbol->setColor(QColor::fromHsv(i * 360 / k, 255, 255));
				categories.append(QgsRendererCategory(i, symbol, QString("Cluster %1").arg(i)));
			}

			// 应用分类符号化渲染器
			QgsCategorizedSymbolRenderer* renderer = new QgsCategorizedSymbolRenderer("CLUSTER_ID", categories);
			savedLayer->setRenderer(renderer);
			savedLayer->triggerRepaint(); // 重绘图层
		}
		else
		{
			QMessageBox::critical(nullptr, tr("错误"), tr("无法加载聚类结果图层"));
		}
	}
	// 如果用户选择不保存嗯，则只是添加图层，流程同上
	else if (reply == QMessageBox::No)
	{
		QgsVectorLayer* newLayer = new QgsVectorLayer("Point?crs=" + layer->crs().authid(), newLayerName, "memory");
		QgsFields fields = layer->fields();
		fields.append(QgsField("CLUSTER_ID", QVariant::Int));
		newLayer->dataProvider()->addAttributes(fields.toList());
		newLayer->updateFields();

		QgsFeature newFeature;
		QgsAttributes attributes;
		for (int i = 0; i < features.size(); ++i)
		{
			newFeature.setGeometry(features[i].geometry());
			attributes = features[i].attributes();
			attributes.append(labels[i]);
			newFeature.setAttributes(attributes);
			newLayer->dataProvider()->addFeature(newFeature);
		}

		QgsProject::instance()->addMapLayer(newLayer);
		mpMapCanvas->refresh();

		QList<QgsRendererCategory> categories;
		for (int i = 0; i < k; ++i)
		{
			QgsSymbol* symbol = QgsSymbol::defaultSymbol(newLayer->geometryType());
			symbol->setColor(QColor::fromHsv(i * 360 / k, 255, 255));
			categories.append(QgsRendererCategory(i, symbol, QString("Cluster %1").arg(i)));
		}

		QgsCategorizedSymbolRenderer* renderer = new QgsCategorizedSymbolRenderer("CLUSTER_ID", categories);
		newLayer->setRenderer(renderer);
		newLayer->triggerRepaint();
	}
}

// 进行按位置连接属性操作
void VectorAnalysis::performSpatialJoin()
{
	QDialog dialog;
	dialog.resize(350, 150);
	dialog.setWindowTitle(tr("选择图层进行按位置连接属性"));

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	QLabel* poiLabel = new QLabel(tr("请选择POI点图层："), &dialog);
	layout->addWidget(poiLabel);

	// 创建POI点图层下拉框并添加到布局
	QComboBox* poiComboBox = new QComboBox(&dialog);
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values())
	{
		if (l->type() == Qgis::LayerType::Vector && static_cast<QgsVectorLayer*>(l)->geometryType() == Qgis::GeometryType::Point)
		{
			poiComboBox->addItem(l->name(), l->id());
		}
	}
	layout->addWidget(poiComboBox);

	QLabel* polygonLabel = new QLabel(tr("请选择面图层："), &dialog);
	layout->addWidget(polygonLabel);

	// 创建面图层下拉框并添加到布局
	QComboBox* polygonComboBox = new QComboBox(&dialog);
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values())
	{
		if (l->type() == Qgis::LayerType::Vector && static_cast<QgsVectorLayer*>(l)->geometryType() == Qgis::GeometryType::Polygon)
		{
			polygonComboBox->addItem(l->name(), l->id());
		}
	}
	layout->addWidget(polygonComboBox);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 如果确认要进行操作
	if (dialog.exec() == QDialog::Accepted)
	{
		// 获取选中的POI点图层ID和面图层
		QString poiLayerId = poiComboBox->currentData().toString();
		QString polygonLayerId = polygonComboBox->currentData().toString();
		QgsVectorLayer* poiLayer = dynamic_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayer(poiLayerId));
		QgsVectorLayer* polygonLayer = dynamic_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayer(polygonLayerId));

		if (!poiLayer || !polygonLayer)
		{
			QMessageBox::warning(nullptr, tr("错误"), tr("无法找到选择的图层。"));
			return;
		}

		// 调用实现按位置连接属性函数
		applySpatialJoin(poiLayer, polygonLayer);
	}
}

// 实现按位置连接属性函数
void VectorAnalysis::applySpatialJoin(QgsVectorLayer* poiLayer, QgsVectorLayer* polygonLayer)
{
	QgsFields fields = polygonLayer->fields();
	fields.append(QgsField("PoiId", QVariant::Int));

	// 创建新矢量图层
	QString newLayerName = polygonLayer->name() + "_joined";
	QgsVectorLayer* newLayer = new QgsVectorLayer("Polygon?crs=" + polygonLayer->crs().authid(), newLayerName, "memory");
	newLayer->dataProvider()->addAttributes(fields.toList());  // 添加字段
	newLayer->updateFields();  // 更新字段

	// 获取面图层的要素迭代器，创建poi图层空间索引
	QgsFeatureIterator polygonIt = polygonLayer->getFeatures();
	QgsFeature polygonFeature;
	QgsSpatialIndex poiIndex(poiLayer->getFeatures());

	// 遍历面图层的要素
	while (polygonIt.nextFeature(polygonFeature))
	{
		QgsGeometry polygonGeom = polygonFeature.geometry();

		// 查找与面要素相交的POI要素ID
		QList<QgsFeatureId> intersectedPoiIds = poiIndex.intersects(polygonGeom.boundingBox());

		bool found = false;
		for (const QgsFeatureId& id : intersectedPoiIds)
		{
			QgsFeature poiFeature;
			QgsFeatureIterator poiIt = poiLayer->getFeatures(QgsFeatureRequest().setFilterFid(id));
			if (poiIt.nextFeature(poiFeature))
			{
				// 如果找到相交的POI要素，创建新的面要素并添加到新图层
				if (polygonGeom.contains(poiFeature.geometry()))
				{
					QgsFeature newFeature(polygonFeature);
					newFeature.setFields(fields, true);
					newFeature.setAttribute("PoiId", poiFeature.id());
					newLayer->dataProvider()->addFeature(newFeature);
					found = true;
					break;
				}
			}
		}
		// 如果没有找到相交的POI要素，创建新的面要素并添加到新图层
		if (!found)
		{
			QgsFeature newFeature(polygonFeature);
			newFeature.setFields(fields, true);
			newFeature.setAttribute("PoiId", QVariant());
			newLayer->dataProvider()->addFeature(newFeature);
		}
	}

	// 询问用户是否保存连接结果到文件
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(nullptr, tr("保存连接结果"), tr("是否需要保存连接结果到文件？"),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

	if (reply == QMessageBox::Yes)
	{
		QString defaultPath = "../Results"; // 设置默认保存路径
		QString fileName = QFileDialog::getSaveFileName(nullptr, tr("保存连接结果"), defaultPath, tr("Shapefile (*.shp)"));

		if (fileName.isEmpty())
		{
			return;
		}

		// 保存为shp文件
		QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormat(
			newLayer,
			fileName,
			"UTF-8",
			newLayer->crs(),
			"ESRI Shapefile"
		);

		if (error != QgsVectorFileWriter::NoError)
		{
			QMessageBox::critical(nullptr, tr("错误"), tr("无法保存连接结果图层"));
			return;
		}

		// 加载保存的图层到当前工程中
		QgsVectorLayer* savedLayer = new QgsVectorLayer(fileName, newLayerName, "ogr");
		if (savedLayer->isValid())
		{
			QgsProject::instance()->addMapLayer(savedLayer);
			mpMapCanvas->refresh();
		}
		else
		{
			QMessageBox::critical(nullptr, tr("错误"), tr("无法加载连接结果图层"));
		}
	}
	// 若选择不保存则直接将新图层添加到画布
	else if (reply == QMessageBox::No)
	{
		QgsProject::instance()->addMapLayer(newLayer);
		mpMapCanvas->refresh();
	}
}

//实现表转矢量点
void VectorAnalysis::performExcelToShp() {
	// 1. 打开文件对话框选择CSV文件
	QString csvFilePath = QFileDialog::getOpenFileName(nullptr, "选择CSV文件", "", "CSV Files (*.csv)");
	if (csvFilePath.isEmpty()) {
		return;  // 用户取消了选择
	}

	// 2. 打开并读取CSV文件
	QFile csvFile(csvFilePath);
	if (!csvFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::critical(nullptr, "错误", "无法打开CSV文件!");
		return;
	}

	QTextStream in(&csvFile);
	QStringList columnNames;
	QVector<QStringList> rows;

	bool firstLine = true;
	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList fields = line.split(",", Qt::SkipEmptyParts);  // 获取每一行的数据

		if (firstLine) {
			columnNames = fields;  // 第一行是表头
			firstLine = false;
		}
		else {
			rows.append(fields);  // 存储除第一行外的每一行
		}
	}

	if (columnNames.isEmpty() || rows.isEmpty()) {
		QMessageBox::critical(nullptr, "错误", "CSV文件为空或没有有效数据！");
		return;
	}

	// 3. 弹出对话框动态选择X列和Y列
	QDialog dialog;
	dialog.resize(350, 200);
	dialog.setWindowTitle(tr("选择X列和Y列"));

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	// 添加X列选择的控件
	QLabel* xLabel = new QLabel(tr("请选择X坐标列："), &dialog);
	layout->addWidget(xLabel);

	QComboBox* xComboBox = new QComboBox(&dialog);
	xComboBox->addItems(columnNames);  // 添加CSV列名到下拉框
	layout->addWidget(xComboBox);

	// 添加Y列选择的控件
	QLabel* yLabel = new QLabel(tr("请选择Y坐标列："), &dialog);
	layout->addWidget(yLabel);

	QComboBox* yComboBox = new QComboBox(&dialog);
	yComboBox->addItems(columnNames);  // 添加CSV列名到下拉框
	layout->addWidget(yComboBox);

	// 添加对话框按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	// 连接按钮事件
	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 显示对话框并处理用户选择
	if (dialog.exec() == QDialog::Accepted) {
		QString selectedXColumn = xComboBox->currentText();
		QString selectedYColumn = yComboBox->currentText();

		// 验证用户选择的列是否有效
		int xIndex = columnNames.indexOf(selectedXColumn);
		int yIndex = columnNames.indexOf(selectedYColumn);

		if (xIndex == -1 || yIndex == -1) {
			QMessageBox::critical(nullptr, "错误", "选择的X或Y列无效！");
			return;
		}

		// 使用选中的列索引继续执行后续逻辑
		qDebug() << "X列：" << selectedXColumn << "索引：" << xIndex;
		qDebug() << "Y列：" << selectedYColumn << "索引：" << yIndex;

		// 4. 弹出保存对话框选择Shapefile保存路径
		QString shpFilePath = QFileDialog::getSaveFileName(nullptr, "保存为Shapefile", "", "Shapefile (*.shp)");
		if (shpFilePath.isEmpty()) {
			return;  // 用户取消了选择
		}

		// 获取文件名（不包括路径和扩展名）
		QString layerName = QFileInfo(shpFilePath).baseName();  // 提取文件名（去除路径和扩展名）

		// 5. 创建初始图层
		QgsVectorLayer* baseLayer = new QgsVectorLayer("Point?crs=EPSG:4326", "CSV点图层", "memory");
		if (!baseLayer->isValid()) {
			QMessageBox::critical(nullptr, "错误", "无法创建内存矢量图层！");
			return;
		}

		// 6. 定义字段，动态添加CSV文件中的所有字段
		QgsFields fields;
		fields.append(QgsField("ID", QVariant::Int));  // 保留ID字段

		// 动态添加CSV中所有的字段
		for (const QString& columnName : columnNames) {
			fields.append(QgsField(columnName, QVariant::String));  // 可以根据实际情况调整字段类型
		}

		// 创建新图层，并使用用户输入的文件名作为图层名称
		QgsVectorLayer* newLayer = new QgsVectorLayer("Point?crs=" + baseLayer->crs().authid(), layerName, "memory");
		QgsVectorDataProvider* newProvider = newLayer->dataProvider();
		newProvider->addAttributes(fields.toList());  // 添加字段
		newLayer->updateFields();  // 更新字段

		// 7. 为每个要素设置几何和属性
		int id = 1;  // 唯一的ID
		QgsFeatureList feaList;
		for (const QStringList& row : rows) {
			if (row.size() > std::max(xIndex, yIndex)) {
				bool xOk, yOk;
				double x = row[xIndex].toDouble(&xOk);
				double y = row[yIndex].toDouble(&yOk);

				if (xOk && yOk) {
					QgsFeature feature;
					feature.setGeometry(QgsGeometry::fromPointXY(QgsPointXY(x, y)));

					QgsAttributes attributes;
					attributes.append(QVariant(id++));  // 设置ID字段

					// 添加CSV文件中其他字段的值
					for (int i = 0; i < columnNames.size(); ++i) {
						attributes.append(QVariant(row[i]));
					}

					feature.setAttributes(attributes);

					// 添加到要素列表
					feaList.append(feature);
				}
				else {
					qWarning() << "无效的坐标值：" << row[xIndex] << ", " << row[yIndex];
				}
			}
		}

		// 批量添加要素到新图层
		if (!newProvider->addFeatures(feaList)) {
			QMessageBox::critical(nullptr, "错误", "要素添加失败！");
			return;
		}

		// 更新图层范围和字段
		newLayer->updateExtents();
		newLayer->updateFields();

		// 8. 将新图层保存为Shapefile
		QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormat(
			newLayer,
			shpFilePath,
			"UTF-8",
			newLayer->crs(),
			"ESRI Shapefile"
		);

		if (error != QgsVectorFileWriter::NoError) {
			QMessageBox::critical(nullptr, "错误", "保存Shapefile失败！");
			return;
		}

		QMessageBox::information(nullptr, "成功", "Shapefile已成功创建！");

		QgsProject::instance()->addMapLayer(newLayer);
		QMessageBox::information(nullptr, "成功", "矢量文件已加载！");
	}
	else {
		// 用户取消了对话框
		return;
	}
}
//-----------------------------------------裁剪-------------------------------------------------------------------------------------
//4.2矢量裁剪分析：分别用矩形、圆和任意多边形对矢量图层进行裁剪，并将裁剪结果输出到矢量文件。

//功能矢量裁剪1：矢量图层之间的裁剪

// 创建矢量裁剪分析窗口
void VectorAnalysis::performVectorClipping()
{
	QDialog dialog;
	dialog.resize(350, 150);
	dialog.setWindowTitle(tr("选择裁剪方式"));

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	QLabel* layerLabel = new QLabel(tr("请选择要裁剪的矢量图层："), &dialog);
	layout->addWidget(layerLabel);

	// 创建矢量图层下拉框并添加到布局
	QComboBox* layerComboBox = new QComboBox(&dialog);
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values())
	{
		if (l->type() == Qgis::LayerType::Vector)
		{
			layerComboBox->addItem(l->name(), l->id());
		}
	}
	layout->addWidget(layerComboBox);

	QLabel* polygonLabel = new QLabel(tr("请选择裁剪多边形图层："), &dialog);
	layout->addWidget(polygonLabel);

	// 创建多边形图层下拉框并添加到布局
	QComboBox* polygonComboBox = new QComboBox(&dialog);
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values())
	{
		if (l->type() == Qgis::LayerType::Vector && static_cast<QgsVectorLayer*>(l)->geometryType() == Qgis::GeometryType::Polygon)
		{
			polygonComboBox->addItem(l->name(), l->id());
		}
	}
	layout->addWidget(polygonComboBox);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 如果确认要进行操作
	if (dialog.exec() == QDialog::Accepted)
	{
		// 获取选中的图层ID
		QString selectedLayerId = layerComboBox->currentData().toString();
		QString polygonLayerId = polygonComboBox->currentData().toString();
		QgsVectorLayer* selectedLayer = dynamic_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayer(selectedLayerId));
		QgsVectorLayer* polygonLayer = dynamic_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayer(polygonLayerId));

		if (!selectedLayer || !polygonLayer)
		{
			QMessageBox::warning(nullptr, tr("错误"), tr("无法找到选择的图层。"));
			return;
		}

		// 自动识别被裁剪图层的几何类型
		Qgis::GeometryType selectedGeometryType = selectedLayer->geometryType();

		// 调用实际裁剪操作
		vectorClipping(selectedLayer, polygonLayer, selectedGeometryType);
	}
}
// 图层裁剪并输出
void VectorAnalysis::vectorClipping(QgsVectorLayer* selectedLayer, QgsVectorLayer* polygonLayer, Qgis::GeometryType geometryType)
{
	// 执行裁剪操作
	QgsVectorLayer* clippedLayer = clipVectorLayer(selectedLayer, polygonLayer, geometryType);

	if (clippedLayer)
	{
		// 直接将裁剪后的图层添加到项目中
		QgsProject::instance()->addMapLayer(clippedLayer);
		QMessageBox::information(nullptr, tr("成功"), tr("裁剪结果已添加到当前项目。"));
	}
	else
	{
		QMessageBox::warning(nullptr, tr("错误"), tr("裁剪操作失败。"));
	}
}
// 图层裁剪实现
QgsVectorLayer* VectorAnalysis::clipVectorLayer(QgsVectorLayer* selectedLayer, QgsVectorLayer* polygonLayer, Qgis::GeometryType geometryType)
{
	QgsFeatureRequest request;
	QgsVectorLayer* clippedLayer = nullptr;

	// 创建新的图层根据几何类型
	switch (geometryType)
	{
	case Qgis::GeometryType::Point:
		clippedLayer = new QgsVectorLayer("Point?crs=" + selectedLayer->crs().toWkt(), "Clipped Points", "memory");
		break;
	case Qgis::GeometryType::Line:
		clippedLayer = new QgsVectorLayer("LineString?crs=" + selectedLayer->crs().toWkt(), "Clipped Lines", "memory");
		break;
	case Qgis::GeometryType::Polygon:
		clippedLayer = new QgsVectorLayer("Polygon?crs=" + selectedLayer->crs().toWkt(), "Clipped Polygons", "memory");
		break;
	default:
		return nullptr; // 不支持其他类型
	}

	QgsFeatureIterator iter = selectedLayer->getFeatures(request);
	QgsFeature feature;

	// 遍历要裁剪的图层要素
	while (iter.nextFeature(feature))
	{
		// 获取多边形图层的要素
		QgsFeatureIterator polygonIter = polygonLayer->getFeatures(request);
		QgsFeature polygonFeature;

		// 遍历所有裁剪多边形要素
		while (polygonIter.nextFeature(polygonFeature))
		{
			// 进行实际的几何裁剪：获取要素与多边形的交集部分
			QgsGeometry intersection = feature.geometry().intersection(polygonFeature.geometry());

			if (!intersection.isEmpty())
			{
				// 如果交集非空，添加到新的图层
				QgsFeature clippedFeature;
				clippedFeature.setGeometry(intersection);
				clippedLayer->dataProvider()->addFeature(clippedFeature);
			}
		}
	}

	return clippedLayer;
}


//功能矢量裁剪2：自定义窗口对矢量图层的裁剪

//创建矩形裁剪工具
QgsGeometry VectorAnalysis::createRectangleGeometry() {
	QgsMapToolRectangle* mapTool = new QgsMapToolRectangle(mpMapCanvas, this);  // 使用 QgsMapToolRectangle 类型
	mpMapCanvas->setMapTool(mapTool);

	// 等待用户绘制矩形
	while (!mapTool->isCanceled()) {  // 通过 mapTool 调用 isCanceled
		QCoreApplication::processEvents();
	}

	// 获取矩形的几何
	QgsGeometry geometry = mapTool->geometry();

	// 清理资源，删除 mapTool 对象
	delete mapTool;

	// 返回创建的矩形几何体
	return geometry;
}

//创建圆裁剪工具
QgsGeometry VectorAnalysis::createCircleGeometry() {
	// 创建一个圆形工具实例，传入地图画布和当前的VectorAnalysis父对象
	QgsMapToolCircle* mapTool = new QgsMapToolCircle(mpMapCanvas, this);

	// 设置该工具为当前活动工具，开始等待用户输入
	mpMapCanvas->setMapTool(mapTool);

	// 等待直到用户完成圆形绘制操作
	while (!mapTool->isCanceled()) {
		QCoreApplication::processEvents();  // 处理事件，保持界面响应
	}

	// 获取圆心和半径
	QgsPointXY center = mapTool->center();
	double radius = mapTool->radius();

	QgsGeometry geometry = mapTool->geometry();

	// 清理资源，删除mapTool对象
	delete mapTool;

	// 返回创建的圆形几何体
	return geometry;
}

//创建多边形裁剪工具
QgsGeometry VectorAnalysis::createPolygonGeometry() {
	// 创建一个多边形裁剪工具
	QgsMapToolPolygon* mapTool = new QgsMapToolPolygon(mpMapCanvas, this);
	mpMapCanvas->setMapTool(mapTool);

	// 等待用户绘制多边形
	while (!mapTool->isCanceled()) {
		QCoreApplication::processEvents();  // 处理事件，保持UI响应
	}

	// 获取裁剪工具绘制的几何体
	QgsGeometry geometry = mapTool->geometry();

	// 清理工作，删除 mapTool 防止内存泄漏
	delete mapTool;

	// 返回用户绘制的多边形几何体
	return geometry;
}

//选择裁剪工具与被裁剪文件
void VectorAnalysis::performCustomVectorClipping() {
	// 创建矢量裁剪分析窗口
	QDialog dialog;
	dialog.resize(350, 250);
	dialog.setWindowTitle(tr("选择自定义裁剪方式"));
	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	// 选择要裁剪的矢量图层
	QLabel* layerLabel = new QLabel(tr("请选择要裁剪的矢量图层："), &dialog);
	layout->addWidget(layerLabel);

	QComboBox* layerComboBox = new QComboBox(&dialog);
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values()) {
		if (l->type() == Qgis::LayerType::Vector) {
			layerComboBox->addItem(l->name(), l->id());
		}
	}
	layout->addWidget(layerComboBox);

	// 选择裁剪工具（矩形、圆形、任意多边形）
	QLabel* toolLabel = new QLabel(tr("请选择裁剪工具："), &dialog);
	layout->addWidget(toolLabel);

	QComboBox* toolComboBox = new QComboBox(&dialog);
	toolComboBox->addItem(tr("矩形"));
	toolComboBox->addItem(tr("圆形"));
	toolComboBox->addItem(tr("任意多边形"));
	layout->addWidget(toolComboBox);

	// 添加确认和取消按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	// 连接按钮事件
	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 显示对话框
	if (dialog.exec() == QDialog::Accepted) {
		// 获取用户选择的图层ID和裁剪工具
		QString layerId = layerComboBox->currentData().toString();
		QString toolType = toolComboBox->currentText();
		QgsVectorLayer* layer = dynamic_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayer(layerId));

		if (!layer) {
			QMessageBox::warning(nullptr, tr("错误"), tr("无法找到选择的图层。"));
			return;
		}

		// 创建裁剪工具实例（根据选择的工具类型）
		QgsGeometry clipGeometry;
		if (toolType == tr("矩形")) {
			clipGeometry = createRectangleGeometry();  // 创建矩形裁剪区域
		}
		else if (toolType == tr("圆形")) {
			clipGeometry = createCircleGeometry();  // 创建圆形裁剪区域
		}
		else if (toolType == tr("任意多边形")) {
			clipGeometry = createPolygonGeometry();  // 创建多边形裁剪区域
		}

		// 执行裁剪
		QgsVectorLayer* clippedLayer = customClipVectorLayer(layer, clipGeometry);

		// 将裁剪后的图层添加到项目中
		QgsProject::instance()->addMapLayer(clippedLayer);
	}
}
//自定义裁剪框的矢量图层裁剪实现
QgsVectorLayer* VectorAnalysis::customClipVectorLayer(QgsVectorLayer* layer, const QgsGeometry& clipGeometry) {
	// 获取图层的几何类型
	Qgis::GeometryType layerType = QgsWkbTypes::geometryType(layer->wkbType());

	// 创建一个适当的输出图层，类型与输入图层相同
	QString layerTypeStr;
	if (layerType == Qgis::GeometryType::Point) {
		layerTypeStr = "Point";
	}
	else if (layerType == Qgis::GeometryType::Line) {
		layerTypeStr = "LineString";
	}
	else if (layerType == Qgis::GeometryType::Polygon) {
		layerTypeStr = "Polygon";
	}
	else {
		// 不支持的几何类型
		QMessageBox::warning(nullptr, tr("错误"), tr("不支持的图层类型。"));
		return nullptr;
	}

	// 创建新的内存图层，类型与输入图层相同
	QgsVectorLayer* clippedLayer = new QgsVectorLayer(layerTypeStr + "?crs=" + layer->crs().authid(), "Clipped Layer", "memory");
	QgsFields fields = layer->fields();
	clippedLayer->dataProvider()->addAttributes(fields.toList());
	clippedLayer->updateFields();

	// 遍历输入图层的要素
	QgsFeatureIterator iter = layer->getFeatures();
	QgsFeature feature;
	QgsFeature clippedFeature;

	while (iter.nextFeature(feature)) {
		// 获取当前要素的几何
		QgsGeometry featureGeometry = feature.geometry();

		// 对每个要素，检查它是否与裁剪几何相交
		if (featureGeometry.intersects(clipGeometry)) {
			// 用图层要素的几何裁剪clipGeometry
			QgsGeometry clippedGeometry = clipGeometry.intersection(featureGeometry);

			// 如果裁剪后的几何有效（非空），则将其添加到新图层
			if (!clippedGeometry.isEmpty()) {
				clippedFeature.setGeometry(clippedGeometry);
				clippedFeature.setFields(fields);
				if (!clippedLayer->dataProvider()->addFeature(clippedFeature)) {
					QgsMessageLog::logMessage("Failed to add feature to clipped layer.", "VectorAnalysis", Qgis::MessageLevel::Critical);
				}
			}
		}
	}

	return clippedLayer;
}
//-----------------------------------------裁剪-------------------------------------------------------------------------------------