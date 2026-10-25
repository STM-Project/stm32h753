#include "network_memory.h"
#include "main.h"
/* Network-only overlay; existing LCD/SDRAM/D1/D2 policies are unchanged.
 * Called before any network memory/DMA is used. CubeMX may also configure
 * these same regions, in which case this call is idempotent. */
void NetworkConfigureMemory ( void )
{
	MPU_Region_InitTypeDef region = { 0 };
	region.Enable = MPU_REGION_ENABLE;
	region.Number = MPU_REGION_NUMBER5;
	region.BaseAddress = 0x30040000;
	region.Size = MPU_REGION_SIZE_32KB;
	region.TypeExtField = MPU_TEX_LEVEL1;
	region.AccessPermission = MPU_REGION_FULL_ACCESS;
	region.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
	region.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	region.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	region.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	HAL_MPU_Disable ();
	HAL_MPU_ConfigRegion ( &region );
	region.Number = MPU_REGION_NUMBER6;
	region.Size = MPU_REGION_SIZE_256B;
	region.TypeExtField = MPU_TEX_LEVEL0;
	region.IsShareable = MPU_ACCESS_SHAREABLE;
	region.IsBufferable = MPU_ACCESS_BUFFERABLE;
	HAL_MPU_ConfigRegion ( &region );
	HAL_MPU_Enable ( MPU_PRIVILEGED_DEFAULT );
}
