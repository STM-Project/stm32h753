
#include "stm32h7xx_hal.h"
#include "stmpe811.h"
#include "i2c.h"

#define STMPE811_ID                     0x0811

/* Identification registers & System Control */
#define STMPE811_REG_CHP_ID_LSB         0x00
#define STMPE811_REG_CHP_ID_MSB         0x01
#define STMPE811_REG_ID_VER             0x02

/* Global interrupt Enable bit */
#define STMPE811_GIT_EN                 0x01

/* IO expander functionalities */
#define STMPE811_ADC_FCT                0x01
#define STMPE811_TS_FCT                 0x02
#define STMPE811_IO_FCT                 0x04
#define STMPE811_TEMPSENS_FCT           0x08

/* Global Interrupts definitions */
#define STMPE811_GIT_IO                 0x80  /* IO interrupt                   */
#define STMPE811_GIT_ADC                0x40  /* ADC interrupt                  */
#define STMPE811_GIT_TEMP               0x20  /* Not implemented                */
#define STMPE811_GIT_FE                 0x10  /* FIFO empty interrupt           */
#define STMPE811_GIT_FF                 0x08  /* FIFO full interrupt            */
#define STMPE811_GIT_FOV                0x04  /* FIFO overflowed interrupt      */
#define STMPE811_GIT_FTH                0x02  /* FIFO above threshold interrupt */
#define STMPE811_GIT_TOUCH              0x01  /* Touch is detected interrupt    */
#define STMPE811_ALL_GIT                0x1F  /* All global interrupts          */
#define STMPE811_TS_IT                  (STMPE811_GIT_TOUCH | STMPE811_GIT_FTH |  STMPE811_GIT_FOV | STMPE811_GIT_FF | STMPE811_GIT_FE) /* Touch screen interrupts */

/* General Control Registers */
#define STMPE811_REG_SYS_CTRL1          0x03
#define STMPE811_REG_SYS_CTRL2          0x04
#define STMPE811_REG_SPI_CFG            0x08

/* Interrupt system Registers */
#define STMPE811_REG_INT_CTRL           0x09
#define STMPE811_REG_INT_EN             0x0A
#define STMPE811_REG_INT_STA            0x0B
#define STMPE811_REG_IO_INT_EN          0x0C
#define STMPE811_REG_IO_INT_STA         0x0D

/* IO Registers */
#define STMPE811_REG_IO_SET_PIN         0x10
#define STMPE811_REG_IO_CLR_PIN         0x11
#define STMPE811_REG_IO_MP_STA          0x12
#define STMPE811_REG_IO_DIR             0x13
#define STMPE811_REG_IO_ED              0x14
#define STMPE811_REG_IO_RE              0x15
#define STMPE811_REG_IO_FE              0x16
#define STMPE811_REG_IO_AF              0x17

/* ADC Registers */
#define STMPE811_REG_ADC_INT_EN         0x0E
#define STMPE811_REG_ADC_INT_STA        0x0F
#define STMPE811_REG_ADC_CTRL1          0x20
#define STMPE811_REG_ADC_CTRL2          0x21
#define STMPE811_REG_ADC_CAPT           0x22
#define STMPE811_REG_ADC_DATA_CH0       0x30
#define STMPE811_REG_ADC_DATA_CH1       0x32
#define STMPE811_REG_ADC_DATA_CH2       0x34
#define STMPE811_REG_ADC_DATA_CH3       0x36
#define STMPE811_REG_ADC_DATA_CH4       0x38
#define STMPE811_REG_ADC_DATA_CH5       0x3A
#define STMPE811_REG_ADC_DATA_CH6       0x3B
#define STMPE811_REG_ADC_DATA_CH7       0x3C

/* Touch Screen Registers */
#define STMPE811_REG_TSC_CTRL           0x40
#define STMPE811_REG_TSC_CFG            0x41
#define STMPE811_REG_WDM_TR_X           0x42
#define STMPE811_REG_WDM_TR_Y           0x44
#define STMPE811_REG_WDM_BL_X           0x46
#define STMPE811_REG_WDM_BL_Y           0x48
#define STMPE811_REG_FIFO_TH            0x4A
#define STMPE811_REG_FIFO_STA           0x4B
#define STMPE811_REG_FIFO_SIZE          0x4C
#define STMPE811_REG_TSC_DATA_X         0x4D
#define STMPE811_REG_TSC_DATA_Y         0x4F
#define STMPE811_REG_TSC_DATA_Z         0x51
#define STMPE811_REG_TSC_DATA_XYZ       0x52
#define STMPE811_REG_TSC_FRACT_XYZ      0x56
#define STMPE811_REG_TSC_DATA_INC       0x57
#define STMPE811_REG_TSC_DATA_NON_INC   0xD7
#define STMPE811_REG_TSC_I_DRIVE        0x58
#define STMPE811_REG_TSC_SHIELD         0x59

