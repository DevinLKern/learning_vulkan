#include <engine/frontend/renderer.h>

#include <stdio.h>
#include <stdlib.h>


int main() {
    if (InitializeGraphicsLibraryFramework()) {
        return 0;
    }
    
    Renderer renderer = Renderer_Create();
    if (renderer.component_count == 0) {
        UninitializeGraphicsLibraryFramework();
        return 0;
    }

    MemoryArena arena = MemoryArena_Create(RendererContex_CalculateRequiredBytes(&renderer.context));

    if (Renderer_Initialize(&renderer, &arena)) {
        Renderer_Cleanup(&renderer);
        UninitializeGraphicsLibraryFramework();
        return 0;
    }

    Renderer_Loop(&renderer);
    
    Renderer_Cleanup(&renderer);

    MemoryArena_Free(&arena);

    UninitializeGraphicsLibraryFramework();
}