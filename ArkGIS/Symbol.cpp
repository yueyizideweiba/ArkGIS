#include "Symbol.h"
#include <QPainter>
#include <QDomElement>
#include <qsvggenerator.h>
#include <QByteArray>
#include <QSvgRenderer>
#include <QXmlStreamWriter>
#include <QFile>
#include <QMessageBox>
#include <QPainter>
#include <qgssymbollayer.h>
#include <qgsmarkersymbollayer.h>
#include <qgsmarkersymbol.h>

// 绘制 SVG 符号
void drawSvgMarker(QPainter& painter, const SymbolLayer& layer, int size) {
	QString svgData = layer.properties.value("name");

	// 解码 Base64 的 SVG 数据
	QByteArray svgBytes = QByteArray::fromBase64(svgData.toUtf8());
	QSvgRenderer svgRenderer(svgBytes);

	// 在指定大小的矩形区域内绘制 SVG
	QRect targetRect(-size / 2, -size / 2, size, size);
	svgRenderer.render(&painter, targetRect);
}

// 将逗号分隔的颜色字符串转换为 QColor
QColor parseColor(const QString& colorStr) {
	QStringList rgba = colorStr.split(',');
	if (rgba.size() != 4) return QColor(0, 0, 0, 255);  // 默认黑色

	int r = rgba[0].toInt();
	int g = rgba[1].toInt();
	int b = rgba[2].toInt();
	int a = rgba[3].toInt();
	return QColor(r, g, b, a);
}

QPixmap Symbol::generatePreview(int size) const {
	// 创建高分辨率的画布
	QPixmap pixmap(size * 2, size * 2);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);

	// 定义线条，从左到右水平
	QLineF line(0, size, size * 2, size);

	// 遍历每个图层进行绘制
	for (const SymbolLayer& layer : layers) {
		layer.draw(painter, size * 2, line);  // 调用图层的绘制方法
	}

	// 缩放到指定大小
	return pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

// 将符号及其图层导出为 XML
bool Symbol::exportToXML(QDomDocument& doc, QDomElement& parent) const {
	// 第一层：创建 <symbol> 元素并设置属性
	QDomElement symbolElem = doc.createElement("symbol");
	symbolElem.setAttribute("name", name);
	symbolElem.setAttribute("type", type);  // 根据符号类型输出 type
	symbolElem.setAttribute("tags", tags);
	symbolElem.setAttribute("alpha", QString::number(alpha));

	// 第二层：遍历所有图层并添加为 <layer> 元素
	for (const SymbolLayer& layer : layers) {
		QDomElement layerElem = doc.createElement("layer");
		QString layerClassString;

		switch (layer.symbolClass) {
		case SymbolClass::SimpleMarker:
			layerClassString = "SimpleMarker";
			break;
		case SymbolClass::SvgMarker:
			layerClassString = "SvgMarker";
			break;
		case SymbolClass::SimpleLine:
			layerClassString = "SimpleLine";
			break;
		case SymbolClass::ArrowLine:
			layerClassString = "ArrowLine";
			break;
		case SymbolClass::SimpleFill:
			layerClassString = "SimpleFill";
			break;
		case SymbolClass::PointPatternFill:
			layerClassString = "PointPatternFill";
			break;
		case SymbolClass::CircleLayer:
			layerClassString = "CircleLayer";
			break;
		case SymbolClass::SquareLayer:
			layerClassString = "SquareLayer";
			break;
		case SymbolClass::TriangleLayer:
			layerClassString = "TriangleLayer";
			break;
		case SymbolClass::PentagonLayer:
			layerClassString = "PentagonLayer";
			break;
		case SymbolClass::StarLayer:
			layerClassString = "StarLayer";
			break;
		default:
			layerClassString = "UnknownLayer";
			break;
		}
		layerElem.setAttribute("class", layerClassString);

		// 第三层：添加图层的属性（prop）作为子元素
		for (auto it = layer.properties.begin(); it != layer.properties.end(); ++it) {
			QDomElement propElem = doc.createElement("prop");
			propElem.setAttribute("k", it.key());
			propElem.setAttribute("v", it.value());
			layerElem.appendChild(propElem);
		}

		// 如果图层有 SVG 数据，将其编码为 base64 并存储
		if (!layer.svgData.isEmpty()) {
			QDomElement svgPropElem = doc.createElement("prop");
			svgPropElem.setAttribute("k", "name");
			svgPropElem.setAttribute("v", "base64:" + QString(layer.svgData.toBase64()));
			layerElem.appendChild(svgPropElem);
		}

		// 将图层添加到 symbol 中
		symbolElem.appendChild(layerElem);
	}

	// 将符号添加到父元素中
	parent.appendChild(symbolElem);
	return true;
}

