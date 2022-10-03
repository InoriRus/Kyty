#include "Kyty/Core/SDLSubsystem.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/MemoryAlloc.h"

#include <cstring>

// IWYU pragma: no_include <intrin.h>
// IWYU pragma: no_include "SDL_error.h"
// IWYU pragma: no_include "SDL_platform.h"
// IWYU pragma: no_include "SDL_stdinc.h"
// IWYU pragma: no_include "begin_code.h"

#include "SDL.h"

namespace Kyty::Core {

void* SDLCALL game_malloc_func(size_t size)
{
	return Core::mem_alloc(size);
}

void* SDLCALL game_calloc_func(size_t nmemb, size_t size)
{
	void* p = Core::mem_alloc(nmemb * size);
	std::memset(p, 0, nmemb * size);
	return p;
}

void* SDLCALL game_realloc_func(void* mem, size_t size)
{
	return Core::mem_realloc(mem, size);
}

void SDLCALL game_free_func(void* mem)
{
	Core::mem_free(mem);
}

KYTY_SUBSYSTEM_INIT(SDL)
{
	int sdl_alloc = SDL_GetNumAllocations();

	if (sdl_alloc != 0)
	{
		printf("warning: SDL static alloc: %d blocks\n", sdl_alloc);
	} else
	{
		if (SDL_SetMemoryFunctions(game_malloc_func, game_calloc_func, game_realloc_func, game_free_func) != 0)
		{
			KYTY_SUBSYSTEM_FAIL("%s\n", SDL_GetError());
		}
	}

#if SDL_DYNAMIC_API != 0
#error "SDL_DYNAMIC_API"
#endif

	if (SDL_Init(0) < 0)
	{
		KYTY_SUBSYSTEM_FAIL("%s\n", SDL_GetError());
	}
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(SDL) {}

KYTY_SUBSYSTEM_DESTROY(SDL)
{
	SDL_Quit();
}

} // namespace Kyty::Core
