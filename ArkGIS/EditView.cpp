#include "EditView.h"

EditView::EditView(QgsMapCanvas* canvas, QObject* parent)
	: QgsMapTool(canvas), mpCanvas(canvas), mCurrentLayer(nullptr), mEditMode(None), mpRubberBand(nullptr), mSelectionRubberBand(nullptr) {
	setAction(nullptr);
}

// 设置当前编辑图层
void EditView::setLayer(QgsVectorLayer* layer, EditMode mode) {
	mCurrentLayer = layer;
	mEditMode = mode;
	points.clear();  // 清空用于线段的点列表

	if (mode == CopyMode) {

	}

	// 创建橡皮筋对象用于绘制动态线段
	if (mode == SelectAndAttriMode) {
		if (mpRubberBand) {
			delete mpRubberBand;
			// 清空当前选择的要素
			mCurrentLayer->removeSelection();
		}
	}
	else if (mode == MoveMode) {
		// 进入移动模式
		if (mCurrentLayer->selectedFeatureIds().isEmpty()) {
			QMessageBox::warning(nullptr, tr("错误"), tr("没有选中的要素！"));
			return;
		}
	}
	else if (mode == PolygonMode || mode == LineMode) {
		Qgis::GeometryType geometryType = (mode == PolygonMode) ? Qgis::GeometryType::Polygon : Qgis::GeometryType::Line;
		mpRubberBand = new QgsRubberBand(mpCanvas, Qgis::GeometryType::Line);
		mpRubberBand->setColor(Qt::black);  // 设置橡皮筋的颜色
		mpRubberBand->setWidth(2);        // 设置橡皮筋的宽度

		if (mode == PolygonMode) {
			// 创建自定义符号并应用到橡皮筋
			QgsFillSymbol* symbol = QgsFillSymbol::createSimple({ {"color", "0,255,0,50"},  // 50% 透明度绿色填充
																{"outline_color", "lightgreen"}, // 绿色边界
																{"outline_width", "0.4"} });
			mpRubberBand->setSymbol(symbol);  // 应用自定义符号
		}
		else {
			mpRubberBand->setColor(Qt::black);  // 线段颜色
		}
		mpRubberBand->setWidth(2);  // 设置橡皮筋宽度
	}
}

// 重置编辑状态
void EditView::reset() {
	points.clear();  // 清空点击的点
	if (mpRubberBand) {
		mpRubberBand->reset(Qgis::GeometryType::Line);  // 重置橡皮筋
	}
	mEditMode = None;  // 重置编辑模式
}

void EditView::canvasPressEvent(QgsMapMouseEvent* event) {
	if (mEditMode == MoveMode) {
		QgsPointXY clickPoint = event->mapPoint();
		QgsFeatureIterator iterator = mCurrentLayer->getSelectedFeatures();
		QgsFeature feature;
		while (iterator.nextFeature(feature)) {
			if (GeometryRelation::pointIntersects(clickPoint, feature.geometry())) {
				mMovingFeature = feature;  // 记录当前选中的要素
				mMoveStartPoint = clickPoint;  // 记录鼠标点击时的位置
				mIsMoving = true;
				break;
			}
		}
	}
}

