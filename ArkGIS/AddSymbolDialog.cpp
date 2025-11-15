#include "AddSymbolDialog.h"
#include "SymbolLibraryManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QBuffer>
#include <QToolBar>
#include <QFile>
#include <QDir>
#include <QUuid>
#include <QSvgRenderer>

AddSymbolDialog::AddSymbolDialog(QWidget* parent)
	: QDialog(parent), scene(new CustomGraphicsScene(this)) {
	setupUI();
}

void AddSymbolDialog::setupUI() {
	setWindowTitle("添加符号");
	resize(1000, 600);

	QHBoxLayout* mainLayout = new QHBoxLayout(this);

	// 符号库选择、符号名称输入、工具栏和绘图区
	QVBoxLayout* leftLayout = new QVBoxLayout();

	// 符号库选择与符号名称输入
	QHBoxLayout* nameLayout = new QHBoxLayout();
	nameLayout->addWidget(new QLabel("符号名称:"));
	nameEdit = new QLineEdit(this);
	nameEdit->setMinimumWidth(150);
	nameLayout->addWidget(nameEdit);
	leftLayout->addLayout(nameLayout);

	QHBoxLayout* libraryLayout = new QHBoxLayout();
	libraryLayout->addWidget(new QLabel("选择符号库:"));
	libraryComboBox = new QComboBox(this);
	libraryComboBox->setMinimumWidth(150);
	loadAvailableLibraries();  // 加载符号库
	libraryLayout->addWidget(libraryComboBox);
	leftLayout->addLayout(libraryLayout);

	// 选择绘制模式
	QToolBar* toolbar = new QToolBar(this);
	toolbar->addAction("绘制圆", [this]() { scene->setDrawMode(DrawMode::Circle); });
	toolbar->addAction("绘制正方形", [this]() { scene->setDrawMode(DrawMode::Square); });
	toolbar->addAction("绘制线条", [this]() { scene->setDrawMode(DrawMode::Line); });
	toolbar->addAction("自由画笔", [this]() { scene->setDrawMode(DrawMode::FreeDraw); });
	leftLayout->addWidget(toolbar);

	// 绘图区
	QGraphicsView* view = new QGraphicsView(scene, this);
	view->setFixedSize(500, 500);  // 固定大小的正方形
	view->setAlignment(Qt::AlignCenter);  // 居中对齐

	view->setDragMode(QGraphicsView::NoDrag);  // 禁止拖动
	leftLayout->addWidget(view, 0, Qt::AlignCenter);  // 绘图区居中显示

	// 添加和删除图层按钮
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	QPushButton* addLayerButton = new QPushButton("添加图层", this);
	QPushButton* removeLayerButton = new QPushButton("删除图层", this);
	connect(addLayerButton, &QPushButton::clicked, this, &AddSymbolDialog::addLayer);
	connect(removeLayerButton, &QPushButton::clicked, this, &AddSymbolDialog::removeLayer);
	buttonLayout->addWidget(addLayerButton);
	buttonLayout->addWidget(removeLayerButton);
	leftLayout->addLayout(buttonLayout);

	// 将左侧布局添加到主布局
	mainLayout->addLayout(leftLayout);

	// 右侧：图层树、符号预览和图层预览
	QVBoxLayout* rightLayout = new QVBoxLayout();

	// 图层树
	layerTree = new QTreeWidget(this);
	layerTree->setHeaderLabels({ "图层名", "描述" });
	layerTree->header()->setSectionResizeMode(QHeaderView::Stretch);
	layerTree->setMinimumHeight(300);
	connect(layerTree, &QTreeWidget::itemClicked, this, &AddSymbolDialog::onLayerSelected);
	rightLayout->addWidget(layerTree);

	// 符号总预览与单图层预览区域
	QHBoxLayout* previewLayout = new QHBoxLayout();

	// 符号总预览
	QVBoxLayout* symbolPreviewLayout = new QVBoxLayout();
	QLabel* symbolPreviewTitle = new QLabel("符号总预览", this);
	symbolPreviewLayout->addWidget(symbolPreviewTitle, 0, Qt::AlignCenter);

	symbolPreviewLabel = new QLabel(this);
	symbolPreviewLabel->setFixedSize(200, 200);
	symbolPreviewLabel->setStyleSheet("background-color: white; border: 1px solid black;");
	symbolPreviewLayout->addWidget(symbolPreviewLabel, 0, Qt::AlignCenter);
	previewLayout->addLayout(symbolPreviewLayout);

	// 单图层预览
	QVBoxLayout* layerPreviewLayout = new QVBoxLayout();
	QLabel* layerPreviewTitle = new QLabel("单图层预览", this);
	layerPreviewLayout->addWidget(layerPreviewTitle, 0, Qt::AlignCenter);

	layerPreviewLabel = new QLabel(this);
	layerPreviewLabel->setFixedSize(200, 200);
	layerPreviewLabel->setStyleSheet("background-color: white; border: 1px solid black;");
	layerPreviewLayout->addWidget(layerPreviewLabel, 0, Qt::AlignCenter);
	previewLayout->addLayout(layerPreviewLayout);

	rightLayout->addLayout(previewLayout);

	// 确认按钮
	QPushButton* okButton = new QPushButton("确定", this);
	connect(okButton, &QPushButton::clicked, this, &AddSymbolDialog::onOkButtonClicked);
	rightLayout->addWidget(okButton);

	// 将右侧布局添加到主布局
	mainLayout->addLayout(rightLayout);

	updatePreview();
}


