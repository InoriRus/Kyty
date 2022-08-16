#include "Emulator/Common.h"
#include "Emulator/Graphics/Graphics.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

namespace LibGen4 {

LIB_VERSION("GraphicsDriver", 1, "GraphicsDriver", 1, 1);

namespace Gen4 = Graphics::Gen4;

LIB_DEFINE(InitGraphicsDriver_1)
{
	PRINT_NAME_ENABLE(true);

	LIB_FUNC("gAhCn6UiU4Y", Gen4::GraphicsSetVsShader);
	LIB_FUNC("V31V01UiScY", Gen4::GraphicsUpdateVsShader);
	LIB_FUNC("bQVd5YzCal0", Gen4::GraphicsSetPsShader);
	LIB_FUNC("5uFKckiJYRM", Gen4::GraphicsSetPsShader350);
	LIB_FUNC("4MgRw-bVNQU", Gen4::GraphicsUpdatePsShader);
	LIB_FUNC("mLVL7N7BVBg", Gen4::GraphicsUpdatePsShader350);
	LIB_FUNC("Kx-h-nWQJ8A", Gen4::GraphicsSetCsShaderWithModifier);
	LIB_FUNC("HlTPoZ-oY7Y", Gen4::GraphicsDrawIndex);
	LIB_FUNC("GGsn7jMTxw4", Gen4::GraphicsDrawIndexAuto);
	LIB_FUNC("zwY0YV91TTI", Gen4::GraphicsSubmitCommandBuffers);
	LIB_FUNC("xbxNatawohc", Gen4::GraphicsSubmitAndFlipCommandBuffers);
	LIB_FUNC("yvZ73uQUqrk", Gen4::GraphicsSubmitDone);
	LIB_FUNC("b08AgtPlHPg", Gen4::GraphicsAreSubmitsAllowed);
	LIB_FUNC("iBt3Oe00Kvc", Gen4::GraphicsFlushMemory);
	LIB_FUNC("b0xyllnVY-I", Gen4::GraphicsAddEqEvent);
	LIB_FUNC("PVT+fuoS9gU", Gen4::GraphicsDeleteEqEvent);
	LIB_FUNC("Idffwf3yh8s", Gen4::GraphicsDrawInitDefaultHardwareState);
	LIB_FUNC("QhnyReteJ1M", Gen4::GraphicsDrawInitDefaultHardwareState175);
	LIB_FUNC("0H2vBYbTLHI", Gen4::GraphicsDrawInitDefaultHardwareState200);
	LIB_FUNC("yb2cRhagD1I", Gen4::GraphicsDrawInitDefaultHardwareState350);
	LIB_FUNC("nF6bFRUBRAU", Gen4::GraphicsDispatchInitDefaultHardwareState);
	LIB_FUNC("1qXLHIpROPE", Gen4::GraphicsInsertWaitFlipDone);
	LIB_FUNC("0BzLGljcwBo", Gen4::GraphicsDispatchDirect);
	LIB_FUNC("29oKvKXzEZo", Gen4::GraphicsMapComputeQueue);
	LIB_FUNC("ArSg-TGinhk", Gen4::GraphicsUnmapComputeQueue);
	LIB_FUNC("ffrNQOshows", Gen4::GraphicsComputeWaitOnAddress);
	LIB_FUNC("bX5IbRvECXk", Gen4::GraphicsDingDong);
	LIB_FUNC("W1Etj-jlW7Y", Gen4::GraphicsInsertPushMarker);
	LIB_FUNC("7qZVNgEu+SY", Gen4::GraphicsInsertPopMarker);
	LIB_FUNC("+AFvOEXrKJk", Gen4::GraphicsSetEmbeddedVsShader);
	LIB_FUNC("ZFqKFl23aMc", Gen4::GraphicsRegisterOwner);
	LIB_FUNC("nvEwfYAImTs", Gen4::GraphicsRegisterResource);
	LIB_FUNC("Fwvh++m9IQI", Gen4::GraphicsGetGpuCoreClockFrequency);
	LIB_FUNC("jg33rEKLfVs", Gen4::GraphicsIsUserPaEnabled);
	LIB_FUNC("ln33zjBrfjk", Gen4::GraphicsGetTheTessellationFactorRingBufferBaseAddress);
}

} // namespace LibGen4

