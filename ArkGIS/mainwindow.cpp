#include "MainWindow.h"
#include <iostream>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <qgsproject.h>
#include <QgsMapLayer.h>
#include <QVector>
#include <QtMath>
#include <random>
#include <QVariant>
#include <QgsVectorLayerExporter.h>
#include <QgsRasterPipe.h>
#include <QgsRasterFileWriter.h>
#include <QgsProjectionSelectionWidget.h>
#include <gdal.h>
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "ogr_spatialref.h"
#include "ogrsf_frmts.h"
#include <qgsstylemanagerdialog.h>  // 样式管理器的头文件

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent),
	mpMapCanvas(new QgsMapCanvas(this)),
	mpFileTreeWidget(new QTreeWidget(this)),
	mpLayerTreeView(new QgsLayerTreeView(this)),
	mLayerManager(new LayerManager(mpMapCanvas, mpFileTreeWidget, mpLayerTreeView, this)),
	mVectorAnalysis(new VectorAnalysis(mpMapCanvas, this)),
	mRasterAnalysis(new RasterAnalysis(mpMapCanvas, this)),
	mEditView(new EditView(mpMapCanvas, this)),
	mDataFrameCrsSet(false),
	mBufferDialog(new BufferDialog(mpMapCanvas, this)),
	mvectorToRasterConverter(new VectorToRasterConverter(mpMapCanvas, this)),
	mpPassivationLineTool(nullptr)
{
	setMinimumSize(QSize(1800, 1100));
	setWindowTitle("ArkGIS");

	// 创建窗口各组件
	createMenuBar();
	createToolBar();
	createDockWidgets();
	createMapCanvas();
	createStatusBar();
	createRotateTool();

	// 导入样式表
	QFile file("./theme.qss");
	if (file.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream stream(&file);
		qApp->setStyleSheet(stream.readAll());
		file.close();
	}

	updateVectorLayersComboBox();
}

MainWindow::~MainWindow()
{

}

// 创建菜单栏
void MainWindow::createMenuBar()
{
	QMenuBar* mpMenuBar = new QMenuBar(this);
	QMenu* mpProjectMenu = mpMenuBar->addMenu(tr("工程(&J)"));
	mpProjectMenu->addAction(tr("打开工程文件"), this, &MainWindow::openProject);
	mpProjectMenu->addAction(tr("保存工程文件"), this, &MainWindow::saveProject);

	QMenu* mpEditMenu = mpMenuBar->addMenu(tr("编辑(&E)"));
	mpEditMenu->addAction(tr("钝化线"), this, &MainWindow::editLinePassivationLine);
	mpEditMenu->addAction(tr("反向线"), this, &MainWindow::editLineInverseLine);
	mpEditMenu->addAction(tr("相交线剪断"), this, &MainWindow::editLineIntersectingLines);
	mpEditMenu->addAction(tr("抽稀线"), this, &MainWindow::editLineSimplification);

	QMenu* mpViewMenu = mpMenuBar->addMenu(tr("视图(&V)"));

	QMenu* mpLayerMenu = mpMenuBar->addMenu(tr("图层(&L)"));
	mpLayerMenu->addAction(tr("按类别符号化图层-唯一值"), mLayerManager, &LayerManager::performCategorizedSymbology);

	QMenu* mpAddlayerMenu = mpLayerMenu->addMenu(tr("添加图层"));
	mpAddlayerMenu->addAction(tr("矢量图层"), mLayerManager, &LayerManager::addVectorLayer);
	mpAddlayerMenu->addAction(tr("栅格图层"), mLayerManager, &LayerManager::addRasterLayer);
	mpAddlayerMenu->addAction(tr("分隔符文本图层"), mLayerManager, &LayerManager::addDelimitedTextLayer);

	QMenu* mpSettingsMenu = mpMenuBar->addMenu(tr("设置(&S)"));
	mpSettingsMenu->addAction(tr("数据框坐标系"), this, &MainWindow::openDataFrameCrsDialog);
	mpSettingsMenu->addAction(tr("QGIS样式管理器"), this, &MainWindow::openStyleManager);
	mpSettingsMenu->addAction(tr("ArkGIS样式管理器"), this, &MainWindow::openSymbolManager);

	QMenu* mpVectorMenu = mpMenuBar->addMenu(tr("矢量(&O)"));
	mpVectorMenu->addAction(tr("K均值聚类"), mVectorAnalysis, &VectorAnalysis::performKMeansClustering);
	mpVectorMenu->addAction(tr("按位置连接属性"), mVectorAnalysis, &VectorAnalysis::performSpatialJoin);
	mpVectorMenu->addAction(tr("表转矢量"), mVectorAnalysis, &VectorAnalysis::performExcelToShp);
	mpVectorMenu->addAction(tr("矢量转栅格"), this, &MainWindow::onVectorToRasterButtonClicked);
	mpVectorMenu->addAction(tr("缓冲区分析"), this, &MainWindow::on_pushButtonBuffer_clicked);

	//-----------------------------------------裁剪-------------------------------------------------------------------------------------
	mpVectorMenu->addAction(tr("矢量图层裁剪"), mVectorAnalysis, &VectorAnalysis::performVectorClipping);
	mpVectorMenu->addAction(tr("自定义矢量图层裁剪"), mVectorAnalysis, &VectorAnalysis::performCustomVectorClipping);

	//-----------------------------------------裁剪-------------------------------------------------------------------------------------

	QMenu* mpRasterMenu = mpMenuBar->addMenu(tr("栅格(&R)"));
	mpRasterMenu->addAction(tr("栅格图层统计"), mRasterAnalysis, &RasterAnalysis::performRasterLayerStatistics);
	mpRasterMenu->addAction(tr("栅格计算器"), mRasterAnalysis, &RasterAnalysis::rasterCalculator);

	QMenu* mpDatabaseMenu = mpMenuBar->addMenu(tr("数据库(&D)"));
	QMenu* mpWebMenu = mpMenuBar->addMenu(tr("Web(&W)"));
	QMenu* mpMeshMenu = mpMenuBar->addMenu(tr("网孔(&M)"));

	QMenu* mpProcessMenu = mpMenuBar->addMenu(tr("数据处理(&C)"));
	mpProcessMenu->addAction(tr("投影变换"), this, &MainWindow::performProjectionTransformation);

	QMenu* mpHelpMenu = mpMenuBar->addMenu(tr("帮助(&H)"));
	setMenuBar(mpMenuBar);
}

