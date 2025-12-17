#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFontDialog>
#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QToolBar>
#include <QFontComboBox>
#include <QSpinBox>
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentCurveIndex(-1),
      dragModeEnabled(false), isDragging(false), draggedGraph(nullptr), draggedPointIndex(-1),
      hasAutoRescaled(false)
{
    // 设置程序默认字体为Microsoft YaHei
    QFont defaultFont("Microsoft YaHei", 9);
    QApplication::setFont(defaultFont);
    
    // 初始化默认字体
    plotTitleFont = QFont("Microsoft YaHei", 12, QFont::Bold);
    xAxisLabelFont = QFont("Microsoft YaHei", 10);
    yAxisLabelFont = QFont("Microsoft YaHei", 10);
    
    setupUI();
    
    // 初始化图表属性
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    customPlot->xAxis->setLabel("X轴");
    customPlot->xAxis->setLabelFont(xAxisLabelFont);
    customPlot->yAxis->setLabel("Y轴");
    customPlot->yAxis->setLabelFont(yAxisLabelFont);
    customPlot->legend->setVisible(true);
    customPlot->xAxis->grid()->setVisible(true);
    customPlot->yAxis->grid()->setVisible(true);
    
    // 默认显示X2轴和Y2轴，但不显示刻度值
    customPlot->xAxis2->setVisible(true);
    customPlot->xAxis2->setTickLabels(false);
    customPlot->yAxis2->setVisible(true);
    customPlot->yAxis2->setTickLabels(false);
    
    // 设置默认图表标题
    customPlot->plotLayout()->insertRow(0);
    QCPTextElement* title = new QCPTextElement(customPlot, "图表示例标题");
    title->setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    customPlot->plotLayout()->addElement(0, 0, title);
    
    // 默认设置为对数坐标轴
    customPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
    QSharedPointer<QCPAxisTickerLog> logTickerX(new QCPAxisTickerLog);
    customPlot->xAxis->setTicker(logTickerX);
    customPlot->xAxis->setNumberFormat("eb");
    customPlot->xAxis->setNumberPrecision(0);
    
    customPlot->xAxis2->setScaleType(QCPAxis::stLogarithmic);
    QSharedPointer<QCPAxisTickerLog> logTickerX2(new QCPAxisTickerLog);
    customPlot->xAxis2->setTicker(logTickerX2);
    customPlot->xAxis2->setNumberFormat("eb");
    customPlot->xAxis2->setNumberPrecision(0);
    
    customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    QSharedPointer<QCPAxisTickerLog> logTickerY(new QCPAxisTickerLog);
    customPlot->yAxis->setTicker(logTickerY);
    customPlot->yAxis->setNumberFormat("eb");
    customPlot->yAxis->setNumberPrecision(0);
    
    customPlot->yAxis2->setScaleType(QCPAxis::stLogarithmic);
    QSharedPointer<QCPAxisTickerLog> logTickerY2(new QCPAxisTickerLog);
    customPlot->yAxis2->setTicker(logTickerY2);
    customPlot->yAxis2->setNumberFormat("eb");
    customPlot->yAxis2->setNumberPrecision(0);
    
    // 默认显示子刻度线（对数坐标下特别有用）
    customPlot->xAxis->grid()->setSubGridVisible(true);
    customPlot->yAxis->grid()->setSubGridVisible(true);
    
    // 设置网格线样式
    customPlot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::SolidLine));
    customPlot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::SolidLine));
    customPlot->xAxis->grid()->setSubGridPen(QPen(QColor(180, 180, 180), 1, Qt::DotLine));
    customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(180, 180, 180), 1, Qt::DotLine));
    
    // 设置默认坐标轴范围
    customPlot->xAxis->setRange(0.1, 10000);
    customPlot->xAxis2->setRange(0.1, 10000);
    customPlot->yAxis->setRange(10, 100000);
    customPlot->yAxis2->setRange(10, 100000);
    
    // 连接坐标轴范围同步：当X/Y轴范围改变时，同步X2/Y2轴
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));
    
    // 默认启用X轴反转
    customPlot->xAxis->setRangeReversed(true);
    customPlot->xAxis2->setRangeReversed(true);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    
    setupLeftPanel();
    setupCenterPanel();
    setupRightPanel();
    
    mainLayout->addWidget(curveList->parentWidget(), 1);
    mainLayout->addWidget(customPlot, 3);
    mainLayout->addWidget(rightPanel, 1);
    
    setWindowTitle("CSV曲线分析工具 v1.0");
    showMaximized();  // 默认最大化显示（全屏）
}

void MainWindow::setupLeftPanel()
{
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    
    QLabel* lblTitle = new QLabel("曲线列表");
    lblTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    curveList = new QListWidget();
    connect(curveList, &QListWidget::currentRowChanged, this, &MainWindow::onCurveSelected);
    
    btnAddCurve = new QPushButton("+ 新增曲线");
    btnDeleteCurve = new QPushButton("- 删除曲线");
    
    connect(btnAddCurve, &QPushButton::clicked, this, &MainWindow::onAddCurve);
    connect(btnDeleteCurve, &QPushButton::clicked, this, &MainWindow::onDeleteCurve);
    
    leftLayout->addWidget(lblTitle);
    leftLayout->addWidget(curveList);
    leftLayout->addWidget(btnAddCurve);
    leftLayout->addWidget(btnDeleteCurve);
}

void MainWindow::setupCenterPanel()
{
    customPlot = new QCustomPlot();
}

