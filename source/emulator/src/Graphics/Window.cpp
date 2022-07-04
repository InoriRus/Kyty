#include "Emulator/Graphics/Window.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Subsystems.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Timer.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Config.h"
#include "Emulator/Controller.h"
#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/Image.h"
#include "Emulator/Graphics/Utils.h"
#include "Emulator/Graphics/VideoOut.h"
#include "Emulator/Loader/SystemContent.h"
#include "Emulator/Loader/VirtualMemory.h"
#include "Emulator/Profiler.h"

#include "SDL.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_gamecontroller.h"
#include "SDL_joystick.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_mouse.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"
#include "SDL_thread.h"
#include "SDL_touch.h"
#include "SDL_video.h"
#include "SDL_vulkan.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vk_platform.h>

// IWYU pragma: no_include <intrin.h>

#ifdef KYTY_EMU_ENABLED

//#define KYTY_ENABLE_BEST_PRACTICES
#define KYTY_ENABLE_DEBUG_PRINTF
#define KYTY_DBG_INPUT

namespace Kyty::Libs::Graphics {

constexpr float FPS_AVERAGE_FRAMES = 5.0f;
constexpr float FPS_UPDATE_TIME    = 0.25f;

struct EventKeyboard
{
	bool     down;
	bool     up;
	bool     pressed;
	bool     released;
	bool     repeat;
	int      scan_code;
	int      key_code;
	uint16_t mod;
	double   timestamp_seconds;
};

struct EventMouse
{
	bool   down;
	bool   up;
	bool   left;
	bool   middle;
	bool   right;
	bool   x1;
	bool   x2;
	bool   touch;
	bool   pressed;
	bool   released;
	int    num_of_clicks;
	bool   wheel;
	int    x;
	int    y;
	bool   motion;
	int    motion_x;
	int    motion_y;
	double timestamp_seconds;
};

struct EventFinger
{
	bool   down;
	bool   up;
	bool   motion;
	int    touch_id;
	int    finger_id;
	float  x;
	float  y;
	float  dx;
	float  dy;
	float  pressure;
	double timestamp_seconds;
};

struct EventController
{
	int    id;
	int    button;
	int    axis_id;
	int    axis_value;
	bool   down;
	bool   up;
	bool   added;
	bool   removed;
	bool   remapped;
	bool   axis;
	bool   pressed;
	bool   released;
	double timestamp_seconds;
};

enum class DisplayOrientation
{
	Unknown,          /* The display orientation can't be determined */
	Landscape,        /* The display is in landscape mode, with the right side up, relative to portrait mode */
	LandscapeFlipped, /* The display is in landscape mode, with the left side up, relative to portrait mode */
	Portrait,         /* The display is in portrait mode */
	PortraitFlipped,  /* The display is in portrait mode, upside down */

	DisplayEventOrientation = 0xF0
};

struct EventDisplay
{
	DisplayOrientation orientation;
};

constexpr uint32_t KYTY_SDL_BUTTON_LMASK  = SDL_BUTTON_LMASK;  // NOLINT(hicpp-signed-bitwise)
constexpr uint32_t KYTY_SDL_BUTTON_MMASK  = SDL_BUTTON_MMASK;  // NOLINT(hicpp-signed-bitwise)
constexpr uint32_t KYTY_SDL_BUTTON_RMASK  = SDL_BUTTON_RMASK;  // NOLINT(hicpp-signed-bitwise)
constexpr uint32_t KYTY_SDL_BUTTON_X1MASK = SDL_BUTTON_X1MASK; // NOLINT(hicpp-signed-bitwise)
constexpr uint32_t KYTY_SDL_BUTTON_X2MASK = SDL_BUTTON_X2MASK; // NOLINT(hicpp-signed-bitwise)

// struct GraphicContext;

struct GameApi
{
	bool (*init)(GameApi* game, const Core::Timer& timer, void* data)            = nullptr;
	bool (*render_and_update)(GameApi* game, const Core::Timer& timer)           = nullptr;
	bool (*close)(GameApi* game)                                                 = nullptr;
	void (*event_quit)(GameApi* game)                                            = nullptr;
	void (*event_terminate)(GameApi* game)                                       = nullptr;
	void (*event_keyboard)(GameApi* game, const EventKeyboard* key)              = nullptr;
	void (*event_mouse)(GameApi* game, const EventMouse* mb)                     = nullptr;
	void (*event_finger)(GameApi* game, const EventFinger* f)                    = nullptr;
	void (*event_controller)(GameApi* game, const EventController* f)            = nullptr;
	void (*event_display)(GameApi* game, const EventDisplay* d)                  = nullptr;
	void (*event_low_memory)(GameApi* game)                                      = nullptr;
	void (*event_will_enter_background)(GameApi* game)                           = nullptr;
	void (*event_did_enter_background)(GameApi* game)                            = nullptr;
	void (*event_will_enter_foreground)(GameApi* game)                           = nullptr;
	void (*event_did_enter_foreground)(GameApi* game)                            = nullptr;
	void (*event_resize)(GameApi* game, uint32_t new_width, uint32_t new_height) = nullptr;
	void (*show_window)(GameApi* game, const Core::Timer& timer)                 = nullptr;
	bool (*need_exit)(GameApi* game)                                             = nullptr;
	bool (*is_paused)(GameApi* game)                                             = nullptr;

	int (*poll_event)(GameApi* game)                    = nullptr;
	int (*wait_event)(GameApi* game)                    = nullptr;
	void (*process_event)(GameApi* game, double time_s) = nullptr;

	void* data1 = nullptr;
	void* data2 = nullptr;

	bool     m_game_need_exit        = {false};
	bool     m_game_is_paused        = {false};
	uint32_t m_screen_width          = {0};
	uint32_t m_screen_height         = {0};
	double   m_current_time_seconds  = {0.0};
	double   m_previous_time_seconds = {0.0};
	int      m_update_num            = {0};
	int      m_frame_num             = {0};
	double   m_update_time_seconds   = {0.0};
	double   m_current_fps           = {0.0};
	int      m_max_updates_per_frame = {4};
	double   m_update_fixed_time     = 1.0 / 60.0;
	int      m_fps_frames_num        = {0};
	double   m_fps_start_time        = {0};
};

struct GameApiPrivateStruct
{
	GameApiPrivateStruct() = default;

	Core::Mutex     mutex;
	int             skip_frames = 0;
	GraphicContext* ctx         = nullptr;
};

// void game_main_loop(GameApi* game);

GameApi* game_create_api();
void     game_delete_api(GameApi* api);
int      game_poll_event(GameApi* game);
int      game_wait_event(GameApi* game);
void     game_process_event(GameApi* game, double time_s);

struct VulkanExtensions
{
	bool enable_validation_layers = false;

	Vector<const char*>           required_extensions;
	Vector<VkExtensionProperties> available_extensions;
	Vector<const char*>           required_layers;
	Vector<VkLayerProperties>     available_layers;
};

struct SurfaceCapabilities
{
	VkSurfaceCapabilitiesKHR   capabilities {};
	Vector<VkSurfaceFormatKHR> formats;
	Vector<VkPresentModeKHR>   present_modes;
	bool                       format_srgb_bgra32  = false;
	bool                       format_unorm_bgra32 = false;
};

struct WindowContext
{
	GraphicContext       graphic_ctx;
	VulkanSwapchain*     swapchain            = nullptr;
	SDL_Window*          window               = nullptr;
	bool                 window_hidden        = true;
	VkSurfaceKHR         surface              = nullptr;
	SurfaceCapabilities* surface_capabilities = nullptr;
	GameApi*             game                 = nullptr;

	char device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE] = {0};
	char processor_name[64]                            = {0};

	Core::Mutex   mutex;
	bool          graphic_initialized = false;
	Core::CondVar graphic_initialized_condvar;
};

static WindowContext* g_window_ctx = nullptr;

constexpr const char* KYTY_SDL_WINDOW_CAPTION = "Game";
// constexpr uint32_t    KYTY_SDL_WINDOW_FLAGS       = (static_cast<uint32_t>(SDL_WINDOW_HIDDEN) |
// static_cast<uint32_t>(SDL_WINDOW_OPENGL));
constexpr uint32_t KYTY_SDL_WINDOW_FLAGS       = (static_cast<uint32_t>(SDL_WINDOW_HIDDEN) | static_cast<uint32_t>(SDL_WINDOW_VULKAN));
constexpr int      KYTY_SDL_WINDOWPOS_CENTERED = SDL_WINDOWPOS_CENTERED; /*NOLINT(hicpp-signed-bitwise)*/

static int game_sdl_event_filter(void* /*userdata*/, SDL_Event* /*event*/)
{
	//	game_api_private_t *p = (game_api_private_t*)userdata;
	//
	//	if (event->type == SDL_WINDOWEVENT)
	//	{
	//		if (event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
	//		{
	//			p->mutex.Lock();
	//			{
	//				//p->resize_events_num++;
	//				//printf("game_sdl_event_filter\n");
	//				p->skip_frames++;
	//			}
	//			p->mutex.Unlock();
	//		}
	//	}

	return 0;
}

static void CalcFrameTime(GameApi* game, double game_time_s)
{
	game->m_previous_time_seconds = game->m_current_time_seconds;
	game->m_current_time_seconds  = game_time_s;

	game->m_frame_num++;

	int fps_model = 1;

	if (fps_model == 1)
	{
		game->m_fps_frames_num++;
		if (game->m_current_time_seconds - game->m_fps_start_time > FPS_UPDATE_TIME)
		{
			game->m_current_fps    = static_cast<double>(game->m_fps_frames_num) / (game->m_current_time_seconds - game->m_fps_start_time);
			game->m_fps_frames_num = 0;
			game->m_fps_start_time = game->m_current_time_seconds;
		}
	} else
	{
		game->m_current_fps = (1.0f / (game->m_current_time_seconds - game->m_previous_time_seconds)) * (1.0f / FPS_AVERAGE_FRAMES) +
		                      game->m_current_fps * (1.0f - (1.0f / FPS_AVERAGE_FRAMES));
	}
}

static bool Init(GameApi* /*game*/)
{
	return true;
}
static bool Update(GameApi* /*game*/)
{
	return true;
}
static bool Render(GameApi* /*game*/)
{
	return true;
}
static bool Close(GameApi* /*game*/)
{
	return true;
}
static void SetPause(GameApi* game, bool flag)
{
	printf("Pause: %s\n", flag ? "true" : "false");

	game->m_game_is_paused = flag;
}

