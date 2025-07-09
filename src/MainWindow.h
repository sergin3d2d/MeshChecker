#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>
#include "Mesh.h"
#include "MeshChecker.h"
#include "IntersectionResult.h"

struct BatchCheckResult {
    QString filePath;
    MeshChecker::CheckResult checkResult;
};



struct BatchIntersectionResult {
    QString filePath;
    int intersectingFaces;
    bool onMannequin;
};

class QTabWidget;
class ViewerWidget;
class QLabel;
class QTableWidget;
class QListWidget;
class QProgressDialog;
class QCheckBox;
class QGroupBox;
class QTextEdit;
class QSpinBox;
class QSlider;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoadMesh();
    void onCheckMesh();
    void onCheckFinished();
    void onSelectFolder();
    void onExportCsv();
    void onLoadMannequin();
    void onLoadApparel();
    void onCheckIntersection();
    void onCheckIntersectionFinished();
    void onIntersectionVisualizationToggled();
    void updateCameraStatus(const QString& status);
    void onVisualizationToggled();
    void onCheckDegenerate();
    void onLogMessage(const QString& message);
    void onBatchResultReady(int index);
    void onBatchCheckFinished();
    void onLoadMannequinForBatchIntersection();
    void onSelectApparelFolder();
    void onBatchIntersectionResultReady(int index);
    void onBatchIntersectionFinished();
    void onExportBatchIntersectionCsv();
    void onHighlightRadiusChanged(int value);

private:
    void setupUI();
    void updateIntersectionView();

    QTabWidget *tabWidget;
    ViewerWidget *viewerWidget;
    QTextEdit* console;
    Mesh currentMesh;
    Mesh mannequinMesh;
    std::vector<Mesh> apparelMeshes;
    QString currentMeshPath;
    MeshChecker::CheckResult lastCheckResult;

    // Single Check
    QLabel* watertightResultLabel;
    QLabel* nonManifoldResultLabel;
    QLabel* selfIntersectionResultLabel;
    QLabel* holesResultLabel;
    QLabel* degenerateFacesResultLabel;
    QLabel* hasUvsResultLabel;
    QLabel* overlappingUvsResultLabel;
    QLabel* uvsOutOfBoundsResultLabel;

    // Visualization Toggles
    QCheckBox* showIntersectionsCheck;
    QCheckBox* showNonManifoldCheck;
    QCheckBox* showHolesCheck;
    QCheckBox* showOverlappingUvsCheck;
    QSlider* highlightRadiusSlider;
    QLabel* highlightRadiusLabel;

    // Check selection
    QCheckBox* checkWatertightCheck;
    QCheckBox* checkNonManifoldCheck;
    QCheckBox* checkSelfIntersectCheck;
    QCheckBox* checkHolesCheck;
    QCheckBox* checkDegenerateFacesCheck;
    QCheckBox* checkUVOverlapCheck;
    QCheckBox* checkUVBoundsCheck;

    // Batch Check
    QTableWidget* batchResultsTable;
    QCheckBox* batchCheckWatertightCheck;
    QCheckBox* batchCheckNonManifoldCheck;
    QCheckBox* batchCheckSelfIntersectCheck;
    QCheckBox* batchCheckHolesCheck;
    QCheckBox* batchCheckDegenerateFacesCheck;
    QCheckBox* batchCheckUVOverlapCheck;
    QCheckBox* batchCheckUVBoundsCheck;
    QCheckBox* batchAutoThreadsCheck;
    QSpinBox* batchThreadsSpinBox;

    // Intersection Check
    QListWidget* intersectionResultsList;
    QLabel* intersectionCountLabel;
    QCheckBox* showMannequinCheck;
    QCheckBox* showApparelCheck;
    QCheckBox* showIntersectionsCheck_IntersectionTab;
    std::vector<IntersectionResult> intersectionResults;

    // Status Bar
    QLabel* fileNameLabel;
    QLabel* cameraStatusLabel;

    // Batch Intersection Check
    QTableWidget* batchIntersectionResultsTable;
    Mesh batchIntersectionMannequin;

    // Async
    QFutureWatcher<MeshChecker::CheckResult> checkWatcher;
    QFutureWatcher<std::vector<IntersectionResult>> intersectionCheckWatcher;
    QFutureWatcher<BatchCheckResult> batchCheckWatcher;
    QFutureWatcher<BatchIntersectionResult> batchIntersectionWatcher;
    QProgressDialog* progressDialog;
};

#endif // MAINWINDOW_H
