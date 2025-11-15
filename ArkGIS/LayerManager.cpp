#pragma warning(disable:4996)
#include "LayerManager.h"
#include "StyleManager.h"
#include "SymbolLibrary.h"
#include "SymbolLibraryManager.h"
#include "CustomDialog.h"
#include "ProjectionTransformDialog.h"
#include <iostream>
#include <QFileDialog>
#include <QMessageBox>
#include <QgsProject.h>
#include <QgsLayerTree.h>
#include <QgsLayerTreeModel.h>
#include <QgsLayerTreeView.h>
#include <QgsVectorLayer.h>
#include <QgsRasterLayer.h>
#include <QgsMapLayer.h>
#include <QgsMapCanvas.h>
#include <QgsCategorizedSymbolRenderer.h>
#include <QgsVectorFileWriter.h>
#include <QTableWidget>
#include <QgsSymbol.h>
#include <QColor>
#include <qgscallout.h>>
#include <QgsFields.h>
#include <QgsField.h>
#include <QgsFeature.h>
#include <QgsFeatureIterator.h>
#include <QgsFeatureRequest.h>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDialog>
#include <QTextStream>
#include <QProgressDialog>
#include <QDoubleSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QMenuBar>
#include <QToolButton>
#include <QDockWidget>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QAbstractItemModel>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QgsMarkerSymbol.h>
#include <QgsLineSymbol.h>
#include <QgsFillSymbol.h>
#include <QgsSingleSymbolRenderer.h>
#include <QgsWkbTypes.h> 
#include <qgsmarkersymbollayer.h>
#include <QBuffer>
#include <QSvgGenerator>
#include <QPushButton>
#include <QInputDialog>
#include <QgsFeatureRequest.h>
#include <QGroupBox>
#include <QFormLayout>
#include <QColorDialog>
#include <QGraphicsPixmapItem>
#include <QgsGuiUtils.h>
#include <QgsProjectionSelectionWidget.h>

LayerManager::LayerManager(QgsMapCanvas* canvas, QTreeWidget* fileTreeWidget, QgsLayerTreeView* layerTreeView, MainWindow* mainWindow, QObject* parent)
	: QObject(parent),
	mpMapCanvas(canvas),
	mpFileTreeWidget(fileTreeWidget),
	mpLayerTreeView(layerTreeView),
	mMainWindow(mainWindow)
{
	// 检查传入的指针是否为 nullptr
	if (!mpMapCanvas || !mpFileTreeWidget || !mpLayerTreeView) {
		qFatal("LayerManager: 初始化失败 - 传入的指针不能为空。");
	}

	// 连接图层树视图的右键菜单请求信号
	connect(mpLayerTreeView, &QgsLayerTreeView::customContextMenuRequested,
		this, &LayerManager::showLayerTreeContextMenu);
}

// 添加矢量图层
void LayerManager::addVectorLayer()
{
	QString fileName = QFileDialog::getOpenFileName(nullptr, tr("添加矢量图层"), "", tr("矢量图层 (*.shp *.json *.geojson *.gml)"));
	if (!fileName.isEmpty()) {
		QgsVectorLayer* vectorLayer = new QgsVectorLayer(fileName, QFileInfo(fileName).baseName(), "ogr");

		if (vectorLayer->isValid()) {
			// 检查是否已设置数据框坐标系
			if (!mMainWindow->dataFrameCrs().isValid()) {
				// 未设置数据框坐标系，则将其设置为当前图层的坐标系
				mMainWindow->setDataFrameCrs(vectorLayer->crs());
			}
			else if (vectorLayer->crs() != mMainWindow->dataFrameCrs()) {
				// 如果图层坐标系与数据框坐标系不同，进行临时投影转换
				QgsCoordinateTransform transform(vectorLayer->crs(), mMainWindow->dataFrameCrs(), QgsProject::instance());

				// 遍历图层的所有要素并转换其几何
				QgsFeatureIterator it = vectorLayer->getFeatures();
				QgsFeature feature;
				vectorLayer->startEditing();
				while (it.nextFeature(feature)) {
					QgsGeometry geom = feature.geometry();
					geom.transform(transform);
					feature.setGeometry(geom);
					vectorLayer->updateFeature(feature);
				}
				vectorLayer->commitChanges();

				// 将图层的 CRS 更新为数据框坐标系
				vectorLayer->setCrs(mMainWindow->dataFrameCrs());
			}

			// 将图层添加到项目并更新地图画布
			QgsProject::instance()->addMapLayer(vectorLayer);
			mpMapCanvas->refresh();
		}
		else {
			QMessageBox::critical(nullptr, tr("错误"), tr("无法加载矢量图层"));
			delete vectorLayer;
		}
	}
}

void LayerManager::addRasterLayer()
{
	QString fileName = QFileDialog::getOpenFileName(nullptr, tr("添加栅格图层"), "", tr("栅格图层 (*.tif *.tiff *.img *.png *.jpg *.jpeg)"));
	if (!fileName.isEmpty()) {
		QgsRasterLayer* rasterLayer = new QgsRasterLayer(fileName, QFileInfo(fileName).baseName(), "gdal");

		if (rasterLayer->isValid()) {
			// 检查是否已设置数据框坐标系
			if (!mMainWindow->dataFrameCrs().isValid()) {
				// 未设置数据框坐标系，则将其设置为当前图层的坐标系
				mMainWindow->setDataFrameCrs(rasterLayer->crs());
			}
			else if (rasterLayer->crs() != mMainWindow->dataFrameCrs()) {
				// 如果图层坐标系与数据框坐标系不同，进行临时投影转换
				QgsCoordinateTransform transform(rasterLayer->crs(), mMainWindow->dataFrameCrs(), QgsProject::instance());

				// 设置其CRS进行坐标系转换
				rasterLayer->setCrs(mMainWindow->dataFrameCrs());
			}

			// 将图层添加到项目并更新地图画布
			QgsProject::instance()->addMapLayer(rasterLayer);
			mpMapCanvas->refresh();
		}
		else {
			QMessageBox::critical(nullptr, tr("错误"), tr("无法加载栅格图层"));
			delete rasterLayer;
		}
	}
}