// 创建工具栏
void MainWindow::createToolBar()
{
	// 主工具栏
	QToolBar* mpMainToolBar = addToolBar(tr("Main Toolbar"));
	mpMainToolBar->setIconSize(QSize(50, 50));
	mpMainToolBar->addAction(QIcon("./icons/1.png"), tr("打开工程"), this, &MainWindow::openProject);
	mpMainToolBar->addAction(QIcon("./icons/2.png"), tr("保存工程"), this, &MainWindow::saveProject);
	mpMainToolBar->addAction(QIcon("./icons/3.png"), tr("退出"), this, &MainWindow::close);

	mpEditToolBar = addToolBar(tr("Edit Toolbar"));
	mpEditLayerComboBox = new QComboBox(this);
	mpEditLayerComboBox->setObjectName("选择编辑图层");
	mpEditLayerComboBox->setEditable(false);
	connect(mLayerManager, &LayerManager::layersChanged, this, &MainWindow::updateVectorLayersComboBox);

	// 初始化编辑模式按钮
	mpEditAction = new QAction(QIcon("./icons/edit.png"), tr("编辑模式"), this);
	mpEditAction->setCheckable(true);
	mpEditAction->setChecked(false);
	connect(mpEditAction, &QAction::triggered, this, &MainWindow::onEditButtonClicked);

	mpSelectAction = new QAction(QIcon("./icons/4.png"), tr("选择与修改属性"), this);
	mpSelectAction->setEnabled(false);
	connect(mpSelectAction, &QAction::triggered, this, &MainWindow::onSelectModeToggle);

	// 初始化创建点按钮
	mpCreatePointAction = new QAction(QIcon("./icons/9.png"), tr("创建点"), this);
	mpCreatePointAction->setEnabled(false);
	connect(mpCreatePointAction, &QAction::triggered, this, &MainWindow::onCreatePointClicked);

	// 初始化创建线按钮
	mpCreateLineAction = new QAction(QIcon("./icons/10.png"), tr("创建线"), this);
	mpCreateLineAction->setEnabled(false);
	connect(mpCreateLineAction, &QAction::triggered, this, &MainWindow::onCreateLineClicked);

	// 初始化创建面按钮
	mpCreatePolygonAction = new QAction(QIcon("./icons/11.png"), tr("创建面"), this);
	mpCreatePolygonAction->setEnabled(false);
	connect(mpCreatePolygonAction, &QAction::triggered, this, &MainWindow::onCreatePolygonClicked);

	// 初始化删除要素按钮
	mpDeleteAction = new QAction(QIcon("./icons/delete.png"), tr("删除"), this);
	mpDeleteAction->setEnabled(false);
	connect(mpDeleteAction, &QAction::triggered, this, &MainWindow::onDeleteClicked);

	// 初始化保存编辑按钮
	mpSaveEditAction = new QAction(QIcon("./icons/saveeditlayer.png"), tr("保存编辑"), this);
	mpSaveEditAction->setEnabled(false);
	connect(mpSaveEditAction, &QAction::triggered, this, &MainWindow::onEditSaveClicked);

	// 初始化取消编辑按钮
	mpCancelEditAction = new QAction(QIcon("./icons/canceledit.png"), tr("取消编辑"), this);
	mpCancelEditAction->setEnabled(false);
	connect(mpCancelEditAction, &QAction::triggered, this, &MainWindow::onEditCancelClicked);

	mpUndoAction = new QAction(QIcon("./icons/undo.png"), tr("撤销"), this);
	mpUndoAction->setEnabled(false);
	connect(mpUndoAction, &QAction::triggered, this, &MainWindow::undoEdit);

	mpRedoAction = new QAction(QIcon("./icons/redo.png"), tr("重做"), this);
	mpRedoAction->setEnabled(false);
	connect(mpRedoAction, &QAction::triggered, this, &MainWindow::redoEdit);

	// 初始化移动按钮
	mpMoveAction = new QAction(QIcon("./icons/5.png"), tr("移动"), this);
	mpMoveAction->setEnabled(false);
	connect(mpMoveAction, &QAction::triggered, this, &MainWindow::onMoveClicked);

	// 初始化复制按钮
	mpCopyAction = new QAction(QIcon("./icons/copy.png"), tr("复制"), this);
	mpCopyAction->setEnabled(false);
	connect(mpCopyAction, &QAction::triggered, this, &MainWindow::onCopyClicked);

	// 初始化旋转按钮
	mpRotatePolygonAction = new QAction(QIcon("./icons/rotate.png"), tr("旋转"), this);
	mpRotatePolygonAction->setEnabled(false);
	connect(mpRotatePolygonAction, &QAction::triggered, this, &MainWindow::onRotateClicked);

	// 初始化合并面按钮
	mpMergePolygonAction = new QAction(QIcon("./icons/merge.png"), tr("合并面"), this);
	mpMergePolygonAction->setEnabled(false);
	connect(mpMergePolygonAction, &QAction::triggered, this, &MainWindow::onMergeClicked);

	// 初始化插入点按钮
	mpInsertPointAction = new QAction(QIcon("./icons/insertpoint.png"), tr("插入点"), this);
	mpInsertPointAction->setEnabled(false);
	connect(mpInsertPointAction, &QAction::triggered, this, &MainWindow::onInsertPointClicked);


	mpEditToolBar->addAction(mpUndoAction);
	mpEditToolBar->addAction(mpRedoAction);

	// 将编辑相关的控件添加到工具栏
	mpEditToolBar->addWidget(mpEditLayerComboBox);
	mpEditToolBar->addAction(mpEditAction);
	mpEditToolBar->addAction(mpSelectAction);
	mpEditToolBar->addAction(mpMoveAction);
	mpEditToolBar->addAction(mpCopyAction);
	mpEditToolBar->addAction(mpRotatePolygonAction);
	mpEditToolBar->addAction(mpMergePolygonAction);
	mpEditToolBar->addAction(mpCreatePointAction);
	mpEditToolBar->addAction(mpCreateLineAction);
	mpEditToolBar->addAction(mpCreatePolygonAction);
	mpEditToolBar->addAction(mpInsertPointAction);
	mpEditToolBar->addAction(mpDeleteAction);
	mpEditToolBar->addAction(mpSaveEditAction);
	mpEditToolBar->addAction(mpCancelEditAction);

	// 编辑工具栏
	QToolBar* mpToolBar = addToolBar(tr("MoveToolbar"));

	// mpToolBar->addAction(QIcon("./icons/8.png"), tr("复位"), this, &MainWindow::zoomToLayer);
	mpToolBar->addAction(QIcon("./icons/proj.png"), tr("投影变换"), this, &MainWindow::performProjectionTransformation);
	mpToolBar->addAction(QIcon("./icons/14.png"), tr("关于"), this, &MainWindow::about);


}

// 点击与更改属性
void MainWindow::onSelectModeToggle() {
	QString currentLayerName = mpEditLayerComboBox->currentText();
	QgsProject* project = QgsProject::instance();
	QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);
	QgsVectorLayer* currentLayer = static_cast<QgsVectorLayer*>(layers.first());

	chooseAndMoveEditView = new EditView(mpMapCanvas);
	chooseAndMoveEditView->setLayer(currentLayer, EditView::SelectAndAttriMode); // 使用复合模式
	mpMapCanvas->setMapTool(chooseAndMoveEditView);
}

void MainWindow::zoomToLayer()
{
	// 获取当前选中的图层
	QgsMapLayer* layer = mpLayerTreeView->currentLayer();
	if (!layer)
		return;

	// 获取图层范围并更新地图画布
	mpMapCanvas->setExtent(layer->extent());
	mpMapCanvas->refresh();
}

void MainWindow::setEditState(bool enabled)
{
	// 更新编辑状态
	mpDeleteAction->setEnabled(enabled);
	mpCreatePointAction->setEnabled(enabled);
	mpCreateLineAction->setEnabled(enabled);
	mpCreatePolygonAction->setEnabled(enabled);
	mpSaveEditAction->setEnabled(enabled);
	mpCancelEditAction->setEnabled(enabled);
	mpSelectAction->setEnabled(enabled);
	mpUndoAction->setEnabled(enabled);
	mpRedoAction->setEnabled(enabled);
	mpMoveAction->setEnabled(enabled);
	mpCopyAction->setEnabled(enabled);
	mpRotatePolygonAction->setEnabled(enabled);
	mpInsertPointAction->setEnabled(enabled);
	emit editStateChanged(enabled);
}

void MainWindow::updateVectorLayersComboBox()
{
	// 保存当前选中的图层名称
	QString currentLayerName = mpEditLayerComboBox->currentText();

	// 清空现有的下拉框选项
	mpEditLayerComboBox->clear();

	// 获取QGIS项目实例
	QgsProject* project = QgsProject::instance();

	// 遍历项目中的所有图层
	const auto layers = project->mapLayers().values();
	for (const QgsMapLayer* layer : layers) {
		// 检查图层是否为矢量图层
		if (layer->type() == Qgis::LayerType::Vector) {
			// 将矢量图层名称添加到下拉框中
			mpEditLayerComboBox->addItem(layer->name());
		}
	}

	// 恢复之前的选择
	int index = mpEditLayerComboBox->findText(currentLayerName);
	if (index != -1) {
		mpEditLayerComboBox->setCurrentIndex(index);
	}
	else if (mpEditLayerComboBox->count() > 0) {
		// 如果当前选中的图层不再存在，设置第一个图层为选中状态
		mpEditLayerComboBox->setCurrentIndex(0);
	}
}

void MainWindow::onEditButtonClicked()
{
	// 检查编辑模式是否被启用
	bool editEnabled = mpEditAction->isChecked();
	setEditState(editEnabled);

	if (editEnabled) {
		// 获取当前选中的图层名称
		QString currentLayerName = mpEditLayerComboBox->currentText();
		QgsProject* project = QgsProject::instance();
		QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);

		if (!layers.isEmpty() && layers.first()->type() == Qgis::LayerType::Vector) {
			QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());

			// 确保图层处于可编辑状态
			if (!vectorLayer->isEditable()) {
				vectorLayer->startEditing();
			}

			// 初始化 EditCommandManager
			if (!mEditCommandManager) {
				mEditCommandManager = new EditCommandManager(vectorLayer, this);
				connect(mEditCommandManager, &EditCommandManager::stateChanged, this, [=](bool canUndo, bool canRedo) {
					mpUndoAction->setEnabled(canUndo);
					mpRedoAction->setEnabled(canRedo);
					});
			}

			// 设置编辑工具栏的状态
			mpEditLayerComboBox->setEnabled(false);
			mpUndoAction->setEnabled(false);
			mpRedoAction->setEnabled(false);

			// 根据图层的几何类型设置工具按钮的可用性
			switch (vectorLayer->geometryType()) {
			case Qgis::GeometryType::Point:
				mpCreatePointAction->setEnabled(true);
				mpCreateLineAction->setEnabled(false);
				mpCreatePolygonAction->setEnabled(false);
				break;
			case Qgis::GeometryType::Line:
				mpCreatePointAction->setEnabled(false);
				mpCreateLineAction->setEnabled(true);
				mpCreatePolygonAction->setEnabled(false);
				break;
			case Qgis::GeometryType::Polygon:
				mpCreatePointAction->setEnabled(false);
				mpCreateLineAction->setEnabled(false);
				mpCreatePolygonAction->setEnabled(true);
				break;
			default:
				mpCreatePointAction->setEnabled(false);
				mpCreateLineAction->setEnabled(false);
				mpCreatePolygonAction->setEnabled(false);
				break;
			}

			// 启用其他编辑功能按钮
			mpSelectAction->setEnabled(true);
			mpDeleteAction->setEnabled(true);
			mpSaveEditAction->setEnabled(true);
			mpMoveAction->setEnabled(true);
			mpCancelEditAction->setEnabled(true);
			mpUndoAction->setEnabled(true);
			mpRedoAction->setEnabled(true);
			mpCopyAction->setEnabled(true);
			mpRotatePolygonAction->setEnabled(true);
			mpMergePolygonAction->setEnabled(true);

		}
		else {
			// 如果未选择有效的矢量图层
			QMessageBox::warning(this, tr("错误"), tr("未选择有效的矢量图层！"));
			mpEditAction->setChecked(false); // 禁用编辑模式
			setEditState(false);
		}
	}
	else {
		// 重置工具栏和状态
		mpEditLayerComboBox->setEnabled(true);
		mpCreatePointAction->setEnabled(false);
		mpCreateLineAction->setEnabled(false);
		mpCreatePolygonAction->setEnabled(false);
		mpMoveAction->setEnabled(false);
		mpSelectAction->setEnabled(false);
		mpDeleteAction->setEnabled(false);
		mpSaveEditAction->setEnabled(false);
		mpCancelEditAction->setEnabled(false);
		mpUndoAction->setEnabled(false);
		mpRedoAction->setEnabled(false);
		mpCopyAction->setEnabled(false);
		mpRotatePolygonAction->setEnabled(false);
	}
}


