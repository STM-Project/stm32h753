################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/General_Utilities/_debug.c \
../Application/General_Utilities/common.c \
../Application/General_Utilities/double_float.c \
../Application/General_Utilities/errors_service.c \
../Application/General_Utilities/mini_printf.c \
../Application/General_Utilities/sd_card.c \
../Application/General_Utilities/string_oper.c \
../Application/General_Utilities/timer.c 

OBJS += \
./Application/General_Utilities/_debug.o \
./Application/General_Utilities/common.o \
./Application/General_Utilities/double_float.o \
./Application/General_Utilities/errors_service.o \
./Application/General_Utilities/mini_printf.o \
./Application/General_Utilities/sd_card.o \
./Application/General_Utilities/string_oper.o \
./Application/General_Utilities/timer.o 

C_DEPS += \
./Application/General_Utilities/_debug.d \
./Application/General_Utilities/common.d \
./Application/General_Utilities/double_float.d \
./Application/General_Utilities/errors_service.d \
./Application/General_Utilities/mini_printf.d \
./Application/General_Utilities/sd_card.d \
./Application/General_Utilities/string_oper.d \
./Application/General_Utilities/timer.d 


# Each subdirectory must supply rules for building sources it contributes
Application/General_Utilities/%.o Application/General_Utilities/%.su Application/General_Utilities/%.cyclo: ../Application/General_Utilities/%.c Application/General_Utilities/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Application -I../Application/General_Utilities -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-General_Utilities

clean-Application-2f-General_Utilities:
	-$(RM) ./Application/General_Utilities/_debug.cyclo ./Application/General_Utilities/_debug.d ./Application/General_Utilities/_debug.o ./Application/General_Utilities/_debug.su ./Application/General_Utilities/common.cyclo ./Application/General_Utilities/common.d ./Application/General_Utilities/common.o ./Application/General_Utilities/common.su ./Application/General_Utilities/double_float.cyclo ./Application/General_Utilities/double_float.d ./Application/General_Utilities/double_float.o ./Application/General_Utilities/double_float.su ./Application/General_Utilities/errors_service.cyclo ./Application/General_Utilities/errors_service.d ./Application/General_Utilities/errors_service.o ./Application/General_Utilities/errors_service.su ./Application/General_Utilities/mini_printf.cyclo ./Application/General_Utilities/mini_printf.d ./Application/General_Utilities/mini_printf.o ./Application/General_Utilities/mini_printf.su ./Application/General_Utilities/sd_card.cyclo ./Application/General_Utilities/sd_card.d ./Application/General_Utilities/sd_card.o ./Application/General_Utilities/sd_card.su ./Application/General_Utilities/string_oper.cyclo ./Application/General_Utilities/string_oper.d ./Application/General_Utilities/string_oper.o ./Application/General_Utilities/string_oper.su ./Application/General_Utilities/timer.cyclo ./Application/General_Utilities/timer.d ./Application/General_Utilities/timer.o ./Application/General_Utilities/timer.su

.PHONY: clean-Application-2f-General_Utilities

