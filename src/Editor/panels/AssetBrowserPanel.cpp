#include "AssetBrowserPanel.h"
#include "../IconManager.h"
#include "../PlacementMode.h"
#include "../AssetPreviewModel.h"
#include "../AssetTagManager.h"
#include "../AssetListView.h"
#include "../../core/CrashHandler.h"
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QTextStream>
#include <QHeaderView>
#include <QApplication>
#include <QStyle>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDebug>
#include <QUrl>
#include <QDrag>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <stdexcept>


AssetBrowserPanel::AssetBrowserPanel(QWidget *parent)
    : QWidget(parent)
    , m_assetModel(nullptr)
    , m_filterModel(nullptr)
    , m_filtersVisible(true)
    , m_placementMode(nullptr)
    , m_tagManager(nullptr)
{
    // Initialize components with safety checks
    try {
        m_assetModel = new AssetPreviewModel(this);
        if (!m_assetModel) {
            qCritical() << "AssetBrowserPanel: Failed to create AssetPreviewModel";
            return;
        }

        m_filterModel = new AssetFilterProxyModel(this);
        if (!m_filterModel) {
            qCritical() << "AssetBrowserPanel: Failed to create AssetFilterProxyModel";
            return;
        }

        m_placementMode = new PlacementMode(this);
        if (!m_placementMode) {
            qCritical() << "AssetBrowserPanel: Failed to create PlacementMode";
            return;
        }

        m_tagManager = new AssetTagManager(this);
        if (!m_tagManager) {
            qCritical() << "AssetBrowserPanel: Failed to create AssetTagManager";
            return;
        }

        setupUI();
        setupFilterPanel();
        setupPlacementModePanel();
        setupTaggingPanel();

    } catch (const std::exception& e) {
        qCritical() << "AssetBrowserPanel: Exception during initialization:" << e.what();
    } catch (...) {
        qCritical() << "AssetBrowserPanel: Unknown exception during initialization";
    }
}

AssetBrowserPanel::~AssetBrowserPanel()
{
    try {
        // Disconnect all signals to prevent callbacks during destruction
        if (m_assetList) {
            disconnect(m_assetList, nullptr, this, nullptr);
        }

        if (m_assetModel) {
            disconnect(m_assetModel, nullptr, this, nullptr);
        }

        if (m_filterModel) {
            disconnect(m_filterModel, nullptr, this, nullptr);
        }

        // Clear the model from the view first to prevent access during destruction
        if (m_assetList) {
            m_assetList->setModel(nullptr);
        }

        // The models will be automatically deleted as child objects
        qDebug() << "AssetBrowserPanel: Destructor completed safely";

    } catch (const std::exception& e) {
        qCritical() << "AssetBrowserPanel: Exception in destructor:" << e.what();
    } catch (...) {
        qCritical() << "AssetBrowserPanel: Unknown exception in destructor";
    }
}

void AssetBrowserPanel::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    // Create main horizontal splitter (filter panel on left, content on right)
    m_splitter = new QSplitter(Qt::Horizontal);

    // Create filter panel with scroll area
    m_filterScrollArea = new QScrollArea();
    m_filterScrollArea->setWidgetResizable(true);
    m_filterScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_filterScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_filterScrollArea->setMinimumWidth(200);
    m_filterScrollArea->setMaximumWidth(280);

    m_filterPanel = new QFrame();
    m_filterPanel->setFrameStyle(QFrame::StyledPanel);
    m_filterScrollArea->setWidget(m_filterPanel);

    // Asset list with filter model - add safety checks
    if (!m_assetModel || !m_filterModel) {
        qCritical() << "AssetBrowserPanel::setupUI: Asset model or filter model is null";
        return;
    }

    m_assetList = new AssetListView();
    if (!m_assetList) {
        qCritical() << "AssetBrowserPanel::setupUI: Failed to create AssetListView";
        return;
    }

    m_assetModel->setFilter(QDir::Files);
    m_filterModel->setSourceModel(m_assetModel);
    m_assetList->setModel(m_filterModel);
    m_assetList->setViewMode(QListView::IconMode);
    m_assetList->setResizeMode(QListView::Adjust);
    m_assetList->setIconSize(QSize(64, 64));
    m_assetList->setGridSize(QSize(80, 80));

    // Configure lazy loading for the asset list - DISABLED until project is loaded
    if (m_assetList && m_assetModel) {
        m_assetList->setPreviewModel(m_assetModel);
        m_assetList->setViewportPreviewsEnabled(false); // Disabled by default for safety
        m_assetList->setPreviewRequestDelay(300); // 300ms delay

        // Configure preview model for lazy loading - DISABLED until project is loaded
        m_assetModel->setPreviewsEnabled(false); // Disabled by default for safety
        m_assetModel->setLazyLoadingEnabled(true);
        m_assetModel->setMaxConcurrentPreviews(3);
        m_assetModel->setPreviewGenerationDelay(500);
    } else {
        qWarning() << "AssetBrowserPanel::setupUI: Cannot configure preview settings - asset list or model is null";
    }

    // Preview panel removed for cleaner interface

    // Create status bar
    m_statusBar = new QStatusBar();
    m_statusBar->showMessage("Ready");

    // Right panel layout (asset list only - preview removed)
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_assetList, 1);

    // Add widgets to splitter
    m_splitter->addWidget(m_filterScrollArea);
    m_splitter->addWidget(rightWidget);
    m_splitter->setSizes({300, 700});

    // Add everything to main layout
    m_layout->addWidget(m_splitter);
    m_layout->addWidget(m_statusBar);

    // Connect signals
    connect(m_assetList, &QListView::clicked, this, &AssetBrowserPanel::onAssetClicked);
    connect(m_assetList, &QListView::doubleClicked, this, &AssetBrowserPanel::onAssetDoubleClicked);
    connect(m_assetList, &AssetListView::assetDragStarted, [this](const QStringList& filePaths) {
        qDebug() << "Assets dragged:" << filePaths;
        // The drag and drop will be handled by the scene view or other drop targets
    });
}

