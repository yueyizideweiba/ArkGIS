/************************************************************
FileName: LayerManager.h
Author: XYH
Version : 1.0
Date: 2024-10-19
Description: 图层管理及图层相关操作类
Function List:
1.addVectorLayer - 添加矢量图层
2.addRasterLayer - 添加栅格图层
3.addDelimitedTextLayer - 添加分隔符图层
4.onFileTreeContextMenu - 文件树右键菜单
5.selectDirectory - 选择文件目录
6.populateFileTree - 填充文件树
7.showLayerTreeContextMenu - 图层树右键菜单
8.updateMapCanvasLayers - 更新画布上的图层
9.performCategorizedSymbology - 按类别符号化图层
10.addLayerFromFile - 将读取的数据添加到图层
11.populateFileTreeHelper - 辅助填充文件树
12.removeSelectedLayer - 移除图层
13.saveSelectedLayer - 保存图层
14.viewLayerAttributes - 查看属性表
15.applyCategorizedSymbology - 实现分类符号化
16.openSymbolizationDialog - 打开符号化对话框
17.applySymbolToLayer - 应用符号
***********************************************************/

#pragma once

#include <QObject>
#include <qgsmapcanvas.h>
#include <qgslayertreeview.h>
#include <QTreeWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include "Symbol.h"
#include "MainWindow.h"
#include "SymbolLibrary.h"
#include <qgsmaptool.h> 
#include <QMessageBox>
#include <QCheckBox>
#include <QgsMapToolIdentifyFeature.h>  // QGIS点击识别工具
#include "RectangleSelectTool.h"  // 自定义矩形框选工具

class MainWindow;

class LayerManager : public QObject
{
	Q_OBJECT

public:
	explicit LayerManager(QgsMapCanvas* canvas, QTreeWidget* fileTreeWidget, QgsLayerTreeView* layerTreeView, MainWindow* mainWindow, QObject* parent = nullptr);

	void openSymbolizationDialog();  // 打开符号化对话框
	void applySymbolToLayer(QgsVectorLayer* layer, const Symbol& symbol);  // 应用符号

signals:
	void layersChanged();

public slots:
	// 添加图层相关槽函数
	void addVectorLayer();
	void addRasterLayer();
	void addDelimitedTextLayer();

	/*************************************************
	 * Function: onFileTreeContextMenu
	 * Description: 显示文件树右键菜单
	 * Parameters:
	 * pos : 右键点击位置 QPoint
	 *************************************************/
	void onFileTreeContextMenu(const QPoint& pos);

	void selectDirectory(); // 选择目录

	/*************************************************
	* Function: populateFileTree
	* Description: 根据指定路径填充文件树
	* Parameters:
	* path : 目录路径 QString
	*************************************************/
	void populateFileTree(const QString& path);

	/*************************************************
	 * Function: showLayerTreeContextMenu
	 * Description: 显示图层树右键菜单
	 * Parameters:
	 * pos : 右键点击位置 QPoint
	 *************************************************/
	void showLayerTreeContextMenu(const QPoint& pos);

	void updateMapCanvasLayers(); // 更新地图画布

	void performCategorizedSymbology(); // 按类别符号化图层

	void setupPointSymbolizationUI(QVBoxLayout* layout, QgsVectorLayer* vectorLayer);
	void setupLineSymbolizationUI(QVBoxLayout* layout, QgsVectorLayer* vectorLayer);
	void setupPolygonSymbolizationUI(QVBoxLayout* layout, QgsVectorLayer* vectorLayer);
	void applySymbolToLayerSimple(QgsVectorLayer* layer);

	// 显示注记
	void showLayerLabels();

private:
	void addLayerFromFile(); // 添加数据到图层

	/*************************************************
	 * Function: populateFileTreeHelper
	 * Description: 辅助函数，递归填充文件树
	 * Parameters:
	 * path : 目录路径 QString
	 * parentItem : 父项指针 QTreeWidgetItem*
	 *************************************************/
	void populateFileTreeHelper(const QString& path, QTreeWidgetItem* parentItem);

	void zoomToLayer();
	void removeSelectedLayer(); // 移除图层
	void saveSelectedLayer(); // 保存图层
	void viewLayerAttributes(); // 查看属性表

	/*************************************************
	 * Function: applyCategorizedSymbology
	 * Description: 应用分类符号化到矢量图层
	 * Parameters:
	 * layer : 矢量图层指针 QgsVectorLayer*
	 * fieldName : 字段名称 QString
	 *************************************************/
	void applyCategorizedSymbology(QgsVectorLayer* layer, const QString& fieldName);

	// 应用图层注记
	void applyLayerLabels(QgsVectorLayer* vectorLayer, const QString& upFieldName, const QString& mainFieldName, const QString& downFieldName, int fontSize, QColor fontColor,
		bool useBuffer, QColor bufferColor, double bufferSize, bool isTemporary);

	QgsMapCanvas* mpMapCanvas; // 绘图画布
	QTreeWidget* mpFileTreeWidget; // 文件树
	QgsLayerTreeView* mpLayerTreeView; // 图层树
	QgsLayerTreeModel* mpLayerTreeModel; // 图层树模式
	QString mSelectedFilePath; // 选择目录路径
	QMenu* mpContextMenu; // 右键菜单
	SymbolLibrary* symbolLibrary; // 符号库
	QColor mPointColor;
	int mPointSize = 2;
	double mPointOpacity;
	QColor mLineColor;
	int mLineWidth;
	double mLineOpacity;
	QColor mPolygonColor;
	double mPolygonOpacity;

	MainWindow* mMainWindow;  // 引用 MainWindow

};
