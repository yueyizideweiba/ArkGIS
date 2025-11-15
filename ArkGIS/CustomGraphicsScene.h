/************************************************************
FileName: CustomGraphicsScene.h
Author: XYH
Version : 1.0
Date: 2024-10-24
Description: 自定义图形场景类，继承自QGraphicsScene，用于处理自定义绘图功能。
Function List:
1. CustomGraphicsScene - 构造函数，初始化自定义图形场景
2. setDrawMode - 设置当前绘图模式
3. getCurrentDrawing - 获取当前绘图的Pixmap表示
4. mousePressEvent - 重写鼠标按下事件处理
5. mouseMoveEvent - 重写鼠标移动事件处理
6. mouseReleaseEvent - 重写鼠标释放事件处理
Attributes:
1. DrawMode - 枚举类型，定义了不同的绘图模式
2. currentMode - 当前的绘图模式
3. startPos - 鼠标按下的起始位置
4. currentItem - 当前正在绘制的图形项
5. freeDrawPath - 用于自由绘制的路径
*************************************************************/

#ifndef CUSTOMGRAPHICSSCENE_H
#define CUSTOMGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QPointF>
#include <QPainterPath>
#include <QBuffer>
#include <QSvgGenerator>
#include <QPainter>

// 定义绘图模式
enum class DrawMode {
	None,       // 不绘制
	Circle,     // 圆形
	Square,     // 正方形
	Line,       // 直线
	FreeDraw    // 自由绘制
};

class CustomGraphicsScene : public QGraphicsScene {
	Q_OBJECT

public:
	explicit CustomGraphicsScene(QObject* parent = nullptr);
	void setDrawMode(DrawMode mode);  // 设置当前绘图模式
	QPixmap getCurrentDrawing();

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
	DrawMode currentMode = DrawMode::None;  // 当前的绘图模式
	QPointF startPos;  // 鼠标按下的起始位置
	QGraphicsItem* currentItem = nullptr;  // 当前正在绘制的图形项
	QPainterPath freeDrawPath;  // 用于自由绘制的路径
};

#endif
