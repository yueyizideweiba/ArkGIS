#include "CustomDialog.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QgsFeatureIterator.h>
#include <QgsField.h>

CustomDialog::CustomDialog(QgsLayerTreeView* layerTreeView, QgsVectorLayer* vectorLayer, QWidget* parent)
	: QDialog(parent), m_layerTreeView(layerTreeView), m_vectorLayer(vectorLayer), m_tableWidget(nullptr), m_editButton(nullptr), m_addColumnButton(nullptr), m_saveButton(nullptr), m_hasUnsavedChanges(false) {
	initializeUI();
}

CustomDialog::~CustomDialog() {

}

void CustomDialog::initializeUI() {
	setWindowTitle(tr("属性表 - %1").arg(m_vectorLayer->name()));
	resize(800, 600);

	QVBoxLayout* layout = new QVBoxLayout(this);

	// 创建 TabWidget
	QTabWidget* tabWidget = new QTabWidget(this);

	// 属性表标签页
	QWidget* attributeTablePage = new QWidget(this);
	QVBoxLayout* attributeTableLayout = new QVBoxLayout(attributeTablePage);
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	buttonLayout->setAlignment(Qt::AlignLeft);
	buttonLayout->setSpacing(5);

	m_editButton = new QToolButton(this);
	m_editButton->setIcon(QIcon("./icons/editattri.png"));
	m_editButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

	m_addColumnButton = new QToolButton(this);
	m_addColumnButton->setIcon(QIcon("./icons/addcol.png"));
	m_addColumnButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	m_addColumnButton->setEnabled(false);

	m_saveButton = new QToolButton(this);
	m_saveButton->setIcon(QIcon("./icons/saveedit.png"));
	m_saveButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	m_saveButton->setEnabled(false);

	buttonLayout->addWidget(m_editButton);
	buttonLayout->addWidget(m_addColumnButton);
	buttonLayout->addWidget(m_saveButton);
	attributeTableLayout->addLayout(buttonLayout);

	m_tableWidget = new QTableWidget(this);
	m_tableWidget->setColumnCount(m_vectorLayer->fields().count());
	QStringList headers;
	for (const QgsField& field : m_vectorLayer->fields()) {
		headers << field.name();
	}
	m_tableWidget->setHorizontalHeaderLabels(headers);
	populateTable();
	attributeTableLayout->addWidget(m_tableWidget);

	tabWidget->addTab(attributeTablePage, "属性表");

	// 字段编辑标签页
	QWidget* fieldEditorPage = new QWidget(this);
	QVBoxLayout* fieldEditorLayout = new QVBoxLayout(fieldEditorPage);

	QTableWidget* fieldTableWidget = new QTableWidget(this);
	fieldTableWidget->setColumnCount(3); // 字段名、类型、长度
	fieldTableWidget->setHorizontalHeaderLabels({ "字段名", "字段类型", "长度" });

	// 填充字段结构信息
	populateFieldTable(fieldTableWidget);

	QPushButton* saveFieldsButton = new QPushButton("保存字段修改", this);
	connect(saveFieldsButton, &QPushButton::clicked, [this, fieldTableWidget]() {
		saveFieldChanges(fieldTableWidget);
		});

	fieldEditorLayout->addWidget(fieldTableWidget);
	fieldEditorLayout->addWidget(saveFieldsButton);

	tabWidget->addTab(fieldEditorPage, "字段编辑");

	layout->addWidget(tabWidget);

	connect(m_editButton, &QToolButton::clicked, this, &CustomDialog::enableEditing);
	connect(m_saveButton, &QToolButton::clicked, this, &CustomDialog::saveChanges);
	connect(m_tableWidget, &QWidget::customContextMenuRequested, this, &CustomDialog::handleCustomContextMenu);
	connect(m_addColumnButton, &QToolButton::clicked, this, &CustomDialog::addNewColumn);
	connect(m_tableWidget, &QTableWidget::itemChanged, this, &CustomDialog::handleItemChanged);
}

// 填充表格数据
void CustomDialog::populateTable() {
	// 获取图层的特征迭代器
	QgsFeatureIterator it = m_vectorLayer->getFeatures();
	QgsFeature feature;
	int row = 0;
	// 遍历所有特征
	while (it.nextFeature(feature)) {
		// 在表格中插入新行
		m_tableWidget->insertRow(row);
		for (int col = 0; col < feature.fields().count(); ++col) {
			// 获取特征的属性值，并转换为字符串
			QString value = QString::fromUtf8(feature.attribute(col).toString().toUtf8());
			// 创建新的表格项
			QTableWidgetItem* item = new QTableWidgetItem(value);
			// 将特征ID存储在表格项的用户角色数据中
			item->setData(Qt::UserRole, feature.id());
			// 将表格项添加到对应的行和列
			m_tableWidget->setItem(row, col, item);
		}
		++row;
	}
}

