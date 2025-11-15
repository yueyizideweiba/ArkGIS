#include "SymbolLibraryManager.h"
#include "SymbolManager.h"
#include <QDir>
#include <QFile>
#include <QMessageBox>

// 获取符号库管理器的单例实例
SymbolLibraryManager& SymbolLibraryManager::instance() {
	static SymbolLibraryManager instance;
	return instance;
}

// 私有构造函数
SymbolLibraryManager::SymbolLibraryManager() = default;

// 初始化符号库：检查并创建默认符号库
void SymbolLibraryManager::initialize() {
	QDir dir(libraryDirectory);
	if (!dir.exists()) dir.mkpath(libraryDirectory);  // 创建目录

	currentLibraryPath = libraryDirectory + "/default.xml";
	if (!QFile::exists(currentLibraryPath)) {
		createDefaultLibrary();  // 创建默认符号库
	}

	loadLibrary(currentLibraryPath);  // 加载默认符号库
}

void SymbolLibraryManager::createDefaultLibrary() {
	// 创建红色圆形符号
	Symbol redCircle("Red Circle");
	redCircle.type = "marker";
	redCircle.alpha = 1.0;

	SymbolLayer circleLayer(SymbolType::Marker, SymbolClass::CircleLayer);
	circleLayer.properties["color"] = "255,0,0,255";  // 红色
	circleLayer.properties["size"] = "120";  // 圆的大小
	redCircle.layers.append(circleLayer);

	// 创建绿色正方形符号
	Symbol greenSquare("Green Square");
	greenSquare.type = "marker";
	greenSquare.alpha = 0.8;

	SymbolLayer squareLayer(SymbolType::Marker, SymbolClass::SquareLayer);
	squareLayer.properties["color"] = "0,255,0,204";  // 绿色，80%透明度
	squareLayer.properties["size"] = "200";  // 正方形的边长
	squareLayer.properties["angle"] = "45";  // 旋转 45 度
	greenSquare.layers.append(squareLayer);

	// 创建蓝色三角形符号
	Symbol blueTriangle("Blue Triangle");
	blueTriangle.type = "marker";
	blueTriangle.alpha = 0.9;

	SymbolLayer triangleLayer(SymbolType::Marker, SymbolClass::TriangleLayer);
	triangleLayer.properties["color"] = "0,0,255,230";  // 蓝色
	triangleLayer.properties["size"] = "20";  // 三角形大小
	triangleLayer.properties["angle"] = "0";  // 无旋转
	blueTriangle.layers.append(triangleLayer);

	// 创建黄色五边形符号
	Symbol yellowPentagon("Yellow Pentagon");
	yellowPentagon.type = "marker";
	yellowPentagon.alpha = 0.85;

	SymbolLayer pentagonLayer(SymbolType::Marker, SymbolClass::PentagonLayer);
	pentagonLayer.properties["color"] = "255,255,0,180";  // 黄色，70%透明度
	pentagonLayer.properties["size"] = "40";  // 五边形大小
	yellowPentagon.layers.append(pentagonLayer);

	// 创建紫色星形符号
	Symbol purpleStar("Purple Star");
	purpleStar.type = "marker";
	purpleStar.alpha = 0.95;

	SymbolLayer starLayer(SymbolType::Marker, SymbolClass::StarLayer);
	starLayer.properties["color"] = "128,0,128,240";  // 紫色
	starLayer.properties["size"] = "50";  // 星形大小
	starLayer.properties["angle"] = "15";  // 旋转 15 度
	purpleStar.layers.append(starLayer);

	// 创建橙色圆形符号并带有偏移
	Symbol orangeCircle("Orange Circle with Offset");
	orangeCircle.type = "marker";
	orangeCircle.alpha = 1.0;

	SymbolLayer offsetCircleLayer(SymbolType::Marker, SymbolClass::CircleLayer);
	offsetCircleLayer.properties["color"] = "255,165,0,255";  // 橙色
	offsetCircleLayer.properties["size"] = "30";  // 圆的大小
	offsetCircleLayer.properties["offset"] = "10,10";  // 偏移 (10, 10)
	orangeCircle.layers.append(offsetCircleLayer);

	// 将符号添加到符号库
	currentLibrary.addSymbol(redCircle);
	currentLibrary.addSymbol(greenSquare);
	currentLibrary.addSymbol(blueTriangle);
	currentLibrary.addSymbol(yellowPentagon);
	currentLibrary.addSymbol(purpleStar);
	currentLibrary.addSymbol(orangeCircle);

	// 保存符号库到 XML 文件
	if (!currentLibraryPath.isEmpty()) {
		currentLibrary.exportToXML(currentLibraryPath);
	}
	else {
		qWarning() << "符号库路径未定义，无法保存默认符号库。";
	}
}

