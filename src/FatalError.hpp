#pragma once
#ifndef FATAL_ERROR_HPP
#define FATAL_ERROR_HPP

#include <spdlog/spdlog.h>

#define FATAL_ERROR(...)    (SPDLOG_CRITICAL(__VA_ARGS__), std::exit(1));

#endif //FATAL_ERROR_HPP
