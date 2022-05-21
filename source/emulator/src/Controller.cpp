#include "Emulator/Controller.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Controller {

LIB_NAME("Pad", "Pad");

struct PadData
{
	uint32_t buttons;
	uint8_t  left_stick_x;
	uint8_t  left_stick_y;
	uint8_t  right_stick_x;
	uint8_t  right_stick_y;
	uint8_t  analog_buttons_l2;
	uint8_t  analog_buttons_r2;
	uint8_t  padding[2];
	float    orientation_x;
	float    orientation_y;
	float    orientation_z;
	float    orientation_w;
	float    acceleration_x;
	float    acceleration_y;
	float    acceleration_z;
	float    angular_velocity_x;
	float    angular_velocity_y;
	float    angular_velocity_z;
	uint8_t  touch_data_touch_num;
	uint8_t  touch_data_reserve[3];
	uint32_t touch_data_reserve1;
	uint16_t touch_data_touch0_x;
	uint16_t touch_data_touch0_y;
	uint8_t  touch_data_touch0_id;
	uint8_t  touch_data_touch0_reserve[3];
	uint16_t touch_data_touch1_x;
	uint16_t touch_data_touch1_y;
	uint8_t  touch_data_touch1_id;
	uint8_t  touch_data_touch1_reserve[3];
	bool     connected;
	uint64_t timestamp;
	uint32_t extension_unit_data_extension_unit_id;
	uint8_t  extension_unit_data_reserve[1];
	uint8_t  extension_unit_data_data_length;
	uint8_t  extension_unit_data_data[10];
	uint8_t  connected_count;
	uint8_t  reserve[2];
	uint8_t  device_unique_data_len;
	uint8_t  device_unique_data[12];
};

struct PadControllerInformation
{
	float    touch_pixel_density;
	uint16_t touch_resolution_x;
	uint16_t touch_resolution_y;
	uint8_t  stick_dead_zone_left;
	uint8_t  stick_dead_zone_right;
	uint8_t  connection_type;
	uint8_t  connected_count;
	bool     connected;
	int      device_class;
};

struct PadVibrationParam
{
	uint8_t large_motor;
	uint8_t small_motor;
};

struct ControllerState
{
	uint64_t time                                  = 0;
	uint32_t buttons                               = 0;
	int      axes[static_cast<int>(Axis::AxisMax)] = {128, 128, 128, 128, 0, 0};
};

class GameController
{
public:
	GameController()          = default;
	virtual ~GameController() = default;

	KYTY_CLASS_NO_COPY(GameController);

	void Connect(int id);
	void Disconnect(int id);
	void Button(int id, uint32_t button, bool down);
	void Axis(int id, Axis axis, int value);
	void GetConnectionInfo(bool* flag, int* count);
	void ReadState(ControllerState* state, bool* flag, int* count);
	int  ReadStates(ControllerState* states, int states_num, bool* flag, int* count);

private:
	static constexpr uint32_t STATES_MAX = 64;

	struct StatePrivate
	{
		bool obtained = false;
	};

	void                          CheckActive();
	[[nodiscard]] ControllerState GetLastState() const;
	void                          AddState(const ControllerState& state);

	Core::Mutex     m_mutex;
	Vector<int>     m_connected_ids;
	int             m_active_id       = -1;
	bool            m_connected       = false;
	int             m_connected_count = 0;
	ControllerState m_states[STATES_MAX];
	StatePrivate    m_private[STATES_MAX];
	ControllerState m_last_state;
	uint32_t        m_states_num  = 0;
	uint32_t        m_first_state = 0;
};

static GameController* g_controller = nullptr;

KYTY_SUBSYSTEM_INIT(Controller)
{
	EXIT_IF(g_controller != nullptr);

	g_controller = new GameController;
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Controller) {}

KYTY_SUBSYSTEM_DESTROY(Controller) {}