static bool RenderAndUpdate(GameApi* game)
{
	static double lag = 0.0;

	lag += game->m_current_time_seconds - game->m_previous_time_seconds;

	int num = 0;

	bool ok = true;

	while (lag >= game->m_update_fixed_time)
	{
		if (num < game->m_max_updates_per_frame)
		{
			ok = ok && Update(game);

			game->m_update_num++;
			num++;
			game->m_update_time_seconds = game->m_update_num * game->m_update_fixed_time;
		}

		lag -= game->m_update_fixed_time;
	}

	ok = ok && Render(game);

	return ok;
}

bool game_init(GameApi* game, const Core::Timer& timer, void* data)
{
	EXIT_IF(game == nullptr);
	EXIT_IF(data == nullptr);
	EXIT_IF(game->data1 || game->data2);

	auto* ctx = static_cast<GraphicContext*>(data);

	EXIT_IF(ctx->screen_width == 0 || ctx->screen_height == 0);

	auto* pdata = new GameApiPrivateStruct;
	pdata->ctx  = ctx;

	game->data1 = pdata;
	game->data2 = new SDL_Event;

	SDL_AddEventWatch(game_sdl_event_filter, game->data1);

	game->m_screen_width  = ctx->screen_width;
	game->m_screen_height = ctx->screen_height;

	// SDL_ShowWindow(ctx->window);

	CalcFrameTime(game, timer.GetTimeS());

	return Init(game);
}

bool game_render_and_update(GameApi* game, const Core::Timer& /*timer*/)
{
	return RenderAndUpdate(game);
}

bool game_close(GameApi* game)
{
	EXIT_IF(!game);

	EXIT_IF(!game->data1 || !game->data2);

	delete (static_cast<GameApiPrivateStruct*>(game->data1));
	delete (static_cast<SDL_Event*>(game->data2));

	return Close(game);
}

void game_show_window(GameApi* game, const Core::Timer& timer)
{
	EXIT_IF(!game);

	auto* p = static_cast<GameApiPrivateStruct*>(game->data1);

	EXIT_IF(!p);

	p->mutex.Lock();
	{
		if (p->skip_frames > 0)
		{
			p->skip_frames--;
			printf("skip frame %d\n", p->skip_frames);
		} else
		{
			VideoOut::VideoOutBeginVblank();
			if (VideoOut::VideoOutFlipWindow(100000))
			{
				CalcFrameTime(game, timer.GetTimeS());
			}
			VideoOut::VideoOutEndVblank();
		}
	}
	p->mutex.Unlock();
}

void game_event_quit(GameApi* game)
{
	printf("Event: quit\n");

	game->m_game_need_exit = true;
}

void game_event_terminate(GameApi* game)
{
	printf("Event: terminate\n");

	game->m_game_need_exit = true;
}

void game_event_keyboard(GameApi* game, const EventKeyboard* key)
{
#ifdef KYTY_DBG_INPUT
	printf("Key: time = %.04f, %s%s, %s%s, %s, scan = %d, key = %d, mod = %04" PRIx16 "\n", key->timestamp_seconds,
	       (key->down ? "down" : ""), (key->up ? "up" : ""), (key->pressed ? "pressed" : ""), (key->released ? "released" : ""),
	       (key->repeat ? "repeat" : ""), key->scan_code, key->key_code, key->mod);
#endif

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
	if (key->down && key->key_code == SDLK_ESCAPE)
	{
		game->m_game_need_exit = true;
	}

	if (key->down && key->key_code == SDLK_SPACE)
	{
		SetPause(game, !game->m_game_is_paused);
	}
#endif
}

void game_event_mouse([[maybe_unused]] GameApi* game, [[maybe_unused]] const EventMouse* mb)
{
#ifdef KYTY_DBG_INPUT
	if (mb->wheel)
	{
		printf("Mouse wheel: time = %.04f, %s[%d, %d]\n", mb->timestamp_seconds, (mb->touch ? "touch, " : ""), mb->x, mb->y);
	} else if (mb->motion)
	{
		printf("Mouse motion: time = %.04f, %s%s%s%s%s%s, [%d, %d], (%d, %d)\n", mb->timestamp_seconds, (mb->left ? "left" : ""),
		       (mb->middle ? "middle" : ""), (mb->right ? "right" : ""), (mb->x1 ? "x1" : ""), (mb->x2 ? "x2" : ""),
		       (mb->touch ? "_touch" : ""), mb->x, mb->y, mb->motion_x, mb->motion_y);
	} else
	{
		printf("Mouse click: time = %.04f, %d, %s%s%s%s%s%s, %s%s, %s%s, [%d, %d]\n", mb->timestamp_seconds, mb->num_of_clicks,
		       (mb->left ? "left" : ""), (mb->middle ? "middle" : ""), (mb->right ? "right" : ""), (mb->x1 ? "x1" : ""),
		       (mb->x2 ? "x2" : ""), (mb->touch ? "_touch" : ""), (mb->down ? "down" : ""), (mb->up ? "up" : ""),
		       (mb->pressed ? "pressed" : ""), (mb->released ? "released" : ""), mb->x, mb->y);
	}
#endif
}

void game_event_finger([[maybe_unused]] GameApi* game, [[maybe_unused]] const EventFinger* f)
{
#ifdef KYTY_DBG_INPUT
	if (f->motion)
	{
		printf("Finger motion: time = %.04f, %d, %d, (x,y) = [%f, %f], (dx,dy) = [%f, %f], pressure = %f\n", f->timestamp_seconds,
		       f->touch_id, f->finger_id, f->x, f->y, f->dx, f->dy, f->pressure);
	} else
	{
		printf("Finger press: time = %.04f, %d, %d, %s%s, (x,y) = [%f, %f], (dx,dy) = [%f, %f], pressure = %f\n", f->timestamp_seconds,
		       f->touch_id, f->finger_id, (f->down ? "down" : ""), (f->up ? "up" : ""), f->x, f->y, f->dx, f->dy, f->pressure);
	}
#endif
}

// static int controller_get_axis(int min, int max, int value)
//{
//	int v = (255 * (value - min)) / (max - min);
//	return (v < 0 ? 0 : (v > 255 ? 255 : v));
//}

void game_event_controller([[maybe_unused]] GameApi* game, [[maybe_unused]] const EventController* f)
{
	EXIT_NOT_IMPLEMENTED(f->remapped);

#ifdef KYTY_DBG_INPUT
	if (f->added || f->removed)
	{
		printf("Controller %s: %d, time = %.04f\n", (f->added ? "added" : "removed"), f->id, f->timestamp_seconds);
	} else if (f->axis)
	{
		printf("Controller axis: %d, axis = %d, value = %d, time = %.04f\n", f->id, f->axis_id, f->axis_value, f->timestamp_seconds);
	} else
	{
		printf("Controller button: "
		       "%d, %s%s, %s%s, button = %d, time = %.04f\n",
		       f->id, (f->down ? "down" : ""), (f->up ? "up" : ""), (f->pressed ? "pressed" : ""), (f->released ? "released" : ""),
		       f->button, f->timestamp_seconds);
	}
#endif

	if (f->added)
	{
		auto* pad = SDL_GameControllerOpen(f->id);
		EXIT_NOT_IMPLEMENTED(pad == nullptr);
		int id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));
		Controller::ControllerConnect(id);
	}

	if (f->removed)
	{
		Controller::ControllerDisconnect(f->id);
		SDL_GameControllerClose(SDL_GameControllerFromInstanceID(f->id));
	}

	if (f->down || f->up)
	{
		uint32_t button = 0;
		switch (f->button)
		{
			case SDL_CONTROLLER_BUTTON_A: button = Controller::PAD_BUTTON_CROSS; break;
			case SDL_CONTROLLER_BUTTON_B: button = Controller::PAD_BUTTON_CIRCLE; break;
			case SDL_CONTROLLER_BUTTON_X: button = Controller::PAD_BUTTON_SQUARE; break;
			case SDL_CONTROLLER_BUTTON_Y: button = Controller::PAD_BUTTON_TRIANGLE; break;
			case SDL_CONTROLLER_BUTTON_BACK: button = Controller::PAD_BUTTON_TOUCH_PAD; break;
			case SDL_CONTROLLER_BUTTON_GUIDE: break;
			case SDL_CONTROLLER_BUTTON_START: button = Controller::PAD_BUTTON_OPTIONS; break;
			case SDL_CONTROLLER_BUTTON_LEFTSTICK: button = Controller::PAD_BUTTON_L3; break;
			case SDL_CONTROLLER_BUTTON_RIGHTSTICK: button = Controller::PAD_BUTTON_R3; break;
			case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: button = Controller::PAD_BUTTON_L1; break;
			case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: button = Controller::PAD_BUTTON_R1; break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP: button = Controller::PAD_BUTTON_UP; break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN: button = Controller::PAD_BUTTON_DOWN; break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT: button = Controller::PAD_BUTTON_LEFT; break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: button = Controller::PAD_BUTTON_RIGHT; break;
			default: break;
		}
		if (button != 0)
		{
			Controller::ControllerButton(f->id, button, f->down);
		}
	}

	if (f->axis)
	{
		int value = -1;
		if (f->axis_id == SDL_CONTROLLER_AXIS_TRIGGERLEFT || f->axis_id == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
		{
			value = Controller::controller_get_axis(0, SDL_JOYSTICK_AXIS_MAX, f->axis_value);
		} else
		{
			value = Controller::controller_get_axis(SDL_JOYSTICK_AXIS_MIN, SDL_JOYSTICK_AXIS_MAX, f->axis_value);
		}

		Controller::Axis axis = Controller::Axis::AxisMax;
		switch (f->axis_id)
		{
			case SDL_CONTROLLER_AXIS_LEFTX: axis = Controller::Axis::LeftX; break;
			case SDL_CONTROLLER_AXIS_LEFTY: axis = Controller::Axis::LeftY; break;
			case SDL_CONTROLLER_AXIS_RIGHTX: axis = Controller::Axis::RightX; break;
			case SDL_CONTROLLER_AXIS_RIGHTY: axis = Controller::Axis::RightY; break;
			case SDL_CONTROLLER_AXIS_TRIGGERLEFT: axis = Controller::Axis::TriggerLeft; break;
			case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: axis = Controller::Axis::TriggerRight; break;
		}

		if (axis != Controller::Axis::AxisMax)
		{
			Controller::ControllerAxis(f->id, axis, value);
		}
	}
}

