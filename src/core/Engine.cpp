#include "../../include/core/Engine.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include <SDL3/SDL.h>

namespace core {

Engine::Engine() = default;

Engine::~Engine() {
    if (!shutdown_)
        shutdown();
}

bool Engine::initialize(const char* title, int width, int height) {
    if (initialized_) {
        return true;
    }

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    // Create GPU device
    gpu_device_ = std::make_unique<rendering::GPUDevice>();
    if (!gpu_device_) {
        SDL_Log("Failed to create GPU device");
        window_ = nullptr;
        SDL_Quit();
        return false;
    }

    // Create window
    window_ = SDL_CreateWindow(title, width, height, 0L);
    if (!window_) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_device_->handle(), window_))
	{
		SDL_Log("GPUClaimWindow failed");
		return false;
	}

    // Create subsystems
    renderer_ = std::make_unique<rendering::Renderer>(gpu_device_.get(), window_);
    texture_manager_ = std::make_unique<rendering::TextureManager>(gpu_device_.get());
    font_manager_ = std::make_unique<rendering::FontManager>(gpu_device_.get(), texture_manager_.get());
    ui_renderer_ = std::make_unique<rendering::UIRenderer>(gpu_device_.get());
    ui_renderer_->setTextureManager(texture_manager_.get());
    ui_renderer_->setFontManager(font_manager_.get());
    asset_manager_ = std::make_unique<assets::AssetManager>(texture_manager_.get(), nullptr);

    // Register texture manager as a world resource for the renderer
    world_.set_resource(texture_manager_.get());

    // Initialize viewport with initial window size (handles HiDPI)
    int pixel_w = 0, pixel_h = 0;
    SDL_GetWindowSizeInPixels(window_, &pixel_w, &pixel_h);
    on_resize(pixel_w, pixel_h);

    time_.reset();
    initialized_ = true;

    SDL_Log("Engine initialized successfully");
    return true;
}

void Engine::shutdown() noexcept {
    if (!initialized_) {
        return;
    }

    // Notify game of shutdown
    if (game_) {
        game_->on_shutdown(*this);
        ui_manager_.setRoot(nullptr);
        game_.reset();
    }
    SDL_Log("Game Shutdown");

    // Remove texture manager from world resources
    world_.remove_resource<rendering::TextureManager>();
    
    // Clear world before destroying subsystems
    world_.clear();
    SDL_Log("World Clear");

    // Destroy subsystems in reverse order
    if (asset_manager_) {
        asset_manager_->clear();
        asset_manager_.reset();
        SDL_Log("Asset Manager Reset");
    }

    ui_renderer_.reset();
    SDL_Log("UI Renderer Reset");

    if (font_manager_) {
        font_manager_->clear();
        font_manager_.reset();
        SDL_Log("Font Manager Reset");
    }

    if (texture_manager_) {
        texture_manager_->clear();
        texture_manager_.reset();
        SDL_Log("Texture Manager Reset");
    }

    renderer_.reset();
    SDL_Log("Renderer Reset");

    if (gpu_device_) {
        gpu_device_.reset();
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        SDL_Log("Destroy window");
        window_ = nullptr;
    }

    SDL_Quit();
    initialized_ = false;
    
    SDL_Log("Engine shutdown complete");
    shutdown_ = true;
}

void Engine::set_game(std::unique_ptr<Game> game) noexcept {
    game_ = std::move(game);
}

void Engine::run() {
    if (!initialized_) {
        SDL_Log("Engine not initialized");
        return;
    }

    // Call game startup hook
    if (game_) {
        game_->on_startup(*this);
    }

    running_ = true;
    accumulator_ = 0.0;
    time_.reset();

    SDL_Log("Entering main loop...");

    while (running_) {
        time_.tick();
        
        // Cap frame time to prevent spiral of death
        double frame_time = time_.unscaled_delta_time();
        if (frame_time > max_frame_time_) {
            frame_time = max_frame_time_;
        }

        accumulator_ += frame_time;

        // Process input
        process_events();

        // Fixed timestep updates
        const double fixed_dt = time_.fixed_delta_time();
        while (accumulator_ >= fixed_dt) {
            if (!time_.is_paused()) {
                fixed_update(static_cast<float>(fixed_dt));
            }
            accumulator_ -= fixed_dt;
        }

        // Variable timestep update
        update(time_.delta_time());

        // Render with interpolation
        const double alpha = accumulator_ / fixed_dt;
        render(alpha);

        // Check if we need to shutdown
        if (!running_) {
            shutdown();
        }
    }
}

