/************************************************************
FileName: StyleManager.h
Author: XYH
Version : 1.0
Date: 2024-10-25
Description: 样式管理器类，继承自QObject，用于管理和应用地图图层的样式。
Function List:
1. StyleManager - 构造函数，初始化样式管理器
2. openStyleManager - 打开样式管理器，对指定图层应用样式
3. createStyleDialog - 创建样式管理对话框，用于用户选择和配置样式
4. loadSystemSymbolLibrary - 加载系统符号库，提供样式选择
*************************************************************/

#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QObject>
#include <QDialog>
#include <QComboBox>
#include <QgsSymbol.h>
#include <QgsMapLayer.h>
#include<QVBoxLayout>
#include<QLabel>
#include<QgsColorRampShader.h>
#include<QgsSingleBandPseudoColorRenderer.h>
#include<vector>
class QgsVectorLayer;
class QgsRasterLayer;
class QgsStyle;
class QgsRendererWidget;
class QgsCategorizedSymbolRenderer;

class StyleManager : public QObject
{
	Q_OBJECT

public:
	explicit StyleManager(QObject* parent = nullptr);
	void openStyleManager(QgsMapLayer* layer); // 打开样式管理器
	//void applyRasterStyle(QgsRasterLayer* layer, int band, int renderMode);
	void setupColorRampSelector(QVBoxLayout* layout, QColor& startColor, QColor& endColor); // 设置颜色渐变选择器
	void drawColorRamp(QPixmap& pixmap, QColor& startColor, QColor& endColor); // 绘制颜色渐变
	void applyRasterStyle(QgsRasterLayer* layer, int band, int renderMode, QColor startColor, QColor endColor);
	QDialog* createTifStyleDialog(QgsRasterLayer* layer); // 创建栅格样式管理对话框
	QColor m_startColor;
	QColor m_endColor;
	QgsRasterLayer* gloBalrasterLayer;
	std::vector<QgsColorRampShader> shader; // 颜色渐变着色器
	QLabel* colorRampLabel; // 颜色渐变标签
	QPixmap colorRampPixmap;
	std::vector<QgsSingleBandPseudoColorRenderer* >renderer;

	QComboBox* bandComboBox;
	std::vector<QComboBox* >renderModeComboBox;
	QColor interpolateColor(const QColor& startColor, const QColor& endColor, double ratio);
	void clearParameters();

private:
	QDialog* createStyleDialog(QgsVectorLayer* layer); // 创建样式管理对话框
	QDialog* dialog;
};

#endif