/************************************************************
FileName: mainwindow.h
Author: XYH
Version : 1.0
Date: 2024-10-24
Description: 主窗口类，负责管理整个应用程序的主要界面元素和功能。
Function List:
1. openProject - 打开工程文件
2. saveProject - 保存工程文件
3. about - 显示关于信息弹窗
4. createMenuBar - 创建主菜单栏
5. createToolBar - 创建工具条
6. createDockWidgets - 创建所有边栏
7. createMapCanvas - 创建地图画布即主绘图区，用于显示地图和进行交互
8. createStatusBar - 创建状态栏
9. updateScale - 更新地图比例尺
10. updateRotation - 更新地图旋转角度
11. setEditState - 设置编辑状态，启用或禁用编辑模式
12. onEditButtonClicked - 处理编辑按钮点击
13. onEditSaveClicked - 处理保存编辑按钮点击
14. onEditCancelClicked - 处理取消编辑按钮点击
15. onCreatePointClicked - 处理创建点按钮点击
16. onCreateLineClicked - 处理创建线按钮点击
17. onCreatePolygonClicked - 处理创建面按钮点击
18. updateVectorLayersComboBox - 更新选定要编辑的矢量图层框
*************************************************************/

#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QLabel> 
#include <QMenu> 
#include <QMenuBar> 
#include <QToolBar> 
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QProgressBar>
#include <QProgressDialog>
#include <QToolButton>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QApplication>
#include <QVector>
#include <QtMath>
#include <QTimer>
#include <QTableWidget>
#include <random>
#include <QGroupBox>
#include <QSlider>

// QGIS头文件
#include <qgsmapcanvas.h>
#include <qgsmaplayer.h>
#include <qgslayertree.h>
#include <qgslayertreeview.h>
#include <qgslayertreemodel.h>
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h>
#include <qgslayertreenode.h>
#include <qgslayertreegroup.h>
#include <qgsproject.h>
#include <qgsmaptoolpan.h>
#include <qgspointxy.h>
#include <qgslayertreeregistrybridge.h>
#include <qgslayertreelayer.h>
#include <qgsfield.h>
#include <qgsvectorfilewriter.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>
#include <qgssymbol.h>
#include <qgscategorizedsymbolrenderer.h>
#include <qgswkbtypes.h>
#include <qgsspatialindex.h>
#include <qgsrasterdataprovider.h>
#include <qgsrasterbandstats.h>
#include <qgslayertreemodellegendnode.h>
#include <qprogressdialog.h>
#include "LayerManager.h"
#include "VectorAnalysis.h"
#include "RasterAnalysis.h"
#include "SymbolManager.h"
#include "ProjectionTransformDialog.h"
#include "EditView.h"
#include "EditCommandManager.h"
#include "bufferdialog.h"
#include "VectorToRasterConverter.h"
#include "ogrsf_frmts.h"
#include"PassivationLineTool.h"
#include"EditLine.h"

class LayerManager;
class VectorToRasterConverter;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();

	// 编辑模式接口声明
	void setEditState(bool enabled);
	void onEditButtonClicked();
	void onEditSaveClicked();
	void onEditCancelClicked();
	void onCreatePointClicked();
	void onCreateLineClicked();
	void onCreatePolygonClicked();
	void updateVectorLayersComboBox();

	void performProjectionTransformation();

	QgsCoordinateReferenceSystem dataFrameCrs() const;  // 获取数据框坐标系
	void setDataFrameCrs(const QgsCoordinateReferenceSystem& crs);  // 设置数据框坐标系

	//--------------------------------------------反向线、相交线剪断-----------------------------------------------------------
	void editLineInverseLine();
	void editLineIntersectingLines();
	//--------------------------------------------反向线、相交线剪断-----------------------------------------------------------

signals:
	void editStateChanged(bool enabled);
	void dataFrameCrsChanged(const QgsCoordinateReferenceSystem& crs);  // 数据框坐标系变更信号