// 保存更改
void CustomDialog::saveChanges() {
	// 如果有未保存的更改
	if (m_hasUnsavedChanges) {
		// 禁用添加列和保存按钮
		m_addColumnButton->setEnabled(false);
		m_saveButton->setEnabled(false);
		QMessageBox::information(this, "成功", "更改已保存！");
		// 重置未保存更改的标志
		m_hasUnsavedChanges = false;
	}
	else {
		// 提交图层的更改
		m_vectorLayer->commitChanges();
		m_addColumnButton->setEnabled(false);
		m_saveButton->setEnabled(false);
		QMessageBox::warning(this, "错误", "未进行任何修改");
	}
}

// 启用编辑功能
void CustomDialog::enableEditing() {
	// 开始编辑图层
	m_vectorLayer->startEditing();
	// 设置表格的编辑触发方式
	m_tableWidget->setEditTriggers(QTableWidget::DoubleClicked);
	m_tableWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
	// 设置表格的上下文菜单策略
	m_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	// 遍历表格的所有项，设置为可编辑
	for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
		for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
			QTableWidgetItem* item = m_tableWidget->item(row, col);
			if (item) {
				item->setFlags(item->flags() | Qt::ItemIsEditable);
			}
		}
	}
	// 启用添加列和保存按钮
	m_addColumnButton->setEnabled(true);
	m_saveButton->setEnabled(true);
	// 设置未保存更改的标志
	m_hasUnsavedChanges = true;
}

// 表格项双击事件
void CustomDialog::handleItemDoubleClicked(QTableWidgetItem* item) {
	// 如果编辑按钮可用，则编辑当前项
	if (!m_editButton->isEnabled()) return;
	m_tableWidget->editItem(item);
}

// 处理表格项更改事件
void CustomDialog::handleItemChanged(QTableWidgetItem* item) {
	// 如果编辑按钮不可用，则不处理更改
	if (!m_editButton->isEnabled()) return;
	// 获取行和列索引
	int row = item->row();
	int column = item->column();
	// 获取新的值
	QString newValue = item->text();
	// 获取列的字段类型
	QgsField field = m_vectorLayer->fields().at(column);
	// 验证数据类型是否匹配
	if (!validateDataType(field.type(), newValue)) {
		// 如果不匹配，设置背景色为红色，并显示警告
		item->setBackground(Qt::red);
		QMessageBox::warning(this, "错误", "修改的值与列的数据类型不匹配。");
		// 恢复原来的值
		m_tableWidget->blockSignals(true);
		item->setText(item->data(Qt::UserRole).toString());
		m_tableWidget->blockSignals(false);
		// 设置未保存更改的标志
		m_hasUnsavedChanges = true;
	}
	else {
		// 如果匹配，设置背景色为白色
		item->setBackground(Qt::white);
		QgsFeature feature;
		QgsFeatureId featureId = item->data(Qt::UserRole).toLongLong();
		if (m_vectorLayer->getFeatures(QgsFeatureRequest(featureId)).nextFeature(feature)) {
			// 更新属性
			feature.setAttribute(column, newValue);
			// 更新图层
			m_vectorLayer->updateFeature(feature);
			// 设置未保存更改的标志
			m_hasUnsavedChanges = true;
		}
	}
}

// 验证数据类型
bool CustomDialog::validateDataType(QVariant::Type type, const QString& newValue) {
	// 根据数据类型进行验证
	switch (type) {
	case QVariant::String:
		// 字符串类型总是匹配
		return true;
	case QVariant::Int:
		// 整数类型需要转换后的值不为0
		return newValue.toInt() != 0;
	case QVariant::Double:
		// 浮点数类型需要转换后的值不为0
		return newValue.toDouble() != 0;
	default:
		// 其他类型不匹配
		return false;
	}
}

// 处理自定义上下文菜单事件
void CustomDialog::handleCustomContextMenu(const QPoint& pos) {
	QMenu contextMenu;
	// 获取当前位置的索引
	QModelIndex index = m_tableWidget->indexAt(pos);
	// 如果索引无效，则返回
	if (!index.isValid()) {
		return;
	}

	// 如果表格的编辑触发方式不是所有触发方式则返回
	if (m_tableWidget->editTriggers() != QAbstractItemView::AllEditTriggers) {
		return;
	}

	// 添加删除列的菜单项
	QAction* deleteColumnAction = contextMenu.addAction("删除列");
	connect(deleteColumnAction, &QAction::triggered, [this, index]() {
		// 获取要删除的列索引
		int colToDelete = index.column();
		// 开始编辑图层
		m_vectorLayer->startEditing();
		if (m_vectorLayer->deleteAttribute(colToDelete)) {
			m_tableWidget->removeColumn(colToDelete);
			// 提交图层的更改
			m_vectorLayer->commitChanges();
			QMessageBox::information(this, "成功", "列已删除！");
		}
		else {
			QMessageBox::warning(this, "错误", "删除列失败！");
		}
		});

	QAction* deleteRowAction = contextMenu.addAction("删除行");
	connect(deleteRowAction, &QAction::triggered, [this, index]() {
		// 获取要删除的行索引
		int rowToDelete = index.row();
		QgsFeatureId featureId = m_tableWidget->item(rowToDelete, 0)->data(Qt::UserRole).toLongLong();
		// 开始编辑图层
		m_vectorLayer->startEditing();
		// 删除特征
		if (m_vectorLayer->deleteFeature(featureId)) {
			m_tableWidget->removeRow(rowToDelete);
			// 提交图层的更改
			m_vectorLayer->commitChanges();
			QMessageBox::information(this, "成功", "行已删除！");
		}
		else {
			QMessageBox::warning(this, "错误", "删除行失败！");
		}
		});

	// 显示上下文菜单
	contextMenu.exec(m_tableWidget->mapToGlobal(pos));
}

