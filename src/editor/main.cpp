#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QKeyEvent>
#include <QDebug>
#include <QThread>
#include <iostream>
#include <fstream>

#include "MainWindow.h"
#include "../core/CrashHandler.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

// Qt Static Plugin Imports for Windows
#ifdef QT_STATIC
    #include <QtPlugin>
    // Import the Windows platform plugin
    Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

class DebugKeyFilter : public QObject
{
public:
    DebugKeyFilter(bool& debugMode) : m_debugMode(debugMode) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_F3) {
                m_debugMode = true;
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    bool& m_debugMode;
};
#include "ProjectManager.h"
#include "MainWindow.h"
#include "lupine/core/Engine.h"
#include "lupine/core/Project.h"
#include "lupine/core/Scene.h"
#include "lupine/core/ComponentRegistration.h"

bool attachToParentConsole() {
#ifdef _WIN32
    // Try to attach to parent console (if launched from command line)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // Successfully attached to parent console
        // Redirect stdout, stdin, stderr to console
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

        // Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
        // point to console as well
        std::ios::sync_with_stdio(true);

        return true;
    }
    return false;
#else
    // On Unix-like systems, check if stdout is a terminal
    return isatty(STDOUT_FILENO);
#endif
}

bool g_attachedToParentConsole = false;

void setupConsole() {
#ifdef _WIN32
    // First try to attach to parent console (if launched from command line)
    if (attachToParentConsole()) {
        g_attachedToParentConsole = true;
        std::cout << std::endl; // Add newline after command prompt
        std::cout << "=== Lupine Editor (Command Line Mode) ===" << std::endl;
        std::cout << "Using parent console for output..." << std::endl;
        std::cout << "=========================================" << std::endl;
    } else {
        // No parent console, allocate a new console for this GUI application
        g_attachedToParentConsole = false;
        if (AllocConsole()) {
            // Redirect stdout, stdin, stderr to console
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
            freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
            freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

            // Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
            // point to console as well
            std::ios::sync_with_stdio(true);

            // Set console title
            SetConsoleTitleW(L"Lupine Editor Debug Console");

            std::cout << "=== Lupine Editor Debug Console ===" << std::endl;
            std::cout << "Console allocated successfully!" << std::endl;
            std::cout << "Press any key to close console when editor exits..." << std::endl;
            std::cout << "=====================================" << std::endl;
        }
    }
#endif
}

