#include "../../include/procgen/TerrainConstraints.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace procgen {
namespace {

constexpr std::uint32_t SERIALIZATION_MAGIC = 0x43545247u; // GRTC
constexpr std::uint32_t SERIALIZATION_VERSION = 1;

void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (std::uint32_t shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
    }
}

void append_float(std::vector<std::uint8_t>& bytes, float value) {
    append_u32(bytes, std::bit_cast<std::uint32_t>(value));
}

bool read_u32(std::span<const std::uint8_t> bytes, std::size_t& cursor, std::uint32_t& value) {
    if (cursor + 4 > bytes.size()) {
        return false;
    }

    value = 0;
    for (std::uint32_t shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(bytes[cursor++]) << shift;
    }
    return true;
}

bool read_float(std::span<const std::uint8_t> bytes, std::size_t& cursor, float& value) {
    std::uint32_t bits = 0;
    if (!read_u32(bytes, cursor, bits)) {
        return false;
    }
    value = std::bit_cast<float>(bits);
    return std::isfinite(value);
}

float lerp(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

} // namespace

TerrainConstraintField::TerrainConstraintField(std::uint32_t width, std::uint32_t height) {
    resize(width, height);
}

bool TerrainConstraintField::has_expected_sample_count() const noexcept {
    const auto count = static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height);
    return m_values.size() == count && m_influences.size() == count;
}

void TerrainConstraintField::resize(std::uint32_t width, std::uint32_t height) {
    m_width = width;
    m_height = height;
    const auto count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    m_values.assign(count, 0.0f);
    m_influences.assign(count, 0.0f);
}

void TerrainConstraintField::clear() noexcept {
    std::fill(m_values.begin(), m_values.end(), 0.0f);
    std::fill(m_influences.begin(), m_influences.end(), 0.0f);
}

std::size_t TerrainConstraintField::index(std::uint32_t x, std::uint32_t y) const noexcept {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(x);
}

void TerrainConstraintField::paint(
    TerrainConstraintTool tool,
    float normalized_x,
    float normalized_y,
    float normalized_radius,
    float strength
) {
    if (!has_expected_sample_count() || m_width == 0 || m_height == 0) {
        return;
    }

    normalized_x = std::clamp(normalized_x, 0.0f, 1.0f);
    normalized_y = std::clamp(normalized_y, 0.0f, 1.0f);
    normalized_radius = std::max(normalized_radius, 0.0001f);
    strength = std::clamp(strength, 0.0f, 1.0f);
    const float target = terrain_constraint_value(tool);

    for (std::uint32_t y = 0; y < m_height; ++y) {
        const float v = m_height > 1 ? static_cast<float>(y) / static_cast<float>(m_height - 1) : 0.0f;
        for (std::uint32_t x = 0; x < m_width; ++x) {
            const float u = m_width > 1 ? static_cast<float>(x) / static_cast<float>(m_width - 1) : 0.0f;
            const float dx = u - normalized_x;
            const float dy = v - normalized_y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (distance > normalized_radius) {
                continue;
            }

            const float falloff = 1.0f - distance / normalized_radius;
            const float amount = strength * falloff * falloff * (3.0f - 2.0f * falloff);
            const auto sample_index = index(x, y);
            m_values[sample_index] = lerp(m_values[sample_index], target, amount);
            m_influences[sample_index] = std::max(m_influences[sample_index], amount);
        }
    }
}

TerrainConstraintSample TerrainConstraintField::sample_at(std::uint32_t x, std::uint32_t y) const noexcept {
    if (!has_expected_sample_count() || x >= m_width || y >= m_height) {
        return {};
    }

    const auto sample_index = index(x, y);
    return {m_values[sample_index], m_influences[sample_index]};
}

TerrainConstraintSample TerrainConstraintField::sample(float normalized_x, float normalized_y) const noexcept {
    if (!has_expected_sample_count() || m_width == 0 || m_height == 0) {
        return {};
    }

    const float sample_x = std::clamp(normalized_x, 0.0f, 1.0f) * static_cast<float>(m_width - 1);
    const float sample_y = std::clamp(normalized_y, 0.0f, 1.0f) * static_cast<float>(m_height - 1);
    const auto x0 = static_cast<std::uint32_t>(std::floor(sample_x));
    const auto y0 = static_cast<std::uint32_t>(std::floor(sample_y));
    const auto x1 = std::min(x0 + 1, m_width - 1);
    const auto y1 = std::min(y0 + 1, m_height - 1);
    const float tx = sample_x - static_cast<float>(x0);
    const float ty = sample_y - static_cast<float>(y0);

    const auto a = sample_at(x0, y0);
    const auto b = sample_at(x1, y0);
    const auto c = sample_at(x0, y1);
    const auto d = sample_at(x1, y1);
    return {
        lerp(lerp(a.elevation, b.elevation, tx), lerp(c.elevation, d.elevation, tx), ty),
        lerp(lerp(a.influence, b.influence, tx), lerp(c.influence, d.influence, tx), ty)
    };
}

float terrain_constraint_value(TerrainConstraintTool tool) noexcept {
    switch (tool) {
        case TerrainConstraintTool::Ocean:
            return -0.25f;
        case TerrainConstraintTool::ShallowWater:
            return -0.05f;
        case TerrainConstraintTool::Valley:
            return 0.05f;
        case TerrainConstraintTool::Mountain:
            return 1.0f;
    }
    return 0.0f;
}

std::vector<std::uint8_t> serialize_terrain_constraints(const TerrainConstraintField& field) {
    if (!field.has_expected_sample_count()) {
        return {};
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(16 + field.m_values.size() * 8);
    append_u32(bytes, SERIALIZATION_MAGIC);
    append_u32(bytes, SERIALIZATION_VERSION);
    append_u32(bytes, field.m_width);
    append_u32(bytes, field.m_height);
    for (std::size_t index = 0; index < field.m_values.size(); ++index) {
        append_float(bytes, field.m_values[index]);
        append_float(bytes, field.m_influences[index]);
    }
    return bytes;
}

std::optional<TerrainConstraintField> deserialize_terrain_constraints(std::span<const std::uint8_t> bytes) {
    std::size_t cursor = 0;
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    if (!read_u32(bytes, cursor, magic)
        || !read_u32(bytes, cursor, version)
        || !read_u32(bytes, cursor, width)
        || !read_u32(bytes, cursor, height)
        || magic != SERIALIZATION_MAGIC
        || version != SERIALIZATION_VERSION
        || width == 0
        || height == 0) {
        return std::nullopt;
    }

    const auto count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (count > (std::numeric_limits<std::size_t>::max() - 16) / 8 || bytes.size() != 16 + count * 8) {
        return std::nullopt;
    }

    TerrainConstraintField field(width, height);
    for (std::size_t index = 0; index < count; ++index) {
        if (!read_float(bytes, cursor, field.m_values[index])
            || !read_float(bytes, cursor, field.m_influences[index])
            || field.m_values[index] < -1.0f
            || field.m_values[index] > 1.0f
            || field.m_influences[index] < 0.0f
            || field.m_influences[index] > 1.0f) {
            return std::nullopt;
        }
    }
    return field;
}

} // namespace procgen
