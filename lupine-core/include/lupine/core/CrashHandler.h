#pragma once

#include <string>
#include <functional>
#include <fstream>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#include <signal.h>
#include <exception>
#endif

namespace Lupine {

/**
 * @brief Comprehensive crash handler for debugging and error reporting
 */
class CrashHandler {
public:
    using CrashCallback = std::function<void(const std::string& crash_info)>;
    
    /**
     * @brief Initialize the crash handler
     * @param log_directory Directory to write crash logs
     * @param callback Optional callback for crash notifications
     */
    static void Initialize(const std::string& log_directory = "logs", 
                          CrashCallback callback = nullptr);
    
    /**
     * @brief Shutdown the crash handler
     */
    static void Shutdown();
    
    /**
     * @brief Log a critical error with stack trace
     * @param message Error message
     * @param file Source file name
     * @param line Line number
     * @param function Function name
     */
    static void LogCriticalError(const std::string& message,
                                const char* file = nullptr,
                                int line = 0,
                                const char* function = nullptr);
    
    /**
     * @brief Log an exception with details
     * @param exception The exception to log
     * @param context Additional context information
     */
    static void LogException(const std::exception& exception, 
                           const std::string& context = "");
    
    /**
     * @brief Log current stack trace
     * @param message Optional message to include
     */
    static void LogStackTrace(const std::string& message = "Stack trace");
    
    /**
     * @brief Check if crash handler is initialized
     */
    static bool IsInitialized();

    /**
     * @brief Log startup progress to file (for debugging crashes during startup)
     * @param message Progress message
     */
    static void LogStartupProgress(const std::string& message);

    /**
     * @brief Track function calls to detect recursion
     * @param function_name Name of the function being called
     * @param max_depth Maximum allowed recursion depth (default: 50)
     * @return true if recursion limit exceeded
     */
    static bool TrackFunctionCall(const std::string& function_name, int max_depth = 50);

    /**
     * @brief Remove function from call tracking (call when exiting function)
     * @param function_name Name of the function being exited
     */
    static void UntrackFunctionCall(const std::string& function_name);

    /**
     * @brief Log current call stack for debugging
     */
    static void LogCallStack();

private:
    static std::unique_ptr<CrashHandler> s_instance;
    static std::string s_log_directory;
    static CrashCallback s_crash_callback;
    static bool s_initialized;

    // Recursion tracking
    static std::unordered_map<std::string, int> s_function_call_counts;
    static std::mutex s_call_tracking_mutex;

public:
    CrashHandler();

public:
    ~CrashHandler();

private:
    
    void SetupSignalHandlers();
    void SetupExceptionHandlers();
    
    static void WriteStackTrace(std::ofstream& log_file);
    static void WriteCrashLog(const std::string& crash_type, 
                             const std::string& message,
                             const std::string& additional_info = "");
    
    static std::string GetTimestamp();
    static std::string GetSystemInfo();
    static std::string GetModuleInfo();
    
#ifdef _WIN32
    static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* exception_info);
    static void SignalHandler(int signal);
    static void PureCallHandler();
    static void InvalidParameterHandler(const wchar_t* expression,
                                       const wchar_t* function,
                                       const wchar_t* file,
                                       unsigned int line,
                                       uintptr_t reserved);
    static void TerminateHandler();
    
    static std::string GetExceptionString(DWORD exception_code);
    static void WriteWindowsStackTrace(std::ofstream& log_file, CONTEXT* context = nullptr);
#endif
};

// Convenience macros for crash logging
#define LUPINE_LOG_CRITICAL(message) \
    Lupine::CrashHandler::LogCriticalError(message, __FILE__, __LINE__, __FUNCTION__)

#define LUPINE_LOG_EXCEPTION(exception, context) \
    Lupine::CrashHandler::LogException(exception, context)

#define LUPINE_LOG_STACK_TRACE(message) \
    Lupine::CrashHandler::LogStackTrace(message)

#define LUPINE_LOG_STARTUP(message) \
    Lupine::CrashHandler::LogStartupProgress(message)

// Safe execution macro with exception handling
#define LUPINE_SAFE_EXECUTE(code, error_message) \
    try { \
        code; \
    } catch (const std::exception& e) { \
        LUPINE_LOG_EXCEPTION(e, error_message); \
    } catch (...) { \
        LUPINE_LOG_CRITICAL(std::string(error_message) + " - Unknown exception"); \
    }

// Recursion tracking macros
#define LUPINE_TRACK_FUNCTION() \
    if (Lupine::CrashHandler::TrackFunctionCall(__FUNCTION__)) { \
        LUPINE_LOG_CRITICAL(std::string("RECURSION DETECTED in ") + __FUNCTION__); \
        Lupine::CrashHandler::LogCallStack(); \
        return; \
    }

#define LUPINE_TRACK_FUNCTION_WITH_RETURN(return_value) \
    if (Lupine::CrashHandler::TrackFunctionCall(__FUNCTION__)) { \
        LUPINE_LOG_CRITICAL(std::string("RECURSION DETECTED in ") + __FUNCTION__); \
        Lupine::CrashHandler::LogCallStack(); \
        return return_value; \
    }

#define LUPINE_UNTRACK_FUNCTION() \
    Lupine::CrashHandler::UntrackFunctionCall(__FUNCTION__)

} // namespace Lupine

// RAII class for automatic function tracking
class FunctionTracker {
public:
    FunctionTracker(const std::string& func_name) : function_name(func_name) {
        if (Lupine::CrashHandler::TrackFunctionCall(function_name)) {
            LUPINE_LOG_CRITICAL(std::string("RECURSION DETECTED in ") + function_name);
            Lupine::CrashHandler::LogCallStack();
            recursion_detected = true;
        }
    }
    ~FunctionTracker() {
        if (!recursion_detected) {
            Lupine::CrashHandler::UntrackFunctionCall(function_name);
        }
    }
    bool HasRecursion() const { return recursion_detected; }
private:
    std::string function_name;
    bool recursion_detected = false;
};

namespace Lupine {

#define LUPINE_AUTO_TRACK_FUNCTION() \
    FunctionTracker _func_tracker(__FUNCTION__); \
    if (_func_tracker.HasRecursion()) return;

} // namespace Lupine