// 创建点要素的槽函数
void MainWindow::onCreatePointClicked() {
	if (mEditCommandManager) {
		mEditCommandManager->beginCommand(tr("创建点要素"));

		QString currentLayerName = mpEditLayerComboBox->currentText();
		QgsProject* project = QgsProject::instance();
		// 根据图层名称获取图层对象列表
		QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);
		QgsMapLayer* layer = nullptr;

		if (!layers.isEmpty()) {
			layer = layers.first();
		}

		if (!layers.isEmpty() && layers.first()->type() == Qgis::LayerType::Vector) {
			QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());
			if (vectorLayer->geometryType() == Qgis::GeometryType::Point) {

				// 创建并设置 EditView
				EditView* editView = new EditView(mpMapCanvas);
				editView->setLayer(vectorLayer, EditView::PointMode);  // 启用线段编辑模式
				mpMapCanvas->setMapTool(editView);


			}
		}
		else {
			QMessageBox::warning(this, tr("错误"), tr("选中的图层不是点图层！"));
		}
	}
	else {
		QMessageBox::warning(this, tr("错误"), tr("未选中有效的图层！"));
	}
	mEditCommandManager->endCommand();
}


void MainWindow::onCreateLineClicked()
{
	if (mEditCommandManager) {
		mEditCommandManager->beginCommand(tr("创建线要素"));
		QString currentLayerName = mpEditLayerComboBox->currentText();
		QgsProject* project = QgsProject::instance();
		QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);

		if (!layers.isEmpty() && layers.first()->type() == Qgis::LayerType::Vector) {
			QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());

			// 检查图层是否为线图层
			if (vectorLayer->geometryType() == Qgis::GeometryType::Line) {
				// 创建并设置 EditView
				EditView* editView = new EditView(mpMapCanvas);
				editView->setLayer(vectorLayer, EditView::LineMode);  // 启用线段编辑模式
				mpMapCanvas->setMapTool(editView);
			}
			else {
				QMessageBox::warning(this, tr("错误"), tr("选中的图层不是线图层！"));
			}
		}
		else {
			QMessageBox::warning(this, tr("错误"), tr("未选中有效的图层！"));
		}
		mEditCommandManager->endCommand();
	}
}


void MainWindow::onCreatePolygonClicked()
{
	if (mEditCommandManager) {
		mEditCommandManager->beginCommand(tr("创建面要素"));
		QString currentLayerName = mpEditLayerComboBox->currentText();
		QgsProject* project = QgsProject::instance();
		QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);

		if (!layers.isEmpty() && layers.first()->type() == Qgis::LayerType::Vector) {
			QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());

			// 检查图层是否为多边形图层
			if (vectorLayer->geometryType() == Qgis::GeometryType::Polygon) {
				// 创建并设置 EditView
				EditView* editView = new EditView(mpMapCanvas);
				editView->setLayer(vectorLayer, EditView::PolygonMode);  // 启用多边形编辑模式
				mpMapCanvas->setMapTool(editView);
			}
			else {
				QMessageBox::warning(this, tr("错误"), tr("选中的图层不是多边形图层！"));
			}
		}
		else {
			QMessageBox::warning(this, tr("错误"), tr("未选中有效的图层！"));
		}
		mEditCommandManager->endCommand();
	}
}

void MainWindow::onEditSaveClicked()
{
	QString currentLayerName = mpEditLayerComboBox->currentText();
	QgsProject* project = QgsProject::instance();
	QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);

	if (!layers.isEmpty() && layers.first()->type() == Qgis::LayerType::Vector) {
		QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());

		// 检查图层是否处于编辑模式
		if (vectorLayer->isEditable()) {
			// 提交图层的更改
			if (vectorLayer->commitChanges()) {
				QMessageBox::information(this, tr("保存成功"), tr("编辑已成功保存。"));
			}
			else {
				QMessageBox::warning(this, tr("保存失败"), tr("无法保存更改，请检查错误！"));
				vectorLayer->rollBack();  // 如果提交失败，回滚到之前的状态
			}
			// 清除所有高亮显示的要素
			vectorLayer->removeSelection();
		}
		else {
			QMessageBox::warning(this, tr("错误"), tr("图层不在编辑模式，无法保存！"));
		}
	}

	// 重置地图工具，退出编辑状态
	resetMapTool();
}

void MainWindow::onEditCancelClicked()
{
	QString currentLayerName = mpEditLayerComboBox->currentText();
	QgsProject* project = QgsProject::instance();
	QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);

	if (!layers.isEmpty() && layers.first()->type() == Qgis::LayerType::Vector) {
		QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());

		// 检查图层是否处于编辑模式
		if (vectorLayer->isEditable()) {
			vectorLayer->rollBack();
			// 清除所有高亮显示的要素
			vectorLayer->removeSelection();

			// 刷新地图画布以显示回滚后的内容
			mpMapCanvas->refresh();
			QMessageBox::information(this, tr("取消编辑"), tr("所有未保存的更改已被取消。"));
		}
		else {
			QMessageBox::warning(this, tr("错误"), tr("图层不在编辑模式，无法取消编辑！"));
		}
	}

	// 重置地图工具，退出编辑状态
	resetMapTool();
}

void MainWindow::resetMapTool()
{
	// 将地图工具重置为平移工具
	QgsMapToolPan* panTool = new QgsMapToolPan(mpMapCanvas);
	mpMapCanvas->setMapTool(panTool);

	// 恢复按钮状态
	mpEditAction->setEnabled(true);
	mpEditLayerComboBox->setEnabled(true);
	mpEditAction->setChecked(false);
	setEditState(false);

	mRotationAngle = 0;

	// 退出编辑模式
	if (mEditCommandManager) {
		disconnect(mEditCommandManager, &EditCommandManager::stateChanged, this, nullptr);
		delete mEditCommandManager;
		mEditCommandManager = nullptr;
	}
}

