#include <tstr.h>

#define _STR_TEMPLATE	"test/config/sys_strerror.c.template"
#define _STR_DATA		"test/config/winerror.h.data"
#define _STR_BAK		"test/config/sys_strerror.c"

#define _STR_MESSAGEID	"//\r\n// MessageId: "

const char * errstruct_create(const char * data);

//
// 模板文件 : sys_strerror.c.template
// 素材文件 : winerror.h.data
// 生成文件 : sys_strerror.c.bak
//
void test_sys_strerror(void) {
	char c;
	const char * si;
	tstr_t errf = tstr_freadend(_STR_DATA);
	tstr_t cstr = tstr_creates(
		"#if defined(__GNUC__)\r\n"
		"\r\n"
		"#include <string.h>\r\n"
		"\r\n"
		"inline const char * sys_strerror(int error) {\r\n"
		"	return strerror(error);\r\n"
		"}\r\n"
		"\r\n"
		"#endif\r\n"
		"\r\n"
		"#if defined(_MSC_VER)\r\n"
		"\r\n"
		"#include <stdio.h>\r\n"
		"#include <winerror.h>\r\n"
		"\r\n"
		"#define DWORD int\r\n"
		"\r\n"
		"const char * sys_strerror(int error) {\r\n"
		"	switch (error) {\r\n"
	);

	//
	// 越封装, 越复杂
	// 完全可以不用任何特性, 搞定上面需求. 但为了求出厂吧.
	//

	for (si = errf->str; (c = *si); ++si) {

		if (c == '/') {
			if (!strncmp(_STR_MESSAGEID, si, sizeof(_STR_MESSAGEID) - 1)) {
				// 找见了这段内存
				tstr_appends(cstr, errstruct_create(si));

				// 开始清除数据到最后了
				while (*si++ != '#')
					;
			}

			while (*si++ != '\r')
				;
		}

	}

	// 开始输出后半段
	tstr_appends(cstr, 
		"	}\r\n"
		"\r\n"
		"	fprintf(stderr, \"sys_strerror invaild error = %d.\\n\", error);\r\n"
		"	return \"The aliens are coming. Go tell your favorite people\";\r\n"
		"}\r\n"
		"\r\n"
		"#undef DWORD\r\n"
		"\r\n"
		"#endif"
	);

	// 写到文件里面
	tstr_fwrites(_STR_BAK, cstr->str);
	puts("socket_strerrno function success -> " _STR_BAK);

	tstr_delete(cstr);
	tstr_delete(errf);
}

#define UTC_E_ESCALATION_DIRECTORY_ALREADY_EXISTS _HRESULT_TYPEDEF_(0x87C5102FL)
const char *
errstruct_create(const char * data) {
	static tstr_t _msgs;

	char c, i, onec;
	const char * si, * ei;

	if(!_msgs)
		_msgs = tstr_creates("    case ");
	_msgs->len = sizeof("    case ") - 1;

/*
	//
	// MessageId: UTC_E_ESCALATION_DIRECTORY_ALREADY_EXISTS
	//
	// MessageText:
	//
	// The escalation working directory for the requested escalation could not be created because it already exists
	//
	#define UTC_E_ESCALATION_DIRECTORY_ALREADY_EXISTS _HRESULT_TYPEDEF_(0x87C5102FL)

	or

	//
	// MessageId: SEC_E_DECRYPT_FAILURE
	//
	// MessageText:
	//
	// The specified data could not be decrypted.
	//
	//
	#define SEC_E_DECRYPT_FAILURE            _HRESULT_TYPEDEF_(0x80090330L)

	or

	// SymbolicName=XACT_E_CONNECTION_REQUEST_DENIED
	//
	// MessageId: 0x8004D100L (No symbolic name defined)
	//
	// MessageText:
	//
	// The request to connect to the specified transaction coordinator was denied.
	//

	... ...

	to

	case {errno}: return "{str}";

	case UTC_E_ESCALATION_DIRECTORY_ALREADY_EXISTS: return "The escalation working directory for the requested escalation could not be created because it already exists";
*/
	// step 1 : add {errno}
	i = 0;
	si = data + sizeof(_STR_MESSAGEID) - 1;

	onec = *si == '0' ? 'L' : '\r';
	while ((c = *si++) != onec) {
		tstr_appendc(_msgs, c);
		++i;
	}

	while (i++ < 71)
		tstr_appendc(_msgs, ' ');

	// step 2 : add {: return "}
	tstr_appends(_msgs, ": return \"");

	// step 3 : add {str}

	// 读取8个 '/', 找到开始 si, 找到结束的 ei
	onec = 8;
	i = 0;
	while (i < onec) {
		while (*++si != '/')
			;
		if(si[-1] == '/' || si[1] == '/')
			++i;
	}
	
	ei = si += 2;
	if (*si == '\r')
		ei = si += 5;
	
	// 处理结尾ei
	onec = 1;
	i = 0;
	while (i < onec) {
		while (*++ei != '/')
			;
		if (ei[-1] == '/' || ei[1] == '/')
			++i;
	}

	ei -= 2;
	while (si < ei) {
		c = *si++;
		if (c == '"' || c == '\\')
			tstr_appendc(_msgs, '\\');
		tstr_appendc(_msgs, c);
	}

	// step 4 : add {";}
	tstr_appends(_msgs, "\";\r\n");

	return _msgs->str;
}