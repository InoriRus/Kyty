#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICSRUN_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICSRUN_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

class CommandProcessor;

void GraphicsRunInit();

void     GraphicsRunSubmit(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw);
void     GraphicsRunSubmitAndFlip(uint32_t* cmd_draw_buffer, uint32_t num_draw_dw, uint32_t* cmd_const_buffer, uint32_t num_const_dw,
                                  int handle, int index, int flip_mode, int64_t flip_arg);
uint32_t GraphicsRunMapComputeQueue(uint32_t pipe_id, uint32_t queue_id, uint32_t* ring_addr, uint32_t ring_size_dw,
                                    uint32_t* read_ptr_addr);
void     GraphicsRunUnmapComputeQueue(uint32_t id);
void     GraphicsRunWait();
void     GraphicsRunDone();
void     GraphicsRunDingDong(uint32_t ring_id, uint32_t offset_dw);
int      GraphicsRunGetFrameNum();
bool     GraphicsRunAreSubmitsAllowed();

void GraphicsRunCommandProcessorLock(CommandProcessor* cp);
void GraphicsRunCommandProcessorUnlock(CommandProcessor* cp);
void GraphicsRunCommandProcessorFlush(CommandProcessor* cp);
void GraphicsRunCommandProcessorWait(CommandProcessor* cp);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICSRUN_H_ */