// 添加分隔符文本图层
void LayerManager::addDelimitedTextLayer()
{
	QString fileName = QFileDialog::getOpenFileName(nullptr, tr("添加分隔符文本图层"), "", tr("分隔符文本图层 (*.csv *.txt)"));
	if (!fileName.isEmpty()) {
		// 创建分隔符文本图层对象
		QgsVectorLayer* delimitedTextLayer = new QgsVectorLayer(fileName, QFileInfo(fileName).baseName(), "ogr");
		if (delimitedTextLayer->isValid()) {
			// 将图层添加到项目中
			QgsProject::instance()->addMapLayer(delimitedTextLayer);
			updateMapCanvasLayers();
		}
		else {
			// 错误消息
			QMessageBox::critical(nullptr, tr("错误"), tr("无法加载分隔符文本图层"));
			delete delimitedTextLayer;
		}
	}
}

// 文件树右键菜单响应函数
void LayerManager::onFileTreeContextMenu(const QPoint& pos)
{
	// 获取右键点击的图层项
	QTreeWidgetItem* item = mpFileTreeWidget->itemAt(pos);
	if (!item)  // 如果没有点击在任何项上直接返回
		return;

	mSelectedFilePath = item->data(0, Qt::UserRole).toString();
	if (mSelectedFilePath.isEmpty())  // 如果文件路径为空直接返回
		return;

	// 创建右键菜单
	mpContextMenu = new QMenu();
	QAction* addAction = new QAction(tr("添加到图层"), this);

	// 连接菜单项的触发信号
	connect(addAction, &QAction::triggered, this, &LayerManager::addLayerFromFile);

	mpContextMenu->addAction(addAction);
	// 显示右键菜单，位置为鼠标点击位置转换到全局坐标系
	mpContextMenu->exec(mpFileTreeWidget->viewport()->mapToGlobal(pos));
}

// 选择路径
void LayerManager::selectDirectory()
{
	// 获取路径
	QString directory = QFileDialog::getExistingDirectory(nullptr, tr("选择目录"), QDir::homePath());
	if (!directory.isEmpty()) {
		// 调用填充文件树函数
		populateFileTree(directory);
	}
}

// 填充文件树的函数
void LayerManager::populateFileTree(const QString& path)
{
	// 清空文件树
	mpFileTreeWidget->clear();
	QDir dir(path);

	// 获取目录中所有子目录和文件的信息列表并遍历文件信息列表
	QFileInfoList fileList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files);
	foreach(QFileInfo fileInfo, fileList)
	{
		// 为每个文件或目录创建一个新的树项，并添加到文件树控件中
		QTreeWidgetItem* item = new QTreeWidgetItem(mpFileTreeWidget);
		item->setText(0, fileInfo.fileName());

		if (fileInfo.isDir()) {
			// 如果是目录则递归调用辅助函数，继续填充该目录
			populateFileTreeHelper(fileInfo.absoluteFilePath(), item);
		}
		else {
			// 如果是文件则设置用户数据为文件的绝对路径
			item->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
		}
	}
}

// 辅助函数，用于递归填充目录树
void LayerManager::populateFileTreeHelper(const QString& path, QTreeWidgetItem* parentItem)
{
	QDir dir(path);
	QFileInfoList fileList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files);
	foreach(QFileInfo fileInfo, fileList)
	{
		QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
		item->setText(0, fileInfo.fileName());

		if (fileInfo.isDir()) {
			populateFileTreeHelper(fileInfo.absoluteFilePath(), item);
		}
		else {
			item->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
		}
	}
}

// 显示图层树右键菜单
void LayerManager::showLayerTreeContextMenu(const QPoint& pos)
{
	QModelIndex index = mpLayerTreeView->indexAt(pos);
	if (!index.isValid())
		return;

	QMenu contextMenu;
	QAction* zoomToLayerAction = contextMenu.addAction(tr("缩放到图层"));
	connect(zoomToLayerAction, &QAction::triggered, this, &LayerManager::zoomToLayer);

	QAction* removeLayerAction = contextMenu.addAction(tr("移除图层"));
	connect(removeLayerAction, &QAction::triggered, this, &LayerManager::removeSelectedLayer);

	QAction* saveLayerAction = contextMenu.addAction(tr("保存图层"));
	connect(saveLayerAction, &QAction::triggered, this, &LayerManager::saveSelectedLayer);

	QAction* viewAttributesAction = contextMenu.addAction(tr("查看属性表"));
	connect(viewAttributesAction, &QAction::triggered, this, &LayerManager::viewLayerAttributes);

	QAction* showLabelsAction = contextMenu.addAction(tr("显示注记"));
	connect(showLabelsAction, &QAction::triggered, this, &LayerManager::showLayerLabels);

	// 添加样式管理器菜单项
	QAction* styleManagerAction = contextMenu.addAction(tr("QGIS符号化"));
	connect(styleManagerAction, &QAction::triggered, this, [=]() {
		QgsMapLayer* layer = mpLayerTreeView->currentLayer();
		if (layer)
		{
			StyleManager* styleManager = new StyleManager(this);
			styleManager->openStyleManager(layer);
		}
		});

	QAction* symbolizationAction = contextMenu.addAction(tr("符号化"));
	connect(symbolizationAction, &QAction::triggered, this, &LayerManager::openSymbolizationDialog);

	contextMenu.exec(mpLayerTreeView->viewport()->mapToGlobal(pos));
}

