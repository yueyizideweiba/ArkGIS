/************************************************************
FileName: VectorAnalysis.h
Author: XYH
Version : 1.0
Date: 2024-10-20
Description: 矢量相关分析的实现类
Function List:
1. performKMeansClustering - 创建K值聚类参数窗口并调用实现函数
2. performSpatialJoin - 创建按位置连接属性窗口并调用实现函数
3. applyKMeansClustering - 实现K值聚类
4. applySpatialJoin - 实现按位置连接属性
***********************************************************/

#pragma once

#include <QObject>
#include <QgsMapCanvas.h>
#include <QDialog>

class VectorAnalysis : public QObject
{
	Q_OBJECT

public:
	VectorAnalysis(QgsMapCanvas* mapCanvas, QObject* parent = nullptr);

public slots:
	void performKMeansClustering(); // 创建K值聚类参数窗口
	void performSpatialJoin(); // 创建按位置连接属性窗口
	void performExcelToShp();//创建表转矢量接口
	//-----------------------------------------裁剪-----------------------------------------------------------
	void performVectorClipping();
	//创建矢量裁剪分析窗口，分别用矩形、圆和任意多边形对矢量图层进行裁剪，并将裁剪结果输出到矢量文件。
	void performCustomVectorClipping();
	//-----------------------------------------裁剪-----------------------------------------------------------
private:
	/*************************************************
	* Function: applyKMeansClustering
	* Description: 实现K值聚类
	* Parameters:
	* layer : 矢量图层指针 QgsVectorLayer*
	* k : 聚类数目 int
	*************************************************/
	void applyKMeansClustering(QgsVectorLayer* layer, int k);

	/*************************************************
	 * Function: applySpatialJoin
	 * Description: 实现按位置连接属性
	 * Parameters:
	 * poiLayer : 点图层指针 QgsVectorLayer*
	 * polygonLayer : 面图层指针 QgsVectorLayer*
	 *************************************************/
	void applySpatialJoin(QgsVectorLayer* poiLayer, QgsVectorLayer* polygonLayer);

	QgsMapCanvas* mpMapCanvas; // 绘图画布
	//-----------------------------------------裁剪-------------------------------------------------------------------------------------
//矢量图层裁剪
	void vectorClipping(QgsVectorLayer* selectedLayer, QgsVectorLayer* polygonLayer, Qgis::GeometryType geometryType);
	QgsVectorLayer* clipVectorLayer(QgsVectorLayer* selectedLayer, QgsVectorLayer* polygonLayer, Qgis::GeometryType geometryType);
	//自定义裁剪窗口矢量裁剪，矩形、圆、任意多边形
	QgsGeometry createRectangleGeometry();
	QgsGeometry createCircleGeometry();
	QgsGeometry createPolygonGeometry();
	QgsVectorLayer* customClipVectorLayer(QgsVectorLayer* layer, const QgsGeometry& clipGeometry);
	//-----------------------------------------裁剪-------------------------------------------------------------------------------------

};
