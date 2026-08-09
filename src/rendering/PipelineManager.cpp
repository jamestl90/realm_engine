#include "../../include/rendering/PipelineManager.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include "../../include/rendering/Sprite.hpp"
#include "../../include/rendering/ShaderReflection.hpp"
#include "../../include/core/Config.hpp"
#include "../../include/rendering/UniformBuffers.hpp"
#include <fstream>
#include <cassert>

namespace rendering {

std::size_t PipelineConfigHash::operator()(const PipelineConfig& config) const noexcept {
    std::size_t hash = 0;
    
    auto hash_combine = [&hash](auto value) {
        hash ^= std::hash<decltype(value)>{}(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    };

    hash_combine(config.blend.enable);
    hash_combine(static_cast<int>(config.blend.src_color));
    hash_combine(static_cast<int>(config.blend.dst_color));
    hash_combine(static_cast<int>(config.blend.src_alpha));
    hash_combine(static_cast<int>(config.blend.dst_alpha));
    hash_combine(static_cast<int>(config.blend.color_op));
    hash_combine(static_cast<int>(config.blend.alpha_op));
    hash_combine(config.depth_stencil.depth_test);
    hash_combine(config.depth_stencil.depth_write);
    hash_combine(static_cast<int>(config.depth_stencil.depth_compare));
    hash_combine(config.depth_stencil.stencil_test);
    hash_combine(static_cast<int>(config.primitive_type));
    hash_combine(static_cast<int>(config.cull_mode));
    hash_combine(static_cast<int>(config.front_face));
    hash_combine(config.scissor_test);

    return hash;
}

bool PipelineConfigEqual::operator()(const PipelineConfig& a, const PipelineConfig& b) const noexcept {
    return a.blend.enable == b.blend.enable &&
           a.blend.src_color == b.blend.src_color &&
           a.blend.dst_color == b.blend.dst_color &&
           a.blend.src_alpha == b.blend.src_alpha &&
           a.blend.dst_alpha == b.blend.dst_alpha &&
           a.blend.color_op == b.blend.color_op &&
           a.blend.alpha_op == b.blend.alpha_op &&
           a.depth_stencil.depth_test == b.depth_stencil.depth_test &&
           a.depth_stencil.depth_write == b.depth_stencil.depth_write &&
           a.depth_stencil.depth_compare == b.depth_stencil.depth_compare &&
           a.depth_stencil.stencil_test == b.depth_stencil.stencil_test &&
           a.depth_stencil.stencil_read_mask == b.depth_stencil.stencil_read_mask &&
           a.depth_stencil.stencil_write_mask == b.depth_stencil.stencil_write_mask &&
           a.primitive_type == b.primitive_type &&
           a.cull_mode == b.cull_mode &&
           a.front_face == b.front_face &&
           a.scissor_test == b.scissor_test;
}

PipelineManager::PipelineManager(GPUDevice* device)
    : device_(device) {
    assert(device_ && "GPUDevice cannot be null");
    core_pipelines_.fill(nullptr);
    vertex_shaders_.fill(nullptr);
    fragment_shaders_.fill(nullptr);
    pipelines_.reserve(32);
}

PipelineManager::~PipelineManager() {
    release_all();
}

PipelineManager::PipelineManager(PipelineManager&& other) noexcept
    : device_(other.device_)
    , swapchain_format_(other.swapchain_format_)
    , core_pipelines_(other.core_pipelines_)
    , vertex_shaders_(other.vertex_shaders_)
    , fragment_shaders_(other.fragment_shaders_)
    , pipeline_cache_(std::move(other.pipeline_cache_))
    , pipelines_(std::move(other.pipelines_))
    , next_handle_(other.next_handle_) {
    other.device_ = nullptr;
    other.core_pipelines_.fill(nullptr);
    other.vertex_shaders_.fill(nullptr);
    other.fragment_shaders_.fill(nullptr);
}

PipelineManager& PipelineManager::operator=(PipelineManager&& other) noexcept {
    if (this != &other) {
        release_all();
        device_ = other.device_;
        swapchain_format_ = other.swapchain_format_;
        core_pipelines_ = other.core_pipelines_;
        vertex_shaders_ = other.vertex_shaders_;
        fragment_shaders_ = other.fragment_shaders_;
        pipeline_cache_ = std::move(other.pipeline_cache_);
        pipelines_ = std::move(other.pipelines_);
        next_handle_ = other.next_handle_;
        other.device_ = nullptr;
        other.core_pipelines_.fill(nullptr);
        other.vertex_shaders_.fill(nullptr);
        other.fragment_shaders_.fill(nullptr);
    }
    return *this;
}

bool PipelineManager::initialise(SDL_GPUTextureFormat swapchain_format) {
    swapchain_format_ = swapchain_format;

    if (!initialise_sprite_pipeline(swapchain_format)) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to initialise sprite pipeline");
        return false;
    }

    // init other pipelines here
    
    SDL_Log("PipelineManager initialised successfully");
    return true;
}

SDL_GPUGraphicsPipeline* PipelineManager::get_core_pipeline(PipelineType type) const noexcept {
    const auto index = static_cast<std::size_t>(type);
    if (index >= core_pipelines_.size()) {
        return nullptr;
    }
    return core_pipelines_[index];
}

PipelineHandle PipelineManager::get_or_create_pipeline(
    PipelineType base_type,
    const PipelineConfig& config
) {
    auto it = pipeline_cache_.find(config);
    if (it != pipeline_cache_.end()) {
        return it->second;
    }

    // Load shader reflection data
    const auto base_path = config::get_executable_dir() / "assets" / "Shaders";
    
    std::string shader_prefix;
    switch (base_type) {
        case PipelineType::Sprite:
        case PipelineType::Text:
            shader_prefix = "sprite";
            break;
        case PipelineType::Shape:
            shader_prefix = "shape";
            break;
        default:
            shader_prefix = "sprite";
            break;
    }
    
    auto vert_reflection = load_shader_reflection((base_path / (shader_prefix + ".vert.reflect.json")).string());
    auto frag_reflection = load_shader_reflection((base_path / (shader_prefix + ".frag.reflect.json")).string());

    SDL_GPUGraphicsPipeline* pipeline = create_pipeline(base_type, config, swapchain_format_, vert_reflection, frag_reflection);
    if (!pipeline) {
        return INVALID_PIPELINE_HANDLE;
    }

    PipelineHandle handle = next_handle_++;
    PipelineEntry entry;
    entry.pipeline = pipeline;
    entry.base_type = base_type;
    entry.config = config;
    pipelines_.push_back(entry);
    pipeline_cache_[config] = handle;

    return handle;
}

SDL_GPUGraphicsPipeline* PipelineManager::get_pipeline(PipelineHandle handle) const noexcept {
    if (handle == INVALID_PIPELINE_HANDLE || handle > pipelines_.size()) {
        return nullptr;
    }
    return pipelines_[handle - 1].pipeline;
}

std::optional<ShaderBytecode> PipelineManager::load_shader(
    const std::string& path,
    SDL_GPUShaderStage stage
) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to open shader: %s", path.c_str());
        return std::nullopt;
    }

    const std::streamsize size_in_bytes = file.tellg();
    
    // SPIR-V Requirement: Must be a multiple of 4 bytes
    if (size_in_bytes <= 0 || (size_in_bytes % 4) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Invalid SPIR-V file size: %s", path.c_str());
        return std::nullopt;
    }

    file.seekg(0, std::ios::beg);

    ShaderBytecode bytecode;
    bytecode.stage = stage;
    bytecode.data.resize(static_cast<std::size_t>(size_in_bytes));

    if (!file.read(reinterpret_cast<char*>(bytecode.data.data()), size_in_bytes)) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to read shader: %s", path.c_str());
        return std::nullopt;
    }

    return bytecode;
}

