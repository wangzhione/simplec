#
# 前期编译一些目录结构使用宏
#
SRC_PATH 		?= ./simplec

MAIN_DIR		?= main
BODY_DIR		?= main/body

CURL_DIR		?= extern/curl
ICONV_DIR		?= extern/iconv
OPENSSL_DIR		?= extern/openssl
PTHREAD_DIR		?= extern/pthread

IOP_DIR			?= module/iop
COLOG_DIR		?= module/colog
SCHEAD_DIR 		?= module/schead
STRUCT_DIR		?= module/struct
SERVICE_DIR		?= module/service

TEST_DIR		?= test

TAR_PATH		?= ./Output
OBJ_DIR			?= obj

#
# DIRS		: 所有可变编译文件目录
# IINC		: -I 需要导入的include 目录
# SRCC		: 所有 .c 文件
#
DIRS	=	$(CURL_DIR) $(ICONV_DIR) $(OPENSSL_DIR) \
			$(IOP_DIR) $(COLOG_DIR) $(SCHEAD_DIR) \
			$(STRUCT_DIR) $(SERVICE_DIR)

IINC	=	$(foreach v, $(DIRS), -I$(SRC_PATH)/$(v)/include)
SRCC	=	$(wildcard $(foreach v, $(MAIN_DIR) $(DIRS) $(TEST_DIR), $(SRC_PATH)/$(v)/*.c))

OBJC	=	$(wildcard $(foreach v, $(DIRS), $(SRC_PATH)/$(v)/*.c))
OBJO	=	$(foreach v, $(OBJC), $(notdir $(basename $(v))).o)
OBJP	=	$(TAR_PATH)/$(OBJ_DIR)/

TESTC	=	$(wildcard $(SRC_PATH)/$(TEST_DIR)/*.c)
TESTE	=	$(foreach v, $(TESTC), $(notdir $(basename $(v))).exe)

#
# 全局编译的设置
#
CC		= gcc 
LIB 	= -lpthread -lm -liconv -lssl -lcurl
CFLAGS 	= -g -Wall -Wno-unused-result -O2 -std=gnu11

DEF		= -D_HAVE_EPOLL

RHAD	= $(CC) $(CFLAGS) $(IINC) $(DEF)
RUNO	= $(RHAD) -c -o $(OBJP)$@ $<
RUN		= $(RHAD) -o $(TAR_PATH)/$@ $(foreach v, $^, $(OBJP)$(v)) $(LIB)

# 单元测试使用, 生成指定主函数的运行程序
RUNT	= $(CC) $(CFLAGS) $(DIR) --entry=$(basename $@) -nostartfiles \
		  -o $(TAR_PATH)/$(TEST_DIR)/$@ $(foreach v, $^, $(OBJP)$(v))

#
# 具体的产品生产								
#
.PHONY:all clean cleanall

all : main.exe $(TESTE)

#
# 主运行程序main
#
main.exe:main.o simplec.o libsimplec.a
	$(RUN)

#
# -> 单元测试程序生成
#
define CALL_TEST
$(1) : $$(notdir $$(basename $(1))).o libsimplec.a | $$(TAR_PATH)
	$$(RUNT) $(LIB)
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
	-mkdir -p $@/$(TEST_DIR)/config
	-cp -r $(SRC_PATH)/$(TEST_DIR)/config $@/$(TEST_DIR)
	-cp $(SRC_PATH)/config/* $@/$(TEST_DIR)/config

# 清除操作
clean :
	-rm -rf $(OBJP)*

cleanall :
	-rm -rf $(TAR_PATH)