void MainWindow::setupRightPanel()
{
    rightPanel = new QWidget();
    QVBoxLayout* mainRightLayout = new QVBoxLayout(rightPanel);
    mainRightLayout->setContentsMargins(0, 0, 0, 0);
    
    // ========== 上部分：曲线属性 ==========
    QGroupBox* curveGroup = new QGroupBox("曲线属性");
    QVBoxLayout* curveGroupLayout = new QVBoxLayout(curveGroup);
    
    // 使用 QTabWidget 分类显示曲线属性
    QTabWidget* curveTabWidget = new QTabWidget();
    
    // ========== 曲线标签页 1：数据源 ==========
    QWidget* dataTab = new QWidget();
    QFormLayout* dataLayout = new QFormLayout(dataTab);
    
    edtCurveName = new QLineEdit();
    edtCsvPath = new QLineEdit();
    edtCsvPath->setReadOnly(true);
    btnSelectCsv = new QPushButton("浏览...");
    
    QHBoxLayout* csvLayout = new QHBoxLayout();
    csvLayout->addWidget(edtCsvPath);
    csvLayout->addWidget(btnSelectCsv);
    
    cmbXColumn = new QComboBox();
    cmbYColumn = new QComboBox();
    
    dataLayout->addRow("曲线名称:", edtCurveName);
    dataLayout->addRow("CSV文件:", csvLayout);
    dataLayout->addRow("X列:", cmbXColumn);
    dataLayout->addRow("Y列:", cmbYColumn);
    
    curveTabWidget->addTab(dataTab, "数据源");
    
    // ========== 曲线标签页 2：样式设置 ==========
    QWidget* styleTab = new QWidget();
    QFormLayout* styleLayout = new QFormLayout(styleTab);
    
    btnCurveColor = new QPushButton("选择颜色");
    btnCurveColor->setMinimumHeight(30);
    
    cmbLineStyle = new QComboBox();
    cmbLineStyle->addItem("无线型", static_cast<int>(Qt::NoPen));
    cmbLineStyle->addItem("实线", static_cast<int>(Qt::SolidLine));
    cmbLineStyle->addItem("虚线", static_cast<int>(Qt::DashLine));
    cmbLineStyle->addItem("点线", static_cast<int>(Qt::DotLine));
    cmbLineStyle->addItem("点划线", static_cast<int>(Qt::DashDotLine));
    cmbLineStyle->addItem("双点划线", static_cast<int>(Qt::DashDotDotLine));
    
    spinLineWidth = new QDoubleSpinBox();
    spinLineWidth->setMinimum(0.5);
    spinLineWidth->setMaximum(10.0);
    spinLineWidth->setSingleStep(0.5);
    spinLineWidth->setValue(1.0);
    
    cmbScatterShape = new QComboBox();
    cmbScatterShape->addItem("无散点", static_cast<int>(QCPScatterStyle::ssNone));
    cmbScatterShape->addItem("实心圆", static_cast<int>(QCPScatterStyle::ssDisc));
    cmbScatterShape->addItem("空心圆", static_cast<int>(QCPScatterStyle::ssCircle));
    cmbScatterShape->addItem("实心方形", static_cast<int>(QCPScatterStyle::ssSquare));
    cmbScatterShape->addItem("十字", static_cast<int>(QCPScatterStyle::ssCross));
    cmbScatterShape->addItem("加号", static_cast<int>(QCPScatterStyle::ssPlus));
    cmbScatterShape->addItem("星形", static_cast<int>(QCPScatterStyle::ssStar));
    cmbScatterShape->addItem("三角形", static_cast<int>(QCPScatterStyle::ssTriangle));
    
    spinScatterSize = new QDoubleSpinBox();
    spinScatterSize->setMinimum(1.0);
    spinScatterSize->setMaximum(20.0);
    spinScatterSize->setSingleStep(1.0);
    spinScatterSize->setValue(6.0);
    
    styleLayout->addRow("线条颜色:", btnCurveColor);
    styleLayout->addRow("线型:", cmbLineStyle);
    styleLayout->addRow("线宽:", spinLineWidth);
    styleLayout->addRow("散点样式:", cmbScatterShape);
    styleLayout->addRow("散点大小:", spinScatterSize);
    
    curveTabWidget->addTab(styleTab, "样式设置");
    
    // 将 TabWidget 添加到曲线属性分组
    curveGroupLayout->addWidget(curveTabWidget);
    
    // ========== 拉点功能（独立分组） ==========
    QGroupBox* dragGroup = new QGroupBox("拉点功能");
    QVBoxLayout* dragLayout = new QVBoxLayout(dragGroup);
    
    chkDragMode = new QCheckBox("启用拉点模式");
    chkDragMode->setStyleSheet("font-weight: bold; color: #0066cc;");
    
    lblDragStatus = new QLabel("状态：未启用");
    lblDragStatus->setStyleSheet("color: #666;");
    
    QHBoxLayout* dragBtnLayout1 = new QHBoxLayout();
    btnUndo = new QPushButton("← 撤销");
    btnRedo = new QPushButton("重做 →");
    btnUndo->setEnabled(false);
    btnRedo->setEnabled(false);
    dragBtnLayout1->addWidget(btnUndo);
    dragBtnLayout1->addWidget(btnRedo);
    
    QHBoxLayout* dragBtnLayout2 = new QHBoxLayout();
    btnSaveData = new QPushButton("💾 保存修改");
    btnResetData = new QPushButton("↺ 重置数据");
    btnSaveData->setEnabled(false);
    btnResetData->setEnabled(false);
    btnSaveData->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    btnResetData->setStyleSheet("background-color: #f44336; color: white;");
    dragBtnLayout2->addWidget(btnSaveData);
    dragBtnLayout2->addWidget(btnResetData);
    
    QLabel* lblDragTip = new QLabel("提示：启用后点击数据点并上下拖动");
    lblDragTip->setStyleSheet("font-size: 10px; color: #999; font-style: italic;");
    lblDragTip->setWordWrap(true);
    
    dragLayout->addWidget(chkDragMode);
    dragLayout->addWidget(lblDragStatus);
    dragLayout->addLayout(dragBtnLayout1);
    dragLayout->addLayout(dragBtnLayout2);
    dragLayout->addWidget(lblDragTip);
    
    curveGroupLayout->addWidget(dragGroup);
    
    // ========== 下部分：图表属性 ==========
    QGroupBox* plotGroup = new QGroupBox("图表属性");
    QVBoxLayout* plotGroupLayout = new QVBoxLayout(plotGroup);
    
    // 使用 QTabWidget 分类显示
    QTabWidget* tabWidget = new QTabWidget();
    
    // ========== 标签页 1：基本信息 ==========
    QWidget* basicTab = new QWidget();
    QVBoxLayout* basicTabLayout = new QVBoxLayout(basicTab);
    
    // 标题和轴标签
    QFormLayout* basicLayout = new QFormLayout();
    
    edtPlotTitle = new QLineEdit("图表示例标题");
    edtPlotTitle->setReadOnly(true);
    QPushButton* btnEditPlotTitle = new QPushButton("编辑...");
    btnEditPlotTitle->setMaximumWidth(60);
    QHBoxLayout* plotTitleLayout = new QHBoxLayout();
    plotTitleLayout->addWidget(edtPlotTitle);
    plotTitleLayout->addWidget(btnEditPlotTitle);
    
    edtXAxisLabel = new QLineEdit("X轴");
    edtXAxisLabel->setReadOnly(true);
    QPushButton* btnEditXAxis = new QPushButton("编辑...");
    btnEditXAxis->setMaximumWidth(60);
    QHBoxLayout* xAxisLabelLayout = new QHBoxLayout();
    xAxisLabelLayout->addWidget(edtXAxisLabel);
    xAxisLabelLayout->addWidget(btnEditXAxis);
    
    edtYAxisLabel = new QLineEdit("Y轴");
    edtYAxisLabel->setReadOnly(true);
    QPushButton* btnEditYAxis = new QPushButton("编辑...");
    btnEditYAxis->setMaximumWidth(60);
    QHBoxLayout* yAxisLabelLayout = new QHBoxLayout();
    yAxisLabelLayout->addWidget(edtYAxisLabel);
    yAxisLabelLayout->addWidget(btnEditYAxis);
    
    basicLayout->addRow("图表标题:", plotTitleLayout);
    basicLayout->addRow("X轴标签:", xAxisLabelLayout);
    basicLayout->addRow("Y轴标签:", yAxisLabelLayout);
    
    connect(btnEditPlotTitle, &QPushButton::clicked, this, &MainWindow::onEditPlotTitle);
    connect(btnEditXAxis, &QPushButton::clicked, this, &MainWindow::onEditXAxisLabel);
    connect(btnEditYAxis, &QPushButton::clicked, this, &MainWindow::onEditYAxisLabel);
    
    basicTabLayout->addLayout(basicLayout);
    
    // 坐标轴范围
    QGroupBox* rangeGroup = new QGroupBox("坐标轴范围");
    QFormLayout* rangeLayout = new QFormLayout(rangeGroup);
    
    spinXMin = new QDoubleSpinBox();
    spinXMin->setRange(-1e10, 1e10);
    spinXMin->setDecimals(10);
    spinXMin->setValue(0.1);
    
    spinXMax = new QDoubleSpinBox();
    spinXMax->setRange(-1e10, 1e10);
    spinXMax->setDecimals(10);
    spinXMax->setValue(10000);
    
    spinYMin = new QDoubleSpinBox();
    spinYMin->setRange(-1e10, 1e10);
    spinYMin->setDecimals(10);
    spinYMin->setValue(10);
    
    spinYMax = new QDoubleSpinBox();
    spinYMax->setRange(-1e10, 1e10);
    spinYMax->setDecimals(10);
    spinYMax->setValue(100000);
    
    btnApplyRange = new QPushButton("应用范围");
    btnAutoRange = new QPushButton("自动范围");
    
    QHBoxLayout* xRangeLayout = new QHBoxLayout();
    xRangeLayout->addWidget(new QLabel("最小:"));
    xRangeLayout->addWidget(spinXMin);
    xRangeLayout->addWidget(new QLabel("最大:"));
    xRangeLayout->addWidget(spinXMax);
    
    QHBoxLayout* yRangeLayout = new QHBoxLayout();
    yRangeLayout->addWidget(new QLabel("最小:"));
    yRangeLayout->addWidget(spinYMin);
    yRangeLayout->addWidget(new QLabel("最大:"));
    yRangeLayout->addWidget(spinYMax);
    
    rangeLayout->addRow("X轴范围:", xRangeLayout);
    rangeLayout->addRow("Y轴范围:", yRangeLayout);
    
    QHBoxLayout* rangeBtnLayout = new QHBoxLayout();
    rangeBtnLayout->addWidget(btnApplyRange);
    rangeBtnLayout->addWidget(btnAutoRange);
    rangeLayout->addRow("", rangeBtnLayout);
    
    basicTabLayout->addWidget(rangeGroup);
    basicTabLayout->addStretch();
    
    tabWidget->addTab(basicTab, "基本信息");
    
    // ========== 标签页 2：坐标轴设置 ==========
    QWidget* axisTab = new QWidget();
    QVBoxLayout* axisTabLayout = new QVBoxLayout(axisTab);
    
    QGridLayout* axisLayout = new QGridLayout();
    
    cmbXAxisScaleType = new QComboBox();
    cmbXAxisScaleType->addItem("线性", 0);
    cmbXAxisScaleType->addItem("对数", 1);
    cmbXAxisScaleType->setCurrentIndex(1);
    
    cmbYAxisScaleType = new QComboBox();
    cmbYAxisScaleType->addItem("线性", 0);
    cmbYAxisScaleType->addItem("对数", 1);
    cmbYAxisScaleType->setCurrentIndex(1);
    
    chkXAxisTickLabels = new QCheckBox();
    chkXAxisTickLabels->setChecked(true);
    chkYAxisTickLabels = new QCheckBox();
    chkYAxisTickLabels->setChecked(true);
    
    chkShowX2Axis = new QCheckBox();
    chkShowX2Axis->setChecked(true);
    chkShowY2Axis = new QCheckBox();
    chkShowY2Axis->setChecked(true);
    
    chkX2AxisTickLabels = new QCheckBox();
    chkX2AxisTickLabels->setChecked(false);
    chkY2AxisTickLabels = new QCheckBox();
    chkY2AxisTickLabels->setChecked(false);
    
    axisLayout->addWidget(new QLabel(""), 0, 0);
    axisLayout->addWidget(new QLabel("<b>X轴</b>"), 0, 1, Qt::AlignCenter);
    axisLayout->addWidget(new QLabel("<b>Y轴</b>"), 0, 2, Qt::AlignCenter);
    
    axisLayout->addWidget(new QLabel("坐标类型:"), 1, 0);
    axisLayout->addWidget(cmbXAxisScaleType, 1, 1);
    axisLayout->addWidget(cmbYAxisScaleType, 1, 2);
    
    axisLayout->addWidget(new QLabel("显示刻度值:"), 2, 0);
    axisLayout->addWidget(chkXAxisTickLabels, 2, 1);
    axisLayout->addWidget(chkYAxisTickLabels, 2, 2);
    
    axisLayout->addWidget(new QLabel("显示副轴:"), 3, 0);
    axisLayout->addWidget(chkShowX2Axis, 3, 1);
    axisLayout->addWidget(chkShowY2Axis, 3, 2);
    
    axisLayout->addWidget(new QLabel("副轴刻度值:"), 4, 0);
    axisLayout->addWidget(chkX2AxisTickLabels, 4, 1);
    axisLayout->addWidget(chkY2AxisTickLabels, 4, 2);
    
    // 添加X轴反转选项
    chkXAxisReversed = new QCheckBox();
    chkXAxisReversed->setChecked(true);  // 默认启用反转
    
    axisLayout->addWidget(new QLabel("X轴反转:"), 5, 0);
    axisLayout->addWidget(chkXAxisReversed, 5, 1);
    
    axisTabLayout->addLayout(axisLayout);
    axisTabLayout->addStretch();
    
    tabWidget->addTab(axisTab, "坐标轴设置");
    
    // ========== 标签页 3：显示选项 ==========
    QWidget* displayTab = new QWidget();
    QFormLayout* displayLayout = new QFormLayout(displayTab);
    
    chkShowGrid = new QCheckBox();
    chkShowGrid->setChecked(true);
    chkShowMinorGrid = new QCheckBox();
    chkShowMinorGrid->setChecked(true);
    chkShowLegend = new QCheckBox();
    chkShowLegend->setChecked(true);
    
    displayLayout->addRow("显示网格:", chkShowGrid);
    displayLayout->addRow("显示子刻度线:", chkShowMinorGrid);
    displayLayout->addRow("显示图例:", chkShowLegend);
    
    tabWidget->addTab(displayTab, "显示选项");
    
    // 将 TabWidget 添加到图表属性分组
    plotGroupLayout->addWidget(tabWidget);
    
    // ========== 导出功能（独立区域） ==========
    QGroupBox* exportGroup = new QGroupBox("导出功能");
    QVBoxLayout* exportLayout = new QVBoxLayout(exportGroup);
    exportLayout->setContentsMargins(8, 8, 8, 8);
    exportLayout->setSpacing(6);
    
    QPushButton* btnExport = new QPushButton("🖼️ 导出为图片");
    btnExport->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; font-size: 12px; border-radius: 4px; } QPushButton:hover { background-color: #1976D2; }");
    btnExport->setMinimumHeight(35);
    
    QLabel* lblExportInfo = new QLabel("• 600x600px  • 缩放2.0x  • 质量95");
    lblExportInfo->setStyleSheet("color: #888; font-size: 10px;");
    lblExportInfo->setAlignment(Qt::AlignCenter);
    
    exportLayout->addWidget(btnExport);
    exportLayout->addWidget(lblExportInfo);
    
    connect(btnExport, &QPushButton::clicked, this, &MainWindow::onExportImage);
    
    plotGroupLayout->addWidget(exportGroup);
    
    // 组装主布局
    mainRightLayout->addWidget(curveGroup, 1); // 曲线属性占据1份
    mainRightLayout->addWidget(plotGroup, 2);  // 图表属性占据2份
    
    // 连接信号
    connect(edtCurveName, &QLineEdit::textChanged, this, &MainWindow::onCurveNameChanged);
    connect(btnSelectCsv, &QPushButton::clicked, this, &MainWindow::onSelectCsvFile);
    connect(cmbXColumn, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onXColumnChanged);
    connect(cmbYColumn, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onYColumnChanged);
    connect(btnCurveColor, &QPushButton::clicked, this, &MainWindow::onCurveColorChanged);
    connect(cmbLineStyle, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onCurveLineStyleChanged);
    connect(spinLineWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onCurveLineWidthChanged);
    connect(cmbScatterShape, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onCurveScatterShapeChanged);
    connect(spinScatterSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onCurveScatterSizeChanged);
    
    connect(edtPlotTitle, &QLineEdit::textChanged, this, &MainWindow::onPlotTitleChanged);
    connect(edtXAxisLabel, &QLineEdit::textChanged, this, &MainWindow::onXAxisLabelChanged);
    connect(edtYAxisLabel, &QLineEdit::textChanged, this, &MainWindow::onYAxisLabelChanged);
    connect(chkShowGrid, &QCheckBox::stateChanged, this, &MainWindow::onShowGridChanged);
    connect(chkShowLegend, &QCheckBox::stateChanged, this, &MainWindow::onShowLegendChanged);
    connect(chkShowMinorGrid, &QCheckBox::stateChanged, this, &MainWindow::onShowMinorGridChanged);
    connect(chkShowX2Axis, &QCheckBox::stateChanged, this, &MainWindow::onShowX2AxisChanged);
    connect(chkShowY2Axis, &QCheckBox::stateChanged, this, &MainWindow::onShowY2AxisChanged);
    connect(cmbXAxisScaleType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onXAxisScaleTypeChanged);
    connect(cmbYAxisScaleType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onYAxisScaleTypeChanged);
    connect(chkXAxisTickLabels, &QCheckBox::stateChanged, this, &MainWindow::onXAxisTickLabelsChanged);
    connect(chkYAxisTickLabels, &QCheckBox::stateChanged, this, &MainWindow::onYAxisTickLabelsChanged);
    connect(chkX2AxisTickLabels, &QCheckBox::stateChanged, this, &MainWindow::onX2AxisTickLabelsChanged);
    connect(chkY2AxisTickLabels, &QCheckBox::stateChanged, this, &MainWindow::onY2AxisTickLabelsChanged);
    connect(chkXAxisReversed, &QCheckBox::stateChanged, this, &MainWindow::onXAxisReversedChanged);
    connect(btnApplyRange, &QPushButton::clicked, this, &MainWindow::onAxisRangeChanged);
    connect(btnAutoRange, &QPushButton::clicked, this, [this]() {
        customPlot->rescaleAxes();
        
        // 同步X2轴和Y2轴的范围
        customPlot->xAxis2->setRange(customPlot->xAxis->range());
        customPlot->yAxis2->setRange(customPlot->yAxis->range());
        
        // 更新输入框显示
        spinXMin->setValue(customPlot->xAxis->range().lower);
        spinXMax->setValue(customPlot->xAxis->range().upper);
        spinYMin->setValue(customPlot->yAxis->range().lower);
        spinYMax->setValue(customPlot->yAxis->range().upper);
        customPlot->replot();
    });
    
    // 拉点功能信号连接
    connect(chkDragMode, &QCheckBox::toggled, this, &MainWindow::onDragModeToggled);
    connect(btnSaveData, &QPushButton::clicked, this, &MainWindow::onSaveModifiedData);
    connect(btnUndo, &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(btnRedo, &QPushButton::clicked, this, &MainWindow::onRedo);
    connect(btnResetData, &QPushButton::clicked, this, &MainWindow::onResetData);
    
    // 鼠标事件连接
    connect(customPlot, &QCustomPlot::mousePress, this, &MainWindow::onPlotMousePress);
    connect(customPlot, &QCustomPlot::mouseMove, this, &MainWindow::onPlotMouseMove);
    connect(customPlot, &QCustomPlot::mouseRelease, this, &MainWindow::onPlotMouseRelease);
}

void MainWindow::onAddCurve()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择CSV文件", "", "CSV文件 (*.csv);;所有文件 (*)");
    if (fileName.isEmpty())
        return;
    
    CurveData newCurve;
    newCurve.name = QString("曲线 %1").arg(curves.size() + 1);
    newCurve.csvFilePath = fileName;
    newCurve.xColumn = 0;
    newCurve.yColumn = 1;
    newCurve.color = QColor(Qt::GlobalColor(Qt::blue + (curves.size() % 5)));
    newCurve.lineStyle = Qt::NoPen;  // 默认无线型
    newCurve.lineWidth = 1.0;
    newCurve.scatterShape = QCPScatterStyle::ssDisc;  // 默认实心圆
    newCurve.scatterSize = 6.0;
    newCurve.modified = false;  // 初始未修改
    
    // 尝试加载数据，如果失败也不报错，只是数据为空
    loadCSV(newCurve.csvFilePath, newCurve.xColumn, newCurve.yColumn, 
            newCurve.xData, newCurve.yData, newCurve.rawDataLines, 
            newCurve.hasHeader, newCurve.headerLine);
    
    newCurve.graph = customPlot->addGraph();
    newCurve.graph->setData(newCurve.xData, newCurve.yData);
    newCurve.graph->setName(newCurve.name);
    newCurve.graph->setPen(QPen(newCurve.color, newCurve.lineWidth, newCurve.lineStyle));
    newCurve.graph->setScatterStyle(QCPScatterStyle(newCurve.scatterShape, newCurve.color, newCurve.color, newCurve.scatterSize));
    newCurve.graph->setSelectable(QCP::stMultipleDataRanges);  // 设置为可选择
    newCurve.graph->selectionDecorator()->setPen(QPen(Qt::red, 2));  // 选中时用红色高亮
    
    curves.append(newCurve);
    curveList->addItem(newCurve.name);
    
    // 如果需要则自动调整范围
    autoRescaleIfNeeded();
    
    customPlot->replot();
    
    curveList->setCurrentRow(curves.size() - 1);
}

