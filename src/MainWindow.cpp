#include "MainWindow.h"
#include "ViewerWidget.h"
#include "ObjLoader.h"
#include "MeshChecker.h"
#include "Logger.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QDirIterator>
#include <QTextStream>
#include <QListWidget>
#include <QGroupBox>
#include <QStatusBar>
#include <QtConcurrent/QtConcurrent>
#include <QProgressDialog>
#include <QCheckBox>
#include <QSplitter>
#include <QTextEdit>
#include <QSpinBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    connect(&checkWatcher, &QFutureWatcher<MeshChecker::CheckResult>::finished, this, &MainWindow::onCheckFinished);
    connect(&intersectionCheckWatcher, &QFutureWatcher<std::vector<IntersectionResult>>::finished, this, &MainWindow::onCheckIntersectionFinished);
    connect(&batchCheckWatcher, &QFutureWatcher<BatchCheckResult>::resultReadyAt, this, &MainWindow::onBatchResultReady);
    connect(&batchCheckWatcher, &QFutureWatcher<BatchCheckResult>::finished, this, &MainWindow::onBatchCheckFinished);
    connect(&Logger::getInstance(), &Logger::messageLogged, this, &MainWindow::onLogMessage);
    Logger::getInstance().init("mesh_checker.log");
}

MainWindow::~MainWindow()
{
}

void MainWindow::onLogMessage(const QString& message)
{
    console->append(message);
}

