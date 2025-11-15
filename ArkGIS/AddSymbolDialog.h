/************************************************************
FileName: AddSymbolDialog.h
Author: XYH
Version : 1.0
Date: 2024-10-25
Description: 添加符号对话框类，继承自QDialog，用于创建和管理地图符号。
Function List:
1. AddSymbolDialog - 构造函数，初始化添加符号对话框
2. symbolName - 获取符号名称
3. getSymbol - 获取符号对象
4. addLayer - 添加图层到符号管理
5. removeLayer - 删除选中的图层
6. updatePreview - 更新符号的预览图
7. onLayerSelected - 处理图层选择事件
8. loadAvailableLibraries - 加载可用的符号库
9. onOkButtonClicked - 处理确定按钮点击事件
10. setupUI - 初始化用户界面布局
*************************************************************/

#ifndef ADDSYMBOLDIALOG_H
#define ADDSYMBOLDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QGraphicsView>
#include <QLabel>
#include <QComboBox>
#include "CustomGraphicsScene.h"
#include "Symbol.h"

class AddSymbolDialog : public QDialog {
	Q_OBJECT

public:
	AddSymbolDialog(QWidget* parent = nullptr);
	QString symbolName() const;
	Symbol getSymbol() const;

signals:
	void symbolAdded();

private slots:
	void addLayer();              // 添加图层
	void removeLayer();           // 删除选中的图层
	void updatePreview();         // 更新符号的预览图
	void onLayerSelected(QTreeWidgetItem* item, int column);  // 图层选择事件
	void loadAvailableLibraries();
	void onOkButtonClicked();

private:
	void setupUI();               // 初始化UI布局

	QLineEdit* nameEdit;          // 符号名称输入框
	QComboBox* libraryComboBox;   // 符号库选择下拉框
	QTreeWidget* layerTree;       // 图层管理树状图
	CustomGraphicsScene* scene;   // 自定义绘图区域
	QLabel* layerPreviewLabel;   // 单图层预览窗口
	QLabel* symbolPreviewLabel;         // 总预览图
	Symbol symbol;                // 当前符号
};

#endif