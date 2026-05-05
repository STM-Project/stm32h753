################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/_smtp.c \
../Application/esp32wroom.c \
../Application/sntp_dns.c \
../Application/variables.c \
../Application/wwwPages.c 

OBJS += \
./Application/_smtp.o \
./Application/esp32wroom.o \
./Application/sntp_dns.o \
./Application/variables.o \
./Application/wwwPages.o 

C_DEPS += \
./Application/_smtp.d \
./Application/esp32wroom.d \
./Application/sntp_dns.d \
./Application/variables.d \
./Application/wwwPages.d 


# Each subdirectory must supply rules for building sources it contributes
Application/%.o Application/%.su Application/%.cyclo: ../Application/%.c Application/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H753xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Application -I../Application/General_Utilities -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application

clean-Application:
	-$(RM) ./Application/_smtp.cyclo ./Application/_smtp.d ./Application/_smtp.o ./Application/_smtp.su ./Application/esp32wroom.cyclo ./Application/esp32wroom.d ./Application/esp32wroom.o ./Application/esp32wroom.su ./Application/sntp_dns.cyclo ./Application/sntp_dns.d ./Application/sntp_dns.o ./Application/sntp_dns.su ./Application/variables.cyclo ./Application/variables.d ./Application/variables.o ./Application/variables.su ./Application/wwwPages.cyclo ./Application/wwwPages.d ./Application/wwwPages.o ./Application/wwwPages.su

.PHONY: clean-Application

