#include "SymbolManager.h"
#include "AddSymbolDialog.h"
#include "CustomGraphicsScene.h"
#include "SymbolLibraryManager.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDomDocument>
#include <QStandardItem>
#include <QTextStream>
#include <QDebug>
#include <QComboBox>
#include <QMenu>
#include <QgsStyle.h>
#include <QToolBar>
#include <QGraphicsView>
#include <QSvgGenerator>
#include <QInputDialog>
#include <QBuffer>
#include <qgsmarkersymbollayer.h>
#include <QGroupBox>
#include <QSplitter>

SymbolManager::SymbolManager(QWidget* parent) : QDialog(parent) {
	setupUI();
}

void SymbolManager::setupUI() {
	setWindowTitle("符号管理器");
	resize(1000, 700);  // 窗口尺寸

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	// 顶部工具栏：符号库选择、导入/导出、新建库
	QToolBar* toolBar = new QToolBar(this);
	toolBar->setMovable(false);

	libraryComboBox = new QComboBox(this);
	libraryComboBox->setMinimumWidth(200);

	toolBar->addWidget(new QLabel("选择符号库: ", this));
	toolBar->addWidget(libraryComboBox);
	toolBar->addSeparator();
	toolBar->addAction("导入符号库", this, &SymbolManager::importSymbols);
	toolBar->addAction("导出符号库", this, &SymbolManager::exportSymbols);
	toolBar->addAction("新建符号库", this, &SymbolManager::createNewSymbolDatabase);
	mainLayout->addWidget(toolBar);

	// 搜索框：用于快速查找符号
	QHBoxLayout* searchLayout = new QHBoxLayout();
	QLineEdit* searchBox = new QLineEdit(this);
	searchBox->setPlaceholderText("搜索符号...");
	searchLayout->addWidget(new QLabel("搜索:"));
	searchLayout->addWidget(searchBox);
	mainLayout->addLayout(searchLayout);

	// 左侧符号列表
	symbolModel = new QStandardItemModel(this);
	symbolView = new QListView(this);
	symbolView->setModel(symbolModel);
	symbolView->setViewMode(QListView::IconMode);
	symbolView->setSpacing(10);
	symbolView->setIconSize(QSize(64, 64));
	symbolView->setGridSize(QSize(100, 120));
	symbolView->setResizeMode(QListView::Adjust);
	symbolView->setWordWrap(false);
	symbolView->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(symbolView, &QListView::customContextMenuRequested,
		this, &SymbolManager::onCustomContextMenuRequested);

	// 符号预览区域（带详细信息）
	QLabel* previewLabel = new QLabel("符号预览", this);
	previewLabel->setAlignment(Qt::AlignCenter);

	QPixmap defaultPreview(128, 128);
	defaultPreview.fill(Qt::lightGray);

	// 使用 QGraphicsView 保证预览的符号居中并按比例显示
	QGraphicsScene* previewScene = new QGraphicsScene(this);
	QGraphicsView* previewView = new QGraphicsView(previewScene, this);
	previewView->setFixedSize(150, 150);
	previewView->setAlignment(Qt::AlignCenter);
	previewView->setStyleSheet("border: 1px solid gray;");

	// 符号详细信息显示
	QLabel* symbolInfo = new QLabel("符号信息：", this);
	symbolInfo->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	symbolInfo->setWordWrap(true);

	QWidget* previewWidget = new QWidget(this);
	QVBoxLayout* previewLayout = new QVBoxLayout(previewWidget);
	previewLayout->addWidget(previewLabel);
	previewLayout->addWidget(previewView, 0, Qt::AlignCenter);  // 居中符号预览
	previewLayout->addWidget(symbolInfo);

	// 布局：符号列表和符号预览并排显示
	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->addWidget(symbolView);
	splitter->addWidget(previewWidget);
	mainLayout->addWidget(splitter);

	// 底部按钮：添加符号、添加自定义符号
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	QPushButton* addButton = new QPushButton("添加符号", this);
	buttonLayout->addWidget(addButton);
	mainLayout->addLayout(buttonLayout);

	// 连接按钮信号槽
	connect(addButton, &QPushButton::clicked, this, &SymbolManager::addSymbol);

	// 加载符号库列表并设置默认符号库
	loadAvailableLibraries();
	loadDefaultLibrary();

	// 搜索框过滤符号列表
	connect(searchBox, &QLineEdit::textChanged, this, [=](const QString& text) {
		for (int i = 0; i < symbolModel->rowCount(); ++i) {
			QStandardItem* item = symbolModel->item(i);
			bool match = item->text().contains(text, Qt::CaseInsensitive);
			symbolView->setRowHidden(i, !match);  // 隐藏不匹配项
		}
		});

	// 切换符号库时更新符号列表
	connect(libraryComboBox, &QComboBox::currentTextChanged, this, &SymbolManager::onLibraryChanged);

	// 符号选择时更新预览和信息
	connect(symbolView->selectionModel(), &QItemSelectionModel::currentChanged,
		this, [=](const QModelIndex& index) {
			if (index.isValid()) {
				const auto& symbols =
					SymbolLibraryManager::instance().getCurrentLibrary().getSymbols();
				if (index.row() < symbols.size()) {
					const Symbol& symbol = symbols.at(index.row());

					// 更新符号预览
					previewScene->clear();  // 清空上一个符号的预览
					QPixmap preview = symbol.generatePreview(128);
					previewScene->addPixmap(preview);

					// 显示符号详细信息
					QString info = QString(
						"名称: %1\n"
						"层数: %2\n"
						"透明度: %3\n"
						"标签: %4")
						.arg(symbol.name)
						.arg(symbol.layers.size())
						.arg(symbol.alpha)
						.arg(symbol.tags);
					symbolInfo->setText(info);
				}
			}
		});
}