// 更新地图画布上的图层
void LayerManager::updateMapCanvasLayers() {
	// 列表存储图层
	QList<QgsMapLayer*> layers;
	QgsLayerTreeGroup* root = QgsProject::instance()->layerTreeRoot();

	// 遍历所有图层节点
	for (QgsLayerTreeLayer* layerNode : root->findLayers()) {
		if (layerNode->isVisible()) {
			// 如果图层节点可见，将其图层添加到列表中
			layers.append(layerNode->layer());
		}
	}
	mpMapCanvas->setLayers(layers);

	// 如果列表不为空，设置地图画布的范围为第一个图层的范围，并刷新地图画布
	if (!layers.isEmpty()) {
		mpMapCanvas->setExtent(layers.first()->extent());
		mpMapCanvas->refresh();
	}
	emit layersChanged();
}

// 从文件添加图层
void LayerManager::addLayerFromFile()
{
	// 选中的文件路径为空直接返回
	if (mSelectedFilePath.isEmpty())
		return;

	// 获取文件信息及扩展名
	QFileInfo fileInfo(mSelectedFilePath);
	QString fileExtension = fileInfo.suffix().toLower();

	// 加载矢量图层
	if (fileExtension == "shp" || fileExtension == "json" || fileExtension == "geojson" || fileExtension == "gml")
	{
		QgsVectorLayer* vectorLayer = new QgsVectorLayer(mSelectedFilePath, fileInfo.baseName(), "ogr");
		if (vectorLayer->isValid())
		{
			// 添加图层并更新地图画布图层
			QgsProject::instance()->addMapLayer(vectorLayer);
			updateMapCanvasLayers();
		}
		else
		{
			// 如果图层无效，显示错误消息并删除图层
			QMessageBox::critical(nullptr, tr("错误"), tr("无法加载矢量图层"));
			delete vectorLayer;
		}
	}
	// 加载栅格图层，同矢量图层
	else if (fileExtension == "tif" || fileExtension == "tiff" || fileExtension == "png" || fileExtension == "jpg" || fileExtension == "jpeg")
	{
		QgsRasterLayer* rasterLayer = new QgsRasterLayer(mSelectedFilePath, fileInfo.baseName());
		if (rasterLayer->isValid())
		{
			QgsProject::instance()->addMapLayer(rasterLayer);
			updateMapCanvasLayers();
		}
		else
		{
			QMessageBox::critical(nullptr, tr("错误"), tr("无法加载栅格图层"));
			delete rasterLayer;
		}
	}
}

void LayerManager::zoomToLayer()
{
	// 获取当前选中的图层
	QgsMapLayer* layer = mpLayerTreeView->currentLayer();
	if (!layer)
		return;

	// 获取图层范围并更新地图画布
	mpMapCanvas->setExtent(layer->extent());
	mpMapCanvas->refresh();
}

// 移除选中的图层
void LayerManager::removeSelectedLayer()
{
	// 获取当前选中的图层
	QgsMapLayer* layer = mpLayerTreeView->currentLayer();
	if (!layer)
		return;

	// 从项目中移除图层并更新地图画布图层
	QgsProject::instance()->removeMapLayer(layer);
	updateMapCanvasLayers();
	mpMapCanvas->refresh();
}

// 保存选中的图层
void LayerManager::saveSelectedLayer()
{
	QgsMapLayer* layer = mpLayerTreeView->currentLayer();
	if (!layer)
		return;

	// 获取文件保存路径
	QString fileName = QFileDialog::getSaveFileName(nullptr, tr("保存图层"), "", tr("Shapefile (*.shp);;GeoJSON (*.geojson);;GML (*.gml)"));
	if (fileName.isEmpty())
		return;

	// 将选中的图层转换为矢量图层
	QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);
	if (vectorLayer)
	{
		QgsVectorFileWriter::writeAsVectorFormat(vectorLayer, fileName, "UTF-8", vectorLayer->crs(), "ESRI Shapefile");
	}
	else
	{
		// 如果选中的不是矢量图层则显示警告
		QMessageBox::warning(nullptr, tr("错误"), tr("仅支持保存矢量图层"));
	}
}

// 查看选中图层的属性表
void LayerManager::viewLayerAttributes() {
	QgsMapLayer* layer = mpLayerTreeView->currentLayer();
	if (!layer) {
		QMessageBox::warning(nullptr, tr("错误"), tr("无法找到选择的图层。"));
		return;
	}

	QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);
	if (!vectorLayer) {
		QMessageBox::warning(nullptr, tr("错误"), tr("仅支持矢量图层。"));
		return;
	}

	CustomDialog* dialog = new CustomDialog(mpLayerTreeView, vectorLayer);
	dialog->exec();
}


