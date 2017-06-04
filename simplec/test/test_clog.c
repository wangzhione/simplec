#include <clog.h>

//
// 测试 clog.h 接口的使用
//
void test_clog(void) {
	
	// 开启 clog 日志库
	cl_start();

	// 测试时间类型
	struct timeval tv;

	gettimeofday(&tv, NULL);
	printf("sec = %ld, usec = %ld.\n", tv.tv_sec, tv.tv_usec);

	time_t t = time(NULL);
	printf("sec = %"PRId64".\n", t);

	CL_ERROR("test_clog %d.", 1);
	CL_ERROR("test_clog %d.", 2);
	CL_DEBUG("test_clog %d.", 3);
	CL_INFO("test_clog %d.", 4);
}