/* Touch Screen Pins definition */
#define STMPE811_TOUCH_YD               STMPE811_PIN_7
#define STMPE811_TOUCH_XD               STMPE811_PIN_6
#define STMPE811_TOUCH_YU               STMPE811_PIN_5
#define STMPE811_TOUCH_XU               STMPE811_PIN_4
#define STMPE811_TOUCH_IO_ALL           (uint32_t)(STMPE811_TOUCH_YD | STMPE811_TOUCH_XD | STMPE811_TOUCH_YU | STMPE811_TOUCH_XU)

/* IO Pins definition */
#define STMPE811_PIN_0                  0x01
#define STMPE811_PIN_1                  0x02
#define STMPE811_PIN_2                  0x04
#define STMPE811_PIN_3                  0x08
#define STMPE811_PIN_4                  0x10
#define STMPE811_PIN_5                  0x20
#define STMPE811_PIN_6                  0x40
#define STMPE811_PIN_7                  0x80
#define STMPE811_PIN_ALL                0xFF

/* IO Pins directions */
#define STMPE811_DIRECTION_IN           0x00
#define STMPE811_DIRECTION_OUT          0x01

/* IO IT types */
#define STMPE811_TYPE_LEVEL             0x00
#define STMPE811_TYPE_EDGE              0x02

/* IO IT polarity */
#define STMPE811_POLARITY_LOW           0x00
#define STMPE811_POLARITY_HIGH          0x04

/* IO Pin IT edge modes */
#define STMPE811_EDGE_FALLING           0x01
#define STMPE811_EDGE_RISING            0x02

/* TS registers masks */
#define STMPE811_TS_CTRL_ENABLE         0x01
#define STMPE811_TS_CTRL_STATUS         0x80

#define I2C_TIMEOUT  100 /*<! Value of Timeout when I2C communication fails */

#define TS_I2C_ADDRESS                  0x82

#define TS_SWAP_NONE                    0x00
#define TS_SWAP_X                       0x01
#define TS_SWAP_Y                       0x02
#define TS_SWAP_XY                      0x04

#define TOUCH_RESOLUTION	1

typedef enum
{
   TS_OK       = 0x00,
   TS_ERROR    = 0x01,
   TS_TIMEOUT  = 0x02
}TS_StatusTypeDef;

typedef enum
{
  IO_PIN_RESET = 0,
  IO_PIN_SET
}IO_PinState;

typedef struct
{
  void       (*Init)(uint16_t);
  uint16_t   (*ReadID)(uint16_t);
  void       (*Reset)(uint16_t);
  void       (*Start)(uint16_t, uint32_t);
  void       (*WritePin)(uint16_t, uint32_t, uint8_t);
  uint32_t   (*ReadPin)(uint16_t, uint32_t);
  void       (*EnableIT)(uint16_t);
  void       (*ClearIT)(uint16_t, uint32_t);

}IO_DrvTypeDef;

typedef struct
{
  void       (*Init)(uint16_t);
  uint16_t   (*ReadID)(uint16_t);
  void       (*Reset)(uint16_t);
  void       (*Start)(uint16_t);
  uint8_t    (*DetectTouch)(uint16_t);
  void       (*GetXY)(uint16_t, uint16_t*, uint16_t*);
  void       (*EnableIT)(uint16_t);
  void       (*ClearIT)(uint16_t);
}TS_DrvTypeDef;

static TS_DrvTypeDef *ts_driver;
static uint8_t io_failed;
static uint16_t ts_x_boundary, ts_y_boundary;
static uint8_t  ts_orientation;

static uint8_t touchResolution = TOUCH_RESOLUTION;

TS_StateTypeDef  TS_State;

static void IOE_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}

/* Startup recovery only: no touch task or other I2C1 user may run here.
 * PB7=SDA, PB8=SCL, matching the board's CubeMX I2C1 mapping.
 * Open-drain high releases the line; never drive against the controller. */
