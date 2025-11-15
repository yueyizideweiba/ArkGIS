#include "StyleManager.h"
#include "CustomGraphicsScene.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QgsCategorizedSymbolRenderer.h>
#include <QgsStyle.h>
#include <QgsSymbol.h>
#include <QgsVectorLayer.h>
#include <QgsApplication.h>
#include <QMessageBox>
#include <QgsStyleManagerDialog.h>
#include <QToolBar>

// 绘图相关
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPushButton>
#include <QFileDialog>
#include <qsvggenerator.h>
#include <QPainter>
#include <qgssymbol.h>
#include <qgssymbollayer.h>
#include <qgsmarkersymbollayer.h>
#include <QInputDialog>
#include <QBuffer>

// 包含 QGIS 渲染器相关的头文件
#include <qgsrenderer.h>
#include <qgssinglesymbolrenderer.h>
#include <qgscategorizedsymbolrenderer.h>
#include <qgsgraduatedsymbolrenderer.h>

// 包含 QGIS 渲染器小部件
#include <qgssinglesymbolrendererwidget.h>
#include <qgscategorizedsymbolrendererwidget.h>
#include <qgsgraduatedsymbolrendererwidget.h>

//栅格渲染组件
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QMessageBox.h>
#include <QgsRasterLayer.h>
#include <QgsColorRampShader.h>
#include <QgsSingleBandPseudoColorRenderer.h>
#include <QgsMultiBandColorRenderer.h>
#include <QgsRasterShader.h>  
#include< QColorDialog >

class QgsRasterLayer;
class QgsColorRampShader;

StyleManager::StyleManager(QObject* parent) : QObject(parent) {}