// 捕获鼠标点击事件，用于记录点坐标
void EditView::canvasReleaseEvent(QgsMapMouseEvent* event) {
	if (!mCurrentLayer) {
		QMessageBox::warning(nullptr, tr("错误"), tr("未选中图层，无法添加要素！"));
		return;
	}

	if (!mCurrentLayer->isEditable()) {
		if (!mCurrentLayer->startEditing()) {
			QMessageBox::warning(nullptr, tr("错误"), tr("无法进入编辑模式！"));
			return;
		}
	}
	// 根据当前编辑模式进行不同操作
	QgsPointXY mapPoint = event->mapPoint();

	if (mEditMode == PointMode) {
		createPoint(mapPoint);
	}
	else if (mEditMode == LineMode) {
		addPointToLine(mapPoint);
	}
	else if (mEditMode == PolygonMode) {
		addPointToPolygon(mapPoint);
	}
	else if (mEditMode == SelectAndAttriMode) {
		QgsFeature closestFeature;
		double minDistance = std::numeric_limits<double>::max();

		QgsFeatureIterator iterator = mCurrentLayer->getFeatures();
		QgsFeature feature;
		bool foundFeature = false;
		while (iterator.nextFeature(feature)) {
			if (GeometryRelation::pointIntersects(mapPoint, feature.geometry())) {
				foundFeature = true;
				double distance = GeometryRelation::distanceToGeometry(mapPoint, feature.geometry());
				if (distance < minDistance) {
					minDistance = distance;
					closestFeature = feature;
				}
			}
		}

		if (foundFeature) {
			// 检测 Ctrl 键状态
			if (event->modifiers() & Qt::ControlModifier) {
				// Ctrl 键按下，切换选择状态
				if (mCurrentLayer->selectedFeatureIds().contains(closestFeature.id())) {
					mCurrentLayer->deselect(closestFeature.id()); // 取消选择
				}
				else {
					mCurrentLayer->select(closestFeature.id()); // 添加选择
				}
			}
			else {
				// 没有按下 Ctrl 键，仅选择当前要素
				mCurrentLayer->removeSelection();
				mCurrentLayer->select(closestFeature.id());
			}
		}
		else {
			// 未找到相交要素，清空选择
			mCurrentLayer->removeSelection();
		}
	}
	else if (mIsMoving && mEditMode == MoveMode) {
		QgsVectorLayer* layer = dynamic_cast<QgsVectorLayer*>(mCurrentLayer);
		mIsMoving = false;
		reset();  // 重置编辑模式
	}
	else if (mEditMode == CopyMode) {
		// 弹出对话框，获取行数、列数和间距
		QDialog dialog;
		dialog.setWindowTitle(tr("设置复制参数"));
		QFormLayout* formLayout = new QFormLayout(&dialog);

		QSpinBox* rowSpinBox = new QSpinBox(&dialog);
		rowSpinBox->setMinimum(1);
		rowSpinBox->setValue(1);
		formLayout->addRow(tr("行数："), rowSpinBox);

		QSpinBox* colSpinBox = new QSpinBox(&dialog);
		colSpinBox->setMinimum(1);
		colSpinBox->setValue(1);
		formLayout->addRow(tr("列数："), colSpinBox);

		QLineEdit* rowSpacingEdit = new QLineEdit(&dialog);
		rowSpacingEdit->setPlaceholderText(tr("行间距 (数值或 inf)"));
		formLayout->addRow(tr("行间距："), rowSpacingEdit);

		QLineEdit* colSpacingEdit = new QLineEdit(&dialog);
		colSpacingEdit->setPlaceholderText(tr("列间距 (数值或 inf)"));
		formLayout->addRow(tr("列间距："), colSpacingEdit);

		QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
		formLayout->addWidget(buttonBox);

		connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

		if (dialog.exec() == QDialog::Accepted) {
			int rows = rowSpinBox->value();
			int cols = colSpinBox->value();
			QString rowSpacingInput = rowSpacingEdit->text();
			QString colSpacingInput = colSpacingEdit->text();

			double rowSpacing = (rowSpacingInput.toLower() == "inf") ? std::numeric_limits<double>::infinity() : rowSpacingInput.toDouble();
			double colSpacing = (colSpacingInput.toLower() == "inf") ? std::numeric_limits<double>::infinity() : colSpacingInput.toDouble();

			copyFeaturesArray(rows, cols, rowSpacing, colSpacing);
		}
		// 重置模式
		reset();
	}
	else if (mEditMode == InsertPointMode) {
		// 判断是否为线图层或面图层
		if (mCurrentLayer->geometryType() != Qgis::GeometryType::Line && mCurrentLayer->geometryType() != Qgis::GeometryType::Polygon) {
			QMessageBox::warning(nullptr, tr("错误"), tr("只有线图层或面图层才能进行插入点操作"));
			return;
		}

		if (mCurrentLayer->selectedFeatureIds().isEmpty()) {
			QMessageBox::warning(nullptr, tr("错误"), tr("请先选择要编辑的要素！"));
			return;
		}
		// 获取选中的要素
		QgsFeatureList selectedFeatures = mCurrentLayer->selectedFeatures();
		if (selectedFeatures.size() > 1) {
			QMessageBox::warning(nullptr, tr("错误"), tr("一次只能编辑一个要素！"));
			return;
		}
		// 获取选中的线要素
		QgsGeometry selectedGeometry = selectedFeatures[0].geometry();		
		insertPointInLine(selectedGeometry, mapPoint);
		selectedFeatures[0].setGeometry(selectedGeometry);
		mCurrentLayer->updateFeature(selectedFeatures[0]);
		reset();  // 重置编辑模式
	}
}