void MainWindow::onDeleteCurve()
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    customPlot->removeGraph(curves[currentCurveIndex].graph);
    curves.removeAt(currentCurveIndex);
    delete curveList->takeItem(currentCurveIndex);
    
    customPlot->replot();
    
    if (curves.isEmpty()) {
        currentCurveIndex = -1;
        updateCurveProperties();
    }
}

void MainWindow::onCurveSelected()
{
    currentCurveIndex = curveList->currentRow();
    updateCurveProperties();
}

void MainWindow::updateCurveProperties()
{
    bool hasSelection = currentCurveIndex >= 0 && currentCurveIndex < curves.size();
    
    edtCurveName->setEnabled(hasSelection);
    edtCsvPath->setEnabled(hasSelection);
    btnSelectCsv->setEnabled(hasSelection);
    cmbXColumn->setEnabled(hasSelection);
    cmbYColumn->setEnabled(hasSelection);
    btnCurveColor->setEnabled(hasSelection);
    cmbLineStyle->setEnabled(hasSelection);
    spinLineWidth->setEnabled(hasSelection);
    cmbScatterShape->setEnabled(hasSelection);
    spinScatterSize->setEnabled(hasSelection);
    
    if (hasSelection) {
        const CurveData& curve = curves[currentCurveIndex];
        
        // 阻塞信号，防止触发名称改变
        edtCurveName->blockSignals(true);
        edtCurveName->setText(curve.name);
        edtCurveName->blockSignals(false);
        
        edtCsvPath->setText(curve.csvFilePath);
        
        // 更新列选择下拉框
        updateColumnComboBoxes(curve.csvFilePath);
        cmbXColumn->setCurrentIndex(curve.xColumn);
        cmbYColumn->setCurrentIndex(curve.yColumn);
        
        QString colorStyle = QString("background-color: %1;").arg(curve.color.name());
        btnCurveColor->setStyleSheet(colorStyle);
        
        int styleIndex = cmbLineStyle->findData(static_cast<int>(curve.lineStyle));
        cmbLineStyle->setCurrentIndex(styleIndex);
        spinLineWidth->setValue(curve.lineWidth);
        
        int scatterIndex = cmbScatterShape->findData(static_cast<int>(curve.scatterShape));
        cmbScatterShape->setCurrentIndex(scatterIndex);
        spinScatterSize->setValue(curve.scatterSize);
    } else {
        edtCurveName->clear();
        edtCsvPath->clear();
        btnCurveColor->setStyleSheet("");
    }
}