void game_event_display([[maybe_unused]] GameApi* game, [[maybe_unused]] const EventDisplay* d)
{
	auto* p   = static_cast<GameApiPrivateStruct*>(game->data1);
	auto* ctx = static_cast<GraphicContext*>(p->ctx);

	p->mutex.Lock();
	game->m_screen_width  = ctx->screen_width;
	game->m_screen_height = ctx->screen_height;
	p->mutex.Unlock();
}

void game_event_low_memory(GameApi* /*game*/)
{
	printf("Event: low_memory\n");
}

void game_event_will_enter_background(GameApi* game)
{
	printf("Event: will_enter_background\n");

	SetPause(game, true);
}

void game_event_did_enter_background(GameApi* /*game*/)
{
	printf("Event: did_enter_background\n");
}

void game_event_will_enter_foreground(GameApi* /*game*/)
{
	printf("Event: will_enter_foreground\n");
}

void game_event_did_enter_foreground(GameApi* game)
{
	printf("Event: did_enter_foreground\n");

	SetPause(game, false);
}

bool game_need_exit(GameApi* game)
{
	return game->m_game_need_exit;
}

bool game_is_paused(GameApi* game)
{
	return game->m_game_is_paused;
}

void game_event_resize(GameApi* game, uint32_t new_width, uint32_t new_height)
{
	EXIT_IF(new_width == 0 || new_height == 0);
	EXIT_IF(!game);

	auto* p = static_cast<GameApiPrivateStruct*>(game->data1);
	EXIT_IF(p == nullptr);

	auto* ctx = static_cast<GraphicContext*>(p->ctx);
	EXIT_IF(ctx == nullptr);

	p->mutex.Lock();
	{
		p->skip_frames++;
		ctx->screen_width  = new_width;
		ctx->screen_height = new_height;

		game->m_screen_width  = ctx->screen_width;
		game->m_screen_height = ctx->screen_height;
	}
	p->mutex.Unlock();
}

static void process_window_event(GameApi* game, SDL_WindowEvent window)
{
	switch (window.event)
	{
		case SDL_WINDOWEVENT_SHOWN: printf("Window %" PRIu32 " shown\n", window.windowID); break;

		case SDL_WINDOWEVENT_HIDDEN: printf("Window %" PRIu32 " hidden\n", window.windowID); break;

		case SDL_WINDOWEVENT_EXPOSED: printf("Window %" PRIu32 " exposed\n", window.windowID); break;

		case SDL_WINDOWEVENT_MOVED:
			printf("Window %" PRIu32 " moved to %" PRId32 ",%" PRId32 "\n", window.windowID, window.data1, window.data2);
			break;

		case SDL_WINDOWEVENT_RESIZED:
			printf("Window %" PRIu32 " resized to %" PRId32 "x%" PRId32 "\n", window.windowID, window.data1, window.data2);

			if (game->event_resize != nullptr)
			{
				printf("m: %d\n", static_cast<int>(SDL_ThreadID()));
				//				dbg_check();
				game->event_resize(game, window.data1, window.data2);
			}

			break;

		case SDL_WINDOWEVENT_SIZE_CHANGED:
			printf("Window %" PRIu32 " size changed to %" PRId32 "x%" PRId32 "\n", window.windowID, window.data1, window.data2);

			if (game->event_resize != nullptr)
			{
				printf("m: %d\n", static_cast<int>(SDL_ThreadID()));
				//				dbg_check();
				game->event_resize(game, window.data1, window.data2);
			}

			break;

		case SDL_WINDOWEVENT_MINIMIZED: printf("Window %" PRIu32 " minimized\n", window.windowID); break;
		case SDL_WINDOWEVENT_MAXIMIZED: printf("Window %" PRIu32 " maximized\n", window.windowID); break;
		case SDL_WINDOWEVENT_RESTORED: printf("Window %" PRIu32 " restored\n", window.windowID); break;
		case SDL_WINDOWEVENT_ENTER: printf("Mouse entered window %" PRIu32 "\n", window.windowID); break;
		case SDL_WINDOWEVENT_LEAVE: printf("Mouse left window %" PRIu32 "\n", window.windowID); break;
		case SDL_WINDOWEVENT_FOCUS_GAINED: printf("Window %" PRIu32 " gained keyboard focus\n", window.windowID); break;
		case SDL_WINDOWEVENT_FOCUS_LOST: printf("Window %" PRIu32 " lost keyboard focus\n", window.windowID); break;
		case SDL_WINDOWEVENT_CLOSE: printf("Window %" PRIu32 " closed\n", window.windowID); break;
		default: printf("Window %" PRIu32 " got unknown event %" PRIu8 "\n", window.windowID, window.event); break;
	}
}

static void process_display_event(GameApi* game, SDL_DisplayEvent display)
{
	bool sdl = false;

	switch (display.event)
	{
		case SDL_DISPLAYEVENT_ORIENTATION: sdl = true; // @suppress("No break at end of case")
		case static_cast<Uint8>(DisplayOrientation::DisplayEventOrientation):
		{
			printf("Display %" PRIu32 "[%s] changed orientation to %d - ", display.display, sdl ? "SDL" : "Kyty",
			       static_cast<int>(display.data1));

			EventDisplay d {};
			d.orientation = DisplayOrientation::Unknown;

			switch (display.data1)
			{
				case SDL_ORIENTATION_UNKNOWN: printf("UNKNOWN\n"); break;
				case SDL_ORIENTATION_LANDSCAPE:
					printf("LANDSCAPE\n");
					d.orientation = DisplayOrientation::Landscape;
					break;
				case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
					printf("LANDSCAPE_FLIPPED\n");
					d.orientation = DisplayOrientation::LandscapeFlipped;
					break;
				case SDL_ORIENTATION_PORTRAIT:
					printf("PORTRAIT\n");
					d.orientation = DisplayOrientation::Portrait;
					break;
				case SDL_ORIENTATION_PORTRAIT_FLIPPED:
					d.orientation = DisplayOrientation::PortraitFlipped;
					printf("PORTRAIT_FLIPPED\n");
					break;
				default: printf("???\n");
			}

			if (!sdl)
			{
				if (game->event_display != nullptr)
				{
					//				dbg_check();
					game->event_display(game, &d);
				}
			}

			break;
		}
		default: printf("Display %" PRIu32 " got unknown event 0x%" PRIx8 "\n", display.display, display.event); break;
	}
}

int game_poll_event(GameApi* game)
{
	EXIT_IF(!game);

	auto* event = static_cast<SDL_Event*>(game->data2);

	EXIT_IF(!event);

	// SDL_JoystickUpdate();

	return SDL_PollEvent(event);
}

int game_wait_event(GameApi* game)
{
	EXIT_IF(!game);

	auto* event = static_cast<SDL_Event*>(game->data2);

	EXIT_IF(!event);

	return SDL_WaitEvent(event);
}