// 进行分类符号化
void LayerManager::performCategorizedSymbology()
{
	QDialog dialog;
	dialog.resize(350, 150);
	dialog.setWindowTitle(tr("按类别符号化图层"));
	dialog.resize(300, 150);

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	QLabel* label = new QLabel(tr("请选择图层："), &dialog);
	layout->addWidget(label);

	// 创建图层和字段的下拉框
	QComboBox* layerComboBox = new QComboBox(&dialog);
	QComboBox* fieldComboBox = new QComboBox(&dialog);

	// 遍历项目中的所有图层，并将矢量图层添加到图层下拉框中
	for (QgsMapLayer* l : QgsProject::instance()->mapLayers().values())
	{
		if (l->type() == Qgis::LayerType::Vector)
		{
			layerComboBox->addItem(l->name(), l->id());
		}
	}
	layout->addWidget(layerComboBox);

	QLabel* fieldLabel = new QLabel(tr("请选择字段："), &dialog);
	layout->addWidget(fieldLabel);

	layout->addWidget(fieldComboBox);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 定义更新字段下拉框的lambda函数
	auto updateFields = [=]() {
		fieldComboBox->clear();
		QString layerId = layerComboBox->currentData().toString();
		QgsVectorLayer* layer = dynamic_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayer(layerId));
		if (layer)
		{
			const QgsFields& fields = layer->fields();
			for (const QgsField& field : fields)
			{
				fieldComboBox->addItem(field.name());
			}
		}
		};

	// 连接图层下拉框的索引变化信号与更新字段下拉框的lambda函数
	connect(layerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int) {
		updateFields();
		});

	// 手动触发一次更新以填充初始选择图层的字段
	if (layerComboBox->count() > 0) {
		layerComboBox->setCurrentIndex(0);  // 触发信号
		updateFields();
	}

	if (dialog.exec() == QDialog::Accepted)
	{
		// 获取选中的图层ID和字段名称
		QString layerId = layerComboBox->currentData().toString();
		QString fieldName = fieldComboBox->currentText();

		QgsVectorLayer* layer = dynamic_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayer(layerId));
		if (!layer)
		{
			QMessageBox::warning(nullptr, tr("错误"), tr("无法找到选择的图层。"));
			return;
		}

		// 调用分类符号化实现函数
		applyCategorizedSymbology(layer, fieldName);
	}
}

// 实现分类符号化
void LayerManager::applyCategorizedSymbology(QgsVectorLayer* layer, const QString& fieldName)
{
	int fieldIndex = layer->fields().indexFromName(fieldName);
	if (fieldIndex == -1)
	{
		QMessageBox::warning(nullptr, tr("错误"), tr("选择的字段无效。"));
		return;
	}

	QMap<QString, QgsSymbol*> symbols;

	// 遍历要素获取唯一值
	QSet<QString> uniqueValues;
	QgsFeatureIterator it = layer->getFeatures();
	QgsFeature feature;
	while (it.nextFeature(feature))
	{
		uniqueValues.insert(feature.attribute(fieldIndex).toString());
	}

	// 创建符号
	int colorIndex = 0;
	for (const QString& value : uniqueValues) {
		QgsSymbol* symbol = QgsSymbol::defaultSymbol(layer->geometryType());
		symbol->setColor(QColor::fromHsv(colorIndex * 360 / uniqueValues.size(), 255, 255));
		symbols.insert(value, symbol);
		colorIndex++;
	}

	// 创建分类符号化渲染器
	QList<QgsRendererCategory> categories;
	for (auto it = symbols.constBegin(); it != symbols.constEnd(); ++it)
	{
		categories.append(QgsRendererCategory(it.key(), it.value(), it.key()));
	}

	QgsCategorizedSymbolRenderer* renderer = new QgsCategorizedSymbolRenderer(fieldName, categories);
	layer->setRenderer(renderer);
	layer->triggerRepaint();

	QMessageBox::information(nullptr, tr("成功"), tr("分类符号化已应用。"));
}