// 创建边栏窗口
void MainWindow::createDockWidgets()
{
	// 创建浏览器上侧左边栏
	QDockWidget* mpLeftTopDock = new QDockWidget(tr("浏览器"), this);
	mpLeftTopDock->setAllowedAreas(Qt::LeftDockWidgetArea);

	// 创建文件树
	mpFileTreeWidget->setHeaderLabel(tr("文件"));
	mpFileTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(mpFileTreeWidget, &QTreeWidget::customContextMenuRequested, mLayerManager, &LayerManager::onFileTreeContextMenu);
	mpLeftTopDock->setWidget(mpFileTreeWidget);
	addDockWidget(Qt::LeftDockWidgetArea, mpLeftTopDock);

	// 创建选择目录按钮
	QToolButton* selectDirectoryButton = new QToolButton(mpLeftTopDock);
	selectDirectoryButton->setText(tr("选择目录"));
	connect(selectDirectoryButton, &QToolButton::clicked, mLayerManager, &LayerManager::selectDirectory);

	// 将按钮和文件树控件添加到垂直布局中
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(selectDirectoryButton);
	layout->addWidget(mpFileTreeWidget);
	QWidget* container = new QWidget();
	container->setLayout(layout);
	mpLeftTopDock->setWidget(container);

	// 用当前目录填充文件树
	mLayerManager->populateFileTree(QDir::currentPath());

	// 创建图层下侧左边栏
	QDockWidget* mpLeftBottomDock = new QDockWidget(tr("图层"), this);
	mpLeftBottomDock->setAllowedAreas(Qt::LeftDockWidgetArea);
	addDockWidget(Qt::LeftDockWidgetArea, mpLeftBottomDock);

	// 创建图层树模型并设置属性
	QgsLayerTreeModel* mpLayerTreeModel = new QgsLayerTreeModel(QgsProject::instance()->layerTreeRoot(), this);
	mpLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeChangeVisibility);
	mpLayerTreeView->setModel(mpLayerTreeModel);
	mpLayerTreeView->setDropIndicatorShown(true);

	// 连接图层树模型的数据更改信号与更新地图画布图层的槽函数
	connect(mpLayerTreeModel, &QAbstractItemModel::dataChanged, mLayerManager, &LayerManager::updateMapCanvasLayers);

	mpLeftBottomDock->setWidget(mpLayerTreeView);

	// 设置图层树视图的上下文菜单
	mpLayerTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(mpLayerTreeView, &QgsLayerTreeView::customContextMenuRequested, mLayerManager, &LayerManager::showLayerTreeContextMenu);

	// 创建右侧顶部的工具箱边栏
	QDockWidget* mpRightDock = new QDockWidget(tr("工具箱"), this);
	mpRightDock->setAllowedAreas(Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, mpRightDock);
	QVBoxLayout* mpToolLayout = new QVBoxLayout();
	QWidget* toolWidget = new QWidget(this);
	QTreeWidget* mpToolTree = new QTreeWidget(toolWidget);
	mpToolTree->setHeaderHidden(true); // 隐藏表头

	// 工具栏工具树
	QTreeWidgetItem* mpVectorAnalysisItem = new QTreeWidgetItem(mpToolTree);
	mpVectorAnalysisItem->setText(0, tr("矢量分析"));
	QTreeWidgetItem* mpKMeansItem = new QTreeWidgetItem(mpVectorAnalysisItem);
	mpKMeansItem->setText(0, tr("K均值聚类"));
	QTreeWidgetItem* mpSpatialJoinItem = new QTreeWidgetItem(mpVectorAnalysisItem);
	mpSpatialJoinItem->setText(0, tr("按位置连接属性"));

	QTreeWidgetItem* mpRasterAnalysisItem = new QTreeWidgetItem(mpToolTree);
	mpRasterAnalysisItem->setText(0, tr("栅格分析"));
	QTreeWidgetItem* mpRasterStatsItem = new QTreeWidgetItem(mpRasterAnalysisItem);
	mpRasterStatsItem->setText(0, tr("栅格图层统计"));

	QTreeWidgetItem* mpSymItem = new QTreeWidgetItem(mpToolTree);
	mpSymItem->setText(0, tr("符号系统"));
	QTreeWidgetItem* mpClassItem = new QTreeWidgetItem(mpSymItem);
	mpClassItem->setText(0, tr("按类别符号化图层-唯一值"));

	mpToolLayout->addWidget(mpToolTree);
	toolWidget->setLayout(mpToolLayout);
	mpRightDock->setWidget(toolWidget);

	// 将工具树各选项与实现函数链接
	connect(mpToolTree, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item, int) {
		if (item == mpKMeansItem) {
			mVectorAnalysis->performKMeansClustering();
		}
		else if (item == mpSpatialJoinItem) {
			mVectorAnalysis->performSpatialJoin();
		}
		else if (item == mpRasterStatsItem) {
			mRasterAnalysis->performRasterLayerStatistics();
		}
		else if (item == mpClassItem) {
			mLayerManager->performCategorizedSymbology();
		}
		});
}

// 创建地图画布
void MainWindow::createMapCanvas()
{
	mpMapCanvas->setCanvasColor(Qt::white);
	mpMapCanvas->freeze(false);

	// 创建平移工具并设置为地图画布的当前工具
	QgsMapToolPan* mpPanTool = new QgsMapToolPan(mpMapCanvas);
	mpMapCanvas->setMapTool(mpPanTool);
	setCentralWidget(mpMapCanvas); // 设置为主窗口的中央控件
}

// 创建状态栏
void MainWindow::createStatusBar()
{
	QStatusBar* mpStatusBar = new QStatusBar(this);
	setStatusBar(mpStatusBar);

	// 坐标标签
	mpCoordinatesLabel = new QLabel("坐标: ", this);
	mpStatusBar->addPermanentWidget(mpCoordinatesLabel);

	// 坐标系标签
	mpCrsLabel = new QLabel("图层坐标系: ", this);
	mpStatusBar->addPermanentWidget(mpCrsLabel);

	// 连接图层可见性变化信号
	connect(mpLayerTreeView->model(), &QgsLayerTreeModel::dataChanged, this, &MainWindow::updateCrsInfo);

	// 比例尺标签，可选择常用比例
	mpScaleComboBox = new QComboBox(this);
	QStringList mScales = { "1:1000", "1:2500", "1:5000", "1:10000", "1:25000", "1:50000", "1:100000", "1:250000", "1:500000", "1:1000000" };
	mpScaleComboBox->addItems(mScales);
	mpScaleComboBox->setEditable(true);
	mpStatusBar->addPermanentWidget(new QLabel("比例: ", this));
	mpStatusBar->addPermanentWidget(mpScaleComboBox);

	// 旋转角度标签
	mpRotationSpinBox = new QDoubleSpinBox(this);
	mpRotationSpinBox->setRange(-360, 360);
	mpRotationSpinBox->setSingleStep(5);
	mpRotationSpinBox->setSuffix("°");
	mpStatusBar->addPermanentWidget(new QLabel("旋转角度: ", this));
	mpStatusBar->addPermanentWidget(mpRotationSpinBox);

	// 连接比例尺下拉框和旋转角度调整框的信号
	connect(mpScaleComboBox, &QComboBox::currentTextChanged, this, &MainWindow::updateScale);
	connect(mpRotationSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::updateRotation);

	// 连接更新坐标、比例、旋转角度标签的信号
	connect(mpMapCanvas, &QgsMapCanvas::xyCoordinates, this, &MainWindow::updateCoordinates);

	connect(mpMapCanvas, &QgsMapCanvas::scaleChanged, this, [this](double scale) {
		mpScaleComboBox->setCurrentText(QString("1:%1").arg(static_cast<int>(scale)));
		});

	connect(mpMapCanvas, &QgsMapCanvas::rotationChanged, this, [this](double rotation) {
		mpRotationSpinBox->setValue(rotation);
		});

	// 状态栏背景设置为灰色
	mpStatusBar->setStyleSheet("QStatusBar { background-color: lightgray; }");

	// 连接鼠标移动事件
	mpMapCanvas->setMouseTracking(true);
	connect(mpMapCanvas, &QgsMapCanvas::xyCoordinates, this, &MainWindow::updateCoordinates);

	// 创建新行用于进度条和状态标签
	QWidget* progressBarContainer = new QWidget(this);
	QHBoxLayout* progressBarLayout = new QHBoxLayout(progressBarContainer);
	progressBarLayout->setContentsMargins(0, 0, 0, 0);

}

void MainWindow::updateCoordinates(const QgsPointXY& point)
{
	// 判断坐标是否为地理坐标系
	bool isGeographic = qAbs(point.x()) <= 180 && qAbs(point.y()) <= 90;

	if (isGeographic)
	{
		QString xDirection = point.x() >= 0 ? "E" : "W";
		QString yDirection = point.y() >= 0 ? "N" : "S";
		mpCoordinatesLabel->setText(QString("坐标: %1°%2, %3°%4")
			.arg(qAbs(point.x())).arg(xDirection)
			.arg(qAbs(point.y())).arg(yDirection));
	}
	else
	{
		// 如果是投影坐标系，显示投影坐标系的 xy 坐标并加上单位
		mpCoordinatesLabel->setText(QString("坐标: X: %1 m, Y: %2 m").arg(point.x(), 0, 'f', 2).arg(point.y(), 0, 'f', 2));
	}
}

// 更新比例尺
void MainWindow::updateScale()
{
	double scale = mpScaleComboBox->currentText().remove("1:").toDouble();
	if (scale > 0)
	{
		mpMapCanvas->zoomScale(scale);
	}
}

// 更新旋转角度
void MainWindow::updateRotation()
{
	double rotation = mpRotationSpinBox->value();
	mpMapCanvas->setRotation(rotation);
}

// 打开工程文件
void MainWindow::openProject()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("打开工程文件"), "", tr("工程文件 (*.qgz *.qgs)"));
	if (!fileName.isEmpty()) {
		QgsProject* project = QgsProject::instance();
		project->clear(); // 清理当前工程

		if (project->read(fileName)) {
			// 加载成功
			QMessageBox::information(this, tr("打开工程文件"), tr("工程文件已成功打开。"));
		}
		else {
			// 加载失败
			QMessageBox::warning(this, tr("打开工程文件"), tr("工程文件打开失败。"));
		}
	}
}


