##################################################################################################
#							0.前期编译辅助参数支持												 #
##################################################################################################
SRC_PATH 		?= ./simplec
MAIN_DIR		?= main
SCHEAD_DIR 		?= module/schead
SERVICE_DIR		?= module/service
STRUCT_DIR		?= module/struct
TEST_DIR		?= test
TAR_PATH		?= ./Output
BUILD_DIR		?= obj

# 指定一些目录
DIR 	=	-I$(SRC_PATH)/$(SCHEAD_DIR)/include -I$(SRC_PATH)/$(SERVICE_DIR)/include \
			-I$(SRC_PATH)/$(STRUCT_DIR)/include

# 全局替换变量
CC		= gcc 
LIB 	= -lpthread -lm
CFLAGS 	= -g -Wall -O2 -std=gnu99

# 运行指令信息
define NOW_RUNO
$(notdir $(basename $(1))).o : $(1) | $$(TAR_PATH)
	$$(CC) $$(CFLAGS) $$(DIR) -c -o $$(TAR_PATH)/$$(BUILD_DIR)/$$@ $$<
endef

# 单元测试使用, 生成指定主函数的运行程序
RUN_TEST = $(CC) $(CFLAGS) $(DIR) --entry=$(basename $@) -nostartfiles -o \
	$(TAR_PATH)/$(TEST_DIR)/$@ $(foreach v, $^, $(TAR_PATH)/$(BUILD_DIR)/$(v))

# 产生具体的单元测试程序
define TEST_RUN
$(1) : $$(notdir $$(basename $(1))).o libschead.a $(2) | $$(TAR_PATH)
	$$(RUN_TEST) $(LIB)
endef
	
##################################################################################################
#							1.具体的产品生产													 #
##################################################################################################
.PHONY:all clean cleanall

all : main.out\
	$(foreach v, $(wildcard $(SRC_PATH)/$(TEST_DIR)/*.c), $(notdir $(basename $(v))).out)

# 主运行程序main
main.out:main.o simplec.o libschead.a libstruct.a test_sctimeutil.o
	$(CC) $(CFLAGS) $(DIR) -o $(TAR_PATH)/$@ $(foreach v, $^, $(TAR_PATH)/$(BUILD_DIR)/$(v)) $(LIB)

# !!!!! - 生成具体的单元测试程序 - 依赖个人维护 - !!!!!
$(eval $(call TEST_RUN, test_array.out, array.o))
$(eval $(call TEST_RUN, test_atom_rwlock.out))
$(eval $(call TEST_RUN, test_cjson.out, tstr.o))
$(eval $(call TEST_RUN, test_cjson_write.out, tstr.o))
$(eval $(call TEST_RUN, test_csv.out, tstr.o))
$(eval $(call TEST_RUN, test_json_read.out, tstr.o))
$(eval $(call TEST_RUN, test_log.out))
$(eval $(call TEST_RUN, test_scconf.out, tstr.o tree.o))
$(eval $(call TEST_RUN, test_scoroutine.out, scoroutine.o))
$(eval $(call TEST_RUN, test_scpthread.out, scpthread.o scalloc.o))
$(eval $(call TEST_RUN, test_sctimer.out, sctimer.o scalloc.o))
$(eval $(call TEST_RUN, test_sctimeutil.out))
$(eval $(call TEST_RUN, test_tstring.out, tstr.o))
$(eval $(call TEST_RUN, test_xlsmtojson.out, tstr.o))

##################################################################################################
#							2.先产生所需要的所有机器码文件										 #
##################################################################################################

# 循环产生 - 所有 - 链接文件 *.o
SRC_CS = $(wildcard\
	$(SRC_PATH)/$(MAIN_DIR)/*.c\
	$(SRC_PATH)/$(TEST_DIR)/*.c\
	$(SRC_PATH)/$(SCHEAD_DIR)/*.c\
	$(SRC_PATH)/$(SERVICE_DIR)/*.c\
	$(SRC_PATH)/$(STRUCT_DIR)/*.c\
)
$(foreach v, $(SRC_CS), $(eval $(call NOW_RUNO, $(v))))

# 生产 -相关- 静态库
libschead.a : $(foreach v, $(wildcard $(SRC_PATH)/$(SCHEAD_DIR)/*.c), $(notdir $(basename $(v))).o)
	ar cr $(TAR_PATH)/$(BUILD_DIR)/$@ $(foreach v, $^, $(TAR_PATH)/$(BUILD_DIR)/$(v))
libstruct.a : $(foreach v, $(wildcard $(SRC_PATH)/$(STRUCT_DIR)/*.c), $(notdir $(basename $(v))).o)
	ar cr $(TAR_PATH)/$(BUILD_DIR)/$@ $(foreach v, $^, $(TAR_PATH)/$(BUILD_DIR)/$(v))

##################################################################################################
#							3.程序的收尾工作,清除,目录构建										 #
##################################################################################################
$(TAR_PATH):
	-mkdir -p $@/$(BUILD_DIR)
	-mkdir -p $@/test/config
	-cp -r $(SRC_PATH)/test/config $@/test

# 清除操作
clean :
	-rm -rf $(TAR_PATH)/$(BUILD_DIR)/*

cleanall :
	-rm -rf $(TAR_PATH)
