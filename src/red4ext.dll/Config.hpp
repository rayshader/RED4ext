#pragma once

#include "Paths.hpp"

class Config
{
public:
    Config(const Paths& aPaths);
    ~Config() = default;

    const bool HasDevConsole() const;

    const spdlog::level::level_enum GetLogLevel() const;
    const spdlog::level::level_enum GetFlushLevel() const;

private:
    void Load(const std::filesystem::path& aFile);
    void Save(const std::filesystem::path& aFile);

    bool m_devConsole;
    spdlog::level::level_enum m_logLevel;
    spdlog::level::level_enum m_flushOn;
};