void PipelineManager::release_all() {
    if (!device_ || !device_->is_valid()) {
        return;
    }

    SDL_GPUDevice* gpu = device_->handle();
    if (!gpu) {
        return;
    }

    for (auto& entry : pipelines_) {
        if (entry.pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(gpu, entry.pipeline);
            entry.pipeline = nullptr;
        }
    }
    pipelines_.clear();
    pipeline_cache_.clear();

    for (auto& pipeline : core_pipelines_) {
        if (pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);
            pipeline = nullptr;
        }
    }

    for (auto& shader : vertex_shaders_) {
        if (shader) {
            SDL_ReleaseGPUShader(gpu, shader);
            shader = nullptr;
        }
    }
    for (auto& shader : fragment_shaders_) {
        if (shader) {
            SDL_ReleaseGPUShader(gpu, shader);
            shader = nullptr;
        }
    }
}

PipelineConfig PipelineManager::get_default_config(PipelineType type) noexcept {
    PipelineConfig config{};

    config.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    config.blend.color_op = SDL_GPU_BLENDOP_ADD;
    config.blend.alpha_op = SDL_GPU_BLENDOP_ADD;
    config.blend.src_alpha = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    config.blend.dst_alpha = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    config.depth_stencil.depth_compare = SDL_GPU_COMPAREOP_LESS;
    config.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

    switch (type) {
        case PipelineType::Sprite:
        case PipelineType::Shape:
        case PipelineType::Text:
            config.blend.enable = true;
            config.blend.src_color = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            config.blend.dst_color = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            config.depth_stencil.depth_test = false;
            config.depth_stencil.depth_write = false;
            config.cull_mode = SDL_GPU_CULLMODE_NONE;
            break;

        case PipelineType::PostProcess:
            config.blend.enable = false;
            config.depth_stencil.depth_test = false;
            config.depth_stencil.depth_write = false;
            config.cull_mode = SDL_GPU_CULLMODE_NONE;
            break;

        default:
            break;
    }

    return config;
}

