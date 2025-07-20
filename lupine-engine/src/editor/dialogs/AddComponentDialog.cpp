#include "AddComponentDialog.h"
#include "../IconManager.h"
#include <QApplication>
#include <QHeaderView>

AddComponentDialog::AddComponentDialog(QWidget* parent)
    : QDialog(parent)
    , m_loadingTimer(new QTimer(this))
{
    setWindowTitle("Add Component");
    setModal(true);
    resize(600, 400);

    setupUI();

    // Connect loading timer
    connect(m_loadingTimer, &QTimer::timeout, this, &AddComponentDialog::onLoadingTimer);

    // Don't populate components in constructor - do it when shown
}

QString AddComponentDialog::getSelectedComponentName() const {
    return m_selectedComponentName;
}

void AddComponentDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);

    // Load components asynchronously when dialog is first shown
    if (!m_componentsLoaded && !m_isLoading) {
        loadComponentsAsync();
    }
}

void AddComponentDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    // Search bar
    m_searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel("Search:");
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Type to filter components...");
    m_searchLayout->addWidget(searchLabel);
    m_searchLayout->addWidget(m_searchEdit);
    m_mainLayout->addLayout(m_searchLayout);
    
    // Splitter for tree and description
    m_splitter = new QSplitter(Qt::Horizontal);
    
    // Component tree
    m_componentTree = new QTreeWidget();
    m_componentTree->setHeaderLabel("Available Components");
    m_componentTree->setRootIsDecorated(true);
    m_componentTree->setSortingEnabled(true);
    m_componentTree->sortByColumn(0, Qt::AscendingOrder);
    m_splitter->addWidget(m_componentTree);
    
    // Description panel
    QWidget* descriptionWidget = new QWidget();
    QVBoxLayout* descriptionLayout = new QVBoxLayout(descriptionWidget);
    m_descriptionLabel = new QLabel("Component Description");
    m_descriptionLabel->setStyleSheet("font-weight: bold;");
    m_descriptionText = new QTextEdit();
    m_descriptionText->setReadOnly(true);
    m_descriptionText->setMaximumHeight(150);

    // Loading progress
    m_loadingProgress = new QProgressBar();
    m_loadingProgress->setVisible(false);
    m_loadingLabel = new QLabel("Loading components...");
    m_loadingLabel->setVisible(false);
    descriptionLayout->addWidget(m_descriptionLabel);
    descriptionLayout->addWidget(m_descriptionText);
    descriptionLayout->addWidget(m_loadingLabel);
    descriptionLayout->addWidget(m_loadingProgress);
    descriptionLayout->addStretch();
    m_splitter->addWidget(descriptionWidget);
    
    // Set splitter proportions
    m_splitter->setSizes({400, 200});
    m_mainLayout->addWidget(m_splitter);
    
    // Buttons
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();
    m_addButton = new QPushButton("Add Component");
    m_addButton->setEnabled(false);
    m_cancelButton = new QPushButton("Cancel");
    m_buttonLayout->addWidget(m_addButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AddComponentDialog::onSearchTextChanged);
    connect(m_componentTree, &QTreeWidget::itemSelectionChanged, this, &AddComponentDialog::onComponentSelectionChanged);
    connect(m_componentTree, &QTreeWidget::itemDoubleClicked, this, &AddComponentDialog::onComponentDoubleClicked);
    connect(m_addButton, &QPushButton::clicked, this, &AddComponentDialog::onAddButtonClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &AddComponentDialog::onCancelButtonClicked);
}

void AddComponentDialog::loadComponentsAsync() {
    if (m_isLoading || m_componentsLoaded) {
        return;
    }

    m_isLoading = true;
    m_loadingIndex = 0;

    // Show loading UI and hide component tree initially
    m_loadingLabel->setText("Loading components...");
    m_loadingLabel->setVisible(true);
    m_loadingProgress->setVisible(true);
    m_componentTree->setVisible(false);  // Hide tree while loading
    m_addButton->setEnabled(false);

    // Cache component information first
    cacheComponentInfo();

    // Start loading timer for progressive UI updates
    m_loadingTimer->start(10); // Update every 10ms
}

void AddComponentDialog::cacheComponentInfo() {
    // Get all registered components
    auto& registry = Lupine::ComponentRegistry::Instance();
    m_allComponentNames = registry.GetComponentNames();

    // Pre-cache all component information
    m_cachedComponents.clear();
    m_componentCache.clear();
    m_cachedComponents.reserve(m_allComponentNames.size());

    for (const auto& componentName : m_allComponentNames) {
        const Lupine::ComponentInfo* info = registry.GetComponentInfo(componentName);
        if (!info) continue;

        CachedComponentInfo cached;
        cached.name = componentName;
        cached.category = info->category;
        cached.description = info->description;
        cached.displayName = QString::fromStdString(info->name);
        cached.categoryName = QString::fromStdString(info->category);
        cached.descriptionText = QString::fromStdString(info->description);

        m_cachedComponents.push_back(cached);
        m_componentCache[componentName] = std::make_shared<CachedComponentInfo>(cached);
    }

    m_loadingProgress->setMaximum(static_cast<int>(m_cachedComponents.size()));
}

