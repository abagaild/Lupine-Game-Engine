#pragma once

#include <string>
#include <vector>

namespace Lupine {

/**
 * @brief Localization API for scripting languages
 * 
 * This provides a simplified C-style API that can be easily exposed
 * to Lua and Python scripts for localization functionality.
 */
namespace LocalizationAPI {

    /**
     * @brief Get a localized string by key
     * @param key The localization key
     * @return Localized string, or the key itself if not found
     */
    std::string GetString(const std::string& key);

    /**
     * @brief Get a localized string by key with fallback
     * @param key The localization key
     * @param fallback Fallback text if key is not found
     * @return Localized string, or fallback if not found
     */
    std::string GetString(const std::string& key, const std::string& fallback);

    /**
     * @brief Set the current locale
     * @param locale_identifier Locale identifier (e.g., "en_US", "es_ES")
     * @return True if locale was set successfully
     */
    bool SetLocale(const std::string& locale_identifier);

    /**
     * @brief Get the current locale identifier
     * @return Current locale identifier
     */
    std::string GetCurrentLocale();

    /**
     * @brief Get the default locale identifier
     * @return Default locale identifier
     */
    std::string GetDefaultLocale();

    /**
     * @brief Get all supported locale identifiers
     * @return Vector of supported locale identifiers
     */
    std::vector<std::string> GetSupportedLocales();

    /**
     * @brief Check if a locale is supported
     * @param locale_identifier Locale identifier to check
     * @return True if locale is supported
     */
    bool IsLocaleSupported(const std::string& locale_identifier);

    /**
     * @brief Check if a localization key exists
     * @param key The localization key to check
     * @return True if key exists in current or default locale
     */
    bool HasKey(const std::string& key);

    /**
     * @brief Get all available localization keys
     * @return Vector of all localization keys
     */
    std::vector<std::string> GetAllKeys();

    /**
     * @brief Add a new localization key with default value
     * @param key The localization key
     * @param default_value Default value for the key
     * @return True if key was added successfully
     */
    bool AddKey(const std::string& key, const std::string& default_value = "");

    /**
     * @brief Remove a localization key from all locales
     * @param key The localization key to remove
     * @return True if key was removed successfully
     */
    bool RemoveKey(const std::string& key);

    /**
     * @brief Set a localized string for a specific locale
     * @param key The localization key
     * @param locale_identifier Locale identifier
     * @param value The localized string
     * @return True if value was set successfully
     */
    bool SetString(const std::string& key, const std::string& locale_identifier, const std::string& value);

    /**
     * @brief Get a localized string for a specific locale
     * @param key The localization key
     * @param locale_identifier Locale identifier
     * @return Localized string for the specified locale, or empty if not found
     */
    std::string GetStringForLocale(const std::string& key, const std::string& locale_identifier);

    /**
     * @brief Load localization data from file
     * @param file_path Path to localization file
     * @return True if data was loaded successfully
     */
    bool LoadFromFile(const std::string& file_path);

    /**
     * @brief Save localization data to file
     * @param file_path Path to save localization file
     * @return True if data was saved successfully
     */
    bool SaveToFile(const std::string& file_path);

    /**
     * @brief Register a callback for locale changes (C++ only)
     * @param callback Function to call when locale changes
     * @note This is primarily for C++ use; scripts should use events
     */
    void RegisterLocaleChangeCallback(void(*callback)(const std::string& old_locale, const std::string& new_locale));

    /**
     * @brief Clear all locale change callbacks
     */
    void ClearLocaleChangeCallbacks();

} // namespace LocalizationAPI

} // namespace Lupine