SDL_GPUShader* PipelineManager::create_shader(const ShaderBytecode& bytecode) const {
    if (!device_ || !device_->is_valid() || bytecode.data.empty()) {
        return nullptr;
    }

    SDL_GPUShaderCreateInfo create_info{};
    
    create_info.code = bytecode.data.data();
    create_info.code_size = bytecode.data.size(); 
    
    create_info.entrypoint = "main";
    create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    create_info.stage = bytecode.stage;
    
    create_info.num_samplers = bytecode.num_samplers;
    create_info.num_storage_textures = bytecode.num_storage_textures;
    create_info.num_storage_buffers = bytecode.num_storage_buffers;
    create_info.num_uniform_buffers = bytecode.num_uniform_buffers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device_->handle(), &create_info);
    
    if (!shader) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, 
            "SDL_CreateGPUShader failed for stage %d. Error: %s\n"
            "Reflection data: samplers=%u, storage_textures=%u, storage_buffers=%u, uniform_buffers=%u", 
            static_cast<int>(bytecode.stage), 
            SDL_GetError(),
            bytecode.num_samplers,
            bytecode.num_storage_textures,
            bytecode.num_storage_buffers,
            bytecode.num_uniform_buffers);
    }

    return shader;
}

SDL_GPUGraphicsPipeline* PipelineManager::create_pipeline(
    PipelineType type,
    const PipelineConfig& config,
    SDL_GPUTextureFormat target_format,
    const ShaderReflectionData& vert_reflection,
    const ShaderReflectionData& frag_reflection
) {
    const auto type_index = static_cast<std::size_t>(type);
    
    if (!vertex_shaders_[type_index] || !fragment_shaders_[type_index]) {
        return nullptr;
    }

    SDL_GPUVertexInputState vertex_input = build_vertex_input_state(type);

    if (target_format == SDL_GPU_TEXTUREFORMAT_INVALID) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Invalid target format passed to pipeline creation");
        return nullptr;
    }

    SDL_GPUColorTargetBlendState blend_state{};
    blend_state.enable_blend = config.blend.enable;
    blend_state.src_color_blendfactor = config.blend.src_color;
    blend_state.dst_color_blendfactor = config.blend.dst_color;
    blend_state.color_blend_op = config.blend.color_op;
    blend_state.src_alpha_blendfactor = config.blend.src_alpha;
    blend_state.dst_alpha_blendfactor = config.blend.dst_alpha;
    blend_state.alpha_blend_op = config.blend.alpha_op;
    blend_state.color_write_mask = 0xF;

    SDL_GPUColorTargetDescription color_target{};
    color_target.format = target_format;
    color_target.blend_state = blend_state;

    SDL_GPURasterizerState rasterizer{};
    rasterizer.cull_mode = config.cull_mode;
    rasterizer.front_face = config.front_face;
    rasterizer.fill_mode = SDL_GPU_FILLMODE_FILL;

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.vertex_shader = vertex_shaders_[type_index];
    pipeline_info.fragment_shader = fragment_shaders_[type_index];
    pipeline_info.primitive_type = config.primitive_type;
    pipeline_info.vertex_input_state = vertex_input;
    pipeline_info.rasterizer_state = rasterizer;
    
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &color_target;
    
    pipeline_info.target_info.has_depth_stencil_target = config.depth_stencil.depth_test;
    pipeline_info.target_info.depth_stencil_format = config.depth_stencil.depth_test 
        ? SDL_GPU_TEXTUREFORMAT_D16_UNORM
        : SDL_GPU_TEXTUREFORMAT_INVALID;

    pipeline_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;

    SDL_Log("Creating pipeline with reflection - Vert UBOs: %u, Frag Samplers: %u",
        vert_reflection.num_uniform_buffers, frag_reflection.num_samplers);

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_->handle(), &pipeline_info);
    return pipeline;
}