void GameController::Connect(int id)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(m_connected_ids.Contains(id));

	m_connected_ids.Add(id);

	CheckActive();
}

void GameController::Disconnect(int id)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(!m_connected_ids.Contains(id));

	m_connected_ids.Remove(id);

	CheckActive();
}

void GameController::CheckActive()
{
	bool reset = false;

	if (m_connected)
	{
		if (m_connected_ids.IsEmpty())
		{
			m_active_id = -1;
			m_connected = false;
			reset       = true;
		} else
		{
			if (m_connected_ids.At(0) != m_active_id)
			{
				m_active_id = m_connected_ids.At(0);
				m_connected_count++;
				reset = true;
			}
		}
	} else
	{
		if (!m_connected_ids.IsEmpty())
		{
			m_active_id = m_connected_ids.At(0);
			m_connected = true;
			m_connected_count++;
			reset = true;
		}
	}

	if (reset)
	{
		m_states_num = 0;
		m_last_state = ControllerState();
	}
}

ControllerState GameController::GetLastState() const
{
	if (m_states_num == 0)
	{
		return m_last_state;
	}

	auto last = (m_first_state + m_states_num - 1) % STATES_MAX;

	return m_states[last];
}

void GameController::AddState(const ControllerState& state)
{
	if (m_states_num >= STATES_MAX)
	{
		m_states_num  = STATES_MAX - 1;
		m_first_state = (m_first_state + 1) % STATES_MAX;
	}

	auto index = (m_first_state + m_states_num) % STATES_MAX;

	m_states[index] = state;
	m_last_state    = state;

	m_private[index].obtained = false;

	m_states_num++;
}

void GameController::Button(int id, uint32_t button, bool down)
{
	Core::LockGuard lock(m_mutex);

	if (m_active_id == id)
	{
		auto state = GetLastState();

		state.time = LibKernel::KernelGetProcessTime();

		if (down)
		{
			state.buttons |= button;
		} else
		{
			state.buttons &= ~button;
		}

		AddState(state);
	}
}

void GameController::Axis(int id, Controller::Axis axis, int value)
{
	Core::LockGuard lock(m_mutex);

	if (m_active_id == id)
	{
		auto state = GetLastState();

		state.time = LibKernel::KernelGetProcessTime();

		int axis_id = static_cast<int>(axis);

		EXIT_IF(axis_id < 0 || axis_id >= static_cast<int>(Controller::Axis::AxisMax));

		state.axes[axis_id] = value;

		if (axis == Controller::Axis::TriggerLeft)
		{
			if (value > 0)
			{
				state.buttons |= PAD_BUTTON_L2;
			} else
			{
				state.buttons &= ~PAD_BUTTON_L2;
			}
		}

		if (axis == Controller::Axis::TriggerRight)
		{
			if (value > 0)
			{
				state.buttons |= PAD_BUTTON_R2;
			} else
			{
				state.buttons &= ~PAD_BUTTON_R2;
			}
		}

		AddState(state);
	}
}

void GameController::GetConnectionInfo(bool* flag, int* count)
{
	EXIT_IF(flag == nullptr);
	EXIT_IF(count == nullptr);

	Core::LockGuard lock(m_mutex);

	*flag  = m_connected;
	*count = m_connected_count;
}

void GameController::ReadState(ControllerState* state, bool* flag, int* count)
{
	EXIT_IF(flag == nullptr);
	EXIT_IF(count == nullptr);
	EXIT_IF(state == nullptr);

	Core::LockGuard lock(m_mutex);

	*flag  = m_connected;
	*count = m_connected_count;
	*state = GetLastState();
}

