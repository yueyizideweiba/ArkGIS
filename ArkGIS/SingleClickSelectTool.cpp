#include "SingleClickSelectTool.h"
#include <QgsPointXY.h>
#include <QgsGeometry.h>
#include <QgsFeatureRequest.h>
#include <QgsFeatureIterator.h>
#include <QgsMapCanvas.h>
#include <QMessageBox>
#include <QgsMapMouseEvent.h>

SingleClickSelectTool::SingleClickSelectTool(QgsMapCanvas* canvas, QgsVectorLayer* layer)
	: QgsMapTool(canvas), mLayer(layer)
{
	// 设置光标样式，可以根据需要更改
	setCursor(Qt::CrossCursor);
}

void SingleClickSelectTool::canvasReleaseEvent(QgsMapMouseEvent* event)
{
	if (!mLayer || !event)
		return;

	// 获取点击的地图坐标
	QgsPointXY clickPoint = event->mapPoint();

	// 设置选择半径，表示距离点击点的距离
	double searchRadius = canvas()->mapUnitsPerPixel() * 5;

	// 创建一个矩形范围，用于选择在点击范围内的要素
	QgsRectangle searchRect(clickPoint.x() - searchRadius, clickPoint.y() - searchRadius,
		clickPoint.x() + searchRadius, clickPoint.y() + searchRadius);

	// 创建请求对象，筛选该矩形范围内的要素
	QgsFeatureRequest request;
	request.setFilterRect(searchRect);
	QgsFeature feature;

	// 遍历图层中的要素
	QgsFeatureIterator it = mLayer->getFeatures(request);
	if (it.nextFeature(feature))
	{
		emit featureSelected(feature);  // 选择到的第一个要素
	}
	else
	{
		QMessageBox::information(nullptr, tr("No Feature Found"), tr("No feature found near the clicked point."));
	}
}
