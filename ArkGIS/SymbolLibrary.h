/************************************************************
FileName: SymbolLibrary.h
Author: XYH
Version : 1.0
Date: 2024-10-27
Description: 符号库类定义文件，包含SymbolLibrary类，用于管理和存储地图符号。
Function List:
1. SymbolLibrary - 构造函数，初始化符号库
2. addSymbol - 添加符号到库中
3. getSymbols - 获取库中所有符号的列表
4. importFromXML - 从XML文件导入符号库
5. exportToXML - 将符号库导出为XML文件
6. getSymbolByName - 根据名称获取特定的符号
7. removeSymbol - 从库中删除指定的符号

Attributes:
1. symbols - 私有成员，存储符号库中的符号列表
*************************************************************/

#ifndef SYMBOLLIBRARY_H
#define SYMBOLLIBRARY_H

#include <QList>
#include <QString>
#include "Symbol.h"

class SymbolLibrary {
public:
	SymbolLibrary() = default;  // 将构造函数设为 public

	bool addSymbol(const Symbol& symbol);               // 添加符号
	QList<Symbol> getSymbols() const;                   // 获取符号列表
	bool importFromXML(const QString& filePath);        // 从 XML 文件加载符号
	bool exportToXML(const QString& filePath) const;    // 将符号库导出为 XML

	Symbol getSymbolByName(const QString& name) const;  // 根据名称获取符号
	bool removeSymbol(const Symbol& symbol);  // 删除符号

private:
	QList<Symbol> symbols;      // 存储符号的列表
};

#endif