// 打开符号化对话框
void LayerManager::openSymbolizationDialog() {
	QgsMapLayer* layer = mpLayerTreeView->currentLayer();
	QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);

	if (!vectorLayer) {
		QMessageBox::warning(nullptr, "错误", "仅支持矢量图层符号化。");
		return;
	}

	// 保存初始符号避免直接修改
	std::unique_ptr<QgsSymbol> originalSymbol;
	auto* renderer = dynamic_cast<QgsSingleSymbolRenderer*>(vectorLayer->renderer());
	if (renderer) {
		originalSymbol.reset(renderer->symbol()->clone());
	}
	else {
		originalSymbol.reset(QgsSymbol::defaultSymbol(vectorLayer->geometryType()));
	}

	QgsSymbol* currentQgsSymbol = renderer->symbol()->clone();  // 克隆当前符号避免直接修改
	Symbol currentSymbol = Symbol::fromQgsSymbol(currentQgsSymbol);

	// 创建符号化对话框
	QDialog dialog;
	dialog.setWindowTitle(tr("符号化 - %1").arg(vectorLayer->name()));
	dialog.setFixedSize(600, 800);

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	// 创建 QTabWidget 来实现两个不同的页面
	QTabWidget* tabWidget = new QTabWidget(&dialog);
	layout->addWidget(tabWidget);

	// 第一页 根据图层类型进行基础属性符号化
	QWidget* typeBasedPage = new QWidget();
	QVBoxLayout* typeBasedLayout = new QVBoxLayout(typeBasedPage);

	QLabel* typeLabel = new QLabel("图层类型：", &dialog);
	QString layerType;
	if (vectorLayer->geometryType() == Qgis::GeometryType::Point) {
		layerType = "点图层";
	}
	else if (vectorLayer->geometryType() == Qgis::GeometryType::Line) {
		layerType = "线图层";
	}
	else if (vectorLayer->geometryType() == Qgis::GeometryType::Polygon) {
		layerType = "面图层";
	}
	typeLabel->setText(typeLabel->text() + layerType);
	typeBasedLayout->addWidget(typeLabel);

	// 根据图层类型创建不同的属性编辑控件
	if (vectorLayer->geometryType() == Qgis::GeometryType::Point) {
		setupPointSymbolizationUI(typeBasedLayout, vectorLayer);
	}
	else if (vectorLayer->geometryType() == Qgis::GeometryType::Line) {
		setupLineSymbolizationUI(typeBasedLayout, vectorLayer);
	}
	else if (vectorLayer->geometryType() == Qgis::GeometryType::Polygon) {
		setupPolygonSymbolizationUI(typeBasedLayout, vectorLayer);
	}
	tabWidget->addTab(typeBasedPage, "基础属性符号化");

	// 第二页通过符号库选择符号
	QWidget* symbolLibraryPage = new QWidget();
	QVBoxLayout* libraryLayout = new QVBoxLayout(symbolLibraryPage);

	QLabel* label2 = new QLabel("选择符号库：", &dialog);
	libraryLayout->addWidget(label2);

	QComboBox* libraryComboBox = new QComboBox(&dialog);
	libraryLayout->addWidget(libraryComboBox);

	QLabel* label1 = new QLabel(tr("选择符号："));
	libraryLayout->addWidget(label1);

	QListView* symbolListView = new QListView(&dialog);
	QStandardItemModel* model = new QStandardItemModel(symbolListView);
	symbolListView->setModel(model);
	libraryLayout->addWidget(symbolListView);

	QList<QString> libraries = SymbolLibraryManager::instance().getAvailableLibraries();
	for (const QString& library : libraries) {
		libraryComboBox->addItem(library);
	}

	auto updateSymbolList = [&]() {
		model->clear();
		const auto& symbols = SymbolLibraryManager::instance().getCurrentLibrary().getSymbols();
		for (const Symbol& symbol : symbols) {
			QStandardItem* item = new QStandardItem(symbol.name);
			item->setIcon(QIcon(symbol.generatePreview(32)));
			model->appendRow(item);
		}
		if (symbols.isEmpty()) {
			QStandardItem* emptyItem = new QStandardItem("没有符号可显示");
			model->appendRow(emptyItem);
		}
		symbolListView->reset();
		};

	connect(libraryComboBox, &QComboBox::currentTextChanged, [&](const QString& libraryPath) {
		if (SymbolLibraryManager::instance().loadLibrary(libraryPath)) {
			updateSymbolList();
		}
		});

	updateSymbolList();

	// 添加预览区域
	QLabel* previewLabel = new QLabel("当前符号预览：", &dialog);
	libraryLayout->addWidget(previewLabel);

	QGraphicsScene* previewScene = new QGraphicsScene(&dialog);
	QGraphicsView* previewView = new QGraphicsView(previewScene, &dialog);
	previewView->setFixedSize(200, 200);
	previewView->setAlignment(Qt::AlignCenter);
	previewView->setStyleSheet("border: 1px solid gray;");
	libraryLayout->addWidget(previewView, 0, Qt::AlignCenter);

	// 当用户选择不同的符号时更新预览
	connect(symbolListView->selectionModel(), &QItemSelectionModel::currentChanged, this, [&](const QModelIndex& index) {
		if (index.isValid()) {
			const auto& symbols = SymbolLibraryManager::instance().getCurrentLibrary().getSymbols();
			if (index.row() < symbols.size()) {
				const Symbol& symbol = symbols.at(index.row());

				// 更新符号预览
				previewScene->clear();
				QPixmap preview = symbol.generatePreview(128);
				previewScene->addPixmap(preview);
			}
		}
		});

	// 添加到tabWidget
	tabWidget->addTab(symbolLibraryPage, "符号库符号化");

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
	layout->addWidget(buttonBox);

	connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, [&]() {
		if (tabWidget->currentIndex() == 0) {
			// 当前是第一个页面（按图层类型符号化）
			applySymbolToLayerSimple(vectorLayer);
		}
		else {
			QModelIndex selectedIndex = symbolListView->currentIndex();

			// 如果没有选择符号，保留当前符号，并应用属性修改
			Symbol selectedSymbol;
			if (selectedIndex.isValid()) {
				const auto& symbols = SymbolLibraryManager::instance().getCurrentLibrary().getSymbols();
				selectedSymbol = symbols.at(selectedIndex.row());
			}
			else {
				// 没有选择符号时，使用当前符号
				selectedSymbol = currentSymbol;
			}

			// 应用符号到图层
			applySymbolToLayer(vectorLayer, selectedSymbol);

			// 更新预览窗口
			previewScene->clear();
			QPixmap preview = selectedSymbol.generatePreview(128);
			previewScene->addPixmap(preview);
		}
		});

	connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [&]() {
		auto* renderer = new QgsSingleSymbolRenderer(originalSymbol->clone());
		vectorLayer->setRenderer(renderer);
		vectorLayer->triggerRepaint();
		dialog.reject();
		});

	connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, [&]() {
		if (tabWidget->currentIndex() == 0) {
			applySymbolToLayerSimple(vectorLayer);
		}
		else {
			QModelIndex selectedIndex = symbolListView->currentIndex();
			// 如果没有选择符号，保留当前符号，并应用属性修改
			Symbol selectedSymbol;
			if (selectedIndex.isValid()) {
				const auto& symbols = SymbolLibraryManager::instance().getCurrentLibrary().getSymbols();
				selectedSymbol = symbols.at(selectedIndex.row());
			}
			else {
				// 没有选择符号时，使用当前符号
				selectedSymbol = currentSymbol;
			}

			// 应用符号到图层
			applySymbolToLayer(vectorLayer, selectedSymbol);

			// 更新预览窗口
			previewScene->clear();
			QPixmap preview = selectedSymbol.generatePreview(128);
			previewScene->addPixmap(preview);
		}

		// 关闭对话框
		dialog.accept();
		});
	if (dialog.exec() == QDialog::Accepted) {

	}
}