void AddComponentDialog::populateComponentTree() {
    if (!m_componentsLoaded) {
        return; // Don't populate until components are loaded
    }

    m_componentTree->clear();
    m_categoryItems.clear();

    // Use cached component information for faster population
    for (auto& cached : m_cachedComponents) {
        // Find or create category item
        QTreeWidgetItem* categoryItem = findOrCreateCategoryItem(cached.categoryName);

        // Create component item
        QTreeWidgetItem* componentItem = new QTreeWidgetItem(categoryItem);
        componentItem->setText(0, cached.displayName);
        componentItem->setData(0, Qt::UserRole, QString::fromStdString(cached.name));
        componentItem->setToolTip(0, cached.descriptionText);

        // Cache the tree item reference
        cached.treeItem = componentItem;

        // Set icon using the new icon system with fallback
        QIcon icon = IconManager::Instance().GetComponentIcon(cached.name, cached.category);
        componentItem->setIcon(0, icon);
    }

    // Expand all categories
    m_componentTree->expandAll();
}

void AddComponentDialog::filterComponents(const QString& filter) {
    if (!m_componentsLoaded) {
        return; // Can't filter until components are loaded
    }

    // Use cached data for faster filtering
    if (filter.isEmpty()) {
        // Show all items
        for (int i = 0; i < m_componentTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* categoryItem = m_componentTree->topLevelItem(i);
            categoryItem->setHidden(false);
            for (int j = 0; j < categoryItem->childCount(); ++j) {
                categoryItem->child(j)->setHidden(false);
            }
        }
        return;
    }

    // Filter using cached component data
    QString lowerFilter = filter.toLower();

    for (int i = 0; i < m_componentTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* categoryItem = m_componentTree->topLevelItem(i);
        bool categoryHasVisibleChildren = false;

        for (int j = 0; j < categoryItem->childCount(); ++j) {
            QTreeWidgetItem* componentItem = categoryItem->child(j);

            // Fast string matching using cached QString data
            bool matches = componentItem->text(0).toLower().contains(lowerFilter) ||
                          componentItem->toolTip(0).toLower().contains(lowerFilter);

            componentItem->setHidden(!matches);
            if (matches) {
                categoryHasVisibleChildren = true;
            }
        }

        categoryItem->setHidden(!categoryHasVisibleChildren);
    }
}

QTreeWidgetItem* AddComponentDialog::findOrCreateCategoryItem(const QString& category) {
    auto it = m_categoryItems.find(category.toStdString());
    if (it != m_categoryItems.end()) {
        return it->second;
    }
    
    QTreeWidgetItem* categoryItem = new QTreeWidgetItem(m_componentTree);
    categoryItem->setText(0, category);
    categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsSelectable);
    categoryItem->setExpanded(true);
    
    // Set category icon
    categoryItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    categoryItem->setFont(0, QFont("", -1, QFont::Bold));
    
    m_categoryItems[category.toStdString()] = categoryItem;
    return categoryItem;
}

void AddComponentDialog::onComponentSelectionChanged() {
    QList<QTreeWidgetItem*> selectedItems = m_componentTree->selectedItems();

    if (selectedItems.isEmpty() || !selectedItems.first()->parent()) {
        // No selection or category selected
        m_selectedComponentName.clear();
        m_addButton->setEnabled(false);
        m_descriptionText->clear();
        return;
    }

    QTreeWidgetItem* item = selectedItems.first();
    m_selectedComponentName = item->data(0, Qt::UserRole).toString();
    m_addButton->setEnabled(true);

    // Update description using cached data for faster access
    auto it = m_componentCache.find(m_selectedComponentName.toStdString());
    if (it != m_componentCache.end()) {
        m_descriptionText->setPlainText(it->second->descriptionText);
    } else {
        // Fallback to registry lookup if not in cache
        auto& registry = Lupine::ComponentRegistry::Instance();
        const Lupine::ComponentInfo* info = registry.GetComponentInfo(m_selectedComponentName.toStdString());
        if (info) {
            m_descriptionText->setPlainText(QString::fromStdString(info->description));
        }
    }
}

void AddComponentDialog::onComponentDoubleClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)
    
    if (item && item->parent()) {
        // Component item double-clicked
        m_selectedComponentName = item->data(0, Qt::UserRole).toString();
        accept();
    }
}

void AddComponentDialog::onSearchTextChanged(const QString& text) {
    filterComponents(text);
}

void AddComponentDialog::onAddButtonClicked() {
    if (!m_selectedComponentName.isEmpty()) {
        accept();
    }
}

void AddComponentDialog::onCancelButtonClicked() {
    reject();
}

void AddComponentDialog::onLoadingTimer() {
    if (!m_isLoading || m_componentsLoaded) {
        m_loadingTimer->stop();
        return;
    }

    // Process a few components per timer tick to keep UI responsive
    const size_t componentsPerTick = 5;
    size_t endIndex = std::min(m_loadingIndex + componentsPerTick, m_cachedComponents.size());

    // Update progress
    m_loadingProgress->setValue(static_cast<int>(endIndex));

    m_loadingIndex = endIndex;

    // Check if we're done loading
    if (m_loadingIndex >= m_cachedComponents.size()) {
        // Finish loading
        m_loadingTimer->stop();
        m_isLoading = false;
        m_componentsLoaded = true;

        // Hide loading UI and show component tree
        m_loadingLabel->setVisible(false);
        m_loadingProgress->setVisible(false);
        m_componentTree->setVisible(true);   // Show tree when loading complete
        m_componentTree->setEnabled(true);

        // Populate the tree with cached data
        populateComponentTree();

        // Update UI state
        updateLoadingProgress();
    }
}

void AddComponentDialog::updateLoadingProgress() {
    // Re-enable UI elements after loading
    if (m_componentsLoaded) {
        m_componentTree->setEnabled(true);
        // Add button will be enabled when a component is selected
    }
}