void MainWindow::onCurveColorChanged()
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    QColor color = QColorDialog::getColor(curves[currentCurveIndex].color, this, "选择曲线颜色");
    if (!color.isValid())
        return;
    
    curves[currentCurveIndex].color = color;
    curves[currentCurveIndex].graph->setPen(QPen(color, curves[currentCurveIndex].lineWidth, curves[currentCurveIndex].lineStyle));
    curves[currentCurveIndex].graph->setScatterStyle(QCPScatterStyle(curves[currentCurveIndex].scatterShape, color, color, curves[currentCurveIndex].scatterSize));
    
    QString colorStyle = QString("background-color: %1;").arg(color.name());
    btnCurveColor->setStyleSheet(colorStyle);
    
    customPlot->replot();
}

void MainWindow::onCurveLineStyleChanged(int index)
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    Qt::PenStyle style = static_cast<Qt::PenStyle>(cmbLineStyle->currentData().toInt());
    curves[currentCurveIndex].lineStyle = style;
    curves[currentCurveIndex].graph->setPen(QPen(curves[currentCurveIndex].color, curves[currentCurveIndex].lineWidth, style));
    
    customPlot->replot();
}

void MainWindow::onCurveLineWidthChanged(double value)
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    curves[currentCurveIndex].lineWidth = value;
    curves[currentCurveIndex].graph->setPen(QPen(curves[currentCurveIndex].color, value, curves[currentCurveIndex].lineStyle));
    
    customPlot->replot();
}

void MainWindow::onCurveScatterShapeChanged(int index)
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    QCPScatterStyle::ScatterShape shape = static_cast<QCPScatterStyle::ScatterShape>(cmbScatterShape->currentData().toInt());
    curves[currentCurveIndex].scatterShape = shape;
    curves[currentCurveIndex].graph->setScatterStyle(QCPScatterStyle(shape, curves[currentCurveIndex].color, curves[currentCurveIndex].color, curves[currentCurveIndex].scatterSize));
    
    customPlot->replot();
}

void MainWindow::onCurveScatterSizeChanged(double value)
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    curves[currentCurveIndex].scatterSize = value;
    curves[currentCurveIndex].graph->setScatterStyle(QCPScatterStyle(curves[currentCurveIndex].scatterShape, curves[currentCurveIndex].color, curves[currentCurveIndex].color, value));
    
    customPlot->replot();
}

void MainWindow::onCurveNameChanged()
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    QString newName = edtCurveName->text();
    curves[currentCurveIndex].name = newName;
    
    // 同步更新左侧列表
    curveList->item(currentCurveIndex)->setText(newName);
    
    // 同步更新图表图例
    curves[currentCurveIndex].graph->setName(newName);
    
    customPlot->replot();
}

void MainWindow::onSelectCsvFile()
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    QString fileName = QFileDialog::getOpenFileName(this, "选择CSV文件", "", "CSV文件 (*.csv);;所有文件 (*)");
    if (fileName.isEmpty())
        return;
    
    curves[currentCurveIndex].csvFilePath = fileName;
    edtCsvPath->setText(fileName);
    
    // 更新列选择下拉框
    updateColumnComboBoxes(fileName);
    
    // 同步更新列索引（防止超出范围）
    curves[currentCurveIndex].xColumn = cmbXColumn->currentIndex();
    curves[currentCurveIndex].yColumn = cmbYColumn->currentIndex();
    
    // 自动重新加载数据（失败也不报错，只是清空数据）
    CurveData& curve = curves[currentCurveIndex];
    loadCSV(curve.csvFilePath, curve.xColumn, curve.yColumn, curve.xData, curve.yData,
            curve.rawDataLines, curve.hasHeader, curve.headerLine);
    curve.graph->setData(curve.xData, curve.yData);
    
    // 如果需要则自动调整范围
    autoRescaleIfNeeded();
    
    customPlot->replot();
}

void MainWindow::onXColumnChanged(int value)
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    curves[currentCurveIndex].xColumn = cmbXColumn->currentIndex();
    
    // 自动重新加载数据（失败也不报错，只是清空数据）
    CurveData& curve = curves[currentCurveIndex];
    loadCSV(curve.csvFilePath, curve.xColumn, curve.yColumn, curve.xData, curve.yData,
            curve.rawDataLines, curve.hasHeader, curve.headerLine);
    curve.graph->setData(curve.xData, curve.yData);
    
    // 如果需要则自动调整范围
    autoRescaleIfNeeded();
    
    customPlot->replot();
}

void MainWindow::onYColumnChanged(int value)
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    curves[currentCurveIndex].yColumn = cmbYColumn->currentIndex();
    
    // 自动重新加载数据（失败也不报错，只是清空数据）
    CurveData& curve = curves[currentCurveIndex];
    loadCSV(curve.csvFilePath, curve.xColumn, curve.yColumn, curve.xData, curve.yData,
            curve.rawDataLines, curve.hasHeader, curve.headerLine);
    curve.graph->setData(curve.xData, curve.yData);
    
    // 如果需要则自动调整范围
    autoRescaleIfNeeded();
    
    customPlot->replot();
}

bool MainWindow::hasAnyValidData()
{
    // 检查所有曲线是否至少有一条有有效数据
    for (const CurveData& curve : curves) {
        if (!curve.xData.isEmpty() && !curve.yData.isEmpty()) {
            return true;
        }
    }
    return false;
}

void MainWindow::autoRescaleIfNeeded()
{
    // 只在第一次有有效数据时自动调整范围
    if (hasAutoRescaled) {
        return;  // 已经调整过了，不再重复调整
    }
    
    if (!hasAnyValidData()) {
        return;  // 没有任何数据，不调整
    }
    
    // 第一次有数据，自动调整
    customPlot->rescaleAxes();
    
    // 同步X2轴和Y2轴的范围
    customPlot->xAxis2->setRange(customPlot->xAxis->range());
    customPlot->yAxis2->setRange(customPlot->yAxis->range());
    
    // 更新范围输入框
    spinXMin->setValue(customPlot->xAxis->range().lower);
    spinXMax->setValue(customPlot->xAxis->range().upper);
    spinYMin->setValue(customPlot->yAxis->range().lower);
    spinYMax->setValue(customPlot->yAxis->range().upper);
    
    // 标记已经调整过
    hasAutoRescaled = true;
}