void AssetBrowserPanel::setupFilterPanel()
{
    m_filterLayout = new QVBoxLayout(m_filterPanel);
    m_filterLayout->setContentsMargins(8, 8, 8, 8);
    m_filterLayout->setSpacing(8);

    // Title
    QLabel* titleLabel = new QLabel("Asset Browser");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #333;");
    m_filterLayout->addWidget(titleLabel);

    // Navigation section
    QGroupBox* navGroup = new QGroupBox("Navigation");
    QVBoxLayout* navLayout = new QVBoxLayout(navGroup);

    // Navigation buttons
    QHBoxLayout* navButtonLayout = new QHBoxLayout();
    m_upButton = new QPushButton("⬆️ Up");
    m_homeButton = new QPushButton("🏠 Home");
    m_upButton->setMaximumWidth(60);
    m_homeButton->setMaximumWidth(60);
    navButtonLayout->addWidget(m_upButton);
    navButtonLayout->addWidget(m_homeButton);
    navButtonLayout->addStretch();
    navLayout->addLayout(navButtonLayout);

    // Current path display
    m_currentPathLabel = new QLabel("assets/");
    m_currentPathLabel->setStyleSheet("font-family: monospace; background: #f0f0f0; padding: 4px; border: 1px solid #ccc;");
    m_currentPathLabel->setWordWrap(true);
    navLayout->addWidget(m_currentPathLabel);

    m_filterLayout->addWidget(navGroup);

    // Search section
    QGroupBox* searchGroup = new QGroupBox("Search");
    QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search assets...");
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(m_searchEdit);

    QHBoxLayout* searchButtonLayout = new QHBoxLayout();
    m_clearFiltersButton = new QPushButton("Clear All Filters");
    searchButtonLayout->addWidget(m_clearFiltersButton);
    searchButtonLayout->addStretch();
    searchLayout->addLayout(searchButtonLayout);

    m_filterLayout->addWidget(searchGroup);

    // File type filters
    m_fileTypeGroup = new QGroupBox("File Types");
    m_fileTypeLayout = new QVBoxLayout(m_fileTypeGroup);

    m_showImagesCheck = new QCheckBox("🖼️ Images");
    m_show3DModelsCheck = new QCheckBox("🔷 3D Models");
    m_showScriptsCheck = new QCheckBox("📜 Scripts");
    m_showScenesCheck = new QCheckBox("🎭 Scenes");
    m_showAudioCheck = new QCheckBox("🔊 Audio");
    m_showAnimationsCheck = new QCheckBox("🎬 Animations");
    m_showTilemapsCheck = new QCheckBox("🗂️ Tilemaps");
    m_showVideosCheck = new QCheckBox("🎥 Videos");
    m_showTextCheck = new QCheckBox("📝 Text");
    m_showOthersCheck = new QCheckBox("📄 Others");

    // Set all checkboxes checked by default
    m_showImagesCheck->setChecked(true);
    m_show3DModelsCheck->setChecked(true);
    m_showScriptsCheck->setChecked(true);
    m_showScenesCheck->setChecked(true);
    m_showAudioCheck->setChecked(true);
    m_showAnimationsCheck->setChecked(true);
    m_showTilemapsCheck->setChecked(true);
    m_showVideosCheck->setChecked(true);
    m_showTextCheck->setChecked(true);
    m_showOthersCheck->setChecked(true);

    // Add checkboxes to layout
    m_fileTypeLayout->addWidget(m_showImagesCheck);
    m_fileTypeLayout->addWidget(m_show3DModelsCheck);
    m_fileTypeLayout->addWidget(m_showScriptsCheck);
    m_fileTypeLayout->addWidget(m_showScenesCheck);
    m_fileTypeLayout->addWidget(m_showAudioCheck);
    m_fileTypeLayout->addWidget(m_showAnimationsCheck);
    m_fileTypeLayout->addWidget(m_showTilemapsCheck);
    m_fileTypeLayout->addWidget(m_showVideosCheck);
    m_fileTypeLayout->addWidget(m_showTextCheck);
    m_fileTypeLayout->addWidget(m_showOthersCheck);

    m_filterLayout->addWidget(m_fileTypeGroup);

    // Category filters
    m_categoryGroup = new QGroupBox("Categories");
    m_categoryLayout = new QVBoxLayout(m_categoryGroup);

    m_show2DCheck = new QCheckBox("2️⃣ 2D Assets");
    m_show3DCheck = new QCheckBox("3️⃣ 3D Assets");
    m_showUICheck = new QCheckBox("🎛️ UI Assets");

    m_show2DCheck->setChecked(true);
    m_show3DCheck->setChecked(true);
    m_showUICheck->setChecked(true);

    m_categoryLayout->addWidget(m_show2DCheck);
    m_categoryLayout->addWidget(m_show3DCheck);
    m_categoryLayout->addWidget(m_showUICheck);

    m_filterLayout->addWidget(m_categoryGroup);

    // View mode controls
    QGroupBox* viewGroup = new QGroupBox("View Options");
    QVBoxLayout* viewLayout = new QVBoxLayout(viewGroup);

    QHBoxLayout* viewModeLayout = new QHBoxLayout();
    viewModeLayout->addWidget(new QLabel("View Mode:"));
    m_viewModeCombo = new QComboBox();
    m_viewModeCombo->addItem("Icons", QListView::IconMode);
    m_viewModeCombo->addItem("List", QListView::ListMode);
    viewModeLayout->addWidget(m_viewModeCombo);
    viewLayout->addLayout(viewModeLayout);

    m_filterLayout->addWidget(viewGroup);

    // Add stretch to push everything to top
    m_filterLayout->addStretch();

    // Connect filter signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AssetBrowserPanel::onSearchTextChanged);
    connect(m_clearFiltersButton, &QPushButton::clicked, this, &AssetBrowserPanel::onClearFilters);

    // Connect all filter checkboxes
    connect(m_showImagesCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_show3DModelsCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_showScriptsCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_showScenesCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_showAudioCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_showAnimationsCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_showTilemapsCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_showVideosCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_showTextCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_showOthersCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);

    connect(m_show2DCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_show3DCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);
    connect(m_showUICheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onFilterChanged);

    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetBrowserPanel::onViewModeChanged);

    // Connect navigation signals
    connect(m_upButton, &QPushButton::clicked, this, &AssetBrowserPanel::onNavigateUp);
    connect(m_homeButton, &QPushButton::clicked, this, &AssetBrowserPanel::onNavigateHome);
}