QString processSvg(const QByteArray& svgData, const QMap<QString, QString>& properties) {
	QString svgString = QString::fromUtf8(svgData);

	// 替换 'param(fill) #00' 或类似的表达式
	if (properties.contains("color")) {
		QColor fillColor = parseColor(properties["color"]);
		// 使用正则表达式替换 'param(fill)' 后面的颜色代码
		QRegExp fillRegex("param\\(fill\\)\\s*#[0-9a-fA-F]{2}");
		svgString.replace(fillRegex, fillColor.name(QColor::HexRgb));
	}
	else {
		svgString.replace("param(fill) #00", "#000000");
	}

	// 替换 'param(fill-opacity)'
	if (properties.contains("color")) {
		QColor fillColor = parseColor(properties["color"]);
		svgString.replace("param(fill-opacity)", QString::number(fillColor.alphaF()));
	}
	else {
		svgString.replace("param(fill-opacity)", "1.0");
	}

	// 替换 'param(outline) #fff' 或类似的表达式
	if (properties.contains("outline_color")) {
		QColor strokeColor = parseColor(properties["outline_color"]);
		QRegExp strokeRegex("param\\(outline\\)\\s*#[0-9a-fA-F]{3}");
		svgString.replace(strokeRegex, strokeColor.name(QColor::HexRgb));
	}
	else {
		svgString.replace("param(outline) #fff", "#FFFFFF");
	}

	// 替换 'param(outline-width) 0' 或类似的表达式
	if (properties.contains("outline_width")) {
		float strokeWidth = properties["outline_width"].toFloat();
		QRegExp strokeWidthRegex("param\\(outline-width\\)\\s*\\d+");
		svgString.replace(strokeWidthRegex, QString::number(strokeWidth));
	}
	else {
		svgString.replace("param(outline-width) 0", "1.0");
	}

	return svgString;
}