bool MainWindow::loadCSV(const QString& filePath, int xCol, int yCol, QVector<double>& xData, QVector<double>& yData,
                         QVector<QStringList>& rawData, bool& hasHeader, QStringList& header)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    
    xData.clear();
    yData.clear();
    rawData.clear();
    header.clear();
    hasHeader = false;
    
    QTextStream in(&file);
    int lineNumber = 0;
    int skippedLines = 0;
    int validDataLines = 0;
    int filteredLogPoints = 0;  // 记录因对数坐标轴被过滤的点数
    bool firstLineIsHeader = false;
    
    // 检查是否为对数X轴
    bool isLogX = (customPlot->xAxis->scaleType() == QCPAxis::stLogarithmic);
    
    // 读取第一行判断是否为表头
    if (!in.atEnd()) {
        QString firstLine = in.readLine();
        lineNumber++;
        QStringList parts = firstLine.split(',');
        
        if (parts.size() > qMax(xCol, yCol)) {
            // 检查第一行是否为数字
            bool okX, okY;
            parts[xCol].trimmed().toDouble(&okX);
            parts[yCol].trimmed().toDouble(&okY);
            
            if (!okX || !okY) {
                // 第一行不是数字，可能是表头
                firstLineIsHeader = true;
                hasHeader = true;
                header = parts;
                skippedLines++;
            } else {
                // 第一行就是数据
                double x = parts[xCol].trimmed().toDouble();
                double y = parts[yCol].trimmed().toDouble();
                
                // 对数坐标轴下X必须>0
                if (isLogX && x <= 0) {
                    filteredLogPoints++;
                    skippedLines++;
                } else {
                    xData.append(x);
                    yData.append(y);
                    rawData.append(parts);  // 保存原始数据
                    validDataLines++;
                }
            }
        } else {
            skippedLines++;
        }
    }
    
    // 读取剩余数据行
    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNumber++;
        
        // 跳过空行
        if (line.trimmed().isEmpty()) {
            skippedLines++;
            continue;
        }
        
        QStringList parts = line.split(',');
        
        // 检查列索引是否有效
        if (parts.size() <= qMax(xCol, yCol)) {
            skippedLines++;
            continue;
        }
        
        // 尝试转换X和Y列的数据
        bool okX, okY;
        double x = parts[xCol].trimmed().toDouble(&okX);
        double y = parts[yCol].trimmed().toDouble(&okY);
        
        // 只有当X和Y都能成功转换为数字时才添加数据点
        if (okX && okY) {
            // 对数坐标轴下X必须>0
            if (isLogX && x <= 0) {
                filteredLogPoints++;
                skippedLines++;
            } else {
                xData.append(x);
                yData.append(y);
                rawData.append(parts);  // 保存原始数据
                validDataLines++;
            }
        } else {
            skippedLines++;
        }
    }
    
    file.close();
    
    // 如果有被过滤的对数坐标点，显示提示
    if (filteredLogPoints > 0) {
        QMessageBox::warning(nullptr, "对数坐标轴数据过滤", 
            QString("对数X轴下检测到 %1 个 X≤0 的数据点。\n\n"
                    "这些点无法在对数坐标轴上显示，已自动过滤。\n\n"
                    "有效数据点：%2").arg(filteredLogPoints).arg(validDataLines));
    }
    
    return !xData.isEmpty();
}

void MainWindow::onPlotTitleChanged()
{
    // 删除旧标题（如果存在）
    if (customPlot->plotLayout()->elementAt(0) != nullptr && 
        dynamic_cast<QCPTextElement*>(customPlot->plotLayout()->elementAt(0))) {
        customPlot->plotLayout()->remove(customPlot->plotLayout()->elementAt(0));
        customPlot->plotLayout()->simplify();
    }
    
    // 如果标题不为空，添加新标题
    if (!edtPlotTitle->text().isEmpty()) {
        customPlot->plotLayout()->insertRow(0);
        QCPTextElement* title = new QCPTextElement(customPlot, edtPlotTitle->text());
        title->setFont(plotTitleFont);  // 使用保存的字体
        customPlot->plotLayout()->addElement(0, 0, title);
    }
    
    customPlot->replot();
}

void MainWindow::onXAxisLabelChanged()
{
    customPlot->xAxis->setLabel(edtXAxisLabel->text());
    customPlot->xAxis->setLabelFont(xAxisLabelFont);  // 设置字体
    customPlot->replot();
}

void MainWindow::onYAxisLabelChanged()
{
    customPlot->yAxis->setLabel(edtYAxisLabel->text());
    customPlot->yAxis->setLabelFont(yAxisLabelFont);  // 设置字体
    customPlot->replot();
}

void MainWindow::onEditPlotTitle()
{
    // 创建富文本编辑对话框
    QDialog dialog(this);
    dialog.setWindowTitle("编辑图表标题");
    dialog.resize(500, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // 工具栏
    QToolBar* toolbar = new QToolBar();
    
    // 字体选择
    QFontComboBox* fontCombo = new QFontComboBox();
    fontCombo->setCurrentFont(plotTitleFont);
    toolbar->addWidget(fontCombo);
    
    // 字体大小
    QSpinBox* fontSize = new QSpinBox();
    fontSize->setRange(6, 72);
    fontSize->setValue(plotTitleFont.pointSize());
    fontSize->setSuffix(" pt");
    toolbar->addWidget(fontSize);
    
    toolbar->addSeparator();
    
    // 粗体
    QAction* boldAction = toolbar->addAction("粗体");
    boldAction->setCheckable(true);
    boldAction->setChecked(plotTitleFont.bold());
    
    // 斜体
    QAction* italicAction = toolbar->addAction("斜体");
    italicAction->setCheckable(true);
    italicAction->setChecked(plotTitleFont.italic());
    
    // 下划线
    QAction* underlineAction = toolbar->addAction("下划线");
    underlineAction->setCheckable(true);
    underlineAction->setChecked(plotTitleFont.underline());
    
    // 文本编辑器
    QTextEdit* textEdit = new QTextEdit();
    textEdit->setPlainText(edtPlotTitle->text());
    textEdit->setFont(plotTitleFont);
    
    // 按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("确定");
    QPushButton* cancelBtn = new QPushButton("取消");
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    
    layout->addWidget(toolbar);
    layout->addWidget(textEdit);
    layout->addLayout(btnLayout);
    
    // 连接信号
    connect(fontCombo, &QFontComboBox::currentFontChanged, [textEdit](const QFont& font) {
        QFont currentFont = textEdit->font();
        currentFont.setFamily(font.family());
        textEdit->setFont(currentFont);
    });
    
    connect(fontSize, QOverload<int>::of(&QSpinBox::valueChanged), [textEdit](int size) {
        QFont currentFont = textEdit->font();
        currentFont.setPointSize(size);
        textEdit->setFont(currentFont);
    });
    
    connect(boldAction, &QAction::toggled, [textEdit](bool checked) {
        QFont currentFont = textEdit->font();
        currentFont.setBold(checked);
        textEdit->setFont(currentFont);
    });
    
    connect(italicAction, &QAction::toggled, [textEdit](bool checked) {
        QFont currentFont = textEdit->font();
        currentFont.setItalic(checked);
        textEdit->setFont(currentFont);
    });
    
    connect(underlineAction, &QAction::toggled, [textEdit](bool checked) {
        QFont currentFont = textEdit->font();
        currentFont.setUnderline(checked);
        textEdit->setFont(currentFont);
    });
    
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        plotTitleFont = textEdit->font();
        edtPlotTitle->setText(textEdit->toPlainText());
        onPlotTitleChanged();
    }
}

