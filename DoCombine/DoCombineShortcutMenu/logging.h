#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ranges.h>

spdlog::logger& get_default_logger();

#ifndef LOGGER
//! Returns a reference to the default logger
#define LOGGER get_default_logger()
#endif

#ifndef ENTERED
#define ENTERED(...)                                                                                                   \
    {                                                                                                                  \
        std::tuple args = std::make_tuple(__VA_ARGS__);                                                                \
        if (std::tuple_size<decltype(args)> {}())                                                                      \
            LOGGER.debug("Entered " __FUNCTION__ ": {}", spdlog::fmt_lib::join(args, ", "));                           \
        else                                                                                                           \
            LOGGER.debug("Entered " __FUNCTION__);                                                                     \
    }

#endif

#ifndef FINISHED
#define FINISHED(...)                                                                                                  \
    {                                                                                                                  \
        std::tuple args = std::make_tuple(__VA_ARGS__);                                                                \
        if (std::tuple_size<decltype(args)> {}())                                                                      \
            LOGGER.debug("Finished " __FUNCTION__ ": {}", spdlog::fmt_lib::join(args, ", "));                          \
        else                                                                                                           \
            LOGGER.debug("Finished " __FUNCTION__);                                                                    \
    }
#endif