#include <cmath>

void EditView::copyFeaturesArray(int rows, int cols, double rowSpacing, double colSpacing) {

	QgsFeatureList selectedFeatures;
	QgsFeature feature;
	QgsFeatureIterator iterator = mCurrentLayer->getSelectedFeatures();
	while (iterator.nextFeature(feature)) {
		selectedFeatures.append(feature);
	}

	// 创建新图层
	QString layerName = mCurrentLayer->name() + tr("_复制");
	QString geomType;
	switch (mCurrentLayer->geometryType()) {
	case Qgis::GeometryType::Point:
		geomType = "Point";
		break;
	case Qgis::GeometryType::Line:
		geomType = "LineString";
		break;
	case Qgis::GeometryType::Polygon:
		geomType = "Polygon";
		break;
	default:
		QMessageBox::warning(nullptr, tr("错误"), tr("不支持的几何类型！"));
		return;
	}

	QgsFields fields = mCurrentLayer->fields();
	QString crsWkt = mCurrentLayer->crs().toWkt();

	QgsVectorLayer* newLayer = new QgsVectorLayer(geomType + "?crs=" + crsWkt, layerName, "memory");
	if (!newLayer->isValid()) {
		QMessageBox::warning(nullptr, tr("错误"), tr("无法创建新图层！"));
		return;
	}

	// 添加字段
	newLayer->dataProvider()->addAttributes(fields.toList());
	newLayer->updateFields();

	// 开始编辑新图层
	newLayer->startEditing();

	// 获取初始要素的边界，用于计算偏移
	QgsRectangle extent = mCurrentLayer->boundingBoxOfSelected();

	// 如果用户输入无穷大，直接使用图层的边界
	if (std::isinf(rowSpacing)) {
		rowSpacing = extent.height() / (rows > 1 ? (rows - 1) : 1);
	}
	if (std::isinf(colSpacing)) {
		colSpacing = extent.width() / (cols > 1 ? (cols - 1) : 1);
	}

	// 复制要素并添加到新图层
	QList<QgsFeature> newFeatures;
	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < cols; ++col) {
			double offsetX = col * colSpacing;
			double offsetY = row * rowSpacing;

			for (const QgsFeature& feat : selectedFeatures) {
				QgsFeature newFeature;
				newFeature.setFields(fields);
				newFeature.setAttributes(feat.attributes());

				// 移动几何
				QgsGeometry geom = feat.geometry();

				geom.translate(offsetX, offsetY);
				newFeature.setGeometry(geom);

				newFeatures.append(newFeature);
			}
		}
	}

	// 将新要素添加到新图层
	if (!newLayer->dataProvider()->addFeatures(newFeatures)) {
		QMessageBox::warning(nullptr, tr("错误"), tr("添加复制要素失败！"));
		return;
	}

	// 结束编辑并保存新图层
	if (!newLayer->commitChanges()) {
		QMessageBox::warning(nullptr, tr("错误"), tr("无法保存新图层的更改！"));
		return;
	}

	// 将新图层添加到项目和图层树
	QgsProject::instance()->addMapLayer(newLayer);

	// 刷新画布
	mpCanvas->refresh();
}



