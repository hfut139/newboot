# 是否编译bootloader（y:是；n:否）
CONFIG_BOOTLOADER := y

# 禁用隐含规则
MAKEFLAGS += -rR

# 项目名称
PROJECT_NAME := xcplus

# 目标
ifeq ($(CONFIG_BOOTLOADER), y)  #如果它等于y，则编译boot目标，否则编译app
TARGET ?= boot
else
TARGET ?= app
endif

# 输出路径
BUILD_BASE := output
ifeq ($(CONFIG_BOOTLOADER), y)
BUILD := $(BUILD_BASE)/$(PROJECT_NAME)_boot
else
BUILD := $(BUILD_BASE)/$(PROJECT_NAME)
endif

# 生成配置
# DEBUG ?= $(CONFIG_DEBUG)
DEBUG ?=
V ?=

# Toolchain
CROSS_COMPILE ?= tools/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-
KCONF := scripts/kconfig/
ECHO := echo
CP := cp
MKDIR := mkdir -p
PYTHON := python3
OS = $(shell uname -s)
ifeq ($(V),)
QUITE := @
endif

# 平台配置
PLATFORM := stm32f4
MCU := -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard  #Cortex-M4硬件浮点
LDSCRIPT := platform/stm32f407vetx_flash.ld  # 链接脚本

# 库文件
LIBS := -lm

# 宏定义
P_DEF :=
P_DEF += STM32F40_41xxx \
		 USE_STDPERIPH_DRIVER \
         HSE_VALUE=8000000
ifeq ($(DEBUG), y)
P_DEF += DEBUG
endif

s_inc-y = boot \
		  component/crc \
		  component/easylogger/inc \
		  component/ringbuffer \
		  platform/cmsis/core \
		  platform/cmsis/device \
		  platform/driver/inc
s_dir-y = boot \
		  boot/override \
		  component/crc \
		  component/ringbuffer \
		  platform/cmsis/device \
		  platform/driver/src

ifeq ($(DEBUG), y)
s_dir-y += component/easylogger/src \
           component/easylogger/port
endif


s_src-y = $(wildcard $(addsuffix /*.c, $(s_dir-y)))
s_src-y += platform/cmsis/device/startup_stm32f40_41xxx.s

P_INC := $(sort $(s_inc-y))  #去重排序
P_SRC := $(sort $(s_src-y))

# 编译标记
A_FLAGS  = $(MCU) -g
C_FLAGS  = $(MCU) -g
C_FLAGS += -std=gnu11
C_FLAGS += -ffunction-sections -fdata-sections -fno-builtin -fno-strict-aliasing
C_FLAGS += -Wall -Werror
# C_FLAGS += -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
ifeq ($(DEBUG), y)
C_FLAGS += -Wno-comment -Wno-unused-value -Wno-unused-variable -Wno-unused-function
endif
C_FLAGS += -MMD -MP -MF"$(@:%.o=%.d)"  #自动生成 .d 文件（依赖追踪）

ifeq ($(DEBUG), y)
C_FLAGS += -O0  #无优化
else
C_FLAGS += -Os  #优化空间
endif

# 链接标记
L_FLAGS  = $(MCU)
L_FLAGS += -T$(LDSCRIPT)
L_FLAGS += -static -specs=nano.specs
L_FLAGS += -Wl,-cref
L_FLAGS += -Wl,-Map=$(BUILD)/$(TARGET).map  # 生成.map文件
L_FLAGS += -Wl,--gc-sections
L_FLAGS += -Wl,--start-group
L_FLAGS += $(LIBS)
L_FLAGS += -Wl,--end-group

# 依赖对象
B_LIB := $(addprefix $(BUILD)/, $(P_LIB))
B_INC := $(addprefix -I, $(P_INC))
B_DEF := $(addprefix -D, $(P_DEF))
B_OBJ := $(P_SRC)
B_OBJ := $(B_OBJ:.c=.o)
B_OBJ := $(B_OBJ:.s=.o)
B_OBJ := $(addprefix $(BUILD)/, $(B_OBJ))  #B_OBJ 是最终生成的目标文件列表
B_DEP := $(B_OBJ:%.o=%.d)

LINKERFILE := $(LDSCRIPT)

FROCE_RELY := Makefile

.PHONY: FORCE _all all test clean distclean $(BUILD)/$(TARGET)

all: $(BUILD)/$(TARGET)

-include $(B_DEP)

$(BUILD)/%.o: %.s $(FROCE_RELY)   #汇编.s 和.c 文件会生成 .o 文件，并放入 $(BUILD) 路径下
	$(QUITE)$(ECHO) "  AS    $@"
	$(QUITE)$(MKDIR) $(dir $@)
	$(QUITE)$(CROSS_COMPILE)as -c $(A_FLAGS) $< -o$@

$(BUILD)/%.o: %.c $(FROCE_RELY)
	$(QUITE)$(ECHO) "  CC    $@"
	$(QUITE)$(MKDIR) $(dir $@)
	$(QUITE)$(CROSS_COMPILE)gcc -c $(C_FLAGS) $(B_DEF) $(B_INC) $< -o$@
    #elf为可调试文件，hex为intel格式文件，bin为二进制文件
$(BUILD)/$(TARGET): $(LINKERFILE) $(B_OBJ) $(FROCE_RELY)
	$(QUITE)$(MKDIR) $(BUILD)
	$(QUITE)$(ECHO) "  LD    $@.elf"
	$(QUITE)$(ECHO) "  OBJ   $@.hex"
	$(QUITE)$(ECHO) "  OBJ   $@.bin"
	$(QUITE)$(CROSS_COMPILE)gcc $(B_OBJ) $(L_FLAGS) -o$@.elf
	$(QUITE)$(CROSS_COMPILE)objcopy --gap-fill 0x00 -O ihex $@.elf $@.hex
	$(QUITE)$(CROSS_COMPILE)objcopy --gap-fill 0x00 -O binary -S $@.elf $@.bin
	$(QUITE)$(CROSS_COMPILE)size $@.elf
	$(QUITE)$(ECHO) "  BUILD FINISH"

clean:
	$(QUITE)rm -rf $(BUILD)
	$(QUITE)$(ECHO) "clean up"

distclean:
	$(QUITE)rm -rf $(BUILD_BASE)
	$(QUITE)$(ECHO) "distclean up"

test:
	$(QUITE)$(ECHO) $(BUILD) $(BUILD_BASE)
