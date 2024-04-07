#include "logging.h"
#include "pch.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/win_eventlog_sink.h>
#include <spdlog/fmt/ranges.h>
#include <filesystem>
#include <ShlObj.h>
#include "paths.h"

extern HINSTANCE g_hInst_doCombineExt;

namespace
{
const std::string s_module_name = "DoCombineShellExtension";
}

spdlog::logger& get_default_logger()
{
    static std::once_flag spdlog_error_handler_flag;
    std::call_once(spdlog_error_handler_flag, []() {
        spdlog::set_error_handler([](const std::string& msg) {
            spdlog::critical("Logger had an error: {}", msg);
        });

        spdlog::flush_every(std::chrono::seconds(1));
    });

    auto pExistingLogger = spdlog::get(::s_module_name);
    if (!pExistingLogger)
    {
        static std::string    sLogsDir = "";
        static std::once_flag folder_path_flag;
        std::call_once(folder_path_flag, []() {
            auto sModulePath = Paths::get_module_folderpatha(g_hInst_doCombineExt);
            if (sModulePath.find("C:\\Program Files") != std::string::npos)
            {
                char szPath[MAX_PATH];
                SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath);
                sModulePath = std::string(szPath) + "\\TaylorLabs\\DoCombine";
            }

            sLogsDir = sModulePath + "\\logs";
        });

        std::error_code ec;
        if (!std::filesystem::exists(sLogsDir, ec) && !std::filesystem::create_directories(sLogsDir, ec))
        {
            spdlog::default_logger()->sinks().push_back(
                std::make_shared<spdlog::sinks::win_eventlog_sink_mt>(s_module_name));
            spdlog::error("Failed to create logs directory, using default logger instead. Error code: {}", ec.value());
            pExistingLogger = spdlog::default_logger();
        }
        else
        {
            const auto loggerPath = std::format("{}\\{}.log", sLogsDir, ::s_module_name);
            const auto maxSize    = 10 * 1024 * 1024; // 10M
            const auto maxFiles   = 3;                // 5

            spdlog::rotating_logger_mt(::s_module_name, loggerPath, maxSize, maxFiles);

            pExistingLogger = spdlog::get(::s_module_name);
        }
    }

    return *pExistingLogger;
}
