################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/modbus/mb_ip_auth.c \
../src/modbus/mb_log.c \
../src/modbus/mb_pdu.c \
../src/modbus/mb_rtu_adu.c \
../src/modbus/mb_rtu_con.c \
../src/modbus/mb_rtu_master.c \
../src/modbus/mb_rtu_slave.c \
../src/modbus/mb_tcp_adu.c \
../src/modbus/mb_tcp_client.c \
../src/modbus/mb_tcp_con.c \
../src/modbus/mb_tcp_server.c 

OBJS += \
./src/modbus/mb_ip_auth.o \
./src/modbus/mb_log.o \
./src/modbus/mb_pdu.o \
./src/modbus/mb_rtu_adu.o \
./src/modbus/mb_rtu_con.o \
./src/modbus/mb_rtu_master.o \
./src/modbus/mb_rtu_slave.o \
./src/modbus/mb_tcp_adu.o \
./src/modbus/mb_tcp_client.o \
./src/modbus/mb_tcp_con.o \
./src/modbus/mb_tcp_server.o 

C_DEPS += \
./src/modbus/mb_ip_auth.d \
./src/modbus/mb_log.d \
./src/modbus/mb_pdu.d \
./src/modbus/mb_rtu_adu.d \
./src/modbus/mb_rtu_con.d \
./src/modbus/mb_rtu_master.d \
./src/modbus/mb_rtu_slave.d \
./src/modbus/mb_tcp_adu.d \
./src/modbus/mb_tcp_client.d \
./src/modbus/mb_tcp_con.d \
./src/modbus/mb_tcp_server.d 


# Each subdirectory must supply rules for building sources it contributes
src/modbus/%.o: ../src/modbus/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v8 Linux g++ compiler'
	aarch64-linux-gnu-g++ -Wall -O0 -g3 -I"E:\Alien\FPGA\00.NCKU\rootfs\usr\include" -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


