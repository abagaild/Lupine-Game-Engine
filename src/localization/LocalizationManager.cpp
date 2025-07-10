#include "lupine/localization/LocalizationManager.h"
#include "lupine/serialization/JsonUtils.h"
#include <iostream>
#include <algorithm>

namespace Lupine {

// Locale implementation
Locale::Locale(const std::string& lang, const std::string& region, const std::string& display)
    : language_code(lang), region_code(region), display_name(display) {
    if (display_name.empty()) {
        display_name = GetIdentifier();
    }
}

std::string Locale::GetIdentifier() const {
    if (region_code.empty()) {
        return language_code;
    }
    return language_code + "_" + region_code;
}

bool Locale::operator==(const Locale& other) const {
    return language_code == other.language_code && region_code == other.region_code;
}

bool Locale::operator!=(const Locale& other) const {
    return !(*this == other);
}

// LocalizationTable implementation
LocalizationTable::LocalizationTable(const Locale& locale) : m_locale(locale) {}

void LocalizationTable::SetString(const std::string& key, const std::string& value) {
    m_strings[key] = value;
}

std::string LocalizationTable::GetString(const std::string& key) const {
    auto it = m_strings.find(key);
    if (it != m_strings.end()) {
        return it->second;
    }
    return key; // Return key as fallback
}

bool LocalizationTable::HasKey(const std::string& key) const {
    return m_strings.find(key) != m_strings.end();
}

void LocalizationTable::RemoveKey(const std::string& key) {
    m_strings.erase(key);
}

std::vector<std::string> LocalizationTable::GetAllKeys() const {
    std::vector<std::string> keys;
    keys.reserve(m_strings.size());
    for (const auto& pair : m_strings) {
        keys.push_back(pair.first);
    }
    return keys;
}

void LocalizationTable::Clear() {
    m_strings.clear();
}

// LocalizationManager implementation
LocalizationManager& LocalizationManager::Instance() {
    static LocalizationManager instance;
    return instance;
}

void LocalizationManager::Initialize() {
    if (m_initialized) {
        return;
    }
    
    // Set default locale to English (US)
    m_default_locale = Locale("en", "US", "English (United States)");
    m_current_locale = m_default_locale;
    
    // Add default supported locales
    AddSupportedLocale(m_default_locale);
    
    // Create default table
    CreateTable(m_default_locale);
    
    m_initialized = true;
    std::cout << "LocalizationManager initialized with default locale: " << m_default_locale.GetIdentifier() << std::endl;
}

void LocalizationManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    Clear();
    m_initialized = false;
    std::cout << "LocalizationManager shutdown" << std::endl;
}

bool LocalizationManager::SetCurrentLocale(const Locale& locale) {
    if (!IsLocaleSupported(locale)) {
        std::cerr << "Locale not supported: " << locale.GetIdentifier() << std::endl;
        return false;
    }
    
    Locale old_locale = m_current_locale;
    m_current_locale = locale;
    
    // Ensure table exists for current locale
    if (!GetTable(locale)) {
        CreateTable(locale);
    }
    
    NotifyLocaleChange(old_locale, m_current_locale);
    std::cout << "Locale changed from " << old_locale.GetIdentifier() << " to " << m_current_locale.GetIdentifier() << std::endl;
    return true;
}

void LocalizationManager::SetDefaultLocale(const Locale& locale) {
    m_default_locale = locale;
    if (!IsLocaleSupported(locale)) {
        AddSupportedLocale(locale);
    }
    if (!GetTable(locale)) {
        CreateTable(locale);
    }
}

void LocalizationManager::AddSupportedLocale(const Locale& locale) {
    if (!IsLocaleSupported(locale)) {
        m_supported_locales.push_back(locale);
        std::cout << "Added supported locale: " << locale.GetIdentifier() << std::endl;
    }
}

void LocalizationManager::RemoveSupportedLocale(const Locale& locale) {
    auto it = std::find(m_supported_locales.begin(), m_supported_locales.end(), locale);
    if (it != m_supported_locales.end()) {
        m_supported_locales.erase(it);
        RemoveTable(locale);
        std::cout << "Removed supported locale: " << locale.GetIdentifier() << std::endl;
    }
}

bool LocalizationManager::IsLocaleSupported(const Locale& locale) const {
    return std::find(m_supported_locales.begin(), m_supported_locales.end(), locale) != m_supported_locales.end();
}

