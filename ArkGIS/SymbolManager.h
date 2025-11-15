/************************************************************
FileName: SymbolManager.h
Author: XYH
Version : 1.0
Date: 2024-10-26
Description: 符号管理器类定义文件，包含SymbolManager类，用于管理和操作地图符号。
Function List:
1. SymbolManager - 构造函数，初始化符号管理器界面
2. createNewSymbolDatabase - 新建符号数据库
3. switchSymbolLibrary - 切换符号库
4. updateSymbolList - 更新符号列表显示
5. onCustomContextMenuRequested - 处理右键菜单事件
6. deleteSelectedSymbol - 删除选中的符号
7. exportSymbolAsPNG - 导出符号为PNG格式
8. exportSymbolAsSVG - 导出符号为SVG格式
9. importSymbols - 导入符号
10. exportSymbols - 导出符号
11. addSymbol - 添加新符号
12. onLibraryChanged - 响应符号库切换事件
13. loadAvailableLibraries - 加载可用的符号库
14. loadDefaultLibrary - 加载默认符号库
15. setupUI - 设置用户界面
16. getSelectedSymbol - 获取当前选中的符号

Attributes:
1. symbolView - 符号列表视图
2. symbolModel - 符号列表模型
3. symbols - 存储符号的列表
4. symbolLibrary - 当前使用的符号库对象
5. libraryComboBox - 显示可用符号库的下拉框

*************************************************************/

#ifndef SYMBOLMANAGER_H
#define SYMBOLMANAGER_H

#include <QDialog>
#include <QListView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QComboBox>
#include <QgsStyle.h>
#include "Symbol.h"
#include "SymbolLibrary.h"

class SymbolManager : public QDialog {
	Q_OBJECT

public:
	SymbolManager(QWidget* parent = nullptr);
	explicit SymbolManager(QObject* parent = nullptr);
	void createNewSymbolDatabase();  // 新建符号数据库
	void switchSymbolLibrary();  // 切换符号库
	void updateSymbolList();

	void onCustomContextMenuRequested(const QPoint& pos);  // 右键菜单事件
	void deleteSelectedSymbol();  // 删除符号
	void exportSymbolAsPNG();  // 导出符号为 PNG
	void exportSymbolAsSVG();  // 导出符号为 SVG

private slots:
	void importSymbols();
	void exportSymbols();
	void addSymbol();
	void onLibraryChanged(const QString& libraryPath);  // 响应符号库切换

private:
	QListView* symbolView;
	QStandardItemModel* symbolModel;
	QList<Symbol> symbols;  // 存储符号列表
	SymbolLibrary* symbolLibrary;
	QComboBox* libraryComboBox;  // 下拉框显示符号库列表
	void loadAvailableLibraries();  // 加载可用符号库
	void loadDefaultLibrary();      // 加载默认符号库
	void setupUI();

	Symbol getSelectedSymbol();  // 获取选中的符号

};

#endif