void MainWindow::setupUI()
{
    setWindowTitle("Apparel Mesh Checker");
    resize(1280, 720);

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    // Left panel for controls
    QSplitter *leftSplitter = new QSplitter(Qt::Vertical);
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftPanel->setLayout(leftLayout);
    leftPanel->setMinimumWidth(600);
    leftPanel->setMaximumWidth(800);

    tabWidget = new QTabWidget;
    leftLayout->addWidget(tabWidget);

    // Console
    console = new QTextEdit;
    console->setReadOnly(true);
    
    leftSplitter->addWidget(leftPanel);
    leftSplitter->addWidget(console);
    leftSplitter->setSizes({400, 100});


    // Single Check Tab
    QWidget *singleCheckTab = new QWidget;
    QVBoxLayout *singleCheckLayout = new QVBoxLayout(singleCheckTab);
    QPushButton *loadMeshButton = new QPushButton("Load Mesh");
    singleCheckLayout->addWidget(loadMeshButton);
    
    // --- Check Selection ---
    QGroupBox *checksGroup = new QGroupBox("Checks to Perform");
    QVBoxLayout *checksLayout = new QVBoxLayout;
    checkWatertightCheck = new QCheckBox("Watertight");
    checkWatertightCheck->setChecked(true);
    checksLayout->addWidget(checkWatertightCheck);
    checkNonManifoldCheck = new QCheckBox("Non-manifold");
    checkNonManifoldCheck->setChecked(true);
    checksLayout->addWidget(checkNonManifoldCheck);
    checkSelfIntersectCheck = new QCheckBox("Self-intersections");
    checkSelfIntersectCheck->setChecked(true);
    checksLayout->addWidget(checkSelfIntersectCheck);
    checkHolesCheck = new QCheckBox("Holes");
    checkHolesCheck->setChecked(true);
    checksLayout->addWidget(checkHolesCheck);
    checkDegenerateFacesCheck = new QCheckBox("Degenerate faces");
    checkDegenerateFacesCheck->setChecked(true);
    checksLayout->addWidget(checkDegenerateFacesCheck);
    checkUVOverlapCheck = new QCheckBox("Overlapping UVs");
    checkUVOverlapCheck->setChecked(true);
    checksLayout->addWidget(checkUVOverlapCheck);
    checkUVBoundsCheck = new QCheckBox("UVs out of bounds");
    checkUVBoundsCheck->setChecked(true);
    checksLayout->addWidget(checkUVBoundsCheck);
    checksGroup->setLayout(checksLayout);
    singleCheckLayout->addWidget(checksGroup);

    QPushButton *checkButton = new QPushButton("Check Mesh");
    singleCheckLayout->addWidget(checkButton);

    watertightResultLabel = new QLabel("Watertight: -");
    singleCheckLayout->addWidget(watertightResultLabel);

    nonManifoldResultLabel = new QLabel("Non-manifold vertices: -");
    singleCheckLayout->addWidget(nonManifoldResultLabel);

    selfIntersectionResultLabel = new QLabel("Self-intersections: -");
    singleCheckLayout->addWidget(selfIntersectionResultLabel);

    holesResultLabel = new QLabel("Holes: -");
    singleCheckLayout->addWidget(holesResultLabel);

    degenerateFacesResultLabel = new QLabel("Degenerate faces: -");
    singleCheckLayout->addWidget(degenerateFacesResultLabel);

    hasUvsResultLabel = new QLabel("Has UVs: -");
    singleCheckLayout->addWidget(hasUvsResultLabel);

    overlappingUvsResultLabel = new QLabel("Overlapping UVs: -");
    singleCheckLayout->addWidget(overlappingUvsResultLabel);

    uvsOutOfBoundsResultLabel = new QLabel("UVs out of bounds: -");
    singleCheckLayout->addWidget(uvsOutOfBoundsResultLabel);

    // Visualization Toggles
    showIntersectionsCheck = new QCheckBox("Show Self-Intersections");
    showIntersectionsCheck->setChecked(true);
    singleCheckLayout->addWidget(showIntersectionsCheck);

    showNonManifoldCheck = new QCheckBox("Show Non-Manifold Vertices");
    showNonManifoldCheck->setChecked(true);
    singleCheckLayout->addWidget(showNonManifoldCheck);

    showHolesCheck = new QCheckBox("Show Holes");
    showHolesCheck->setChecked(false);
    singleCheckLayout->addWidget(showHolesCheck);

    showOverlappingUvsCheck = new QCheckBox("Show Overlapping UVs");
    showOverlappingUvsCheck->setChecked(false);
    singleCheckLayout->addWidget(showOverlappingUvsCheck);

    connect(showIntersectionsCheck, &QCheckBox::toggled, this, &MainWindow::onVisualizationToggled);
    connect(showNonManifoldCheck, &QCheckBox::toggled, this, &MainWindow::onVisualizationToggled);
    connect(showHolesCheck, &QCheckBox::toggled, this, &MainWindow::onVisualizationToggled);
    connect(showHolesCheck, &QCheckBox::toggled, this, &MainWindow::onVisualizationToggled);
    connect(showOverlappingUvsCheck, &QCheckBox::toggled, this, &MainWindow::onVisualizationToggled);

    singleCheckLayout->addStretch();
    tabWidget->addTab(singleCheckTab, "Single Check");

    connect(loadMeshButton, &QPushButton::clicked, this, &MainWindow::onLoadMesh);
    connect(checkButton, &QPushButton::clicked, this, &MainWindow::onCheckMesh);

    // Batch Check Tab
    QWidget *batchCheckTab = new QWidget;
    QVBoxLayout *batchCheckLayout = new QVBoxLayout(batchCheckTab);
    QPushButton *selectFolderButton = new QPushButton("Select Folder");
    batchCheckLayout->addWidget(selectFolderButton);

    batchResultsTable = new QTableWidget;
    batchResultsTable->setColumnCount(9);
    batchResultsTable->setHorizontalHeaderLabels({ "File", "Watertight", "Non-Manifold", "Self-Intersections", "Holes", "Degenerate", "Has UVs", "Overlapping UVs", "UVs Out of Bounds" });
    batchResultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    batchCheckLayout->addWidget(batchResultsTable);

    // Add checkboxes for batch checks
    QGroupBox *batchChecksGroup = new QGroupBox("Checks to Perform");
    QVBoxLayout *batchChecksLayout = new QVBoxLayout;
    batchCheckWatertightCheck = new QCheckBox("Watertight");
    batchCheckWatertightCheck->setChecked(true);
    batchChecksLayout->addWidget(batchCheckWatertightCheck);
    batchCheckNonManifoldCheck = new QCheckBox("Non-manifold");
    batchCheckNonManifoldCheck->setChecked(true);
    batchChecksLayout->addWidget(batchCheckNonManifoldCheck);
    batchCheckSelfIntersectCheck = new QCheckBox("Self-intersections");
    batchCheckSelfIntersectCheck->setChecked(true);
    batchChecksLayout->addWidget(batchCheckSelfIntersectCheck);
    batchCheckHolesCheck = new QCheckBox("Holes");
    batchCheckHolesCheck->setChecked(true);
    batchChecksLayout->addWidget(batchCheckHolesCheck);
    batchCheckDegenerateFacesCheck = new QCheckBox("Degenerate faces");
    batchCheckDegenerateFacesCheck->setChecked(true);
    batchChecksLayout->addWidget(batchCheckDegenerateFacesCheck);
    batchCheckUVOverlapCheck = new QCheckBox("Overlapping UVs");
    batchCheckUVOverlapCheck->setChecked(true);
    batchChecksLayout->addWidget(batchCheckUVOverlapCheck);
    batchCheckUVBoundsCheck = new QCheckBox("UVs out of bounds");
    batchCheckUVBoundsCheck->setChecked(true);
    batchChecksLayout->addWidget(batchCheckUVBoundsCheck);
    batchChecksGroup->setLayout(batchChecksLayout);
    batchCheckLayout->addWidget(batchChecksGroup);

    // Thread count
    QGroupBox *threadsGroup = new QGroupBox("Parallelism");
    QHBoxLayout *threadsLayout = new QHBoxLayout;
    batchAutoThreadsCheck = new QCheckBox("Auto");
    batchAutoThreadsCheck->setChecked(true);
    threadsLayout->addWidget(batchAutoThreadsCheck);
    batchThreadsSpinBox = new QSpinBox;
    batchThreadsSpinBox->setRange(1, 128);
    batchThreadsSpinBox->setValue(QThread::idealThreadCount());
    batchThreadsSpinBox->setEnabled(false);
    threadsLayout->addWidget(batchThreadsSpinBox);
    threadsGroup->setLayout(threadsLayout);
    batchCheckLayout->addWidget(threadsGroup);

    connect(batchAutoThreadsCheck, &QCheckBox::toggled, batchThreadsSpinBox, &QSpinBox::setDisabled);

    QPushButton *exportCsvButton = new QPushButton("Export to CSV");
    batchCheckLayout->addWidget(exportCsvButton);

    tabWidget->addTab(batchCheckTab, "Batch Check");

    connect(selectFolderButton, &QPushButton::clicked, this, &MainWindow::onSelectFolder);
    connect(exportCsvButton, &QPushButton::clicked, this, &MainWindow::onExportCsv);

    // Intersection Check Tab
    QWidget *intersectionCheckTab = new QWidget;
    QVBoxLayout *intersectionCheckLayout = new QVBoxLayout(intersectionCheckTab);
    QPushButton *loadMannequinButton = new QPushButton("Load Mannequin");
    intersectionCheckLayout->addWidget(loadMannequinButton);

    QPushButton *loadApparelButton = new QPushButton("Load Apparel");
    intersectionCheckLayout->addWidget(loadApparelButton);

    QPushButton *checkIntersectionButton = new QPushButton("Check Intersections");
    intersectionCheckLayout->addWidget(checkIntersectionButton);

    intersectionCountLabel = new QLabel("Intersecting Triangles: -");
    intersectionCheckLayout->addWidget(intersectionCountLabel);

    QGroupBox *visibilityGroup = new QGroupBox("Visibility");
    QVBoxLayout *visibilityLayout = new QVBoxLayout;
    showMannequinCheck = new QCheckBox("Show Mannequin");
    showMannequinCheck->setChecked(true);
    visibilityLayout->addWidget(showMannequinCheck);
    showApparelCheck = new QCheckBox("Show Apparel");
    showApparelCheck->setChecked(true);
    visibilityLayout->addWidget(showApparelCheck);
    visibilityGroup->setLayout(visibilityLayout);
    intersectionCheckLayout->addWidget(visibilityGroup);

    showIntersectionsCheck_IntersectionTab = new QCheckBox("Show Intersections");
    showIntersectionsCheck_IntersectionTab->setChecked(true);
    intersectionCheckLayout->addWidget(showIntersectionsCheck_IntersectionTab);

    intersectionResultsList = new QListWidget;
    intersectionCheckLayout->addWidget(intersectionResultsList);

    tabWidget->addTab(intersectionCheckTab, "Intersection Check");

    connect(loadMannequinButton, &QPushButton::clicked, this, &MainWindow::onLoadMannequin);
    connect(loadApparelButton, &QPushButton::clicked, this, &MainWindow::onLoadApparel);
    connect(checkIntersectionButton, &QPushButton::clicked, this, &MainWindow::onCheckIntersection);
    connect(showMannequinCheck, &QCheckBox::toggled, this, &MainWindow::onIntersectionVisualizationToggled);
    connect(showApparelCheck, &QCheckBox::toggled, this, &MainWindow::onIntersectionVisualizationToggled);
    connect(showIntersectionsCheck_IntersectionTab, &QCheckBox::toggled, this, &MainWindow::onIntersectionVisualizationToggled);

    // Right panel for 3D viewer
    viewerWidget = new ViewerWidget;

    // Add panels to splitter
    mainSplitter->addWidget(leftSplitter);
    mainSplitter->addWidget(viewerWidget);
    mainSplitter->setStretchFactor(1, 1); // Viewer takes remaining space

    // Status Bar
    fileNameLabel = new QLabel("No file loaded");
    cameraStatusLabel = new QLabel("Zoom: 1.0, Rot: 0, 0");
    statusBar()->addWidget(fileNameLabel, 1);
    statusBar()->addWidget(cameraStatusLabel);

    connect(viewerWidget, &ViewerWidget::cameraChanged, this, &MainWindow::updateCameraStatus);
}