void MainWindow::onEditXAxisLabel()
{
    // 创建富文本编辑对话框
    QDialog dialog(this);
    dialog.setWindowTitle("编辑X轴标签");
    dialog.resize(500, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // 工具栏
    QToolBar* toolbar = new QToolBar();
    
    QFontComboBox* fontCombo = new QFontComboBox();
    fontCombo->setCurrentFont(xAxisLabelFont);
    toolbar->addWidget(fontCombo);
    
    QSpinBox* fontSize = new QSpinBox();
    fontSize->setRange(6, 72);
    fontSize->setValue(xAxisLabelFont.pointSize());
    fontSize->setSuffix(" pt");
    toolbar->addWidget(fontSize);
    
    toolbar->addSeparator();
    
    QAction* boldAction = toolbar->addAction("粗体");
    boldAction->setCheckable(true);
    boldAction->setChecked(xAxisLabelFont.bold());
    
    QAction* italicAction = toolbar->addAction("斜体");
    italicAction->setCheckable(true);
    italicAction->setChecked(xAxisLabelFont.italic());
    
    QAction* underlineAction = toolbar->addAction("下划线");
    underlineAction->setCheckable(true);
    underlineAction->setChecked(xAxisLabelFont.underline());
    
    QTextEdit* textEdit = new QTextEdit();
    textEdit->setPlainText(edtXAxisLabel->text());
    textEdit->setFont(xAxisLabelFont);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("确定");
    QPushButton* cancelBtn = new QPushButton("取消");
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    
    layout->addWidget(toolbar);
    layout->addWidget(textEdit);
    layout->addLayout(btnLayout);
    
    connect(fontCombo, &QFontComboBox::currentFontChanged, [textEdit](const QFont& font) {
        QFont currentFont = textEdit->font();
        currentFont.setFamily(font.family());
        textEdit->setFont(currentFont);
    });
    
    connect(fontSize, QOverload<int>::of(&QSpinBox::valueChanged), [textEdit](int size) {
        QFont currentFont = textEdit->font();
        currentFont.setPointSize(size);
        textEdit->setFont(currentFont);
    });
    
    connect(boldAction, &QAction::toggled, [textEdit](bool checked) {
        QFont currentFont = textEdit->font();
        currentFont.setBold(checked);
        textEdit->setFont(currentFont);
    });
    
    connect(italicAction, &QAction::toggled, [textEdit](bool checked) {
        QFont currentFont = textEdit->font();
        currentFont.setItalic(checked);
        textEdit->setFont(currentFont);
    });
    
    connect(underlineAction, &QAction::toggled, [textEdit](bool checked) {
        QFont currentFont = textEdit->font();
        currentFont.setUnderline(checked);
        textEdit->setFont(currentFont);
    });
    
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        xAxisLabelFont = textEdit->font();
        edtXAxisLabel->setText(textEdit->toPlainText());
        onXAxisLabelChanged();
    }
}

void MainWindow::onEditYAxisLabel()
{
    // 创建富文本编辑对话框
    QDialog dialog(this);
    dialog.setWindowTitle("编辑Y轴标签");
    dialog.resize(500, 300);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // 工具栏
    QToolBar* toolbar = new QToolBar();
    
    QFontComboBox* fontCombo = new QFontComboBox();
    fontCombo->setCurrentFont(yAxisLabelFont);
    toolbar->addWidget(fontCombo);
    
    QSpinBox* fontSize = new QSpinBox();
    fontSize->setRange(6, 72);
    fontSize->setValue(yAxisLabelFont.pointSize());
    fontSize->setSuffix(" pt");
    toolbar->addWidget(fontSize);
    
    toolbar->addSeparator();
    
    QAction* boldAction = toolbar->addAction("粗体");
    boldAction->setCheckable(true);
    boldAction->setChecked(yAxisLabelFont.bold());
    
    QAction* italicAction = toolbar->addAction("斜体");
    italicAction->setCheckable(true);
    italicAction->setChecked(yAxisLabelFont.italic());
    
    QAction* underlineAction = toolbar->addAction("下划线");
    underlineAction->setCheckable(true);
    underlineAction->setChecked(yAxisLabelFont.underline());
    
    QTextEdit* textEdit = new QTextEdit();
    textEdit->setPlainText(edtYAxisLabel->text());
    textEdit->setFont(yAxisLabelFont);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("确定");
    QPushButton* cancelBtn = new QPushButton("取消");
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    
    layout->addWidget(toolbar);
    layout->addWidget(textEdit);
    layout->addLayout(btnLayout);
    
    connect(fontCombo, &QFontComboBox::currentFontChanged, [textEdit](const QFont& font) {
        QFont currentFont = textEdit->font();
        currentFont.setFamily(font.family());
        textEdit->setFont(currentFont);
    });
    
    connect(fontSize, QOverload<int>::of(&QSpinBox::valueChanged), [textEdit](int size) {
        QFont currentFont = textEdit->font();
        currentFont.setPointSize(size);
        textEdit->setFont(currentFont);
    });
    
    connect(boldAction, &QAction::toggled, [textEdit](bool checked) {
        QFont currentFont = textEdit->font();
        currentFont.setBold(checked);
        textEdit->setFont(currentFont);
    });
    
    connect(italicAction, &QAction::toggled, [textEdit](bool checked) {
        QFont currentFont = textEdit->font();
        currentFont.setItalic(checked);
        textEdit->setFont(currentFont);
    });
    
    connect(underlineAction, &QAction::toggled, [textEdit](bool checked) {
        QFont currentFont = textEdit->font();
        currentFont.setUnderline(checked);
        textEdit->setFont(currentFont);
    });
    
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        yAxisLabelFont = textEdit->font();
        edtYAxisLabel->setText(textEdit->toPlainText());
        onYAxisLabelChanged();
    }
}

void MainWindow::onShowGridChanged(int state)
{
    bool show = (state == Qt::Checked);
    customPlot->xAxis->grid()->setVisible(show);
    customPlot->yAxis->grid()->setVisible(show);
    customPlot->replot();
}

void MainWindow::onShowLegendChanged(int state)
{
    bool show = (state == Qt::Checked);
    customPlot->legend->setVisible(show);
    customPlot->replot();
}

void MainWindow::onShowMinorGridChanged(int state)
{
    bool show = (state == Qt::Checked);
    customPlot->xAxis->grid()->setSubGridVisible(show);
    customPlot->yAxis->grid()->setSubGridVisible(show);
    customPlot->replot();
}

void MainWindow::onShowX2AxisChanged(int state)
{
    bool show = (state == Qt::Checked);
    customPlot->xAxis2->setVisible(show);
    customPlot->xAxis2->setTickLabels(show);
    customPlot->replot();
}

void MainWindow::onShowY2AxisChanged(int state)
{
    bool show = (state == Qt::Checked);
    customPlot->yAxis2->setVisible(show);
    customPlot->yAxis2->setTickLabels(show);
    customPlot->replot();
}

void MainWindow::onXAxisScaleTypeChanged(int index)
{
    if (index == 0) {
        // 线性坐标 - 同时设置X轴和X2轴
        customPlot->xAxis->setScaleType(QCPAxis::stLinear);
        customPlot->xAxis->setTicker(QSharedPointer<QCPAxisTicker>(new QCPAxisTicker));
        customPlot->xAxis->setNumberFormat("g");
        
        customPlot->xAxis2->setScaleType(QCPAxis::stLinear);
        customPlot->xAxis2->setTicker(QSharedPointer<QCPAxisTicker>(new QCPAxisTicker));
        customPlot->xAxis2->setNumberFormat("g");
    } else {
        // 对数坐标 - 同时设置X轴和X2轴
        customPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
        QSharedPointer<QCPAxisTickerLog> logTickerX(new QCPAxisTickerLog);
        customPlot->xAxis->setTicker(logTickerX);
        customPlot->xAxis->setNumberFormat("eb");
        customPlot->xAxis->setNumberPrecision(0);
        
        customPlot->xAxis2->setScaleType(QCPAxis::stLogarithmic);
        QSharedPointer<QCPAxisTickerLog> logTickerX2(new QCPAxisTickerLog);
        customPlot->xAxis2->setTicker(logTickerX2);
        customPlot->xAxis2->setNumberFormat("eb");
        customPlot->xAxis2->setNumberPrecision(0);
    }
    customPlot->replot();
}

void MainWindow::onYAxisScaleTypeChanged(int index)
{
    if (index == 0) {
        // 线性坐标 - 同时设置Y轴和Y2轴
        customPlot->yAxis->setScaleType(QCPAxis::stLinear);
        customPlot->yAxis->setTicker(QSharedPointer<QCPAxisTicker>(new QCPAxisTicker));
        customPlot->yAxis->setNumberFormat("g");
        
        customPlot->yAxis2->setScaleType(QCPAxis::stLinear);
        customPlot->yAxis2->setTicker(QSharedPointer<QCPAxisTicker>(new QCPAxisTicker));
        customPlot->yAxis2->setNumberFormat("g");
    } else {
        // 对数坐标 - 同时设置Y轴和Y2轴
        customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
        QSharedPointer<QCPAxisTickerLog> logTickerY(new QCPAxisTickerLog);
        customPlot->yAxis->setTicker(logTickerY);
        customPlot->yAxis->setNumberFormat("eb");
        customPlot->yAxis->setNumberPrecision(0);
        
        customPlot->yAxis2->setScaleType(QCPAxis::stLogarithmic);
        QSharedPointer<QCPAxisTickerLog> logTickerY2(new QCPAxisTickerLog);
        customPlot->yAxis2->setTicker(logTickerY2);
        customPlot->yAxis2->setNumberFormat("eb");
        customPlot->yAxis2->setNumberPrecision(0);
    }
    customPlot->replot();
}

void MainWindow::onXAxisTickLabelsChanged(int state)
{
    bool show = (state == Qt::Checked);
    customPlot->xAxis->setTickLabels(show);
    customPlot->replot();
}

void MainWindow::onYAxisTickLabelsChanged(int state)
{
    bool show = (state == Qt::Checked);
    customPlot->yAxis->setTickLabels(show);
    customPlot->replot();
}

void MainWindow::onX2AxisTickLabelsChanged(int state)
{
    bool show = (state == Qt::Checked);
    customPlot->xAxis2->setTickLabels(show);
    customPlot->replot();
}

void MainWindow::onY2AxisTickLabelsChanged(int state)
{
    bool show = (state == Qt::Checked);
    customPlot->yAxis2->setTickLabels(show);
    customPlot->replot();
}

