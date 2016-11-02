#include <schead.h>
#include <sccsv.h>
#include <sclog.h>
#include <tstr.h>

//从文件中读取 csv文件内容
char* _csv_get(FILE * txt, int * prl, int * pcl)
{
	int c, n;
	int cl = 0, rl = 0;
	TSTR_NEW(ts);

	while((c=fgetc(txt))!=EOF) {
		if('"' == c) {	//处理这里数据
			while((c=fgetc(txt))!=EOF) {
				if('"' == c) {
					if((n=fgetc(txt)) == EOF) { //判断下一个字符
						TSTR_DELETE(ts);
						SL_WARNING("The CSV file is invalid one!");
						return NULL;
					}
					if(n != '"'){ //有效字符再次压入栈
						ungetc(n, txt);
						break;
					}
				}

				// 添加得到的字符
				tstr_append(ts, c);
			}
			//继续判断,只有是c == '"' 才会下来,否则都是错的
			if('"' != c){
				TSTR_DELETE(ts);
				SL_WARNING("The CSV file is invalid two!");
				return NULL;
			}
		}
		else if(',' == c){
			tstr_append(ts, '\0');
			++cl;
		}
		else if('\r' == c)
			continue;
		else if('\n' == c){
			tstr_append(ts, '\0');
			++cl;
			++rl;
		}
		else { //其它所有情况只添加数据就可以了
			tstr_append(ts, c);
		}
	}
	
	if(cl % rl){ //检测 , 号是个数是否正常
		TSTR_DELETE(ts);
		SL_WARNING("now csv file is illegal! need check!");
		return NULL;
	}
	
	// 返回最终内容
	*prl = rl;
	*pcl = cl;
	return ts->str;
}

// 将 _csv_get 得到的数据重新构建返回, 执行这个函数认为语法检测都正确了
sccsv_t _csv_get_new(const char * cstr, int rl, int cl)
{
	int i = 0;
	sccsv_t csv = sm_malloc(sizeof(struct sccsv) + sizeof(char *) * cl);
	
	// 这里开始构建内容了
	csv->rlen = rl;
	csv->clen = cl / rl;
	do {
		csv->data[i] = cstr;
		while(*cstr++) //找到下一个位置处
			;
	} while(++i<cl);
	
	return csv;
}

/*
 * 从文件中构建csv对象, 最后需要调用 sccsv_die 释放
 * path		: csv文件内容
 *			: 返回构建好的 sccsv_t 对象
 */
sccsv_t 
sccsv_new(const char * path)
{
	FILE * txt;
	char * cstr;
	int rl, cl;
	
	DEBUG_CODE({
		if(!path || !*path){
			SL_WARNING("params is check !path || !*path .");
			return NULL;
		}
	});

	// 打开文件内容
	if((txt = fopen(path, "r")) == NULL){
		SL_WARNING("fopen %s r is error!", path);
		return NULL;
	}
	// 如果解析 csv 文件内容失败直接返回
	cstr = _csv_get(txt, &rl, &cl);
	fclose(txt);

	// 返回最终结果
	return cstr ? _csv_get_new(cstr, rl, cl) : NULL;
}

/*
 * 释放由sccsv_new构建的对象
 * csv		: sccsv_new 返回对象
 */
inline void 
sccsv_delete(sccsv_t csv) {
	sm_free(csv);
}

/*
 * 获取某个位置的对象内容
 * csv		: sccsv_t 对象, new返回的
 * ri		: 查找的行索引 [0, csv->rlen)
 * ci		: 查找的列索引 [0, csv->clen)
 *			: 返回这一项中内容,后面可以用 atoi, atof, str_dup 等处理了...
 */
inline const char*
sccsv_get(sccsv_t csv, int ri, int ci) {
	DEBUG_CODE({
		if(!csv || ri<0 || ri>=csv->rlen || ci<0 || ci >= csv->clen){
			SL_WARNING("params is csv:%p, ri:%d, ci:%d.", csv, ri, ci);
			return NULL;
		}
	});

	// 返回最终结果
	return csv->data[ri*csv->clen + ci];
}