static int I2C1_WaitSclHigh(void)
{
  for(unsigned wait=0; wait<5; ++wait){
    if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8)==GPIO_PIN_SET)return 1;
    HAL_Delay(1);
  }
  return HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8)==GPIO_PIN_SET;
}

uint8_t BSP_TS_RecoverBus(void)
{
  GPIO_InitTypeDef pins={0};
  int released=0;
  HAL_I2C_DeInit(&hi2c1);
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7|GPIO_PIN_8,GPIO_PIN_SET);
  pins.Pin=GPIO_PIN_7|GPIO_PIN_8;
  pins.Mode=GPIO_MODE_OUTPUT_OD;
  pins.Pull=GPIO_NOPULL;
  pins.Speed=GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB,&pins);
  HAL_Delay(1);
  if(!I2C1_WaitSclHigh())goto restore;
  for(unsigned pulse=0; pulse<9 && HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_7)==GPIO_PIN_RESET; ++pulse){
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);
    if(!I2C1_WaitSclHigh())goto restore;
    HAL_Delay(1);
  }
  /* STOP: SDA rises while SCL is high. Lower SCL first to avoid START. */
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);
  if(!I2C1_WaitSclHigh())goto restore;
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET);
  HAL_Delay(1);
  released=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_7)==GPIO_PIN_SET;
restore:
  /* Always return pins to their CubeMX alternate function, even if stuck. */
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7|GPIO_PIN_8,GPIO_PIN_SET);
  __HAL_RCC_I2C1_FORCE_RESET();
  __HAL_RCC_I2C1_RELEASE_RESET();
  MX_I2C1_Init();
  return (uint8_t)released;
}

static void I2Cx_Error(uint8_t Addr)
{
  io_failed = 1;
  /* De-initialize the IOE comunication BUS */
  HAL_I2C_DeInit(&hi2c1);

  /* Re-Initiaize the IOE comunication BUS */
  MX_I2C1_Init();
}

static uint8_t I2Cx_Read(uint8_t Addr, uint8_t Reg)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint8_t Value = 0;

  status = HAL_I2C_Mem_Read(&hi2c1, Addr, Reg, I2C_MEMADD_SIZE_8BIT, &Value, 1, I2C_TIMEOUT);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* Execute user timeout callback */
    I2Cx_Error(Addr);
  }

  return Value;
}

static void I2Cx_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Write(&hi2c1, Addr, (uint16_t)Reg, I2C_MEMADD_SIZE_8BIT, &Value, 1, I2C_TIMEOUT);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* I2C error occured */
    I2Cx_Error(Addr);
  }
}

void IOE_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
  I2Cx_Write(Addr, Reg, Value);
}

uint8_t IOE_Read(uint8_t Addr, uint8_t Reg)
{
  return I2Cx_Read(Addr, Reg);
}

static HAL_StatusTypeDef I2Cx_ReadMultiple(uint8_t Addr, uint16_t Reg, uint16_t MemAddress, uint8_t *Buffer, uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_I2C_Mem_Read(&hi2c1, Addr, (uint16_t)Reg, MemAddress, Buffer, Length, I2C_TIMEOUT);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* I2C error occured */
    I2Cx_Error(Addr);
  }
  return status;
}

static void stmpe811_IO_DisableAF(uint16_t DeviceAddr, uint32_t IO_Pin)
{
  uint8_t tmp = 0;

  /* Get the current state of the IO_AF register */
  tmp = IOE_Read(DeviceAddr, STMPE811_REG_IO_AF);

  /* Enable the selected pins alternate function */
  tmp |= (uint8_t)IO_Pin;

  /* Write back the new value in IO AF register */
  IOE_Write(DeviceAddr, STMPE811_REG_IO_AF, tmp);

}

static void stmpe811_IO_EnableAF(uint16_t DeviceAddr, uint32_t IO_Pin)
{
  uint8_t tmp = 0;

  /* Get the current register value */
  tmp = IOE_Read(DeviceAddr, STMPE811_REG_IO_AF);

  /* Enable the selected pins alternate function */
  tmp &= ~(uint8_t)IO_Pin;

  /* Write back the new register value */
  IOE_Write(DeviceAddr, STMPE811_REG_IO_AF, tmp);
}

static void stmpe811_Reset(uint16_t DeviceAddr)
{
  /* Power Down the stmpe811 */
  IOE_Write(DeviceAddr, STMPE811_REG_SYS_CTRL1, 2);

  /* Wait for a delay to ensure registers erasing */
  IOE_Delay(10);

  /* Power On the Codec after the power off => all registers are reinitialized */
  IOE_Write(DeviceAddr, STMPE811_REG_SYS_CTRL1, 0);

  /* Wait for a delay to ensure registers erasing */
  IOE_Delay(2);
}