void AssetBrowserPanel::setupPlacementModePanel()
{
    // Create placement mode group
    m_placementModeGroup = new QGroupBox("Placement Mode");
    m_placementModeGroup->setCheckable(false);

    QVBoxLayout* placementLayout = new QVBoxLayout(m_placementModeGroup);
    placementLayout->setSpacing(4);
    placementLayout->setContentsMargins(6, 6, 6, 6);

    // Placement mode toggle
    m_placementModeCheck = new QCheckBox("Enable Placement Mode");
    m_placementModeCheck->setToolTip("Enable placement mode for precise asset placement with snapping");
    placementLayout->addWidget(m_placementModeCheck);

    // Placement mode type
    QHBoxLayout* modeTypeLayout = new QHBoxLayout();
    modeTypeLayout->addWidget(new QLabel("Mode:"));
    m_placementModeTypeCombo = new QComboBox();
    m_placementModeTypeCombo->addItems({
        "Free Place",
        "Grid Snap",
        "Surface Snap",
        "Combined"
    });
    m_placementModeTypeCombo->setToolTip("Select placement mode type");
    modeTypeLayout->addWidget(m_placementModeTypeCombo);
    placementLayout->addLayout(modeTypeLayout);

    // Surface detection type
    QHBoxLayout* surfaceDetectionLayout = new QHBoxLayout();
    surfaceDetectionLayout->addWidget(new QLabel("Surface Detection:"));
    m_surfaceDetectionTypeCombo = new QComboBox();
    m_surfaceDetectionTypeCombo->addItems({
        "Physics",
        "Terrain",
        "Mesh",
        "All"
    });
    m_surfaceDetectionTypeCombo->setToolTip("Select surface detection method");
    surfaceDetectionLayout->addWidget(m_surfaceDetectionTypeCombo);
    placementLayout->addLayout(surfaceDetectionLayout);

    // Snapping options
    QGroupBox* snapGroup = new QGroupBox("Snapping Options");
    QVBoxLayout* snapLayout = new QVBoxLayout(snapGroup);
    snapLayout->setSpacing(2);
    snapLayout->setContentsMargins(4, 4, 4, 4);

    m_gridSnapCheck = new QCheckBox("Grid Snap");
    m_gridSnapCheck->setToolTip("Snap to grid in X/Z (or X/Y for 2D)");
    snapLayout->addWidget(m_gridSnapCheck);

    m_gridSnapYCheck = new QCheckBox("Grid Snap Y");
    m_gridSnapYCheck->setToolTip("Snap to Y-axis grid for easy floor placement");
    snapLayout->addWidget(m_gridSnapYCheck);

    m_surfaceSnapCheck = new QCheckBox("Surface Snap");
    m_surfaceSnapCheck->setToolTip("Snap to detected surfaces");
    snapLayout->addWidget(m_surfaceSnapCheck);

    m_alignToSurfaceNormalCheck = new QCheckBox("Align to Surface Normal");
    m_alignToSurfaceNormalCheck->setToolTip("Rotate objects to align with surface normal");
    snapLayout->addWidget(m_alignToSurfaceNormalCheck);

    placementLayout->addWidget(snapGroup);

    // Placement settings
    QGroupBox* settingsGroup = new QGroupBox("Placement Settings");
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsGroup);
    settingsLayout->setSpacing(2);
    settingsLayout->setContentsMargins(4, 4, 4, 4);

    // Grid size
    QHBoxLayout* gridSizeLayout = new QHBoxLayout();
    gridSizeLayout->addWidget(new QLabel("Grid Size:"));
    m_gridSizeSpinBox = new QDoubleSpinBox();
    m_gridSizeSpinBox->setRange(0.1, 10.0);
    m_gridSizeSpinBox->setValue(1.0);
    m_gridSizeSpinBox->setSingleStep(0.1);
    m_gridSizeSpinBox->setDecimals(1);
    m_gridSizeSpinBox->setSuffix(" units");
    gridSizeLayout->addWidget(m_gridSizeSpinBox);
    settingsLayout->addLayout(gridSizeLayout);

    // Placement grid Y
    QHBoxLayout* gridYLayout = new QHBoxLayout();
    gridYLayout->addWidget(new QLabel("Y Level:"));
    m_placementGridYSpinBox = new QDoubleSpinBox();
    m_placementGridYSpinBox->setRange(-100.0, 100.0);
    m_placementGridYSpinBox->setValue(0.0);
    m_placementGridYSpinBox->setSingleStep(0.5);
    m_placementGridYSpinBox->setDecimals(1);
    gridYLayout->addWidget(m_placementGridYSpinBox);
    settingsLayout->addLayout(gridYLayout);

    // Surface snap tolerance
    QHBoxLayout* toleranceLayout = new QHBoxLayout();
    toleranceLayout->addWidget(new QLabel("Surface Range:"));
    m_surfaceSnapToleranceSpinBox = new QDoubleSpinBox();
    m_surfaceSnapToleranceSpinBox->setRange(1.0, 1000.0);
    m_surfaceSnapToleranceSpinBox->setValue(100.0);
    m_surfaceSnapToleranceSpinBox->setSingleStep(10.0);
    m_surfaceSnapToleranceSpinBox->setDecimals(1);
    m_surfaceSnapToleranceSpinBox->setSuffix(" units");
    m_surfaceSnapToleranceSpinBox->setToolTip("Maximum distance to search for surfaces");
    toleranceLayout->addWidget(m_surfaceSnapToleranceSpinBox);
    settingsLayout->addLayout(toleranceLayout);

    placementLayout->addWidget(settingsGroup);

    // Ghost opacity
    QHBoxLayout* opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel("Ghost:"));
    m_ghostOpacitySlider = new QSlider(Qt::Horizontal);
    m_ghostOpacitySlider->setRange(10, 80);
    m_ghostOpacitySlider->setValue(30);
    m_ghostOpacityLabel = new QLabel("30%");
    m_ghostOpacityLabel->setMinimumWidth(30);
    opacityLayout->addWidget(m_ghostOpacitySlider);
    opacityLayout->addWidget(m_ghostOpacityLabel);
    placementLayout->addLayout(opacityLayout);

    // Default placement types
    QGroupBox* defaultsGroup = new QGroupBox("Default Placement Types");
    QVBoxLayout* defaultsLayout = new QVBoxLayout(defaultsGroup);
    defaultsLayout->setSpacing(2);
    defaultsLayout->setContentsMargins(4, 4, 4, 4);

    // 2D Sprite default
    QHBoxLayout* sprite2DLayout = new QHBoxLayout();
    sprite2DLayout->addWidget(new QLabel("2D Sprite:"));
    m_default2DSpriteCombo = new QComboBox();
    m_default2DSpriteCombo->addItems({
        "Sprite2D Component on 2D Node",
        "Sprite2D Component on 2D Node with RigidBody2D",
        "Sprite2D Component on 2D Node with StaticBody2D",
        "Sprite2D Component on 2D Node with CharacterBody2D"
    });
    sprite2DLayout->addWidget(m_default2DSpriteCombo);
    defaultsLayout->addLayout(sprite2DLayout);

    // 3D Sprite default
    QHBoxLayout* sprite3DLayout = new QHBoxLayout();
    sprite3DLayout->addWidget(new QLabel("3D Sprite:"));
    m_default3DSpriteCombo = new QComboBox();
    m_default3DSpriteCombo->addItems({
        "Sprite3D Component on 3D Node",
        "Sprite3D Component on 3D Node with RigidBody3D",
        "Sprite3D Component on 3D Node with StaticBody3D",
        "Sprite3D Component on 3D Node with CharacterBody3D"
    });
    sprite3DLayout->addWidget(m_default3DSpriteCombo);
    defaultsLayout->addLayout(sprite3DLayout);

    // 3D Model default
    QHBoxLayout* model3DLayout = new QHBoxLayout();
    model3DLayout->addWidget(new QLabel("3D Model:"));
    m_default3DModelCombo = new QComboBox();
    m_default3DModelCombo->addItems({
        "Static Mesh Component on 3D Node",
        "Static Mesh Component on 3D Node with RigidBody3D",
        "Static Mesh Component on 3D Node with StaticBody3D",
        "Static Mesh Component on 3D Node with CharacterBody3D",
        "Skinned Mesh Component on 3D Node"
    });
    model3DLayout->addWidget(m_default3DModelCombo);
    defaultsLayout->addLayout(model3DLayout);

    placementLayout->addWidget(defaultsGroup);

    // Add to filter layout
    m_filterLayout->addWidget(m_placementModeGroup);

    // Connect signals
    connect(m_placementModeCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onPlacementModeToggled);
    connect(m_placementModeTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetBrowserPanel::onPlacementModeTypeChanged);
    connect(m_surfaceDetectionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetBrowserPanel::onSurfaceDetectionTypeChanged);
    connect(m_gridSnapCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onGridSnapToggled);
    connect(m_gridSnapYCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onGridSnapYToggled);
    connect(m_surfaceSnapCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onSurfaceSnapToggled);
    connect(m_alignToSurfaceNormalCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onAlignToSurfaceNormalToggled);
    connect(m_gridSizeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AssetBrowserPanel::onGridSizeChanged);
    connect(m_placementGridYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AssetBrowserPanel::onPlacementGridYChanged);
    connect(m_surfaceSnapToleranceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AssetBrowserPanel::onSurfaceSnapToleranceChanged);
    connect(m_ghostOpacitySlider, &QSlider::valueChanged, this, &AssetBrowserPanel::onGhostOpacityChanged);
    connect(m_default2DSpriteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetBrowserPanel::onDefaultPlacementTypeChanged);
    connect(m_default3DSpriteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetBrowserPanel::onDefaultPlacementTypeChanged);
    connect(m_default3DModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AssetBrowserPanel::onDefaultPlacementTypeChanged);
}

