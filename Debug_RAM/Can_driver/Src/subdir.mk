################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Can_driver/Src/Can.c 

OBJS += \
./Can_driver/Src/Can.o 

C_DEPS += \
./Can_driver/Src/Can.d 


# Each subdirectory must supply rules for building sources it contributes
Can_driver/Src/%.o: ../Can_driver/Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@Can_driver/Src/Can.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