void Engine::resize_window(int width, int height)
{
    SDL_SetWindowSize(window_, width, height);
    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    // Note: on_resize will be called via SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
}

void Engine::on_resize(int pixel_w, int pixel_h)
{
    if (pixel_w <= 0 || pixel_h <= 0) {
        return;
    }

    window_width_ = pixel_w;
    window_height_ = pixel_h;

    // Update UI manager with logical size
    ui_manager_.initialize(window_, static_cast<float>(LOGICAL_W), static_cast<float>(LOGICAL_H));

    SDL_Log("Window resized: %dx%d (logical: %dx%d)", pixel_w, pixel_h, LOGICAL_W, LOGICAL_H);
}

bool Engine::screen_to_logical(float screen_x, float screen_y,
                                float& logical_x, float& logical_y) const noexcept
{
    // Map screen pixels to logical coordinates
    // Full window maps to full logical space
    logical_x = (screen_x / static_cast<float>(window_width_)) * static_cast<float>(LOGICAL_W);
    logical_y = (screen_y / static_cast<float>(window_height_)) * static_cast<float>(LOGICAL_H);

    return true;
}

void Engine::logical_to_screen(float logical_x, float logical_y,
                                float& screen_x, float& screen_y) const noexcept
{
    // Map logical coordinates to screen pixels
    screen_x = (logical_x / static_cast<float>(LOGICAL_W)) * static_cast<float>(window_width_);
    screen_y = (logical_y / static_cast<float>(LOGICAL_H)) * static_cast<float>(window_height_);
}

void Engine::process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Let UI manager handle the event first
        const bool ui_consumed = ui_manager_.handleEvent(event);
        if (game_) {
            game_->on_event(*this, event, ui_consumed);
        }
        if (ui_consumed) {
            continue; // Event was consumed by UI
        }

        switch (event.type) {
            case SDL_EVENT_QUIT:
                quit();
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    quit();
                }
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                // GPU swapchain has been resized - update viewport
                const int pixel_w = event.window.data1;
                const int pixel_h = event.window.data2;
                on_resize(pixel_w, pixel_h);
                if (game_) {
                    game_->on_resized(*this, pixel_w, pixel_h);
                }
                break;
            }
            default:
                break;
        }
    }
}

void Engine::fixed_update(float dt) {
    // Update all ECS systems with fixed timestep
    world_.update(dt);
}

void Engine::update(double dt) {
    // Update UI manager
    ui_manager_.update(static_cast<float>(dt));

    // Call game update hook
    if (game_) {
        game_->on_update(*this, dt);
    }
}

void Engine::render(double alpha) {
    if (!renderer_->begin_frame()) {
        quit();
    }

    // Set logical resolution for coordinate mapping
    renderer_->set_logical_size(LOGICAL_W, LOGICAL_H);

    renderer_->clear(0, 0, 51, 255);

    // Render ECS world sprites first
    renderer_->render(world_, alpha);

    // Call game render hook for custom rendering between world sprites and retained UI.
    if (game_) {
        game_->on_render(*this, alpha);
    }

    render_ui();
    renderer_->present();
}

void Engine::render_ui() {
    auto* root = ui_manager_.root();
    if (!renderer_ || !ui_renderer_ || !root) {
        return;
    }

    renderer_->end_render_pass();
    ui_renderer_->render(
        renderer_->command_buffer(),
        renderer_->swapchain_texture(),
        root,
        static_cast<float>(LOGICAL_W),
        static_cast<float>(LOGICAL_H)
    );
}

} // namespace core