// 解析 XML 并创建符号对象
Symbol Symbol::importFromXML(const QDomElement& element) {
	Symbol symbol;

	// 第一层：解析 symbol 元素
	symbol.name = element.attribute("name", "Unnamed Symbol");
	QString symbolTypeString = element.attribute("type", "unknown");
	symbol.tags = element.attribute("tags", "");
	symbol.alpha = element.attribute("alpha", "1.0").toFloat();

	// 将 symbolTypeString 转换为 SymbolType 枚举
	SymbolType symbolType;
	if (symbolTypeString == "marker") {
		symbolType = SymbolType::Marker;
	}
	else if (symbolTypeString == "line") {
		symbolType = SymbolType::Line;
	}
	else if (symbolTypeString == "fill") {
		symbolType = SymbolType::Fill;
	}
	else {
		symbolType = SymbolType::Marker;  // 默认类型
	}

	// 第二层：遍历所有 layer 节点
	QDomNodeList layerNodes = element.elementsByTagName("layer");
	for (int i = 0; i < layerNodes.size(); ++i) {
		QDomElement layerElem = layerNodes.at(i).toElement();
		QString layerClassString = layerElem.attribute("class", "UnknownLayer");

		// 将 layerClassString 转换为 SymbolClass 枚举
		SymbolClass symbolclass;
		if (layerClassString == "SimpleMarker") {
			symbolclass = SymbolClass::SimpleMarker;
		}
		else if (layerClassString == "SvgMarker") {
			symbolclass = SymbolClass::SvgMarker;
		}
		else if (layerClassString == "SimpleLine") {
			symbolclass = SymbolClass::SimpleLine;
		}
		else if (layerClassString == "MarkerLine") {
			symbolclass = SymbolClass::MarkerLine;
		}
		else if (layerClassString == "ArrowLine") {
			symbolclass = SymbolClass::ArrowLine;
		}
		else if (layerClassString == "SimpleFill") {
			symbolclass = SymbolClass::SimpleFill;
		}
		else if (layerClassString == "PointPatternFill") {
			symbolclass = SymbolClass::PointPatternFill;
		}
		else if (layerClassString == "CircleLayer") {
			symbolclass = SymbolClass::CircleLayer;
		}
		else if (layerClassString == "SquareLayer") {
			symbolclass = SymbolClass::SquareLayer;
		}
		else if (layerClassString == "TriangleLayer") {
			symbolclass = SymbolClass::TriangleLayer;
		}
		else if (layerClassString == "PentagonLayer") {
			symbolclass = SymbolClass::PentagonLayer;
		}
		else if (layerClassString == "StarLayer") {
			symbolclass = SymbolClass::StarLayer;
		}
		else {
			symbolclass = SymbolClass::SimpleMarker;  // 默认类型
		}

		// 创建 SymbolLayer 对象
		SymbolLayer layer(symbolType, symbolclass);

		// 第三层：解析每个 layer 中的 prop 属性
		QDomNodeList propNodes = layerElem.elementsByTagName("prop");
		for (int j = 0; j < propNodes.size(); ++j) {
			QDomElement propElem = propNodes.at(j).toElement();
			QString key = propElem.attribute("k");
			QString value = propElem.attribute("v");

			// 将属性存储到 layer 的 properties 中
			layer.properties[key] = value;

			// 特殊处理 SVG 数据
			if (key == "name" && value.startsWith("base64:")) {
				layer.svgData = QByteArray::fromBase64(value.mid(7).toUtf8());
			}
		}

		// 检查是否有嵌套的 symbol 元素（例如 MarkerLine 中的 SimpleMarker）
		QDomNodeList nestedSymbolNodes = layerElem.elementsByTagName("symbol");
		for (int j = 0; j < nestedSymbolNodes.size(); ++j) {
			QDomElement nestedSymbolElem = nestedSymbolNodes.at(j).toElement();
			Symbol nestedSymbol = Symbol::importFromXML(nestedSymbolElem);
			// 将嵌套的 Symbol 转换为 SymbolLayer 并添加到 childLayers
			for (const SymbolLayer& nestedLayer : nestedSymbol.layers) {
				layer.childLayers.append(nestedLayer);
			}
		}

		// 将解析出的图层添加到 symbol 的 layers 列表中
		symbol.layers.append(layer);
	}

	return symbol;
}


// 导出符号为 SVG
bool Symbol::exportToSVG(const QString& filePath) const {
	QSvgGenerator generator;
	generator.setFileName(filePath);
	generator.setSize(QSize(128, 128));  // 设置输出大小（SVG视图大小）
	generator.setViewBox(QRect(0, 0, 128, 128));  // 设置视图窗口
	generator.setTitle(name);  // 设置 SVG 标题

	QPainter painter(&generator);
	painter.setRenderHint(QPainter::Antialiasing);  // 启用抗锯齿

	// 遍历符号的所有图层
	for (const SymbolLayer& layer : layers) {
		switch (layer.symbolType) {
		case SymbolType::Marker:
			if (layer.symbolClass == SymbolClass::SimpleMarker) {
				painter.setBrush(layer.fillColor);
				painter.setPen(QPen(layer.strokeColor, layer.strokeWidth));
				painter.drawEllipse(QRectF(0, 0, 128, 128));
			}
			else if (layer.symbolClass == SymbolClass::SvgMarker && !layer.svgData.isEmpty()) {
				QSvgRenderer svgRenderer(layer.svgData);
				svgRenderer.render(&painter, QRectF(0, 0, 128, 128));
			}
			break;

		case SymbolType::Line:
			if (layer.symbolClass == SymbolClass::SimpleLine) {
				QPen pen(layer.strokeColor, layer.strokeWidth);
				painter.setPen(pen);
				painter.drawLine(QPointF(0, 64), QPointF(128, 64));  // 绘制简单线条
			}
			else if (layer.symbolClass == SymbolClass::ArrowLine) {
				QPen pen(layer.strokeColor, layer.strokeWidth);
				painter.setPen(pen);
				painter.drawLine(QPointF(0, 64), QPointF(128, 64));  // 绘制箭头线
				// 可以进一步在终点位置添加箭头形状的绘制逻辑
			}
			break;

		case SymbolType::Fill:
			if (layer.symbolClass == SymbolClass::SimpleFill) {
				QPen pen(layer.strokeColor, layer.strokeWidth);
				painter.setPen(pen);
				painter.setBrush(layer.fillColor);
				painter.drawRect(0, 0, 128, 128);  // 绘制填充的矩形
			}
			else if (layer.symbolClass == SymbolClass::PointPatternFill) {
				// 这里可以实现绘制点模式填充的逻辑
			}
			break;
		}

		// 如果图层包含其他图形或变换属性，可以在此进行进一步的处理
		if (layer.properties.contains("angle")) {
			qreal angle = layer.properties["angle"].toDouble();
			QTransform transform;
			transform.rotate(angle);
			painter.setTransform(transform, true);
		}

		if (layer.properties.contains("offset")) {
			QStringList offsets = layer.properties["offset"].split(",");
			qreal offsetX = offsets[0].toDouble();
			qreal offsetY = offsets[1].toDouble();
			painter.translate(offsetX, offsetY);
		}
	}

	painter.end();  // 结束绘制
	return QFile::exists(filePath);  // 检查文件是否成功创建
}

