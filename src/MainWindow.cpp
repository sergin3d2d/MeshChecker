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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    connect(&checkWatcher, &QFutureWatcher<MeshChecker::CheckResult>::finished, this, &MainWindow::onCheckFinished);
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
    showHolesCheck->setChecked(true);
    singleCheckLayout->addWidget(showHolesCheck);

    showOverlappingUvsCheck = new QCheckBox("Show Overlapping UVs");
    showOverlappingUvsCheck->setChecked(true);
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

    intersectionResultsList = new QListWidget;
    intersectionCheckLayout->addWidget(intersectionResultsList);

    tabWidget->addTab(intersectionCheckTab, "Intersection Check");

    connect(loadMannequinButton, &QPushButton::clicked, this, &MainWindow::onLoadMannequin);
    connect(loadApparelButton, &QPushButton::clicked, this, &MainWindow::onLoadApparel);

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
    viewerWidget->clearMesh();
    // 2. Clear the results from the last check
    lastCheckResult.clear();
    // 3. Clear the mesh object itself
    currentMesh = Mesh();

    if (ObjLoader::load(filePath.toStdString(), currentMesh)) {
        currentMeshPath = filePath;
        fileNameLabel->setText(QFileInfo(filePath).fileName());
        
        // 4. Ensure the mesh has default colors before rendering
        currentMesh.colors.assign(currentMesh.vertices.size(), glm::vec3(0.7f, 0.7f, 0.7f));
        
        // 5. Send the clean mesh to the viewer
        viewerWidget->setMesh(&currentMesh, &lastCheckResult);
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
    progressDialog->show();

    QFuture<MeshChecker::CheckResult> future = QtConcurrent::run(&MeshChecker::check, currentMesh, checksToPerform);
    checkWatcher.setFuture(future);
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
    while (it.hasNext()) {
        QString filePath = it.next();
        Logger::getInstance().log("Checking file: " + filePath.toStdString());
        QCoreApplication::processEvents(); 

        Mesh mesh;
        if (ObjLoader::load(filePath.toStdString(), mesh)) {
            std::set<MeshChecker::CheckType> checksToPerform;
            if (batchCheckWatertightCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::Watertight);
            if (batchCheckNonManifoldCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::NonManifold);
            if (batchCheckSelfIntersectCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::SelfIntersect);
            if (batchCheckHolesCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::Holes);
            if (batchCheckDegenerateFacesCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::DegenerateFaces);
            if (batchCheckUVOverlapCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::UVOverlap);
            if (batchCheckUVBoundsCheck->isChecked()) checksToPerform.insert(MeshChecker::CheckType::UVBounds);

            MeshChecker::CheckResult result = MeshChecker::check(mesh, checksToPerform);
            int row = batchResultsTable->rowCount();
            batchResultsTable->insertRow(row);
            batchResultsTable->setItem(row, 0, new QTableWidgetItem(QFileInfo(filePath).fileName()));
            batchResultsTable->setItem(row, 1, new QTableWidgetItem(result.is_watertight ? "Yes" : "No"));
            batchResultsTable->setItem(row, 2, new QTableWidgetItem(QString::number(result.non_manifold_vertices_count)));
            batchResultsTable->setItem(row, 3, new QTableWidgetItem(QString::number(result.self_intersections_count)));
            batchResultsTable->setItem(row, 4, new QTableWidgetItem(QString::number(result.holes_count)));
            batchResultsTable->setItem(row, 5, new QTableWidgetItem(QString::number(result.degenerate_faces_count)));
            batchResultsTable->setItem(row, 6, new QTableWidgetItem(result.has_uvs ? "Yes" : "No"));
            batchResultsTable->setItem(row, 7, new QTableWidgetItem(QString::number(result.overlapping_uv_islands_count)));
            batchResultsTable->setItem(row, 8, new QTableWidgetItem(QString::number(result.uvs_out_of_bounds_count)));
        } else {
            Logger::getInstance().log("Failed to load file: " + filePath.toStdString());
        }
    }
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
        if (!ObjLoader::load(filePath.toStdString(), mannequinMesh)) {
            QMessageBox::critical(this, "Error", "Failed to load mannequin mesh.");
            Logger::getInstance().log("Failed to load mannequin.");
        } else {
            Logger::getInstance().log("Mannequin loaded successfully.");
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
        if (ObjLoader::load(filePath.toStdString(), mesh)) {
            apparelMeshes.push_back(mesh);
            Logger::getInstance().log("Loaded apparel item: " + filePath.toStdString());
        } else {
            Logger::getInstance().log("Failed to load apparel item: " + filePath.toStdString());
        }
    }

    if (mannequinMesh.vertices.empty()) {
        QMessageBox::warning(this, "Warning", "No mannequin loaded.");
        Logger::getInstance().log("Intersection check skipped: No mannequin loaded.");
        return;
    }

    Logger::getInstance().log("Performing intersection check...");
    intersectionResultsList->clear();
    for (size_t i = 0; i < apparelMeshes.size(); ++i) {
        bool intersects = MeshChecker::intersects(mannequinMesh, apparelMeshes[i]);
        QString resultText = QString("Apparel %1 intersects: %2").arg(i + 1).arg(intersects ? "Yes" : "No");
        intersectionResultsList->addItem(resultText);
        Logger::getInstance().log(resultText.toStdString());
    }
    Logger::getInstance().log("Intersection check finished.");
}

void MainWindow::updateCameraStatus(const QString& status)
{
    cameraStatusLabel->setText(status);
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

    viewerWidget->setMesh(&currentMesh, &lastCheckResult);
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