LocalizationTable* LocalizationManager::GetTable(const Locale& locale) {
    std::string identifier = locale.GetIdentifier();
    auto it = m_tables.find(identifier);
    if (it != m_tables.end()) {
        return it->second.get();
    }
    return nullptr;
}

LocalizationTable* LocalizationManager::GetCurrentTable() {
    return GetTable(m_current_locale);
}

LocalizationTable* LocalizationManager::CreateTable(const Locale& locale) {
    std::string identifier = locale.GetIdentifier();
    auto table = std::make_unique<LocalizationTable>(locale);
    LocalizationTable* table_ptr = table.get();
    m_tables[identifier] = std::move(table);
    std::cout << "Created localization table for: " << identifier << std::endl;
    return table_ptr;
}

void LocalizationManager::RemoveTable(const Locale& locale) {
    std::string identifier = locale.GetIdentifier();
    auto it = m_tables.find(identifier);
    if (it != m_tables.end()) {
        m_tables.erase(it);
        std::cout << "Removed localization table for: " << identifier << std::endl;
    }
}

std::string LocalizationManager::GetLocalizedString(const std::string& key) const {
    return GetLocalizedString(key, key);
}

std::string LocalizationManager::GetLocalizedString(const std::string& key, const std::string& fallback) const {
    // Try current locale first
    auto current_table = const_cast<LocalizationManager*>(this)->GetCurrentTable();
    if (current_table && current_table->HasKey(key)) {
        return current_table->GetString(key);
    }
    
    // Try default locale if different from current
    if (m_current_locale != m_default_locale) {
        auto default_table = const_cast<LocalizationManager*>(this)->GetTable(m_default_locale);
        if (default_table && default_table->HasKey(key)) {
            return default_table->GetString(key);
        }
    }
    
    // Return fallback
    return fallback;
}

bool LocalizationManager::HasLocalizationKey(const std::string& key) const {
    auto current_table = const_cast<LocalizationManager*>(this)->GetCurrentTable();
    if (current_table && current_table->HasKey(key)) {
        return true;
    }
    
    if (m_current_locale != m_default_locale) {
        auto default_table = const_cast<LocalizationManager*>(this)->GetTable(m_default_locale);
        if (default_table && default_table->HasKey(key)) {
            return true;
        }
    }
    
    return false;
}

void LocalizationManager::AddKeyToAllLocales(const std::string& key, const std::string& default_value) {
    for (const auto& locale : m_supported_locales) {
        auto table = GetTable(locale);
        if (!table) {
            table = CreateTable(locale);
        }
        if (!table->HasKey(key)) {
            table->SetString(key, default_value.empty() ? key : default_value);
        }
    }
}

void LocalizationManager::RemoveKeyFromAllLocales(const std::string& key) {
    for (const auto& locale : m_supported_locales) {
        auto table = GetTable(locale);
        if (table) {
            table->RemoveKey(key);
        }
    }
}

std::vector<std::string> LocalizationManager::GetAllKeys() const {
    std::vector<std::string> all_keys;
    for (const auto& [identifier, table] : m_tables) {
        auto keys = table->GetAllKeys();
        for (const auto& key : keys) {
            if (std::find(all_keys.begin(), all_keys.end(), key) == all_keys.end()) {
                all_keys.push_back(key);
            }
        }
    }
    return all_keys;
}

void LocalizationManager::RegisterLocaleChangeCallback(const LocaleChangeCallback& callback) {
    m_locale_change_callbacks.push_back(callback);
}

void LocalizationManager::ClearLocaleChangeCallbacks() {
    m_locale_change_callbacks.clear();
}

void LocalizationManager::NotifyLocaleChange(const Locale& old_locale, const Locale& new_locale) {
    for (const auto& callback : m_locale_change_callbacks) {
        callback(old_locale, new_locale);
    }
}

