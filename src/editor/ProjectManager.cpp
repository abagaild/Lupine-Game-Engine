#include "ProjectManager.h"
#include "lupine/core/Project.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/serialization/SceneSerializer.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>

ProjectManager::ProjectManager(QWidget *parent)
    : QDialog(parent)
    , m_selectedProjectPath("")
    , m_settings(new QSettings(this))
{
    setWindowTitle("Lupine Project Manager");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);
    resize(800, 600);
    
    setupUI();
    loadRecentProjects();
    setupRecentProjects();
}

ProjectManager::~ProjectManager()
{
    saveRecentProjects();
}

void ProjectManager::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_contentLayout = new QHBoxLayout();
    
    // Left side - Logo and recent projects
    m_leftLayout = new QVBoxLayout();
    
    // Logo
    m_logoLabel = new QLabel();
    QPixmap logo(":/images/logo.png");
    if (logo.isNull()) {
        // Create placeholder logo
        logo = QPixmap(200, 100);
        logo.fill(QColor(42, 130, 218));
    } else {
        logo = logo.scaled(200, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    m_logoLabel->setPixmap(logo);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    
    // Title
    m_titleLabel = new QLabel("Lupine Game Engine");
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #8a2be2;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    // Recent projects
    m_recentLabel = new QLabel("Recent Projects:");
    m_recentLabel->setStyleSheet("font-weight: bold; margin-top: 20px;");
    
    m_recentProjectsList = new QListWidget();
    m_recentProjectsList->setMinimumHeight(300);
    
    m_openProjectButton = new QPushButton("Open Existing Project...");
    
    m_leftLayout->addWidget(m_logoLabel);
    m_leftLayout->addWidget(m_titleLabel);
    m_leftLayout->addWidget(m_recentLabel);
    m_leftLayout->addWidget(m_recentProjectsList);
    m_leftLayout->addWidget(m_openProjectButton);
    m_leftLayout->addStretch();
    
    // Right side - New project
    m_rightLayout = new QVBoxLayout();
    
    m_newProjectLabel = new QLabel("Create New Project:");
    m_newProjectLabel->setStyleSheet("font-weight: bold;");
    
    QLabel* nameLabel = new QLabel("Project Name:");
    m_projectNameEdit = new QLineEdit();
    m_projectNameEdit->setPlaceholderText("Enter project name...");
    
    QLabel* locationLabel = new QLabel("Location:");
    QHBoxLayout* locationLayout = new QHBoxLayout();
    m_projectLocationEdit = new QLineEdit();
    m_projectLocationEdit->setText(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/LupineProjects");
    m_browseLocationButton = new QPushButton("Browse...");
    locationLayout->addWidget(m_projectLocationEdit);
    locationLayout->addWidget(m_browseLocationButton);
    
    m_createProjectButton = new QPushButton("Create Project");
    m_createProjectButton->setEnabled(false);
    m_createProjectButton->setStyleSheet("QPushButton { background-color: #8a2be2; color: white; font-weight: bold; padding: 8px; border-radius: 4px; }");
    
    // Add stretch before content to center it vertically
    m_rightLayout->addStretch();
    m_rightLayout->addWidget(m_newProjectLabel);
    m_rightLayout->addWidget(nameLabel);
    m_rightLayout->addWidget(m_projectNameEdit);
    m_rightLayout->addWidget(locationLabel);
    m_rightLayout->addLayout(locationLayout);
    m_rightLayout->addWidget(m_createProjectButton);
    // Add stretch after content to center it vertically
    m_rightLayout->addStretch();
    
    // Add layouts to content
    m_contentLayout->addLayout(m_leftLayout, 1);
    m_contentLayout->addLayout(m_rightLayout, 1);
    
    // Bottom buttons
    m_buttonLayout = new QHBoxLayout();
    m_cancelButton = new QPushButton("Cancel");
    m_openButton = new QPushButton("Open");
    m_openButton->setEnabled(false);
    
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addWidget(m_openButton);
    
    // Add to main layout
    m_mainLayout->addLayout(m_contentLayout);
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_openProjectButton, &QPushButton::clicked, this, &ProjectManager::onOpenProject);
    connect(m_recentProjectsList, &QListWidget::itemClicked, this, &ProjectManager::onRecentProjectClicked);
    connect(m_recentProjectsList, &QListWidget::itemDoubleClicked, this, &ProjectManager::onOpenSelectedProject);
    connect(m_projectNameEdit, &QLineEdit::textChanged, this, &ProjectManager::onProjectNameChanged);
    connect(m_browseLocationButton, &QPushButton::clicked, this, &ProjectManager::onBrowseLocation);
    connect(m_createProjectButton, &QPushButton::clicked, this, &ProjectManager::onCreateProject);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_openButton, &QPushButton::clicked, this, &ProjectManager::onOpenSelectedProject);
}

