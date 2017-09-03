#include <scsocket.h>

//
// 测试 clog.h 接口的使用
//
void test_clog(void) {
	// 测试时间类型
	struct timeval tv;
	struct timespec tc;

	// 开启 clog 日志库
	cl_start("test_clog.log");

	// 测试一下等待时间函数
	printf("sec = %"PRId64".\n", time(NULL));
	puts("等待几秒试试");
	usleep(1000000);
	printf("sec = %"PRId64".\n", time(NULL));

	timespec_get(&tc, TIME_UTC);
	printf("sec = %"PRId64", usec = %ld.\n", tc.tv_sec, tc.tv_nsec);

	gettimeofday(&tv, NULL);
	printf("sec = %ld, usec = %ld.\n", tv.tv_sec, tv.tv_usec);

	CL_ERROR("test_clog %d.", 1);
	CL_ERROR("test_clog %d.", 2);
	CL_DEBUG("test_clog %d.", 3);
	CL_INFOS("test_clog %d.", 4);
}