// 保存工程文件
void MainWindow::saveProject() {
	// 获取保存文件路径
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("保存工程文件"),
		"",
		tr("QGIS 项目文件 (*.qgz *.qgs)"));

	// 如果用户取消了保存操作，文件名将是空的
	if (fileName.isEmpty()) {
		return;
	}

	// 根据用户选择的文件名后缀，确保文件名以.qgz或.qgs结尾
	if (!fileName.endsWith(".qgz") && !fileName.endsWith(".qgs")) {
		// 如果用户未手动添加后缀，可以根据文件过滤器默认选择.qgs
		fileName += ".qgs";
	}

	// 保存项目
	QgsProject* project = QgsProject::instance();
	if (project->write(fileName)) {
		// 保存成功
		QMessageBox::information(this, tr("保存工程文件"), tr("工程文件已成功保存。"));
	}
	else {
		// 保存失败
		QMessageBox::warning(this, tr("保存工程文件"), tr("工程文件保存失败。"));
	}
}


// 关于信息窗口
void MainWindow::about() {
	if (!mpAboutMessageBox) {
		mpAboutMessageBox = new QMessageBox(this);
		mpAboutMessageBox->setIcon(QMessageBox::Information);
		mpAboutMessageBox->setWindowTitle(tr("关于"));
		mpAboutMessageBox->setText(tr("作者: 谢宇瀚、曾亦凡、王芊卓、万科君、叶昱彤、邱馨\n中国地质大学(武汉) 地理与信息工程学院 114222&114223\n2024.11"));
		mpAboutMessageBox->setStandardButtons(QMessageBox::Ok);
	}
	mpAboutMessageBox->exec();
}

// 打开通过QGIS的API制作的样式管理器
void MainWindow::openStyleManager()
{
	// 获取 QGIS 项目默认的样式实例
	QgsStyle* defaultStyle = QgsStyle::defaultStyle();
	if (!defaultStyle) {
		QMessageBox::warning(this, tr("错误"), tr("无法加载默认样式。"));
		return;
	}

	// 创建样式管理器对话框
	QgsStyleManagerDialog* styleManagerDialog = new QgsStyleManagerDialog(defaultStyle, this);
	styleManagerDialog->setAttribute(Qt::WA_DeleteOnClose);
	styleManagerDialog->show();  // 显示样式管理器
}

// 打开自己制作的样式管理器
void MainWindow::openSymbolManager()
{
	if (!mSymbolManager) {
		mSymbolManager = new SymbolManager(this);
	}
	mSymbolManager->show(); // 显示样式管理器
}

void MainWindow::updateCrsInfo()
{
	// 获取当前选中的图层
	const auto layers = QgsProject::instance()->layerTreeRoot()->checkedLayers();

	// 当且仅当有一个图层被勾选时，显示其坐标系信息
	if (layers.size() == 1) {
		QgsMapLayer* layer = layers.first();
		if (layer) {
			// 获取图层的坐标系
			const QgsCoordinateReferenceSystem crs = layer->crs();

			// 检查是否为地理坐标系
			QString geographicCrs, projectedCrs;
			if (crs.isGeographic()) {
				geographicCrs = crs.description(); // 地理坐标系描述
				projectedCrs = "无"; // 无投影坐标系
			}
			else {
				// 若为投影坐标系
				QgsCoordinateReferenceSystem geoCrs = crs.toGeographicCrs(); // 获取地理基准坐标系
				geographicCrs = geoCrs.description();
				projectedCrs = crs.description(); // 投影坐标系描述
			}

			// 组合显示地理和投影坐标系
			QString crsDescription = QString("地理坐标系: %1 - 投影坐标系: %2")
				.arg(geographicCrs)
				.arg(projectedCrs);

			// 更新状态栏标签
			mpCrsLabel->setText(crsDescription);
		}
	}
	else {
		// 如果没有图层或多个图层被勾选，清空坐标系信息
		mpCrsLabel->setText("坐标系: 未选择或多个图层");
	}
}