void game_process_event(GameApi* game, double time_s)
{
	EXIT_IF(!game);

	auto* event = static_cast<SDL_Event*>(game->data2);

	EXIT_IF(!event);

	// printf("Event: 0x%04" PRIx32 "\n", event.type);

	EXIT_IF(SDL_GetEventState(SDL_DISPLAYEVENT) != SDL_ENABLE);

	switch (event->type)
	{
		case SDL_QUIT:
			if (game->event_quit != nullptr)
			{
				game->event_quit(game);
			}
			break;

		case SDL_APP_TERMINATING:
			if (game->event_terminate != nullptr)
			{
				game->event_terminate(game);
			}
			break;

		case SDL_APP_LOWMEMORY:
			if (game->event_low_memory != nullptr)
			{
				game->event_low_memory(game);
			}
			break;

		case SDL_APP_WILLENTERBACKGROUND:
			if (game->event_will_enter_background != nullptr)
			{
				game->event_will_enter_background(game);
			}
			break;

		case SDL_APP_DIDENTERBACKGROUND:
			if (game->event_did_enter_background != nullptr)
			{
				game->event_did_enter_background(game);
			}
			break;

		case SDL_APP_WILLENTERFOREGROUND:
			if (game->event_will_enter_foreground != nullptr)
			{
				game->event_will_enter_foreground(game);
			}
			break;

		case SDL_APP_DIDENTERFOREGROUND:
			if (game->event_did_enter_foreground != nullptr)
			{
				game->event_did_enter_foreground(game);
			}
			break;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			EventKeyboard key {};

			key.down              = (event->type == SDL_KEYDOWN);
			key.up                = (event->type == SDL_KEYUP);
			key.pressed           = (event->key.state == SDL_PRESSED);
			key.released          = (event->key.state == SDL_RELEASED);
			key.repeat            = (event->key.repeat != 0u);
			key.scan_code         = event->key.keysym.scancode;
			key.key_code          = event->key.keysym.sym;
			key.mod               = event->key.keysym.mod;
			key.timestamp_seconds = time_s;

			if (game->event_keyboard != nullptr)
			{
				game->event_keyboard(game, &key);
			}

			break;
		}

		case SDL_WINDOWEVENT: process_window_event(game, event->window); break;

		case SDL_DISPLAYEVENT: process_display_event(game, event->display); break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			EventMouse mb {};

			// printf("event.button.which = %" PRIu32"\n", event.button.which);

			mb.down              = (event->button.type == SDL_MOUSEBUTTONDOWN);
			mb.up                = (event->button.type == SDL_MOUSEBUTTONUP);
			mb.left              = (event->button.button == SDL_BUTTON_LEFT);
			mb.middle            = (event->button.button == SDL_BUTTON_MIDDLE);
			mb.right             = (event->button.button == SDL_BUTTON_RIGHT);
			mb.x1                = (event->button.button == SDL_BUTTON_X1);
			mb.x2                = (event->button.button == SDL_BUTTON_X2);
			mb.touch             = (event->button.which == SDL_TOUCH_MOUSEID);
			mb.pressed           = (event->button.state == SDL_PRESSED);
			mb.released          = (event->button.state == SDL_RELEASED);
			mb.num_of_clicks     = event->button.clicks;
			mb.wheel             = false;
			mb.x                 = event->button.x;
			mb.y                 = event->button.y;
			mb.motion            = false;
			mb.motion_x          = 0;
			mb.motion_y          = 0;
			mb.timestamp_seconds = time_s;

			if (game->event_mouse != nullptr)
			{
				game->event_mouse(game, &mb);
			}

			break;
		}

		case SDL_MOUSEWHEEL:
		{
			EventMouse mb {};

			mb.down              = false;
			mb.up                = false;
			mb.left              = false;
			mb.middle            = false;
			mb.right             = false;
			mb.x1                = false;
			mb.x2                = false;
			mb.touch             = (event->wheel.which == SDL_TOUCH_MOUSEID);
			mb.pressed           = false;
			mb.released          = false;
			mb.num_of_clicks     = 0;
			mb.wheel             = true;
			mb.x                 = event->wheel.x;
			mb.y                 = event->wheel.y;
			mb.motion            = false;
			mb.motion_x          = 0;
			mb.motion_y          = 0;
			mb.timestamp_seconds = time_s;

			if (game->event_mouse != nullptr)
			{
				game->event_mouse(game, &mb);
			}

			break;
		}

		case SDL_MOUSEMOTION:
		{
			EventMouse mb {};

			mb.down              = false;
			mb.up                = false;
			mb.left              = ((event->motion.state & KYTY_SDL_BUTTON_LMASK) != 0u);
			mb.middle            = ((event->motion.state & KYTY_SDL_BUTTON_MMASK) != 0u);
			mb.right             = ((event->motion.state & KYTY_SDL_BUTTON_RMASK) != 0u);
			mb.x1                = ((event->motion.state & KYTY_SDL_BUTTON_X1MASK) != 0u);
			mb.x2                = ((event->motion.state & KYTY_SDL_BUTTON_X2MASK) != 0u);
			mb.touch             = (event->motion.which == SDL_TOUCH_MOUSEID);
			mb.pressed           = false;
			mb.released          = false;
			mb.num_of_clicks     = 0;
			mb.wheel             = false;
			mb.x                 = event->motion.x;
			mb.y                 = event->motion.y;
			mb.motion            = true;
			mb.motion_x          = event->motion.xrel;
			mb.motion_y          = event->motion.yrel;
			mb.timestamp_seconds = time_s;

			if (game->event_mouse != nullptr)
			{
				game->event_mouse(game, &mb);
			}

			break;
		}

		case SDL_FINGERMOTION:
		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		{
			EventFinger f {};

			f.down              = (event->tfinger.type == SDL_FINGERDOWN);
			f.up                = (event->tfinger.type == SDL_FINGERUP);
			f.motion            = (event->tfinger.type == SDL_FINGERMOTION);
			f.finger_id         = static_cast<int>(event->tfinger.fingerId);
			f.touch_id          = static_cast<int>(event->tfinger.touchId);
			f.x                 = event->tfinger.x;
			f.y                 = event->tfinger.y;
			f.dx                = event->tfinger.dx;
			f.dy                = event->tfinger.dy;
			f.pressure          = event->tfinger.pressure;
			f.timestamp_seconds = time_s;

			if (game->event_finger != nullptr)
			{
				game->event_finger(game, &f);
			}

			break;
		}

			//		case SDL_JOYAXISMOTION:
			//		case SDL_JOYBALLMOTION:
			//		case SDL_JOYHATMOTION:
			//		case SDL_JOYBUTTONDOWN:
			//		case SDL_JOYBUTTONUP:
			//		case SDL_JOYDEVICEADDED:
			//		case SDL_JOYDEVICEREMOVED:
			//		{
			//			EXIT("joystick event: %d\n", static_cast<int>(event->type));
			//			break;
			//		}

		case SDL_CONTROLLERAXISMOTION:
		{
			EventController c {};

			c.id                = event->caxis.which;
			c.button            = SDL_CONTROLLER_BUTTON_INVALID;
			c.axis_id           = event->caxis.axis;
			c.axis_value        = event->caxis.value;
			c.down              = false;
			c.up                = false;
			c.added             = false;
			c.removed           = false;
			c.remapped          = false;
			c.axis              = true;
			c.pressed           = false;
			c.released          = false;
			c.timestamp_seconds = time_s;

			if (game->event_controller != nullptr)
			{
				game->event_controller(game, &c);
			}

			break;
		}

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
		{
			EventController c {};

			c.id                = event->cbutton.which;
			c.button            = event->cbutton.button;
			c.axis_id           = SDL_CONTROLLER_AXIS_INVALID;
			c.axis_value        = 0;
			c.down              = (event->cbutton.type == SDL_CONTROLLERBUTTONDOWN);
			c.up                = (event->cbutton.type == SDL_CONTROLLERBUTTONUP);
			c.added             = false;
			c.removed           = false;
			c.remapped          = false;
			c.axis              = false;
			c.pressed           = (event->cbutton.state == SDL_PRESSED);
			c.released          = (event->cbutton.state == SDL_RELEASED);
			c.timestamp_seconds = time_s;

			if (game->event_controller != nullptr)
			{
				game->event_controller(game, &c);
			}

			break;
		}

		case SDL_CONTROLLERDEVICEADDED:
		case SDL_CONTROLLERDEVICEREMOVED:
		case SDL_CONTROLLERDEVICEREMAPPED:
		{
			EventController c {};

			c.id                = event->cdevice.which;
			c.button            = SDL_CONTROLLER_BUTTON_INVALID;
			c.axis_id           = SDL_CONTROLLER_AXIS_INVALID;
			c.axis_value        = 0;
			c.down              = false;
			c.up                = false;
			c.added             = (event->cdevice.type == SDL_CONTROLLERDEVICEADDED);
			c.removed           = (event->cdevice.type == SDL_CONTROLLERDEVICEREMOVED);
			c.remapped          = (event->cdevice.type == SDL_CONTROLLERDEVICEREMAPPED);
			c.axis              = false;
			c.pressed           = false;
			c.released          = false;
			c.timestamp_seconds = time_s;

			if (game->event_controller != nullptr)
			{
				game->event_controller(game, &c);
			}

			break;
		}
	}
}

GameApi* game_create_api()
{
	auto* api = new GameApi;

	// memset(api, 0, sizeof(GameApi));

	api->init              = game_init;
	api->render_and_update = game_render_and_update;
	api->close             = game_close;

	api->event_quit                  = game_event_quit;
	api->event_terminate             = game_event_terminate;
	api->event_keyboard              = game_event_keyboard;
	api->event_mouse                 = game_event_mouse;
	api->event_finger                = game_event_finger;
	api->event_controller            = game_event_controller;
	api->event_display               = game_event_display;
	api->event_low_memory            = game_event_low_memory;
	api->event_will_enter_background = game_event_will_enter_background;
	api->event_did_enter_background  = game_event_did_enter_background;
	api->event_will_enter_foreground = game_event_will_enter_foreground;
	api->event_did_enter_foreground  = game_event_did_enter_foreground;
	api->event_resize                = game_event_resize;

	api->show_window = game_show_window;

	api->need_exit = game_need_exit;
	api->is_paused = game_is_paused;

	api->poll_event    = game_poll_event;
	api->wait_event    = game_wait_event;
	api->process_event = game_process_event;

	return api;
}

void game_delete_api(GameApi* api)
{
	Delete(api);
}

void game_main_loop(GameApi* game, void* data)
{
	bool need_exit = false;

	Core::Timer timer;
	timer.Start();

	if (!game->init(game, timer, data))
	{
		need_exit = true;
	}

	for (;;)
	{
		if (need_exit)
		{
			break;
		}

		if (game->poll_event(game) != 0)
		{
			if (game->process_event != nullptr)
			{
				game->process_event(game, timer.GetTimeS());
			}
			continue;
		}

		if (game->is_paused(game))
		{
			if (!timer.IsPaused())
			{
				timer.Pause();
			}

			game->wait_event(game);

			if (game->process_event != nullptr)
			{
				game->process_event(game, timer.GetTimeS());
			}
			need_exit = game->need_exit(game);
			continue;
		}

		need_exit = game->need_exit(game);

		if (game->is_paused(game))
		{
			if (!timer.IsPaused())
			{
				timer.Pause();
			}
		} else
		{
			if (timer.IsPaused())
			{
				timer.Resume();
			}

			if (!need_exit)
			{
				need_exit = !game->render_and_update(game, timer);
			}

			if (!need_exit)
			{
				//				dbg_inc();
				game->show_window(game, timer);
			}
		}
	}

	game->close(game);
}

static void WindowCreate(WindowContext* ctx)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(ctx->window != nullptr);
	EXIT_IF(ctx->graphic_ctx.screen_width == 0);
	EXIT_IF(ctx->graphic_ctx.screen_height == 0);

	int width  = static_cast<int>(ctx->graphic_ctx.screen_width);
	int height = static_cast<int>(ctx->graphic_ctx.screen_height);

	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
	{
		EXIT("%s\n", SDL_GetError());
	}

	// SDL_JoystickEventState(SDL_ENABLE);
	// SDL_JoystickOpen(device_index)

	printf("WindowCreate(): width = %d, height = %d\n", width, height);

	ctx->window = SDL_CreateWindow(KYTY_SDL_WINDOW_CAPTION, KYTY_SDL_WINDOWPOS_CENTERED, KYTY_SDL_WINDOWPOS_CENTERED, width, height,
	                               KYTY_SDL_WINDOW_FLAGS);

	ctx->window_hidden = true;

	if (ctx->window == nullptr)
	{
		EXIT("%s\n", SDL_GetError());
	}

	SDL_SetWindowResizable(ctx->window, SDL_FALSE);
}

static void VulkanGetSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface, SurfaceCapabilities* r)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &r->capabilities);

	uint32_t formats_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_count, nullptr);

	EXIT_NOT_IMPLEMENTED(formats_count == 0);

	r->formats = Vector<VkSurfaceFormatKHR>(formats_count); // @suppress("Ambiguous problem")
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_count, r->formats.GetData());

	uint32_t present_modes_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, nullptr);

	EXIT_NOT_IMPLEMENTED(present_modes_count == 0);

	r->present_modes = Vector<VkPresentModeKHR>(present_modes_count); // @suppress("Ambiguous problem")
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, r->present_modes.GetData());

	r->format_srgb_bgra32 = false;
	for (const auto& f: r->formats)
	{
		if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			r->format_srgb_bgra32 = true;
			break;
		}
		if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			r->format_unorm_bgra32 = true;
			break;
		}
	}
}

