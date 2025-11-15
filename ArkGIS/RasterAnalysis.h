/************************************************************
FileName: RasterAnalysis.h
Author: XYH
Version : 1.0
Date: 2024-10-22
Description: 栅格相关分析的实现类
Function List:
1. performRasterLayerStatistics - 实现栅格图层统计并保存结果
***********************************************************/

#pragma once

#include <QObject>
#include <QgsMapCanvas.h>
#include <QDialog>

class RasterAnalysis : public QObject
{
	Q_OBJECT

public:
	RasterAnalysis(QgsMapCanvas* mapCanvas, QObject* parent = nullptr);

public slots:
	void performRasterLayerStatistics(); // 实现栅格图层统计并保存结果
	void rasterCalculator(); // 实现栅格计算器

private:
	QgsMapCanvas* mpMapCanvas; // 绘图画布
};