// 辅助函数：绘制 SimpleMarker
void drawSimpleMarker(QPainter& painter, const SymbolLayer& markerLayer, QPointF position, qreal angle, int size, QByteArray svgData, QMap<QString, QString> properties) {
	// 保存当前画笔状态
	painter.save();

	// 应用旋转和位移
	painter.translate(position);
	painter.rotate(angle);

	// 设置颜色和笔
	QColor color = Qt::black;
	if (markerLayer.properties.contains("color")) {
		color = parseColor(markerLayer.properties["color"]);
	}
	painter.setBrush(color);

	QColor outlineColor = Qt::NoPen;
	if (markerLayer.properties.contains("outline_color")) {
		outlineColor = parseColor(markerLayer.properties["outline_color"]);
	}
	float outlineWidth = 0.0f;
	if (markerLayer.properties.contains("outline_width")) {
		outlineWidth = markerLayer.properties["outline_width"].toFloat();
	}
	painter.setPen(QPen(outlineColor, outlineWidth));

	// 解析大小
	int markerSize = size;
	if (markerLayer.properties.contains("size")) {
		markerSize = markerLayer.properties["size"].toInt();
	}

	// 根据 markerClass 绘制具体形状
	switch (markerLayer.symbolClass) {
	case SymbolClass::SimpleMarker: {
		// 绘制简单标记符号
		painter.setBrush(color);
		painter.setPen(QPen(outlineColor, outlineWidth));
		painter.drawEllipse(QRectF(0, 0, size, size));
		break;
	}
	case SymbolClass::SvgMarker:
	{
		// 处理 SVG 数据中的 param() 表达式
		QString processedSvg = processSvg(svgData, properties);

		// 转换回 QByteArray
		QByteArray processedSvgBytes = processedSvg.toUtf8();

		// 使用更高分辨率的画布，确保绘制质量
		int highResSize = size * 2;
		QPixmap pixmap(highResSize, highResSize);
		pixmap.fill(Qt::transparent);

		QPainter layerPainter(&pixmap);
		layerPainter.setRenderHint(QPainter::Antialiasing);



		// 获取旋转角度
		qreal angle = 0;
		if (properties.contains("angle")) {
			angle = properties.value("angle").toDouble();
		}

		// 获取偏移值
		qreal offsetX = 0, offsetY = 0;
		if (properties.contains("offset")) {
			QStringList offsets = properties.value("offset").split(",");
			if (offsets.size() == 2) {
				offsetX = offsets[0].toDouble() * 2;  // 高分辨率下偏移值扩大2倍
				offsetY = offsets[1].toDouble() * 2;
			}
		}

		// 应用旋转和偏移
		layerPainter.save();
		layerPainter.translate(highResSize / 2 + offsetX, highResSize / 2 + offsetY);  // 平移到中心并加上偏移
		layerPainter.rotate(angle);  // 旋转
		layerPainter.translate(-highResSize / 2, -highResSize / 2);  // 将图层平移回原点

		// 渲染处理后的 SVG 数据
		if (!processedSvgBytes.isEmpty()) {
			QSvgRenderer svgRenderer(processedSvgBytes);
			if (!svgRenderer.isValid()) {
			}
			else {
				QRectF renderRect(0, 0, highResSize, highResSize);
				svgRenderer.render(&layerPainter, renderRect);
			}
		}

		// 恢复画笔状态
		layerPainter.restore();

		// 将高分辨率图像缩小到目标尺寸
		painter.drawPixmap(0, 0, pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
	break;
	}
	// 恢复画笔状态
	painter.restore();
}

void SymbolLayer::draw(QPainter& painter, int size, const QLineF& line) const {

	// 解析颜色属性
	QColor fillColor = Qt::black;  // 默认填充颜色为黑色
	if (properties.contains("color")) {
		QStringList rgba = properties["color"].split(",");
		if (rgba.size() == 4) {
			fillColor = QColor(rgba[0].toInt(), rgba[1].toInt(), rgba[2].toInt(), rgba[3].toInt());
		}
	}

	QColor strokeColor = Qt::NoPen;  // 默认无边框
	if (properties.contains("outline_color")) {
		QStringList rgba = properties["outline_color"].split(",");
		if (rgba.size() == 4) {
			strokeColor = QColor(rgba[0].toInt(), rgba[1].toInt(), rgba[2].toInt(), rgba[3].toInt());
		}
	}

	float strokeWidth = 0.0f;  // 默认边线宽度
	if (properties.contains("outline_width")) {
		strokeWidth = properties["outline_width"].toFloat();
	}

	// 解析图层大小
	int symbolSize = size;  // 默认大小使用传入的 size 参数
	if (properties.contains("size")) {
		symbolSize = properties["size"].toInt();
	}

	// 使用解析到的大小和颜色设置画笔
	painter.setBrush(fillColor);
	painter.setPen(QPen(strokeColor, strokeWidth));

	// 应用旋转角度
	if (properties.contains("angle")) {
		qreal angle = properties["angle"].toDouble();
		painter.save();  // 保存当前状态
		painter.translate(size / 2, size / 2);  // 将画布平移到中心点
		painter.rotate(angle);  // 旋转
		painter.translate(-size / 2, -size / 2);  // 再平移回去
	}

	// 应用偏移
	if (properties.contains("offset")) {
		QStringList offsets = properties["offset"].split(",");
		if (offsets.size() == 2) {
			qreal offsetX = offsets[0].toDouble();
			qreal offsetY = offsets[1].toDouble();
			painter.translate(offsetX * 2, offsetY * 2);  // 应用两倍尺寸的偏移
		}
	}

	// 恢复画笔状态
	painter.restore();

	// 根据符号类型 SymbolType 来处理不同的图层
	switch (symbolType) {
	case SymbolType::Marker: {
		// Marker 类型的图层
		switch (symbolClass) {
		case SymbolClass::CircleLayer: {
			// 确保设置填充颜色和笔
			painter.setBrush(fillColor);  // 使用设置好的填充颜色
			painter.setPen(QPen(strokeColor, strokeWidth));  // 使用设置好的边框颜色和宽度

			// 设置圆的半径
			int radius = properties.value("size", QString::number(size / 2)).toInt();

			// 绘制圆形
			painter.drawEllipse(QRectF((size - radius * 2) / 2, (size - radius * 2) / 2, radius * 2, radius * 2));
			break;
		}
		case SymbolClass::SquareLayer: {
			int side = properties.value("size", QString::number(size)).toInt();
			painter.drawRect(QRectF((size - side) / 2, (size - side) / 2, side, side));
			break;
		}
		case SymbolClass::TriangleLayer: {
			QPolygonF triangle;
			triangle << QPointF(size / 2, 0)
				<< QPointF(0, size)
				<< QPointF(size, size);
			painter.drawPolygon(triangle);
			break;
		}
		case SymbolClass::PentagonLayer: {
			// 确保设置填充颜色和笔
			painter.setBrush(fillColor);  // 使用设置好的填充颜色
			painter.setPen(QPen(strokeColor, strokeWidth));  // 使用设置好的边框颜色和宽度

			// 绘制五边形
			QPolygonF pentagon;
			for (int i = 0; i < 5; ++i) {
				qreal angle = 2 * M_PI * i / 5;
				pentagon << QPointF(size / 2 + (size / 3) * cos(angle), size / 2 + (size / 3) * sin(angle));
			}
			painter.drawPolygon(pentagon);  // 绘制多边形
			break;
		}
		case SymbolClass::StarLayer: {
			QPolygonF star;
			for (int i = 0; i < 10; ++i) {
				qreal angle = M_PI * i / 5;
				qreal radius = (i % 2 == 0) ? size / 3 : size / 6;
				star << QPointF(size / 2 + radius * cos(angle),
					size / 2 + radius * sin(angle));
			}
			painter.drawPolygon(star);
			break;
		}
		case SymbolClass::SimpleMarker: {
			// 绘制简单标记符号
			painter.setBrush(fillColor);
			painter.setPen(QPen(strokeColor, strokeWidth));
			painter.drawEllipse(QRectF(0, 0, symbolSize, symbolSize));
			break;
		}
		case SymbolClass::SvgMarker:
		{
			// 处理 SVG 数据中的 param() 表达式
			QString processedSvg = processSvg(svgData, properties);

			// 转换回 QByteArray
			QByteArray processedSvgBytes = processedSvg.toUtf8();

			// 使用更高分辨率的画布，确保绘制质量
			int highResSize = size * 2;
			QPixmap pixmap(highResSize, highResSize);
			pixmap.fill(Qt::transparent);

			QPainter layerPainter(&pixmap);
			layerPainter.setRenderHint(QPainter::Antialiasing);

			// 获取旋转角度
			qreal angle = 0;
			if (properties.contains("angle")) {
				angle = properties.value("angle").toDouble();
			}

			// 获取偏移值
			qreal offsetX = 0, offsetY = 0;
			if (properties.contains("offset")) {
				QStringList offsets = properties.value("offset").split(",");
				if (offsets.size() == 2) {
					offsetX = offsets[0].toDouble() * 2;  // 高分辨率下偏移值扩大2倍
					offsetY = offsets[1].toDouble() * 2;
				}
			}

			// 应用旋转和偏移
			layerPainter.save();
			layerPainter.translate(highResSize / 2 + offsetX, highResSize / 2 + offsetY);  // 平移到中心并加上偏移
			layerPainter.rotate(angle);  // 旋转
			layerPainter.translate(-highResSize / 2, -highResSize / 2);  // 将图层平移回原点

			// 渲染处理后的 SVG 数据
			if (!processedSvgBytes.isEmpty()) {
				QSvgRenderer svgRenderer(processedSvgBytes);
				if (!svgRenderer.isValid()) {
				}
				else {
					QRectF renderRect(0, 0, highResSize, highResSize);
					svgRenderer.render(&layerPainter, renderRect);
				}
			}

			// 恢复画笔状态
			layerPainter.restore();

			// 将高分辨率图像缩小到目标尺寸
			painter.drawPixmap(0, 0, pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
		break;

		default:
			break;
		}
		break;
	}
	case SymbolType::Line: {
		// Line 类型的图层
		switch (symbolClass) {
		case SymbolClass::SimpleLine: {
			// 绘制简单线条
			QPen pen;
			if (properties.contains("line_color")) {
				pen.setColor(parseColor(properties["line_color"]));
			}
			else {
				pen.setColor(Qt::black);
			}

			if (properties.contains("line_width")) {
				pen.setWidthF(properties["line_width"].toFloat());
			}
			else {
				pen.setWidthF(1.0);
			}

			if (properties.contains("line_style")) {
				QString style = properties["line_style"];
				if (style == "solid") {
					pen.setStyle(Qt::SolidLine);
				}
				else if (style == "dash") {
					pen.setStyle(Qt::DashLine);
				}
				// 可以根据需要添加其他样式
				else {
					pen.setStyle(Qt::SolidLine);
				}
			}
			else {
				pen.setStyle(Qt::SolidLine);
			}

			if (properties.contains("capstyle")) {
				QString cap = properties["capstyle"];
				if (cap == "square") {
					pen.setCapStyle(Qt::SquareCap);
				}
				else if (cap == "round") {
					pen.setCapStyle(Qt::RoundCap);
				}
				else if (cap == "flat") {
					pen.setCapStyle(Qt::FlatCap);
				}
				else {
					pen.setCapStyle(Qt::SquareCap);
				}
			}

			if (properties.contains("joinstyle")) {
				QString join = properties["joinstyle"];
				if (join == "bevel") {
					pen.setJoinStyle(Qt::BevelJoin);
				}
				else if (join == "miter") {
					pen.setJoinStyle(Qt::MiterJoin);
				}
				else if (join == "round") {
					pen.setJoinStyle(Qt::RoundJoin);
				}
				else {
					pen.setJoinStyle(Qt::BevelJoin);
				}
			}

			painter.setPen(pen);

			// 应用偏移（如果有）
			if (properties.contains("offset")) {
				QStringList offsets = properties["offset"].split(',');
				if (offsets.size() == 2) {
					qreal offsetX = offsets[0].toDouble();
					qreal offsetY = offsets[1].toDouble();
					painter.translate(offsetX, offsetY);
				}
			}

			// 绘制线条
			painter.drawLine(line);
			break;
		}
		case SymbolClass::MarkerLine: {
			// 绘制 MarkerLine
			// 首先，绘制线条
			QPen pen;
			if (properties.contains("line_color")) {
				pen.setColor(parseColor(properties["line_color"]));
			}
			else {
				pen.setColor(Qt::black);
			}

			if (properties.contains("line_width")) {
				pen.setWidthF(properties["line_width"].toFloat());
			}
			else {
				pen.setWidthF(1.0);
			}

			if (properties.contains("line_style")) {
				QString style = properties["line_style"];
				if (style == "solid") {
					pen.setStyle(Qt::SolidLine);
				}
				else if (style == "dash") {
					pen.setStyle(Qt::DashLine);
				}
				// 可以根据需要添加其他样式
				else {
					pen.setStyle(Qt::SolidLine);
				}
			}
			else {
				pen.setStyle(Qt::SolidLine);
			}

			if (properties.contains("capstyle")) {
				QString cap = properties["capstyle"];
				if (cap == "square") {
					pen.setCapStyle(Qt::SquareCap);
				}
				else if (cap == "round") {
					pen.setCapStyle(Qt::RoundCap);
				}
				else if (cap == "flat") {
					pen.setCapStyle(Qt::FlatCap);
				}
				else {
					pen.setCapStyle(Qt::SquareCap);
				}
			}

			if (properties.contains("joinstyle")) {
				QString join = properties["joinstyle"];
				if (join == "bevel") {
					pen.setJoinStyle(Qt::BevelJoin);
				}
				else if (join == "miter") {
					pen.setJoinStyle(Qt::MiterJoin);
				}
				else if (join == "round") {
					pen.setJoinStyle(Qt::RoundJoin);
				}
				else {
					pen.setJoinStyle(Qt::BevelJoin);
				}
			}

			painter.setPen(pen);

			// 应用偏移（如果有）
			if (properties.contains("offset")) {
				QStringList offsets = properties["offset"].split(',');
				if (offsets.size() == 2) {
					qreal offsetX = offsets[0].toDouble();
					qreal offsetY = offsets[1].toDouble();
					painter.translate(offsetX, offsetY);
				}
			}

			// 绘制线条
			painter.drawLine(line);

			// 计算标记的间隔
			qreal interval = 0.2; // 默认间隔
			if (properties.contains("interval")) {
				interval = properties["interval"].toDouble();
			}

			// 标记间隔基于线的长度
			qreal lineLength = line.length();
			qreal spacing = lineLength * interval;

			// 获取是否旋转标记
			bool rotateMarkers = false;
			if (properties.contains("rotate")) {
				rotateMarkers = properties["rotate"].toInt() != 0;
			}

			// 获取 childLayers 中的 SimpleMarker
			for (const SymbolLayer& childLayer : childLayers) {
				if (childLayer.symbolType == SymbolType::Marker && childLayer.symbolClass == SymbolClass::SimpleMarker) {
					// 绘制标记
					int numMarkers = static_cast<int>(lineLength / spacing);
					for (int m = 0; m < numMarkers; ++m) {
						qreal distance = m * spacing + spacing / 2;
						if (distance > lineLength) break;
						QPointF point = line.pointAt(distance / lineLength);

						// 计算角度
						qreal angle = 0.0;
						if (rotateMarkers) {
							// 计算线的角度
							angle = std::atan2(line.dy(), line.dx()) * 180.0 / M_PI;
						}

						// 获取 marker size
						qreal markerSize = 0.375; // 默认大小
						if (childLayer.properties.contains("size")) {
							markerSize = childLayer.properties["size"].toFloat();
						}

						drawSimpleMarker(painter, childLayer, point, angle, size * 0.375, svgData, properties); // 使用 SimpleMarker 的 size
					}
				}
			}

			break;
		}
		default:
			break;
		}
		break;
	}
	case SymbolType::Fill: {
		// Fill 类型的图层
		switch (symbolClass) {
		case SymbolClass::SimpleFill: {
			// 绘制简单填充区域
			painter.setBrush(fillColor);
			painter.setPen(QPen(strokeColor, strokeWidth));
			painter.drawRect(QRectF(0, 0, size, size));
			break;
		}
		case SymbolClass::PointPatternFill: {
			// 点模式填充，读取点距离和大小
			int distanceX = 10, distanceY = 10;
			if (properties.contains("distance_x")) {
				distanceX = properties.value("distance_x").toInt();
			}
			if (properties.contains("distance_y")) {
				distanceY = properties.value("distance_y").toInt();
			}

			for (int x = 0; x < size; x += distanceX) {
				for (int y = 0; y < size; y += distanceY) {
					painter.drawEllipse(QPointF(x, y), symbolSize / 10, symbolSize / 10);  // 绘制小点
				}
			}
			break;
		}
		case SymbolClass::RandomMarkerFill: {
			// 随机标记填充，读取点数量
			int pointCount = 10;  // 默认10个点
			if (properties.contains("point_count")) {
				pointCount = properties.value("point_count").toInt();
			}
			break;
		}
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
}

// 判断符号是否为 SimpleMarker 类型
bool Symbol::isSimpleMarker() const {
	// 检查符号的第一个图层是否为 SimpleMarker
	if (!layers.isEmpty()) {
		return layers.first().symbolClass == SymbolClass::SimpleMarker;
	}
	return false;
}

// 获取 SimpleMarker 的颜色
QColor Symbol::getColor() const {
	if (isSimpleMarker() && !layers.isEmpty()) {
		return layers.first().fillColor;
	}
	return QColor(); // 返回默认颜色
}

// 设置 SimpleMarker 的颜色
void Symbol::setColor(const QColor& color) {
	if (isSimpleMarker() && !layers.isEmpty()) {
		layers.first().fillColor = color;
	}
}

// 获取 SimpleMarker 的大小
int Symbol::getSize() const {
	if (isSimpleMarker() && !layers.isEmpty()) {
		// 假设大小存储在属性中，键为 "size"
		if (layers.first().properties.contains("size")) {
			bool ok;
			int size = layers.first().properties["size"].toInt(&ok);
			if (ok) {
				return size;
			}
		}
		// 如果属性中没有 "size" 键，则返回默认大小，例如 10
		return 10;
	}
	return 0; // 非 SimpleMarker 返回 0
}

// 设置 SimpleMarker 的大小
void Symbol::setSize(int size) {
	if (isSimpleMarker() && !layers.isEmpty()) {
		// 将大小存储在属性中，键为 "size"
		layers.first().properties["size"] = QString::number(size);
	}
}

Symbol Symbol::fromQgsSymbol(const QgsSymbol* qgsSymbol) {
	Symbol symbol;

	const QgsMarkerSymbol* markerSymbol = dynamic_cast<const QgsMarkerSymbol*>(qgsSymbol);
	if (!markerSymbol) {
		return symbol;
	}

	symbol.type = "marker";
	symbol.alpha = markerSymbol->opacity();

	const QgsSimpleMarkerSymbolLayer* simpleLayer = dynamic_cast<const QgsSimpleMarkerSymbolLayer*>(markerSymbol->symbolLayer(0));
	if (simpleLayer) {
		SymbolLayer layer(SymbolType::Marker, SymbolClass::SimpleMarker);
		layer.strokeColor = simpleLayer->strokeColor();
		layer.fillColor = simpleLayer->fillColor();
		layer.strokeWidth = simpleLayer->strokeWidth();

		int size = static_cast<int>(simpleLayer->size());
		symbol.setSize(size);

		symbol.layers.append(layer);
	}

	return symbol;
}