bool LocalizationManager::LoadFromFile(const std::string& file_path) {
    try {
        JsonNode root = JsonUtils::LoadFromFile(file_path);

        if (!root.HasKey("localization")) {
            std::cerr << "Invalid localization file format" << std::endl;
            return false;
        }

        JsonNode locData = root["localization"];

        // Load default locale
        if (locData.HasKey("default_locale")) {
            JsonNode defaultLocaleData = locData["default_locale"];
            Locale defaultLocale;
            defaultLocale.language_code = defaultLocaleData["language_code"].AsString();
            defaultLocale.region_code = defaultLocaleData["region_code"].AsString();
            defaultLocale.display_name = defaultLocaleData["display_name"].AsString();
            SetDefaultLocale(defaultLocale);
        }

        // Load current locale
        if (locData.HasKey("current_locale")) {
            JsonNode currentLocaleData = locData["current_locale"];
            Locale currentLocale;
            currentLocale.language_code = currentLocaleData["language_code"].AsString();
            currentLocale.region_code = currentLocaleData["region_code"].AsString();
            currentLocale.display_name = currentLocaleData["display_name"].AsString();
            SetCurrentLocale(currentLocale);
        }

        // Load supported locales
        if (locData.HasKey("supported_locales")) {
            JsonNode supportedLocalesArray = locData["supported_locales"];
            if (supportedLocalesArray.IsArray()) {
                for (const auto& localeData : supportedLocalesArray.AsArray()) {
                    Locale locale;
                    locale.language_code = localeData["language_code"].AsString();
                    locale.region_code = localeData["region_code"].AsString();
                    locale.display_name = localeData["display_name"].AsString();
                    AddSupportedLocale(locale);
                }
            }
        }

        // Load localization tables
        if (locData.HasKey("tables")) {
            JsonNode tablesData = locData["tables"];
            for (const auto& [identifier, tableData] : tablesData.AsObject()) {
                // Parse locale from identifier
                std::string lang_code, region_code;
                size_t underscore_pos = identifier.find('_');
                if (underscore_pos != std::string::npos) {
                    lang_code = identifier.substr(0, underscore_pos);
                    region_code = identifier.substr(underscore_pos + 1);
                } else {
                    lang_code = identifier;
                }

                Locale locale(lang_code, region_code);
                auto table = CreateTable(locale);

                if (tableData.HasKey("strings")) {
                    JsonNode stringsData = tableData["strings"];
                    for (const auto& [key, value] : stringsData.AsObject()) {
                        table->SetString(key, value.AsString());
                    }
                }
            }
        }

        std::cout << "Loaded localization data from: " << file_path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading localization file: " << e.what() << std::endl;
        return false;
    }
}

bool LocalizationManager::SaveToFile(const std::string& file_path) const {
    try {
        JsonNode root;
        JsonNode locData;

        // Save default locale
        JsonNode defaultLocaleData;
        defaultLocaleData["language_code"] = m_default_locale.language_code;
        defaultLocaleData["region_code"] = m_default_locale.region_code;
        defaultLocaleData["display_name"] = m_default_locale.display_name;
        locData["default_locale"] = defaultLocaleData;

        // Save current locale
        JsonNode currentLocaleData;
        currentLocaleData["language_code"] = m_current_locale.language_code;
        currentLocaleData["region_code"] = m_current_locale.region_code;
        currentLocaleData["display_name"] = m_current_locale.display_name;
        locData["current_locale"] = currentLocaleData;

        // Save supported locales
        JsonNode supportedLocalesArray;
        supportedLocalesArray.value = std::vector<JsonNode>();
        for (const auto& locale : m_supported_locales) {
            JsonNode localeData;
            localeData["language_code"] = locale.language_code;
            localeData["region_code"] = locale.region_code;
            localeData["display_name"] = locale.display_name;
            supportedLocalesArray.Push(localeData);
        }
        locData["supported_locales"] = supportedLocalesArray;

        // Save localization tables
        JsonNode tablesData;
        for (const auto& [identifier, table] : m_tables) {
            JsonNode tableData;
            JsonNode stringsData;

            for (const auto& [key, value] : table->GetAllStrings()) {
                stringsData[key] = value;
            }

            tableData["strings"] = stringsData;
            tablesData[identifier] = tableData;
        }
        locData["tables"] = tablesData;

        root["localization"] = locData;

        bool success = JsonUtils::SaveToFile(root, file_path, true);
        if (success) {
            std::cout << "Saved localization data to: " << file_path << std::endl;
        }
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Error saving localization file: " << e.what() << std::endl;
        return false;
    }
}

void LocalizationManager::Clear() {
    m_tables.clear();
    m_supported_locales.clear();
    m_locale_change_callbacks.clear();
    m_current_locale = Locale();
    m_default_locale = Locale();
}

// Convenience functions
std::string GetLocalizedString(const std::string& key) {
    return LocalizationManager::Instance().GetLocalizedString(key);
}

std::string GetLocalizedString(const std::string& key, const std::string& fallback) {
    return LocalizationManager::Instance().GetLocalizedString(key, fallback);
}

} // namespace Lupine