//样式管理器
void StyleManager::openStyleManager(QgsMapLayer* layer)
{
	if (!layer)
	{
		QMessageBox::warning(nullptr, tr("错误"), tr("无效的图层。"));
		return;
	}

	// 如果是矢量图层，打开样式管理器对话框
	QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);
	QgsRasterLayer* rasterLayer = qobject_cast<QgsRasterLayer*>(layer);
	gloBalrasterLayer = rasterLayer;

	if (vectorLayer)
	{
		// 打开自定义的样式管理器对话框
		QDialog* dialog = createStyleDialog(vectorLayer);
		dialog->exec(); // 显示对话框并等待用户操作
	}
	else if (rasterLayer)
	{
		//QgsRasterLayer* rasterLayer = qobject_cast<QgsRasterLayer*>(layer);

		QColor startColor = QColor(Qt::red);
		QColor endColor = QColor(Qt::blue);
		//applyRasterStyle(gloBalrasterLayer,0,0,startColor, endColor);
		qDebug() << "secussfully opened";
		dialog = createTifStyleDialog(gloBalrasterLayer);
		qDebug() << "dialog created";
		dialog->exec(); // 显示对话框并等待用户操作
		qDebug() << "dialog finished";
		//QMessageBox::warning(nullptr, tr("错误"), tr("样式管理器只支持矢量图层。"));
	}
}
//创建样式管理器对话框
QDialog* StyleManager::createStyleDialog(QgsVectorLayer* layer)
{
	QDialog* dialog = new QDialog();
	dialog->setWindowTitle(tr("样式管理器 - %1").arg(layer->name()));
	dialog->resize(800, 1000);

	QVBoxLayout* layout = new QVBoxLayout(dialog);

	// 加载系统符号库
	QgsStyle* style = QgsStyle::defaultStyle();
	style->importXml("./symbols/default.xml");

	// 创建符号库选择器
	QLabel* label = new QLabel(tr("符号库选择："), dialog);
	layout->addWidget(label);

	// 创建一个占位控件，用于显示不同的渲染器小部件
	QWidget* rendererWidgetContainer = new QWidget(dialog);
	QVBoxLayout* rendererLayout = new QVBoxLayout(rendererWidgetContainer);
	layout->addWidget(rendererWidgetContainer);

	// 保存原始的渲染器状态，以便在用户点击 "Cancel" 时可以恢复
	QgsFeatureRenderer* originalRenderer = layer->renderer()->clone();

	QgsFeatureRenderer* renderer = layer->renderer();
	QgsRendererWidget* rendererWidget = nullptr;

	// 根据用户选择的模式，动态创建渲染器小部件
	auto updateRendererWidget = [=, &rendererWidget]() mutable {
		// 如果有旧的渲染器小部件，移除并删除它
		if (rendererWidget)
		{
			rendererLayout->removeWidget(rendererWidget);
			rendererWidget->deleteLater();
			delete rendererWidget;
		}

		// 创建新的渲染器小部件
		QgsRendererWidget* newRendererWidget = nullptr;

		// 单一符号化渲染
		if (dynamic_cast<QgsSingleSymbolRenderer*>(renderer))
		{
			newRendererWidget = new QgsSingleSymbolRendererWidget(layer, style, dynamic_cast<QgsSingleSymbolRenderer*>(renderer));
		}
		else
		{
			newRendererWidget = new QgsSingleSymbolRendererWidget(layer, style, new QgsSingleSymbolRenderer(QgsSymbol::defaultSymbol(layer->geometryType())));
		}

		// 将新的渲染器小部件赋值给 rendererWidget
		rendererWidget = newRendererWidget;

		// 添加新的渲染器小部件到布局
		if (rendererWidget)
		{
			rendererLayout->addWidget(rendererWidget);
		}
		};

	// 初始化时加载当前渲染器小部件
	updateRendererWidget();

	// 确认、取消和应用按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, dialog);
	layout->addWidget(buttonBox);

	QCheckBox* saveCheckBox = new QCheckBox(tr("保存到指定文件夹"), dialog);
	saveCheckBox->setChecked(true);  // 默认选中
	layout->addWidget(saveCheckBox);


	// 连接“应用”按钮的信号，确保应用当前的符号化设置，但不关闭对话框
	connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, dialog, [=]() {
		// 检查渲染器是否为空
		if (!rendererWidget || !rendererWidget->renderer()) {
			QMessageBox::warning(dialog, tr("错误"), tr("无法获取渲染器。"));
			return;
		}

		// 设置渲染器并刷新地图
		layer->setRenderer(rendererWidget->renderer());
		layer->triggerRepaint();  // 触发重新绘制
		//TAGING
		if (saveCheckBox->isChecked()) {
			// 确保 sld_data 目录存在
			QDir sldDataDir("sld_data");
			if (!sldDataDir.exists()) {
				sldDataDir.mkpath(".");
			}

			// 构建相对路径
			QString sldFilePath = sldDataDir.absoluteFilePath(QString("symbol_%1.sld").arg(layer->name()));

			// 保存 SLD 文件
			bool resultFlag = true;
			QString statusMessage = layer->saveSldStyle(sldFilePath, resultFlag);

			if (resultFlag) {
				qDebug() << "SLD file saved successfully at: " << sldFilePath;
			}
			else {
				qDebug() << "Failed to save SLD file: " << statusMessage;
			}
		}


		});

	// 连接“确定”按钮的信号，应用符号化设置并关闭对话框
	connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, dialog, [=]() {
		// 检查渲染器是否为空
		if (!rendererWidget || !rendererWidget->renderer()) {
			QMessageBox::warning(dialog, tr("错误"), tr("无法获取渲染器。"));
			return;
		}

		// 设置渲染器并刷新地图
		layer->setRenderer(rendererWidget->renderer());
		layer->triggerRepaint();  // 触发重新绘制
		dialog->accept();  // 关闭对话框

		// 删除原始渲染器对象
		delete originalRenderer;
		});

	// 连接“取消”按钮的信号，恢复原始的渲染器状态并关闭对话框
	connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, dialog, [=]() {
		// 恢复原始的渲染器
		layer->setRenderer(originalRenderer);
		layer->triggerRepaint();  // 触发重新绘制

		dialog->reject();  // 关闭对话框
		});

	return dialog;
};

QDialog* StyleManager::createTifStyleDialog(QgsRasterLayer* layer)
{
	QDialog* dialog = new QDialog();
	dialog->setWindowTitle(tr("栅格样式管理器 - %1").arg(layer->name()));
	dialog->resize(600, 400);

	QVBoxLayout* layout = new QVBoxLayout(dialog);

	// 波段选择
	QLabel* bandLabel = new QLabel(tr("选择波段："), dialog);
	bandComboBox = new QComboBox(dialog);
	for (int i = 1; i <= layer->bandCount(); ++i)
	{
		bandComboBox->addItem(QString::number(i));
	}
	layout->addWidget(bandLabel);
	layout->addWidget(bandComboBox);

	// 渲染模式选择
	QLabel* renderModeLabel = new QLabel(tr("渲染模式："), dialog);
	QComboBox* temp_renderModeComboBox = new QComboBox(dialog);
	renderModeComboBox.push_back(temp_renderModeComboBox);
	temp_renderModeComboBox->addItems({ tr("伪色彩"), tr("真彩色") });
	layout->addWidget(renderModeLabel);
	layout->addWidget(renderModeComboBox[renderModeComboBox.size() - 1]);


	// 颜色选择器
	m_startColor = QColor(Qt::red);
	m_endColor = QColor(Qt::blue);
	setupColorRampSelector(layout, m_startColor, m_endColor);

	// 应用按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, dialog);
	layout->addWidget(buttonBox);
	//applyRasterStyle(gloBalrasterLayer, bandComboBox->currentText().toInt(), renderModeComboBox[renderModeComboBox.size() - 1]->currentIndex(), m_startColor, m_endColor);

	//applyRasterStyle(gloBalrasterLayer, 0, 0, m_startColor, m_endColor);
	// 连接“应用”按钮的信号
	connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, [&]() {
		applyRasterStyle(gloBalrasterLayer, 0, 0, m_startColor, m_endColor);

		});

	// 连接“确定”按钮的信号
	connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, [this, dialog]() {
		applyRasterStyle(gloBalrasterLayer, bandComboBox->currentText().toInt(), renderModeComboBox[renderModeComboBox.size() - 1]->currentIndex(), m_startColor, m_endColor);
		//clearParameters();
		dialog->accept();
		qDebug() << "thngs solved";
		});

	// 连接“取消”按钮的信号
	connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [this, dialog]() {
		//clearParameters();
		dialog->reject();
		});
	qDebug() << "dialog is going to returned";
	return dialog;
}

