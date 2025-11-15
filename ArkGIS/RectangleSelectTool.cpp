#include "RectangleSelectTool.h"
#include <QgsMapMouseEvent.h>
#include <QgsRectangle.h>
#include <QgsFeatureIterator.h>
#include <QgsGeometry.h>
#include <QgsSpatialIndex.h>
#include <QgsProject.h>
#include <QgsMapCanvas.h>

RectangleSelectTool::RectangleSelectTool(QgsMapCanvas* canvas, QgsVectorLayer* layer, QObject* parent)
	: QgsMapTool(canvas), mCanvas(canvas), mLayer(layer), mIsDrawing(false)
{
	// 初始化橡皮筋，用于绘制矩形
	mRubberBand = new QgsRubberBand(mCanvas, Qgis::GeometryType::Polygon);
	mRubberBand->setColor(Qt::blue);
	mRubberBand->setWidth(2);
	mRubberBand->setFillColor(QColor(0, 0, 255, 50)); // 半透明填充
	mRubberBand->setToGeometry(QgsGeometry(), mLayer);
}

RectangleSelectTool::~RectangleSelectTool()
{
	if (mRubberBand) {
		delete mRubberBand;
	}
}

void RectangleSelectTool::activate()
{
	QgsMapTool::activate();
	mCanvas->setCursor(Qt::CrossCursor); // 设置光标为十字形
}

void RectangleSelectTool::deactivate()
{
	QgsMapTool::deactivate();
	mRubberBand->reset(Qgis::GeometryType::Polygon);
	mCanvas->unsetCursor();
}

void RectangleSelectTool::canvasPressEvent(QgsMapMouseEvent* e)
{
	if (e->button() == Qt::LeftButton) {
		mStartPoint = e->pos(); // 记录起始点
		mIsDrawing = true;
		mRubberBand->reset(Qgis::GeometryType::Polygon);
		mRubberBand->addPoint(mCanvas->getCoordinateTransform()->toMapCoordinates(mStartPoint));
		mRubberBand->show();
	}
}

void RectangleSelectTool::canvasMoveEvent(QgsMapMouseEvent* e)
{
	if (mIsDrawing) {
		QPoint currentPoint = e->pos();
		QgsPointXY p1 = mCanvas->getCoordinateTransform()->toMapCoordinates(mStartPoint);
		QgsPointXY p2 = mCanvas->getCoordinateTransform()->toMapCoordinates(currentPoint);

		QgsRectangle rect(p1, p2);

		// 更新橡皮筋的几何形状
		mRubberBand->reset(Qgis::GeometryType::Polygon);
		mRubberBand->addPoint(QgsPointXY(rect.xMinimum(), rect.yMinimum()));
		mRubberBand->addPoint(QgsPointXY(rect.xMaximum(), rect.yMinimum()));
		mRubberBand->addPoint(QgsPointXY(rect.xMaximum(), rect.yMaximum()));
		mRubberBand->addPoint(QgsPointXY(rect.xMinimum(), rect.yMaximum()));
		mRubberBand->addPoint(QgsPointXY(rect.xMinimum(), rect.yMinimum()));
		mRubberBand->show();
	}
}

void RectangleSelectTool::canvasReleaseEvent(QgsMapMouseEvent* e)
{
	if (e->button() == Qt::LeftButton && mIsDrawing) {
		mIsDrawing = false;
		QPoint endPoint = e->pos();

		QgsPointXY p1 = mCanvas->getCoordinateTransform()->toMapCoordinates(mStartPoint);
		QgsPointXY p2 = mCanvas->getCoordinateTransform()->toMapCoordinates(endPoint);

		QgsRectangle rect(p1, p2);

		// 查询选中的要素
		QList<QgsFeature> selectedFeatures;

		// 使用空间索引提高查询效率
		QgsSpatialIndex spatialIndex(mLayer->getFeatures());

		QList<QgsFeatureId> featureIds = spatialIndex.intersects(rect);

		QgsFeatureIterator fit = mLayer->getFeatures(QgsFeatureRequest().setFilterRect(rect).setFlags(QgsFeatureRequest::ExactIntersect));
		QgsFeature feature;
		while (fit.nextFeature(feature)) {
			if (feature.hasGeometry() && feature.geometry().intersects(QgsGeometry::fromRect(rect))) {
				selectedFeatures.append(feature);
			}
		}

		// 输出选中的要素数量
		qDebug() << "Number of selected features:" << selectedFeatures.size();

		emit featuresSelected(selectedFeatures);

		// 清除橡皮筋
		mRubberBand->reset(Qgis::GeometryType::Polygon);
		mRubberBand->hide();
	}
}