// 捕获鼠标移动事件，用于更新橡皮筋位置
void EditView::canvasMoveEvent(QgsMapMouseEvent* event) {
	if (mEditMode == LineMode && !points.isEmpty()) {
		QgsPointXY mapPoint = event->mapPoint();
		updateRubberBand(mapPoint);
	}
	else if (mEditMode == PolygonMode && !points.isEmpty()) {
		QgsPointXY mapPoint = event->mapPoint();
		updateRubberBand(mapPoint);  // 更新橡皮筋动态效果
	}
	if (mIsMoving && mEditMode == MoveMode) {
		QgsPointXY currentPoint = event->mapPoint();
		QgsVector moveDelta = currentPoint - mMoveStartPoint;

		QgsVectorLayer* layer = dynamic_cast<QgsVectorLayer*>(mCurrentLayer);
		if (layer) {
			QgsFeatureIterator featureIterator = layer->getSelectedFeatures();
			QgsFeature feature;
			while (featureIterator.nextFeature(feature)) {
				QgsGeometry geometry = feature.geometry();

				geometry.translate(moveDelta.x(), moveDelta.y());

				QgsFeature updatedFeature = feature;
				updatedFeature.setGeometry(geometry);

				if (layer->updateFeature(updatedFeature)) {
					mMoveStartPoint = currentPoint;
					mpCanvas->refresh();
				}
			}
		}
	}
}

// 捕获鼠标双击事件，用于结束线段绘制
void EditView::canvasDoubleClickEvent(QgsMapMouseEvent* event) {
	if (mEditMode == LineMode) {
		finishLine();  // 双击完成线段
	}
	else if (mEditMode == PolygonMode) {
		finishPolygon();  // 双击完成面绘制
	}
	else if (mEditMode == SelectAndAttriMode) {
		QgsPointXY mapPoint = event->mapPoint();
		if (!mCurrentLayer) {
			QMessageBox::warning(nullptr, tr("错误"), tr("未选中图层，无法操作！"));
			return;
		}

		QgsFeature closestFeature;
		double minDistance = std::numeric_limits<double>::max();

		QgsFeatureIterator iterator = mCurrentLayer->getFeatures();
		QgsFeature feature;
		bool foundFeature = false;
		while (iterator.nextFeature(feature)) {
			if (GeometryRelation::pointIntersects(mapPoint, feature.geometry())) {
				foundFeature = true;
				double distance = GeometryRelation::distanceToGeometry(mapPoint, feature.geometry());
				if (distance < minDistance) {
					minDistance = distance;
					closestFeature = feature;
				}
			}
		}

		if (foundFeature) {
			showFeatureAttributes(closestFeature); // 显示属性对话框
		}
		else {
			QMessageBox::warning(nullptr, tr("错误"), tr("未找到相交要素！"));
		}
	}
}

// 创建点要素
void EditView::createPoint(const QgsPointXY& point) {
	QgsFeature feature(mCurrentLayer->fields());
	feature.setGeometry(QgsGeometry::fromPointXY(point));

	// 初始化属性
	initializeAttributes(feature);

	if (mCurrentLayer->addFeature(feature)) {
		mpCanvas->refresh();
	}
	else {
		QMessageBox::warning(nullptr, tr("错误"), tr("无法添加要素！"));
	}
}

// 添加点到线段列表
void EditView::addPointToLine(const QgsPointXY& point) {
	points.append(point);  // 将点击的点添加到线段的点列表

	// 更新橡皮筋，添加新的顶点
	mpRubberBand->addPoint(point, true);
}