void MainWindow::onXAxisReversedChanged(int state)
{
    bool reversed = (state == Qt::Checked);
    customPlot->xAxis->setRangeReversed(reversed);
    customPlot->xAxis2->setRangeReversed(reversed);
    customPlot->replot();
}

void MainWindow::onAxisRangeChanged()
{
    // 同时设置X轴和X2轴的范围
    customPlot->xAxis->setRange(spinXMin->value(), spinXMax->value());
    customPlot->xAxis2->setRange(spinXMin->value(), spinXMax->value());
    
    // 同时设置Y轴和Y2轴的范围
    customPlot->yAxis->setRange(spinYMin->value(), spinYMax->value());
    customPlot->yAxis2->setRange(spinYMin->value(), spinYMax->value());
    
    customPlot->replot();
}

void MainWindow::updateColumnComboBoxes(const QString& filePath)
{
    // 保存当前选择
    int currentXIndex = cmbXColumn->currentIndex();
    int currentYIndex = cmbYColumn->currentIndex();
    
    // 阻塞信号，避免在更新下拉框时触发数据加载
    cmbXColumn->blockSignals(true);
    cmbYColumn->blockSignals(true);
    
    // 清空下拉框
    cmbXColumn->clear();
    cmbYColumn->clear();
    
    if (filePath.isEmpty()) {
        cmbXColumn->blockSignals(false);
        cmbYColumn->blockSignals(false);
        return;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        cmbXColumn->blockSignals(false);
        cmbYColumn->blockSignals(false);
        return;
    }
    
    QTextStream in(&file);
    if (in.atEnd()) {
        file.close();
        cmbXColumn->blockSignals(false);
        cmbYColumn->blockSignals(false);
        return;
    }
    
    // 读取第一行作为表头
    QString headerLine = in.readLine();
    QStringList headers = headerLine.split(',');
    
    // 读取多行数据来检测列类型
    QVector<QStringList> dataLines;
    int maxCheckLines = 10; // 检查前10行来判断类型
    while (!in.atEnd() && dataLines.size() < maxCheckLines) {
        QString line = in.readLine();
        if (!line.trimmed().isEmpty()) {
            dataLines.append(line.split(','));
        }
    }
    file.close();
    
    // 检测第一行是否为表头
    bool hasHeader = false;
    if (!headers.isEmpty() && !dataLines.isEmpty()) {
        bool ok;
        headers[0].trimmed().toDouble(&ok);
        hasHeader = !ok; // 如果第一个值不能转为数字，说明有表头
    }
    
    // 检测每列的数据类型
    QVector<bool> isNumericColumn(headers.size(), true);
    for (const QStringList& dataLine : dataLines) {
        for (int i = 0; i < qMin(dataLine.size(), headers.size()); ++i) {
            if (isNumericColumn[i]) {
                bool ok;
                dataLine[i].trimmed().toDouble(&ok);
                if (!ok && !dataLine[i].trimmed().isEmpty()) {
                    isNumericColumn[i] = false;
                }
            }
        }
    }
    
    // 填充下拉框
    for (int i = 0; i < headers.size(); ++i) {
        QString columnName;
        QString typeLabel = isNumericColumn[i] ? "数字" : "文本";
        
        if (hasHeader) {
            // 有表头：显示 "Column A: 列名 (类型)"
            columnName = QString("Column %1: %2 (%3)")
                .arg(QChar('A' + i))
                .arg(headers[i].trimmed())
                .arg(typeLabel);
        } else {
            // 无表头：显示 "Column A (类型)"
            columnName = QString("Column %1 (%2)")
                .arg(QChar('A' + i))
                .arg(typeLabel);
        }
        
        cmbXColumn->addItem(columnName, i);
        cmbYColumn->addItem(columnName, i);
    }
    
    // 恢复之前的选择，如果索引有效
    if (currentXIndex >= 0 && currentXIndex < cmbXColumn->count())
        cmbXColumn->setCurrentIndex(currentXIndex);
    else if (cmbXColumn->count() > 0)
        cmbXColumn->setCurrentIndex(0);
        
    if (currentYIndex >= 0 && currentYIndex < cmbYColumn->count())
        cmbYColumn->setCurrentIndex(currentYIndex);
    else if (cmbYColumn->count() > 1)
        cmbYColumn->setCurrentIndex(1);
    else if (cmbYColumn->count() > 0)
        cmbYColumn->setCurrentIndex(0);
    
    // 恢复信号
    cmbXColumn->blockSignals(false);
    cmbYColumn->blockSignals(false);
}

// ========== 拉点功能实现 ==========

void MainWindow::onDragModeToggled(bool enabled)
{
    dragModeEnabled = enabled;
    
    if (enabled) {
        // 启用拉点模式
        lblDragStatus->setText("状态：<b style='color: #4CAF50;'>已启用</b>");
        lblDragStatus->setStyleSheet("color: #4CAF50;");
        
        // 禁用图表的拖拽和缩放，只允许选择
        customPlot->setInteractions(QCP::iSelectPlottables);
        
        // 启用相关按钮
        updateDragControls();
    } else {
        // 禁用拉点模式
        lblDragStatus->setText("状态：未启用");
        lblDragStatus->setStyleSheet("color: #666;");
        
        // 恢复图表的正常交互
        customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
        
        // 重置拖拽状态
        isDragging = false;
        draggedGraph = nullptr;
        draggedPointIndex = -1;
        
        // 禁用按钮
        updateDragControls();
    }
}

void MainWindow::onSaveModifiedData()
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    CurveData& curve = curves[currentCurveIndex];
    if (!curve.modified) {
        QMessageBox::information(this, "提示", "当前曲线未被修改");
        return;
    }
    
    // 弹出保存对话框
    QString fileName = QFileDialog::getSaveFileName(this, "保存修改后的数据", 
                                                     curve.csvFilePath, 
                                                     "CSV文件 (*.csv);;所有文件 (*)");
    if (fileName.isEmpty())
        return;
    
    // 保存数据到CSV
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法打开文件进行写入");
        return;
    }
    
    QTextStream out(&file);
    
    // 写入表头（如果有）
    if (curve.hasHeader && !curve.headerLine.isEmpty()) {
        out << curve.headerLine.join(",") << "\n";
    }
    
    // 写入数据（使用原始数据，但更新Y列）
    for (int i = 0; i < curve.rawDataLines.size() && i < curve.yData.size(); ++i) {
        QStringList line = curve.rawDataLines[i];
        
        // 更新Y列的值
        if (curve.yColumn < line.size()) {
            line[curve.yColumn] = QString::number(curve.yData[i], 'g', 10);
        }
        
        out << line.join(",") << "\n";
    }
    
    file.close();
    
    curve.modified = false;
    updateDragControls();
    
    QMessageBox::information(this, "成功", QString("数据已保存到：\n%1").arg(fileName));
}

void MainWindow::onUndo()
{
    if (undoStack.isEmpty())
        return;
    
    HistoryState currentState;
    currentState.curveIndex = currentCurveIndex;
    if (currentCurveIndex >= 0 && currentCurveIndex < curves.size()) {
        currentState.yData = curves[currentCurveIndex].yData;
    }
    
    HistoryState prevState = undoStack.pop();
    
    // 保存当前状态到重做栈
    redoStack.push(currentState);
    
    // 恢复到之前的状态
    if (prevState.curveIndex >= 0 && prevState.curveIndex < curves.size()) {
        curves[prevState.curveIndex].yData = prevState.yData;
        curves[prevState.curveIndex].graph->setData(curves[prevState.curveIndex].xData, 
                                                     curves[prevState.curveIndex].yData);
        curves[prevState.curveIndex].modified = true;
        customPlot->replot();
    }
    
    updateDragControls();
}

void MainWindow::onRedo()
{
    if (redoStack.isEmpty())
        return;
    
    HistoryState currentState;
    currentState.curveIndex = currentCurveIndex;
    if (currentCurveIndex >= 0 && currentCurveIndex < curves.size()) {
        currentState.yData = curves[currentCurveIndex].yData;
    }
    
    HistoryState nextState = redoStack.pop();
    
    // 保存当前状态到撤销栈
    undoStack.push(currentState);
    
    // 恢复到之后的状态
    if (nextState.curveIndex >= 0 && nextState.curveIndex < curves.size()) {
        curves[nextState.curveIndex].yData = nextState.yData;
        curves[nextState.curveIndex].graph->setData(curves[nextState.curveIndex].xData, 
                                                     curves[nextState.curveIndex].yData);
        curves[nextState.curveIndex].modified = true;
        customPlot->replot();
    }
    
    updateDragControls();
}

