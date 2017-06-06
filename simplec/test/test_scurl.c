#include <scurl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//
// 测试 url 协议的解析
//
void test_scurl(void) {
	int len, nlen;
	char * nurl;
	const char * url;

	url = "http://www.baidu.com/s?_\\ie=utf-8&f=8&tn=baidu&wd=临时邮箱";
	len = strlen(url);
	nurl = url_encode(url, len, &nlen);
	printf(" url[ len = %d]: %s\n",  len,  url);
	printf("nurl[nlen = %d]: %s\n", nlen, nurl);
	url_decode(nurl, nlen);
	printf("nurl[nlen = %d]: %s\n", nlen, nurl);
	free(nurl);
}