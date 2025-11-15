/************************************************************
FileName: projectiontransformdialog.h
Author: XYH
Version : 1.0
Date: 2024-10-26
Description: 投影转换对话框类，继承自QDialog，用于选择和转换地图图层的坐标参考系统。
Function List:
1. ProjectionTransformDialog - 构造函数，初始化投影转换对话框
2. selectedLayer - 获取当前选择的图层
3. selectedFile - 获取选择的文件路径
4. targetCrs - 获取目标坐标参考系统
5. selectFile - 选择文件并设置图层
6. onLayerSourceChanged - 处理图层源选择变化事件
*************************************************************/

#ifndef PROJECTIONTRANSFORMDIALOG_H
#define PROJECTIONTRANSFORMDIALOG_H

#include <QDialog>
#include <QgsCoordinateReferenceSystem.h>
#include <QgsMapLayer.h>
#include <QRadioButton>
#include "LayerManager.h"
#include "RectangleSelectTool.h"
#include "SingleClickSelectTool.h"

class QComboBox;
class QLabel;
class QPushButton;
class QLineEdit;
class QgsProjectionSelectionWidget;

class ProjectionTransformDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ProjectionTransformDialog(QgsMapCanvas* mapCanvas, QWidget* parent = nullptr);
	~ProjectionTransformDialog();

	QComboBox* mLayerSourceComboBox;

	QgsMapLayer* selectedLayer() const;
	QString selectedFile() const;
	QgsCoordinateReferenceSystem targetCrs() const;
	bool selectFile();

private slots:
	void onLayerSourceChanged(int index);      // 切换图层来源
	void activateSingleSelectTool();           // 激活点击选择工具
	void activateBoxSelectTool();              // 激活框选工具
	void enableSelectionMode();                // 启用部分要素选择模式
	void handleSelectionComplete();            // 处理选择完成后的操作
	void cancelSelectionMode();
	void onFeaturesSelected(const QList<QgsFeature>& features);

protected:
	void closeEvent(QCloseEvent* event) override;

private:
	QComboBox* mLayerComboBox;
	QLineEdit* mFileLineEdit;
	QPushButton* mSelectFileButton;
	QgsProjectionSelectionWidget* mCrsSelector;
	QRadioButton* mSingleClickButton;          // 点击选择单选按钮
	QRadioButton* mBoxSelectButton;            // 框选单选按钮
	QPushButton* mSelectFeaturesButton;        // 选择部分要素按钮
	QPushButton* mConfirmButton;               // 用于保存文件路径
	QPushButton* selectcancelButton;
	bool mIsSelectingFeatures;                 // 标记是否处于选择模式

	QLabel* fileLabel;
	QPushButton* okButton;
	QPushButton* cancelButton;
	QLabel* layerSourceLabel;
	QLabel* selectModeLabel;

	QList<QgsMapLayer*> mLayers;
	QgsMapCanvas* mpMapCanvas;

	QList<QgsFeature> mSelectedFeatures; // 用于存储选中的要素
};

#endif
