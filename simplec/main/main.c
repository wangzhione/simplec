#include <scconf.h>
#include <scsocket.h>

//
// simple c 开发框架的启动函数总入口
// 变量函数环境启动初始化, 改动要慎重. atexit 先注册, 后调用 等同于数组栈
//
int main(int argc, char * argv[]) {
    // 初始化socket 库操作, 否则 errno == 126
    socket_start();

    // 初始化随机序列
    sh_srand((int32_t)time(NULL));

    // 开启 _DEBUG模式下结束等待
    SH_PAUSE();

    // 简单创建 _STR_LOGDIR 日志目录
    sh_mkdir(cnf_get("LogDir"));

    // 目前系统是默认启动 clog 日志
    cl_start(cnf_get("SimplecLog"));

    // stderr 错误信息重定位, 跟随系统长生不死
    if (!freopen(cnf_get("SysErrLog"), "ab", stderr)) {
        CL_ERROR("freopen ab %s is error!", cnf_get("SysErrLog"));
        exit(EXIT_FAILURE);
    }

    /*
     * simple c -> 具体业务跑起来 , -> ︿(￣︶￣)︿
     */
    /* 8-> */ EXTERN_RUN(simplec_main); /* <-8 */

    return EXIT_SUCCESS;
}
