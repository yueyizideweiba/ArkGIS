/************************************************************
FileName: SymbolLibraryManager.h
Author: XYH
Version : 1.0
Date: 2024-10-26
Description: 符号库管理器类定义文件，包含SymbolLibraryManager类，用于管理地图符号库。
Function List:
1. instance - 获取单例实例
2. initialize - 初始化符号库管理器
3. setLibraryDirectory - 设置符号库目录
4. getAvailableLibraries - 获取可用的符号库文件路径列表
5. getCurrentLibrary - 获取当前符号库的引用
6. loadLibrary - 加载指定路径的符号库文件
7. getCurrentLibraryPath - 获取当前符号库的路径
8. addSymbolToCurrentLibrary - 向当前符号库添加符号并自动保存
9. addSymbolToLibrary - 向指定的符号库添加符号
10. importLibrary - 导入符号库并添加到路径列表
11. createDefaultLibrary - 创建默认符号库

Attributes:
1. libraryDirectory - 符号库目录路径
2. currentLibraryPath - 当前使用的符号库文件路径
3. currentLibrary - 当前符号库对象
4. importedLibraries - 已导入的符号库路径列表

*************************************************************/

#ifndef SYMBOLLIBRARYMANAGER_H
#define SYMBOLLIBRARYMANAGER_H

#include <QString>
#include <QList>
#include "SymbolLibrary.h"

class SymbolLibraryManager {
public:
	static SymbolLibraryManager& instance();  // 获取单例实例

	void initialize();  // 初始化符号库
	void setLibraryDirectory(const QString& dirPath);  // 设置符号库目录
	QList<QString> getAvailableLibraries() const;  // 获取符号库文件路径
	SymbolLibrary& getCurrentLibrary();  // 获取当前符号库
	bool loadLibrary(const QString& filePath);  // 加载符号库文件
	QString getCurrentLibraryPath() const;  // 获取当前符号库路径
	bool addSymbolToCurrentLibrary(const Symbol& symbol);  // 添加符号并自动保存
	bool addSymbolToLibrary(const QString& libraryPath, const Symbol& symbol);  // 添加符号到指定库

	// 导入符号库并添加到路径列表
	void importLibrary(const QString& path);

private:
	SymbolLibraryManager();  // 私有构造函数

	QString libraryDirectory = "./symbols";  // 默认符号库目录
	QString currentLibraryPath;  // 当前使用的符号库路径
	SymbolLibrary currentLibrary;  // 当前符号库

	QList<QString> importedLibraries;        // 已导入的符号库路径列表

	void createDefaultLibrary();  // 创建默认符号库
};

#endif
