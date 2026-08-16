#pragma once

#include "TerrainMesh.hpp"
#include <SDL3/SDL_gpu.h>

namespace rendering {

class GPUDevice;
class Renderer;

class TerrainRenderer {
public:
    explicit TerrainRenderer(GPUDevice* device) noexcept;
    ~TerrainRenderer();

    TerrainRenderer(const TerrainRenderer&) = delete;
    TerrainRenderer& operator=(const TerrainRenderer&) = delete;
    TerrainRenderer(TerrainRenderer&&) = delete;
    TerrainRenderer& operator=(TerrainRenderer&&) = delete;

    void set_mesh(TerrainMesh mesh);
    void clear_mesh();

    [[nodiscard]] bool is_enabled() const noexcept { return m_enabled; }
    void set_enabled(bool enabled) noexcept { m_enabled = enabled; }

    [[nodiscard]] float elevation_scale() const noexcept { return m_elevationScale; }
    void set_elevation_scale(float scale) noexcept;

    [[nodiscard]] float viewport_left_ratio() const noexcept { return m_viewportLeftRatio; }
    void set_viewport_left_ratio(float ratio) noexcept;

    [[nodiscard]] const TerrainMesh& mesh() const noexcept { return m_mesh; }
    void render(Renderer& renderer);

private:
    [[nodiscard]] bool upload_mesh(Renderer& renderer);
    void release_buffers() noexcept;

    GPUDevice* m_device{nullptr};
    TerrainMesh m_mesh;
    SDL_GPUBuffer* m_vertexBuffer{nullptr};
    SDL_GPUBuffer* m_indexBuffer{nullptr};
    bool m_meshDirty{false};
    bool m_enabled{false};
    float m_elevationScale{100.0f};
    float m_viewportLeftRatio{0.0f};
};

} // namespace rendering