// 添加新列
void CustomDialog::addNewColumn() {
	// 用户输入的新列名是否有效
	bool ok;
	QString fieldName = QInputDialog::getText(this, "添加属性列", "请输入新列名：", QLineEdit::Normal, "", &ok);
	if (!ok || fieldName.isEmpty()) return;

	// 提供列类型选项供用户选择
	QStringList fieldTypes = { "字符串", "整数", "浮点数" };
	QString fieldType = QInputDialog::getItem(this, "选择列类型", "类型：", fieldTypes, 0, false, &ok);
	if (!ok) return;

	// 根据用户选择的类型创建新的字段
	QgsField newField(fieldName, fieldType == "字符串" ? QVariant::String : (fieldType == "整数" ? QVariant::Int : QVariant::Double));
	m_vectorLayer->startEditing();
	// 尝试向数据提供者添加新字段
	if (m_vectorLayer->dataProvider()->addAttributes({ newField })) {
		// 更新图层的字段信息
		m_vectorLayer->updateFields();
		QStringList headers;
		for (const QgsField& field : m_vectorLayer->fields()) {
			headers << field.name();
		}
		// 设置表格的列数和列标题
		m_tableWidget->setColumnCount(m_vectorLayer->fields().count());
		m_tableWidget->setHorizontalHeaderLabels(headers);
		QMessageBox::information(this, "成功", "新列已添加！");
	}
	else {
		QMessageBox::warning(this, "错误", "无法添加新列。");
	}
}

void CustomDialog::closeEvent(QCloseEvent* event) {
	if (m_hasUnsavedChanges) {
		auto reply = QMessageBox::question(this, "确认", "还有未保存的更改，确定要退出吗？", QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::No) {
			event->ignore();
			return;
		}
		m_vectorLayer->rollBack();
	}
	event->accept();
}

void CustomDialog::populateFieldTable(QTableWidget* fieldTableWidget) {
	fieldTableWidget->setRowCount(m_vectorLayer->fields().count());
	int row = 0;
	for (const QgsField& field : m_vectorLayer->fields()) {
		// 字段名
		QTableWidgetItem* nameItem = new QTableWidgetItem(field.name());
		fieldTableWidget->setItem(row, 0, nameItem);

		// 字段类型
		QTableWidgetItem* typeItem = new QTableWidgetItem(QVariant::typeToName(field.type()));
		fieldTableWidget->setItem(row, 1, typeItem);

		// 字段长度
		QTableWidgetItem* lengthItem = new QTableWidgetItem(QString::number(field.length()));
		fieldTableWidget->setItem(row, 2, lengthItem);

		++row;
	}
}

void CustomDialog::saveFieldChanges(QTableWidget* fieldTableWidget) {
	m_vectorLayer->startEditing(); // 开始编辑图层

	for (int row = 0; row < fieldTableWidget->rowCount(); ++row) {
		QString newName = fieldTableWidget->item(row, 0)->text();
		QString newType = fieldTableWidget->item(row, 1)->text();
		int newLength = fieldTableWidget->item(row, 2)->text().toInt();

		// 获取字段索引
		int fieldIndex = row;
		QgsField originalField = m_vectorLayer->fields().at(fieldIndex);

		// 检查是否需要更新字段属性
		if (originalField.name() != newName || originalField.typeName() != newType || originalField.length() != newLength) {
			// 删除原字段
			if (!m_vectorLayer->dataProvider()->deleteAttributes({ fieldIndex })) {
				QMessageBox::warning(this, "错误", QString("无法删除字段 '%1'").arg(originalField.name()));
				return;
			}

			// 创建新字段，匹配构造函数参数
			QgsField updatedField(newName, QVariant::nameToType(newType.toUtf8().constData()), QString(), newLength);

			// 添加新字段
			if (!m_vectorLayer->dataProvider()->addAttributes({ updatedField })) {
				QMessageBox::warning(this, "错误", QString("无法添加字段 '%1'").arg(newName));
				return;
			}
		}
	}

	m_vectorLayer->updateFields(); // 更新字段信息
	m_vectorLayer->commitChanges(); // 提交更改
	QMessageBox::information(this, "成功", "字段修改已保存！");
}


