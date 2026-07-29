################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/bsp.c \
../src/fonts.c \
../src/game.c \
../src/lcd.c \
../src/main.c \
../src/qepn.c \
../src/qfn.c \
../src/qfn_init.c 

OBJS += \
./src/bsp.o \
./src/fonts.o \
./src/game.o \
./src/lcd.o \
./src/main.o \
./src/qepn.o \
./src/qfn.o \
./src/qfn_init.o 

C_DEPS += \
./src/bsp.d \
./src/fonts.d \
./src/game.d \
./src/lcd.d \
./src/main.d \
./src/qepn.d \
./src/qfn.d \
./src/qfn_init.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	mb-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -I/home/btjanicki/workspace/BTJ_Grad_Plat_proj/export/BTJ_Grad_Plat_proj/sw/BTJ_Grad_Plat_proj/standalone_domain/bspinclude/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v11.0 -mno-xl-soft-mul -mhard-float -mxl-float-convert -mxl-float-sqrt -Wl,--no-relax -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