static void IOE_Init(void)
{
	MX_I2C1_Init();
}

static void stmpe811_Init(uint16_t DeviceAddr)
{
   /* Initialize IO BUS layer */
   IOE_Init();
      
   /* Generate stmpe811 Software reset */
   stmpe811_Reset(DeviceAddr);
}

static uint16_t stmpe811_ReadID(uint16_t DeviceAddr)
{
  /* Initialize IO BUS layer */
  IOE_Init(); 
  
  /* Return the device ID value */
  return ((IOE_Read(DeviceAddr, STMPE811_REG_CHP_ID_LSB) << 8) |\
          (IOE_Read(DeviceAddr, STMPE811_REG_CHP_ID_MSB)));
}

static void stmpe811_EnableGlobalIT(uint16_t DeviceAddr)
{
  uint8_t tmp = 0;
  
  /* Read the Interrupt Control register  */
  tmp = IOE_Read(DeviceAddr, STMPE811_REG_INT_CTRL);
  
  /* Set the global interrupts to be Enabled */    
  tmp |= (uint8_t)STMPE811_GIT_EN;
  
  /* Write Back the Interrupt Control register */
  IOE_Write(DeviceAddr, STMPE811_REG_INT_CTRL, tmp); 
}

static void stmpe811_EnableITSource(uint16_t DeviceAddr, uint8_t Source)
{
  uint8_t tmp = 0;
  
  /* Get the current value of the INT_EN register */
  tmp = IOE_Read(DeviceAddr, STMPE811_REG_INT_EN);

  /* Set the interrupts to be Enabled */    
  tmp |= Source; 
  
  /* Set the register */
  IOE_Write(DeviceAddr, STMPE811_REG_INT_EN, tmp);   
}

static void stmpe811_ClearGlobalIT(uint16_t DeviceAddr, uint8_t Source)
{
  /* Write 1 to the bits that have to be cleared */
  IOE_Write(DeviceAddr, STMPE811_REG_INT_STA, Source);
}

static void stmpe811_IO_Start(uint16_t DeviceAddr, uint32_t IO_Pin)
{
  uint8_t mode;
  
  /* Get the current register value */
  mode = IOE_Read(DeviceAddr, STMPE811_REG_SYS_CTRL2);
  
  /* Set the Functionalities to be Disabled */    
  mode &= ~(STMPE811_IO_FCT | STMPE811_ADC_FCT);  
  
  /* Write the new register value */  
  IOE_Write(DeviceAddr, STMPE811_REG_SYS_CTRL2, mode); 

  /* Disable AF for the selected IO pin(s) */
  stmpe811_IO_DisableAF(DeviceAddr, (uint8_t)IO_Pin);
}

static void stmpe811_IO_WritePin(uint16_t DeviceAddr, uint32_t IO_Pin, uint8_t PinState)
{
  /* Apply the bit value to the selected pin */
  if (PinState != 0)
  {
    /* Set the register */
    IOE_Write(DeviceAddr, STMPE811_REG_IO_SET_PIN, (uint8_t)IO_Pin);
  }
  else
  {
    /* Set the register */
    IOE_Write(DeviceAddr, STMPE811_REG_IO_CLR_PIN, (uint8_t)IO_Pin);
  } 
}

static uint32_t stmpe811_IO_ReadPin(uint16_t DeviceAddr, uint32_t IO_Pin)
{
  return((uint32_t)(IOE_Read(DeviceAddr, STMPE811_REG_IO_MP_STA) & (uint8_t)IO_Pin));
}

static uint16_t IOE_ReadMultiple(uint8_t Addr, uint8_t Reg, uint8_t *Buffer, uint16_t Length)
{
	return I2Cx_ReadMultiple(Addr, Reg, I2C_MEMADD_SIZE_8BIT, Buffer, Length);
}

