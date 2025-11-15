#include "EditLine.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QPushButton>
#include <qgsproject.h>
#include <qgsvectorlayer.h>
#include <qgsmaplayer.h>
#include <qgsgeometry.h>
#include <qgsfeature.h>
#include <qgsvectorlayer.h>
#include <qgsgeometry.h>
#include <qgspoint.h>
#include <qgslinestring.h>
#include <qgsproject.h>
#include <vector>
#include <cmath>
#include <QgsCurve.h>               // 用于处理曲线几何（如 QgsCurve）
#include <QgsPointXY.h>             // 用于点类型 QgsPointXY
#include <qfiledialog.h>
#include <QgsVectorFileWriter.h>


// ----------------- EditLine 实现 -----------------

EditLine::EditLine(QObject* parent)
	: QObject(parent)
{
}

void EditLine::executeSimplification(QWidget* parent)
{
	// 创建对话框实例
	EditLineDialog dialog(parent);

	// 显示对话框并确认用户选择
	if (dialog.exec() == QDialog::Accepted) {
		QString selectedLayer = dialog.getSelectedLayer();  // 获取选择的图层
		QString selectedMethod = dialog.getSelectedMethod(); // 获取选择的抽稀方法

		// 根据选择的抽稀方法调用对应的算法
		if (selectedMethod == "Douglas-Peucker") {
			douglasPeucker(selectedLayer);
		}
		else if (selectedMethod == "Topology Simplify") {
			topologySimplify(selectedLayer);
		}
		else {
			qWarning() << "Unknow Simplify:" << selectedMethod;
		}
	}
}

// ----------------- EditLineDialog 实现 -----------------
EditLine::EditLineDialog::EditLineDialog(QWidget* parent)
	: QDialog(parent)
{
	setupUI();
}

void EditLine::EditLineDialog::setupUI()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	// 图层选择
	QLabel* layerLabel = new QLabel(tr("choose layer:"), this); // 图层选择标签
	mLayerComboBox = new QComboBox(this);

	// 遍历项目中的图层，筛选出线图层
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values()) {
		if (l->type() == Qgis::LayerType::Vector &&
			static_cast<QgsVectorLayer*>(l)->geometryType() == Qgis::GeometryType::Line) {
			mLayerComboBox->addItem(l->name(), l->id()); // 添加图层名和图层ID
		}
	}

	// 方法选择
	QLabel* methodLabel = new QLabel(tr("choose method:"), this); // 方法选择标签
	mMethodComboBox = new QComboBox(this);
	mMethodComboBox->addItems({ tr("Douglas-Peucker"), tr("Topology Simplify") });

	// 按钮
	mOkButton = new QPushButton(tr("OK"), this);   // 确定按钮
	mCancelButton = new QPushButton(tr("Cancel"), this); // 取消按钮

	// 布局设置
	mainLayout->addWidget(layerLabel);
	mainLayout->addWidget(mLayerComboBox);
	mainLayout->addWidget(methodLabel);
	mainLayout->addWidget(mMethodComboBox);
	mainLayout->addWidget(mOkButton);
	mainLayout->addWidget(mCancelButton);

	// 信号槽连接
	connect(mOkButton, &QPushButton::clicked, this, &QDialog::accept);
	connect(mCancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString EditLine::EditLineDialog::getSelectedLayer() const
{
	return mLayerComboBox->currentText(); // 返回当前选择的图层
}

QString EditLine::EditLineDialog::getSelectedMethod() const
{
	return mMethodComboBox->currentText(); // 返回当前选择的抽稀方法
}

// 道格拉斯普克法抽稀
// 应用道格拉斯-普克算法对要素几何进行抽稀
void EditLine::douglasPeucker(const QString& layerName) {
	QgsVectorLayer* layer = nullptr;

	// 查找指定图层
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values()) {
		if (l->name() == layerName && l->type() == Qgis::LayerType::Vector) {
			layer = static_cast<QgsVectorLayer*>(l);
			break;
		}
	}

	if (!layer || layer->geometryType() != Qgis::GeometryType::Line) {
		qWarning() << "Cannot find the specified layer or the layer type is not a line:" << layerName;
		return;
	}

	double tolerance = 100.0; // 抽稀的容差

	if (!layer->startEditing()) {
		qWarning() << "Cannot start editing layer.";
		return;
	}

	int originalPointCount = 0; // 原始点总数
	int simplifiedPointCount = 0; // 抽稀后的点总数

	QgsFeatureIterator featureIterator = layer->getFeatures();
	QgsFeature feature;

	// 遍历所有要素
	while (featureIterator.nextFeature(feature)) {
		QgsGeometry geom = feature.geometry();

		//if (geom.isEmpty()) {
		//    qDebug() << "该要素几何为空";
		//    continue;
		//}

		// 获取多段线几何
		QgsMultiPolylineXY multiPolyline = geom.asMultiPolyline();
		if (multiPolyline.isEmpty()) {
			qDebug() << "该要素没有有效的多段线";
			continue;
		}

		// 存储处理后的多段线
		QgsMultiPolylineXY simplifiedMultiPolyline;

		for (const QgsPolylineXY& polyline : multiPolyline) {
			qDebug() << "正在处理多段线";
			std::vector<QgsPointXY> pointsXY;
			for (const QgsPointXY& pt : polyline) {
				pointsXY.push_back(pt);
				originalPointCount++;
			}

			// 将 QgsPointXY 转换为 QgsPoint
			std::vector<QgsPoint> points;
			for (const QgsPointXY& pt : pointsXY) {
				points.emplace_back(pt.x(), pt.y());
			}

			// 运行 Douglas-Peucker 算法
			std::vector<QgsPoint> simplifiedPoints;
			douglasPeuckerRecursion(points, tolerance, simplifiedPoints);

			simplifiedPointCount += simplifiedPoints.size();

			// 将抽稀结果转换回 QgsPolylineXY
			QgsPolylineXY simplifiedPolyline;
			for (const QgsPoint& pt : simplifiedPoints) {
				simplifiedPolyline.append(QgsPointXY(pt.x(), pt.y()));
			}

			simplifiedMultiPolyline.append(simplifiedPolyline);
		}

		// 用简化后的几何替换原始几何
		QgsGeometry simplifiedGeometry = QgsGeometry::fromMultiPolylineXY(simplifiedMultiPolyline);

		if (!simplifiedGeometry.isNull() && simplifiedGeometry.isGeosValid()) {
			feature.setGeometry(simplifiedGeometry);
			if (!layer->updateFeature(feature)) {
				qWarning() << "Failed to update feature ID:" << feature.id();
			}
		}
		else {
			qWarning() << "Simplified geometry is invalid, skipping element ID:" << feature.id();
		}
	}

	// 提交修改
	if (!layer->commitChanges()) {
		qWarning() << "Failed to commit changes.";
		layer->rollBack();
	}
	else {
		qDebug() << "Douglas-Peucker is done. The layer has been updated.";
		layer->triggerRepaint();  // 刷新图层
	}

	// 弹出窗口显示点数变化
	QMessageBox::information(nullptr, "Simplification Results",
		QString("Original points: %1\nSimplified points: %2")
		.arg(originalPointCount)
		.arg(simplifiedPointCount));
}