// 在线要素或面要素中插入点
void EditView::insertPointInLine(QgsGeometry& geom, const QgsPointXY& point) {
	QgsPolylineXY polyline;
	// 判断线要素或面要素
	if (geom.type() == Qgis::GeometryType::Line) {
		// 判断 wkt 类型
		if (geom.wkbType() == Qgis::WkbType::MultiLineString) {
			polyline = geom.asMultiPolyline()[0];
		}
		else {
			polyline = geom.asPolyline();
		}
	}
	else {
		// 判断 wkt 类型
		if (geom.wkbType() == Qgis::WkbType::MultiPolygon) {
			polyline = geom.asMultiPolygon()[0][0];
		}
		else {
			polyline = geom.asPolygon()[0];
		}
	}
	// 查找最近的线段
	double minDistance = std::numeric_limits<double>::max();
	int closestSegmentIndex = -1;
	for (int i = 0; i < polyline.size() - 1; i++) {
		QgsPointXY p1 = polyline[i];
		QgsPointXY p2 = polyline[i + 1];
		QgsPolylineXY segment;
		segment << p1 << p2;
		QgsGeometry geomSegment = QgsGeometry::fromPolylineXY(segment);
		// 计算点到线段的距离
		double distance = GeometryRelation::distanceToGeometry(point, geomSegment);
		if (distance < minDistance) {
			minDistance = distance;
			closestSegmentIndex = i;
		}
	}
	if (closestSegmentIndex != -1) {
		// 在线段中插入点
		polyline.insert(closestSegmentIndex + 1, point);
		if (geom.type() == Qgis::GeometryType::Line) {
			geom = QgsGeometry::fromPolylineXY(polyline);
		}
		else {
			QgsPolygonXY polygon;
			if (geom.wkbType() == Qgis::WkbType::MultiPolygon) {
				polygon = geom.asMultiPolygon()[0];
			}
			else {
				polygon = geom.asPolygon();
			}
			polygon[0] = polyline;
			geom = QgsGeometry::fromPolygonXY(polygon);
		}
	}
	else {
		qDebug() << "No closest segment found.";
	}
}

// 更新橡皮筋的动态效果
void EditView::updateRubberBand(const QgsPointXY& tempPoint) {
	mpRubberBand->removeLastPoint();  // 移除上一个动态点
	mpRubberBand->addPoint(tempPoint, true);  // 添加当前鼠标位置为动态点
}

// 完成线要素的创建
void EditView::finishLine() {
	if (points.size() < 2) {
		QMessageBox::warning(nullptr, tr("错误"), tr("线段必须至少包含两个点！"));
		return;
	}

	QgsPolylineXY polyline;
	for (const QgsPointXY& point : points) {
		polyline.push_back(point);  // 将QList中的点逐个添加到vector中
	}

	QgsFeature feature(mCurrentLayer->fields());
	feature.setGeometry(QgsGeometry::fromPolylineXY(polyline));  // 使用转换后的 polyline

	// 初始化属性
	initializeAttributes(feature);

	if (mCurrentLayer->addFeature(feature)) {
		mpCanvas->refresh();
	}
	else {
		QMessageBox::warning(nullptr, tr("错误"), tr("无法添加要素！"));
	}

	reset();  // 清空当前线创建状态
}

// 添加点到面顶点列表
void EditView::addPointToPolygon(const QgsPointXY& point) {
	points.append(point);  // 将点击的点添加到面顶点列表

	// 更新橡皮筋，添加新的顶点
	mpRubberBand->addPoint(point, true);
}

// 完成面要素的创建
void EditView::finishPolygon() {
	if (points.size() < 3) {
		QMessageBox::warning(nullptr, tr("错误"), tr("面必须至少包含三个点！"));
		return;
	}

	// 创建 QgsPolygonXY (QList<QgsPointXY> 的列表)
	QgsPolygonXY polygon;
	QgsPolylineXY ring;
	for (const QgsPointXY& point : points) {
		ring.push_back(point);
	}
	polygon.push_back(ring);  // 添加闭合的环作为面

	// 创建要素并设置几何
	QgsFeature feature(mCurrentLayer->fields());
	feature.setGeometry(QgsGeometry::fromPolygonXY(polygon));  // 创建面几何

	// 初始化属性
	initializeAttributes(feature);

	if (mCurrentLayer->addFeature(feature)) {
		mpCanvas->refresh();  // 刷新地图画布
		mCurrentLayer->triggerRepaint();  // 强制图层重新渲染，确保应用样式
	}
	else {
		QMessageBox::warning(nullptr, tr("错误"), tr("无法添加要素！"));
	}

	reset();  // 清空当前面创建状态
}

// 初始化要素的属性字段
void EditView::initializeAttributes(QgsFeature& feature) {
	const QgsFields& fields = mCurrentLayer->fields();
	for (int i = 0; i < fields.count(); ++i) {
		const QgsField& field = fields.at(i);
		if (field.type() == QVariant::Int) {
			feature.setAttribute(i, 0);  // 整数类型字段默认值为 0
		}
		else if (field.type() == QVariant::Double) {
			feature.setAttribute(i, 0.0);  // 浮点数类型字段默认值为 0.0
		}
		else if (field.type() == QVariant::String) {
			feature.setAttribute(i, "");  // 字符串类型字段默认值为空字符串
		}
		else if (field.type() == QVariant::Bool) {
			feature.setAttribute(i, false);  // 布尔类型字段默认值为 false
		}
		else {
			feature.setAttribute(i, QVariant());  // 其他类型，使用无值
		}
	}
};