int GameController::ReadStates(ControllerState* states, int states_num, bool* flag, int* count)
{
	EXIT_IF(flag == nullptr);
	EXIT_IF(count == nullptr);
	EXIT_IF(states == nullptr);
	EXIT_IF(states_num < 1 || states_num > STATES_MAX);

	Core::LockGuard lock(m_mutex);

	*flag  = m_connected;
	*count = m_connected_count;

	int ret_num = 0;

	if (m_connected)
	{
		if (m_states_num == 0)
		{
			ret_num   = 1;
			states[0] = m_last_state;
		} else
		{
			for (uint32_t i = 0; i < m_states_num; i++)
			{
				if (ret_num >= states_num)
				{
					break;
				}
				auto index = (m_first_state + i) % STATES_MAX;
				if (!m_private[index].obtained)
				{
					m_private[index].obtained = true;

					states[ret_num++] = m_states[index];
				}
			}
		}
	}

	return ret_num;
}

void ControllerConnect(int id)
{
	EXIT_IF(g_controller == nullptr);

	g_controller->Connect(id);
}

void ControllerDisconnect(int id)
{
	EXIT_IF(g_controller == nullptr);

	g_controller->Disconnect(id);
}

void ControllerButton(int id, uint32_t button, bool down)
{
	EXIT_IF(g_controller == nullptr);

	g_controller->Button(id, button, down);
}

void ControllerAxis(int id, Axis axis, int value)
{
	EXIT_IF(g_controller == nullptr);

	g_controller->Axis(id, axis, value);
}

int KYTY_SYSV_ABI PadInit()
{
	PRINT_NAME();

	return OK;
}

int KYTY_SYSV_ABI PadOpen(int user_id, int type, int index, const void* param)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(user_id != 1);
	EXIT_NOT_IMPLEMENTED(type != 0);
	EXIT_NOT_IMPLEMENTED(index != 0);
	EXIT_NOT_IMPLEMENTED(param != nullptr);

	int handle = 1;

	return handle;
}

int KYTY_SYSV_ABI PadSetMotionSensorState(int handle, bool enable)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(handle != 1);

	printf("\t enable = %s\n", (enable ? "true" : "false"));

	return OK;
}

int KYTY_SYSV_ABI PadGetControllerInformation(int handle, PadControllerInformation* info)
{
	PRINT_NAME();

	EXIT_IF(g_controller == nullptr);

	int  connected_count = 0;
	bool connected       = false;

	g_controller->GetConnectionInfo(&connected, &connected_count);

	EXIT_NOT_IMPLEMENTED(handle != 1);
	EXIT_NOT_IMPLEMENTED(info == nullptr);

	info->touch_pixel_density   = 44.86f;
	info->touch_resolution_x    = 1920;
	info->touch_resolution_y    = 943;
	info->stick_dead_zone_left  = controller_get_axis(-32768, 32767, 8000) - 128;
	info->stick_dead_zone_right = controller_get_axis(-32768, 32767, 8000) - 128;
	info->connection_type       = 0;
	info->connected_count       = (connected_count > 255 ? 255 : connected_count);
	info->connected             = connected;
	info->device_class          = 0;

	return OK;
}

int KYTY_SYSV_ABI PadReadState(int handle, PadData* data)
{
	PRINT_NAME();

	EXIT_IF(g_controller == nullptr);

	int             connected_count = 0;
	bool            connected       = false;
	ControllerState state;

	g_controller->ReadState(&state, &connected, &connected_count);

	EXIT_NOT_IMPLEMENTED(handle != 1);
	EXIT_NOT_IMPLEMENTED(data == nullptr);

	data->buttons                = state.buttons;
	data->left_stick_x           = state.axes[static_cast<int>(Axis::LeftX)];
	data->left_stick_y           = state.axes[static_cast<int>(Axis::LeftY)];
	data->right_stick_x          = state.axes[static_cast<int>(Axis::RightX)];
	data->right_stick_y          = state.axes[static_cast<int>(Axis::RightY)];
	data->analog_buttons_l2      = state.axes[static_cast<int>(Axis::TriggerLeft)];
	data->analog_buttons_r2      = state.axes[static_cast<int>(Axis::TriggerRight)];
	data->orientation_x          = 0.0f;
	data->orientation_y          = 0.0f;
	data->orientation_z          = 0.0f;
	data->orientation_w          = 1.0f;
	data->acceleration_x         = 0.0f;
	data->acceleration_y         = 0.0f;
	data->acceleration_z         = 0.0f;
	data->angular_velocity_x     = 0.0f;
	data->angular_velocity_y     = 0.0f;
	data->angular_velocity_z     = 0.0f;
	data->touch_data_touch_num   = 0;
	data->touch_data_touch0_x    = 0;
	data->touch_data_touch0_y    = 0;
	data->touch_data_touch0_id   = 1;
	data->touch_data_touch1_x    = 0;
	data->touch_data_touch1_y    = 0;
	data->touch_data_touch1_id   = 2;
	data->connected              = connected;
	data->timestamp              = state.time;
	data->connected_count        = connected_count;
	data->device_unique_data_len = 0;

	return OK;
}