void AssetBrowserPanel::setupTaggingPanel()
{
    // Create tagging group
    m_taggingGroup = new QGroupBox("Asset Tagging");
    m_taggingGroup->setCheckable(false);

    QVBoxLayout* taggingLayout = new QVBoxLayout(m_taggingGroup);
    taggingLayout->setSpacing(4);
    taggingLayout->setContentsMargins(6, 6, 6, 6);

    // Preview toggle
    m_previewsEnabledCheck = new QCheckBox("Enable Previews");
    m_previewsEnabledCheck->setChecked(true);
    m_previewsEnabledCheck->setToolTip("Show image and model previews instead of generic icons");
    taggingLayout->addWidget(m_previewsEnabledCheck);

    // Lazy loading controls
    QGroupBox* lazyLoadingGroup = new QGroupBox("Performance");
    QVBoxLayout* lazyLayout = new QVBoxLayout(lazyLoadingGroup);
    lazyLayout->setSpacing(2);
    lazyLayout->setContentsMargins(4, 4, 4, 4);

    m_lazyLoadingCheck = new QCheckBox("Lazy Loading");
    m_lazyLoadingCheck->setChecked(true);
    m_lazyLoadingCheck->setToolTip("Load previews only when visible (recommended for large projects)");
    lazyLayout->addWidget(m_lazyLoadingCheck);

    // Max concurrent previews
    QHBoxLayout* concurrentLayout = new QHBoxLayout();
    concurrentLayout->addWidget(new QLabel("Max Concurrent:"));
    m_maxConcurrentSpinBox = new QSpinBox();
    m_maxConcurrentSpinBox->setRange(1, 10);
    m_maxConcurrentSpinBox->setValue(3);
    m_maxConcurrentSpinBox->setToolTip("Maximum number of previews to generate simultaneously");
    concurrentLayout->addWidget(m_maxConcurrentSpinBox);
    lazyLayout->addLayout(concurrentLayout);

    // Preview delay
    QHBoxLayout* delayLayout = new QHBoxLayout();
    delayLayout->addWidget(new QLabel("Delay (ms):"));
    m_previewDelaySpinBox = new QSpinBox();
    m_previewDelaySpinBox->setRange(0, 2000);
    m_previewDelaySpinBox->setValue(500);
    m_previewDelaySpinBox->setSuffix(" ms");
    m_previewDelaySpinBox->setToolTip("Delay before starting preview generation");
    delayLayout->addWidget(m_previewDelaySpinBox);
    lazyLayout->addLayout(delayLayout);

    taggingLayout->addWidget(lazyLoadingGroup);

    // Tag search
    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel("Search Tags:"));
    m_tagSearchEdit = new QLineEdit();
    m_tagSearchEdit->setPlaceholderText("Filter by tags...");
    searchLayout->addWidget(m_tagSearchEdit);
    taggingLayout->addLayout(searchLayout);

    // Tag filter list
    m_tagFilterList = new QListWidget();
    m_tagFilterList->setMaximumHeight(120);
    m_tagFilterList->setSelectionMode(QAbstractItemView::MultiSelection);
    taggingLayout->addWidget(m_tagFilterList);

    // Tag management buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_createTagButton = new QPushButton("Create Tag");
    m_manageTagsButton = new QPushButton("Manage");
    m_tagAssetButton = new QPushButton("Tag Asset");

    buttonLayout->addWidget(m_createTagButton);
    buttonLayout->addWidget(m_manageTagsButton);
    buttonLayout->addWidget(m_tagAssetButton);
    taggingLayout->addLayout(buttonLayout);

    // Add to filter layout
    m_filterLayout->addWidget(m_taggingGroup);

    // Connect signals
    connect(m_previewsEnabledCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onPreviewsToggled);
    connect(m_lazyLoadingCheck, &QCheckBox::toggled, this, &AssetBrowserPanel::onLazyLoadingToggled);
    connect(m_maxConcurrentSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AssetBrowserPanel::onMaxConcurrentChanged);
    connect(m_previewDelaySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AssetBrowserPanel::onPreviewDelayChanged);
    connect(m_tagSearchEdit, &QLineEdit::textChanged, this, &AssetBrowserPanel::onTagFilterChanged);
    connect(m_tagFilterList, &QListWidget::itemSelectionChanged, this, &AssetBrowserPanel::onTagFilterChanged);
    connect(m_createTagButton, &QPushButton::clicked, this, &AssetBrowserPanel::onCreateTag);
    connect(m_manageTagsButton, &QPushButton::clicked, this, &AssetBrowserPanel::onManageTags);
    connect(m_tagAssetButton, &QPushButton::clicked, this, &AssetBrowserPanel::onTagAsset);

    // Connect tag manager signals
    connect(m_tagManager, &AssetTagManager::tagsChanged, this, [this]() {
        // Refresh tag filter list
        updateTagFilterList();
    });
}

