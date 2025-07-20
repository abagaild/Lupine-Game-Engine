#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

namespace Lupine {

/**
 * @brief Represents a locale with language and region information
 */
struct Locale {
    std::string language_code;  // e.g., "en", "es", "fr"
    std::string region_code;    // e.g., "US", "ES", "FR" (optional)
    std::string display_name;   // e.g., "English (United States)"
    
    Locale() = default;
    Locale(const std::string& lang, const std::string& region = "", const std::string& display = "");
    
    /**
     * @brief Get the full locale identifier (e.g., "en_US", "es_ES")
     */
    std::string GetIdentifier() const;
    
    /**
     * @brief Check if this locale matches another
     */
    bool operator==(const Locale& other) const;
    bool operator!=(const Locale& other) const;
};

/**
 * @brief Represents a localization table for a specific locale
 */
class LocalizationTable {
public:
    LocalizationTable() = default;
    explicit LocalizationTable(const Locale& locale);
    
    /**
     * @brief Get the locale this table represents
     */
    const Locale& GetLocale() const { return m_locale; }
    
    /**
     * @brief Set a localized string for a key
     */
    void SetString(const std::string& key, const std::string& value);
    
    /**
     * @brief Get a localized string for a key
     */
    std::string GetString(const std::string& key) const;
    
    /**
     * @brief Check if a key exists in this table
     */
    bool HasKey(const std::string& key) const;
    
    /**
     * @brief Remove a key from this table
     */
    void RemoveKey(const std::string& key);
    
    /**
     * @brief Get all keys in this table
     */
    std::vector<std::string> GetAllKeys() const;
    
    /**
     * @brief Get all key-value pairs
     */
    const std::unordered_map<std::string, std::string>& GetAllStrings() const { return m_strings; }
    
    /**
     * @brief Clear all strings
     */
    void Clear();
    
    /**
     * @brief Get the number of strings in this table
     */
    size_t GetStringCount() const { return m_strings.size(); }

private:
    Locale m_locale;
    std::unordered_map<std::string, std::string> m_strings;
};

/**
 * @brief Callback function type for locale change notifications
 */
using LocaleChangeCallback = std::function<void(const Locale& old_locale, const Locale& new_locale)>;

/**
 * @brief Main localization manager singleton
 * 
 * This class manages all localization data, handles locale switching,
 * and provides the main API for retrieving localized strings.
 */
class LocalizationManager {
public:
    /**
     * @brief Get the singleton instance
     */
    static LocalizationManager& Instance();
    
    /**
     * @brief Initialize the localization system
     */
    void Initialize();
    
    /**
     * @brief Shutdown the localization system
     */
    void Shutdown();
    
    /**
     * @brief Set the current locale
     */
    bool SetCurrentLocale(const Locale& locale);
    
    /**
     * @brief Get the current locale
     */
    const Locale& GetCurrentLocale() const { return m_current_locale; }
    
    /**
     * @brief Set the default locale (fallback)
     */
    void SetDefaultLocale(const Locale& locale);
    
    /**
     * @brief Get the default locale
     */
    const Locale& GetDefaultLocale() const { return m_default_locale; }
    
    /**
     * @brief Add a supported locale
     */
    void AddSupportedLocale(const Locale& locale);
    
    /**
     * @brief Remove a supported locale
     */
    void RemoveSupportedLocale(const Locale& locale);
    
    /**
     * @brief Get all supported locales
     */
    const std::vector<Locale>& GetSupportedLocales() const { return m_supported_locales; }
    
    /**
     * @brief Check if a locale is supported
     */
    bool IsLocaleSupported(const Locale& locale) const;
    
    /**
     * @brief Get a localization table for a specific locale
     */
    LocalizationTable* GetTable(const Locale& locale);
    
    /**
     * @brief Get the current localization table
     */
    LocalizationTable* GetCurrentTable();
    
    /**
     * @brief Create a new localization table for a locale
     */
    LocalizationTable* CreateTable(const Locale& locale);
    
    /**
     * @brief Remove a localization table
     */
    void RemoveTable(const Locale& locale);
    
    /**
     * @brief Get a localized string by key
     * Falls back to default locale if not found in current locale
     */
    std::string GetLocalizedString(const std::string& key) const;
    
    /**
     * @brief Get a localized string by key with fallback
     */
    std::string GetLocalizedString(const std::string& key, const std::string& fallback) const;
    
    /**
     * @brief Check if a localization key exists
     */
    bool HasLocalizationKey(const std::string& key) const;
    
    /**
     * @brief Add a key to all supported locales with default value
     */
    void AddKeyToAllLocales(const std::string& key, const std::string& default_value = "");
    
    /**
     * @brief Remove a key from all locales
     */
    void RemoveKeyFromAllLocales(const std::string& key);
    
    /**
     * @brief Get all localization keys across all locales
     */
    std::vector<std::string> GetAllKeys() const;
    
    /**
     * @brief Register a callback for locale changes
     */
    void RegisterLocaleChangeCallback(const LocaleChangeCallback& callback);
    
    /**
     * @brief Unregister all locale change callbacks
     */
    void ClearLocaleChangeCallbacks();
    
    /**
     * @brief Load localization data from file
     */
    bool LoadFromFile(const std::string& file_path);
    
    /**
     * @brief Save localization data to file
     */
    bool SaveToFile(const std::string& file_path) const;
    
    /**
     * @brief Clear all localization data
     */
    void Clear();

private:
    LocalizationManager() = default;
    ~LocalizationManager() = default;
    
    // Disable copy and move
    LocalizationManager(const LocalizationManager&) = delete;
    LocalizationManager& operator=(const LocalizationManager&) = delete;
    LocalizationManager(LocalizationManager&&) = delete;
    LocalizationManager& operator=(LocalizationManager&&) = delete;
    
    /**
     * @brief Notify all callbacks of locale change
     */
    void NotifyLocaleChange(const Locale& old_locale, const Locale& new_locale);
    
    Locale m_current_locale;
    Locale m_default_locale;
    std::vector<Locale> m_supported_locales;
    std::unordered_map<std::string, std::unique_ptr<LocalizationTable>> m_tables;
    std::vector<LocaleChangeCallback> m_locale_change_callbacks;
    bool m_initialized = false;
};

/**
 * @brief Convenience function to get localized string
 */
std::string GetLocalizedString(const std::string& key);

/**
 * @brief Convenience function to get localized string with fallback
 */
std::string GetLocalizedString(const std::string& key, const std::string& fallback);

} // namespace Lupine