int KYTY_SYSV_ABI PadRead(int handle, PadData* data, int num)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(num < 1 || num > 64);
	EXIT_NOT_IMPLEMENTED(handle != 1);
	EXIT_NOT_IMPLEMENTED(data == nullptr);

	EXIT_IF(g_controller == nullptr);

	int             connected_count = 0;
	bool            connected       = false;
	ControllerState states[64];

	int ret_num = g_controller->ReadStates(states, num, &connected, &connected_count);

	if (!connected)
	{
		ret_num = 1;
	}

	for (int i = 0; i < ret_num; i++)
	{
		data[i].buttons                = states[i].buttons;
		data[i].left_stick_x           = states[i].axes[static_cast<int>(Axis::LeftX)];
		data[i].left_stick_y           = states[i].axes[static_cast<int>(Axis::LeftY)];
		data[i].right_stick_x          = states[i].axes[static_cast<int>(Axis::RightX)];
		data[i].right_stick_y          = states[i].axes[static_cast<int>(Axis::RightY)];
		data[i].analog_buttons_l2      = states[i].axes[static_cast<int>(Axis::TriggerLeft)];
		data[i].analog_buttons_r2      = states[i].axes[static_cast<int>(Axis::TriggerRight)];
		data[i].orientation_x          = 0.0f;
		data[i].orientation_y          = 0.0f;
		data[i].orientation_z          = 0.0f;
		data[i].orientation_w          = 1.0f;
		data[i].acceleration_x         = 0.0f;
		data[i].acceleration_y         = 0.0f;
		data[i].acceleration_z         = 0.0f;
		data[i].angular_velocity_x     = 0.0f;
		data[i].angular_velocity_y     = 0.0f;
		data[i].angular_velocity_z     = 0.0f;
		data[i].touch_data_touch_num   = 0;
		data[i].touch_data_touch0_x    = 0;
		data[i].touch_data_touch0_y    = 0;
		data[i].touch_data_touch0_id   = 1;
		data[i].touch_data_touch1_x    = 0;
		data[i].touch_data_touch1_y    = 0;
		data[i].touch_data_touch1_id   = 2;
		data[i].connected              = connected;
		data[i].timestamp              = states[i].time;
		data[i].connected_count        = connected_count;
		data[i].device_unique_data_len = 0;
	}

	return ret_num;
}

int KYTY_SYSV_ABI PadSetVibration(int handle, const PadVibrationParam* param)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(handle != 1);

	printf("\t large_motor = %d\n", static_cast<int>(param->large_motor));
	printf("\t small_motor = %d\n", static_cast<int>(param->small_motor));

	return OK;
}

int KYTY_SYSV_ABI PadResetLightBar(int handle)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(handle != 1);

	return OK;
}

int KYTY_SYSV_ABI PadSetLightBar(int handle, const PadLightBarParam* param)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(handle != 1);
	EXIT_NOT_IMPLEMENTED(param == nullptr);

	return OK;
}

} // namespace Kyty::Libs::Controller

#endif // KYTY_EMU_ENABLED