void StyleManager::setupColorRampSelector(QVBoxLayout* layout, QColor& startColor, QColor& endColor)
{
	// 起始颜色选择
	QLabel* startColorLabel = new QLabel(tr("起始颜色："), layout->parentWidget());
	QPushButton* startColorButton = new QPushButton(tr("选择颜色"), layout->parentWidget());
	layout->addWidget(startColorLabel);
	layout->addWidget(startColorButton);

	// 终止颜色选择
	QLabel* endColorLabel = new QLabel(tr("终止颜色："), layout->parentWidget());
	QPushButton* endColorButton = new QPushButton(tr("选择颜色"), layout->parentWidget());
	layout->addWidget(endColorLabel);
	layout->addWidget(endColorButton);

	// 色带显示
	colorRampPixmap.scaled(200, 50, Qt::KeepAspectRatio);
	drawColorRamp(colorRampPixmap, startColor, endColor);
	colorRampLabel = new QLabel(layout->parentWidget());
	colorRampLabel->setPixmap(colorRampPixmap);
	layout->addWidget(colorRampLabel);


	/*QColor newStartColor = QColorDialog::getColor(startColor, nullptr, tr("选择起始颜色"));
		startColor = newStartColor;
		drawColorRamp(colorRampPixmap, startColor, endColor);
		colorRampLabel->setPixmap(colorRampPixmap);

		QColor newEndColor = QColorDialog::getColor(endColor, nullptr, tr("选择终止颜色"));


		if (newEndColor.isValid())
		{
			endColor = newEndColor;
			drawColorRamp(colorRampPixmap, startColor, endColor);
			colorRampLabel->setPixmap(colorRampPixmap);
		}*/


	connect(startColorButton, &QPushButton::clicked, [&]() {
		QColor newStartColor = QColorDialog::getColor(startColor, nullptr, tr("选择起始颜色"));
		if (newStartColor.isValid())
		{
			startColor = newStartColor;
			qDebug() << "startColor: " << startColor;

			// 确保 colorRampPixmap 的大小正确
			if (colorRampPixmap.size() != QSize(200, 30)) {
				colorRampPixmap = QPixmap(200, 30);
				colorRampPixmap.fill(Qt::white);
			}

			// 绘制色带
			drawColorRamp(colorRampPixmap, startColor, endColor);
			qDebug() << "通过颜色计算";

			// 设置标签的 pixmap
			if (colorRampLabel) {
				colorRampLabel->setPixmap(colorRampPixmap);
				qDebug() << "通过颜色设置" << "标签";
			}
			else {
				qDebug() << "colorRampLabel is null";
			}
		}
		});
	connect(endColorButton, &QPushButton::clicked, [&]() {
		QColor newEndColor = QColorDialog::getColor(endColor, nullptr, tr("选择终止颜色"));
		if (newEndColor.isValid())
		{
			endColor = newEndColor;
			drawColorRamp(colorRampPixmap, startColor, endColor);
			colorRampLabel->setPixmap(colorRampPixmap);
		}
		});
}

void StyleManager::drawColorRamp(QPixmap& pixmap, QColor& startColor, QColor& endColor)
{
	QPainter painter(&pixmap);
	painter.fillRect(pixmap.rect(), Qt::white);

	int width = pixmap.width();
	int height = pixmap.height();
	qDebug() << "width: " << width << "height: " << height << "进行时段-E1";
	for (int x = 0; x < width; ++x)
	{
		qDebug() << "width: " << width << "height: " << height << "进行时段-E1_0";
		double ratio = static_cast<double>(x) / (width - 1);
		QColor color = startColor;
		color.setRedF(startColor.redF() * (1 - ratio) + endColor.redF() * ratio);
		color.setGreenF(startColor.greenF() * (1 - ratio) + endColor.greenF() * ratio);
		color.setBlueF(startColor.blueF() * (1 - ratio) + endColor.blueF() * ratio);
		painter.fillRect(x, 0, 1, height, color);

	}
	//qDebug() << "width: " << width << "height: " << height << "进行时段-E2";
}

