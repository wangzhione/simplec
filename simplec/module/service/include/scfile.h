#ifndef _H_SIMPLEC_SCFILE
#define _H_SIMPLEC_SCFILE

#include <schead.h>

//
// file_mtime - 得到文件最后修改时间
// path     : 文件名称
// return   : -1 表示获取出错, 否则就是时间戳
//
extern time_t file_mtime(const char * path);

//
// file_set - 设置需要更新的文件内容
// path     : 文件路径
// func     : NULL 表示清除, 否则 func(arg, path -> FILE)
// arg      : 注入的额外参数
// return   : void
//
extern void file_set_(const char * path, void * arg, 
                      void (* func)(void * arg, FILE * cnf));
#define file_set(path, arg, func) \
        file_set_(path, arg, (void (*)(void *, FILE *))func)

//
// file_update - 更新注册配置解析事件
// return   : void
//
extern void file_update(void);

//
// file_delete - 清除已经注册的, 需要在 update 之前或者之后
// return   : void
//
void file_delete(void);

#endif//_H_SIMPLEC_SCFILE
