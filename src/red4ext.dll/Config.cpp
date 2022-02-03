#include "stdafx.hpp"
#include "Config.hpp"
#include "Utils.hpp"

#define DEFAULT_TOML_EXCEPTION_MSG L"An exception occured while parsing the config file:\n\n{}\n\nFile: {}"

Config::Config(const Paths& aPaths)
    : m_version(0)
    , m_dev()
    , m_logging()
    , m_plugins()
{
    const auto file = aPaths.GetConfigFile();

    std::error_code err;
    if (std::filesystem::exists(file, err))
    {
        Load(file);
    }
    else if (err)
    {
        auto errVal = err.value();
        const auto& category = err.category();
        auto msg = category.message(errVal);

        SHOW_MESSAGE_BOX_AND_EXIT_FILE_LINE(L"An error occured while checking config file existence:\n{}\n\nFile: {}",
                                            Utils::Widen(msg));
    }
    else
    {
        Save(file);
    }
}

const size_t Config::GetVersion() const
{
    return m_version;
}

const Config::DevConfig& Config::GetDev() const
{
    return m_dev;
}

const Config::LoggingConfig& Config::GetLogging() const
{
    return m_logging;
}

const Config::PluginsConfig& Config::GetPlugins() const
{
    return m_plugins;
}

void Config::Load(const std::filesystem::path& aFile)
{
    try
    {
        auto config = toml::parse(aFile);
        if (config.contains("version"))
        {
            auto version = toml::find<size_t>(config, "version");
            switch (version)
            {
            case 1:
            {
                LoadV1(config);
                break;
            }
            default:
            {
                SHOW_MESSAGE_BOX_AND_EXIT_FILE_LINE("The config file does not have a valid version.\n\nFile: {}",
                                                    aFile);
                break;
            }
            }
        }
        else
        {
            SHOW_MESSAGE_BOX_AND_EXIT_FILE_LINE("The config file does not have a version.\n\nFile: {}", aFile);
        }
    }
    catch (const std::exception& e)
    {
        SHOW_MESSAGE_BOX_AND_EXIT_FILE_LINE(DEFAULT_TOML_EXCEPTION_MSG, Utils::Widen(e.what()), aFile);
    }
}

void Config::Save(const std::filesystem::path& aFile)
{
    try
    {
        using ordered_value = toml::basic_value<toml::preserve_comments, tsl::ordered_map>;

        auto logLevel = spdlog::level::to_string_view(m_logging.level).data();
        auto flushOn = spdlog::level::to_string_view(m_logging.flushOn).data();

        ordered_value config{{"version", LatestVersion},
                             {"logging", ordered_value{{"level", logLevel}, {"flush_on", flushOn}}}};

        config.comments().push_back(
            " See https://wiki.redmodding.org/red4ext/getting-started/configuration for more options.");

        std::ofstream file(aFile, std::ios::out);
        file.exceptions(std::ostream::badbit | std::ostream::failbit);

        file << config;
    }
    catch (const std::exception& e)
    {
        SHOW_MESSAGE_BOX_FILE_LINE(MB_ICONWARNING | MB_OK,
                                   "An exception occured while saving the config file:\n\n{}\n\nFile: {}",
                                   Utils::Widen(e.what()), aFile);
    }
}

void Config::LoadV1(const toml::value& aConfig)
{
    m_version = 1;

    m_dev.LoadV1(aConfig);
    m_logging.LoadV1(aConfig);
    m_plugins.LoadV1(aConfig);
}

void Config::DevConfig::LoadV1(const toml::value& aConfig)
{
    hasConsole = toml::find_or(aConfig, "dev", "console", hasConsole);
    waitForDebugger = toml::find_or(aConfig, "dev", "wait_for_debugger", waitForDebugger);
}

void Config::LoggingConfig::LoadV1(const toml::value& aConfig)
{
    auto levelName = toml::find_or(aConfig, "logging", "level", "");
    if (!levelName.empty())
    {
        auto requestedLevel = spdlog::level::from_str(levelName);

        // If the level is set to off, but the requested level is not "off" then the user might mistyped the levels.
        // spdlog return "level::off" if there is no match.
        if (requestedLevel == spdlog::level::off && levelName != "off")
        {
            requestedLevel = level;
        }

        level = requestedLevel;
    }

    levelName = toml::find_or(aConfig, "logging", "flush_on", "");
    if (!levelName.empty())
    {
        auto requestedLevel = spdlog::level::from_str(levelName.data());

        // Do not allow flushing to be off.
        if (requestedLevel == spdlog::level::off)
        {
            requestedLevel = flushOn;
        }

        flushOn = requestedLevel;
    }

    maxFiles = toml::find_or(aConfig, "logging", "max_files", maxFiles);
    if (maxFiles < 1)
    {
        maxFiles = 5;
    }

    maxFileSize = toml::find_or(aConfig, "logging", "max_file_size", maxFileSize);
    if (maxFileSize < 1)
    {
        maxFileSize = 10;
    }
}

void Config::PluginsConfig::LoadV1(const toml::value& aConfig)
{
    isEnabled = toml::find_or(aConfig, "plugins", "enabled", isEnabled);

    std::vector<std::string> blacklistedPlugins;
    blacklistedPlugins = toml::find_or(aConfig, "plugins", "blacklist", blacklistedPlugins);

    for (const auto& plugin : blacklistedPlugins)
    {
        blacklist.emplace(Utils::Widen(plugin));
    }
}