// 投影实现函数
void MainWindow::performProjectionTransformation()
{
	ProjectionTransformDialog* dialog = new ProjectionTransformDialog(mpMapCanvas, this);
	dialog->setModal(false);  // 设置为非模态
	dialog->show();
	GDALAllRegister();
	OGRRegisterAll();

	if (dialog->exec() == QDialog::Accepted) {
		QgsCoordinateReferenceSystem targetCrs = dialog->targetCrs();
		QString outputPath = QFileDialog::getSaveFileName(this, tr("保存转换后的文件"), "", tr("*.shp;;*.geojson;;*.tif;;*.qgs"));
		if (outputPath.isEmpty()) {
			return;
		}

		QString driverName;
		if (outputPath.endsWith(".shp", Qt::CaseInsensitive)) {
			driverName = "ESRI Shapefile";
		}
		else if (outputPath.endsWith(".geojson", Qt::CaseInsensitive)) {
			driverName = "GeoJSON";
		}
		else if (outputPath.endsWith(".tif", Qt::CaseInsensitive)) {
			driverName = "GTiff";
		}
		else if (outputPath.endsWith(".qgs", Qt::CaseInsensitive)) {
			driverName = "qgs";
		}
		else {
			QMessageBox::warning(this, tr("错误"), tr("不支持的文件格式！"));
			return;
		}

		QList<QgsMapLayer*> originalLayers;

		// 判断图层来源：如果选择文件则使用选定文件的路径
		if (dialog->mLayerSourceComboBox->currentIndex() == 0) {
			originalLayers.append(dialog->selectedLayer());
		}
		else {
			QString inputPath = dialog->selectedFile();
			if (inputPath.isEmpty()) {
				QMessageBox::warning(this, tr("错误"), tr("请选择文件。"));
				return;
			}

			// 检查是否为QGS项目文件
			if (inputPath.endsWith(".qgs", Qt::CaseInsensitive)) {
				QgsProject* tempProject = new QgsProject();
				if (!tempProject->read(inputPath)) {
					QMessageBox::warning(this, tr("错误"), tr("无法加载QGS项目文件。"));
					return;
				}
				originalLayers = tempProject->mapLayers().values();

				if (originalLayers.isEmpty()) {
					QMessageBox::warning(this, tr("错误"), tr("QGS项目文件中没有图层。"));
					return;
				}
			}
			else {
				// 支持加载非QGS项目文件的矢量和栅格图层
				QgsMapLayer* vectorLayer = new QgsVectorLayer(inputPath, QFileInfo(inputPath).fileName(), "ogr");
				if (vectorLayer && vectorLayer->isValid()) {
					originalLayers.append(vectorLayer);
				}
				else {
					QgsRasterLayer* rasterLayer = new QgsRasterLayer(inputPath, QFileInfo(inputPath).fileName());
					if (rasterLayer && rasterLayer->isValid()) {
						originalLayers.append(rasterLayer);
					}
					else {
						QMessageBox::warning(this, tr("错误"), tr("选定图层无效。"));
						return;
					}
				}
			}
		}

		// 遍历每个原始图层进行投影转换
		for (QgsMapLayer* originalLayer : originalLayers) {
			QString inputPath = originalLayer->dataProvider()->dataSourceUri(); // 获取当前图层的数据源路径

			if (originalLayer->type() == Qgis::LayerType::Vector) {
				// 处理矢量图层
				if (driverName == "qgs") {
					// 创建新的QgsProject实例
					QgsProject newProject;

					// 遍历每个原始图层，进行处理
					for (QgsMapLayer* originalLayer : originalLayers) {
						// 创建新的图层
						QgsMapLayer* newLayer = originalLayer->clone(); // 克隆原始图层
						if (newLayer) {
							// 转换坐标参考系统
							newLayer->setCrs(targetCrs); // 设置目标CRS

							// 添加到新的项目中
							newProject.addMapLayer(newLayer);
						}
					}

					// 保存新的QGS项目文件
					if (!newProject.write(outputPath)) {
						QMessageBox::warning(this, tr("错误"), tr("无法保存QGS项目文件。"));
						return;
					}

					QMessageBox::information(this, tr("投影转换成功"), tr("QGS项目已成功转换并保存。"));
				}
				else {
					// 处理矢量图层
					GDALDataset* poSrcDS = (GDALDataset*)GDALOpenEx(inputPath.toUtf8().constData(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
					if (poSrcDS == nullptr) {
						QMessageBox::warning(this, tr("错误"), tr("无法打开源数据集。"));
						return;
					}

					OGRLayer* poLayer = poSrcDS->GetLayer(0);
					if (poLayer == nullptr) {
						GDALClose(poSrcDS);
						QMessageBox::warning(this, tr("错误"), tr("无法获取源图层。"));
						return;
					}

					OGRSpatialReference oTargetSRS;
					oTargetSRS.importFromWkt(targetCrs.toWkt().toUtf8().constData());

					GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(driverName.toUtf8().constData());
					if (poDriver == nullptr) {
						GDALClose(poSrcDS);
						QMessageBox::warning(this, tr("错误"), tr("指定的驱动未找到。"));
						return;
					}

					// 设置编码为 UTF-8，防止中文乱码
					char** papszOptions = nullptr;
					papszOptions = CSLSetNameValue(papszOptions, "ENCODING", "UTF-8");

					GDALDataset* poDstDS = poDriver->Create(outputPath.toUtf8().constData(), 0, 0, 0, GDT_Unknown, papszOptions);
					if (poDstDS == nullptr) {
						GDALClose(poSrcDS);
						QMessageBox::warning(this, tr("错误"), tr("无法创建目标数据集。"));
						return;
					}

					OGRLayer* poDstLayer = poDstDS->CreateLayer(poLayer->GetName(), &oTargetSRS, poLayer->GetGeomType(), nullptr);
					if (poDstLayer == nullptr) {
						GDALClose(poSrcDS);
						GDALClose(poDstDS);
						QMessageBox::warning(this, tr("错误"), tr("无法创建目标图层。"));
						return;
					}

					// 设置图层编码
					poDstLayer->SetMetadataItem("ENCODING", "UTF-8");

					// 创建字段并确保支持UTF-8字符
					OGRFeatureDefn* poFDefn = poLayer->GetLayerDefn();
					for (int i = 0; i < poFDefn->GetFieldCount(); i++) {
						OGRFieldDefn* poFieldDefn = poFDefn->GetFieldDefn(i);
						poDstLayer->CreateField(poFieldDefn);
					}

					OGRFeature* poFeature;
					poLayer->ResetReading();
					while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
						OGRFeature* poDstFeature = OGRFeature::CreateFeature(poDstLayer->GetLayerDefn());
						poDstFeature->SetFrom(poFeature);
						OGRGeometry* poGeometry = poFeature->GetGeometryRef();
						if (poGeometry != nullptr) {
							poGeometry->transformTo(&oTargetSRS);
							poDstFeature->SetGeometry(poGeometry);
						}
						if (poDstLayer->CreateFeature(poDstFeature) != OGRERR_NONE) {
							qDebug() << "创建要素失败。";
						}
						OGRFeature::DestroyFeature(poFeature);
						OGRFeature::DestroyFeature(poDstFeature);
					}

					GDALClose(poSrcDS);
					GDALClose(poDstDS);

					QgsVectorLayer* transformedLayer = new QgsVectorLayer(outputPath, QFileInfo(outputPath).baseName(), "ogr");
					if (transformedLayer->isValid()) {
						QgsProject::instance()->addMapLayer(transformedLayer);
						QMessageBox::information(this, tr("投影转换成功"), tr("文件已成功转换并加载。"));
					}
					else {
						QMessageBox::warning(this, tr("投影转换失败"), tr("转换后的文件无效，请检查格式和路径。"));
					}
				}
			}
			else if (originalLayer->type() == Qgis::LayerType::Raster) {
				// 处理栅格图层
				GDALDataset* poSrcDS = (GDALDataset*)GDALOpen(inputPath.toUtf8().constData(), GA_ReadOnly);
				if (poSrcDS == nullptr) {
					QMessageBox::warning(this, tr("错误"), tr("无法打开源数据集。"));
					return;
				}

				OGRSpatialReference oTargetSRS;
				oTargetSRS.importFromWkt(targetCrs.toWkt().toUtf8().constData());

				GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(driverName.toUtf8().constData());
				if (poDriver == nullptr) {
					GDALClose(poSrcDS);
					QMessageBox::warning(this, tr("错误"), tr("指定的驱动未找到。"));
					return;
				}

				GDALDataset* poDstDS = poDriver->CreateCopy(outputPath.toUtf8().constData(), poSrcDS, FALSE, nullptr, nullptr, nullptr);
				if (poDstDS == nullptr) {
					GDALClose(poSrcDS);
					QMessageBox::warning(this, tr("错误"), tr("无法创建目标数据集。"));
					return;
				}

				// 设置目标坐标系
				char* pszTargetWKT = nullptr;
				oTargetSRS.exportToWkt(&pszTargetWKT);
				poDstDS->SetProjection(pszTargetWKT);
				CPLFree(pszTargetWKT);

				CPLErr eErr = GDALReprojectImage(poSrcDS, nullptr, poDstDS, nullptr, GRA_Bilinear, 0.0, 0.0, nullptr, nullptr, nullptr);
				if (eErr != CE_None) {
					GDALClose(poSrcDS);
					GDALClose(poDstDS);
					QMessageBox::warning(this, tr("错误"), tr("栅格投影转换失败。"));
					return;
				}

				GDALClose(poSrcDS);
				GDALClose(poDstDS);

				QgsRasterLayer* transformedLayer = new QgsRasterLayer(outputPath, QFileInfo(outputPath).baseName(), "gdal");
				if (transformedLayer->isValid()) {
					transformedLayer->setCrs(targetCrs);
					QgsProject::instance()->addMapLayer(transformedLayer);
					mpMapCanvas->setDestinationCrs(transformedLayer->crs());
					mpMapCanvas->setExtent(transformedLayer->extent()); // 自动调整视图
					mpMapCanvas->refresh(); // 刷新地图画布
					QMessageBox::information(this, tr("投影转换成功"), tr("文件已成功转换并加载。"));
				}
				else {
					QMessageBox::warning(this, tr("投影转换失败"), tr("转换后的文件无效，请检查格式和路径。"));
				}
			}
		}
	}
}

QgsCoordinateReferenceSystem MainWindow::dataFrameCrs() const {
	return mDataFrameCrs;
}

void MainWindow::setDataFrameCrs(const QgsCoordinateReferenceSystem& crs) {
	mDataFrameCrs = crs;
	mDataFrameCrsSet = true;
	mpMapCanvas->setDestinationCrs(crs);  // 设置画布坐标系
	emit dataFrameCrsChanged(crs);  // 发出坐标系变更信号
}


void MainWindow::openDataFrameCrsDialog() {
	// 创建对话框
	QDialog dialog(this);
	dialog.resize(350, 150);
	dialog.setWindowTitle(tr("设置数据框坐标系"));

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	// 显示当前坐标系信息
	QLabel* currentCrsLabel = new QLabel(tr("当前数据框坐标系:"));
	layout->addWidget(currentCrsLabel);

	QString crsDescription = mDataFrameCrsSet ? mDataFrameCrs.description() : tr("未设置");
	QLabel* crsInfoLabel = new QLabel(crsDescription);
	layout->addWidget(crsInfoLabel);

	// 创建 QgsProjectionSelectionWidget
	QgsProjectionSelectionWidget* projectionSelector = new QgsProjectionSelectionWidget();
	layout->addWidget(projectionSelector);

	// 如果当前已设置数据框坐标系，则将其作为初始值显示在选择器中
	if (mDataFrameCrsSet) {
		projectionSelector->setCrs(mDataFrameCrs);
	}

	// "设置"按钮，点击时应用新的坐标系
	QPushButton* setButton = new QPushButton(tr("设置坐标系"));
	layout->addWidget(setButton);

	connect(setButton, &QPushButton::clicked, this, [=, &dialog]() {
		// 获取用户选择的坐标系
		QgsCoordinateReferenceSystem selectedCrs = projectionSelector->crs();
		if (selectedCrs.isValid()) {
			setDataFrameCrs(selectedCrs);  // 更新数据框坐标系
			crsInfoLabel->setText(selectedCrs.description());  // 更新显示
		}
		});

	// 添加对话框的按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
	layout->addWidget(buttonBox);
	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

	dialog.exec();
}

void MainWindow::onDeleteClicked()
{
	if (mEditCommandManager) {
		mEditCommandManager->beginCommand(tr("删除要素"));


		// 获取当前选中的图层名称
		QString currentLayerName = mpEditLayerComboBox->currentText();
		QgsProject* project = QgsProject::instance();
		QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);

		if (layers.isEmpty() || layers.first()->type() != Qgis::LayerType::Vector) {
			QMessageBox::warning(this, tr("错误"), tr("未选中有效的矢量图层！"));
			return;
		}

		QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());

		// 检查图层是否处于编辑模式
		if (!vectorLayer->isEditable()) {
			QMessageBox::warning(this, tr("错误"), tr("请先进入编辑模式！"));
			return;
		}

		// 获取所有选中的要素 ID
		QgsFeatureIds selectedIds = vectorLayer->selectedFeatureIds();
		if (selectedIds.isEmpty()) {
			QMessageBox::warning(this, tr("错误"), tr("当前没有选中的要素！"));
			return;
		}

		// 删除选中的要素
		bool success = vectorLayer->deleteFeatures(selectedIds);

		if (success) {
			QMessageBox::information(this, tr("成功"), tr("选中的要素已成功删除。"));
		}
		else {
			QMessageBox::warning(this, tr("错误"), tr("删除选中要素时发生错误！"));
		}

		vectorLayer->removeSelection(); // 清除选中状态
		mpMapCanvas->refresh();         // 刷新地图画布

		mEditCommandManager->endCommand();
	}
}

void MainWindow::undoEdit()
{
	if (mEditCommandManager) {
		mEditCommandManager->undo();
	}
}

void MainWindow::redoEdit()
{
	if (mEditCommandManager) {
		mEditCommandManager->redo();
	}
}