// 应用符号选择的符号到图层
void LayerManager::applySymbolToLayer(QgsVectorLayer* layer, const Symbol& symbol) {
	// 生成临时SVG文件路径
	QString tempSvgPath = QDir::temp().absoluteFilePath(QUuid::createUuid().toString() + ".svg");

	// 创建缓冲区以保存SVG数据
	QByteArray svgData;
	QBuffer buffer(&svgData);
	buffer.open(QIODevice::WriteOnly);

	// 配置SVG生成器以绘制到缓冲区中
	QSvgGenerator svgGenerator;
	svgGenerator.setOutputDevice(&buffer);
	svgGenerator.setSize(QSize(512, 512));
	svgGenerator.setViewBox(QRect(0, 0, 512, 512));

	// 使用QPainter绘制预览到SVG生成器中
	QPainter painter;
	painter.begin(&svgGenerator);
	painter.drawPixmap(0, 0, symbol.generatePreview(512));
	painter.end();

	// 将SVG数据写入临时文件
	QFile tempSvgFile(tempSvgPath);
	if (!tempSvgFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QMessageBox::warning(nullptr, "错误", "无法创建临时SVG文件。");
		return;
	}
	tempSvgFile.write(svgData);
	tempSvgFile.close();

	// 使用SVG文件创建SVG符号层并设置大小，应用到符号层
	auto* svgLayer = new QgsSvgMarkerSymbolLayer(tempSvgPath);
	svgLayer->setSize(9.0);
	auto* markerSymbol = new QgsMarkerSymbol();
	markerSymbol->changeSymbolLayer(0, svgLayer);

	// 创建单一符号渲染器，应用到图层并触发重绘
	auto* renderer = new QgsSingleSymbolRenderer(markerSymbol);
	layer->setRenderer(renderer);
	layer->triggerRepaint();
}


// 点符号基础属性设置页面与参数获取
void LayerManager::setupPointSymbolizationUI(QVBoxLayout* layout, QgsVectorLayer* vectorLayer) {
	QGroupBox* pointGroup = new QGroupBox("点符号属性", nullptr);
	QFormLayout* pointLayout = new QFormLayout(pointGroup);

	// 设置大小
	QSpinBox* sizeSpinBox = new QSpinBox();
	sizeSpinBox->setRange(1, 100);
	sizeSpinBox->setValue(mPointSize);
	connect(sizeSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value) {
		mPointSize = value;
		});
	pointLayout->addRow("大小：", sizeSpinBox);

	// 设置颜色
	QPushButton* colorButton = new QPushButton("选择颜色");
	QColor currentColor = mPointColor;
	colorButton->setStyleSheet(QString("background-color: %1").arg(currentColor.name()));
	connect(colorButton, &QPushButton::clicked, this, [this, colorButton]() {
		QColor color = QColorDialog::getColor(mPointColor, nullptr, "选择颜色");
		if (color.isValid()) {
			mPointColor = color;
			colorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
		}
		});
	pointLayout->addRow("颜色：", colorButton);

	// 设置透明度
	QSlider* opacitySlider = new QSlider(Qt::Horizontal);
	opacitySlider->setRange(0, 255);
	opacitySlider->setValue(mPointOpacity);
	connect(opacitySlider, &QSlider::valueChanged, [&](int value) {
		mPointOpacity = value;
		});
	pointLayout->addRow("透明度：", opacitySlider);

	pointLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

	layout->addWidget(pointGroup);
}

// 线符号基础属性设置页面与参数获取
void LayerManager::setupLineSymbolizationUI(QVBoxLayout* layout, QgsVectorLayer* vectorLayer) {
	QGroupBox* lineGroup = new QGroupBox("线符号属性", nullptr);
	QFormLayout* lineLayout = new QFormLayout(lineGroup);

	// 设置宽度
	QSpinBox* widthSpinBox = new QSpinBox();
	widthSpinBox->setRange(1, 20);
	widthSpinBox->setValue(mLineWidth);
	connect(widthSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value) {
		mLineWidth = value;
		});
	lineLayout->addRow("宽度：", widthSpinBox);

	// 设置颜色
	QPushButton* colorButton = new QPushButton("选择颜色");
	QColor currentColor = mLineColor;
	colorButton->setStyleSheet(QString("background-color: %1").arg(currentColor.name()));
	connect(colorButton, &QPushButton::clicked, this, [this, colorButton]() {
		QColor color = QColorDialog::getColor(mLineColor, nullptr, "选择颜色");
		if (color.isValid()) {
			mLineColor = color;
			colorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
		}
		});
	lineLayout->addRow("颜色：", colorButton);

	// 设置透明度
	QSlider* opacitySlider = new QSlider(Qt::Horizontal);
	opacitySlider->setRange(0, 255);
	opacitySlider->setValue(mLineOpacity);
	connect(opacitySlider, &QSlider::valueChanged, [&](int value) {
		mLineOpacity = value;
		});
	lineLayout->addRow("透明度：", opacitySlider);

	lineLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

	layout->addWidget(lineGroup);
}

// 面符号基础属性设置页面与参数获取
void LayerManager::setupPolygonSymbolizationUI(QVBoxLayout* layout, QgsVectorLayer* vectorLayer) {
	QGroupBox* polygonGroup = new QGroupBox("面符号属性", nullptr);
	QFormLayout* polygonLayout = new QFormLayout(polygonGroup);

	// 设置填充颜色
	QPushButton* fillColorButton = new QPushButton("选择填充颜色");
	QColor currentColor = mPolygonColor;
	fillColorButton->setStyleSheet(QString("background-color: %1").arg(currentColor.name()));
	connect(fillColorButton, &QPushButton::clicked, this, [this, fillColorButton]() {
		QColor color = QColorDialog::getColor(mPolygonColor, nullptr, "选择颜色");
		if (color.isValid()) {
			mPolygonColor = color;
			fillColorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
		}
		});
	polygonLayout->addRow("填充颜色：", fillColorButton);

	// 设置透明度
	QSlider* opacitySlider = new QSlider(Qt::Horizontal);
	opacitySlider->setRange(0, 255);
	opacitySlider->setValue(mPolygonOpacity);
	connect(opacitySlider, &QSlider::valueChanged, [&](int value) {
		mPolygonOpacity = value;
		});
	polygonLayout->addRow("透明度：", opacitySlider);

	polygonLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

	layout->addWidget(polygonGroup);
}