void AssetBrowserPanel::setProjectPath(const QString& projectPath)
{
    qDebug() << "AssetBrowserPanel::setProjectPath - Starting with path:" << projectPath;
    LUPINE_LOG_STARTUP("AssetBrowserPanel: Setting project path: " + projectPath.toStdString());

    // Validate input
    if (projectPath.isEmpty()) {
        qWarning() << "AssetBrowserPanel: Empty project path provided";
        LUPINE_LOG_STARTUP("AssetBrowserPanel: ERROR - Empty project path provided");
        return;
    }

    // Ensure models are properly initialized before setting paths
    if (!m_assetModel || !m_filterModel) {
        qCritical() << "AssetBrowserPanel::setProjectPath - Models not initialized";
        LUPINE_LOG_STARTUP("AssetBrowserPanel: ERROR - Models not initialized");
        return;
    }

    // Validate project directory exists
    QDir projectDir(projectPath);
    if (!projectDir.exists()) {
        qWarning() << "AssetBrowserPanel: Project directory does not exist:" << projectPath;
        return;
    }

    try {
        m_projectPath = projectPath;
        QString assetsPath = projectDir.filePath("assets");

        // Create assets directory if it doesn't exist
        if (!QDir(assetsPath).exists()) {
            qDebug() << "AssetBrowserPanel: Creating assets directory:" << assetsPath;
            if (!projectDir.mkpath("assets")) {
                qWarning() << "AssetBrowserPanel: Failed to create assets directory:" << assetsPath;
                // Continue with project directory instead
                assetsPath = projectPath;
            }
        }

        // Set up the asset model to show all files with safety checks
        if (m_assetModel) {
            qDebug() << "AssetBrowserPanel: Setting up asset model";
            m_assetModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        } else {
            qWarning() << "AssetBrowserPanel: Asset model is null";
            return;
        }

        // Initialize tag manager with project path with safety checks
        if (m_tagManager) {
            qDebug() << "AssetBrowserPanel: Initializing tag manager";
            m_tagManager->setProjectPath(projectPath);
            updateTagFilterList();
        } else {
            qWarning() << "AssetBrowserPanel: Tag manager is null";
        }

        // Navigate to the assets folder with safety checks
        qDebug() << "AssetBrowserPanel: Navigating to assets folder:" << assetsPath;
        navigateToFolder(assetsPath);

        // COMPLETELY DISABLE all preview functionality to prevent crashes
        qDebug() << "AssetBrowserPanel: Preview functionality completely disabled to prevent crashes";
        if (m_assetModel && m_assetList) {
            // Keep previews completely disabled
            m_assetModel->setPreviewsEnabled(false);
            m_assetList->setViewportPreviewsEnabled(false);

            // DO NOT enable previews automatically - they can be manually enabled later if needed
            qDebug() << "AssetBrowserPanel: Previews will remain disabled for stability";
        } else {
            qWarning() << "AssetBrowserPanel: Cannot configure previews - asset model or list is null";
        }

        qDebug() << "AssetBrowserPanel::setProjectPath - Completed successfully";

    } catch (const std::exception& e) {
        qCritical() << "AssetBrowserPanel::setProjectPath - Exception:" << e.what();
    } catch (...) {
        qCritical() << "AssetBrowserPanel::setProjectPath - Unknown exception occurred";
    }
}

void AssetBrowserPanel::enableModelPreviews(bool enabled)
{
    if (m_assetModel) {
        m_assetModel->setModelPreviewsEnabled(enabled);
        qDebug() << "AssetBrowserPanel: Model previews" << (enabled ? "enabled" : "disabled");
    }
}

QString AssetBrowserPanel::getSelectedAssetPath() const
{
    if (!m_assetList || !m_assetList->selectionModel()) {
        return QString();
    }

    QModelIndexList selectedIndexes = m_assetList->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        return QString();
    }

    QModelIndex filterIndex = selectedIndexes.first();
    QModelIndex sourceIndex = m_filterModel->mapToSource(filterIndex);
    return m_assetModel->filePath(sourceIndex);
}



void AssetBrowserPanel::onAssetClicked(const QModelIndex& index)
{
    QModelIndex sourceIndex = m_filterModel->mapToSource(index);
    QString assetPath = m_assetModel->filePath(sourceIndex);
    // Preview functionality is handled by the asset model itself
    emit assetSelected(assetPath);
}

void AssetBrowserPanel::onAssetDoubleClicked(const QModelIndex& index)
{
    QModelIndex sourceIndex = m_filterModel->mapToSource(index);
    QString assetPath = m_assetModel->filePath(sourceIndex);

    QFileInfo fileInfo(assetPath);
    if (fileInfo.isDir()) {
        // Navigate into the folder
        navigateToFolder(assetPath);
    } else {
        // Handle file double-click
        emit assetDoubleClicked(assetPath);
    }
}

void AssetBrowserPanel::onNavigateUp()
{
    if (!m_currentFolderPath.isEmpty()) {
        QDir currentDir(m_currentFolderPath);
        if (currentDir.cdUp()) {
            QString parentPath = currentDir.absolutePath();
            // Make sure we don't go above the assets folder
            QDir assetsDir(QDir(m_projectPath).filePath("assets"));
            if (parentPath.startsWith(assetsDir.absolutePath())) {
                navigateToFolder(parentPath);
            }
        }
    }
}

void AssetBrowserPanel::onNavigateHome()
{
    if (!m_projectPath.isEmpty()) {
        QDir projectDir(m_projectPath);
        QString assetsPath = projectDir.filePath("assets");
        navigateToFolder(assetsPath);
    }
}

void AssetBrowserPanel::onSearchTextChanged(const QString& text)
{
    m_filterModel->setSearchFilter(text);
    updateStatusBar();
}

void AssetBrowserPanel::onFilterChanged()
{
    applyFilters();
    updateStatusBar();
}

void AssetBrowserPanel::onClearFilters()
{
    // Clear search
    m_searchEdit->clear();

    // Check all file type filters
    m_showImagesCheck->setChecked(true);
    m_show3DModelsCheck->setChecked(true);
    m_showScriptsCheck->setChecked(true);
    m_showScenesCheck->setChecked(true);
    m_showAudioCheck->setChecked(true);
    m_showAnimationsCheck->setChecked(true);
    m_showTilemapsCheck->setChecked(true);
    m_showVideosCheck->setChecked(true);
    m_showTextCheck->setChecked(true);
    m_showOthersCheck->setChecked(true);

    // Check all category filters
    m_show2DCheck->setChecked(true);
    m_show3DCheck->setChecked(true);
    m_showUICheck->setChecked(true);

    applyFilters();
    updateStatusBar();
}

