#include "CrashHandler.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <thread>
#include <csignal>
#include <ctime>
#include <unordered_map>
#include <mutex>

#ifdef _WIN32
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")
#endif

namespace Lupine {

std::unique_ptr<CrashHandler> CrashHandler::s_instance = nullptr;
std::string CrashHandler::s_log_directory = "logs";
CrashHandler::CrashCallback CrashHandler::s_crash_callback = nullptr;
bool CrashHandler::s_initialized = false;

// Recursion tracking static members
std::unordered_map<std::string, int> CrashHandler::s_function_call_counts;
std::mutex CrashHandler::s_call_tracking_mutex;

void CrashHandler::Initialize(const std::string& log_directory, CrashCallback callback) {
    if (s_initialized) {
        return;
    }
    
    s_log_directory = log_directory;
    s_crash_callback = callback;
    
    // Create log directory if it doesn't exist
    try {
        std::filesystem::create_directories(s_log_directory);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create crash log directory: " << e.what() << std::endl;
        return;
    }
    
    s_instance = std::make_unique<CrashHandler>();
    s_initialized = true;
    
    std::cout << "CrashHandler initialized successfully. Logs will be written to: "
              << s_log_directory << std::endl;

    // Write initial startup log
    try {
        std::string timestamp = GetTimestamp();
        std::string filename = s_log_directory + "/startup_" + timestamp + ".log";
        std::ofstream startup_log(filename);
        if (startup_log) {
            startup_log << "=== LUPINE EDITOR STARTUP LOG ===" << std::endl;
            startup_log << "Timestamp: " << timestamp << std::endl;
            startup_log << "CrashHandler initialized successfully" << std::endl;
            startup_log.flush();
        }
    } catch (...) {
        // Ignore startup log errors
    }
}

void CrashHandler::Shutdown() {
    if (s_initialized) {
        s_instance.reset();
        s_initialized = false;
        std::cout << "CrashHandler shutdown completed." << std::endl;
    }
}

CrashHandler::CrashHandler() {
    SetupSignalHandlers();
    SetupExceptionHandlers();
}

CrashHandler::~CrashHandler() {
    // Restore default handlers
#ifdef _WIN32
    SetUnhandledExceptionFilter(nullptr);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
#endif
}

void CrashHandler::SetupSignalHandlers() {
#ifdef _WIN32
    signal(SIGABRT, SignalHandler);
    signal(SIGFPE, SignalHandler);
    signal(SIGILL, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGSEGV, SignalHandler);
    signal(SIGTERM, SignalHandler);
#endif
}

void CrashHandler::SetupExceptionHandlers() {
#ifdef _WIN32
    SetUnhandledExceptionFilter(UnhandledExceptionFilter);
    _set_purecall_handler(PureCallHandler);
    _set_invalid_parameter_handler(InvalidParameterHandler);
    std::set_terminate(TerminateHandler);
    // std::set_unexpected is deprecated in C++17 and removed in C++20
#endif
}

void CrashHandler::LogCriticalError(const std::string& message, 
                                   const char* file, 
                                   int line, 
                                   const char* function) {
    if (!s_initialized) {
        std::cerr << "CRITICAL ERROR (CrashHandler not initialized): " << message << std::endl;
        return;
    }
    
    std::ostringstream additional_info;
    if (file) additional_info << "File: " << file << "\n";
    additional_info << "Line: " << line << "\n";
    if (function) additional_info << "Function: " << function << "\n";
    
    WriteCrashLog("CRITICAL_ERROR", message, additional_info.str());
}

void CrashHandler::LogException(const std::exception& exception, const std::string& context) {
    if (!s_initialized) {
        std::cerr << "EXCEPTION (CrashHandler not initialized): " << exception.what() << std::endl;
        return;
    }
    
    std::ostringstream additional_info;
    additional_info << "Exception type: " << typeid(exception).name() << "\n";
    additional_info << "Context: " << context << "\n";
    
    WriteCrashLog("EXCEPTION", exception.what(), additional_info.str());
}

void CrashHandler::LogStackTrace(const std::string& message) {
    if (!s_initialized) {
        std::cerr << "STACK_TRACE (CrashHandler not initialized): " << message << std::endl;
        return;
    }
    
    WriteCrashLog("STACK_TRACE", message);
}

bool CrashHandler::IsInitialized() {
    return s_initialized;
}

void CrashHandler::LogStartupProgress(const std::string& message) {
    try {
        std::string timestamp = GetTimestamp();
        std::string filename = s_log_directory + "/startup_" + timestamp.substr(0, 8) + ".log";

        std::ofstream startup_log(filename, std::ios::app);
        if (startup_log) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            std::tm* tm_ptr = std::localtime(&time_t);
            if (tm_ptr) {
                startup_log << std::put_time(tm_ptr, "%H:%M:%S");
                startup_log << "." << std::setfill('0') << std::setw(3) << ms.count();
                startup_log << " - " << message << std::endl;
                startup_log.flush(); // Force immediate write
            }
        }
    } catch (...) {
        // Ignore errors in startup logging to prevent recursive crashes
    }
}