// 加载默认符号库并展示
void SymbolManager::loadDefaultLibrary() {
	// 获取默认符号库路径
	QString defaultLibraryPath = "./symbols/default.xml";

	if (SymbolLibraryManager::instance().loadLibrary(defaultLibraryPath)) {
		libraryComboBox->setCurrentText(defaultLibraryPath);  // 设置默认符号库为选中项
		updateSymbolList();  // 更新符号列表
	}
	else {
		QMessageBox::warning(this, "错误", "无法加载默认符号库：" + defaultLibraryPath);
	}
}

// 更新符号列表视图
void SymbolManager::updateSymbolList() {
	symbolModel->clear();  // 清空之前的符号显示

	// 获取当前符号库中的符号
	const auto& symbols = SymbolLibraryManager::instance().getCurrentLibrary().getSymbols();

	for (const Symbol& symbol : symbols) {
		QStandardItem* item = new QStandardItem(symbol.name);  // 创建符号项
		item->setIcon(QIcon(symbol.generatePreview(32)));      // 设置符号预览图标
		symbolModel->appendRow(item);  // 添加符号到模型
	}

	// 如果符号库为空，显示占位提示
	if (symbols.isEmpty()) {
		QStandardItem* emptyItem = new QStandardItem("没有符号可显示");
		symbolModel->appendRow(emptyItem);
	}

	// 刷新符号视图
	symbolView->reset();
}

// 导入符号库
void SymbolManager::importSymbols() {
	// 打开文件选择对话框，让用户选择要导入的 XML 文件
	QString filePath = QFileDialog::getOpenFileName(this, "导入符号库", "", "XML 文件 (*.xml)");
	if (filePath.isEmpty()) return;

	// 加载符号库
	if (SymbolLibraryManager::instance().loadLibrary(filePath)) {
		// 将导入的符号库路径添加到符号库管理器
		SymbolLibraryManager::instance().importLibrary(filePath);

		// 添加新符号库路径到下拉框中
		libraryComboBox->addItem(filePath);

		// 设置新导入的库为当前选中库，并更新符号列表显示
		libraryComboBox->setCurrentText(filePath);
		updateSymbolList();

		QMessageBox::information(this, "导入成功", "符号库已成功导入并显示。");
	}
	else {
		QMessageBox::warning(this, "导入失败", "无法加载符号库：" + filePath);
	}
}

// 导出符号库
void SymbolManager::exportSymbols() {
	// 选择保存路径
	QString filePath = QFileDialog::getSaveFileName(this, "导出符号库", "", "XML 文件 (*.xml)");
	if (filePath.isEmpty()) return;

	// 获取当前符号库中的所有符号
	const auto& symbols = SymbolLibraryManager::instance().getCurrentLibrary().getSymbols();

	// 创建 XML 文档
	QDomDocument doc;
	QDomElement root = doc.createElement("qgis_style");  // 根节点
	doc.appendChild(root);

	// 将符号导出为 XML
	for (const Symbol& symbol : symbols) {
		symbol.exportToXML(doc, root);  // 将符号添加到 XML 结构中
	}

	// 打开文件进行写入
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		QMessageBox::warning(this, "错误", "无法写入 XML 文件");
		return;
	}

	// 写入 XML 数据到文件
	QTextStream stream(&file);
	stream << doc.toString();

	QMessageBox::information(this, "导出成功", "符号库已成功导出为 XML。");
}

// 添加符号
void SymbolManager::addSymbol() {
	AddSymbolDialog dialog(this);

	// 连接信号：符号添加成功后刷新符号列表
	connect(&dialog, &AddSymbolDialog::symbolAdded, this, &SymbolManager::updateSymbolList);


	if (dialog.exec() == QDialog::Accepted) {
		Symbol newSymbol = dialog.getSymbol();
		newSymbol.name = dialog.symbolName();
		symbols.append(newSymbol);
		updateSymbolList();
	}
}

