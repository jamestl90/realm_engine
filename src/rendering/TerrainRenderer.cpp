#include "../../include/rendering/TerrainRenderer.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include "../../include/rendering/PipelineManager.hpp"
#include "../../include/rendering/Renderer.hpp"
#include "../../include/rendering/UniformBuffers.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace rendering {
namespace {

struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

using Matrix4 = std::array<float, 16>;

Vec3 operator-(const Vec3& left, const Vec3& right) noexcept {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

float dot(const Vec3& left, const Vec3& right) noexcept {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3 cross(const Vec3& left, const Vec3& right) noexcept {
    return {
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

Vec3 normalize(const Vec3& value) noexcept {
    const float length = std::sqrt(dot(value, value));
    if (length <= 0.00001f) {
        return {};
    }
    return {value.x / length, value.y / length, value.z / length};
}

Matrix4 multiply(const Matrix4& left, const Matrix4& right) noexcept {
    Matrix4 result{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t index = 0; index < 4; ++index) {
                result[column * 4 + row] += left[index * 4 + row]
                    * right[column * 4 + index];
            }
        }
    }
    return result;
}

Vec3 transform_point(const Matrix4& matrix, const Vec3& point) noexcept {
    return {
        matrix[0] * point.x + matrix[4] * point.y + matrix[8] * point.z + matrix[12],
        matrix[1] * point.x + matrix[5] * point.y + matrix[9] * point.z + matrix[13],
        matrix[2] * point.x + matrix[6] * point.y + matrix[10] * point.z + matrix[14]
    };
}

Matrix4 look_at(const Vec3& eye, const Vec3& target, const Vec3& up) noexcept {
    const Vec3 forward = normalize(target - eye);
    const Vec3 side = normalize(cross(forward, up));
    const Vec3 camera_up = cross(side, forward);

    return {
        side.x, camera_up.x, -forward.x, 0.0f,
        side.y, camera_up.y, -forward.y, 0.0f,
        side.z, camera_up.z, -forward.z, 0.0f,
        -dot(side, eye), -dot(camera_up, eye), dot(forward, eye), 1.0f
    };
}

Matrix4 orthographic(
    float left,
    float right,
    float bottom,
    float top,
    float near_plane,
    float far_plane
) noexcept {
    Matrix4 result{};
    result[0] = 2.0f / (right - left);
    result[5] = 2.0f / (top - bottom);
    result[10] = 1.0f / (near_plane - far_plane);
    result[12] = -(right + left) / (right - left);
    result[13] = -(top + bottom) / (top - bottom);
    result[14] = near_plane / (near_plane - far_plane);
    result[15] = 1.0f;
    return result;
}

Matrix4 terrain_view_projection(
    const TerrainMesh& mesh,
    float elevation_scale,
    float viewport_aspect
) noexcept {
    const float centre_elevation = (mesh.minimum_elevation + mesh.maximum_elevation)
        * elevation_scale * 0.5f;
    const float horizontal_extent = std::max(mesh.extent_x, mesh.extent_y);
    const Vec3 target{0.0f, 0.0f, centre_elevation};
    const Vec3 eye{
        0.0f,
        0.0f,
        centre_elevation + std::max(horizontal_extent, 1.0f)
    };
    const Matrix4 view = look_at(eye, target, {0.0f, 1.0f, 0.0f});

    const float half_x = mesh.extent_x * 0.5f;
    const float half_y = mesh.extent_y * 0.5f;
    const float minimum_z = mesh.minimum_elevation * elevation_scale;
    const float maximum_z = mesh.maximum_elevation * elevation_scale;
    const std::array<Vec3, 8> corners{
        Vec3{-half_x, -half_y, minimum_z}, Vec3{half_x, -half_y, minimum_z},
        Vec3{-half_x, half_y, minimum_z}, Vec3{half_x, half_y, minimum_z},
        Vec3{-half_x, -half_y, maximum_z}, Vec3{half_x, -half_y, maximum_z},
        Vec3{-half_x, half_y, maximum_z}, Vec3{half_x, half_y, maximum_z}
    };

    float minimum_view_x = std::numeric_limits<float>::max();
    float maximum_view_x = std::numeric_limits<float>::lowest();
    float minimum_view_y = std::numeric_limits<float>::max();
    float maximum_view_y = std::numeric_limits<float>::lowest();
    float minimum_distance = std::numeric_limits<float>::max();
    float maximum_distance = 0.0f;
    for (const Vec3& corner : corners) {
        const Vec3 view_corner = transform_point(view, corner);
        minimum_view_x = std::min(minimum_view_x, view_corner.x);
        maximum_view_x = std::max(maximum_view_x, view_corner.x);
        minimum_view_y = std::min(minimum_view_y, view_corner.y);
        maximum_view_y = std::max(maximum_view_y, view_corner.y);
        const float distance = -view_corner.z;
        minimum_distance = std::min(minimum_distance, distance);
        maximum_distance = std::max(maximum_distance, distance);
    }

    const float centre_x = (minimum_view_x + maximum_view_x) * 0.5f;
    const float centre_y = (minimum_view_y + maximum_view_y) * 0.5f;
    constexpr float framing_margin = 0.52f;
    float view_half_width = (maximum_view_x - minimum_view_x) * framing_margin;
    float view_half_height = (maximum_view_y - minimum_view_y) * framing_margin;
    const float safe_aspect = std::max(viewport_aspect, 0.01f);
    if (view_half_width / std::max(view_half_height, 0.01f) < safe_aspect) {
        view_half_width = view_half_height * safe_aspect;
    } else {
        view_half_height = view_half_width / safe_aspect;
    }

    const float near_plane = std::max(0.1f, minimum_distance * 0.8f);
    const float far_plane = std::max(near_plane + 1.0f, maximum_distance * 1.2f);
    const Matrix4 projection = orthographic(
        centre_x - view_half_width,
        centre_x + view_half_width,
        centre_y - view_half_height,
        centre_y + view_half_height,
        near_plane,
        far_plane
    );
    return multiply(projection, view);
}

} // namespace

TerrainRenderer::TerrainRenderer(GPUDevice* device) noexcept
    : m_device(device) {
}

TerrainRenderer::~TerrainRenderer() {
    release_buffers();
}

void TerrainRenderer::set_mesh(TerrainMesh mesh) {
    m_mesh = std::move(mesh);
    m_meshDirty = true;
}

void TerrainRenderer::clear_mesh() {
    m_mesh = {};
    m_meshDirty = false;
    release_buffers();
}

void TerrainRenderer::set_elevation_scale(float scale) noexcept {
    if (std::isfinite(scale)) {
        m_elevationScale = std::clamp(scale, 1.0f, 300.0f);
    }
}

void TerrainRenderer::set_viewport_left_ratio(float ratio) noexcept {
    if (std::isfinite(ratio)) {
        m_viewportLeftRatio = std::clamp(ratio, 0.0f, 0.95f);
    }
}

void TerrainRenderer::render(Renderer& renderer) {
    if (!m_enabled || m_mesh.empty() || !m_device || !m_device->is_valid()) {
        return;
    }
    if (!renderer.command_buffer() || !renderer.swapchain_texture()) {
        return;
    }

    if (m_meshDirty && !upload_mesh(renderer)) {
        return;
    }
    if (!m_vertexBuffer || !m_indexBuffer) {
        return;
    }

    SDL_GPURenderPass* render_pass = renderer.current_render_pass();
    PipelineManager* pipeline_manager = m_device->pipeline_manager();
    SDL_GPUGraphicsPipeline* pipeline = pipeline_manager
        ? pipeline_manager->get_core_pipeline(PipelineType::Terrain)
        : nullptr;
    if (!render_pass || !pipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Terrain render pass or pipeline is unavailable");
        return;
    }

    const float viewport_x = static_cast<float>(renderer.swapchain_width()) * m_viewportLeftRatio;
    const float viewport_width = static_cast<float>(renderer.swapchain_width()) - viewport_x;
    const float viewport_height = static_cast<float>(renderer.swapchain_height());
    SDL_GPUViewport terrain_viewport{
        viewport_x,
        0.0f,
        viewport_width,
        viewport_height,
        0.0f,
        1.0f
    };
    SDL_SetGPUViewport(render_pass, &terrain_viewport);

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

    SDL_GPUBufferBinding vertex_binding{};
    vertex_binding.buffer = m_vertexBuffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

    SDL_GPUBufferBinding index_binding{};
    index_binding.buffer = m_indexBuffer;
    SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    TerrainViewData view_data{};
    const float viewport_aspect = viewport_width / std::max(viewport_height, 1.0f);
    const Matrix4 view_projection = terrain_view_projection(
        m_mesh,
        m_elevationScale,
        viewport_aspect
    );
    std::memcpy(view_data.view_projection, view_projection.data(), sizeof(view_data.view_projection));
    view_data.terrain_parameters[0] = m_elevationScale;
    SDL_PushGPUVertexUniformData(
        renderer.command_buffer(),
        0,
        &view_data,
        sizeof(view_data)
    );

    TerrainLightData light_data{};
    const Vec3 light_direction = normalize({-0.35f, -0.45f, 0.82f});
    light_data.direction_and_ambient[0] = light_direction.x;
    light_data.direction_and_ambient[1] = light_direction.y;
    light_data.direction_and_ambient[2] = light_direction.z;
    light_data.direction_and_ambient[3] = 0.38f;
    SDL_PushGPUFragmentUniformData(
        renderer.command_buffer(),
        0,
        &light_data,
        sizeof(light_data)
    );

    SDL_DrawGPUIndexedPrimitives(
        render_pass,
        static_cast<Uint32>(m_mesh.indices.size()),
        1,
        0,
        0,
        0
    );

    SDL_GPUViewport full_viewport{
        0.0f,
        0.0f,
        static_cast<float>(renderer.swapchain_width()),
        static_cast<float>(renderer.swapchain_height()),
        0.0f,
        1.0f
    };
    SDL_SetGPUViewport(render_pass, &full_viewport);
}

bool TerrainRenderer::upload_mesh(Renderer& renderer) {
    if (!m_mesh.has_expected_shape() || m_mesh.empty()) {
        return false;
    }

    const std::size_t vertex_bytes = m_mesh.vertices.size() * sizeof(TerrainVertex);
    const std::size_t index_bytes = m_mesh.indices.size() * sizeof(std::uint32_t);
    if (vertex_bytes > std::numeric_limits<Uint32>::max()
        || index_bytes > std::numeric_limits<Uint32>::max()
        || vertex_bytes + index_bytes > std::numeric_limits<Uint32>::max()) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Terrain mesh exceeds SDL GPU buffer limits");
        return false;
    }

    renderer.end_render_pass();
    release_buffers();

    SDL_GPUBufferCreateInfo vertex_info{};
    vertex_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_info.size = static_cast<Uint32>(vertex_bytes);
    m_vertexBuffer = SDL_CreateGPUBuffer(m_device->handle(), &vertex_info);

    SDL_GPUBufferCreateInfo index_info{};
    index_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_info.size = static_cast<Uint32>(index_bytes);
    m_indexBuffer = SDL_CreateGPUBuffer(m_device->handle(), &index_info);
    if (!m_vertexBuffer || !m_indexBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create terrain GPU buffers: %s", SDL_GetError());
        release_buffers();
        (void)renderer.resume_render_pass(true);
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = static_cast<Uint32>(vertex_bytes + index_bytes);
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(m_device->handle(), &transfer_info);
    if (!transfer) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create terrain transfer buffer: %s", SDL_GetError());
        (void)renderer.resume_render_pass(true);
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(m_device->handle(), transfer, false);
    if (!mapped) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to map terrain transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(m_device->handle(), transfer);
        (void)renderer.resume_render_pass(true);
        return false;
    }
    std::memcpy(mapped, m_mesh.vertices.data(), vertex_bytes);
    std::memcpy(static_cast<std::uint8_t*>(mapped) + vertex_bytes, m_mesh.indices.data(), index_bytes);
    SDL_UnmapGPUTransferBuffer(m_device->handle(), transfer);

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(renderer.command_buffer());
    if (!copy_pass) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to begin terrain copy pass: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(m_device->handle(), transfer);
        (void)renderer.resume_render_pass(true);
        return false;
    }