void CrashHandler::WriteCrashLog(const std::string& crash_type, 
                                const std::string& message,
                                const std::string& additional_info) {
    try {
        std::string timestamp = GetTimestamp();
        std::string filename = s_log_directory + "/crash_" + timestamp + ".log";
        
        std::ofstream log_file(filename, std::ios::app);
        if (!log_file) {
            std::cerr << "Failed to open crash log file: " << filename << std::endl;
            return;
        }
        
        log_file << "=====================================\n";
        log_file << "LUPINE ENGINE CRASH REPORT\n";
        log_file << "=====================================\n";
        log_file << "Timestamp: " << timestamp << "\n";
        log_file << "Crash Type: " << crash_type << "\n";
        log_file << "Message: " << message << "\n";
        log_file << "Thread ID: " << std::this_thread::get_id() << "\n";
        log_file << "\n";
        
        if (!additional_info.empty()) {
            log_file << "Additional Information:\n";
            log_file << additional_info << "\n";
        }
        
        log_file << "System Information:\n";
        log_file << GetSystemInfo() << "\n";
        
        log_file << "Module Information:\n";
        log_file << GetModuleInfo() << "\n";
        
        log_file << "Stack Trace:\n";
        WriteStackTrace(log_file);
        
        log_file << "=====================================\n\n";
        log_file.flush();
        
        std::cout << "Crash logged to: " << filename << std::endl;
        
        if (s_crash_callback) {
            s_crash_callback(crash_type + ": " + message);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to write crash log: " << e.what() << std::endl;
    }
}

std::string CrashHandler::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    std::tm* tm_ptr = std::localtime(&time_t);
    if (tm_ptr) {
        oss << std::put_time(tm_ptr, "%Y%m%d_%H%M%S");
    } else {
        oss << "unknown_time";
    }
    oss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string CrashHandler::GetSystemInfo() {
    std::ostringstream info;
    
#ifdef _WIN32
    SYSTEM_INFO sys_info;
    ::GetSystemInfo(&sys_info);
    
    MEMORYSTATUSEX mem_info;
    mem_info.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&mem_info);
    
    info << "Platform: Windows\n";
    info << "Processor Architecture: " << sys_info.wProcessorArchitecture << "\n";
    info << "Number of Processors: " << sys_info.dwNumberOfProcessors << "\n";
    info << "Page Size: " << sys_info.dwPageSize << " bytes\n";
    info << "Total Physical Memory: " << (mem_info.ullTotalPhys / 1024 / 1024) << " MB\n";
    info << "Available Physical Memory: " << (mem_info.ullAvailPhys / 1024 / 1024) << " MB\n";
#else
    info << "Platform: Unix/Linux\n";
#endif
    
    return info.str();
}

std::string CrashHandler::GetModuleInfo() {
    std::ostringstream info;
    
#ifdef _WIN32
    HANDLE process = GetCurrentProcess();
    HMODULE modules[1024];
    DWORD bytes_needed;
    
    if (EnumProcessModules(process, modules, sizeof(modules), &bytes_needed)) {
        int module_count = bytes_needed / sizeof(HMODULE);
        
        for (int i = 0; i < module_count && i < 10; ++i) { // Limit to first 10 modules
            char module_name[MAX_PATH];
            if (GetModuleFileNameExA(process, modules[i], module_name, sizeof(module_name))) {
                info << "Module " << i << ": " << module_name << "\n";
            }
        }
    }
#endif
    
    return info.str();
}

void CrashHandler::WriteStackTrace(std::ofstream& log_file) {
#ifdef _WIN32
    WriteWindowsStackTrace(log_file);
#else
    log_file << "Stack trace not available on this platform\n";
#endif
}

