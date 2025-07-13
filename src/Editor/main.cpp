#include "lupine/Engine.h"
#include <iostream>
#include <memory>

#ifdef LUPINE_HAS_QT6
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QMessageBox>
#include <QWidget>
#endif

#ifdef LUPINE_HAS_QT6
class LupineEditorWindow : public QMainWindow {
    Q_OBJECT
    
public:
    LupineEditorWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupEngine();
    }
    
    ~LupineEditorWindow() {
        m_engine.Shutdown();
    }
    
private slots:
    void onNewProject() {
        m_logOutput->append("Creating new project...");
        m_engine.Log("New project requested");
    }
    
    void onOpenProject() {
        m_logOutput->append("Opening project...");
        m_engine.Log("Open project requested");
    }
    
    void onAbout() {
        QMessageBox::about(this, "About Lupine Editor", 
            QString("Lupine Game Engine Editor\nVersion: %1\n\nA cross-platform game development environment.")
            .arg(QString::fromStdString(Lupine::Engine::GetVersion())));
    }
    
    void onEngineLog() {
        // Simulate some engine activity
        static int counter = 0;
        counter++;
        
        QString message = QString("Engine tick #%1 - All systems operational").arg(counter);
        m_logOutput->append(message.toStdString().c_str());
        m_engine.Log(message.toStdString());
        
        if (counter >= 10) {
            m_engineTimer->stop();
            m_logOutput->append("Engine simulation completed");
        }
    }
    
private:
    void setupUI() {
        setWindowTitle("Lupine Game Engine Editor");
        setMinimumSize(800, 600);
        
        // Central widget
        QWidget* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        // Main layout
        QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
        
        // Header
        QLabel* headerLabel = new QLabel("Lupine Game Engine Editor", this);
        headerLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px;");
        headerLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(headerLabel);
        
        // Button layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        
        QPushButton* newProjectBtn = new QPushButton("New Project", this);
        QPushButton* openProjectBtn = new QPushButton("Open Project", this);
        QPushButton* aboutBtn = new QPushButton("About", this);
        
        connect(newProjectBtn, &QPushButton::clicked, this, &LupineEditorWindow::onNewProject);
        connect(openProjectBtn, &QPushButton::clicked, this, &LupineEditorWindow::onOpenProject);
        connect(aboutBtn, &QPushButton::clicked, this, &LupineEditorWindow::onAbout);
        
        buttonLayout->addWidget(newProjectBtn);
        buttonLayout->addWidget(openProjectBtn);
        buttonLayout->addStretch();
        buttonLayout->addWidget(aboutBtn);
        
        mainLayout->addLayout(buttonLayout);
        
        // Log output
        QLabel* logLabel = new QLabel("Engine Log:", this);
        mainLayout->addWidget(logLabel);
        
        m_logOutput = new QTextEdit(this);
        m_logOutput->setReadOnly(true);
        m_logOutput->setMaximumHeight(200);
        mainLayout->addWidget(m_logOutput);
        
        // Menu bar
        QMenuBar* menuBar = this->menuBar();
        QMenu* fileMenu = menuBar->addMenu("File");
        fileMenu->addAction("New Project", this, &LupineEditorWindow::onNewProject);
        fileMenu->addAction("Open Project", this, &LupineEditorWindow::onOpenProject);
        fileMenu->addSeparator();
        fileMenu->addAction("Exit", this, &QWidget::close);
        
        QMenu* helpMenu = menuBar->addMenu("Help");
        helpMenu->addAction("About", this, &LupineEditorWindow::onAbout);
        
        // Status bar
        statusBar()->showMessage("Ready");
        
        // Engine simulation timer
        m_engineTimer = new QTimer(this);
        connect(m_engineTimer, &QTimer::timeout, this, &LupineEditorWindow::onEngineLog);
    }
    
    void setupEngine() {
        if (m_engine.Initialize()) {
            m_logOutput->append("Engine initialized successfully");
            statusBar()->showMessage("Engine Ready");
            
            // Start engine simulation
            m_engineTimer->start(1000); // Every second
        } else {
            m_logOutput->append("Failed to initialize engine");
            statusBar()->showMessage("Engine Error");
        }
    }
    
private:
    Lupine::Engine m_engine;
    QTextEdit* m_logOutput;
    QTimer* m_engineTimer;
};

#include "main.moc"
#endif

int main(int argc, char* argv[]) {
    std::cout << "=== Lupine Game Engine Editor ===" << std::endl;
    std::cout << "Version: " << Lupine::Engine::GetVersion() << std::endl;
    
#ifdef LUPINE_HAS_QT6
    QApplication app(argc, argv);
    
    LupineEditorWindow window;
    window.show();
    
    // For CI testing, close after a short time
    QTimer::singleShot(5000, &app, &QApplication::quit);
    
    return app.exec();
#else
    std::cout << "Qt6 not available, running in console mode" << std::endl;
    
    Lupine::Engine engine;
    if (engine.Initialize()) {
        std::cout << "Engine initialized successfully" << std::endl;
        engine.Log("Editor running in console mode");
        engine.Shutdown();
        std::cout << "Editor finished successfully" << std::endl;
        return 0;
    } else {
        std::cerr << "Failed to initialize engine" << std::endl;
        return 1;
    }
#endif
}
