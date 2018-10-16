#include "scconf.h"

// conf_set - 创建设置函数, kv 是 [ abc\012345 ]这样的结构
static void conf_set(dict_t conf, char * key) {
    char * val = key;
    while (*val++)
        ;
    dict_set(conf, key, strdup(val));
}

// READBR - 将这一行读取完毕
#define READBR(txt, c)                      \
while (c != EOF && '\n' != c)               \
    c = fgetc(txt)                          \

// READCH - 读取到固定字符
#define READCH(txt, c, ch)                  \
while (c != EOF && ch != c) {               \
    if (!isspace(c))                        \
       tstr_appendc(str, c);                \
    c = fgetc(txt);                         \
}                                           \
if (ch != c) /* 无效的解析直接结束 */ break

// 开始解析串
static void conf_parse(dict_t root, FILE * txt) {
    char c, n;
    TSTR_CREATE(str);

    //这里处理读取问题
    while ((c = fgetc(txt)) != EOF) {
        //1.0 先跳过空白字符
        while (c != EOF && isspace(c))
            c = fgetc(txt);

        //2.0 如果遇到第一个字符不是 '$'
        if (c != '$') { 
            READBR(txt, c);
            continue;
        }
        //2.1 第一个字符是 $ 合法字符, 开头不能是空格,否则也读取完毕
        if ((c = fgetc(txt)) != EOF && isspace(c)) {
            READBR(txt, c);
            continue;
        }

        //重新开始记录
        str->len = 0;

        //3.0 找到第一个等号
        READCH(txt, c, '=');

        c = '\0';
        //4.0 找到 第一个 "
        READCH(txt, c, '\"');

        //4.1 寻找第二个等号
        for (n = c; (c = fgetc(txt)) != EOF; n = c) {
            if (c == '\"' ) {
                if (n != '\\')
                    break;
                // 回退一个 '\\' 字符
                --str->len;
            }
            tstr_appendc(str, c);
        }
        if (c != '\"') //无效的解析直接结束
            break;

        // 这里就是合法字符了,开始检测 了, 
        conf_set(root, tstr_cstr(str));

        // 最后读取到行末尾
        READBR(txt, c);
        if (c != '\n')
            break;
    }

    TSTR_DELETE(str);
}

//
// conf_xxx 得到配置写对象, 失败都返回 NULL
// conf     : conf_create 创建的对象
// path     : 配置所在路径
// key      : 查找的key
// return   : 返回要得到的对象, 失败为 NULL
//
dict_t 
conf_create(const char * path) {
    dict_t conf;
    FILE * txt = fopen(path, "rb");
    if (NULL == txt) {
        RETNUL("fopen rb is error path = %s.", path);
    }

    // 创建具体配置字典对象
    conf = dict_create(free);
    if (conf) {
        // 解析添加具体内容
        conf_parse(conf, txt);
    }

    fclose(txt);
    return conf;
}

#define STR_CNF    "config/config.ini"

//
// cnf_instance 启动系统主配置, 得到配置中值
// key      : 待查询的 key
// return   : 返回要的对象, 创建的或查询的
//
inline dict_t
cnf_instance(void) {
    // 主配置单例对象
    static dict_t cnf;

    if (NULL == cnf) {
        cnf = conf_create(STR_CNF);
        if (NULL == cnf) {
            EXIT("conf_create error = "STR_CNF);
        }
        // 最终销毁主程序对象数据, 交给操作系统
    }

    return cnf;
}