void MainWindow::onLoadMesh()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Load Mesh", "", "OBJ Files (*.obj)");
    if (filePath.isEmpty()) {
        return;
    }

    Logger::getInstance().log("Loading mesh: " + filePath.toStdString());

    // --- Definitive State Cleanup ---
    // 1. Clear all data from the viewer's GPU buffers
    viewerWidget->clearMeshes();
    // 2. Clear the results from the last check
    lastCheckResult.clear();
    // 3. Clear the mesh object itself
    currentMesh = Mesh();

    if (ObjLoader::load_indexed(filePath.toStdString(), currentMesh)) {
        currentMeshPath = filePath;
        fileNameLabel->setText(QFileInfo(filePath).fileName());
        
        // 4. Ensure the mesh has default colors before rendering
        currentMesh.colors.assign(currentMesh.vertices.size(), glm::vec3(0.7f, 0.7f, 0.7f));
        
        // 5. Send the clean mesh to the viewer
        viewerWidget->setMeshes({&currentMesh}, &lastCheckResult, nullptr);
        viewerWidget->focusOnMesh();
        Logger::getInstance().log("Mesh loaded successfully.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to load mesh.");
        Logger::getInstance().log("Failed to load mesh.");
    }
}

void MainWindow::onCheckMesh()
{
    if (currentMesh.vertices.empty()) {
        QMessageBox::warning(this, "Warning", "No mesh loaded.");
        return;
    }

    Logger::getInstance().log("Starting mesh check...");

    std::set<MeshChecker::CheckType> checksToPerform;
    if (checkWatertightCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::Watertight);
    if (checkNonManifoldCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::NonManifold);
    if (checkSelfIntersectCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::SelfIntersect);
    if (checkHolesCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::Holes);
    if (checkDegenerateFacesCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::DegenerateFaces);
    if (checkUVOverlapCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::UVOverlap);
    if (checkUVBoundsCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::UVBounds);

    progressDialog = new QProgressDialog("Checking mesh...", "Cancel", 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    
    QFuture<MeshChecker::CheckResult> future = QtConcurrent::run(&MeshChecker::check, currentMesh, checksToPerform);
    checkWatcher.setFuture(future);
    
    connect(&checkWatcher, &QFutureWatcher<MeshChecker::CheckResult>::finished, progressDialog, &QProgressDialog::reset);
    
    progressDialog->exec();
}

void MainWindow::onCheckFinished()
{
    progressDialog->hide();
    lastCheckResult = checkWatcher.result();

    Logger::getInstance().log("Mesh check finished.");

    watertightResultLabel->setText(QString("Watertight: %1").arg(lastCheckResult.is_watertight ? "Yes" : "No"));
    nonManifoldResultLabel->setText(QString("Non-manifold vertices: %1").arg(lastCheckResult.non_manifold_vertices_count));
    selfIntersectionResultLabel->setText(QString("Self-intersections: %1").arg(lastCheckResult.self_intersections_count));
    holesResultLabel->setText(QString("Holes: %1").arg(lastCheckResult.holes_count));
    degenerateFacesResultLabel->setText(QString("Degenerate faces: %1").arg(lastCheckResult.degenerate_faces_count));
    hasUvsResultLabel->setText(QString("Has UVs: %1").arg(lastCheckResult.has_uvs ? "Yes" : "No"));
    overlappingUvsResultLabel->setText(QString("Overlapping UVs: %1").arg(lastCheckResult.overlapping_uv_islands_count));
    uvsOutOfBoundsResultLabel->setText(QString("UVs out of bounds: %1").arg(lastCheckResult.uvs_out_of_bounds_count));

    currentMesh.colors.assign(currentMesh.vertices.size(), glm::vec3(0.7f, 0.7f, 0.7f)); // Reset to default color
    onVisualizationToggled();
}

void MainWindow::onSelectFolder()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (dirPath.isEmpty()) {
        return;
    }

    Logger::getInstance().log("Starting batch check on folder: " + dirPath.toStdString());

    batchResultsTable->setRowCount(0);

    QDirIterator it(dirPath, {"*.obj"}, QDir::Files, QDirIterator::Subdirectories);
    QStringList files;
    while(it.hasNext()){
        files.append(it.next());
    }

    auto processFile = [this](const QString& filePath) -> BatchCheckResult {
        Logger::getInstance().log("Checking file: " + filePath.toStdString());
        
        Mesh mesh;
        if (ObjLoader::load_indexed(filePath.toStdString(), mesh)) {
            std::set<MeshChecker::CheckType> checksToPerform;
            if (batchCheckWatertightCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::Watertight);
            if (batchCheckNonManifoldCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::NonManifold);
            if (batchCheckSelfIntersectCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::SelfIntersect);
            if (batchCheckHolesCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::Holes);
            if (batchCheckDegenerateFacesCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::DegenerateFaces);
            if (batchCheckUVOverlapCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::UVOverlap);
            if (batchCheckUVBoundsCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::UVBounds);

            MeshChecker::CheckResult result = MeshChecker::check(mesh, checksToPerform);
            return {filePath, result};
        } else {
            Logger::getInstance().log("Failed to load file: " + filePath.toStdString());
            return {filePath, MeshChecker::CheckResult()};
        }
    };

    int numThreads = batchAutoThreadsCheck->isChecked() ? QThread::idealThreadCount() : batchThreadsSpinBox->value();
    QThreadPool::globalInstance()->setMaxThreadCount(numThreads);

    QFuture<BatchCheckResult> future = QtConcurrent::mapped(files, processFile);
    batchCheckWatcher.setFuture(future);

    progressDialog = new QProgressDialog("Checking files in folder...", "Cancel", 0, files.count(), this);
    progressDialog->setAttribute(Qt::WA_DeleteOnClose);
    progressDialog->setWindowModality(Qt::WindowModal);
    
    connect(&batchCheckWatcher, &QFutureWatcher<BatchCheckResult>::progressValueChanged, progressDialog, &QProgressDialog::setValue);
    connect(&batchCheckWatcher, &QFutureWatcher<BatchCheckResult>::finished, progressDialog, &QProgressDialog::reset);
    connect(progressDialog, &QProgressDialog::canceled, &batchCheckWatcher, &QFutureWatcher<BatchCheckResult>::cancel);
    
    progressDialog->exec();
}

