#ifndef SINGLECLICKSELECTTOOL_H
#define SINGLECLICKSELECTTOOL_H

#include <QgsMapTool.h>
#include <QgsVectorLayer.h>
#include <QgsFeature.h>
#include <QgsMapCanvas.h>
#include <QObject>
#include <QgsMapMouseEvent.h>

class SingleClickSelectTool : public QgsMapTool
{
	Q_OBJECT

public:
	SingleClickSelectTool(QgsMapCanvas* canvas, QgsVectorLayer* layer);

	void canvasReleaseEvent(QgsMapMouseEvent* event) override;

signals:
	void featureSelected(const QgsFeature& feature);

private:
	QgsVectorLayer* mLayer;  // 要选择的矢量图层
};

#endif