#ifdef _WIN32
LONG WINAPI CrashHandler::UnhandledExceptionFilter(EXCEPTION_POINTERS* exception_info) {
    std::string exception_string = GetExceptionString(exception_info->ExceptionRecord->ExceptionCode);

    std::ostringstream additional_info;
    additional_info << "Exception Code: 0x" << std::hex << exception_info->ExceptionRecord->ExceptionCode << "\n";
    additional_info << "Exception Address: 0x" << std::hex << exception_info->ExceptionRecord->ExceptionAddress << "\n";
    additional_info << "Exception Flags: 0x" << std::hex << exception_info->ExceptionRecord->ExceptionFlags << "\n";

    WriteCrashLog("UNHANDLED_EXCEPTION", exception_string, additional_info.str());

    // Write stack trace with context
    try {
        std::string timestamp = GetTimestamp();
        std::string filename = s_log_directory + "/crash_" + timestamp + "_stacktrace.log";
        std::ofstream stack_file(filename);
        if (stack_file) {
            WriteWindowsStackTrace(stack_file, exception_info->ContextRecord);
        }
    } catch (...) {
        // Ignore errors in stack trace writing
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandler::SignalHandler(int signal) {
    std::string signal_name;
    switch (signal) {
        case SIGABRT: signal_name = "SIGABRT (Abort)"; break;
        case SIGFPE: signal_name = "SIGFPE (Floating Point Exception)"; break;
        case SIGILL: signal_name = "SIGILL (Illegal Instruction)"; break;
        case SIGINT: signal_name = "SIGINT (Interrupt)"; break;
        case SIGSEGV: signal_name = "SIGSEGV (Segmentation Violation)"; break;
        case SIGTERM: signal_name = "SIGTERM (Termination)"; break;
        default: signal_name = "Unknown Signal"; break;
    }

    WriteCrashLog("SIGNAL", signal_name);

    // Restore default handler and re-raise
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

void CrashHandler::PureCallHandler() {
    WriteCrashLog("PURE_CALL", "Pure virtual function call detected");
    std::abort();
}

void CrashHandler::InvalidParameterHandler(const wchar_t* expression,
                                          const wchar_t* function,
                                          const wchar_t* file,
                                          unsigned int line,
                                          uintptr_t reserved) {
    std::ostringstream additional_info;
    if (expression) {
        std::wstring ws(expression);
        std::string expr_str;
        expr_str.reserve(ws.size());
        for (wchar_t wc : ws) {
            if (wc < 128) expr_str.push_back(static_cast<char>(wc));
        }
        additional_info << "Expression: " << expr_str << "\n";
    }
    if (function) {
        std::wstring ws(function);
        std::string func_str;
        func_str.reserve(ws.size());
        for (wchar_t wc : ws) {
            if (wc < 128) func_str.push_back(static_cast<char>(wc));
        }
        additional_info << "Function: " << func_str << "\n";
    }
    if (file) {
        std::wstring ws(file);
        std::string file_str;
        file_str.reserve(ws.size());
        for (wchar_t wc : ws) {
            if (wc < 128) file_str.push_back(static_cast<char>(wc));
        }
        additional_info << "File: " << file_str << "\n";
    }
    additional_info << "Line: " << line << "\n";

    WriteCrashLog("INVALID_PARAMETER", "Invalid parameter detected", additional_info.str());
}

void CrashHandler::TerminateHandler() {
    WriteCrashLog("TERMINATE", "std::terminate called");
    std::abort();
}

// UnexpectedHandler removed as std::set_unexpected is deprecated

std::string CrashHandler::GetExceptionString(DWORD exception_code) {
    switch (exception_code) {
        case EXCEPTION_ACCESS_VIOLATION: return "Access Violation";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array Bounds Exceeded";
        case EXCEPTION_BREAKPOINT: return "Breakpoint";
        case EXCEPTION_DATATYPE_MISALIGNMENT: return "Datatype Misalignment";
        case EXCEPTION_FLT_DENORMAL_OPERAND: return "Float Denormal Operand";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "Float Divide by Zero";
        case EXCEPTION_FLT_INEXACT_RESULT: return "Float Inexact Result";
        case EXCEPTION_FLT_INVALID_OPERATION: return "Float Invalid Operation";
        case EXCEPTION_FLT_OVERFLOW: return "Float Overflow";
        case EXCEPTION_FLT_STACK_CHECK: return "Float Stack Check";
        case EXCEPTION_FLT_UNDERFLOW: return "Float Underflow";
        case EXCEPTION_ILLEGAL_INSTRUCTION: return "Illegal Instruction";
        case EXCEPTION_IN_PAGE_ERROR: return "In Page Error";
        case EXCEPTION_INT_DIVIDE_BY_ZERO: return "Integer Divide by Zero";
        case EXCEPTION_INT_OVERFLOW: return "Integer Overflow";
        case EXCEPTION_INVALID_DISPOSITION: return "Invalid Disposition";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable Exception";
        case EXCEPTION_PRIV_INSTRUCTION: return "Privileged Instruction";
        case EXCEPTION_SINGLE_STEP: return "Single Step";
        case EXCEPTION_STACK_OVERFLOW: return "Stack Overflow";
        default: return "Unknown Exception";
    }
}

void CrashHandler::WriteWindowsStackTrace(std::ofstream& log_file, CONTEXT* context) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    if (!SymInitialize(process, nullptr, TRUE)) {
        log_file << "Failed to initialize symbol handler\n";
        return;
    }

    CONTEXT local_context;
    if (!context) {
        RtlCaptureContext(&local_context);
        context = &local_context;
    }

    STACKFRAME64 stack_frame = {};
    stack_frame.AddrPC.Offset = context->Rip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = context->Rbp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = context->Rsp;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    int frame_count = 0;
    while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &stack_frame,
                       context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {

        if (frame_count++ > 50) break; // Limit stack depth

        DWORD64 address = stack_frame.AddrPC.Offset;

        char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbol_buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement = 0;
        if (SymFromAddr(process, address, &displacement, symbol)) {
            log_file << "  " << frame_count << ": " << symbol->Name
                     << " + 0x" << std::hex << displacement
                     << " (0x" << std::hex << address << ")\n";
        } else {
            log_file << "  " << frame_count << ": <unknown> (0x"
                     << std::hex << address << ")\n";
        }

        // Try to get line information
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD line_displacement = 0;
        if (SymGetLineFromAddr64(process, address, &line_displacement, &line)) {
            log_file << "      " << line.FileName << ":" << line.LineNumber << "\n";
        }
    }

    SymCleanup(process);
}
#endif

bool CrashHandler::TrackFunctionCall(const std::string& function_name, int max_depth) {
    if (!s_initialized) {
        return false;
    }

    std::lock_guard<std::mutex> lock(s_call_tracking_mutex);

    // Increment call count for this function
    s_function_call_counts[function_name]++;

    // Check if we've exceeded the maximum depth
    if (s_function_call_counts[function_name] > max_depth) {
        // Log the recursion detection
        std::ostringstream additional_info;
        additional_info << "Function: " << function_name << "\n";
        additional_info << "Call depth: " << s_function_call_counts[function_name] << "\n";
        additional_info << "Max allowed depth: " << max_depth << "\n";
        additional_info << "Current call stack:\n";

        // Add current function call counts to the log
        for (const auto& [func, count] : s_function_call_counts) {
            if (count > 0) {
                additional_info << "  " << func << ": " << count << " calls\n";
            }
        }

        WriteCrashLog("RECURSION_DETECTED", "Stack overflow prevention triggered", additional_info.str());
        return true;
    }

    return false;
}

void CrashHandler::UntrackFunctionCall(const std::string& function_name) {
    if (!s_initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(s_call_tracking_mutex);

    // Decrement call count for this function
    auto it = s_function_call_counts.find(function_name);
    if (it != s_function_call_counts.end()) {
        it->second--;
        if (it->second <= 0) {
            s_function_call_counts.erase(it);
        }
    }
}

void CrashHandler::LogCallStack() {
    if (!s_initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(s_call_tracking_mutex);

    std::ostringstream call_stack;
    call_stack << "Current function call stack:\n";

    for (const auto& [function_name, count] : s_function_call_counts) {
        if (count > 0) {
            call_stack << "  " << function_name << ": " << count << " calls";
            if (count > 10) {
                call_stack << " (POTENTIAL RECURSION)";
            }
            call_stack << "\n";
        }
    }

    WriteCrashLog("CALL_STACK", "Function call tracking", call_stack.str());
}

} // namespace Lupine