static bool CheckFormat(VkPhysicalDevice device, VkFormat format, bool tile, VkFormatFeatureFlags features)
{
	VkFormatProperties format_props {};
	vkGetPhysicalDeviceFormatProperties(device, format, &format_props);
	if (tile)
	{
		if ((format_props.optimalTilingFeatures & features) == features)
		{
			return true;
		}
	} else
	{
		if ((format_props.linearTilingFeatures & features) == features)
		{
			return true;
		}
	}
	return false;
}

struct QueueInfo
{
	uint32_t family   = 0;
	uint32_t index    = 0;
	bool     graphics = false;
	bool     compute  = false;
	bool     transfer = false;
	bool     present  = false;
};

struct VulkanQueues
{
	uint32_t          family_count = 0;
	Vector<uint32_t>  family_used;
	Vector<QueueInfo> available;
	Vector<QueueInfo> graphics;
	Vector<QueueInfo> compute;
	Vector<QueueInfo> transfer;
	Vector<QueueInfo> present;
};

static void VulkanDumpQueues(const VulkanQueues& qs)
{
	printf("Queues selected:\n");
	printf("\t family_count = %u\n", qs.family_count);
	Core::StringList nums;
	for (auto u: qs.family_used)
	{
		nums.Add(String::FromPrintf("%u", u));
	}
	printf("\t family_used = [%s]\n", nums.Concat(U", ").C_Str());
	printf("\t graphics:\n");
	for (const auto& q: qs.graphics)
	{
		printf("\t\t family = %u, index = %u\n", q.family, q.index);
	}
	printf("\t compute:\n");
	for (const auto& q: qs.compute)
	{
		printf("\t\t family = %u, index = %u\n", q.family, q.index);
	}
	printf("\t transfer:\n");
	for (const auto& q: qs.transfer)
	{
		printf("\t\t family = %u, index = %u\n", q.family, q.index);
	}
	printf("\t present:\n");
	for (const auto& q: qs.present)
	{
		printf("\t\t family = %u, index = %u\n", q.family, q.index);
	}
}

