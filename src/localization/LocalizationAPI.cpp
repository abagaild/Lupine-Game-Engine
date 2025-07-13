#include "lupine/localization/LocalizationAPI.h"
#include "lupine/localization/LocalizationManager.h"
#include <algorithm>

namespace Lupine {
namespace LocalizationAPI {

// Helper function to convert locale identifier to Locale object
static Locale ParseLocaleIdentifier(const std::string& identifier) {
    size_t underscore_pos = identifier.find('_');
    if (underscore_pos != std::string::npos) {
        std::string lang = identifier.substr(0, underscore_pos);
        std::string region = identifier.substr(underscore_pos + 1);
        return Locale(lang, region);
    } else {
        return Locale(identifier);
    }
}

// Helper function to find locale by identifier
static Locale FindLocaleByIdentifier(const std::string& identifier) {
    auto& locManager = LocalizationManager::Instance();
    const auto& supported = locManager.GetSupportedLocales();
    
    for (const auto& locale : supported) {
        if (locale.GetIdentifier() == identifier) {
            return locale;
        }
    }
    
    // If not found, try to parse it
    return ParseLocaleIdentifier(identifier);
}

std::string GetString(const std::string& key) {
    return LocalizationManager::Instance().GetLocalizedString(key);
}

std::string GetString(const std::string& key, const std::string& fallback) {
    return LocalizationManager::Instance().GetLocalizedString(key, fallback);
}

bool SetLocale(const std::string& locale_identifier) {
    Locale locale = FindLocaleByIdentifier(locale_identifier);
    return LocalizationManager::Instance().SetCurrentLocale(locale);
}

std::string GetCurrentLocale() {
    return LocalizationManager::Instance().GetCurrentLocale().GetIdentifier();
}

std::string GetDefaultLocale() {
    return LocalizationManager::Instance().GetDefaultLocale().GetIdentifier();
}

std::vector<std::string> GetSupportedLocales() {
    auto& locManager = LocalizationManager::Instance();
    const auto& locales = locManager.GetSupportedLocales();
    
    std::vector<std::string> identifiers;
    identifiers.reserve(locales.size());
    
    for (const auto& locale : locales) {
        identifiers.push_back(locale.GetIdentifier());
    }
    
    return identifiers;
}

bool IsLocaleSupported(const std::string& locale_identifier) {
    Locale locale = ParseLocaleIdentifier(locale_identifier);
    return LocalizationManager::Instance().IsLocaleSupported(locale);
}

bool HasKey(const std::string& key) {
    return LocalizationManager::Instance().HasLocalizationKey(key);
}

std::vector<std::string> GetAllKeys() {
    return LocalizationManager::Instance().GetAllKeys();
}

bool AddKey(const std::string& key, const std::string& default_value) {
    try {
        LocalizationManager::Instance().AddKeyToAllLocales(key, default_value);
        return true;
    } catch (...) {
        return false;
    }
}

bool RemoveKey(const std::string& key) {
    try {
        LocalizationManager::Instance().RemoveKeyFromAllLocales(key);
        return true;
    } catch (...) {
        return false;
    }
}

bool SetString(const std::string& key, const std::string& locale_identifier, const std::string& value) {
    try {
        auto& locManager = LocalizationManager::Instance();
        Locale locale = FindLocaleByIdentifier(locale_identifier);
        
        auto table = locManager.GetTable(locale);
        if (!table) {
            table = locManager.CreateTable(locale);
        }
        
        if (table) {
            table->SetString(key, value);
            return true;
        }
        
        return false;
    } catch (...) {
        return false;
    }
}

std::string GetStringForLocale(const std::string& key, const std::string& locale_identifier) {
    try {
        auto& locManager = LocalizationManager::Instance();
        Locale locale = FindLocaleByIdentifier(locale_identifier);
        
        auto table = locManager.GetTable(locale);
        if (table && table->HasKey(key)) {
            return table->GetString(key);
        }
        
        return "";
    } catch (...) {
        return "";
    }
}

bool LoadFromFile(const std::string& file_path) {
    return LocalizationManager::Instance().LoadFromFile(file_path);
}

bool SaveToFile(const std::string& file_path) {
    return LocalizationManager::Instance().SaveToFile(file_path);
}

// Static storage for C-style callbacks
static std::vector<void(*)(const std::string&, const std::string&)> s_callbacks;

// Internal callback adapter
static void InternalLocaleChangeCallback(const Locale& old_locale, const Locale& new_locale) {
    std::string old_id = old_locale.GetIdentifier();
    std::string new_id = new_locale.GetIdentifier();
    
    for (auto callback : s_callbacks) {
        if (callback) {
            callback(old_id, new_id);
        }
    }
}

void RegisterLocaleChangeCallback(void(*callback)(const std::string& old_locale, const std::string& new_locale)) {
    if (callback) {
        // Register our internal adapter if this is the first callback
        if (s_callbacks.empty()) {
            LocalizationManager::Instance().RegisterLocaleChangeCallback(InternalLocaleChangeCallback);
        }
        
        s_callbacks.push_back(callback);
    }
}

void ClearLocaleChangeCallbacks() {
    s_callbacks.clear();
    // Note: We don't clear the LocalizationManager callback because other systems might be using it
}

} // namespace LocalizationAPI
} // namespace Lupine