void AssetBrowserPanel::onPlacementModeToggled(bool enabled)
{
    m_placementMode->setEnabled(enabled);

    // Enable/disable other placement controls
    m_placementModeTypeCombo->setEnabled(enabled);
    m_surfaceDetectionTypeCombo->setEnabled(enabled);
    m_gridSnapCheck->setEnabled(enabled);
    m_gridSnapYCheck->setEnabled(enabled);
    m_surfaceSnapCheck->setEnabled(enabled);
    m_alignToSurfaceNormalCheck->setEnabled(enabled);
    m_gridSizeSpinBox->setEnabled(enabled);
    m_placementGridYSpinBox->setEnabled(enabled);
    m_surfaceSnapToleranceSpinBox->setEnabled(enabled);
    m_ghostOpacitySlider->setEnabled(enabled);
}

void AssetBrowserPanel::onPlacementModeTypeChanged(int index)
{
    PlacementModeType mode = static_cast<PlacementModeType>(index);
    m_placementMode->setPlacementModeType(mode);
}

void AssetBrowserPanel::onSurfaceDetectionTypeChanged(int index)
{
    SurfaceDetectionType type = static_cast<SurfaceDetectionType>(index);
    m_placementMode->setSurfaceDetectionType(type);
}

void AssetBrowserPanel::onGridSnapToggled(bool enabled)
{
    m_placementMode->setGridSnapEnabled(enabled);
}

void AssetBrowserPanel::onGridSnapYToggled(bool enabled)
{
    m_placementMode->setGridSnapYEnabled(enabled);
}

void AssetBrowserPanel::onSurfaceSnapToggled(bool enabled)
{
    m_placementMode->setSurfaceSnapEnabled(enabled);
}

void AssetBrowserPanel::onAlignToSurfaceNormalToggled(bool enabled)
{
    m_placementMode->setAlignToSurfaceNormal(enabled);
}

void AssetBrowserPanel::onGridSizeChanged(double value)
{
    m_placementMode->setGridSize(static_cast<float>(value));
}

void AssetBrowserPanel::onPlacementGridYChanged(double value)
{
    m_placementMode->setPlacementGridY(static_cast<float>(value));
}

void AssetBrowserPanel::onSurfaceSnapToleranceChanged(double value)
{
    m_placementMode->setSurfaceSnapTolerance(static_cast<float>(value));
}

void AssetBrowserPanel::onGhostOpacityChanged(int value)
{
    float opacity = value / 100.0f;
    m_placementMode->setGhostOpacity(opacity);
    m_ghostOpacityLabel->setText(QString("%1%").arg(value));
}

void AssetBrowserPanel::onDefaultPlacementTypeChanged()
{
    // Update placement mode with new default types
    m_placementMode->setDefault2DSpriteType(m_default2DSpriteCombo->currentText());
    m_placementMode->setDefault3DSpriteType(m_default3DSpriteCombo->currentText());
    m_placementMode->setDefault3DModelType(m_default3DModelCombo->currentText());
}

void AssetBrowserPanel::onPreviewsToggled(bool enabled)
{
    m_assetModel->setPreviewsEnabled(enabled);
    if (!enabled) {
        m_assetModel->clearPreviewCache();
    }

    // Enable/disable lazy loading controls
    m_lazyLoadingCheck->setEnabled(enabled);
    m_maxConcurrentSpinBox->setEnabled(enabled && m_lazyLoadingCheck->isChecked());
    m_previewDelaySpinBox->setEnabled(enabled && m_lazyLoadingCheck->isChecked());
}

void AssetBrowserPanel::onLazyLoadingToggled(bool enabled)
{
    m_assetModel->setLazyLoadingEnabled(enabled);
    m_assetList->setViewportPreviewsEnabled(enabled);

    // Enable/disable performance controls
    m_maxConcurrentSpinBox->setEnabled(enabled && m_previewsEnabledCheck->isChecked());
    m_previewDelaySpinBox->setEnabled(enabled && m_previewsEnabledCheck->isChecked());

    if (!enabled) {
        // Clear cache to force immediate loading
        m_assetModel->clearPreviewCache();
    }
}

void AssetBrowserPanel::onMaxConcurrentChanged(int value)
{
    m_assetModel->setMaxConcurrentPreviews(value);
}

void AssetBrowserPanel::onPreviewDelayChanged(int value)
{
    m_assetModel->setPreviewGenerationDelay(value);
}

void AssetBrowserPanel::onTagFilterChanged()
{
    // TODO: Implement tag filtering
    // This would filter the asset list based on selected tags
    applyFilters();
}

void AssetBrowserPanel::onCreateTag()
{
    // TODO: Implement tag creation dialog
    // This would open a dialog to create a new tag with name, color, and description
}

void AssetBrowserPanel::onManageTags()
{
    // TODO: Implement tag management dialog
    // This would open a dialog to edit/delete existing tags
}

void AssetBrowserPanel::onTagAsset()
{
    // TODO: Implement asset tagging dialog
    // This would open a dialog to add/remove tags from the selected asset
}

void AssetBrowserPanel::updateTagFilterList()
{
    if (!m_tagFilterList) return;

    m_tagFilterList->clear();

    QList<AssetTag> tags = m_tagManager->getAllTags();
    for (const AssetTag& tag : tags) {
        QListWidgetItem* item = new QListWidgetItem(tag.name);
        item->setData(Qt::UserRole, tag.name);

        // Set tag color as background
        if (tag.color.isValid()) {
            QColor bgColor = tag.color;
            bgColor.setAlpha(100); // Semi-transparent
            item->setBackground(QBrush(bgColor));

            // Choose text color based on background brightness
            int brightness = (bgColor.red() * 299 + bgColor.green() * 587 + bgColor.blue() * 114) / 1000;
            item->setForeground(brightness > 128 ? Qt::black : Qt::white);
        }

        item->setToolTip(tag.description);
        m_tagFilterList->addItem(item);
    }
}

void AssetBrowserPanel::onViewModeChanged()
{
    int mode = m_viewModeCombo->currentData().toInt();
    m_assetList->setViewMode(static_cast<QListView::ViewMode>(mode));

    if (mode == QListView::IconMode) {
        m_assetList->setIconSize(QSize(64, 64));
        m_assetList->setGridSize(QSize(80, 80));
    } else {
        m_assetList->setIconSize(QSize(16, 16));
        m_assetList->setGridSize(QSize());
    }
}