// 计算点到线段的垂直距离
double EditLine::perpendicularDistance(const QgsPoint& point, const QgsPoint& lineStart, const QgsPoint& lineEnd)
{
	double dx = lineEnd.x() - lineStart.x();
	double dy = lineEnd.y() - lineStart.y();

	if (dx == 0 && dy == 0) {
		return std::hypot(point.x() - lineStart.x(), point.y() - lineStart.y());
	}

	double t = ((point.x() - lineStart.x()) * dx + (point.y() - lineStart.y()) * dy) / (dx * dx + dy * dy);
	if (t < 0) {
		dx = point.x() - lineStart.x();
		dy = point.y() - lineStart.y();
	}
	else if (t > 0) {
		dx = point.x() - lineEnd.x();
		dy = point.y() - lineEnd.y();
	}
	else {
		dx = point.x() - (lineStart.x() + t * dx);
		dy = point.y() - (lineStart.y() + t * dy);
	}

	return std::hypot(dx, dy);
}

void EditLine::douglasPeuckerRecursion(const std::vector<QgsPoint>& points, double tolerance, std::vector<QgsPoint>& outPoints)
{
	if (points.size() < 2) {
		return;
	}

	// 起点和终点
	QgsPoint startPoint = points.front();
	QgsPoint endPoint = points.back();

	// 找到最大距离的点
	double maxDistance = 0.0;
	int index = 0;
	for (int i = 1; i < points.size() - 1; ++i) {
		double distance = perpendicularDistance(points[i], startPoint, endPoint);
		if (distance > maxDistance) {
			index = i;
			maxDistance = distance;
		}
	}

	// 如果最大距离大于容差，递归处理
	if (maxDistance > tolerance) {
		std::vector<QgsPoint> leftSublist(points.begin(), points.begin() + index + 1);
		std::vector<QgsPoint> rightSublist(points.begin() + index, points.end());

		douglasPeuckerRecursion(leftSublist, tolerance, outPoints);
		outPoints.pop_back(); // 避免重复起点
		douglasPeuckerRecursion(rightSublist, tolerance, outPoints);
	}
	else {
		// 如果距离小于容差，只保留起点和终点
		outPoints.push_back(startPoint);
		outPoints.push_back(endPoint);
	}
}

