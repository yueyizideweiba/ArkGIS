#include "CustomGraphicsScene.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPathItem>

CustomGraphicsScene::CustomGraphicsScene(QObject* parent)
	: QGraphicsScene(parent) {}

void CustomGraphicsScene::setDrawMode(DrawMode mode) {
	currentMode = mode;
	currentItem = nullptr;  // 重置当前项，准备新绘制
	freeDrawPath = QPainterPath();  // 清空自由绘制路径
}

void CustomGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
	startPos = event->scenePos();  // 获取按下的起始位置

	switch (currentMode) {
	case DrawMode::Circle:
		currentItem = addEllipse(QRectF(startPos, QSizeF(0, 0)), QPen(Qt::black), QBrush(Qt::blue));
		break;
	case DrawMode::Square:
		currentItem = addRect(QRectF(startPos, QSizeF(0, 0)), QPen(Qt::black), QBrush(Qt::green));
		break;
	case DrawMode::Line:
		currentItem = addLine(QLineF(startPos, startPos), QPen(Qt::black));
		break;
	case DrawMode::FreeDraw:
		freeDrawPath.moveTo(startPos);
		currentItem = addPath(freeDrawPath, QPen(Qt::black));
		break;
	default:
		break;
	}

	QGraphicsScene::mousePressEvent(event);  // 继续处理其他事件
}

void CustomGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	if (!currentItem) return;  // 如果没有正在绘制的项，直接返回

	QPointF currentPos = event->scenePos();

	// 根据当前的绘图模式更新图形的大小和位置
	if (QGraphicsEllipseItem* ellipse = dynamic_cast<QGraphicsEllipseItem*>(currentItem)) {
		qreal radius = std::hypot(currentPos.x() - startPos.x(), currentPos.y() - startPos.y());
		ellipse->setRect(startPos.x() - radius, startPos.y() - radius, radius * 2, radius * 2);
	}
	else if (QGraphicsRectItem* rect = dynamic_cast<QGraphicsRectItem*>(currentItem)) {
		qreal size = std::min(std::abs(currentPos.x() - startPos.x()), std::abs(currentPos.y() - startPos.y()));
		rect->setRect(startPos.x(), startPos.y(), size, size);
	}
	else if (QGraphicsLineItem* line = dynamic_cast<QGraphicsLineItem*>(currentItem)) {
		line->setLine(QLineF(startPos, currentPos));
	}
	else if (QGraphicsPathItem* pathItem = dynamic_cast<QGraphicsPathItem*>(currentItem)) {
		freeDrawPath.lineTo(currentPos);
		pathItem->setPath(freeDrawPath);
	}

	QGraphicsScene::mouseMoveEvent(event);  // 继续处理其他事件
}

void CustomGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	currentItem = nullptr;  // 释放当前正在绘制的项
	QGraphicsScene::mouseReleaseEvent(event);  // 继续处理其他事件
}

QPixmap CustomGraphicsScene::getCurrentDrawing() {
	// 创建与场景大小相同的 QPixmap
	QPixmap pixmap(sceneRect().size().toSize());
	pixmap.fill(Qt::transparent);  // 背景透明

	// 使用 QPainter 渲染场景内容到 QPixmap
	QPainter painter(&pixmap);
	render(&painter);
	return pixmap;
}