void StyleManager::applyRasterStyle(QgsRasterLayer* layer, int band, int renderMode, QColor startColor, QColor endColor)
{
	if (renderMode == 0)  // 伪色彩
	{
		QgsColorRampShader temp_shader;
		shader.push_back(temp_shader);

		shader[shader.size() - 1].setColorRampType(QgsColorRampShader::Interpolated);
		//temp_shader.setColorRampType(QgsColorRampShader::Interpolated);
		// 生成颜色映射列表
		QList<QgsColorRampShader::ColorRampItem> colorRampItems;
		QgsRasterDataProvider* provideriance = layer->dataProvider();
		QgsRasterBandStats stats = provideriance->bandStatistics(1, QgsRasterBandStats::All, layer->extent(), 0);
		if (stats.minimumValue == stats.maximumValue) {
			qCritical() << "Minimum and maximum values are the same, cannot create color ramp";
			return;
		}

		double minVal = stats.minimumValue;
		double maxVal = stats.maximumValue;
		qDebug() << "minVal: " << minVal << "maxVal: " << maxVal << "进行时段-E3";

		//int numClasses = 30;  // 30个离散颜色
		int numClasses = 30;  // 30个离散颜色
		for (int i = 0; i <= numClasses; ++i)
		{
			double ratio = static_cast<double>(i) / numClasses;
			double value = minVal + ratio * (maxVal - minVal);
			QColor color = interpolateColor(startColor, endColor, ratio);

			QgsColorRampShader::ColorRampItem item;
			item.color = color;
			item.value = value;
			item.label = QString::number(value);
			colorRampItems.append(item);

			qDebug() << "Color Ramp Item" << i << ": Value" << value << ", Color" << color.name();
		}
		//temp_shader.setColorRampItemList(colorRampItems);
		shader[shader.size() - 1].setColorRampItemList(colorRampItems);
		qDebug() << "Color ramp shader initialized successfully";

		// 创建 QgsRasterShader
		QgsRasterShader* rasterShader = new QgsRasterShader();
		rasterShader->setRasterShaderFunction(&shader[shader.size() - 1]);
		//rasterShader->setRasterShaderFunction(&temp_shader);
		if (!rasterShader) {
			qCritical() << "Failed to create QgsRasterShader";
			return;
		}
		qDebug() << "QgsRasterShader initialized successfully";


		//rendererWidget

	// 创建单波段伪彩色渲染器
		QgsSingleBandPseudoColorRenderer* temp_renderer = new QgsSingleBandPseudoColorRenderer(provideriance, 1, rasterShader);
		renderer.push_back(temp_renderer);

		if (!temp_renderer) {
			qCritical() << "Failed to create QgsSingleBandPseudoColorRenderer";
			return;
		}
		qDebug() << "QgsSingleBandPseudoColorRenderer initialized successfully";

		// 先取消当前的渲染器
		layer->setRenderer(nullptr);
		qDebug() << "Current renderer cleared successfully,renderer.size() is" << renderer.size();

		// 设置新的渲染器
		//renderer = new QgsSingleBandPseudoColorRenderer(layer->dataProvider(), 1, rasterShader);
		layer->setRenderer(renderer[renderer.size() - 1]);
		qDebug() << "New renderer set successfully";

		// 刷新图层
		layer->triggerRepaint();
		qDebug() << "Layer repainted successfully";

	}
	else if (renderMode == 1)  // 假色彩
	{
		// 获取用户选择的波段
		int redBand = band;
		int greenBand = band + 1;
		int blueBand = band + 2;

		// 创建多波段彩色渲染器
		QgsMultiBandColorRenderer* renderer = new QgsMultiBandColorRenderer(layer->dataProvider(), redBand, greenBand, blueBand);
		layer->setRenderer(renderer);
	}

	layer->triggerRepaint();
}
QColor StyleManager::interpolateColor(const QColor& startColor, const QColor& endColor, double ratio)
{
	int r = startColor.red() + (endColor.red() - startColor.red()) * ratio;
	int g = startColor.green() + (endColor.green() - startColor.green()) * ratio;
	int b = startColor.blue() + (endColor.blue() - startColor.blue()) * ratio;
	return QColor(r, g, b);
}
void StyleManager::clearParameters()
{
	// 清空所有参数
	gloBalrasterLayer = nullptr;
	//renderer;
	//shader.setColorRampItemList({});
	colorRampLabel = nullptr;
	dialog = nullptr;
}