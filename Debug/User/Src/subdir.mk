################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/Src/Can_Config.c \
../User/Src/can_receive_control.c \
../User/Src/llcc68.c \
../User/Src/llcc68_app.c \
../User/Src/llcc68_driver_version.c \
../User/Src/llcc68_hal.c 

OBJS += \
./User/Src/Can_Config.o \
./User/Src/can_receive_control.o \
./User/Src/llcc68.o \
./User/Src/llcc68_app.o \
./User/Src/llcc68_driver_version.o \
./User/Src/llcc68_hal.o 

C_DEPS += \
./User/Src/Can_Config.d \
./User/Src/can_receive_control.d \
./User/Src/llcc68.d \
./User/Src/llcc68_app.d \
./User/Src/llcc68_driver_version.d \
./User/Src/llcc68_hal.d 


# Each subdirectory must supply rules for building sources it contributes
User/Src/%.o User/Src/%.su User/Src/%.cyclo: ../User/Src/%.c User/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Geen/Desktop/Wireless-main/Wireless-main/User/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-User-2f-Src

clean-User-2f-Src:
	-$(RM) ./User/Src/Can_Config.cyclo ./User/Src/Can_Config.d ./User/Src/Can_Config.o ./User/Src/Can_Config.su ./User/Src/can_receive_control.cyclo ./User/Src/can_receive_control.d ./User/Src/can_receive_control.o ./User/Src/can_receive_control.su ./User/Src/llcc68.cyclo ./User/Src/llcc68.d ./User/Src/llcc68.o ./User/Src/llcc68.su ./User/Src/llcc68_app.cyclo ./User/Src/llcc68_app.d ./User/Src/llcc68_app.o ./User/Src/llcc68_app.su ./User/Src/llcc68_driver_version.cyclo ./User/Src/llcc68_driver_version.d ./User/Src/llcc68_driver_version.o ./User/Src/llcc68_driver_version.su ./User/Src/llcc68_hal.cyclo ./User/Src/llcc68_hal.d ./User/Src/llcc68_hal.o ./User/Src/llcc68_hal.su

.PHONY: clean-User-2f-Src

