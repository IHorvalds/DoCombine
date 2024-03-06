#include "logging.h"
#include "pch.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/ranges.h>
#include <filesystem>
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
        auto sLogsDir = Paths::get_module_folderpatha(g_hInst_doCombineExt) + "\\..\\logs";
        if (!std::filesystem::exists(sLogsDir) && !std::filesystem::create_directory(sLogsDir))
        {
            spdlog::error("Failed to create logs directory");
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
