/************************************************************
FileName: Symbol.h
Author: XYH
Version : 1.0
Date: 2024-10-26
Description: 符号类定义文件，包含Symbol和SymbolLayer类，用于表示和处理地图符号。
Function List:
1. SymbolLayer构造函数 - 初始化符号图层对象
2. draw - 绘制当前图层
3. Symbol构造函数 - 初始化符号对象
4. generatePreview - 生成符号预览图
5. exportToXML - 导出符号为XML格式
6. importFromXML - 从XML格式导入符号
7. exportToSVG - 导出符号为SVG格式
8. isSimpleMarker - 判断是否为简单标记符号
9. getColor - 获取符号颜色
10. setColor - 设置符号颜色
11. getSize - 获取符号大小
12. setSize - 设置符号大小
13. fromQgsSymbol - 从QgsSymbol对象创建Symbol对象

Attributes:
1. SymbolType - 枚举类型，定义了符号类型，如Marker, Line, Fill
2. SymbolClass - 枚举类型，定义了符号图层类型，如SimpleMarker, SvgMarker, SimpleLine等
*************************************************************/

#ifndef SYMBOL_H
#define SYMBOL_H

#include <QString>
#include <QList>
#include <QMap>
#include <QDomElement>
#include <QPixmap>
#include <QColor>
#include <QgsSymbol.h>

enum class SymbolType {
	Marker,
	Line,
	Fill
};

// 图层类型，如 SimpleFill, SimpleLine, EllipseMarker 等
enum class SymbolClass {
	SimpleMarker,
	SvgMarker,
	SimpleLine,
	MarkerLine,
	ArrowLine,
	RandomMarkerFill,
	PointPatternFill,
	SimpleFill,
	CircleLayer,
	SquareLayer,
	TriangleLayer,
	PentagonLayer,
	StarLayer
};

// 图层类，表示符号的单个图层
class SymbolLayer {
public:
	SymbolType symbolType;  // 符号类型
	SymbolClass symbolClass;    // 图层类型
	QMap<QString, QString> properties;  // 属性键值对
	QByteArray svgData;     // SVG base64 数据

	QColor strokeColor;  // 边线颜色
	QColor fillColor;    // 填充颜色
	float strokeWidth = 1.0;  // 线宽

	QList<SymbolLayer> childLayers; // 用于嵌套符号

	SymbolLayer(SymbolType symbolType = SymbolType::Marker, SymbolClass layerType = SymbolClass::SimpleMarker)
		: symbolType(symbolType), symbolClass(layerType) {}

	// 绘制当前图层
	void draw(QPainter& painter, int size, const QLineF& line) const;

};

// 符号类，表示整个符号，包括多个图层
class Symbol {
public:
	QString name;    // 符号名称
	QString type;    // 符号类型，如 marker
	QString tags;    // 符号标签
	float alpha = 1.0;  // 透明度
	QList<SymbolLayer> layers;  // 符号的图层列表

	Symbol(const QString& name = "") : name(name) {}

	QPixmap generatePreview(int size = 64) const;  // 生成符号预览
	bool exportToXML(QDomDocument& doc, QDomElement& parent) const;  // 导出为XML
	static Symbol importFromXML(const QDomElement& element);  // 从XML导入符号

	bool exportToSVG(const QString& filePath) const;  // 导出为 SVG

	bool isSimpleMarker() const;
	QColor getColor() const;
	void setColor(const QColor& color);
	int getSize() const;
	void setSize(int size);
	static Symbol fromQgsSymbol(const QgsSymbol* qgsSymbol);
};

#endif