static VulkanQueues VulkanFindQueues(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t graphics_num, uint32_t compute_num,
                                     uint32_t transfer_num, uint32_t present_num)
{
	EXIT_IF(device == nullptr);
	EXIT_IF(surface == nullptr);

	VulkanQueues qs;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	Vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.GetData());

	qs.family_count = queue_family_count;

	uint32_t family = 0;
	for (auto& f: queue_families)
	{
		VkBool32 presentation_supported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, family, surface, &presentation_supported);

		printf("\tqueue family: %s [count = %u], [present = %s]\n", string_VkQueueFlags(f.queueFlags).c_str(), f.queueCount,
		       (presentation_supported == VK_TRUE ? "true" : "false"));

		for (uint32_t i = 0; i < f.queueCount; i++)
		{
			QueueInfo info;
			info.family   = family;
			info.index    = i;
			info.graphics = (f.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
			info.compute  = (f.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
			info.transfer = (f.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
			info.present  = (presentation_supported == VK_TRUE);

			qs.available.Add(info);
		}

		qs.family_used.Add(0);

		family++;
	}

	for (uint32_t i = 0; i < graphics_num; i++)
	{
		if (auto index = qs.available.Find(true, [](auto& q, auto& b) { return q.graphics == b; }); qs.available.IndexValid(index))
		{
			qs.family_used[qs.available.At(index).family]++;
			qs.graphics.Add(qs.available.At(index));
			qs.available.RemoveAt(index);
		}
	}

	for (uint32_t i = 0; i < compute_num; i++)
	{
		if (auto index = qs.available.Find(true, [](auto& q, auto& b) { return q.compute == b; }); qs.available.IndexValid(index))
		{
			qs.family_used[qs.available.At(index).family]++;
			qs.compute.Add(qs.available.At(index));
			qs.available.RemoveAt(index);
		}
	}

	for (uint32_t i = 0; i < transfer_num; i++)
	{
		if (auto index = qs.available.Find(true, [](auto& q, auto& b) { return q.transfer == b; }); qs.available.IndexValid(index))
		{
			qs.family_used[qs.available.At(index).family]++;
			qs.transfer.Add(qs.available.At(index));
			qs.available.RemoveAt(index);
		}
	}

	for (uint32_t i = 0; i < present_num; i++)
	{
		if (auto index = qs.available.Find(true, [](auto& q, auto& b) { return q.present == b; }); qs.available.IndexValid(index))
		{
			qs.family_used[qs.available.At(index).family]++;
			qs.present.Add(qs.available.At(index));
			qs.available.RemoveAt(index);
		}
	}

	return qs;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void VulkanFindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const Vector<const char*>& device_extensions,
                                     SurfaceCapabilities* out_capabilities, VkPhysicalDevice* out_device, VulkanQueues* out_queues)
{
	EXIT_IF(instance == nullptr);
	EXIT_IF(surface == nullptr);
	EXIT_IF(out_capabilities == nullptr);
	EXIT_IF(out_device == nullptr);
	EXIT_IF(out_queues == nullptr);

	uint32_t devices_count = 0;
	vkEnumeratePhysicalDevices(instance, &devices_count, nullptr);

	EXIT_NOT_IMPLEMENTED(devices_count == 0);

	Vector<VkPhysicalDevice> devices(devices_count);
	vkEnumeratePhysicalDevices(instance, &devices_count, devices.GetData());

	VkPhysicalDevice best_device = nullptr;
	VulkanQueues     best_queues;

	for (const auto& device: devices)
	{
		bool skip_device = false;

		VkPhysicalDeviceProperties device_properties {};
		VkPhysicalDeviceFeatures2  device_features2 {};

		VkPhysicalDeviceColorWriteEnableFeaturesEXT color_write_ext {};
		color_write_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT;
		color_write_ext.pNext = nullptr;

		device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		device_features2.pNext = &color_write_ext;

		vkGetPhysicalDeviceProperties(device, &device_properties);
		vkGetPhysicalDeviceFeatures2(device, &device_features2);

		printf("Vulkan device: %s\n", device_properties.deviceName);

		auto qs = VulkanFindQueues(device, surface, GraphicContext::QUEUE_GFX_NUM, GraphicContext::QUEUE_COMPUTE_NUM,
		                           GraphicContext::QUEUE_UTIL_NUM, GraphicContext::QUEUE_PRESENT_NUM);

		VulkanDumpQueues(qs);

		if (qs.graphics.Size() != GraphicContext::QUEUE_GFX_NUM ||
		    !(qs.compute.Size() >= 1 && qs.compute.Size() <= GraphicContext::QUEUE_COMPUTE_NUM) ||
		    qs.transfer.Size() != GraphicContext::QUEUE_UTIL_NUM || qs.present.Size() != GraphicContext::QUEUE_PRESENT_NUM)
		{
			printf("Not enough queues\n");
			skip_device = true;
		}

		if (color_write_ext.colorWriteEnable != VK_TRUE)
		{
			printf("colorWriteEnable is not supported\n");
			skip_device = true;
		}

		if (device_features2.features.fragmentStoresAndAtomics != VK_TRUE)
		{
			printf("fragmentStoresAndAtomics is not supported\n");
			skip_device = true;
		}

		if (device_features2.features.samplerAnisotropy != VK_TRUE)
		{
			printf("samplerAnisotropy is not supported\n");
			skip_device = true;
		}

		//		if (device_features2.features.shaderImageGatherExtended != VK_TRUE)
		//		{
		//			printf("shaderImageGatherExtended is not supported\n");
		//			skip_device = true;
		//		}

		if (!skip_device)
		{
			uint32_t extensions_count = 0;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, nullptr);

			EXIT_NOT_IMPLEMENTED(extensions_count == 0);

			Vector<VkExtensionProperties> available_extensions(extensions_count);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, available_extensions.GetData());

			for (const char* ext: device_extensions)
			{
				if (!available_extensions.Contains(ext, [](auto p, auto ext) { return strcmp(p.extensionName, ext) == 0; }))
				{
					skip_device = true;
					break;
				}
			}

			if (skip_device)
			{
				for (const auto& ext: available_extensions)
				{
					printf("Vulkan available extension: %s, version = %u\n", ext.extensionName, ext.specVersion);
				}
			}
		}

		if (!skip_device)
		{
			VulkanGetSurfaceCapabilities(device, surface, out_capabilities);

			if ((out_capabilities->capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
			{
				printf("Surface cannot be destination of blit\n");
				skip_device = true;
			}
		}

		if (!skip_device && !CheckFormat(device, VK_FORMAT_R8G8B8A8_SRGB, false, VK_FORMAT_FEATURE_BLIT_SRC_BIT))
		{
			printf("Format VK_FORMAT_R8G8B8A8_SRGB cannot be used as transfer source\n");
			skip_device = true;
		}

		if (!skip_device && !CheckFormat(device, VK_FORMAT_D32_SFLOAT, true, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			printf("Format VK_FORMAT_D32_SFLOAT cannot be used as depth buffer\n");
			skip_device = true;
		}

		if (!skip_device && !CheckFormat(device, VK_FORMAT_D32_SFLOAT_S8_UINT, true, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			printf("Format VK_FORMAT_D32_SFLOAT_S8_UINT cannot be used as depth buffer\n");
			skip_device = true;
		}

		if (!skip_device && !CheckFormat(device, VK_FORMAT_D16_UNORM, true, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			printf("Format VK_FORMAT_D16_UNORM cannot be used as depth buffer\n");
			skip_device = true;
		}

		if (!skip_device && !CheckFormat(device, VK_FORMAT_D24_UNORM_S8_UINT, true, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			printf("Format VK_FORMAT_D24_UNORM_S8_UINT cannot be used as depth buffer\n");
			skip_device = true;
		}

		if (!skip_device &&
		    !CheckFormat(device, VK_FORMAT_BC3_SRGB_BLOCK, true, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		{
			printf("Format VK_FORMAT_BC3_SRGB_BLOCK cannot be used as texture\n");
			skip_device = true;
		}

		if (!skip_device &&
		    !CheckFormat(device, VK_FORMAT_R8G8B8A8_SRGB, true, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		{
			printf("Format VK_FORMAT_R8G8B8A8_SRGB cannot be used as texture\n");
			skip_device = true;
		}

		if (!skip_device &&
		    !CheckFormat(device, VK_FORMAT_R8_UNORM, true, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		{
			printf("Format VK_FORMAT_R8_UNORM cannot be used as texture\n");
			skip_device = true;
		}

		if (!skip_device &&
		    !CheckFormat(device, VK_FORMAT_R8G8_UNORM, true, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		{
			printf("Format VK_FORMAT_R8G8_UNORM cannot be used as texture\n");
			skip_device = true;
		}

		if (!skip_device &&
		    !CheckFormat(device, VK_FORMAT_R8G8B8A8_SRGB, true, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		{
			printf("Format VK_FORMAT_R8G8B8A8_SRGB cannot be used as texture\n");

			if (!skip_device && !CheckFormat(device, VK_FORMAT_R8G8B8A8_UNORM, true,
			                                 VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
			{
				printf("Format VK_FORMAT_R8G8B8A8_UNORM cannot be used as texture\n");
				skip_device = true;
			}
		}

		if (!skip_device &&
		    !CheckFormat(device, VK_FORMAT_B8G8R8A8_SRGB, true, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		{
			printf("Format VK_FORMAT_B8G8R8A8_SRGB cannot be used as texture\n");

			if (!skip_device && !CheckFormat(device, VK_FORMAT_B8G8R8A8_UNORM, true,
			                                 VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
			{
				printf("Format VK_FORMAT_B8G8R8A8_UNORM cannot be used as texture\n");
				skip_device = true;
			}
		}

		/*if (!skip_device && !CheckFormat(device, VK_FORMAT_S8_UINT, true, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
		    printf("Format VK_FORMAT_S8_UINT cannot be used as depth buffer");
		    skip_device = true;
		}*/

		if (!skip_device && device_properties.limits.maxSamplerAnisotropy < 16.0f)
		{
			printf("maxSamplerAnisotropy < 16.0f");
			skip_device = true;
		}

		if (skip_device)
		{
			continue;
		}

		if (best_device == nullptr || device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			best_device = device;
			best_queues = qs;
		}
	}

	*out_device = best_device;
	*out_queues = best_queues;
}

static VkDevice VulkanCreateDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VulkanExtensions* r,
                                   const VulkanQueues& queues, const Vector<const char*>& device_extensions)
{
	EXIT_IF(physical_device == nullptr);
	EXIT_IF(r == nullptr);
	EXIT_IF(surface == nullptr);

	Vector<VkDeviceQueueCreateInfo> queue_create_info(queues.family_count);
	Vector<Vector<float>>           queue_priority(queues.family_count);
	uint32_t                        queue_create_info_num = 0;

	for (uint32_t i = 0; i < queues.family_count; i++)
	{
		if (queues.family_used[i] != 0)
		{
			for (uint32_t pi = 0; pi < queues.family_used[i]; pi++)
			{
				queue_priority[queue_create_info_num].Add(1.0f);
			}

			queue_create_info[queue_create_info_num].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info[queue_create_info_num].pNext            = nullptr;
			queue_create_info[queue_create_info_num].flags            = 0;
			queue_create_info[queue_create_info_num].queueFamilyIndex = i;
			queue_create_info[queue_create_info_num].queueCount       = queues.family_used[i];
			queue_create_info[queue_create_info_num].pQueuePriorities = queue_priority[queue_create_info_num].GetDataConst();

			queue_create_info_num++;
		}
	}

	VkPhysicalDeviceFeatures device_features {};
	device_features.fragmentStoresAndAtomics = VK_TRUE;
	device_features.samplerAnisotropy        = VK_TRUE;
	// device_features.shaderImageGatherExtended = VK_TRUE;

	VkPhysicalDeviceColorWriteEnableFeaturesEXT color_write_ext {};
	color_write_ext.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT;
	color_write_ext.pNext            = nullptr;
	color_write_ext.colorWriteEnable = VK_TRUE;

	VkDeviceCreateInfo create_info {};
	create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pNext                   = &color_write_ext;
	create_info.flags                   = 0;
	create_info.pQueueCreateInfos       = queue_create_info.GetDataConst();
	create_info.queueCreateInfoCount    = queue_create_info_num;
	create_info.enabledLayerCount       = (r->enable_validation_layers ? r->required_layers.Size() : 0);
	create_info.ppEnabledLayerNames     = (r->enable_validation_layers ? r->required_layers.GetDataConst() : nullptr);
	create_info.enabledExtensionCount   = device_extensions.Size();
	create_info.ppEnabledExtensionNames = device_extensions.GetDataConst();
	create_info.pEnabledFeatures        = &device_features;

	VkDevice device = nullptr;

	vkCreateDevice(physical_device, &create_info, nullptr, &device);

	return device;
}

static void VulkanGetExtensions(SDL_Window* window, VulkanExtensions* r)
{
	EXIT_IF(window == nullptr);
	EXIT_IF(r == nullptr);

	uint32_t required_extensions_count  = 0;
	uint32_t available_extensions_count = 0;
	uint32_t available_layers_count     = 0;

	auto sdl_result = SDL_Vulkan_GetInstanceExtensions(window, &required_extensions_count, nullptr);

	EXIT_NOT_IMPLEMENTED(sdl_result == SDL_FALSE);
	EXIT_NOT_IMPLEMENTED(required_extensions_count == 0);

	r->required_extensions = Vector<const char*>(required_extensions_count, false); // @suppress("Ambiguous problem")
	r->required_extensions.Memset(0);

	sdl_result = SDL_Vulkan_GetInstanceExtensions(window, &required_extensions_count, r->required_extensions.GetData());

	EXIT_NOT_IMPLEMENTED(sdl_result == SDL_FALSE);
	EXIT_NOT_IMPLEMENTED(required_extensions_count == 0);
	EXIT_NOT_IMPLEMENTED(required_extensions_count != r->required_extensions.Size());

	vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, nullptr);

	r->available_extensions = Vector<VkExtensionProperties>(available_extensions_count); // @suppress("Ambiguous problem")
	r->available_extensions.Memset(0);

	vkEnumerateInstanceExtensionProperties(nullptr, &available_extensions_count, r->available_extensions.GetData());

	EXIT_NOT_IMPLEMENTED(available_extensions_count != r->available_extensions.Size());

	r->enable_validation_layers = Config::VulkanValidationEnabled();

	if (r->available_extensions.Contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, [](auto s, auto l) { return strcmp(s.extensionName, l) == 0; }))
	{
		r->required_extensions.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	} else
	{
		r->enable_validation_layers = false;
	}

	for (const char* ext: r->required_extensions)
	{
		printf("Vulkan required extension: %s\n", ext);
	}

	for (const auto& ext: r->available_extensions)
	{
		printf("Vulkan available extension: %s, version = %u\n", ext.extensionName, ext.specVersion);
	}

	vkEnumerateInstanceLayerProperties(&available_layers_count, nullptr);

	r->available_layers = Vector<VkLayerProperties>(available_layers_count); // @suppress("Ambiguous problem")
	r->available_layers.Memset(0);
	vkEnumerateInstanceLayerProperties(&available_layers_count, r->available_layers.GetData());

	EXIT_NOT_IMPLEMENTED(available_layers_count != r->available_layers.Size());

	for (const auto& l: r->available_layers)
	{
		printf("Vulkan available layer: %s, specVersion = %u, implVersion = %u, %s\n", l.layerName, l.specVersion, l.implementationVersion,
		       l.description);
	}

	r->required_layers = {"VK_LAYER_KHRONOS_validation"};

	if (r->enable_validation_layers)
	{
		for (const char* l: r->required_layers)
		{
			if (!r->available_layers.Contains(l, [](auto s, auto l) { return strcmp(s.layerName, l) == 0; }))
			{
				printf("no validation layer: %s\n", l);
				r->enable_validation_layers = false;
				break;
			}
		}
	}

	if (r->enable_validation_layers)
	{
		vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &available_extensions_count, nullptr);

		Vector<VkExtensionProperties> available_extensions(available_extensions_count);

		vkEnumerateInstanceExtensionProperties("VK_LAYER_KHRONOS_validation", &available_extensions_count, available_extensions.GetData());

		for (const auto& ext: available_extensions)
		{
			printf("VK_LAYER_KHRONOS_validation available extension: %s, version = %u\n", ext.extensionName, ext.specVersion);
		}

		if (available_extensions.Contains("VK_EXT_validation_features", [](auto s, auto l) { return strcmp(s.extensionName, l) == 0; }))
		{
			r->required_extensions.Add("VK_EXT_validation_features");
		} else
		{
			r->enable_validation_layers = false;
		}
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                                   VkDebugUtilsMessageTypeFlagsEXT             message_types,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                                   void* /*user_data*/)
{
	const char* severity_str   = nullptr;
	const char* severity_color = FG_DEFAULT;
	bool        skip           = false;
	bool        error          = false;
	bool        debug_printf   = false;
	switch (message_severity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			severity_str   = "V";
			severity_color = FG_BRIGHT_WHITE;
			skip           = true;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			if ((message_types & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0 && Config::SpirvDebugPrintfEnabled() &&
			    strcmp(callback_data->pMessageIdName, "UNASSIGNED-DEBUG-PRINTF") == 0)
			{
				debug_printf   = true;
				severity_color = FG_BRIGHT_YELLOW;
				skip           = true;
			} else
			{
				severity_str   = "I";
				severity_color = FG_DEFAULT;
			}
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			severity_str   = "W";
			severity_color = FG_RED;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			severity_str   = "E";
			severity_color = FG_BRIGHT_RED;
			error          = true;
			break;
		default: severity_str = "?";
	}

	if (error)
	{
		EXIT("%s[Vulkan][%s][%u]: %s%s\n", severity_color, severity_str, static_cast<uint32_t>(message_types), callback_data->pMessage,
		     FG_DEFAULT);
	}

	if (!skip)
	{
		printf("%s[Vulkan][%s][%u]: %s%s\n", severity_color, severity_str, static_cast<uint32_t>(message_types), callback_data->pMessage,
		       FG_DEFAULT);
	}

	if (debug_printf)
	{
		auto strs = String::FromUtf8(callback_data->pMessage).Split(U'|');
		if (!strs.IsEmpty())
		{
			printf("%s%s%s\n", severity_color, strs.At(strs.Size() - 1).C_Str(), FG_DEFAULT);
		}
	}

	return VK_FALSE;
}

static VKAPI_ATTR VkResult VKAPI_CALL VulkanCreateDebugUtilsMessengerEXT(VkInstance                                instance,
                                                                         const VkDebugUtilsMessengerCreateInfoEXT* create_info,
                                                                         const VkAllocationCallbacks*              allocator,
                                                                         VkDebugUtilsMessengerEXT*                 messenger)
{
	EXIT_IF(instance == nullptr);

	if (auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	    func != nullptr)
	{
		return func(instance, create_info, allocator, messenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

[[maybe_unused]] static VkSwapchainKHR VulkanCreateSwapchainInternal(VkDevice device, VkSurfaceKHR surface, uint32_t width, uint32_t height,
                                                                     uint32_t image_count, SurfaceCapabilities* r,
                                                                     VkFormat* swapchain_format, VkExtent2D* swapchain_extent,
                                                                     VkImage** swapchain_images, VkImageView** swapchain_image_views,
                                                                     uint32_t* swapchain_images_count)
{
	EXIT_IF(device == nullptr);
	EXIT_IF(surface == nullptr);
	EXIT_IF(r == nullptr);
	EXIT_IF(swapchain_format == nullptr);
	EXIT_IF(swapchain_extent == nullptr);
	EXIT_IF(swapchain_images == nullptr);
	EXIT_IF(swapchain_image_views == nullptr);
	EXIT_IF(swapchain_images_count == nullptr);

	EXIT_NOT_IMPLEMENTED(r->formats.IsEmpty());

	VkExtent2D extent {};
	extent.width  = std::clamp(width, r->capabilities.minImageExtent.width, r->capabilities.maxImageExtent.width);
	extent.height = std::clamp(height, r->capabilities.minImageExtent.height, r->capabilities.maxImageExtent.height);

	image_count = std::clamp(image_count, r->capabilities.minImageCount, r->capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR create_info {};
	create_info.sType         = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.pNext         = nullptr;
	create_info.flags         = 0;
	create_info.surface       = surface;
	create_info.minImageCount = image_count;

	if (r->format_unorm_bgra32)
	{
		create_info.imageFormat     = VK_FORMAT_B8G8R8A8_UNORM;
		create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	} else if (r->format_srgb_bgra32)
	{
		create_info.imageFormat     = VK_FORMAT_B8G8R8A8_SRGB;
		create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	} else
	{
		create_info.imageFormat     = r->formats.At(0).format;
		create_info.imageColorSpace = r->formats.At(0).colorSpace;
	}

	create_info.imageExtent           = extent;
	create_info.imageArrayLayers      = 1;
	create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices   = nullptr;
	create_info.preTransform          = r->capabilities.currentTransform;
	create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode           = VK_PRESENT_MODE_FIFO_KHR;
	create_info.clipped               = VK_TRUE;
	create_info.oldSwapchain          = nullptr;

	*swapchain_format = create_info.imageFormat;
	*swapchain_extent = extent;

	VkSwapchainKHR swapchain = nullptr;

	vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain);

	vkGetSwapchainImagesKHR(device, swapchain, swapchain_images_count, nullptr);
	EXIT_NOT_IMPLEMENTED(*swapchain_images_count == 0);

	*swapchain_images = new VkImage[*swapchain_images_count];
	vkGetSwapchainImagesKHR(device, swapchain, swapchain_images_count, *swapchain_images);

	*swapchain_image_views = new VkImageView[*swapchain_images_count];
	for (uint32_t i = 0; i < *swapchain_images_count; i++)
	{
		VkImageViewCreateInfo create_info {};
		create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.pNext                           = nullptr;
		create_info.flags                           = 0;
		create_info.image                           = (*swapchain_images)[i];
		create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format                          = *swapchain_format;
		create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.baseMipLevel   = 0;
		create_info.subresourceRange.layerCount     = 1;
		create_info.subresourceRange.levelCount     = 1;

		vkCreateImageView(device, &create_info, nullptr, &((*swapchain_image_views)[i]));
	}

	return swapchain;
}

static void VulkanCreateQueues(GraphicContext* ctx, const VulkanQueues& queues)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(ctx->device == nullptr);
	EXIT_IF(queues.graphics.Size() != 1);
	EXIT_IF(queues.transfer.Size() != 1);
	EXIT_IF(queues.present.Size() != 1);
	EXIT_IF(!(queues.compute.Size() >= 1 && queues.compute.Size() <= GraphicContext::QUEUE_COMPUTE_NUM));

	auto get_queue = [ctx](int id, const QueueInfo& info, bool with_mutex = false)
	{
		ctx->queues[id].family = info.family;
		ctx->queues[id].index  = info.index;
		EXIT_IF(ctx->queues[id].vk_queue != nullptr);
		vkGetDeviceQueue(ctx->device, ctx->queues[id].family, ctx->queues[id].index, &ctx->queues[id].vk_queue);
		EXIT_NOT_IMPLEMENTED(ctx->queues[id].vk_queue == nullptr);
		if (with_mutex)
		{
			ctx->queues[id].mutex = new Core::Mutex;
		}
	};

	get_queue(GraphicContext::QUEUE_GFX, queues.graphics.At(0));
	get_queue(GraphicContext::QUEUE_UTIL, queues.transfer.At(0));
	get_queue(GraphicContext::QUEUE_PRESENT, queues.present.At(0));

	for (int id = 0; id < GraphicContext::QUEUE_COMPUTE_NUM; id++)
	{
		bool with_mutex = (GraphicContext::QUEUE_COMPUTE_NUM == queues.compute.Size());
		get_queue(GraphicContext::QUEUE_COMPUTE_START + id, queues.compute.At(id % queues.compute.Size()), with_mutex);
	}
}

static VulkanSwapchain* VulkanCreateSwapchain(GraphicContext* ctx, uint32_t image_count)
{
	EXIT_IF(g_window_ctx == nullptr);
	EXIT_IF(ctx == nullptr);
	EXIT_IF(ctx->screen_width == 0);
	EXIT_IF(ctx->screen_height == 0);

	Core::LockGuard lock(g_window_ctx->mutex);

	auto* s = new VulkanSwapchain;

	s->swapchain = VulkanCreateSwapchainInternal(ctx->device, g_window_ctx->surface, ctx->screen_width, ctx->screen_height, image_count,
	                                             g_window_ctx->surface_capabilities, &s->swapchain_format, &s->swapchain_extent,
	                                             &s->swapchain_images, &s->swapchain_image_views, &s->swapchain_images_count);
	if (s->swapchain == nullptr)
	{
		EXIT("Could not create swapchain");
	}

	s->current_index = static_cast<uint32_t>(-1);

	VkSemaphoreCreateInfo present_complete_info {};
	present_complete_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	present_complete_info.pNext = nullptr;
	present_complete_info.flags = 0;

	auto result = vkCreateSemaphore(ctx->device, &present_complete_info, nullptr, &s->present_complete_semaphore);
	EXIT_NOT_IMPLEMENTED(result != VK_SUCCESS);

	VkFenceCreateInfo fence_info;
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.pNext = nullptr;
	fence_info.flags = 0;

	result = vkCreateFence(ctx->device, &fence_info, nullptr, &s->present_complete_fence);
	EXIT_NOT_IMPLEMENTED(result != VK_SUCCESS);

	return s;
}

static void VulkanCreate(WindowContext* ctx)
{
	EXIT_IF(ctx->window == nullptr);
	EXIT_IF(ctx->graphic_ctx.instance != nullptr);
	EXIT_IF(ctx->graphic_ctx.physical_device != nullptr);
	EXIT_IF(ctx->graphic_ctx.device != nullptr);
	EXIT_IF(ctx->surface_capabilities != nullptr);

	VulkanExtensions r;
	VulkanGetExtensions(ctx->window, &r);

	VkApplicationInfo app_info {};
	app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext              = nullptr;
	app_info.pApplicationName   = "Kyty";
	app_info.applicationVersion = 1;
	app_info.pEngineName        = "Kyty";
	app_info.engineVersion      = 1;
	app_info.apiVersion         = VK_API_VERSION_1_2; // NOLINT

	VkValidationFeatureDisableEXT disabled_features[] = {};
	VkValidationFeatureEnableEXT  enabled_features[]  = {
#ifdef KYTY_ENABLE_BEST_PRACTICES
	    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
#endif
#ifdef KYTY_ENABLE_DEBUG_PRINTF
	    VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
#endif
	};

	uint32_t enabled_features_count = sizeof(enabled_features) / sizeof(VkValidationFeatureEnableEXT);

#ifdef KYTY_ENABLE_DEBUG_PRINTF
	if (!Config::SpirvDebugPrintfEnabled())
	{
		enabled_features_count--;
	}
#endif

	VkValidationFeaturesEXT validation_features {};
	validation_features.sType                          = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validation_features.pNext                          = nullptr;
	validation_features.enabledValidationFeatureCount  = enabled_features_count;
	validation_features.pEnabledValidationFeatures     = enabled_features;
	validation_features.disabledValidationFeatureCount = 0;
	validation_features.pDisabledValidationFeatures    = disabled_features;

	VkDebugUtilsMessengerCreateInfoEXT dbg_create_info {};
	dbg_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	dbg_create_info.pNext           = &validation_features;
	dbg_create_info.flags           = 0;
	dbg_create_info.messageSeverity = static_cast<uint32_t>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) |
	                                  static_cast<uint32_t>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) |
	                                  static_cast<uint32_t>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) |
	                                  static_cast<uint32_t>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
	dbg_create_info.messageType = static_cast<uint32_t>(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) |
	                              static_cast<uint32_t>(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) |
	                              static_cast<uint32_t>(VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
	dbg_create_info.pfnUserCallback = VulkanDebugMessengerCallback;
	dbg_create_info.pUserData       = nullptr;

	VkInstanceCreateInfo inst_info {};
	inst_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext                   = (r.enable_validation_layers ? &dbg_create_info : nullptr);
	inst_info.flags                   = 0;
	inst_info.pApplicationInfo        = &app_info;
	inst_info.enabledExtensionCount   = r.required_extensions.Size();
	inst_info.ppEnabledExtensionNames = r.required_extensions.GetDataConst();
	inst_info.enabledLayerCount       = (r.enable_validation_layers ? r.required_layers.Size() : 0);
	inst_info.ppEnabledLayerNames     = (r.enable_validation_layers ? r.required_layers.GetDataConst() : nullptr);

	VkResult result = vkCreateInstance(&inst_info, nullptr, &ctx->graphic_ctx.instance);
	if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		EXIT("Unable to find a compatible Vulkan Driver");
	} else if (result != VK_SUCCESS)
	{
		EXIT("Could not create a Vulkan instance (for unknown reasons)");
	}

	if (r.enable_validation_layers)
	{
		dbg_create_info.pNext = nullptr;
		if (VulkanCreateDebugUtilsMessengerEXT(ctx->graphic_ctx.instance, &dbg_create_info, nullptr, &ctx->graphic_ctx.debug_messenger) !=
		    VK_SUCCESS)
		{
			EXIT("Could not create debug messenger");
		}
	}

	if (SDL_Vulkan_CreateSurface(ctx->window, ctx->graphic_ctx.instance, &ctx->surface) == SDL_FALSE)
	{
		EXIT("Could not create a Vulkan surface");
	}

	Vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
	                                         VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME, "VK_KHR_maintenance1"};

#ifdef KYTY_ENABLE_DEBUG_PRINTF
	if (Config::SpirvDebugPrintfEnabled())
	{
		device_extensions.Add(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
	}
#endif

	ctx->surface_capabilities = new SurfaceCapabilities {};

	VulkanQueues queues;

	VulkanFindPhysicalDevice(ctx->graphic_ctx.instance, ctx->surface, device_extensions, ctx->surface_capabilities,
	                         &ctx->graphic_ctx.physical_device, &queues);

	if (ctx->graphic_ctx.physical_device == nullptr)
	{
		EXIT("Could not find suitable device");
	}

	VkPhysicalDeviceProperties device_properties {};
	vkGetPhysicalDeviceProperties(ctx->graphic_ctx.physical_device, &device_properties);

	printf("Select device: %s\n", device_properties.deviceName);

	memcpy(ctx->device_name, device_properties.deviceName, sizeof(ctx->device_name));
	memcpy(ctx->processor_name, Loader::GetSystemInfo().ProcessorName.C_Str(), sizeof(ctx->processor_name));

	ctx->graphic_ctx.device = VulkanCreateDevice(ctx->graphic_ctx.physical_device, ctx->surface, &r, queues, device_extensions);
	if (ctx->graphic_ctx.device == nullptr)
	{
		EXIT("Could not create device");
	}

	VulkanCreateQueues(&ctx->graphic_ctx, queues);

	ctx->swapchain = VulkanCreateSwapchain(&ctx->graphic_ctx, 2);
}

void WindowInit(uint32_t width, uint32_t height)
{
	EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());
	EXIT_IF(g_window_ctx != nullptr);

	g_window_ctx = new WindowContext;

	g_window_ctx->graphic_ctx.screen_width  = width;
	g_window_ctx->graphic_ctx.screen_height = height;
}

void WindowWaitForGraphicInitialized()
{
	EXIT_IF(g_window_ctx == nullptr);

	Core::LockGuard lock(g_window_ctx->mutex);

	while (!g_window_ctx->graphic_initialized)
	{
		g_window_ctx->graphic_initialized_condvar.Wait(&g_window_ctx->mutex);
	}
}

void WindowRun()
{
	EXIT_IF(g_window_ctx == nullptr);

	// KYTY_PROFILER_THREAD("Thread_Window");
	EASY_MAIN_THREAD

	GameApi* api = nullptr;

	g_window_ctx->mutex.Lock();
	{
		EXIT_IF(g_window_ctx->graphic_initialized);

		WindowCreate(g_window_ctx);
		VulkanCreate(g_window_ctx);
		api = game_create_api();

		g_window_ctx->game = api;

		g_window_ctx->graphic_initialized = true;
		g_window_ctx->graphic_initialized_condvar.Signal();
	}
	g_window_ctx->mutex.Unlock();

	game_main_loop(api, &g_window_ctx->graphic_ctx);
	// game_delete_api(api);

	Core::SubsystemsListSingleton::Instance()->ShutdownAll();
	std::_Exit(0);
}

VkSurfaceCapabilitiesKHR* VulkanGetSurfaceCapabilities()
{
	EXIT_IF(g_window_ctx == nullptr);

	Core::LockGuard lock(g_window_ctx->mutex);

	// auto* ctx = &g_window_ctx->graphic_ctx;

	return &g_window_ctx->surface_capabilities->capabilities;
}

GraphicContext* WindowGetGraphicContext()
{
	EXIT_IF(g_window_ctx == nullptr);

	Core::LockGuard lock(g_window_ctx->mutex);

	return &g_window_ctx->graphic_ctx;
}

void WindowUpdateIcon()
{
	EXIT_IF(g_window_ctx == nullptr);

	static Image* icon = Loader::SystemContentGetIcon();

	if (icon != nullptr)
	{
		SDL_SetWindowIcon(g_window_ctx->window, static_cast<SDL_Surface*>(icon->GetSdlSurface()));
	}
}

void WindowUpdateTitle()
{
	EXIT_IF(g_window_ctx == nullptr);
	EXIT_IF(g_window_ctx->game == nullptr);

	static char title[128];
	static char title_id[12];
	static char app_ver[8];
	static bool has_title    = Loader::SystemContentParamSfoGetString("TITLE", title, sizeof(title));
	static bool has_title_id = Loader::SystemContentParamSfoGetString("TITLE_ID", title_id, sizeof(title_id));
	static bool has_app_ver  = Loader::SystemContentParamSfoGetString("APP_VER", app_ver, sizeof(app_ver));

	auto fps = String::FromPrintf("%s%s%s%s%s%s[%s] [%s], frame: %d, fps: %f", (has_title ? title : ""), (has_title ? ", " : ""),
	                              (has_title_id ? title_id : ""), (has_title_id ? ", " : ""), (has_app_ver ? app_ver : ""),
	                              (has_app_ver ? ", " : ""), g_window_ctx->device_name, g_window_ctx->processor_name,
	                              g_window_ctx->game->m_frame_num, g_window_ctx->game->m_current_fps);

	SDL_SetWindowTitle(g_window_ctx->window, fps.C_Str());
}

void WindowDrawBuffer(VideoOutVulkanImage* image)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(image == nullptr);
	EXIT_IF(g_window_ctx == nullptr);
	EXIT_IF(g_window_ctx->swapchain == nullptr);

	if (g_window_ctx->window_hidden)
	{
		WindowUpdateIcon();

		SDL_ShowWindow(g_window_ctx->window);

		g_window_ctx->window_hidden = false;
	}

	g_window_ctx->swapchain->current_index = static_cast<uint32_t>(-1);

	auto result = vkAcquireNextImageKHR(g_window_ctx->graphic_ctx.device, g_window_ctx->swapchain->swapchain, UINT64_MAX,
	                                    /*g_window_ctx->swapchain->present_complete_semaphore*/ nullptr,
	                                    g_window_ctx->swapchain->present_complete_fence, &g_window_ctx->swapchain->current_index);

	EXIT_NOT_IMPLEMENTED(result != VK_SUCCESS);
	EXIT_NOT_IMPLEMENTED(g_window_ctx->swapchain->current_index == static_cast<uint32_t>(-1));

	do
	{
		result = vkWaitForFences(g_window_ctx->graphic_ctx.device, 1, &g_window_ctx->swapchain->present_complete_fence, VK_TRUE, 100000000);
	} while (result == VK_TIMEOUT);
	EXIT_NOT_IMPLEMENTED(result != VK_SUCCESS);

	vkResetFences(g_window_ctx->graphic_ctx.device, 1, &g_window_ctx->swapchain->present_complete_fence);

	auto* blt_src_image = image;
	auto* blt_dst_image = g_window_ctx->swapchain;

	EXIT_IF(blt_src_image == nullptr);
	EXIT_IF(blt_dst_image == nullptr);

	CommandBuffer buffer(GraphicContext::QUEUE_PRESENT);
	// buffer.SetQueue(GraphicContext::QUEUE_PRESENT);

	EXIT_NOT_IMPLEMENTED(buffer.IsInvalid());

	auto* vk_buffer = buffer.GetPool()->buffers[buffer.GetIndex()];

	buffer.Begin();

	UtilBlitImage(&buffer, blt_src_image, blt_dst_image);

	VkImageMemoryBarrier pre_present_barrier {};
	pre_present_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	pre_present_barrier.pNext                           = nullptr;
	pre_present_barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
	pre_present_barrier.dstAccessMask                   = VK_ACCESS_MEMORY_READ_BIT;
	pre_present_barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	pre_present_barrier.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	pre_present_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	pre_present_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	pre_present_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	pre_present_barrier.subresourceRange.baseMipLevel   = 0;
	pre_present_barrier.subresourceRange.levelCount     = 1;
	pre_present_barrier.subresourceRange.baseArrayLayer = 0;
	pre_present_barrier.subresourceRange.layerCount     = 1;
	pre_present_barrier.image                           = g_window_ctx->swapchain->swapchain_images[g_window_ctx->swapchain->current_index];
	vkCmdPipelineBarrier(vk_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
	                     &pre_present_barrier);

	buffer.End();
	buffer.ExecuteWithSemaphore();

	VkPresentInfoKHR present;
	present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext              = nullptr;
	present.swapchainCount     = 1;
	present.pSwapchains        = &g_window_ctx->swapchain->swapchain;
	present.pImageIndices      = &g_window_ctx->swapchain->current_index;
	present.pWaitSemaphores    = &buffer.GetPool()->semaphores[buffer.GetIndex()];
	present.waitSemaphoreCount = 1;
	present.pResults           = nullptr;

	const auto& queue = g_window_ctx->graphic_ctx.queues[GraphicContext::QUEUE_PRESENT];

	EXIT_IF(queue.mutex != nullptr);

	result = vkQueuePresentKHR(queue.vk_queue, &present);
	EXIT_NOT_IMPLEMENTED(result != VK_SUCCESS);

	WindowUpdateTitle();
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