// 新建符号数据库
void SymbolManager::createNewSymbolDatabase() {
	QString filePath = QFileDialog::getSaveFileName(nullptr, "新建符号数据库", "", "XML 文件 (*.xml)");
	if (filePath.isEmpty()) return;

	SymbolLibrary& library = SymbolLibraryManager::instance().getCurrentLibrary();
	library.exportToXML(filePath);
	QMessageBox::information(nullptr, "成功", "符号数据库已创建。");
}

// 切换符号库目录
void SymbolManager::switchSymbolLibrary() {
	QString dir = QFileDialog::getExistingDirectory(nullptr, "选择符号库目录");
	if (dir.isEmpty()) return;

	SymbolLibraryManager::instance().setLibraryDirectory(dir);

	QList<QString> libraries = SymbolLibraryManager::instance().getAvailableLibraries();
	if (libraries.isEmpty()) {
		QMessageBox::information(nullptr, "信息", "未找到任何符号库。");
	}
	else {
		QMessageBox::information(nullptr, "信息", QString("找到 %1 个符号库。").arg(libraries.size()));
	}
}

// 加载当前目录中的符号库列表到下拉框
void SymbolManager::loadAvailableLibraries() {
	libraryComboBox->clear();

	QList<QString> libraries = SymbolLibraryManager::instance().getAvailableLibraries();
	for (const QString& library : libraries) {
		libraryComboBox->addItem(library);
	}
}

// 当符号库切换时，加载新符号库并更新符号列表
void SymbolManager::onLibraryChanged(const QString& libraryPath) {
	if (SymbolLibraryManager::instance().loadLibrary(libraryPath)) {
		updateSymbolList();  // 加载符号库后更新符号列表
	}
	else {
		QMessageBox::warning(this, "错误", "无法加载符号库：" + libraryPath);
	}
}

// 符号右键菜单
void SymbolManager::onCustomContextMenuRequested(const QPoint& pos) {
	QModelIndex index = symbolView->indexAt(pos);
	if (!index.isValid()) return;  // 如果没有选中符号，直接返回

	QMenu contextMenu(this);
	QAction* deleteAction = contextMenu.addAction("删除符号");
	QAction* exportPNGAction = contextMenu.addAction("导出为 PNG");
	QAction* exportSVGAction = contextMenu.addAction("导出为 SVG");

	// 连接菜单项的槽函数
	connect(deleteAction, &QAction::triggered, this, &SymbolManager::deleteSelectedSymbol);
	connect(exportPNGAction, &QAction::triggered, this, &SymbolManager::exportSymbolAsPNG);
	connect(exportSVGAction, &QAction::triggered, this, &SymbolManager::exportSymbolAsSVG);

	// 在鼠标位置弹出右键菜单
	contextMenu.exec(symbolView->viewport()->mapToGlobal(pos));
}

// 获取选中的符号
Symbol SymbolManager::getSelectedSymbol() {
	QModelIndex index = symbolView->currentIndex();
	QString symbolName = symbolModel->item(index.row())->text();
	return SymbolLibraryManager::instance().getCurrentLibrary().getSymbolByName(symbolName);
}

// 删除选中的符号
void SymbolManager::deleteSelectedSymbol() {
	Symbol symbol = getSelectedSymbol();  // 获取当前选中的符号

	// 调用符号库的删除函数
	if (SymbolLibraryManager::instance().getCurrentLibrary().removeSymbol(symbol)) {
		QMessageBox::information(this, "删除成功", "符号已删除并从符号库中移除。");
		updateSymbolList();  // 更新符号列表
	}
	else {
		QMessageBox::warning(this, "删除失败", "符号未找到或删除失败。");
	}
}

// 导出符号为PNG文件
void SymbolManager::exportSymbolAsPNG() {
	Symbol symbol = getSelectedSymbol();
	QString filePath = QFileDialog::getSaveFileName(this, "导出为 PNG", "", "PNG 文件 (*.png)");
	if (filePath.isEmpty()) return;

	QPixmap pixmap = symbol.generatePreview(128);
	if (pixmap.save(filePath, "PNG")) {
		QMessageBox::information(this, "导出成功", "符号已成功导出为 PNG");
	}
	else {
		QMessageBox::warning(this, "导出失败", "无法导出符号为 PNG");
	}
}

// 导出符号为Svg文件
void SymbolManager::exportSymbolAsSVG() {
	Symbol symbol = getSelectedSymbol();
	QString filePath = QFileDialog::getSaveFileName(this, "导出为 SVG", "", "SVG 文件 (*.svg)");
	if (filePath.isEmpty()) return;

	if (symbol.exportToSVG(filePath)) {
		QMessageBox::information(this, "导出成功", "符号已成功导出为 SVG");
	}
	else {
		QMessageBox::warning(this, "导出失败", "无法导出符号为 SVG");
	}
}