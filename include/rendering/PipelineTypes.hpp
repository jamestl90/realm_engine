#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <cstdint>

namespace rendering {

// Core pipeline types available in the engine
enum class PipelineType : std::uint8_t {
    Sprite,         // Textured 2D sprites
    Shape,          // Solid colour shapes
    Text,           // Text rendering
    PostProcess,    // Full-screen post effects
    Count           // Number of core pipeline types
};

// Blend mode configuration
struct BlendConfig {
    bool enable{true};
    SDL_GPUBlendFactor src_color{SDL_GPU_BLENDFACTOR_SRC_ALPHA};
    SDL_GPUBlendFactor dst_color{SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA};
    SDL_GPUBlendFactor src_alpha{SDL_GPU_BLENDFACTOR_ONE};
    SDL_GPUBlendFactor dst_alpha{SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA};
    SDL_GPUBlendOp color_op{SDL_GPU_BLENDOP_ADD};
    SDL_GPUBlendOp alpha_op{SDL_GPU_BLENDOP_ADD};
};

// Depth/stencil configuration
struct DepthStencilConfig {
    bool depth_test{false};
    bool depth_write{false};
    SDL_GPUCompareOp depth_compare{SDL_GPU_COMPAREOP_LESS};
    bool stencil_test{false};
    std::uint8_t stencil_read_mask{0xFF};
    std::uint8_t stencil_write_mask{0xFF};
};

// Pipeline configuration
struct PipelineConfig {
    BlendConfig blend;
    DepthStencilConfig depth_stencil;
    SDL_GPUPrimitiveType primitive_type{SDL_GPU_PRIMITIVETYPE_TRIANGLELIST};
    SDL_GPUCullMode cull_mode{SDL_GPU_CULLMODE_NONE};
    SDL_GPUFrontFace front_face{SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE};
    bool scissor_test{false};
};

} // namespace rendering
