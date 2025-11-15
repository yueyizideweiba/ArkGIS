#ifndef RECTANGLESELECTTOOL_H
#define RECTANGLESELECTTOOL_H

#include <QObject>
#include <QgsMapTool.h>
#include <QgsRubberBand.h>
#include <QgsFeature.h>
#include <QgsVectorLayer.h>

class RectangleSelectTool : public QgsMapTool
{
	Q_OBJECT

public:
	explicit RectangleSelectTool(QgsMapCanvas* canvas, QgsVectorLayer* layer, QObject* parent = nullptr);
	~RectangleSelectTool();

	// 重写 QgsMapTool 的事件处理函数
	void canvasPressEvent(QgsMapMouseEvent* e) override;
	void canvasMoveEvent(QgsMapMouseEvent* e) override;
	void canvasReleaseEvent(QgsMapMouseEvent* e) override;
	void activate() override;
	void deactivate() override;

signals:
	void featuresSelected(const QList<QgsFeature>& features);

private:
	QgsMapCanvas* mCanvas;               // 地图画布
	QgsVectorLayer* mLayer;              // 目标图层
	QgsRubberBand* mRubberBand;          // 橡皮筋，用于绘制矩形
	QPoint mStartPoint;                   // 矩形起始点
	bool mIsDrawing;                      // 是否正在绘制
};

#endif 