void ProjectManager::setupRecentProjects()
{
    m_recentProjectsList->clear();
    
    for (const QString& projectPath : m_recentProjects) {
        QFileInfo fileInfo(projectPath);
        if (fileInfo.exists()) {
            QString projectName = fileInfo.baseName();
            QListWidgetItem* item = new QListWidgetItem(projectName);
            item->setData(Qt::UserRole, projectPath);
            item->setToolTip(projectPath);
            m_recentProjectsList->addItem(item);
        }
    }
}

void ProjectManager::onNewProject()
{
    // Focus on new project section
    m_projectNameEdit->setFocus();
}

void ProjectManager::onOpenProject()
{
    QString projectPath = QFileDialog::getOpenFileName(
        this,
        "Open Lupine Project",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Lupine Projects (*.lupine)"
    );
    
    if (!projectPath.isEmpty()) {
        m_selectedProjectPath = projectPath;
        addRecentProject(projectPath);
        accept();
    }
}

void ProjectManager::onRecentProjectClicked()
{
    QListWidgetItem* item = m_recentProjectsList->currentItem();
    if (item) {
        m_selectedProjectPath = item->data(Qt::UserRole).toString();
        m_openButton->setEnabled(true);
    }
}

void ProjectManager::onProjectNameChanged()
{
    bool hasName = !m_projectNameEdit->text().trimmed().isEmpty();
    m_createProjectButton->setEnabled(hasName);
}

void ProjectManager::onBrowseLocation()
{
    QString location = QFileDialog::getExistingDirectory(
        this,
        "Select Project Location",
        m_projectLocationEdit->text()
    );
    
    if (!location.isEmpty()) {
        m_projectLocationEdit->setText(location);
    }
}

void ProjectManager::onCreateProject()
{
    QString name = m_projectNameEdit->text().trimmed();
    QString location = m_projectLocationEdit->text().trimmed();
    
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Invalid Name", "Please enter a project name.");
        return;
    }
    
    if (createNewProject(name, location)) {
        QString projectDir = QDir(location).filePath(name);
        QString projectPath = QDir(projectDir).filePath(name + ".lupine");
        m_selectedProjectPath = projectPath;
        addRecentProject(projectPath);
        accept();
    }
}

void ProjectManager::onOpenSelectedProject()
{
    if (!m_selectedProjectPath.isEmpty()) {
        accept();
    }
}

void ProjectManager::addRecentProject(const QString& projectPath)
{
    m_recentProjects.removeAll(projectPath);
    m_recentProjects.prepend(projectPath);
    
    while (m_recentProjects.size() > MAX_RECENT_PROJECTS) {
        m_recentProjects.removeLast();
    }
    
    setupRecentProjects();
}

void ProjectManager::loadRecentProjects()
{
    m_recentProjects = m_settings->value("recentProjects").toStringList();
}

void ProjectManager::saveRecentProjects()
{
    m_settings->setValue("recentProjects", m_recentProjects);
}

bool ProjectManager::createNewProject(const QString& name, const QString& location)
{
    QDir locationDir(location);
    if (!locationDir.exists()) {
        if (!locationDir.mkpath(".")) {
            QMessageBox::critical(this, "Error", "Failed to create project directory.");
            return false;
        }
    }

    QString projectDir = locationDir.filePath(name);
    if (QDir(projectDir).exists()) {
        QMessageBox::warning(this, "Project Exists", "A project with this name already exists in the selected location.");
        return false;
    }

    try {
        // Create project directory
        if (!QDir().mkpath(projectDir)) {
            QMessageBox::critical(this, "Error", "Failed to create project directory.");
            return false;
        }

        // Create subdirectories
        QDir projDir(projectDir);
        projDir.mkpath("assets");
        projDir.mkpath("assets/textures");
        projDir.mkpath("assets/models");
        projDir.mkpath("assets/audio");
        projDir.mkpath("assets/scripts");
        projDir.mkpath("scenes");

        // Use Lupine engine to create the project
        if (!Lupine::Project::CreateProject(projectDir.toStdString(), name.toStdString())) {
            QMessageBox::critical(this, "Error", "Failed to create project files.");
            return false;
        }

        // Create a default main scene
        QString scenesDir = QDir(projectDir).filePath("scenes");
        QString mainScenePath = QDir(scenesDir).filePath("main.scene");

        // Create a simple default scene
        auto defaultScene = std::make_unique<Lupine::Scene>("Main Scene");
        defaultScene->CreateRootNode<Lupine::Node>("Root");

        if (!Lupine::SceneSerializer::SerializeToFile(defaultScene.get(), mainScenePath.toStdString())) {
            QMessageBox::warning(this, "Warning", "Failed to create default scene, but project was created successfully.");
        }

        return true;
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to create project: %1").arg(e.what()));
        return false;
    }
}
