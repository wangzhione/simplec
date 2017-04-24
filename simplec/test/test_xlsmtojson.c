#include <schead.h>
#include <sclog.h>
#include <sccsv.h>
#include <tstr.h>

#define _STR_CSVPATH	"test/config/destiny.csv"

// 将csv转换成json文件输出, 成功返回0, 错误见状态码<0
int csvtojson(const char* path);

void test_xlsmtojson(void) {
	int rt;
	
	// sccsv 使用了 sclog 需要启动 sclog
	sl_start();

	rt = csvtojson(_STR_CSVPATH);
	if(0 == rt) 
		printf("%s 转换成功\n", _STR_CSVPATH);
	else
		printf("%s 转换失败, rt = %d\n", _STR_CSVPATH, rt);
	
#ifdef __GNUC__
	exit(EXIT_SUCCESS);
#endif
}

// 得到生成json文件的名称, 需要自己free
static char* _csvtojsonpath(const char * path) {
	char *tarp;
	size_t len = strlen(path);
	// 判断后缀名
	if(tstr_icmp(path + len - 4, ".csv")) {
		CERR("path is %s need *.csv", path);
		return NULL;
	}
	// 这里申请内存进行处理
	if((tarp = malloc(len+2))==NULL) {
		CERR("malloc is error!");
		return NULL;
	}
	
	// 返回最终结果
	memcpy(tarp, path, len - 3);
	// 添加json后缀名
	tarp[len-3] = 'j';
	tarp[len-2] = 's';
	tarp[len-1] = 'o';
	tarp[len] = 'n';
	tarp[len + 1] = '\0';
	return tarp;
}

// csv read -> json write
static void _csvtojson(sccsv_t csv, FILE* json) {
	// 第一行, 第一列都是不处理的
	int c, r;
	// 第二行 内容是对象中主键内容
	int clen = csv->clen - 1, rlen = csv->rlen - 1;
	// 先确定最优行和列
	while (rlen > 2) {
		if (*sccsv_get(csv, rlen, 1))
			break;
		--rlen;
	}
	while (clen > 1) {
		if (*sccsv_get(csv, 0, clen))
			break;
		--clen;
	}

	// 最外层是个数组
	fputs("[\n", json);
	for (r = 2; r <= rlen; ++r) {
		// 当对象处理
		fputs("\t{\n", json);

		// 输出当前对象中内容
		for (c = 1; c <= clen; ++c) {
			fprintf(json, "\t\t\"%s\":%s", sccsv_get(csv, 1, c),sccsv_get(csv, r, c));
			fputs(c == clen ? "\n" : ",\n", json);
		}

		// 最新的json语法支持多个 ','
		fputs(r == rlen ? "\t}\n" : "\t},\n", json);
	}

	fputs("]", json);
}

// 将csv转换成json文件输出, 成功返回0, 错误见状态码<0
int 
csvtojson(const char* path) {
	char* tarp;
	FILE* json;
	sccsv_t csv;
	
	if(!path || !*path) {
		CERR("path is null!");
		return RT_Error_Param;
	}
	
	// 继续判断后缀名
	if((tarp = _csvtojsonpath(path)) == NULL ) {
		CERR("path = %s is error!", path);
		return RT_Error_Param;
	}
	// 这里开始打开文件, 并判断
	if((csv = sccsv_create(path)) == NULL) {
		free(tarp);
		CERR("sccsv_new %s is error!", path);
		return RT_Error_Fopen;
	}
	if((json = fopen(tarp, "wb")) == NULL ) {
		sccsv_delete(csv);
		free(tarp);
		CERR("fopen %s wb is error!", tarp);
		return RT_Error_Fopen;
	}
	
	// 到这里一切前戏都好了开始转换了
	_csvtojson(csv, json);
	
	fclose(json);
	sccsv_delete(csv);
	free(tarp);
	return RT_Success_Base;
}