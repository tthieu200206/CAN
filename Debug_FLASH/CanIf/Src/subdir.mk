################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CanIf/Src/CanIf.c 

OBJS += \
./CanIf/Src/CanIf.o 

C_DEPS += \
./CanIf/Src/CanIf.d 


# Each subdirectory must supply rules for building sources it contributes
CanIf/Src/%.o: ../CanIf/Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@CanIf/Src/CanIf.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


