#include "SymbolLibrary.h"
#include "SymbolLibraryManager.h"
#include <QFile>
#include <QDomDocument>
#include <QMessageBox>
#include <QTextStream>
#include <iostream>
#include <QDebug>

// 添加符号
bool SymbolLibrary::addSymbol(const Symbol& symbol) {
	symbols.append(symbol);
	return true;
}

// 获取符号列表
QList<Symbol> SymbolLibrary::getSymbols() const {
	return symbols;
}

// 从 XML 文件加载符号库
bool SymbolLibrary::importFromXML(const QString& filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(nullptr, "错误", "无法打开符号库文件");
		return false;
	}

	QDomDocument doc;
	if (!doc.setContent(&file)) {
		QMessageBox::warning(nullptr, "错误", "符号库 XML 格式错误");
		file.close();
		return false;
	}
	file.close();

	QDomNodeList symbolNodes = doc.elementsByTagName("symbol");
	for (int i = 0; i < symbolNodes.size(); ++i) {
		Symbol symbol = Symbol::importFromXML(symbolNodes.at(i).toElement());
		addSymbol(symbol);
	}
	return true;
}

// 导出符号库为 XML
bool SymbolLibrary::exportToXML(const QString& filePath) const {
	QDomDocument doc;
	QDomElement root = doc.createElement("qgis_style");
	root.setAttribute("version", "2");
	doc.appendChild(root);

	for (const Symbol& symbol : symbols) {
		symbol.exportToXML(doc, root);
	}

	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		QMessageBox::warning(nullptr, "错误", "无法保存符号库文件");
		return false;
	}

	QTextStream stream(&file);
	stream << doc.toString();
	file.close();
	return true;
}

// 根据名称获取符号
Symbol SymbolLibrary::getSymbolByName(const QString& name) const {
	for (const Symbol& symbol : symbols) {
		if (symbol.name == name) {
			return symbol;
		}
	}
	return Symbol();  // 返回一个空符号（如果找不到）
}

// 删除符号并更新符号库文件
bool SymbolLibrary::removeSymbol(const Symbol& symbol) {
	for (int i = 0; i < symbols.size(); ++i) {
		if (symbols[i].name == symbol.name) {
			symbols.removeAt(i);  // 从符号列表中删除符号

			// 更新符号库文件
			QString filePath = SymbolLibraryManager::instance().getCurrentLibraryPath();
			if (exportToXML(filePath)) {
				return true;  // 删除成功且符号库更新成功
			}
			else {
				qWarning() << "无法更新符号库文件：" << filePath;
				return false;  // 符号库更新失败
			}
		}
	}
	return false;  // 符号未找到，删除失败
}