// 基础属性符号化页面应用渲染
void LayerManager::applySymbolToLayerSimple(QgsVectorLayer* vectorLayer) {
	if (vectorLayer->geometryType() == Qgis::GeometryType::Point) {
		// 应用点符号
		QgsSimpleMarkerSymbolLayer* symbolLayer = new QgsSimpleMarkerSymbolLayer();
		QColor colorWithAlpha = mPointColor;
		colorWithAlpha.setAlpha(mPointOpacity);  // 设置颜色的透明度
		symbolLayer->setColor(colorWithAlpha);
		symbolLayer->setSize(mPointSize);

		QgsMarkerSymbol* markerSymbol = new QgsMarkerSymbol();
		markerSymbol->changeSymbolLayer(0, symbolLayer);

		QgsSingleSymbolRenderer* renderer = new QgsSingleSymbolRenderer(markerSymbol);
		vectorLayer->setRenderer(renderer);
	}
	else if (vectorLayer->geometryType() == Qgis::GeometryType::Line) {
		// 应用线符号
		QgsLineSymbol* lineSymbol = new QgsLineSymbol();
		lineSymbol->setColor(mLineColor);
		lineSymbol->setWidth(mLineWidth);
		lineSymbol->setOpacity(mLineOpacity / 255.0);  // 设置透明度

		QgsSingleSymbolRenderer* renderer = new QgsSingleSymbolRenderer(lineSymbol);
		vectorLayer->setRenderer(renderer);
	}
	else if (vectorLayer->geometryType() == Qgis::GeometryType::Polygon) {
		// 应用面符号
		QgsFillSymbol* fillSymbol = new QgsFillSymbol();
		fillSymbol->setColor(mPolygonColor);
		fillSymbol->setOpacity(mPolygonOpacity / 255.0);  // 设置透明度

		QgsSingleSymbolRenderer* renderer = new QgsSingleSymbolRenderer(fillSymbol);
		vectorLayer->setRenderer(renderer);
	}

	// 更新图层的渲染
	vectorLayer->triggerRepaint();
}

void LayerManager::showLayerLabels()
{
	// 获取当前选中的图层
	QgsMapLayer* layer = mpLayerTreeView->currentLayer();
	QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);

	if (!vectorLayer)
	{
		QMessageBox::warning(nullptr, tr("错误"), tr("仅支持矢量图层的注记功能。"));
		return;
	}

	// 创建注记设置对话框
	QDialog dialog;
	dialog.setWindowTitle(tr("设置注记"));
	dialog.resize(400, 350);

	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	// 字段选择
	QLabel* fieldLabel = new QLabel(tr("请选择注记字段："));
	QComboBox* fieldComboBox = new QComboBox(&dialog);
	const QgsFields& fields = vectorLayer->fields();
	for (const QgsField& field : fields)
	{
		fieldComboBox->addItem(field.name());
	}
	layout->addWidget(fieldLabel);
	layout->addWidget(fieldComboBox);

	QLabel* upfieldLabel = new QLabel(tr("请选择上标注记字段："));
	QComboBox* upfieldComboBox = new QComboBox(&dialog);
	const QgsFields& upfields = vectorLayer->fields();
	upfieldComboBox->addItem("None");
	for (const QgsField& field : upfields)
	{
		upfieldComboBox->addItem(field.name());
	}
	layout->addWidget(upfieldLabel);
	layout->addWidget(upfieldComboBox);

	QLabel* downfieldLabel = new QLabel(tr("请选择下标注记字段："));
	QComboBox* downfieldComboBox = new QComboBox(&dialog);
	const QgsFields& downfields = vectorLayer->fields();
	downfieldComboBox->addItem("None");
	for (const QgsField& field : downfields)
	{
		downfieldComboBox->addItem(field.name());
	}
	layout->addWidget(downfieldLabel);
	layout->addWidget(downfieldComboBox);

	// 字体大小
	QLabel* fontSizeLabel = new QLabel(tr("字体大小："));
	QSpinBox* fontSizeSpinBox = new QSpinBox(&dialog);
	fontSizeSpinBox->setRange(6, 48);
	fontSizeSpinBox->setValue(12);  // 默认字体大小
	layout->addWidget(fontSizeLabel);
	layout->addWidget(fontSizeSpinBox);

	// 字体颜色
	QLabel* fontColorLabel = new QLabel(tr("字体颜色："));
	QPushButton* fontColorButton = new QPushButton(tr("选择颜色"), &dialog);
	QColor fontColor = Qt::black;  // 默认字体颜色
	connect(fontColorButton, &QPushButton::clicked, [&]() {
		QColor color = QColorDialog::getColor(fontColor, &dialog, tr("选择字体颜色"));
		if (color.isValid()) {
			fontColor = color;
			fontColorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
		}
		});
	layout->addWidget(fontColorLabel);
	layout->addWidget(fontColorButton);

	// 是否使用缓冲区
	QLabel* bufferEnableLabel = new QLabel(tr("启用缓冲区："));
	QCheckBox* bufferEnableCheckBox = new QCheckBox(&dialog);
	bufferEnableCheckBox->setChecked(true);  // 默认启用
	layout->addWidget(bufferEnableLabel);
	layout->addWidget(bufferEnableCheckBox);

	// 缓冲区颜色
	QLabel* bufferColorLabel = new QLabel(tr("缓冲区颜色："));
	QPushButton* bufferColorButton = new QPushButton(tr("选择颜色"), &dialog);
	QColor bufferColor = Qt::white;  // 默认缓冲区颜色
	connect(bufferColorButton, &QPushButton::clicked, [&]() {
		QColor color = QColorDialog::getColor(bufferColor, &dialog, tr("选择缓冲区颜色"));
		if (color.isValid()) {
			bufferColor = color;
			bufferColorButton->setStyleSheet(QString("background-color: %1").arg(color.name()));
		}
		});
	layout->addWidget(bufferColorLabel);
	layout->addWidget(bufferColorButton);

	// 缓冲区大小
	QLabel* bufferSizeLabel = new QLabel(tr("缓冲区大小："));
	QDoubleSpinBox* bufferSizeSpinBox = new QDoubleSpinBox(&dialog);
	bufferSizeSpinBox->setRange(0.0, 10.0);
	bufferSizeSpinBox->setValue(1.0);  // 默认缓冲区大小
	layout->addWidget(bufferSizeLabel);
	layout->addWidget(bufferSizeSpinBox);

	// 按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
	QPushButton* hideButton = new QPushButton(tr("隐藏注记"), &dialog); // 添加隐藏按钮
	buttonBox->addButton(hideButton, QDialogButtonBox::ActionRole);
	layout->addWidget(buttonBox);

	// 隐藏注记功能
	connect(hideButton, &QPushButton::clicked, [&]() {
		vectorLayer->setLabelsEnabled(false); // 禁用注记
		vectorLayer->triggerRepaint();
		});

	// 临时应用
	connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, [&]() {
		applyLayerLabels(vectorLayer, upfieldComboBox->currentText(), fieldComboBox->currentText(), downfieldComboBox->currentText(), fontSizeSpinBox->value(), fontColor,
		bufferEnableCheckBox->isChecked(), bufferColor, bufferSizeSpinBox->value(), /*isTemporary=*/true);
		});

	// 确认应用
	connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, [&]() {
		applyLayerLabels(vectorLayer, upfieldComboBox->currentText(), fieldComboBox->currentText(), downfieldComboBox->currentText(), fontSizeSpinBox->value(), fontColor,
		bufferEnableCheckBox->isChecked(), bufferColor, bufferSizeSpinBox->value(), /*isTemporary=*/false);
	dialog.accept();
		});

	// 取消操作
	connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [&]() {
		dialog.reject();
		});

	dialog.exec();
}