SDL_GPUVertexInputState PipelineManager::build_vertex_input_state(PipelineType type) const noexcept {
    static SDL_GPUVertexBufferDescription sprite_buffer_desc{};
    static SDL_GPUVertexAttribute sprite_attributes[3]{};
    
    static SDL_GPUVertexBufferDescription shape_buffer_desc{};
    static SDL_GPUVertexAttribute shape_attributes[2]{};

    SDL_GPUVertexInputState state{};

    switch (type) {
        case PipelineType::Sprite:
        case PipelineType::Text:
            sprite_attributes[0].location = 0;
            sprite_attributes[0].buffer_slot = 0;
            sprite_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            sprite_attributes[0].offset = offsetof(SpriteVertex, x);

            sprite_attributes[1].location = 1;
            sprite_attributes[1].buffer_slot = 0;
            sprite_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            sprite_attributes[1].offset = offsetof(SpriteVertex, u);

            sprite_attributes[2].location = 2;
            sprite_attributes[2].buffer_slot = 0;
            sprite_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
            sprite_attributes[2].offset = offsetof(SpriteVertex, r);

            sprite_buffer_desc.slot = 0;
            sprite_buffer_desc.pitch = sizeof(SpriteVertex);
            sprite_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            sprite_buffer_desc.instance_step_rate = 0;

            state.num_vertex_buffers = 1;
            state.vertex_buffer_descriptions = &sprite_buffer_desc;
            state.num_vertex_attributes = 3;
            state.vertex_attributes = sprite_attributes;
            break;
        default:
            break;
    }

    return state;
}

bool PipelineManager::initialise_sprite_pipeline(SDL_GPUTextureFormat format) {
    const auto base_path = config::get_executable_dir() / "assets" / "Shaders";
    
    auto vert_bytecode = load_shader((base_path / "sprite.vert.spv").string(), SDL_GPU_SHADERSTAGE_VERTEX);
    if (!vert_bytecode) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to load sprite vertex shader");
        return false;
    }

    auto vert_reflection = load_shader_reflection((base_path / "sprite.vert.reflect.json").string());

    if (vert_reflection.num_uniform_buffers > 0 && !vert_reflection.validate_ubo_size<CameraData>()) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, 
            "CameraData size mismatch! CPU struct=%zu bytes, GPU expects=%u bytes",
            sizeof(CameraData), vert_reflection.ubo_block_size);
        return false;
    }

    vert_bytecode->num_uniform_buffers = vert_reflection.num_uniform_buffers;
    vert_bytecode->num_storage_textures = vert_reflection.num_storage_textures;
    vert_bytecode->num_storage_buffers = vert_reflection.num_storage_buffers;
    vert_bytecode->num_samplers = vert_reflection.num_samplers;

    auto frag_bytecode = load_shader((base_path / "sprite.frag.spv").string(), SDL_GPU_SHADERSTAGE_FRAGMENT);
    if (!frag_bytecode) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to load sprite fragment shader");
        return false;
    }

    auto frag_reflection = load_shader_reflection((base_path / "sprite.frag.reflect.json").string());

    frag_bytecode->num_uniform_buffers = frag_reflection.num_uniform_buffers;
    frag_bytecode->num_storage_textures = frag_reflection.num_storage_textures;
    frag_bytecode->num_storage_buffers = frag_reflection.num_storage_buffers;
    frag_bytecode->num_samplers = frag_reflection.num_samplers;

    const auto type_index = static_cast<std::size_t>(PipelineType::Sprite);
    vertex_shaders_[type_index] = create_shader(*vert_bytecode);
    if (!vertex_shaders_[type_index]) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create sprite vertex shader");
        return false;
    }

    fragment_shaders_[type_index] = create_shader(*frag_bytecode);
    if (!fragment_shaders_[type_index]) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create sprite fragment shader");
        return false;
    }

    PipelineConfig config = get_default_config(PipelineType::Sprite);
    core_pipelines_[type_index] = create_pipeline(PipelineType::Sprite, config, format, vert_reflection, frag_reflection);
    
    if (!core_pipelines_[type_index]) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create sprite pipeline");
        return false;
    }

    SDL_Log("Sprite pipeline initialised");
    return true;
}

} // namespace rendering
