#include <httputil.h>

// libcurl 库退出操作
static inline void _http_end(void) {
	curl_global_cleanup();
}

//
// http_start - http 库启动
// return	: void
//
inline void 
http_start(void) {
	//
	// CURLcode curl_global_init(long flags);
	// @ 初始化libcurl, 全局只需调一次
	// @ flags : CURL_GLOBAL_DEFAULT    // 等同于 CURL_GLOBAL_ALL
	//           CURL_GLOBAL_ALL        // 初始化所有的可能的调用
	//           CURL_GLOBAL_SSL        // 初始化支持安全套接字层
	//           CURL_GLOBAL_WIN32      // 初始化WIN32套接字库
	//           CURL_GLOBAL_NOTHING    // 没有额外的初始化
	//
	CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (code != CURLE_OK) {
		RETURN(NIL, "curl_global_init error = %s.", curl_easy_strerror(code));
	}

	atexit(_http_end);
}

// 具体看 curl_write_callback , 这里使用libcurl 内部强转
static inline size_t _http_write(char * buf, size_t s, size_t n, tstr_t str) {
	size_t sn = s * n;
	tstr_appendn(str, buf, sn);
	// 返回sn必须这么写
	return sn;
}

//
// 请求共有头部, 请求超时时间
//
#define _INT_TOUT		(4)
static CURL * _http_head(const char * url, tstr_t str) {
	CURL * crl = curl_easy_init();
	if (NULL == crl) {
		RETURN(NULL, "curl_easy_init error !!!");
	}

	// 设置下载属性和常用参数
	curl_easy_setopt(crl, CURLOPT_URL, url);            // 访问的URL
	curl_easy_setopt(crl, CURLOPT_TIMEOUT, _INT_TOUT);  // 设置超时时间 单位s
	curl_easy_setopt(crl, CURLOPT_HEADER, true);        // 下载数据包括HTTP头部
	curl_easy_setopt(crl, CURLOPT_NOSIGNAL, true);      // 屏蔽其它信号
                                                        // 输入函数类型
	curl_easy_setopt(crl, CURLOPT_WRITEFUNCTION, (curl_write_callback)_http_write);	
	curl_easy_setopt(crl, CURLOPT_WRITEDATA, str);      // 输入参数

	return crl;
}

// 请求共有尾部
static inline bool _http_tail(CURL * crl, tstr_t str) {
	CURLcode res = curl_easy_perform(crl);
	tstr_cstr(str);
	curl_easy_cleanup(crl);
	if (res != CURLE_OK) {
		RETURN(false, "curl_easy_perform error = %s!", curl_easy_strerror(res));
	}

	return true;
}

//
// http_get - http get 请求 or https get请求 
// url		: 请求的url
// str		: 返回最终申请的结果, NULL表示只请求不接收数据
// return	: true表示请求成功
//
inline bool 
http_get(const char * url, tstr_t str) {
	CURL * crl = _http_head(url, str);
	return crl ? _http_tail(crl, str) : false;
}

// 添加 https 请求设置
static inline void _http_sethttps(CURL * crl) {
	curl_easy_setopt(crl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(crl, CURLOPT_SSL_VERIFYHOST, false);
}

inline bool 
http_sget(const char * url, tstr_t str) {
	CURL * crl = _http_head(url, str);
	if (crl) {
		_http_sethttps(crl);
		return _http_tail(crl, str);
	}
	return false;
}

// 添加 post 请求设置
static inline void _http_setpost(CURL * crl, const char * params) {
	curl_easy_setopt(crl, CURLOPT_POST, true); 
	curl_easy_setopt(crl, CURLOPT_POSTFIELDS, params);
}

//
// http_post - http post 请求 or https post 请求
// url		: 请求的路径
// params	: 参数集 key=value&.... 串
// str		: 返回结果, 需要自己释放, NULL表示只请求不接受数据返回
// return	: false表示请求失败, 会打印错误信息
//
inline bool 
http_post(const char * url, const char * params, tstr_t str) {
	CURL * crl = _http_head(url, str);
	if (crl) {
		_http_setpost(crl, params);
		return _http_tail(crl, str);
	}
	return false;
}

bool 
http_spost(const char * url, const char * params, tstr_t str) {
	CURL * crl = _http_head(url, str);
	if (crl) {
		_http_sethttps(crl);
		_http_setpost(crl, params);
		return _http_tail(crl, str);
	}
	return false;
}