void MainWindow::onBatchResultReady(int index)
{
    BatchCheckResult result = batchCheckWatcher.resultAt(index);
    
    int row = batchResultsTable->rowCount();
    batchResultsTable->insertRow(row);
    batchResultsTable->setItem(row, 0, new QTableWidgetItem(QFileInfo(result.filePath).fileName()));
    batchResultsTable->setItem(row, 1, new QTableWidgetItem(result.checkResult.is_watertight ? "Yes" : "No"));
    batchResultsTable->setItem(row, 2, new QTableWidgetItem(QString::number(result.checkResult.non_manifold_vertices_count)));
    batchResultsTable->setItem(row, 3, new QTableWidgetItem(QString::number(result.checkResult.self_intersections_count)));
    batchResultsTable->setItem(row, 4, new QTableWidgetItem(QString::number(result.checkResult.holes_count)));
    batchResultsTable->setItem(row, 5, new QTableWidgetItem(QString::number(result.checkResult.degenerate_faces_count)));
    batchResultsTable->setItem(row, 6, new QTableWidgetItem(result.checkResult.has_uvs ? "Yes" : "No"));
    batchResultsTable->setItem(row, 7, new QTableWidgetItem(QString::number(result.checkResult.overlapping_uv_islands_count)));
    batchResultsTable->setItem(row, 8, new QTableWidgetItem(QString::number(result.checkResult.uvs_out_of_bounds_count)));
}

