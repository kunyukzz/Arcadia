#ifndef __ENGINE_ARCHITECTURE_DOC_H__
#define __ENGINE_ARCHITECTURE_DOC_H__

/*
===============================================================================
 Arcadia Engine – Architecture Recap
===============================================================================

 Inversion of Control (IoC)
-----------------------------
- The engine controls the main loop (`application_run()`), not the game.
- The game injects behavior via a `game_t` struct: `init`, `update`, `render`, `shutdown`.
- Enables modularity and reuse. Clean separation between engine and game.

 Subsystem Communication (Engine Context - WIP)
-------------------------------------------------
- Planned: `engine_context_t` to hold references to all core systems:
    - Memory, Input, Logging, Renderer, Event, etc.
- Goal: One central access point for systems.
- Benefits:
    - Reduces tight coupling
    - Simplifies system mocking/testing
    - Makes subsystems more composable

 Messaging: Event System
--------------------------
- Subsystems communicate using the event system.
- Events are pushed and processed: `event_push()`, `event_process()`.
- Example: WINDOW_RESIZE, APP_QUIT, etc.

 Runtime Flow (Simplified)
----------------------------
main()
 └── engine_start()
      ├── platform_init()
      ├── memory/input/event_init()
      ├── game_entry_point()
      │    └── game_init()

main_loop:
  ├── platform_push()         ← input + OS events
  ├── input_update()
  ├── game.update()
  └── game.render()

Renderer (In Progress)
------------------------
- Vulkan instance, physical and logical devices, Vulkan extensions, and Vulkan debugger have been implemented.
- In progress: Creating swapchain, image views, and related Vulkan resources.
- Swapchain management, image handling, and resource synchronization are being developed to ensure proper rendering flow.
- Focus on managing framebuffers, renderpasses, and extending the rendering pipeline.
- Debugging capabilities are integrated with Vulkan's debug messenger for better error reporting.

 Future Improvements
----------------------
[ ] Implement and use `engine_context_t`
[ ] Better shutdown pipeline (e.g., game_shutdown())
[ ] Memory leak tracking per-subsystem
[ ] Allow runtime-subsystem swapping
[ ] Debug/console interface for runtime inspection
[ ] Complete swapchain and image view management
[ ] Expand renderer capabilities (advanced shaders, post-processing)
[ ] Improve Vulkan resource management (e.g., texture loading, buffer management)

===============================================================================
*/

#endif // __ENGINE_ARCHITECTURE_DOC_H__
