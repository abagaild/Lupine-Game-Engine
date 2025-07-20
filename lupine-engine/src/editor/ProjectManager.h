#pragma once

#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QPixmap>
#include <QSplitter>

class ProjectManager : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectManager(QWidget *parent = nullptr);
    ~ProjectManager();
    
    QString getSelectedProjectPath() const { return m_selectedProjectPath; }

private slots:
    void onNewProject();
    void onOpenProject();
    void onRecentProjectClicked();
    void onProjectNameChanged();
    void onBrowseLocation();
    void onCreateProject();
    void onOpenSelectedProject();

private:
    void setupUI();
    void setupRecentProjects();
    void addRecentProject(const QString& projectPath);
    void loadRecentProjects();
    void saveRecentProjects();
    bool createNewProject(const QString& name, const QString& location);
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_contentLayout;
    QVBoxLayout* m_leftLayout;
    QVBoxLayout* m_rightLayout;
    
    // Left side - Recent projects
    QLabel* m_logoLabel;
    QLabel* m_titleLabel;
    QLabel* m_recentLabel;
    QListWidget* m_recentProjectsList;
    QPushButton* m_openProjectButton;
    
    // Right side - New project
    QLabel* m_newProjectLabel;
    QLineEdit* m_projectNameEdit;
    QLineEdit* m_projectLocationEdit;
    QPushButton* m_browseLocationButton;
    QPushButton* m_createProjectButton;
    
    // Bottom buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_cancelButton;
    QPushButton* m_openButton;
    
    // Data
    QString m_selectedProjectPath;
    QStringList m_recentProjects;
    QSettings* m_settings;
    
    // Constants
    static const int MAX_RECENT_PROJECTS = 10;
};

#endif // PROJECTMANAGER_H