// 设置符号库目录
void SymbolLibraryManager::setLibraryDirectory(const QString& dirPath) {
	libraryDirectory = dirPath;
}

void SymbolLibraryManager::importLibrary(const QString& path) {
	// 如果路径不在已导入的符号库列表中，则添加
	if (!importedLibraries.contains(path)) {
		importedLibraries.append(path);
	}
}


// 获取符号库文件路径列表
QList<QString> SymbolLibraryManager::getAvailableLibraries() const {
	QList<QString> libraryPaths;

	// 获取默认符号库目录中的符号库
	QDir dir(libraryDirectory);
	QStringList files = dir.entryList(QStringList() << "*.xml", QDir::Files);
	for (const QString& file : files) {
		libraryPaths.append(dir.absoluteFilePath(file));
	}

	// 添加已导入的符号库路径
	for (const QString& importedLibrary : importedLibraries) {
		if (!libraryPaths.contains(importedLibrary)) {
			libraryPaths.append(importedLibrary);  // 避免重复添加
		}
	}

	return libraryPaths;
}


// 加载符号库文件
bool SymbolLibraryManager::loadLibrary(const QString& filePath) {
	currentLibrary = SymbolLibrary();  // 清除旧符号库数据

	if (!currentLibrary.importFromXML(filePath)) {
		QMessageBox::warning(nullptr, "错误", "无法加载符号库：" + filePath);
		return false;
	}
	currentLibraryPath = filePath;
	return true;
}

// 获取当前符号库
SymbolLibrary& SymbolLibraryManager::getCurrentLibrary() {
	return currentLibrary;
}

// 获取当前符号库路径
QString SymbolLibraryManager::getCurrentLibraryPath() const {
	return currentLibraryPath;
}

// 将符号添加到当前符号库
bool SymbolLibraryManager::addSymbolToCurrentLibrary(const Symbol& symbol) {
	// 添加符号到当前库
	if (!currentLibrary.addSymbol(symbol)) {
		QMessageBox::warning(nullptr, "错误", "符号已存在，无法添加。");
		return false;
	}

	// 保存符号库到 XML 文件
	if (!currentLibrary.exportToXML(currentLibraryPath)) {
		QMessageBox::warning(nullptr, "错误", "无法保存符号库到文件：" + currentLibraryPath);
		return false;
	}

	return true;
}

// 添加符号到指定库文件
bool SymbolLibraryManager::addSymbolToLibrary(const QString& libraryPath, const Symbol& symbol) {
	// 加载目标符号库文件
	SymbolLibrary library;
	if (!library.importFromXML(libraryPath)) {
		QMessageBox::warning(nullptr, "错误", "无法加载符号库：" + libraryPath);
		return false;
	}

	// 添加符号到库
	if (!library.addSymbol(symbol)) {
		QMessageBox::warning(nullptr, "错误", "符号已存在，无法添加。");
		return false;
	}

	// 保存更新后的符号库回 XML 文件
	if (!library.exportToXML(libraryPath)) {
		QMessageBox::warning(nullptr, "错误", "无法保存符号库：" + libraryPath);
		return false;
	}

	qDebug() << "符号已成功添加到符号库：" << libraryPath;
	return true;
}