// 识别并选中要素
void EditView::identifyFromGeometry(const QgsGeometry& geometry) {
	// 获取图层中的要素并判断几何是否相交
	QgsVectorLayer* vectorLayer = dynamic_cast<QgsVectorLayer*>(mCurrentLayer);
	if (!vectorLayer) {
		QMessageBox::warning(nullptr, tr("错误"), tr("当前图层不是矢量图层，无法进行要素识别！"));
		return;
	}

	QgsFeatureRequest request;
	request.setFilterRect(geometry.boundingBox());

	// 遍历要素，找出相交的要素
	QgsFeature feature;
	QgsFeatureIterator iterator = vectorLayer->getFeatures(request);
	while (iterator.nextFeature(feature)) {
		if (geometry.intersects(feature.geometry())) {
			vectorLayer->removeSelection();
			vectorLayer->select(feature.id());  // 高亮选择的要素
			showFeatureAttributes(feature);    // 显示属性
			break;
		}
	}
}

// 显示要素属性的对话框
void EditView::showFeatureAttributes(QgsFeature& feature) {
	QDialog dialog;
	dialog.setWindowTitle(tr("要素属性"));

	QVBoxLayout* layout = new QVBoxLayout(&dialog);
	QTextEdit* textEdit = new QTextEdit(&dialog);
	textEdit->setReadOnly(true);

	QString attributesText;
	const QgsFields& fields = mCurrentLayer->fields();

	for (int i = 0; i < fields.count(); ++i) {
		QString fieldName = fields.at(i).name();
		QVariant fieldValue = feature.attribute(i);
		attributesText += QString("%1: %2\n").arg(fieldName).arg(fieldValue.toString());
	}
	textEdit->setText(attributesText);
	layout->addWidget(textEdit);

	// 添加“编辑”按钮
	QPushButton* editButton = new QPushButton(tr("编辑属性"), &dialog);
	layout->addWidget(editButton);

	connect(editButton, &QPushButton::clicked, [&]() {
		editFeatureAttributes(feature);  // 调用属性编辑方法
		dialog.accept();                 // 关闭当前对话框
		});

	dialog.exec();
}


// 编辑要素属性并保存修改
void EditView::editFeatureAttributes(QgsFeature& feature) {
	if (!mCurrentLayer || !mCurrentLayer->isEditable()) {
		QMessageBox::warning(nullptr, tr("错误"), tr("图层不可编辑，请先进入编辑模式！"));
		return;
	}

	// 创建一个对话框用于编辑属性
	QDialog dialog;
	dialog.setWindowTitle(tr("编辑要素属性"));
	QFormLayout* formLayout = new QFormLayout(&dialog);

	const QgsFields& fields = mCurrentLayer->fields();
	QVector<QLineEdit*> inputFields;

	// 为每个字段创建输入框
	for (int i = 0; i < fields.count(); ++i) {
		QString fieldName = fields.at(i).name();
		QVariant fieldValue = feature.attribute(i);

		QLineEdit* lineEdit = new QLineEdit(fieldValue.toString(), &dialog);
		formLayout->addRow(fieldName, lineEdit);
		inputFields.append(lineEdit);
	}

	// 添加按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	formLayout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 显示对话框并等待用户输入
	if (dialog.exec() == QDialog::Accepted) {
		for (int i = 0; i < fields.count(); ++i) {
			feature.setAttribute(i, inputFields[i]->text());
		}

		// 更新图层中的要素
		if (mCurrentLayer->updateFeature(feature)) {
			QMessageBox::information(nullptr, tr("成功"), tr("要素属性已更新！"));
			mpCanvas->refresh();  // 刷新画布显示
		}
		else {
			QMessageBox::warning(nullptr, tr("错误"), tr("更新要素失败！"));
		}
	}
}

