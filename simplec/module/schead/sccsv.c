#include <sccsv.h>
#include <tstr.h>

//从文件中读取 csv文件内容, 构建一个合法串
static bool _csv_parse(tstr_t tstr, int * prl, int * pcl) {
	int c = -1, n = -1;
	int cl = 0, rl = 0;
	char * sur = tstr->str, * tar = tstr->str;

	while (!!(c = *tar++)) {
		// 小型状态机切换, 相对于csv文件内容解析
		switch (c) {
		case '"': // 双引号包裹的特殊字符处理
			while (!!(c = *tar++)) {
				if ('"' == c) {
					if ((n = *tar) == '\0') // 判断下一个字符
						goto _faild;
					if (n != '"') // 有效字符再次压入栈, 顺带去掉多余 " 字符
						break;
					++tar;
				}

				// 添加得到的字符
				*sur++ = c;
			}
			// 继续判断,只有是c == '"' 才会下来,否则都是错的
			if ('"' != c)
				goto _faild;
			break;
		case ',':
			*sur++ = '\0';
			++cl;
			break;
		case '\r':
			break;
		case '\n':
			*sur++ = '\0';
			++cl;
			++rl;
			break;
		default: // 其它所有情况只添加数据就可以了
			*sur++ = c;
		}
	}
	
	if (cl % rl) { // 检测 , 号是个数是否正常
	_faild:
		RETURN(false, "now csv error c = %d, n = %d, cl = %d, rl = %d.", c, n, cl, rl);
	}
	
	// 返回最终内容
	*prl = rl;
	*pcl = cl;
	// 构建最终处理的串内容
	tstr->len = sur - tstr->str + 1;
	tstr->str[tstr->len - 1] = '\0';
	return true;
}

// 将 _csv_get 得到的数据重新构建返回, 执行这个函数认为语法检测都正确了
static sccsv_t _csv_create(tstr_t tstr) {
	sccsv_t csv;
	size_t pdff;
	char * cstr;
	int rl, cl, i;
	if (!_csv_parse(tstr, &rl, &cl))
		return NULL;

	// 分配最终内存
	pdff = sizeof(struct sccsv) + sizeof(char *) * cl;
	csv = malloc(pdff + tstr->len);
	if (NULL == csv) {
		RETURN(NULL, "malloc error cstr->len = %zu, rl = %d, cl = %d.", tstr->len, rl, cl);
	}

	// 这里开始拷贝内存, 构建内容了
	cstr = (char *)csv + pdff;
	memcpy(cstr, tstr->str, tstr->len);
	csv->rlen = rl;
	csv->clen = cl / rl;
	i = 0;
	do {
		csv->data[i] = cstr;
		while(*cstr++) // 找到下一个位置处
			;
	} while(++i < cl);
	
	return csv;
}

//
// 从文件中构建csv对象, 最后需要调用 sccsv_delete 释放
// path		: csv文件内容
// return	: 返回构建好的 sccsv_t 对象
//
sccsv_t
sccsv_create(const char * path) {
	sccsv_t csv;
	tstr_t tstr = tstr_freadend(path);
	if (NULL == tstr) {
		RETURN(NULL, "tstr_freadend path = %s is error!", path);
	}

	// 如果解析 csv 文件内容失败直接返回
	csv = _csv_create(tstr);

	tstr_delete(tstr);
	// 返回最终结果
	return csv;
}

//
// 释放由sccsv_create构建的对象
// csv		: sccsv_new 返回对象
//
inline void 
sccsv_delete(sccsv_t csv) {
	free(csv);
}

//
// 获取某个位置的对象内容
// csv		: sccsv_t 对象, new返回的
// ri		: 查找的行索引 [0, csv->rlen)
// ci		: 查找的列索引 [0, csv->clen)
// return	: 返回这一项中内容,后面可以用 atoi, atof, tstr_dup 等处理了...
//
inline const char * 
sccsv_get(sccsv_t csv, int ri, int ci) {
	DEBUG_CODE({
		if (!csv || ri < 0 || ri >= csv->rlen || ci < 0 || ci >= csv->clen) {
			RETURN(NULL, "params is csv:%p, ri:%d, ci:%d.", csv, ri, ci);
		}
	});

	// 返回最终结果
	return csv->data[ri * csv->clen + ci];
}