#ifndef _H_SIMPLEC_SCCSV
#define _H_SIMPLEC_SCCSV

//
// 这里是一个解析 csv 文件的 简单解析器.
// 它能够帮助我们切分文件内容, 保存在字符串数组中.
//
typedef struct sccsv {      //内存只能在堆上
    int rlen;               //数据行数,索引[0, rlen)
    int clen;               //数据列数,索引[0, clen)
    const char * data[];    //保存数据一维数组,希望他是二维的 rlen*clen
} * sccsv_t;

//
// 从文件中构建csv对象, 最后需要调用 sccsv_delete 释放
// path		: csv文件内容
// return	: 返回构建好的 sccsv_t 对象
//
extern sccsv_t sccsv_create(const char * path);

//
// 释放由sccsv_create构建的对象
// csv		: sccsv_create 返回对象
//
extern void sccsv_delete(sccsv_t csv);

//
// 获取某个位置的对象内容
// csv		: sccsv_t 对象, new返回的
// ri		: 查找的行索引 [0, csv->rlen)
// ci		: 查找的列索引 [0, csv->clen)
// return	: 返回这一项中内容,后面可以用 atoi, atof, tstr_dup 等处理了...
//
extern const char * sccsv_get(sccsv_t csv, int ri, int ci);

#endif // !_H_SIMPLEC_SCCSV