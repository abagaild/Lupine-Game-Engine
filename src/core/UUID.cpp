#include "lupine/core/UUID.h"
#include <algorithm>

namespace Lupine {

// Static member initialization
std::random_device UUID::s_random_device;
std::mt19937 UUID::s_generator(s_random_device());

UUID::UUID() {
    GenerateUUID();
}

UUID::UUID(const std::string& uuid_string) : m_uuid_string(uuid_string) {
    // Validate the UUID string format (basic check)
    if (uuid_string.length() != 36 ||
        uuid_string[8] != '-' || uuid_string[13] != '-' ||
        uuid_string[18] != '-' || uuid_string[23] != '-') {
        // If invalid, generate a new UUID
        GenerateUUID();
    }
}

std::string UUID::ToString() const {
    return m_uuid_string;
}

bool UUID::IsNil() const {
    return m_uuid_string == "00000000-0000-0000-0000-000000000000";
}

UUID UUID::Generate() {
    return UUID();
}

UUID UUID::FromString(const std::string& uuid_string) {
    return UUID(uuid_string);
}

UUID UUID::Nil() {
    return UUID("00000000-0000-0000-0000-000000000000");
}

bool UUID::operator==(const UUID& other) const {
    return m_uuid_string == other.m_uuid_string;
}

bool UUID::operator!=(const UUID& other) const {
    return m_uuid_string != other.m_uuid_string;
}

bool UUID::operator<(const UUID& other) const {
    return m_uuid_string < other.m_uuid_string;
}

std::size_t UUID::Hash::operator()(const UUID& uuid) const {
    return std::hash<std::string>{}(uuid.m_uuid_string);
}

void UUID::GenerateUUID() {
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    std::uniform_int_distribution<uint16_t> dist16(0, 0xFFFF);

    uint32_t data1 = dist(s_generator);
    uint16_t data2 = dist16(s_generator);
    uint16_t data3 = dist16(s_generator);
    uint16_t data4 = dist16(s_generator);
    uint64_t data5 = (static_cast<uint64_t>(dist(s_generator)) << 16) | dist16(s_generator);

    // Set version (4) and variant bits
    data3 = (data3 & 0x0FFF) | 0x4000; // Version 4
    data4 = (data4 & 0x3FFF) | 0x8000; // Variant 10

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8) << data1 << "-"
        << std::setw(4) << data2 << "-"
        << std::setw(4) << data3 << "-"
        << std::setw(4) << data4 << "-"
        << std::setw(12) << data5;

    m_uuid_string = oss.str();
}

} // namespace Lupine