void MainWindow::onMoveClicked() {
	if (mEditCommandManager) {
		mEditCommandManager->beginCommand(tr("移动要素"));

		QString currentLayerName = mpEditLayerComboBox->currentText();
		QgsProject* project = QgsProject::instance();
		QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);
		QgsVectorLayer* currentLayer = static_cast<QgsVectorLayer*>(layers.first());

		if (currentLayer && !currentLayer->selectedFeatureIds().isEmpty()) {
			chooseAndMoveEditView = new EditView(mpMapCanvas);
			chooseAndMoveEditView->setLayer(currentLayer, EditView::MoveMode); // 切换到移动模式
			mpMapCanvas->setMapTool(chooseAndMoveEditView); // 设置地图工具
		}
		else {
			QMessageBox::warning(nullptr, tr("错误"), tr("请先选择要素！"));
		}

		mEditCommandManager->endCommand();
	}
}

void MainWindow::onCopyClicked() {
	QString currentLayerName = mpEditLayerComboBox->currentText();
	QgsProject* project = QgsProject::instance();
	QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);
	if (layers.isEmpty()) {
		QMessageBox::warning(this, tr("错误"), tr("未找到选中的图层！"));
		return;
	}
	QgsVectorLayer* currentLayer = qobject_cast<QgsVectorLayer*>(layers.first());
	if (!currentLayer) {
		QMessageBox::warning(this, tr("错误"), tr("选中的图层不是矢量图层！"));
		return;
	}

	if (currentLayer->selectedFeatureIds().isEmpty()) {
		QMessageBox::warning(this, tr("错误"), tr("请先选择要复制的要素！"));
		return;
	}

	EditView* copyEditView = new EditView(mpMapCanvas);
	copyEditView->setLayer(currentLayer, EditView::CopyMode); // 切换到复制模式
	mpMapCanvas->setMapTool(copyEditView); // 设置地图工具
}


//缓冲区
void MainWindow::on_pushButtonBuffer_clicked()
{
	mBufferDialog->show();
}

//矢量转栅格
void MainWindow::onVectorToRasterButtonClicked() {
	mvectorToRasterConverter->startConversion();
}

void MainWindow::createRotateTool() {
	// 创建旋转工具小窗
	mpRotateGroupBox = new QGroupBox("RotateTool", this);
	mpRotateGroupBox->resize(340, 200);
	// 设置可拖动
	mpRotateGroupBox->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool);
	QVBoxLayout* groupBoxLayout = new QVBoxLayout(mpRotateGroupBox);
	// 旋转中心标签和文本框
	QLabel* label1 = new QLabel("旋转中心:");
	groupBoxLayout->addWidget(label1);
	QHBoxLayout* hLayout1 = new QHBoxLayout;
	QLabel* label2 = new QLabel("X:");
	mpRotationCenterX = new QLineEdit();
	mpRotationCenterX->setMaximumWidth(120);
	QLabel* label3 = new QLabel("Y:");
	mpRotationCenterY = new QLineEdit();
	mpRotationCenterY->setMaximumWidth(120);
	hLayout1->addWidget(label2);
	hLayout1->addWidget(mpRotationCenterX);
	hLayout1->addWidget(label3);
	hLayout1->addWidget(mpRotationCenterY);
	groupBoxLayout->addLayout(hLayout1);
	// 添加旋转角度标签和文本框
	QHBoxLayout* hLayout2 = new QHBoxLayout;
	QLabel* label4 = new QLabel("旋转角度:");
	QLabel* label5 = new QLabel("°");
	QLineEdit* rotationAngle = new QLineEdit();
	rotationAngle->setMaximumWidth(50);
	rotationAngle->setText("0");
	hLayout2->addWidget(label4);
	hLayout2->addWidget(rotationAngle);
	hLayout2->addWidget(label5);
	groupBoxLayout->addLayout(hLayout2);
	// 添加旋转滑块
	QSlider* slider = new QSlider(Qt::Horizontal);
	slider->setRange(0, 360);  // 设置滑块范围
	slider->setValue(0);      // 设置初始值
	groupBoxLayout->addWidget(slider);

	// 绑定滑块和文本框的值
	connect(slider, &QSlider::valueChanged, [=](int value) {
		rotationAngle->setText(QString::number(value));
		rotate(mpRotationCenterX->text().toDouble(), mpRotationCenterY->text().toDouble(), value);
		});
	connect(rotationAngle, &QLineEdit::textChanged, [=](const QString& text) {
		slider->setValue(text.toDouble());
		rotate(mpRotationCenterX->text().toDouble(), mpRotationCenterY->text().toDouble(), text.toDouble());
		});

	// 设置 GroupBox 的初始状态
	mpRotateGroupBox->setVisible(false); // 默认隐藏
}

void MainWindow::onRotateClicked() {
	// 获取当前选中的图层名称
	QString currentLayerName = mpEditLayerComboBox->currentText();
	QgsProject* project = QgsProject::instance();
	QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);

	if (layers.isEmpty() || layers.first()->type() != Qgis::LayerType::Vector) {
		QMessageBox::warning(this, tr("错误"), tr("未选中有效的矢量图层！"));
		return;
	}

	QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());

	// 检查图层是否处于编辑模式
	if (!vectorLayer->isEditable()) {
		QMessageBox::warning(this, tr("错误"), tr("请先进入编辑模式！"));
		return;
	}

	// 获取所有选中的要素
	if (vectorLayer->selectedFeatures().isEmpty()) {
		QMessageBox::warning(this, tr("错误"), tr("当前没有选中的要素！"));
		return;
	}
	// 默认旋转中心
	QgsPointXY rotationCenter = vectorLayer->selectedFeatures().first().geometry().boundingBox().center();

	QRect actionRect = mpEditToolBar->actionGeometry(mpRotatePolygonAction); // 获取按钮位置
	QPoint globalPos = mpEditToolBar->mapToGlobal(actionRect.bottomLeft()); // 转为全局坐标
	mpRotateGroupBox->move(globalPos.x(), globalPos.y()); // 设置 GroupBox 的位置
	mpRotationCenterX->setText(QString::number(rotationCenter.x()));
	mpRotationCenterY->setText(QString::number(rotationCenter.y()));
	mpRotateGroupBox->setVisible(!mpRotateGroupBox->isVisible());
}

void MainWindow::rotate(double x, double y, double angle) {
	// 获取当前选中的图层名称
	QString currentLayerName = mpEditLayerComboBox->currentText();
	QgsProject* project = QgsProject::instance();
	QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);
	QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());
	QgsFeatureList selectedFeatures = vectorLayer->selectedFeatures();
	QTransform transform;
	// 设置旋转矩阵
	transform.translate(x, y);
	transform.rotate(angle - mRotationAngle);
	transform.translate(-x, -y);

	for (QgsFeature feature : selectedFeatures) {
		QgsGeometry geometry = feature.geometry();
		Qgis::GeometryOperationResult result = geometry.transform(transform);
		if (result == Qgis::GeometryOperationResult::Success) {
			// 设置旋转后的几何
			feature.setGeometry(geometry);
			// 更新要素
			vectorLayer->updateFeature(feature);
		}
	}
	mRotationAngle = angle;
}

void MainWindow::onMergeClicked() {
	// 获取当前选中的图层名称
	QString currentLayerName = mpEditLayerComboBox->currentText();
	QgsProject* project = QgsProject::instance();
	QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);

	if (layers.isEmpty() || layers.first()->type() != Qgis::LayerType::Vector) {
		QMessageBox::warning(this, tr("错误"), tr("未选中有效的矢量图层！"));
		return;
	}

	QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(layers.first());

	// 检查图层是否为面图层
	if (vectorLayer->geometryType() != Qgis::GeometryType::Polygon) {
		QMessageBox::warning(this, tr("错误"), tr("请选择面图层！"));
	}

	// 检查图层是否处于编辑模式
	if (!vectorLayer->isEditable()) {
		QMessageBox::warning(this, tr("错误"), tr("请先进入编辑模式！"));
		return;
	}

	// 获取所有选中的要素
	QgsFeatureList selectedFeatures = vectorLayer->selectedFeatures();
	if (selectedFeatures.size() < 2) {
		QMessageBox::warning(this, tr("错误"), tr("选择的要素少于两个，不能进行合并！"));
		return;
	}

	// 合并几何
	QVector<QgsGeometry> geometries;
	for (const QgsFeature& f : selectedFeatures) {
		geometries.append(f.geometry());
	}
	QgsGeometry mergedGeometry = QgsGeometry::unaryUnion(geometries);

	if (!mergedGeometry.isNull()) {
		// 创建新的要素并添加到图层
		QgsFeature newFeature(vectorLayer->fields());
		newFeature.setGeometry(mergedGeometry);
		vectorLayer->addFeature(newFeature);
		// 删除原来的要素
		vectorLayer->deleteSelectedFeatures();
		mpMapCanvas->refresh();         // 刷新地图画布
	}
	else {
		QMessageBox::warning(this, tr("错误"), tr("合并失败！"));
	}

}

