#include "Emulator/Common.h"
#include "Emulator/Graphics/Graphics.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("GraphicsDriver", 1, "GraphicsDriver", 1, 1);

LIB_DEFINE(InitGraphicsDriver_1)
{
	PRINT_NAME_ENABLE(true);

	LIB_FUNC("gAhCn6UiU4Y", Graphics::GraphicsSetVsShader);
	LIB_FUNC("V31V01UiScY", Graphics::GraphicsUpdateVsShader);
	LIB_FUNC("bQVd5YzCal0", Graphics::GraphicsSetPsShader);
	LIB_FUNC("5uFKckiJYRM", Graphics::GraphicsSetPsShader350);
	LIB_FUNC("4MgRw-bVNQU", Graphics::GraphicsUpdatePsShader);
	LIB_FUNC("mLVL7N7BVBg", Graphics::GraphicsUpdatePsShader350);
	LIB_FUNC("Kx-h-nWQJ8A", Graphics::GraphicsSetCsShaderWithModifier);
	LIB_FUNC("HlTPoZ-oY7Y", Graphics::GraphicsDrawIndex);
	LIB_FUNC("GGsn7jMTxw4", Graphics::GraphicsDrawIndexAuto);
	LIB_FUNC("zwY0YV91TTI", Graphics::GraphicsSubmitCommandBuffers);
	LIB_FUNC("xbxNatawohc", Graphics::GraphicsSubmitAndFlipCommandBuffers);
	LIB_FUNC("yvZ73uQUqrk", Graphics::GraphicsSubmitDone);
	LIB_FUNC("b08AgtPlHPg", Graphics::GraphicsAreSubmitsAllowed);
	LIB_FUNC("iBt3Oe00Kvc", Graphics::GraphicsFlushMemory);
	LIB_FUNC("b0xyllnVY-I", Graphics::GraphicsAddEqEvent);
	LIB_FUNC("PVT+fuoS9gU", Graphics::GraphicsDeleteEqEvent);
	LIB_FUNC("Idffwf3yh8s", Graphics::GraphicsDrawInitDefaultHardwareState);
	LIB_FUNC("QhnyReteJ1M", Graphics::GraphicsDrawInitDefaultHardwareState175);
	LIB_FUNC("0H2vBYbTLHI", Graphics::GraphicsDrawInitDefaultHardwareState200);
	LIB_FUNC("yb2cRhagD1I", Graphics::GraphicsDrawInitDefaultHardwareState350);
	LIB_FUNC("nF6bFRUBRAU", Graphics::GraphicsDispatchInitDefaultHardwareState);
	LIB_FUNC("1qXLHIpROPE", Graphics::GraphicsInsertWaitFlipDone);
	LIB_FUNC("0BzLGljcwBo", Graphics::GraphicsDispatchDirect);
	LIB_FUNC("29oKvKXzEZo", Graphics::GraphicsMapComputeQueue);
	LIB_FUNC("ArSg-TGinhk", Graphics::GraphicsUnmapComputeQueue);
	LIB_FUNC("ffrNQOshows", Graphics::GraphicsComputeWaitOnAddress);
	LIB_FUNC("bX5IbRvECXk", Graphics::GraphicsDingDong);
	LIB_FUNC("W1Etj-jlW7Y", Graphics::GraphicsInsertPushMarker);
	LIB_FUNC("7qZVNgEu+SY", Graphics::GraphicsInsertPopMarker);
	LIB_FUNC("+AFvOEXrKJk", Graphics::GraphicsSetEmbeddedVsShader);
	LIB_FUNC("ZFqKFl23aMc", Graphics::GraphicsRegisterOwner);
	LIB_FUNC("nvEwfYAImTs", Graphics::GraphicsRegisterResource);
	LIB_FUNC("Fwvh++m9IQI", Graphics::GraphicsGetGpuCoreClockFrequency);
	LIB_FUNC("jg33rEKLfVs", Graphics::GraphicsIsUserPaEnabled);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
