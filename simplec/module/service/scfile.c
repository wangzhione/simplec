#include <scfile.h>
#include <tstr.h>

//
// file_mtime - 得到文件最后修改时间
// path     : 文件名称
// return   : -1 表示获取出错, 否则就是时间戳
//
inline time_t 
file_mtime(const char * path) {
#ifdef _MSC_VER
    WIN32_FILE_ATTRIBUTE_DATA wfad;
    if (!GetFileAttributesEx(path, GetFileExInfoStandard, &wfad))
        return -1;
    // 基于 winds x64 sizeof(long) = 4
    return *(time_t *)&wfad.ftLastWriteTime;
#endif

#ifdef __GNUC__
    struct stat ss;
    // 数据最后的修改时间
    return stat(path, &ss) ? -1 : ss.st_mtime;
#endif
}

struct files {
    char * path;            // 文件全路径
    void * arg;             // 额外的参数
    void (* func)(void * arg, FILE * cnf);
    
    unsigned hash;          // 文件路径 hash 值
    time_t mtime;           // 文件结点最后修改时间点

    struct files * next;    // 文件下一个结点
};

static struct files * _fu;

// 找到特定结点, 并返回 hash 值
static struct files * _files_get(const char * p, unsigned * hh) {
    struct files * fu = _fu;
    unsigned h = *hh = tstr_hash(p);
    
    while(fu) {
        if (fu->hash == h && strcmp(fu->path, p) == 0)
            break;
        fu = fu->next;
    }

    return fu;
}

// 构造特定结点
static void _files_add(const char * p, unsigned h, 
                       void * arg, void (* func)(void *, FILE *)) {
    struct files * fu;
    time_t mtime = file_mtime(p);
    if (mtime == -1)
        RETURN(NIL, "file_mtime path error = %s.", p);

    // 开始构建数据
    fu = malloc(sizeof(struct files));
    if (NULL == fu) 
        RETURN(NIL, "malloc struct files is error!");
    fu->path = tstr_dup(p);
    fu->hash = h;
    fu->func = func;
    fu->arg = arg;
    fu->mtime = -1;

    fu->next = _fu;
    _fu = fu;
}

//
// file_set - 设置需要更新的文件内容
// path     : 文件路径
// func     : 注册执行的行为 func(arg, path -> cnf)
// arg      : 注入的额外参数
// return   : void
//
void 
file_set_(const char * path, 
    void (* func)(void * arg, FILE * cnf), void * arg) {
    unsigned hash;
    assert(path || func);
    struct files * fu = _files_get(path, &hash);
    // 添加操作 or 修改操作
    if (!fu)
        _files_add(path, hash, arg, func);
    else {
        fu->func = func;
        fu->arg = arg;
        fu->mtime = -1;   
    }
}

//
// file_update - 更新注册配置解析事件
// return   : void
//
void 
file_update(void) {
    for (struct files * fu = _fu; fu ; fu = fu->next) {
        time_t mtime = file_mtime(fu->path);
        if (mtime != fu->mtime && mtime != -1) {
            // 单独线程中跑, 阻塞操作, 业务上支持读写
            FILE * cnf = fopen(fu->path, "rb+");
            if (NULL == cnf) {
                CERR("fopen path wb error = %s.", fu->path);
                continue;
            }
            fu->mtime = mtime;
            fu->func(cnf, fu->arg);
            fclose(cnf);
        }
    }
}

//
// file_delete - 清除已经注册的, 需要在 update 之前或者之后
// return   : void
//
void 
file_delete(void) {
    struct files * fu = _fu;
    _fu = NULL;
    while (fu) {
        struct files * next = fu->next;
        free(fu->path);
        free(fu);
        fu = next;
    }
}