void MainWindow::onBatchCheckFinished()
{
    batchResultsTable->resizeColumnsToContents();
    for (int i = 0; i < batchResultsTable->columnCount(); ++i) {
        batchResultsTable->setColumnWidth(i, batchResultsTable->columnWidth(i) + 20);
    }
    Logger::getInstance().log("Batch check finished.");
}


void MainWindow::onExportCsv()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save CSV", "", "CSV Files (*.csv)");
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        // Header
        for (int i = 0; i < batchResultsTable->columnCount(); ++i) {
            stream << batchResultsTable->horizontalHeaderItem(i)->text() << (i == batchResultsTable->columnCount() - 1 ? "" : ",");
        }
        stream << "\n";

        // Data
        for (int i = 0; i < batchResultsTable->rowCount(); ++i) {
            for (int j = 0; j < batchResultsTable->columnCount(); ++j) {
                stream << batchResultsTable->item(i, j)->text() << (j == batchResultsTable->columnCount() - 1 ? "" : ",");
            }
            stream << "\n";
        }
        file.close();
        Logger::getInstance().log("Batch results exported to: " + filePath.toStdString());
    } else {
        Logger::getInstance().log("Failed to export batch results to: " + filePath.toStdString());
    }
}

void MainWindow::onLoadMannequin()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Load Mannequin", "", "OBJ Files (*.obj)");
    if (!filePath.isEmpty()) {
        Logger::getInstance().log("Loading mannequin: " + filePath.toStdString());
        if (!ObjLoader::load_indexed(filePath.toStdString(), mannequinMesh)) {
            QMessageBox::critical(this, "Error", "Failed to load mannequin mesh.");
            Logger::getInstance().log("Failed to load mannequin.");
        } else {
            Logger::getInstance().log("Mannequin loaded successfully.");
            mannequinMesh.colors.assign(mannequinMesh.vertices.size(), glm::vec3(0.7f, 0.7f, 0.7f));
            updateIntersectionView();
        }
    }
}

