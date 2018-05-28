#include <httputil.h>
#include <sciconv.h>

#define _STR_BAIDU	"www.baidu.com"

//
// 测试 http 请求, 测试
//
void test_httputil(void) {
	TSTR_CREATE(music);
	http_start();

	http_sget(_STR_BAIDU, music);
	printf("music = %p : %zd.\n", music->str, music->len);
	if (NULL == music->str) {
		EXIT("music->str gets is empty = %s.\n", _STR_BAIDU);
	}

	if (si_isutf8(music->str)) {
		printf("music %s is utf-8.\n", _STR_BAIDU);
		si_utf8togbks(music->str);
	}

	puts(music->str);

	TSTR_DELETE(music);
}

#undef _STR_ZYQ
#undef _STR_SZYQ