// 添加新图层
void AddSymbolDialog::addLayer() {
	// 检查绘图区内容
	if (scene->items().isEmpty()) {
		QMessageBox::warning(this, "警告", "请先在绘图区绘制内容。");
		return;
	}

	// 创建临时SVG路径
	QString tempSvgPath = QDir::temp().absoluteFilePath(QUuid::createUuid().toString() + ".svg");

	// 生成SVG数据
	QByteArray svgData;
	QBuffer buffer(&svgData);
	buffer.open(QIODevice::WriteOnly);

	QSvgGenerator svgGenerator;
	svgGenerator.setOutputDevice(&buffer);
	svgGenerator.setSize(QSize(512, 512));
	svgGenerator.setViewBox(QRect(0, 0, 512, 512));

	QPainter painter;
	painter.begin(&svgGenerator);
	scene->render(&painter);
	painter.end();

	// 创建新图层
	SymbolLayer newLayer(SymbolType::Marker, SymbolClass::SvgMarker);
	newLayer.svgData = svgData;
	symbol.layers.append(newLayer);

	// 将新图层添加到树状图
	QTreeWidgetItem* item = new QTreeWidgetItem(layerTree);
	item->setText(0, QString("图层 %1").arg(symbol.layers.size()));
	item->setText(1, "SvgMarker");
	layerTree->addTopLevelItem(item);

	// 清空绘图区并更新符号总预览
	scene->clear();
	updatePreview();
}

// 当用户点击树状图中的图层时，更新单图层预览
void AddSymbolDialog::onLayerSelected(QTreeWidgetItem* item, int column) {
	int layerIndex = layerTree->indexOfTopLevelItem(item);
	if (layerIndex < 0 || layerIndex >= symbol.layers.size()) return;

	// 获取选中的图层并生成预览
	const SymbolLayer& layer = symbol.layers[layerIndex];

	QPixmap layerPreview(layerPreviewLabel->width(), layerPreviewLabel->height());
	layerPreview.fill(Qt::transparent);

	QPainter painter(&layerPreview);
	painter.setRenderHint(QPainter::Antialiasing);

	// 渲染SVG数据
	QSvgRenderer svgRenderer(layer.svgData);
	svgRenderer.render(&painter, layerPreview.rect());

	layerPreviewLabel->setPixmap(layerPreview);
}

// 移除符号图层
void AddSymbolDialog::removeLayer() {
	QTreeWidgetItem* currentItem = layerTree->currentItem();
	if (!currentItem) return;

	int index = layerTree->indexOfTopLevelItem(currentItem);
	if (index >= 0) {
		delete currentItem;
		symbol.layers.removeAt(index);  // 从符号中删除该图层
		updatePreview();
	}
}

// 更新预览
void AddSymbolDialog::updatePreview() {
	QPixmap preview = symbol.generatePreview(symbolPreviewLabel->width());
	symbolPreviewLabel->setPixmap(preview);
}

QString AddSymbolDialog::symbolName() const {
	return nameEdit->text();
}

Symbol AddSymbolDialog::getSymbol() const {
	return symbol;
}

// 加载可用符号库到下拉框
void AddSymbolDialog::loadAvailableLibraries() {
	QList<QString> libraries = SymbolLibraryManager::instance().getAvailableLibraries();
	for (const QString& library : libraries) {
		libraryComboBox->addItem(library);
	}
}

// 确定按钮的槽函数：保存符号到选定的符号库
void AddSymbolDialog::onOkButtonClicked() {
	QString symbolName = nameEdit->text();
	if (symbolName.isEmpty()) {
		QMessageBox::warning(this, "错误", "符号名称不能为空！");
		return;
	}

	QString selectedLibrary = libraryComboBox->currentText();
	if (selectedLibrary.isEmpty()) {
		QMessageBox::warning(this, "错误", "请选择符号库！");
		return;
	}

	// 设置符号的名称
	symbol.name = symbolName;

	// 添加符号到选中的符号库
	if (SymbolLibraryManager::instance().addSymbolToLibrary(selectedLibrary, symbol)) {
		QMessageBox::information(this, "成功", "符号已添加到符号库！");

		emit symbolAdded();  // 发射符号已添加的信号

		accept();  // 关闭对话框
	}
	else {
		QMessageBox::warning(this, "错误", "添加符号失败，符号可能已存在。");
	}

}