void MainWindow::onLoadApparel()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, "Load Apparel", "", "OBJ Files (*.obj)");
    if (filePaths.isEmpty()) {
        return;
    }

    Logger::getInstance().log("Loading apparel...");
    apparelMeshes.clear();
    for (const QString& filePath : filePaths) {
        Mesh mesh;
        if (ObjLoader::load_indexed(filePath.toStdString(), mesh)) {
            mesh.colors.assign(mesh.vertices.size(), glm::vec3(0.7f, 0.7f, 0.7f));
            apparelMeshes.push_back(mesh);
            Logger::getInstance().log("Loaded apparel item: " + filePath.toStdString());
        } else {
            Logger::getInstance().log("Failed to load apparel item: " + filePath.toStdString());
        }
    }
    updateIntersectionView();
}

void MainWindow::updateCameraStatus(const QString& status)
{
    cameraStatusLabel->setText(status);
}

void MainWindow::onCheckIntersection()
{
    if (mannequinMesh.vertices.empty() || apparelMeshes.empty()) {
        QMessageBox::warning(this, "Warning", "Please load both a mannequin and at least one apparel item.");
        return;
    }

    Logger::getInstance().log("Starting intersection check...");

    auto checkFunc = [this]() {
        std::vector<IntersectionResult> results;
        for (size_t i = 0; i < apparelMeshes.size(); ++i) {
            IntersectionResult result;
            result.intersects = MeshChecker::intersects(mannequinMesh, apparelMeshes[i], result.intersecting_faces);
            results.push_back(result);
        }
        return results;
    };

    QFuture<std::vector<IntersectionResult>> future = QtConcurrent::run(checkFunc);
    intersectionCheckWatcher.setFuture(future);

    progressDialog = new QProgressDialog("Checking intersections...", "Cancel", 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    
    connect(&intersectionCheckWatcher, &QFutureWatcher<std::vector<IntersectionResult>>::finished, progressDialog, &QProgressDialog::reset);
    
    progressDialog->exec();
}

void MainWindow::onCheckIntersectionFinished()
{
    progressDialog->hide();
    intersectionResults = intersectionCheckWatcher.result();

    intersectionResultsList->clear();
    int totalIntersectingTriangles = 0;
    for (size_t i = 0; i < intersectionResults.size(); ++i) {
        QString resultText = QString("Apparel %1 intersects: %2").arg(i + 1).arg(intersectionResults[i].intersects ? "Yes" : "No");
        if (intersectionResults[i].intersects) {
            totalIntersectingTriangles += intersectionResults[i].intersecting_faces.size();
            resultText += QString(" (%1 triangles)").arg(intersectionResults[i].intersecting_faces.size());
        }
        intersectionResultsList->addItem(resultText);
        Logger::getInstance().log(resultText.toStdString());
    }
    intersectionCountLabel->setText(QString("Intersecting Triangles: %1").arg(totalIntersectingTriangles));

    updateIntersectionView();
    Logger::getInstance().log("Intersection check finished.");
}

void MainWindow::onIntersectionVisualizationToggled()
{
    updateIntersectionView();
}

void MainWindow::updateIntersectionView()
{
    // Reset colors
    if (!mannequinMesh.vertices.empty()) {
        mannequinMesh.colors.assign(mannequinMesh.vertices.size(), glm::vec3(0.7f, 0.7f, 0.7f));
    }
    for (auto& apparelMesh : apparelMeshes) {
        apparelMesh.colors.assign(apparelMesh.vertices.size(), glm::vec3(0.7f, 0.7f, 0.7f));
    }

    std::vector<const Mesh*> meshes;
    if (showMannequinCheck->isChecked() && !mannequinMesh.vertices.empty()) {
        meshes.push_back(&mannequinMesh);
    }
    if (showApparelCheck->isChecked()) {
        for (const auto& apparelMesh : apparelMeshes) {
            meshes.push_back(&apparelMesh);
        }
    }

    if (showIntersectionsCheck_IntersectionTab->isChecked()) {
        for (size_t i = 0; i < apparelMeshes.size(); ++i) {
            if (i < intersectionResults.size() && intersectionResults[i].intersects) {
                for (const auto& face_idx : intersectionResults[i].intersecting_faces) {
                    for (int j = 0; j < 3; ++j) {
                        unsigned int vertex_index = apparelMeshes[i].vertex_indices[face_idx * 3 + j];
                        if (vertex_index < apparelMeshes[i].colors.size()) {
                           apparelMeshes[i].colors[vertex_index] = glm::vec3(1.0f, 0.0f, 0.0f); // Red
                        }
                    }
                }
            }
        }
    }

    viewerWidget->setMeshes(meshes, nullptr, &intersectionResults);
    viewerWidget->focusOnMesh();
}


void MainWindow::onVisualizationToggled()
{
    if (currentMesh.vertices.empty()) return;

    currentMesh.colors.assign(currentMesh.vertices.size(), glm::vec3(0.7f, 0.7f, 0.7f)); // Default color

    if (showIntersectionsCheck->isChecked()) {
        for (const auto& face_idx : lastCheckResult.intersecting_faces) {
            for (int i = 0; i < 3; ++i) {
                currentMesh.colors[currentMesh.vertex_indices[face_idx * 3 + i]] = glm::vec3(1.0f, 0.0f, 0.0f);
            }
        }
    }

    if (showNonManifoldCheck->isChecked()) {
        for (const auto& face_idx : lastCheckResult.non_manifold_faces) {
            for (int i = 0; i < 3; ++i) {
                currentMesh.colors[currentMesh.vertex_indices[face_idx * 3 + i]] = glm::vec3(1.0f, 1.0f, 0.0f);
            }
        }
    }

    if (showHolesCheck->isChecked()) {
        // Hole visualization is now handled directly in ViewerWidget
    }

    if (showOverlappingUvsCheck->isChecked()) {
        for (const auto& face_idx : lastCheckResult.overlapping_uv_faces) {
            for (int i = 0; i < 3; ++i) {
                currentMesh.colors[currentMesh.vertex_indices[face_idx * 3 + i]] = glm::vec3(1.0f, 0.0f, 1.0f);
            }
        }
    }

    viewerWidget->setMeshes({&currentMesh}, &lastCheckResult, nullptr);
}

void MainWindow::onCheckDegenerate()
{
    if (currentMeshPath.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No mesh loaded.");
        return;
    }

    Logger::getInstance().log("Checking for degenerate faces using external script...");
    QProcess process;
    QStringList args;
    args << "check_degenerate.py" << currentMeshPath;
    process.start("python3", args);
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QMessageBox::information(this, "Degenerate Check", output);
    Logger::getInstance().log("Degenerate check output: " + output.toStdString());
}