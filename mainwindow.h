#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QColorDialog>
#include <QCheckBox>
#include <QScrollArea>
#include <QStack>
#include "qcustomplot.h"

struct CurveData {
    QString name;
    QString csvFilePath;
    QVector<double> xData;
    QVector<double> yData;
    QCPGraph* graph;
    QColor color;
    Qt::PenStyle lineStyle;
    double lineWidth;
    int xColumn;
    int yColumn;
    QCPScatterStyle::ScatterShape scatterShape;
    double scatterSize;
    bool modified;  // 新增：标记是否被修改过
    
    // 保存原始CSV的完整数据
    QVector<QStringList> rawDataLines;  // 每行的原始数据（所有列）
    bool hasHeader;  // 是否有表头
    QStringList headerLine;  // 表头行
};

// 用于撤销/重做的历史记录结构
struct HistoryState {
    int curveIndex;  // 哪条曲线
    QVector<double> yData;  // Y数据快照
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddCurve();
    void onDeleteCurve();
    void onCurveSelected();
    void onCurveColorChanged();
    void onCurveLineStyleChanged(int index);
    void onCurveLineWidthChanged(double value);
    void onCurveScatterShapeChanged(int index);
    void onCurveScatterSizeChanged(double value);
    void onCurveNameChanged();
    void onSelectCsvFile();
    void onXColumnChanged(int value);
    void onYColumnChanged(int value);
    
    // 图表属性槽函数
    void onPlotTitleChanged();
    void onXAxisLabelChanged();
    void onYAxisLabelChanged();
    void onEditPlotTitle();  // 修改：编辑图表标题（富文本）
    void onEditXAxisLabel();  // 修改：编辑X轴标签（富文本）
    void onEditYAxisLabel();  // 修改：编辑Y轴标签（富文本）
    void onShowGridChanged(int state);
    void onShowLegendChanged(int state);
    void onShowMinorGridChanged(int state);
    void onShowX2AxisChanged(int state);
    void onShowY2AxisChanged(int state);
    void onXAxisScaleTypeChanged(int index);
    void onYAxisScaleTypeChanged(int index);
    void onXAxisTickLabelsChanged(int state);
    void onYAxisTickLabelsChanged(int state);
    void onX2AxisTickLabelsChanged(int state);
    void onY2AxisTickLabelsChanged(int state);
    void onAxisRangeChanged();
    void onXAxisReversedChanged(int state);  // 新增：X轴反转
    
    // 导出功能槽函数
    void onExportImage();
    
    // 拉点功能槽函数
    void onDragModeToggled(bool enabled);
    void onSaveModifiedData();
    void onUndo();
    void onRedo();
    void onResetData();
    void onPlotMousePress(QMouseEvent* event);
    void onPlotMouseMove(QMouseEvent* event);
    void onPlotMouseRelease(QMouseEvent* event);

private:
    void setupUI();
    void setupLeftPanel();
    void setupCenterPanel();
    void setupRightPanel();
    void updateCurveProperties();
    void updatePlotProperties();
    bool loadCSV(const QString& filePath, int xCol, int yCol, QVector<double>& xData, QVector<double>& yData,
                 QVector<QStringList>& rawData, bool& hasHeader, QStringList& header);
    void updateColumnComboBoxes(const QString& filePath);
    void autoRescaleIfNeeded();  // 新增：如果需要则自动调整范围
    bool hasAnyValidData();  // 新增：检查是否有任何有效数据
    
    // 拉点功能辅助函数
    void saveHistoryState();  // 保存当前状态到历史记录
    void updateDragControls();  // 更新拉点控件状态
    int findNearestPoint(QCPGraph* graph, const QPointF& pos, double& distance);  // 查找最近的点
    
    // UI组件
    QCustomPlot* customPlot;
    QListWidget* curveList;
    QPushButton* btnAddCurve;
    QPushButton* btnDeleteCurve;
    
    // 右侧属性面板
    QWidget* rightPanel;
    QScrollArea* scrollArea;
    
    // 曲线属性控件
    QLineEdit* edtCurveName;
    QLineEdit* edtCsvPath;
    QPushButton* btnSelectCsv;
    QComboBox* cmbXColumn;
    QComboBox* cmbYColumn;
    QPushButton* btnCurveColor;
    QComboBox* cmbLineStyle;
    QDoubleSpinBox* spinLineWidth;
    QComboBox* cmbScatterShape;
    QDoubleSpinBox* spinScatterSize;
    
    // 拉点功能控件
    QCheckBox* chkDragMode;
    QPushButton* btnSaveData;
    QPushButton* btnUndo;
    QPushButton* btnRedo;
    QPushButton* btnResetData;
    QLabel* lblDragStatus;
    
    // 图表属性控件
    QLineEdit* edtPlotTitle;
    QLineEdit* edtXAxisLabel;
    QLineEdit* edtYAxisLabel;
    QCheckBox* chkShowGrid;
    QCheckBox* chkShowLegend;
    QCheckBox* chkShowMinorGrid;
    QCheckBox* chkShowX2Axis;
    QCheckBox* chkShowY2Axis;
    QComboBox* cmbXAxisScaleType;
    QComboBox* cmbYAxisScaleType;
    QCheckBox* chkXAxisTickLabels;
    QCheckBox* chkYAxisTickLabels;
    QCheckBox* chkX2AxisTickLabels;
    QCheckBox* chkY2AxisTickLabels;
    QCheckBox* chkXAxisReversed;  // 新增：X轴反转选项
    
    // 坐标轴范围控件
    QDoubleSpinBox* spinXMin;
    QDoubleSpinBox* spinXMax;
    QDoubleSpinBox* spinYMin;
    QDoubleSpinBox* spinYMax;
    QPushButton* btnApplyRange;
    QPushButton* btnAutoRange;
    
    // 数据存储
    QVector<CurveData> curves;
    int currentCurveIndex;
    
    // 字体设置
    QFont plotTitleFont;
    QFont xAxisLabelFont;
    QFont yAxisLabelFont;
    
    // 拉点功能状态
    bool dragModeEnabled;
    bool isDragging;
    QCPGraph* draggedGraph;
    int draggedPointIndex;
    QStack<HistoryState> undoStack;
    QStack<HistoryState> redoStack;
    
    // 自动范围标志
    bool hasAutoRescaled;  // 是否已经自动调整过范围
};

#endif // MAINWINDOW_H