// 拓扑抽稀法
void EditLine::topologySimplify(const QString& layerName) {
	QgsVectorLayer* layer = nullptr;

	// 查找指定图层
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values()) {
		if (l->name() == layerName && l->type() == Qgis::LayerType::Vector) {
			layer = static_cast<QgsVectorLayer*>(l);
			break;
		}
	}

	if (!layer || layer->geometryType() != Qgis::GeometryType::Line) {
		qWarning() << "Cannot find the specified layer or the layer type is not a line:" << layerName;
		return;
	}

	double tolerance = 100.0; // 抽稀的容差

	if (!layer->startEditing()) {
		qWarning() << "Cannot start editing layer.";
		return;
	}

	int originalPointCount = 0; // 原始点总数
	int simplifiedPointCount = 0; // 抽稀后的点总数

	QgsFeatureIterator featureIterator = layer->getFeatures();
	QgsFeature feature;

	// 遍历所有要素
	while (featureIterator.nextFeature(feature)) {
		QgsGeometry geom = feature.geometry();

		// 获取多段线几何
		QgsMultiPolylineXY multiPolyline = geom.asMultiPolyline();
		if (multiPolyline.isEmpty()) {
			qDebug() << "该要素没有有效的多段线";
			continue;
		}

		// 存储处理后的多段线
		QgsMultiPolylineXY simplifiedMultiPolyline;

		for (const QgsPolylineXY& polyline : multiPolyline) {
			qDebug() << "正在处理多段线";
			std::vector<QgsPointXY> pointsXY;
			for (const QgsPointXY& pt : polyline) {
				pointsXY.push_back(pt);
				originalPointCount++;
			}

			// 将 QgsPointXY 转换为 QgsPoint
			std::vector<QgsPoint> points;
			for (const QgsPointXY& pt : pointsXY) {
				points.emplace_back(pt.x(), pt.y());
			}

			// 运行拓扑约束的抽稀算法
			std::vector<QgsPoint> simplifiedPoints;
			topologicalDouglasPeuckerRecursion(points, tolerance, simplifiedPoints, layer);

			simplifiedPointCount += simplifiedPoints.size();

			// 将抽稀结果转换回 QgsPolylineXY
			QgsPolylineXY simplifiedPolyline;
			for (const QgsPoint& pt : simplifiedPoints) {
				simplifiedPolyline.append(QgsPointXY(pt.x(), pt.y()));
			}

			simplifiedMultiPolyline.append(simplifiedPolyline);
		}

		// 用简化后的几何替换原始几何
		QgsGeometry simplifiedGeometry = QgsGeometry::fromMultiPolylineXY(simplifiedMultiPolyline);

		if (!simplifiedGeometry.isNull() && simplifiedGeometry.isGeosValid()) {
			feature.setGeometry(simplifiedGeometry);
			if (!layer->updateFeature(feature)) {
				qWarning() << "Failed to update feature ID:" << feature.id();
			}
		}
		else {
			qWarning() << "Simplified geometry is invalid, skipping element ID:" << feature.id();
		}
	}

	// 提交修改
	if (!layer->commitChanges()) {
		qWarning() << "Failed to commit changes.";
		layer->rollBack();
	}
	else {
		qDebug() << "Topological Douglas-Peucker is done. The layer has been updated.";
		layer->triggerRepaint();  // 刷新图层
	}

	// 弹出窗口显示点数变化
	QMessageBox::information(nullptr, "Simplification Results",
		QString("Original points: %1\nSimplified points: %2")
		.arg(originalPointCount)
		.arg(simplifiedPointCount));
}

// 拓扑约束的Douglas-Peucker递归算法
void EditLine::topologicalDouglasPeuckerRecursion(
	const std::vector<QgsPoint>& points,
	double tolerance,
	std::vector<QgsPoint>& simplifiedPoints,
	QgsVectorLayer* layer) {

	if (points.size() < 2) {
		return;
	}

	double maxDistance = 0.0;
	int farthestIndex = 0;

	// 找到最大偏离点
	for (int i = 1; i < points.size() - 1; ++i) {
		double distance = perpendicularDistance(points[i], points.front(), points.back());
		if (distance > maxDistance) {
			maxDistance = distance;
			farthestIndex = i;
		}
	}

	// 如果偏离度大于容差，则继续分割
	if (maxDistance > tolerance) {
		// 左边部分
		std::vector<QgsPoint> leftPoints(points.begin(), points.begin() + farthestIndex + 1);
		topologicalDouglasPeuckerRecursion(leftPoints, tolerance, simplifiedPoints, layer);

		// 右边部分
		std::vector<QgsPoint> rightPoints(points.begin() + farthestIndex, points.end());
		topologicalDouglasPeuckerRecursion(rightPoints, tolerance, simplifiedPoints, layer);
	}
	else {
		// 将起点和终点加入到简化后的点集中
		if (simplifiedPoints.empty() || simplifiedPoints.back() != points.front()) {
			simplifiedPoints.push_back(points.front());
		}
		if (simplifiedPoints.back() != points.back()) {
			simplifiedPoints.push_back(points.back());
		}
	}
}