#ifndef MYQGSMAPTOOL_H
#define MYQGSMAPTOOL_H
//-----------------------------------------裁剪-------------------------------------------------------------------------------------
/*
绘制矩形、圆形或多边形裁剪框的交互类实现
*/

#include "thirdlib/qgis-ltr-dev/include/qgsmaptool.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <qgsproject.h>
#include <qgsmaplayer.h>
#include <QVector>
#include <QtMath>
#include <qgsgeometry.h>
#include <qgsgeometryrubberband.h>
#include <qgsmapcanvas.h>
#include <qgsmapmouseevent.h>
#include <qgsrectangle.h>
#include <qgsfeature.h>
#include <qgsfeaturerequest.h>
#include <qgsfeatureiterator.h>
#include <qgsfields.h>
#include <qgswkbtypes.h>
#include <QgsRubberBand.h>

class VectorAnalysis;
//绘制矩形框
class QgsMapToolRectangle : public QgsMapTool {
	Q_OBJECT

public:
	QgsMapToolRectangle(QgsMapCanvas* canvas, VectorAnalysis* parent)
		: QgsMapTool(canvas), parent(parent), rubberBand(nullptr), m_isCanceled(false), m_isStartSet(false) {
		this->canvas = canvas;
		this->parent = parent;
		rubberBand = new QgsRubberBand(canvas, Qgis::GeometryType::Polygon);
		rubberBand->setColor(Qt::blue);
		rubberBand->setWidth(2);
		rubberBand->setFillColor(QColor(0, 0, 255, 50)); // 半透明填充
	}

	~QgsMapToolRectangle() {
		delete rubberBand;  // 确保内存被释放
	}

	void canvasPressEvent(QgsMapMouseEvent* e) override {
		if (!m_isStartSet) {
			// 第一次点击，设置起始点
			startPoint = e->snapPoint();
			qDebug() << "Start point: " << startPoint.x() << "," << startPoint.y();  // 打印起始点坐标
			m_isStartSet = true;  // 设置起始点已被设置

			// 清空rubberBand几何，准备绘制
			rubberBand->reset(Qgis::GeometryType::Polygon);
			rubberBand->addPoint(startPoint);  // 添加起始点
			rubberBand->show();
		}
	}

	void canvasMoveEvent(QgsMapMouseEvent* e) override {
		if (!m_isStartSet) return;  // 如果起始点未设置，则不做任何操作

		// 获取当前鼠标位置作为矩形的结束点
		endPoint = e->snapPoint();
		qDebug() << "End point: " << endPoint.x() << "," << endPoint.y();  // 打印结束点坐标

		// 创建矩形几何并更新橡皮筋显示
		QgsRectangle rect(startPoint, endPoint);
		rubberBand->reset(Qgis::GeometryType::Polygon);
		rubberBand->addPoint(QgsPointXY(rect.xMinimum(), rect.yMinimum()));
		rubberBand->addPoint(QgsPointXY(rect.xMaximum(), rect.yMinimum()));
		rubberBand->addPoint(QgsPointXY(rect.xMaximum(), rect.yMaximum()));
		rubberBand->addPoint(QgsPointXY(rect.xMinimum(), rect.yMaximum()));
		rubberBand->addPoint(QgsPointXY(rect.xMinimum(), rect.yMinimum()));
		rubberBand->show();
	}

	void canvasReleaseEvent(QgsMapMouseEvent* e) override {
		// 用户完成矩形选择时
		if (m_isStartSet) {
			QgsRectangle rect(startPoint, endPoint);
			clipGeometry = QgsGeometry::fromRect(rect);
			m_isCanceled = true;
		}
	}

	bool isCanceled() const {
		return m_isCanceled;
	}

	QgsGeometry geometry() const {
		return clipGeometry;
	}

private:
	QgsMapCanvas* canvas;
	VectorAnalysis* parent;
	QgsRubberBand* rubberBand;
	QgsPointXY startPoint;  // 起始点
	QgsPointXY endPoint;    // 结束点
	QgsGeometry clipGeometry;  // 裁剪几何体
	bool m_isCanceled;  // 是否完成绘制
	bool m_isStartSet;  // 是否设置了起始点
};

class QgsMapToolCircle : public QgsMapTool {
	Q_OBJECT

public:
	QgsMapToolCircle(QgsMapCanvas* canvas, VectorAnalysis* parent)
		: QgsMapTool(canvas), parent(parent), rubberBand(nullptr), m_isCanceled(false), m_isFirstClick(true) {
		this->canvas = canvas;
		this->parent = parent;
		rubberBand = new QgsRubberBand(canvas, Qgis::GeometryType::Polygon);
		rubberBand->setColor(Qt::blue);
		rubberBand->setWidth(2);
		rubberBand->setFillColor(QColor(0, 0, 255, 50));
	}

	~QgsMapToolCircle() {
		delete rubberBand;
	}

	QgsPointXY center() const {
		return startPoint;  // 圆心是起始点
	}

	double radius() const {
		return startPoint.distance(endPoint);  // 半径是起始点到结束点的距离
	}

	void canvasPressEvent(QgsMapMouseEvent* e) override {
		if (m_isFirstClick) {
			// 第一次点击，记录圆心位置
			startPoint = e->snapPoint();
			rubberBand->addPoint(startPoint);  // 添加圆心到rubberBand
			m_isFirstClick = false;  // 标记为第二次点击
		}
	}