static void stmpe811_TS_Start(uint16_t DeviceAddr)
{
  uint8_t mode;
  
  /* Get the current register value */
  mode = IOE_Read(DeviceAddr, STMPE811_REG_SYS_CTRL2);
  
  /* Set the Functionalities to be Enabled */    
  mode &= ~(STMPE811_IO_FCT);  
  
  /* Write the new register value */  
  IOE_Write(DeviceAddr, STMPE811_REG_SYS_CTRL2, mode); 

  /* Select TSC pins in TSC alternate mode */  
  stmpe811_IO_EnableAF(DeviceAddr, STMPE811_TOUCH_IO_ALL);
  
  /* Set the Functionalities to be Enabled */    
  mode &= ~(STMPE811_TS_FCT | STMPE811_ADC_FCT);  
  
  /* Set the new register value */  
  IOE_Write(DeviceAddr, STMPE811_REG_SYS_CTRL2, mode); 
  
  /* Select Sample Time, bit number and ADC Reference */
  IOE_Write(DeviceAddr, STMPE811_REG_ADC_CTRL1, 0x49);
  
  /* Wait for 2 ms */
  IOE_Delay(2); 
  
  /* Select the ADC clock speed: 3.25 MHz */
  IOE_Write(DeviceAddr, STMPE811_REG_ADC_CTRL2, 0x01);
  
  /* Select 2 nF filter capacitor */
  /* Configuration: 
     - Touch average control    : 4 samples
     - Touch delay time         : 500 uS
     - Panel driver setting time: 500 uS 
  */
  IOE_Write(DeviceAddr, STMPE811_REG_TSC_CFG, 0x9A); 
  
  /* Configure the Touch FIFO threshold: single point reading */
  IOE_Write(DeviceAddr, STMPE811_REG_FIFO_TH, 0x01);
  
  /* Clear the FIFO memory content. */
  IOE_Write(DeviceAddr, STMPE811_REG_FIFO_STA, 0x01);
  
  /* Put the FIFO back into operation mode  */
  IOE_Write(DeviceAddr, STMPE811_REG_FIFO_STA, 0x00);
  
  /* Set the range and accuracy pf the pressure measurement (Z) : 
     - Fractional part :7 
     - Whole part      :1 
  */
  IOE_Write(DeviceAddr, STMPE811_REG_TSC_FRACT_XYZ, 0x01);
  
  /* Set the driving capability (limit) of the device for TSC pins: 50mA */
  IOE_Write(DeviceAddr, STMPE811_REG_TSC_I_DRIVE, 0x01);
  
  /* Touch screen control configuration (enable TSC):
     - No window tracking index
     - XYZ acquisition mode
   */
  IOE_Write(DeviceAddr, STMPE811_REG_TSC_CTRL, 0x01);
  
  /*  Clear all the status pending bits if any */
  IOE_Write(DeviceAddr, STMPE811_REG_INT_STA, 0xFF);

  /* Wait for 2 ms delay */
  IOE_Delay(2); 
}

static uint8_t stmpe811_TS_DetectTouch(uint16_t DeviceAddr)
{
  uint8_t state;
  uint8_t ret = 0;
  
  state = ((IOE_Read(DeviceAddr, STMPE811_REG_TSC_CTRL) & (uint8_t)STMPE811_TS_CTRL_STATUS) == (uint8_t)0x80);
  
  if(state > 0)
  {
    if(IOE_Read(DeviceAddr, STMPE811_REG_FIFO_SIZE) > 0)
    {
      ret = 1;
    }
  }
  else
  {
    /* Reset FIFO */
    IOE_Write(DeviceAddr, STMPE811_REG_FIFO_STA, 0x01);
    /* Enable the FIFO again */
    IOE_Write(DeviceAddr, STMPE811_REG_FIFO_STA, 0x00);
  }
  
  return ret;
}

static void stmpe811_TS_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y)
{
  uint8_t  dataXYZ[4];
  uint32_t uldataXYZ;
  
  IOE_ReadMultiple(DeviceAddr, STMPE811_REG_TSC_DATA_NON_INC, dataXYZ, sizeof(dataXYZ)) ;
  
  /* Calculate positions values */
  uldataXYZ = (dataXYZ[0] << 24)|(dataXYZ[1] << 16)|(dataXYZ[2] << 8)|(dataXYZ[3] << 0);     
  *X = (uldataXYZ >> 20) & 0x00000FFF;     
  *Y = (uldataXYZ >>  8) & 0x00000FFF;     
  
  /* Reset FIFO */
  IOE_Write(DeviceAddr, STMPE811_REG_FIFO_STA, 0x01);
  /* Enable the FIFO again */
  IOE_Write(DeviceAddr, STMPE811_REG_FIFO_STA, 0x00);
}