void AssetBrowserPanel::applyFilters()
{
    // Collect enabled file types
    QSet<QString> enabledTypes;
    if (m_showImagesCheck->isChecked()) enabledTypes.insert("Images");
    if (m_show3DModelsCheck->isChecked()) enabledTypes.insert("3D Models");
    if (m_showScriptsCheck->isChecked()) enabledTypes.insert("Scripts");
    if (m_showScenesCheck->isChecked()) enabledTypes.insert("Scenes");
    if (m_showAudioCheck->isChecked()) enabledTypes.insert("Audio");
    if (m_showAnimationsCheck->isChecked()) enabledTypes.insert("Animations");
    if (m_showTilemapsCheck->isChecked()) enabledTypes.insert("Tilemaps");
    if (m_showVideosCheck->isChecked()) enabledTypes.insert("Videos");
    if (m_showTextCheck->isChecked()) enabledTypes.insert("Text");
    if (m_showOthersCheck->isChecked()) enabledTypes.insert("Others");

    // Collect enabled categories
    QSet<QString> enabledCategories;
    if (m_show2DCheck->isChecked()) enabledCategories.insert("2D");
    if (m_show3DCheck->isChecked()) enabledCategories.insert("3D");
    if (m_showUICheck->isChecked()) enabledCategories.insert("UI");

    // Apply filters to proxy model
    m_filterModel->setFileTypeFilters(enabledTypes);
    m_filterModel->setCategoryFilters(enabledCategories);
}

void AssetBrowserPanel::updateStatusBar()
{
    int totalItems = m_assetModel->rowCount(m_assetList->rootIndex());
    int visibleItems = m_filterModel->rowCount(m_assetList->rootIndex());

    QString message;
    if (totalItems == visibleItems) {
        message = QString("%1 items").arg(totalItems);
    } else {
        message = QString("%1 of %2 items").arg(visibleItems).arg(totalItems);
    }

    m_statusBar->showMessage(message);
}

void AssetBrowserPanel::navigateToFolder(const QString& folderPath)
{
    qDebug() << "AssetBrowserPanel::navigateToFolder - Navigating to:" << folderPath;

    // Validate input
    if (folderPath.isEmpty()) {
        qWarning() << "AssetBrowserPanel: Empty folder path provided";
        return;
    }

    QFileInfo folderInfo(folderPath);
    if (!folderInfo.exists()) {
        qWarning() << "AssetBrowserPanel: Folder does not exist:" << folderPath;
        return;
    }

    if (!folderInfo.isDir()) {
        qWarning() << "AssetBrowserPanel: Path is not a directory:" << folderPath;
        return;
    }

    if (!folderInfo.isReadable()) {
        qWarning() << "AssetBrowserPanel: Folder is not readable:" << folderPath;
        return;
    }

    try {
        m_currentFolderPath = folderPath;

        // Update the asset model to show the new folder with safety checks
        if (!m_assetModel) {
            qWarning() << "AssetBrowserPanel: Asset model is null during navigation";
            return;
        }

        qDebug() << "AssetBrowserPanel: Setting root path on asset model";
        m_assetModel->setRootPath(folderPath);

        QModelIndex sourceIndex = m_assetModel->index(folderPath);
        if (!sourceIndex.isValid()) {
            qWarning() << "AssetBrowserPanel: Invalid source index for folder:" << folderPath;
            return;
        }

        if (!m_filterModel) {
            qWarning() << "AssetBrowserPanel: Filter model is null during navigation";
            return;
        }

        QModelIndex filterIndex = m_filterModel->mapFromSource(sourceIndex);
        if (!m_assetList) {
            qWarning() << "AssetBrowserPanel: Asset list is null during navigation";
            return;
        }

        qDebug() << "AssetBrowserPanel: Setting root index on asset list";
        m_assetList->setRootIndex(filterIndex);

        // Update navigation UI with safety checks
        qDebug() << "AssetBrowserPanel: Updating navigation UI";
        updateNavigationButtons();
        updateStatusBar();

        qDebug() << "AssetBrowserPanel::navigateToFolder - Navigation completed successfully";

    } catch (const std::exception& e) {
        qCritical() << "AssetBrowserPanel::navigateToFolder - Exception:" << e.what();
    } catch (...) {
        qCritical() << "AssetBrowserPanel::navigateToFolder - Unknown exception occurred";
    }
}

void AssetBrowserPanel::updateNavigationButtons()
{
    if (m_currentFolderPath.isEmpty() || m_projectPath.isEmpty()) {
        m_upButton->setEnabled(false);
        m_homeButton->setEnabled(false);
        m_currentPathLabel->setText("No project loaded");
        return;
    }

    // Update current path display
    QDir projectDir(m_projectPath);
    QString assetsPath = projectDir.filePath("assets");
    QString relativePath = QDir(assetsPath).relativeFilePath(m_currentFolderPath);
    if (relativePath == ".") {
        relativePath = "assets/";
    } else {
        relativePath = "assets/" + relativePath + "/";
    }
    m_currentPathLabel->setText(relativePath);

    // Enable/disable up button
    QDir currentDir(m_currentFolderPath);
    QDir assetsDir(assetsPath);
    bool canGoUp = (m_currentFolderPath != assetsPath) &&
                   currentDir.absolutePath().startsWith(assetsDir.absolutePath());
    m_upButton->setEnabled(canGoUp);

    // Home button is always enabled if we have a project
    m_homeButton->setEnabled(true);
}

// Preview panel functionality removed for cleaner interface

// Preview functionality removed for cleaner interface

// All preview methods removed for cleaner interface