void MainWindow::onInsertPointClicked() {
	QString currentLayerName = mpEditLayerComboBox->currentText();
	QgsProject* project = QgsProject::instance();
	QList<QgsMapLayer*> layers = project->mapLayersByName(currentLayerName);
	if (layers.isEmpty()) {
		QMessageBox::warning(this, tr("错误"), tr("未找到选中的图层！"));
		return;
	}
	QgsVectorLayer* currentLayer = qobject_cast<QgsVectorLayer*>(layers.first());
	if (!currentLayer) {
		QMessageBox::warning(this, tr("错误"), tr("选中的图层不是矢量图层！"));
		return;
	}

	EditView* insertEditView = new EditView(mpMapCanvas);
	insertEditView->setLayer(currentLayer, EditView::InsertPointMode); // 切换到复制模式
	mpMapCanvas->setMapTool(insertEditView); // 设置地图工具
}

void MainWindow::editLinePassivationLine()
{
	// 创建 PassivationLineTool 对象
	PassivationLineTool* passivationTool = new PassivationLineTool(mpMapCanvas);

	// 设置地图工具
	mpMapCanvas->setMapTool(passivationTool);

	// 连接信号和槽，以便在处理完成后进行相应的操作
	connect(passivationTool, &PassivationLineTool::finished, this, [this, passivationTool]() {
		// 处理完成后的操作
		qInfo("PassivationLineTool finished processing");

		// 释放资源
		delete passivationTool;
		});
}

void MainWindow::editLineInverseLine()
{
	// 创建对话框
	QDialog dialog;
	dialog.resize(350, 150);
	dialog.setWindowTitle(tr("反转线方向"));

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	QLabel* layerLabel = new QLabel(tr("请选择要反转方向的线矢量图层："), &dialog);
	layout->addWidget(layerLabel);

	// 创建图层选择下拉框
	QComboBox* layerComboBox = new QComboBox(&dialog);
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values())
	{
		if (l->type() == Qgis::LayerType::Vector && static_cast<QgsVectorLayer*>(l)->geometryType() == Qgis::GeometryType::Line)
		{
			layerComboBox->addItem(l->name(), l->id());
			qDebug() << "找到线矢量图层:" << l->name();
		}
	}
	layout->addWidget(layerComboBox);

	// 添加确认/取消按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	// 连接信号槽
	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 显示对话框
	if (dialog.exec() == QDialog::Accepted) {
		// 获取用户选择的图层ID
		QString selectedLayerId = layerComboBox->currentData().toString();
		qDebug() << "用户选择的图层ID：" << selectedLayerId;

		QgsMapLayer* selectedLayer = QgsProject::instance()->mapLayer(selectedLayerId);
		QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(selectedLayer);

		if (vectorLayer && vectorLayer->geometryType() == Qgis::GeometryType::Line)
		{
			qDebug() << "成功找到线矢量图层:" << vectorLayer->name();

			// 获取图层中的所有要素
			QgsFeatureIterator featureIterator = vectorLayer->getFeatures();
			QgsFeature feature;

			// 开始事务，保证操作的原子性
			vectorLayer->startEditing();

			// 遍历所有要素，反转线方向
			while (featureIterator.nextFeature(feature))
			{
				// 获取线的几何数据
				QgsGeometry geom = feature.geometry();

				// 检查几何类型
				if (geom.isEmpty()) {
					qDebug() << "该要素几何为空";
					continue;
				}

				// 使用 asMultiPolyline() 处理可能的多段线几何
				QgsMultiPolylineXY multiPolyline = geom.asMultiPolyline();
				if (multiPolyline.isEmpty()) {
					qDebug() << "该要素没有有效的多段线";
					continue;
				}

				// 遍历多段线中的每条线段
				for (QgsPolylineXY& polyline : multiPolyline)
				{
					// 反转单条线段的方向
					std::reverse(polyline.begin(), polyline.end());
				}

				// 更新几何体
				geom = QgsGeometry::fromMultiPolylineXY(multiPolyline);

				// 设置更新后的几何体
				feature.setGeometry(geom);

				// 保存更新后的要素
				vectorLayer->updateFeature(feature);
			}

			// 提交修改
			vectorLayer->commitChanges();

			// 更新视图
			vectorLayer->triggerRepaint();
			qDebug() << "线条方向已反转";
		}
	}
}

void MainWindow::editLineIntersectingLines() {
	// 创建对话框
	QDialog dialog;
	dialog.resize(350, 150);
	dialog.setWindowTitle(tr("相交线剪断"));

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	QLabel* layerLabel = new QLabel(tr("请选择要进行自相交处理的线矢量图层："), &dialog);
	layout->addWidget(layerLabel);

	// 创建图层选择下拉框
	QComboBox* layerComboBox = new QComboBox(&dialog);
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values()) {
		if (l->type() == Qgis::LayerType::Vector && static_cast<QgsVectorLayer*>(l)->geometryType() == Qgis::GeometryType::Line) {
			layerComboBox->addItem(l->name(), l->id());
			qDebug() << "找到线矢量图层:" << l->name();
		}
	}
	layout->addWidget(layerComboBox);

	// 添加确认/取消按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	// 连接信号槽
	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 显示对话框
	if (dialog.exec() == QDialog::Accepted) {
		// 获取用户选择的图层ID
		QString selectedLayerId = layerComboBox->currentData().toString();
		QgsMapLayer* selectedLayer = QgsProject::instance()->mapLayer(selectedLayerId);

		if (!selectedLayer) {
			QMessageBox::warning(this, tr("警告"), tr("未能找到选择的图层。"));
			return;
		}

		QgsVectorLayer* vectorLayer = static_cast<QgsVectorLayer*>(selectedLayer);
		if (!vectorLayer) {
			QMessageBox::warning(this, tr("警告"), tr("选择的图层不是矢量图层。"));
			return;
		}

		// 获取图层中的所有线要素
		QgsFeatureRequest request;
		QgsFeatureIterator iter = vectorLayer->getFeatures(request);

		// 存储所有线要素
		QList<QgsFeature> features;
		QgsFeature feature;
		while (iter.nextFeature(feature)) {
			features.append(feature);
		}

		// 新建图层以存储剪断后的线
		QgsVectorLayer* newLayer = new QgsVectorLayer("LineString?crs=" + vectorLayer->crs().toWkt(), "剪断后的线", "memory");

		// 获取原始图层的字段并添加到新图层
		QgsFields fields = vectorLayer->fields();
		newLayer->dataProvider()->addAttributes(fields.toList());
		newLayer->updateFields();

		// 创建一个特征ID变量，确保每个新特征有唯一ID
		int featureId = 1;  // 从1开始避免ID冲突
		int addedFeatureCount = 0;  // 用于统计新增特征的数量

		// 处理每个线要素
		for (int i = 0; i < features.size(); ++i) {
			QgsGeometry currentGeometry = features[i].geometry();
			QgsFeature currentFeature = features[i];

			// 遍历其他线要素与当前线要素进行交集操作
			for (int j = 0; j < features.size(); ++j) {
				if (i != j) {
					QgsGeometry intersectingGeometry = features[j].geometry();

					if (currentGeometry.intersects(intersectingGeometry)) {
						// 获取交集几何体
						QgsGeometry intersection = currentGeometry.intersection(intersectingGeometry);

						// 如果交集有效并且不是空几何体，并且交集是线段
						if (!intersection.isEmpty()) {
							// 创建新特征并设置属性
							QgsFeature newFeature;
							newFeature.setGeometry(intersection);
							newFeature.setAttributes(currentFeature.attributes());
							newFeature.setId(featureId++);  // 为新特征分配唯一ID
							newLayer->dataProvider()->addFeature(newFeature);
							addedFeatureCount++;  // 增加新增要素计数
						}
					}
				}
			}

			// 如果当前几何体有效，则将其添加到新图层
			if (currentGeometry.isGeosValid()) {
				QgsFeature newFeature;
				newFeature.setGeometry(currentGeometry);
				newFeature.setAttributes(currentFeature.attributes());  // 保留原始属性
				newFeature.setId(featureId++);  // 为新特征分配唯一ID
				newLayer->dataProvider()->addFeature(newFeature);
				addedFeatureCount++;  // 增加新增要素计数
			}
		}

		// 获取裁剪前的要素数量
		int initialFeatureCount = features.size();

		// 输出调试信息：打印新增特征的数量
		qDebug() << "裁剪前的要素个数：" << initialFeatureCount;
		qDebug() << "裁剪后的要素个数：" << addedFeatureCount;

		// 将处理后的结果加载到地图视图
		QgsProject::instance()->addMapLayer(newLayer);

		// 显示处理完成的消息
		QMessageBox::information(this, tr("处理完成"),
			tr("相交线已成功剪断。裁剪前的要素个数：%1，裁剪后的要素个数：%2")
			.arg(initialFeatureCount)
			.arg(addedFeatureCount));
	}
}

void MainWindow::editLineSimplification()
{
	// 如果尚未创建 mEditLine，则动态分配内存
	if (!mEditLine) {
		mEditLine = new EditLine(this);  // 将父对象设置为 MainWindow，便于内存管理
	}

	// 调用抽稀逻辑处理
	mEditLine->executeSimplification(this);
}

