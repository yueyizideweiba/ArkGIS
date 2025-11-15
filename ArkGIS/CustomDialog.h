/************************************************************
FileName: customdialog.h
Author: XYH
Version : 1.0
Date: 2024-10-25
Description: 自定义对话框类，继承自QDialog，用于显示和编辑矢量图层的属性表。
Function List:
1. CustomDialog - 构造函数，初始化对话框并设置图层树视图和矢量图层
2. ~CustomDialog - 析构函数
3. closeEvent - 重写关闭事件，处理对话框关闭逻辑
4. saveChanges - 保存对属性表的更改
5. enableEditing - 启用或禁用编辑模式
6. handleItemDoubleClicked - 处理表格项双击事件
7. handleItemChanged - 处理表格项更改事件
8. handleCustomContextMenu - 处理自定义上下文菜单事件
9. addNewColumn - 添加新列到属性表
10. initializeUI - 初始化用户界面
11. populateTable - 填充表格数据
12. validateDataType - 验证数据类型是否与新值匹配
*************************************************************/

#ifndef CUSTOMDIALOG_H
#define CUSTOMDIALOG_H

#include <QDialog>
#include <QgsVectorLayer.h>
#include <QgsLayerTreeView.h>
#include <QTableWidget.h>
#include <QToolButton.h>
#include <QPushButton>

class CustomDialog : public QDialog {
	Q_OBJECT

public:
	explicit CustomDialog(QgsLayerTreeView* layerTreeView, QgsVectorLayer* vectorLayer, QWidget* parent = nullptr);
	~CustomDialog();

protected:
	void closeEvent(QCloseEvent* event) override;

private slots:
	void saveChanges();
	void enableEditing();
	void handleItemDoubleClicked(QTableWidgetItem* item);
	void handleItemChanged(QTableWidgetItem* item);
	void handleCustomContextMenu(const QPoint& pos);
	void addNewColumn();

private:
	QgsLayerTreeView* m_layerTreeView;
	QgsVectorLayer* m_vectorLayer;
	QTableWidget* m_tableWidget;
	QToolButton* m_editButton;
	QToolButton* m_addColumnButton;
	QToolButton* m_saveButton;
	bool m_hasUnsavedChanges;

	void initializeUI();
	void populateTable();
	bool validateDataType(QVariant::Type type, const QString& newValue);
	void populateFieldTable(QTableWidget* fieldTableWidget);
	void saveFieldChanges(QTableWidget* fieldTableWidget);
};

#endif