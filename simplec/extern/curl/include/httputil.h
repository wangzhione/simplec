#ifndef _H_SIMPLEC_HTTPUTIL
#define _H_SIMPLEC_HTTPUTIL

#include <tstr.h>
#include <scmd5.h>
#include <curl/curl.h>

//
// http_start - http 库启动
// return	: void
//
extern void http_start(void);

//
// http_get - http get 请求 or https get请求 
// url		: 请求的url
// str		: 返回最终申请的结果, NULL表示只请求不接收数据
// return	: true表示请求成功
//
extern bool http_get(const char * url, tstr_t str);
extern bool http_sget(const char * url, tstr_t str);

//
// http_post - http post 请求 or https post 请求
// url		: 请求的路径
// params	: 参数集 key=value&.... 串
// str		: 返回结果, 需要自己释放, NULL表示只请求不接受数据返回
// return	: false表示请求失败, 会打印错误信息
//
extern bool http_post(const char * url, const char * params, tstr_t str);
extern bool http_spost(const char * url, const char * params, tstr_t str);

#endif // !_H_SIMPLEC_HTTPUTIL