void LayerManager::applyLayerLabels(QgsVectorLayer* vectorLayer, const QString& upFieldName, const QString& mainFieldName, const QString& downFieldName, int fontSize, QColor fontColor,
	bool useBuffer, QColor bufferColor, double bufferSize, bool isTemporary)
{

	qDebug() << "applyLayerLabels";
	qDebug() << upFieldName << mainFieldName << downFieldName;
	QgsPalLayerSettings settings;

	// 使用 HTML 标签和 CSS 样式来定义上标和下标，并使它们在同一侧显示
	QString labelExpression = "concat( \"" + mainFieldName + "\"";

	if (upFieldName != "None") {
		labelExpression += ", '<span style=\"font-size: 0.7em; vertical-align: super; position: relative; top: -0.5em;\">' || \"" + upFieldName + "\" || '</span>'";
	}

	if (downFieldName != "None") {
		if (upFieldName != "None") {
			labelExpression += " || '<br>'";
		}
		labelExpression += " || '<span style=\"font-size: 0.7em; vertical-align: sub; position: relative; bottom: -0.5em;\">' || \"" + downFieldName + "\" || '</span>'";
	}

	labelExpression += ")";
	settings.fieldName = labelExpression;
	settings.isExpression = true; // 如果是表达式则设置为 true
	settings.isExpression = true; // 如果是表达式则设置为 true
	settings.drawLabels = true; // 启用标签绘制
	settings.placement = Qgis::LabelPlacement::AroundPoint; // 标签放置方式
	settings.centroidWhole = false; // 是否从整个要素计算质心
	settings.centroidInside = false; // 质心是否必须在多边形内部
	settings.dist = 0.0; // 距离特征的距离
	settings.distUnits = Qgis::RenderUnit::Millimeters; // 距离单位
	settings.quadOffset = Qgis::LabelQuadrantPosition::Over; // 四分之一位置
	settings.xOffset = 0.0; // 水平偏移
	settings.yOffset = 0.0; // 垂直偏移
	settings.offsetUnits = Qgis::RenderUnit::Millimeters; // 偏移单位
	settings.angleOffset = 0.0; // 旋转角度
	settings.priority = 5; // 标签优先级
	settings.scaleVisibility = true; // 启用缩放可见性
	settings.minimumScale = 0.0; // 最小比例尺
	settings.maximumScale = 100000.0; // 最大比例尺

	// 设置标签格式
	QgsTextFormat format;
	format.setSize(fontSize); // 字体大小
	format.setColor(fontColor); // 字体颜色
	format.setOpacity(1.0); // 透明度
	format.setFont(QFont("Arial", fontSize, QFont::Bold)); // 字体样式
	format.setAllowHtmlFormatting(true); // 允许 HTML 格式
	format.setPreviewBackgroundColor(Qt::white); // 预览背景颜色

	// 设置缓冲区
	if (useBuffer) {
		QgsTextBufferSettings buffer;
		buffer.setEnabled(true);
		buffer.setSize(bufferSize);
		buffer.setColor(bufferColor);
		format.setBuffer(buffer);
	}

	// 设置标签文本
	settings.setFormat(format);

	// 创建简单的标签提供者
	QgsVectorLayerSimpleLabeling* labeling = new QgsVectorLayerSimpleLabeling(settings);

	// 设置图层的标签提供者
	vectorLayer->setLabeling(labeling);
	vectorLayer->setLabelsEnabled(true); // 启用标签

	// 应用标签设置
	vectorLayer->triggerRepaint();
}