namespace LibGen5 {

LIB_VERSION("Graphics5", 1, "Graphics5", 1, 1);

namespace Gen5 = Graphics::Gen5;

LIB_DEFINE(InitGraphicsDriver_1)
{
	PRINT_NAME_ENABLE(true);

	LIB_FUNC("23LRUSvYu1M", Gen5::GraphicsInit);
	LIB_FUNC("2JtWUUiYBXs", Gen5::GraphicsGetRegisterDefaults2);
	LIB_FUNC("wRbq6ZjNop4", Gen5::GraphicsGetRegisterDefaults2Internal);
	LIB_FUNC("f3dg2CSgRKY", Gen5::GraphicsCreateShader);
	LIB_FUNC("vcmNN+AAXnY", Gen5::GraphicsSetCxRegIndirectPatchSetAddress);
	LIB_FUNC("Qrj4c+61z4A", Gen5::GraphicsSetShRegIndirectPatchSetAddress);
	LIB_FUNC("6lNcCp+fxi4", Gen5::GraphicsSetUcRegIndirectPatchSetAddress);
	LIB_FUNC("d-6uF9sZDIU", Gen5::GraphicsSetCxRegIndirectPatchAddRegisters);
	LIB_FUNC("z2duB-hHQSM", Gen5::GraphicsSetShRegIndirectPatchAddRegisters);
	LIB_FUNC("vRoArM9zaIk", Gen5::GraphicsSetUcRegIndirectPatchAddRegisters);
	LIB_FUNC("D9sr1xGUriE", Gen5::GraphicsCreatePrimState);
	LIB_FUNC("HV4j+E0MBHE", Gen5::GraphicsCreateInterpolantMapping);
	LIB_FUNC("V++UgBtQhn0", Gen5::GraphicsGetDataPacketPayloadAddress);
	LIB_FUNC("h9z6+0hEydk", Gen5::GraphicsSuspendPoint);

	LIB_FUNC("n2fD4A+pb+g", Gen5::GraphicsCbSetShRegisterRangeDirect);
	LIB_FUNC("wr23dPKyWc0", Gen5::GraphicsCbReleaseMem);
	LIB_FUNC("TRO721eVt4g", Gen5::GraphicsDcbResetQueue);
	LIB_FUNC("MWiElSNE8j8", Gen5::GraphicsDcbWaitUntilSafeForRendering);
	LIB_FUNC("pFLArOT53+w", Gen5::GraphicsDcbSetShRegisterDirect);
	LIB_FUNC("ZvwO9euwYzc", Gen5::GraphicsDcbSetCxRegistersIndirect);
	LIB_FUNC("-HOOCn0JY48", Gen5::GraphicsDcbSetShRegistersIndirect);
	LIB_FUNC("hvUfkUIQcOE", Gen5::GraphicsDcbSetUcRegistersIndirect);
	LIB_FUNC("GIIW2J37e70", Gen5::GraphicsDcbSetIndexSize);
	LIB_FUNC("Yw0jKSqop+E", Gen5::GraphicsDcbDrawIndexAuto);
	LIB_FUNC("aJf+j5yntiU", Gen5::GraphicsDcbEventWrite);
	LIB_FUNC("57labkp+rSQ", Gen5::GraphicsDcbAcquireMem);
	LIB_FUNC("i1jyy49AjXU", Gen5::GraphicsDcbWriteData);
	LIB_FUNC("VmW0Tdpy420", Gen5::GraphicsDcbWaitRegMem);
	LIB_FUNC("YUeqkyT7mEQ", Gen5::GraphicsDcbSetFlip);
}

} // namespace LibGen5

namespace LibGen5Driver {

LIB_VERSION("Graphics5Driver", 1, "Graphics5Driver", 1, 1);

namespace Gen5Driver = Graphics::Gen5Driver;

LIB_DEFINE(InitGraphicsDriver_1)
{
	PRINT_NAME_ENABLE(true);

	LIB_FUNC("UglJIZjGssM", Gen5Driver::GraphicsDriverSubmitDcb);
}

} // namespace LibGen5Driver

LIB_DEFINE(InitGraphicsDriver_1)
{
	LibGen4::InitGraphicsDriver_1(s);
	LibGen5::InitGraphicsDriver_1(s);
	LibGen5Driver::InitGraphicsDriver_1(s);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