private slots:
	// 打开、保存工程文件
	void openProject();
	void saveProject();
	// 关于信息窗口
	void about();
	void openStyleManager(); // QGIS的API
	void openSymbolManager(); // 自己做的
	void updateCrsInfo();
	void updateCoordinates(const QgsPointXY& mapPoint);

	void openDataFrameCrsDialog();  // 打开数据框坐标系设置对话框

	void onSelectModeToggle();

	void onDeleteClicked();

	void undoEdit();
	void redoEdit();

	void onMoveClicked();
	void onCopyClicked();

	void on_pushButtonBuffer_clicked();//缓冲区
	void onVectorToRasterButtonClicked();//矢量转栅格

	void onRotateClicked();
	void onMergeClicked();
	void onInsertPointClicked();

	void editLinePassivationLine();
	void editLineSimplification();

private:
	// 创建主要部件
	void createMenuBar();
	void createToolBar();
	void createDockWidgets();
	void createMapCanvas();
	void createStatusBar();

	// 更新状态栏显示
	void updateScale();
	void updateRotation();
	void zoomToLayer();
	void resetMapTool();

	void createRotateTool();
	void rotate(double x, double y, double angle);

	QToolBar* mpEditToolBar;
	QgsMapCanvas* mpMapCanvas; // 地图画布
	QTreeWidget* mpFileTreeWidget; // 文件树
	QgsLayerTreeView* mpLayerTreeView; // 图层树
	QgsLayerTreeModel* mpLayerTreeModel; // 图层树模式
	// 状态栏坐标、比例、旋转角度
	QLabel* mpCoordinatesLabel;
	QLabel* mpCrsLabel;
	QLabel* mpActualCrsLabel;    // 实际图层投影坐标系
	QComboBox* mpScaleComboBox;
	QDoubleSpinBox* mpRotationSpinBox;
	// 图层类、矢量分析类、栅格分析类对象
	LayerManager* mLayerManager;
	VectorAnalysis* mVectorAnalysis;
	RasterAnalysis* mRasterAnalysis;
	// 关于信息弹窗
	QMessageBox* mpAboutMessageBox;
	// 样式管理器对象
	SymbolManager* mSymbolManager;

	QComboBox* mpEditLayerComboBox; // 编辑图层下拉列表
	QAction* mpEditAction;      // 编辑模式按钮
	QAction* mpCopyAction;
	QAction* mpSaveEditAction;  // 保存编辑按钮
	QAction* mpCancelEditAction;// 取消编辑按钮
	QAction* mpSelectAction;// 开启选择按钮
	EditView* mEditView;      // 编辑管理器实例
	QAction* mpCreatePointAction;
	QAction* mpCreateLineAction;
	QAction* mpCreatePolygonAction;
	QAction* mpDeleteAction;
	QAction* mpMoveAction;
	QAction* mpMergePolygonAction;
	QAction* mpInsertPointAction;
	QProgressBar* mpProgressBar;
	QLabel* mpStatusLabel;

	QgsCoordinateReferenceSystem mDataFrameCrs;  // 数据框坐标系
	bool mDataFrameCrsSet = false;  // 标志是否已设置数据框坐标系

	QAction* mpUndoAction;
	QAction* mpRedoAction;
	EditCommandManager* mEditCommandManager;
	BufferDialog* mBufferDialog;
	VectorToRasterConverter* mvectorToRasterConverter;

	EditView* chooseAndMoveEditView;

	QGroupBox* mpRotateGroupBox; // 旋转工具组
	QLineEdit* mpRotationCenterX; // 旋转中心X坐标
	QLineEdit* mpRotationCenterY; // 旋转中心Y坐标
	double mRotationAngle = 0; // 已经旋转的角度
	QAction* mpRotatePolygonAction;



	PassivationLineTool* mpPassivationLineTool;
	EditLine* mEditLine;                    // 抽稀逻辑处理类
};