void MainWindow::onResetData()
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    CurveData& curve = curves[currentCurveIndex];
    
    if (!curve.modified) {
        QMessageBox::information(this, "提示", "当前曲线未被修改");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认重置", 
        "确定要重置当前曲线的数据吗？\n这将丢失所有未保存的修改！",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // 重新加载原始数据
        QVector<double> newXData, newYData;
        QVector<QStringList> newRawData;
        bool newHasHeader;
        QStringList newHeader;
        if (loadCSV(curve.csvFilePath, curve.xColumn, curve.yColumn, newXData, newYData,
                    newRawData, newHasHeader, newHeader)) {
            curve.xData = newXData;
            curve.yData = newYData;
            curve.rawDataLines = newRawData;
            curve.hasHeader = newHasHeader;
            curve.headerLine = newHeader;
            curve.graph->setData(curve.xData, curve.yData);
            curve.modified = false;
            
            // 清空撤销/重做栈
            undoStack.clear();
            redoStack.clear();
            
            updateDragControls();
            customPlot->replot();
            
            QMessageBox::information(this, "成功", "数据已重置为原始状态");
        } else {
            QMessageBox::critical(this, "错误", "无法重新加载数据");
        }
    }
}

void MainWindow::onPlotMousePress(QMouseEvent* event)
{
    if (!dragModeEnabled || currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    if (event->button() == Qt::LeftButton) {
        CurveData& curve = curves[currentCurveIndex];
        
        // 使用selectTest检测是否点击到了数据点
        double distance = curve.graph->selectTest(event->pos(), false);
        
        // distance < 0 表示未选中，>= 0 表示选中（值越小越接近）
        if (distance >= 0 && distance < 20) {
            // 手动查找最近的数据点
            double x = customPlot->xAxis->pixelToCoord(event->pos().x());
            double y = customPlot->yAxis->pixelToCoord(event->pos().y());
            
            // 在数据中查找最近的点
            double minDist = 1e10;
            int nearestIdx = -1;
            
            for (int i = 0; i < curve.xData.size(); ++i) {
                double dataX = curve.xData[i];
                double dataY = curve.yData[i];
                
                // 转换为像素坐标计算距离
                double pixelX = customPlot->xAxis->coordToPixel(dataX);
                double pixelY = customPlot->yAxis->coordToPixel(dataY);
                double mousePosX = customPlot->xAxis->coordToPixel(x);
                double mousePosY = customPlot->yAxis->coordToPixel(y);
                
                double dx = pixelX - mousePosX;
                double dy = pixelY - mousePosY;
                double dist = qSqrt(dx * dx + dy * dy);
                
                if (dist < minDist) {
                    minDist = dist;
                    nearestIdx = i;
                }
            }
            
            if (nearestIdx >= 0 && minDist < 30) {  // 30像素容差
                isDragging = true;
                draggedGraph = curve.graph;
                draggedPointIndex = nearestIdx;
                
                // 保存当前状态到撤销栈
                saveHistoryState();
                
                // 高亮显示选中的点
                customPlot->setCursor(Qt::ClosedHandCursor);
            }
        }
    }
}

void MainWindow::onPlotMouseMove(QMouseEvent* event)
{
    if (!dragModeEnabled)
        return;
    
    if (isDragging && draggedGraph && draggedPointIndex >= 0) {
        if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
            return;
        
        CurveData& curve = curves[currentCurveIndex];
        
        // 将鼠标位置转换为图表坐标（只使用Y坐标）
        double newY = customPlot->yAxis->pixelToCoord(event->pos().y());
        
        // 更新数据点的Y值（X值保持不变）
        if (draggedPointIndex < curve.yData.size()) {
            curve.yData[draggedPointIndex] = newY;
            curve.graph->setData(curve.xData, curve.yData);
            curve.modified = true;
            
            customPlot->replot();
            updateDragControls();
        }
    } else if (dragModeEnabled && currentCurveIndex >= 0 && currentCurveIndex < curves.size()) {
        // 鼠标悬停时检查是否在数据点附近
        CurveData& curve = curves[currentCurveIndex];
        double distance = curve.graph->selectTest(event->pos(), false);
        
        if (distance >= 0 && distance < 20) {
            customPlot->setCursor(Qt::OpenHandCursor);
        } else {
            customPlot->setCursor(Qt::ArrowCursor);
        }
    }
}

void MainWindow::onPlotMouseRelease(QMouseEvent* event)
{
    if (isDragging) {
        isDragging = false;
        draggedGraph = nullptr;
        draggedPointIndex = -1;
        customPlot->setCursor(Qt::ArrowCursor);
        
        // 清空重做栈（因为进行了新操作）
        redoStack.clear();
        updateDragControls();
    }
}

void MainWindow::saveHistoryState()
{
    if (currentCurveIndex < 0 || currentCurveIndex >= curves.size())
        return;
    
    HistoryState state;
    state.curveIndex = currentCurveIndex;
    state.yData = curves[currentCurveIndex].yData;
    
    undoStack.push(state);
    
    // 限制撤销栈大小（最多50步）
    if (undoStack.size() > 50) {
        // 移除最早的记录
        QStack<HistoryState> temp;
        while (undoStack.size() > 1) {
            temp.push(undoStack.pop());
        }
        undoStack.clear();
        while (!temp.isEmpty()) {
            undoStack.push(temp.pop());
        }
    }
}

void MainWindow::updateDragControls()
{
    bool hasModified = false;
    if (currentCurveIndex >= 0 && currentCurveIndex < curves.size()) {
        hasModified = curves[currentCurveIndex].modified;
    }
    
    bool hasUndo = !undoStack.isEmpty();
    bool hasRedo = !redoStack.isEmpty();
    
    btnUndo->setEnabled(dragModeEnabled && hasUndo);
    btnRedo->setEnabled(dragModeEnabled && hasRedo);
    btnSaveData->setEnabled(dragModeEnabled && hasModified);
    btnResetData->setEnabled(dragModeEnabled && hasModified);
    
    // 更新状态标签
    if (hasModified) {
        lblDragStatus->setText(QString("状态：<b style='color: #4CAF50;'>已启用</b> | <span style='color: #ff9800;'>已修改 (%1步可撤销)</span>")
                              .arg(undoStack.size()));
    } else if (dragModeEnabled) {
        lblDragStatus->setText("状态：<b style='color: #4CAF50;'>已启用</b>");
    }
}

int MainWindow::findNearestPoint(QCPGraph* graph, const QPointF& pos, double& distance)
{
    if (!graph || graph->data()->isEmpty()) {
        distance = 1e10;
        return -1;
    }
    
    int nearestIndex = -1;
    double minDistPixels = 1e10;
    
    QCPGraphDataContainer::const_iterator begin = graph->data()->constBegin();
    QCPGraphDataContainer::const_iterator end = graph->data()->constEnd();
    
    int index = 0;
    for (QCPGraphDataContainer::const_iterator it = begin; it != end; ++it, ++index) {
        // 获取数据点坐标
        double dataX = it->key;
        double dataY = it->value;
        
        // 转换为像素坐标计算距离
        double pixelX = customPlot->xAxis->coordToPixel(dataX);
        double pixelY = customPlot->yAxis->coordToPixel(dataY);
        double mousePosX = customPlot->xAxis->coordToPixel(pos.x());
        double mousePosY = customPlot->yAxis->coordToPixel(pos.y());
        
        double dx = pixelX - mousePosX;
        double dy = pixelY - mousePosY;
        double dist = qSqrt(dx * dx + dy * dy);
        
        if (dist < minDistPixels) {
            minDistPixels = dist;
            nearestIndex = index;
        }
    }
    
    distance = minDistPixels;
    return nearestIndex;
}

// ========== 导出图片功能 ==========

void MainWindow::onExportImage()
{
    // 使用图表标题作为默认文件名
    QString defaultFileName = edtPlotTitle->text();
    
    // 如果标题为空，使用默认名称
    if (defaultFileName.isEmpty()) {
        defaultFileName = "plot";
    }
    
    // 添加默认扩展名
    defaultFileName += ".jpg";
    
    // 弹出保存对话框
    QString fileName = QFileDialog::getSaveFileName(this, "导出图片", defaultFileName, 
        "JPEG图片 (*.jpg *.jpeg);;PNG图片 (*.png);;BMP图片 (*.bmp);;所有文件 (*)");
    
    if (fileName.isEmpty())
        return;
    
    // 设置导出参数（根据截图的API）
    int width = 600;      // 宽度
    int height = 600;     // 高度
    double scale = 2.0;   // 缩放因子（用于高清输出）
    int quality = 95;     // JPEG质量
    
    // 判断文件格式
    QString suffix = QFileInfo(fileName).suffix().toLower();
    
    bool success = false;
    if (suffix == "jpg" || suffix == "jpeg") {
        // 导出为JPEG
        success = customPlot->saveJpg(fileName, width, height, scale, quality);
    } else if (suffix == "png") {
        // 导出为PNG
        success = customPlot->savePng(fileName, width, height, scale, quality);
    } else if (suffix == "bmp") {
        // 导出为BMP
        success = customPlot->saveBmp(fileName, width, height, scale);
    } else {
        // 默认使用JPG
        success = customPlot->saveJpg(fileName, width, height, scale, quality);
    }
    
    if (success) {
        QMessageBox::information(this, "成功", 
            QString("图片已导出到：\n%1\n\n分辨率：%2x%3\n缩放倍数：%4\n质量：%5")
            .arg(fileName)
            .arg(width)
            .arg(height)
            .arg(scale)
            .arg(quality));
    } else {
        QMessageBox::critical(this, "错误", "图片导出失败！");
    }
}