static void stmpe811_TS_EnableIT(uint16_t DeviceAddr)
{
  /* Enable global TS IT source */
  stmpe811_EnableITSource(DeviceAddr, STMPE811_TS_IT); 
  
  /* Enable global interrupt */
  stmpe811_EnableGlobalIT(DeviceAddr);
}

static void stmpe811_TS_ClearIT(uint16_t DeviceAddr)
{
  /* Clear the global TS IT source */
  stmpe811_ClearGlobalIT(DeviceAddr, STMPE811_TS_IT);
}

/* Touch screen driver structure initialization */
TS_DrvTypeDef stmpe811_ts_drv =
{
  stmpe811_Init,
  stmpe811_ReadID,
  stmpe811_Reset,
  stmpe811_TS_Start,
  stmpe811_TS_DetectTouch,
  stmpe811_TS_GetXY,
  stmpe811_TS_EnableIT,
  stmpe811_TS_ClearIT,
};

/* IO driver structure initialization */
IO_DrvTypeDef stmpe811_io_drv =
{
  stmpe811_Init,
  stmpe811_ReadID,
  stmpe811_Reset,
  stmpe811_IO_Start,
  stmpe811_IO_WritePin,
  stmpe811_IO_ReadPin,
};

uint8_t BSP_TS_Init(uint16_t xSize, uint16_t ySize)
{
  uint8_t ret = TS_ERROR;
  io_failed = 0;
  ts_driver = 0;

  if(stmpe811_ts_drv.ReadID(TS_I2C_ADDRESS) == STMPE811_ID)
  {
    /* Initialize the TS driver structure */
    ts_driver = &stmpe811_ts_drv;

    /* Initialize x and y positions boundaries */
    ts_x_boundary  = xSize;
    ts_y_boundary  = ySize;
    ts_orientation = TS_SWAP_Y;
    ret = TS_OK;
  }

  if(ret == TS_OK)
  {
    /* Initialize the LL TS Driver */
    ts_driver->Init(TS_I2C_ADDRESS);
    ts_driver->Start(TS_I2C_ADDRESS);
    ts_driver->ClearIT(TS_I2C_ADDRESS);
    ts_driver->EnableIT(TS_I2C_ADDRESS);
    /* An ID response alone does not prove configuration writes succeeded. */
    uint8_t ctrl=IOE_Read(TS_I2C_ADDRESS, STMPE811_REG_TSC_CTRL);
    uint8_t irq=IOE_Read(TS_I2C_ADDRESS, STMPE811_REG_INT_CTRL);
    if(io_failed || !(ctrl & STMPE811_TS_CTRL_ENABLE) || !(irq & STMPE811_GIT_EN)){
      ts_driver = 0;
      ret = TS_ERROR;
    }
  }

  return ret;
}

uint8_t BSP_TS_GetState(TS_StateTypeDef *TS_State)
{
  static uint32_t _x = 0, _y = 0, x_p=0, y_p=0;
  uint16_t xDiff, yDiff , x , y;
  uint16_t swap;

  if(!ts_driver){TS_State->TouchDetected=0;return TS_ERROR;}
  TS_State->TouchDetected = ts_driver->DetectTouch(TS_I2C_ADDRESS);

  if(TS_State->TouchDetected)
  {
    ts_driver->GetXY(TS_I2C_ADDRESS, &x, &y);

    if(ts_orientation & TS_SWAP_X)
    {
      x = 4096 - x;
    }

    if(ts_orientation & TS_SWAP_Y)
    {
      y = 4096 - y;
    }

    if(ts_orientation & TS_SWAP_XY)
    {
      swap = y;
      y = x;
      x = swap;
    }

    _x = (ts_x_boundary * x) >> 12;
    _y = (ts_y_boundary * y) >> 12;

    xDiff = x_p > _x? (x_p - _x): (_x - x_p);
    yDiff = y_p > _y? (y_p - _y): (_y - y_p);

    if (xDiff > touchResolution || yDiff > touchResolution )
    {
      x_p = _x;
      y_p = _y;
    }

    TS_State->x = x_p;
    TS_State->y = y_p;
  }

  return TS_OK;
}

void BSP_TS_ClearIT(void)
{
    ts_driver = &stmpe811_ts_drv;
    ts_driver->ClearIT(TS_I2C_ADDRESS);
}

void TOUCH_SetNewResolution(uint8_t resol){
	touchResolution = resol;
}
void TOUCH_SetDefaultResolution(void){
	touchResolution = TOUCH_RESOLUTION;
}
uint8_t* TOUCH_GetPtr2Resolution(void){
	return &touchResolution;
}