int main(int argc, char *argv[])
{
    // Write immediate startup marker to file (before anything else)
    try {
        std::ofstream startup_marker("startup_debug.txt");
        if (startup_marker) {
            startup_marker << "main() started" << std::endl;
            startup_marker.flush();
        }
    } catch (...) {
        // Ignore errors
    }

    // Setup console for debugging (use existing if launched from command line)
    setupConsole();

    std::cout << "Starting Lupine Editor..." << std::endl;

    // Write debug marker
    try {
        std::ofstream debug_file("startup_debug.txt", std::ios::app);
        if (debug_file) {
            debug_file << "About to initialize crash handler" << std::endl;
            debug_file.flush();
        }
    } catch (...) {}

    // Initialize crash handler first
    LUPINE_SAFE_EXECUTE(
        Lupine::CrashHandler::Initialize("logs", [](const std::string& crash_info) {
            std::cerr << "CRASH DETECTED: " << crash_info << std::endl;
            std::cout << "CRASH DETECTED: " << crash_info << std::endl;
        }),
        "Failed to initialize crash handler"
    );

    std::cout << "Crash handler initialized successfully" << std::endl;

    // Write debug marker
    try {
        std::ofstream debug_file("startup_debug.txt", std::ios::app);
        if (debug_file) {
            debug_file << "Crash handler initialized" << std::endl;
            debug_file.flush();
        }
    } catch (...) {}

    // Initialize component registry
    std::cout << "Initializing component registry..." << std::endl;
    LUPINE_SAFE_EXECUTE(
        Lupine::InitializeComponentRegistry(),
        "Failed to initialize component registry"
    );
    std::cout << "Component registry initialized." << std::endl;
    std::cout << "Arguments: ";
    for (int i = 0; i < argc; ++i) {
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl;

    // Log startup progress to file
    LUPINE_LOG_STARTUP("Application startup initiated");

    // Write debug marker before QApplication
    try {
        std::ofstream debug_file("startup_debug.txt", std::ios::app);
        if (debug_file) {
            debug_file << "About to create QApplication" << std::endl;
            debug_file.flush();
        }
    } catch (...) {}

    QApplication app(argc, argv);
    std::cout << "QApplication created successfully" << std::endl;
    LUPINE_LOG_STARTUP("QApplication created successfully");

    // Write debug marker after QApplication
    try {
        std::ofstream debug_file("startup_debug.txt", std::ios::app);
        if (debug_file) {
            debug_file << "QApplication created successfully" << std::endl;
            debug_file.flush();
        }
    } catch (...) {}

    // Check for debug mode (F3 or --debug flag)
    bool debugMode = false;
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--debug" || QString(argv[i]) == "-d") {
            debugMode = true;
            std::cout << "Debug mode enabled via command line" << std::endl;
            break;
        }
    }

    // Install event filter to catch F3 key
    DebugKeyFilter keyFilter(debugMode);
    app.installEventFilter(&keyFilter);
    std::cout << "Event filter installed" << std::endl;
    LUPINE_LOG_STARTUP("Event filter installed");

    // Set application properties
    app.setApplicationName("Lupine Editor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Lupine Engine");
    app.setOrganizationDomain("lupine-engine.org");
    std::cout << "Application properties set" << std::endl;
    LUPINE_LOG_STARTUP("Application properties set");
    
    // Set application style
    app.setStyle(QStyleFactory::create("Fusion"));
    std::cout << "Application style set to Fusion" << std::endl;

    // Apply modern dark theme with purple accents
    std::cout << "Setting up dark theme..." << std::endl;
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 35));           // Dark background
    darkPalette.setColor(QPalette::WindowText, QColor(220, 220, 220));    // Light text
    darkPalette.setColor(QPalette::Base, QColor(20, 20, 25));             // Input backgrounds
    darkPalette.setColor(QPalette::AlternateBase, QColor(40, 40, 45));    // Alternate rows
    darkPalette.setColor(QPalette::ToolTipBase, QColor(50, 50, 55));      // Tooltip background
    darkPalette.setColor(QPalette::ToolTipText, QColor(220, 220, 220));   // Tooltip text
    darkPalette.setColor(QPalette::Text, QColor(220, 220, 220));          // General text
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 50));           // Button background
    darkPalette.setColor(QPalette::ButtonText, QColor(220, 220, 220));    // Button text
    darkPalette.setColor(QPalette::BrightText, QColor(255, 100, 100));    // Error text
    darkPalette.setColor(QPalette::Link, QColor(147, 112, 219));          // Purple links
    darkPalette.setColor(QPalette::Highlight, QColor(138, 43, 226));      // Purple highlight
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);           // Highlighted text
    app.setPalette(darkPalette);

    // Apply modern stylesheet
    app.setStyleSheet(
        "QMainWindow { background-color: #1e1e23; }"
        "QMenuBar { background-color: #2d2d32; color: #dcdcdc; border-bottom: 1px solid #000000; }"
        "QMenuBar::item { background-color: transparent; padding: 4px 8px; }"
        "QMenuBar::item:selected { background-color: #8a2be2; }"
        "QMenu { background-color: #2d2d32; color: #dcdcdc; border: 1px solid #000000; }"
        "QMenu::item:selected { background-color: #8a2be2; }"
        "QToolBar { background-color: #2d2d32; border: 1px solid #000000; }"
        "QDockWidget { background-color: #1e1e23; color: #dcdcdc; }"
        "QDockWidget::title { background-color: #2d2d32; padding: 4px; border: 1px solid #000000; }"
        "QPushButton { background-color: #2d2d32; color: #dcdcdc; border: 1px solid #000000; padding: 6px 12px; border-radius: 3px; }"
        "QPushButton:hover { background-color: #8a2be2; }"
        "QPushButton:pressed { background-color: #7b68ee; }"
        "QTreeWidget, QListWidget { background-color: #14141a; color: #dcdcdc; border: 1px solid #000000; }"
        "QTreeWidget::item:selected, QListWidget::item:selected { background-color: #8a2be2; }"
        "QLineEdit { background-color: #14141a; color: #dcdcdc; border: 1px solid #000000; padding: 4px; border-radius: 3px; }"
        "QTextEdit { background-color: #14141a; color: #dcdcdc; border: 1px solid #000000; }"
        "QScrollBar:vertical { background-color: #2d2d32; width: 12px; }"
        "QScrollBar::handle:vertical { background-color: #8a2be2; border-radius: 6px; }"
        "QScrollBar::handle:vertical:hover { background-color: #9370db; }"
    );
    
    // Create a pointer to store the main window
    MainWindow* mainWindow = nullptr;
    ProjectManager* projectManager = nullptr;

    if (debugMode) {
        // Debug mode: Open editor directly without project
        std::cout << "Starting in debug mode..." << std::endl;

        std::cout << "Creating MainWindow..." << std::endl;
        mainWindow = new MainWindow();
        std::cout << "MainWindow created successfully" << std::endl;

        std::cout << "Showing MainWindow..." << std::endl;
        mainWindow->show();
        std::cout << "MainWindow shown" << std::endl;
    } else {
        // Normal mode: Show project manager
        std::cout << "Starting in normal mode..." << std::endl;
        projectManager = new ProjectManager();

        if (projectManager->exec() == QDialog::Accepted) {
            QString projectPath = projectManager->getSelectedProjectPath();

            if (!projectPath.isEmpty()) {
                // Open main editor window
                std::cout << "Creating MainWindow for project: " << projectPath.toStdString() << std::endl;
                LUPINE_SAFE_EXECUTE({
                    mainWindow = new MainWindow();
                    if (!mainWindow) {
                        LUPINE_LOG_CRITICAL("Failed to create MainWindow");
                        throw std::runtime_error("Failed to create MainWindow");
                    }
                    std::cout << "MainWindow created successfully" << std::endl;

                    mainWindow->openProject(projectPath);
                    std::cout << "Project opened successfully" << std::endl;
                    LUPINE_LOG_STARTUP("Project opened successfully");

                    mainWindow->show();
                    std::cout << "MainWindow shown" << std::endl;
                    LUPINE_LOG_STARTUP("MainWindow shown");

                    // Force process events to ensure window is fully initialized
                    std::cout << "Processing initial events..." << std::endl;
                    LUPINE_LOG_STARTUP("Processing initial events");

                    LUPINE_SAFE_EXECUTE({
                        QApplication::processEvents();
                        std::cout << "Initial events processed" << std::endl;
                        LUPINE_LOG_STARTUP("Initial events processed");
                    }, "Error during initial event processing");

                    // Add a longer delay to allow all panels to fully initialize
                    std::cout << "Allowing panels to initialize..." << std::endl;
                    LUPINE_LOG_STARTUP("Allowing panels to initialize");
                    QThread::msleep(1500);

                    // Process events again to handle any deferred operations
                    std::cout << "Processing deferred events..." << std::endl;
                    LUPINE_LOG_STARTUP("Processing deferred events");

                    LUPINE_SAFE_EXECUTE({
                        QApplication::processEvents();
                        std::cout << "Deferred events processed successfully" << std::endl;
                        LUPINE_LOG_STARTUP("Deferred events processed successfully");
                    }, "Error processing deferred events");

                    std::cout << "Post-initialization delay completed" << std::endl;
                    LUPINE_LOG_STARTUP("Post-initialization delay completed");

                }, "Failed to initialize MainWindow");
            } else {
                std::cout << "No project selected, exiting..." << std::endl;
                delete projectManager;
                return 0;
            }
        } else {
            std::cout << "Project manager cancelled, exiting..." << std::endl;
            delete projectManager;
            return 0;
        }

        delete projectManager;
        projectManager = nullptr;
    }

    int finalResult = 0;

    // Only start the main event loop if we have a main window
    if (mainWindow) {
        std::cout << "Starting main event loop..." << std::endl;
        LUPINE_LOG_STARTUP("Starting main event loop");

        LUPINE_SAFE_EXECUTE({
            finalResult = app.exec();
            std::cout << "Main event loop exited with code: " << finalResult << std::endl;
            LUPINE_LOG_STARTUP("Main event loop exited with code: " + std::to_string(finalResult));
        }, "Critical error in main event loop");
    } else {
        std::cout << "No main window created, exiting..." << std::endl;
        LUPINE_LOG_STARTUP("No main window created, exiting");
        finalResult = 0;
    }

    // Clean up
    LUPINE_SAFE_EXECUTE({
        if (mainWindow) {
            delete mainWindow;
            mainWindow = nullptr;
            std::cout << "MainWindow cleaned up" << std::endl;
        }

        // Shutdown crash handler
        Lupine::CrashHandler::Shutdown();

    }, "Error during cleanup");

    std::cout << "Final application exit code: " << finalResult << std::endl;

#ifdef _WIN32
    // Only wait for input if we allocated our own console (not attached to parent)
    if (!g_attachedToParentConsole) {
        std::cout << "Press any key to close console..." << std::endl;
        std::cin.get();
    } else {
        std::cout << "Exiting (attached to parent console)..." << std::endl;
        // Add a newline to separate from next command prompt
        std::cout << std::endl;
    }
#endif

    return finalResult;
}
