#
# 前期编译一些目录结构使用宏
#
SRC_PATH 	?= ./simplec

MAIN_DIR	?= main

CURL_DIR	?= extern/curl
MPACK_DIR	?= extern/mpack

STRUCT_DIR	?= module/struct
SCHEAD_DIR 	?= module/schead
SERVICE_DIR	?= module/service
POLL_DIR	?= module/poll

TEST_DIR	?= test

TAR_PATH	?= ./Output
OBJ_DIR		?= obj

OUT			?= exe

#
# DIRS		: 所有可变编译文件目录
# IINC		: -I 需要导入的include 目录
# SRCC		: 所有 .c 文件
#
DIRS	=	$(CURL_DIR) $(STRUCT_DIR) $(SCHEAD_DIR) $(SERVICE_DIR) $(POLL_DIR) $(MPACK_DIR)

IINC	=	$(foreach v, $(DIRS), -I$(SRC_PATH)/$(v)/include)
SRCC	=	$(wildcard $(foreach v, $(MAIN_DIR) $(DIRS) $(TEST_DIR), $(SRC_PATH)/$(v)/*.c))

OBJC	=	$(wildcard $(foreach v, $(DIRS), $(SRC_PATH)/$(v)/*.c))
OBJO	=	$(foreach v, $(OBJC), $(notdir $(basename $(v))).o)
OBJP	=	$(TAR_PATH)/$(OBJ_DIR)/

TESTC	=	$(wildcard $(SRC_PATH)/$(TEST_DIR)/*.c)
TESTE	=	$(foreach v, $(TESTC), $(notdir $(basename $(v))).$(OUT))

#
# 全局编译的设置
#
CC		= gcc
LIB 	= -lpthread -lm -lcurl
CFLAGS 	= -g -O2 -Wall -Wno-unused-result -std=gnu11

RHAD	= $(CC) $(CFLAGS) $(IINC)
RTAL	= $(foreach v, $^, $(OBJP)$(v)) $(LIB)
RUNO	= $(RHAD) -c -o $(OBJP)$@ $<
RUN		= $(RHAD) -o $(TAR_PATH)/$@ $(RTAL)

# 单元测试使用, 生成指定主函数的运行程序, 替换回main操作
RUNT	= $(RHAD) -o $(TAR_PATH)/$(TEST_DIR)/$@ $(RTAL)
COPT	= objcopy --redefine-sym $(basename $@)=main $(OBJP)$(basename $@).o

#
# 具体的产品生产								
#
.PHONY : all clean cleanall

all : main.$(OUT) $(TESTE)

#
# 主运行程序main
#
main.$(OUT) : main.o simplec.o libsimplec.a
	$(RUN)

#
# -> 单元测试程序生成
#
define CALL_TEST
$(1) : $$(notdir $$(basename $(1))).o libsimplec.a | $$(TAR_PATH)
	$$(COPT)
	$$(RUNT)
endef

$(foreach v, $(TESTE), $(eval $(call CALL_TEST, $(v))))

#
# 循环产生 -> 所有 - 链接文件 *.o
#
define CALL_RUNO
$(notdir $(basename $(1))).o : $(1) | $$(TAR_PATH)
	$$(RUNO)
endef

$(foreach v, $(SRCC), $(eval $(call CALL_RUNO, $(v))))

#
# 生成 libsimplec.a 静态库, 方便处理所有问题
#
libsimplec.a : $(OBJO)
	ar cr $(OBJP)$@ $(foreach v, $^, $(OBJP)$(v))

#
# 程序的收尾工作,清除,目录构建						
#
$(TAR_PATH):
	-mkdir -p $(OBJP)
	-mkdir -p $@/$(TEST_DIR)/$(TEST_DIR)/config
	-mkdir -p $@/$(TEST_DIR)/logs
	-cp -r $(SRC_PATH)/config $@
	-cp -r $(SRC_PATH)/config/* $@/$(TEST_DIR)/$(TEST_DIR)/config
	-cp -r $(SRC_PATH)/$(TEST_DIR)/config $@/$(TEST_DIR)/$(TEST_DIR)

# 清除操作
clean :
	-rm -rf $(TAR_PATH)/$(TEST_DIR)/$(TEST_DIR)/config
	-rm -rf $(TAR_PATH)/$(TEST_DIR)/logs
	-rm -rf $(TAR_PATH)/main.$(OUT)
	-rm -rf $(OBJP)*

cleanall :
	-rm -rf $(TAR_PATH)
	-rm -rf Debug logs Release
	-rm -rf $(SRC_PATH)/Debug $(SRC_PATH)/logs $(SRC_PATH)/Release