	void canvasMoveEvent(QgsMapMouseEvent* e) override {
		if (m_isFirstClick) return;  // 第一次点击时不需要更新

		// 更新 rubberBand 的几何体，显示当前鼠标位置到圆心的圆形
		QgsPointXY currentPoint = e->snapPoint();
		double radius = startPoint.distance(currentPoint);  // 计算圆形半径

		int numPoints = 100;  // 圆周上的点数
		QList<QgsPointXY> points;
		for (int i = 0; i < numPoints; ++i) {
			double angle = (2 * M_PI * i) / numPoints;
			double x = startPoint.x() + radius * cos(angle);
			double y = startPoint.y() + radius * sin(angle);
			points.append(QgsPointXY(x, y));  // 将每个点添加到列表中
		}

		// 使用 QList<QgsPointXY> 创建 QgsPolylineXY
		QVector<QgsPointXY> vectorPoints = QVector<QgsPointXY>::fromList(points);
		QgsPolylineXY polyline(vectorPoints);
		QgsPolygonXY polygon({ polyline });

		// 使用 QgsPolygonXY 对象创建几何体
		QgsGeometry geom = QgsGeometry::fromPolygonXY(polygon);

		// 更新 rubberBand 显示圆形
		rubberBand->setToGeometry(geom, nullptr);  // 设置几何体
	}

	void canvasReleaseEvent(QgsMapMouseEvent* e) override {
		if (m_isFirstClick) return;  // 第一次点击时不需要触发

		// 完成圆形绘制，记录圆形几何
		QgsPointXY endPoint = e->snapPoint();  // 鼠标释放时确定圆周上的点
		double radius = startPoint.distance(endPoint);  // 计算半径

		// 计算圆周上的若干点来表示圆
		int numPoints = 100;  // 圆周上的点数，可以根据需要调整
		QList<QgsPointXY> points;
		for (int i = 0; i < numPoints; ++i) {
			double angle = (2 * M_PI * i) / numPoints;
			double x = startPoint.x() + radius * cos(angle);
			double y = startPoint.y() + radius * sin(angle);
			points.append(QgsPointXY(x, y));  // 将每个点添加到列表中
		}

		// 使用 QList<QgsPointXY> 创建 QgsPolylineXY
		QVector<QgsPointXY> vectorPoints = QVector<QgsPointXY>::fromList(points);
		QgsPolylineXY polyline(vectorPoints);
		QgsPolygonXY polygon({ polyline });

		// 使用 QgsPolygonXY 对象创建几何体
		clipGeometry = QgsGeometry::fromPolygonXY(polygon);  // 设置圆的几何体

		// 清理资源
		m_isCanceled = true;  // 圆形绘制完成，标记为完成

		// 结束绘制，退出工具
		canvas->unsetMapTool(this);
	}

	bool isCanceled() const {
		return m_isCanceled;
	}

	QgsGeometry geometry() const {
		return clipGeometry;
	}

private:
	QgsMapCanvas* canvas;
	VectorAnalysis* parent;
	QgsRubberBand* rubberBand;
	QgsPointXY startPoint;  // 圆心
	QgsPointXY endPoint;    // 圆周上的点
	QgsGeometry clipGeometry;
	bool m_isCanceled;
	bool m_isFirstClick;    // 标记是否为第一次点击
};

//绘制多边形框
class QgsMapToolPolygon : public QgsMapTool {
	Q_OBJECT

public:
	QgsMapToolPolygon(QgsMapCanvas* canvas, VectorAnalysis* parent)
		: QgsMapTool(canvas), parent(parent), rubberBand(nullptr), m_isCanceled(false) {
		this->canvas = canvas;
		this->parent = parent;
		rubberBand = new QgsRubberBand(canvas, Qgis::GeometryType::Polygon);
		rubberBand->setColor(Qt::blue);
		rubberBand->setWidth(2);
		rubberBand->setFillColor(QColor(0, 0, 255, 50)); // 半透明填充
		points = QList<QgsPointXY>();
	}

	~QgsMapToolPolygon() {
		delete rubberBand;
	}

	void canvasPressEvent(QgsMapMouseEvent* e) override {
		if (e->button() == Qt::LeftButton) {  // 只有左键按下时才添加点
			// 记录当前点击的点
			QgsPointXY clickedPoint = e->mapPoint();
			points.append(clickedPoint);

			// 更新橡皮筋，加入新点
			if (points.size() == 1) {
				rubberBand->reset(Qgis::GeometryType::Polygon);
				rubberBand->addPoint(clickedPoint);
			}
			else {
				rubberBand->addPoint(clickedPoint, true);
			}
			rubberBand->show();  // 显示橡皮筋
		}
	}

	void canvasMoveEvent(QgsMapMouseEvent* e) override {
		if (points.isEmpty()) return;

		// 动态更新橡皮筋的最后一个点，实时显示当前鼠标位置作为多边形的最后一个边
		QgsPointXY currentPoint = e->mapPoint();

		// 临时移除最后一个点并更新
		rubberBand->removeLastPoint();
		rubberBand->addPoint(currentPoint, true);
	}

	void canvasReleaseEvent(QgsMapMouseEvent* e) override {
		if (e->button() == Qt::RightButton && points.size() >= 3) {
			// 右键点击完成多边形绘制，关闭多边形
			points.append(points.first());  // 用第一个点闭合多边形
			QgsGeometry geom = QgsGeometry::fromPolygonXY({ QgsPolylineXY::fromList(points) });
			clipGeometry = geom;  // 设置最终几何体

			m_isCanceled = true;  // 标记为已完成绘制
		}
	}

	bool isCanceled() const {
		return m_isCanceled;
	}

	QgsGeometry geometry() const {
		return clipGeometry;
	}

private:
	QgsMapCanvas* canvas;
	VectorAnalysis* parent;
	QgsRubberBand* rubberBand;
	QList<QgsPointXY> points;
	QgsGeometry clipGeometry;
	bool m_isCanceled;
};


//-----------------------------------------裁剪-------------------------------------------------------------------------------------
#endif // MYQGSMAPTOOL_H