bool AssetBrowserPanel::isImageFile(const QString& filePath) const
{
    QStringList imageExtensions = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".tiff", ".gif", ".webp"};
    for (const QString& ext : imageExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetBrowserPanel::is3DModelFile(const QString& filePath) const
{
    QStringList modelExtensions = {".obj", ".fbx", ".dae", ".gltf", ".glb", ".3ds", ".blend", ".ply"};
    for (const QString& ext : modelExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetBrowserPanel::isTextFile(const QString& filePath) const
{
    QStringList textExtensions = {".txt", ".md", ".json", ".xml", ".csv", ".log", ".ini", ".cfg"};
    for (const QString& ext : textExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetBrowserPanel::isSceneFile(const QString& filePath) const
{
    return filePath.endsWith(".lupscene", Qt::CaseInsensitive);
}

bool AssetBrowserPanel::isScriptFile(const QString& filePath) const
{
    QStringList scriptExtensions = {".py", ".lua", ".js", ".cs", ".cpp", ".h", ".hpp"};
    for (const QString& ext : scriptExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetBrowserPanel::isAudioFile(const QString& filePath) const
{
    QStringList audioExtensions = {".wav", ".ogg", ".mp3", ".flac", ".aac", ".m4a", ".wma"};
    for (const QString& ext : audioExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetBrowserPanel::isAnimationFile(const QString& filePath) const
{
    QStringList animExtensions = {".anim", ".spriteanim", ".skelanim", ".fbx", ".dae"};
    for (const QString& ext : animExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    // Also check for animation naming patterns
    return filePath.contains("_anim", Qt::CaseInsensitive) ||
           filePath.contains("animation", Qt::CaseInsensitive);
}

bool AssetBrowserPanel::isTilemapFile(const QString& filePath) const
{
    QStringList tilemapExtensions = {".tilemap", ".tmx", ".tsx", ".json"};
    for (const QString& ext : tilemapExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return filePath.contains("tilemap", Qt::CaseInsensitive) ||
           filePath.contains("tileset", Qt::CaseInsensitive);
}

bool AssetBrowserPanel::isVideoFile(const QString& filePath) const
{
    QStringList videoExtensions = {".mp4", ".avi", ".mov", ".wmv", ".flv", ".webm", ".mkv"};
    for (const QString& ext : videoExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QString AssetBrowserPanel::getFileCategory(const QString& filePath) const
{
    if (isImageFile(filePath) || isAnimationFile(filePath) || isTilemapFile(filePath)) {
        return "2D";
    }
    if (is3DModelFile(filePath)) {
        return "3D";
    }
    if (isTextFile(filePath) || isScriptFile(filePath)) {
        return "UI"; // Scripts and text files are often UI-related
    }
    return "2D"; // Default to 2D for most assets
}

QString AssetBrowserPanel::getFileType(const QString& filePath) const
{
    if (isImageFile(filePath)) return "Images";
    if (is3DModelFile(filePath)) return "3D Models";
    if (isScriptFile(filePath)) return "Scripts";
    if (isSceneFile(filePath)) return "Scenes";
    if (isAudioFile(filePath)) return "Audio";
    if (isAnimationFile(filePath)) return "Animations";
    if (isTilemapFile(filePath)) return "Tilemaps";
    if (isVideoFile(filePath)) return "Videos";
    if (isTextFile(filePath)) return "Text";
    return "Others";
}

// AssetFilterProxyModel Implementation
AssetFilterProxyModel::AssetFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void AssetFilterProxyModel::setSearchFilter(const QString& searchText)
{
    m_searchText = searchText;
    invalidateFilter();
}

void AssetFilterProxyModel::setFileTypeFilters(const QSet<QString>& enabledTypes)
{
    m_enabledFileTypes = enabledTypes;
    invalidateFilter();
}

void AssetFilterProxyModel::setCategoryFilters(const QSet<QString>& enabledCategories)
{
    m_enabledCategories = enabledCategories;
    invalidateFilter();
}

bool AssetFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QFileSystemModel* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());

    if (!fsModel) {
        return true;
    }

    QString filePath = fsModel->filePath(index);
    QFileInfo fileInfo(filePath);

    // Always show directories
    if (fileInfo.isDir()) {
        return true;
    }

    // Apply search filter
    if (!m_searchText.isEmpty()) {
        if (!fileInfo.fileName().contains(m_searchText, Qt::CaseInsensitive)) {
            return false;
        }
    }

    // Apply file type filter
    if (!m_enabledFileTypes.isEmpty()) {
        QString fileType = getFileType(filePath);
        if (!m_enabledFileTypes.contains(fileType)) {
            return false;
        }
    }

    // Apply category filter
    if (!m_enabledCategories.isEmpty()) {
        QString category = getFileCategory(filePath);
        if (!m_enabledCategories.contains(category)) {
            return false;
        }
    }

    return true;
}

QString AssetFilterProxyModel::getFileType(const QString& filePath) const
{
    if (isImageFile(filePath)) return "Images";
    if (is3DModelFile(filePath)) return "3D Models";
    if (isScriptFile(filePath)) return "Scripts";
    if (isSceneFile(filePath)) return "Scenes";
    if (isAudioFile(filePath)) return "Audio";
    if (isAnimationFile(filePath)) return "Animations";
    if (isTilemapFile(filePath)) return "Tilemaps";
    if (isVideoFile(filePath)) return "Videos";
    if (isTextFile(filePath)) return "Text";
    return "Others";
}

QString AssetFilterProxyModel::getFileCategory(const QString& filePath) const
{
    if (isImageFile(filePath) || isAnimationFile(filePath) || isTilemapFile(filePath)) {
        return "2D";
    }
    if (is3DModelFile(filePath)) {
        return "3D";
    }
    if (isTextFile(filePath) || isScriptFile(filePath)) {
        return "UI";
    }
    return "2D";
}

bool AssetFilterProxyModel::isImageFile(const QString& filePath) const
{
    QStringList imageExtensions = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".tiff", ".gif", ".webp"};
    for (const QString& ext : imageExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetFilterProxyModel::is3DModelFile(const QString& filePath) const
{
    QStringList modelExtensions = {".obj", ".fbx", ".dae", ".gltf", ".glb", ".3ds", ".blend", ".ply"};
    for (const QString& ext : modelExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetFilterProxyModel::isScriptFile(const QString& filePath) const
{
    QStringList scriptExtensions = {".py", ".lua", ".js", ".cs", ".cpp", ".h", ".hpp"};
    for (const QString& ext : scriptExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetFilterProxyModel::isSceneFile(const QString& filePath) const
{
    return filePath.endsWith(".lupscene", Qt::CaseInsensitive);
}

bool AssetFilterProxyModel::isAudioFile(const QString& filePath) const
{
    QStringList audioExtensions = {".wav", ".ogg", ".mp3", ".flac", ".aac", ".m4a", ".wma"};
    for (const QString& ext : audioExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetFilterProxyModel::isAnimationFile(const QString& filePath) const
{
    QStringList animExtensions = {".anim", ".spriteanim", ".skelanim"};
    for (const QString& ext : animExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return filePath.contains("_anim", Qt::CaseInsensitive) ||
           filePath.contains("animation", Qt::CaseInsensitive);
}

bool AssetFilterProxyModel::isTilemapFile(const QString& filePath) const
{
    QStringList tilemapExtensions = {".tilemap", ".tmx", ".tsx"};
    for (const QString& ext : tilemapExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return filePath.contains("tilemap", Qt::CaseInsensitive) ||
           filePath.contains("tileset", Qt::CaseInsensitive);
}

bool AssetFilterProxyModel::isVideoFile(const QString& filePath) const
{
    QStringList videoExtensions = {".mp4", ".avi", ".mov", ".wmv", ".flv", ".webm", ".mkv"};
    for (const QString& ext : videoExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AssetFilterProxyModel::isTextFile(const QString& filePath) const
{
    QStringList textExtensions = {".txt", ".md", ".json", ".xml", ".csv", ".log", ".ini", ".cfg"};
    for (const QString& ext : textExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

#include "AssetBrowserPanel.moc"