    SDL_GPUTransferBufferLocation vertex_source{};
    vertex_source.transfer_buffer = transfer;
    SDL_GPUBufferRegion vertex_destination{};
    vertex_destination.buffer = m_vertexBuffer;
    vertex_destination.size = static_cast<Uint32>(vertex_bytes);
    SDL_UploadToGPUBuffer(copy_pass, &vertex_source, &vertex_destination, false);

    SDL_GPUTransferBufferLocation index_source{};
    index_source.transfer_buffer = transfer;
    index_source.offset = static_cast<Uint32>(vertex_bytes);
    SDL_GPUBufferRegion index_destination{};
    index_destination.buffer = m_indexBuffer;
    index_destination.size = static_cast<Uint32>(index_bytes);
    SDL_UploadToGPUBuffer(copy_pass, &index_source, &index_destination, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_ReleaseGPUTransferBuffer(m_device->handle(), transfer);
    m_meshDirty = false;
    return renderer.resume_render_pass(true);
}

void TerrainRenderer::release_buffers() noexcept {
    if (!m_device || !m_device->is_valid()) {
        m_vertexBuffer = nullptr;
        m_indexBuffer = nullptr;
        return;
    }
    if (m_vertexBuffer) {
        SDL_ReleaseGPUBuffer(m_device->handle(), m_vertexBuffer);
        m_vertexBuffer = nullptr;
    }
    if (m_indexBuffer) {
        SDL_ReleaseGPUBuffer(m_device->handle(), m_indexBuffer);
        m_indexBuffer = nullptr;
    }
}

} // namespace rendering
