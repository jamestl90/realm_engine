# High-Performance 2D Game Engine Architecture

## Overview
This engine is built on a pure Entity Component System (ECS) architecture with Data-Oriented Design (DoD) principles, leveraging SDL3 and the SDL3 GPU API for high-performance 2D rendering.

## Core Design Principles

### 1. ECS-First Architecture
- **Pure ECS**: Entities are IDs, Components are pure data (SoA layout), Systems are logic
- **Data-Oriented Design**: Structure-of-Arrays (SoA) for cache-line efficiency
- **Zero virtual dispatch**: Systems operate on component arrays directly

### 2. Memory Management
- **Zero-allocation main loop**: All frame allocations use arena/pool allocators
- **Cache-line awareness**: 64-byte alignment for hot data structures
- **Predictable access patterns**: Linear memory traversal in systems

### 3. Fixed Timestep Loop
- **Deterministic updates**: Fixed timestep (e.g., 60 Hz) for physics/logic
- **Interpolated rendering**: Smooth visuals independent of update rate
- **Time control**: Pause, slow-motion, fast-forward support

### 4. GPU-Driven Rendering
- **Batch rendering**: Minimize draw calls via texture atlases and instancing
- **Command buffers**: Decouple CPU and GPU work
- **Transfer buffers**: Async uploads for dynamic geometry

## System Architecture

```mermaid
graph TB
    subgraph "Engine Core"
        Engine[Engine]
        Time[Time System]
        Memory[Memory Allocators]
    end
    
    subgraph "ECS Foundation"
        World[ECS World]
        Entity[Entity Manager]
        Component[Component Storage SoA]
        System[System Registry]
    end
    
    subgraph "Game Systems"
        Input[Input System]
        Physics[Physics & Collision]
        Pathfinding[A* Pathfinding]
        Combat[Combat System]
        Audio[Audio System]
    end
    
    subgraph "Rendering Pipeline"
        Renderer[Batch Renderer]
        Sprite[Sprite System]
        Texture[Texture Manager]
        GPU[SDL3 GPU API]
    end
    
    subgraph "Asset Management"
        Assets[Asset Manager]
    end
    
    Engine --> Time
    Engine --> Memory
    Engine --> World
    World --> Entity
    World --> Component
    World --> System
    
    System --> Input
    System --> Physics
    System --> Pathfinding
    System --> Combat
    System --> Audio
    System --> Sprite
    
    Sprite --> Renderer
    Renderer --> Texture
    Renderer --> GPU
    
    Assets --> Texture
    Assets --> Audio
```

## Data Flow: Frame Loop

```mermaid
sequenceDiagram
    participant Main
    participant Engine
    participant Input
    participant Systems
    participant Renderer
    participant GPU
    
    Main->>Engine: Run()
    loop Game Loop
        Engine->>Engine: Accumulate dt
        
        loop While lag >= fixed_dt
            Engine->>Input: Poll Events
            Input-->>Engine: Input State
            
            Engine->>Systems: Fixed Update (dt)
            Note over Systems: Physics, Combat,<br/>Pathfinding, etc.
            Systems-->>Engine: Updated Components
            
            Engine->>Engine: lag -= fixed_dt
        end
        
        Engine->>Engine: Calculate alpha = lag / fixed_dt
        
        Engine->>Renderer: Render(alpha)
        Note over Renderer: Interpolate positions<br/>using alpha
        
        Renderer->>GPU: Submit Command Buffer
        GPU-->>Renderer: Present
    end
```

## Module Descriptions

### ECS Core (`include/ecs/`)
- **Entity**: Type-safe entity IDs (32-bit index + 32-bit generation)
- **Component**: SoA storage with cache-friendly iteration
- **System**: Interface for logic operating on component arrays
- **World**: Coordinates entity creation, component storage, system execution

### Memory Management (`include/memory/`)
- **ArenaAllocator**: Linear allocator, reset per frame
- **PoolAllocator**: Fixed-size block allocator for entities/components

### Core Engine (`include/core/`)
- **Engine**: Main loop with fixed timestep and interpolation
- **Time**: Manages game time, pause, time scale

### Rendering (`include/rendering/`)
- **Renderer**: SDL3 GPU batch renderer with instancing
- **Sprite**: Sprite component (texture region, layer, flip flags)
- **Texture**: Texture atlas management and GPU upload

### Physics (`include/physics/`)
- **SpatialPartition**: Grid-based spatial hash for broad-phase collision
- **Collision**: AABB and circle collision components/queries

### Pathfinding (`include/pathfinding/`)
- **AStar**: A* implementation on DoD grid representation

### Audio (`include/audio/`)
- **AudioSystem**: SDL3 audio playback and mixing

### Input (`include/input/`)
- **InputSystem**: Keyboard/mouse/gamepad polling

### Assets (`include/assets/`)
- **AssetManager**: Load and cache textures, audio, data files

### Combat (`include/combat/`)
- **CombatSystem**: Damage calculation, targeting, effects

## Performance Targets
- **Entities**: 10,000+ simultaneous entities
- **Draw Calls**: < 100 per frame via batching
- **Frame Budget**: 16.67ms (60 FPS) with headroom
- **Memory**: Predictable allocation patterns, no frame allocations

## Dependency Management
- **SDL3**: Core windowing, events, GPU API, audio
- **C++20**: Concepts, ranges, spans for type safety
- **No external ECS libraries**: Custom implementation for full control

## File Structure
```
project/
├── docs/
│   └── ARCHITECTURE.md
├── include/
│   ├── ecs/
│   │   ├── Entity.hpp
│   │   ├── Component.hpp
│   │   ├── System.hpp
│   │   └── World.hpp
│   ├── memory/
│   │   ├── ArenaAllocator.hpp
│   │   └── PoolAllocator.hpp
│   ├── core/
│   │   ├── Engine.hpp
│   │   └── Time.hpp
│   ├── rendering/
│   │   ├── Renderer.hpp
│   │   ├── Sprite.hpp
│   │   └── Texture.hpp
│   ├── physics/
│   │   ├── SpatialPartition.hpp
│   │   └── Collision.hpp
│   ├── pathfinding/
│   │   └── AStar.hpp
│   ├── audio/
│   │   └── AudioSystem.hpp
│   ├── input/
│   │   └── InputSystem.hpp
│   ├── assets/
│   │   └── AssetManager.hpp
│   └── combat/
│       └── CombatSystem.hpp
├── src/
│   └── (implementation files mirror include/)
├── external/
│   └── SDL3/
└── CMakeLists.txt
```

## Next Steps
1. Implement header skeletons with interface definitions
2. Create CMake build system
3. Implement core ECS foundation
4. Build rendering pipeline with SDL3 GPU
5. Add subsystems incrementally
6. Profile and optimize hot paths
