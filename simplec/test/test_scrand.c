#include <schead.h>

// 1亿的数据测试
#define _INT_TEST	(100000000)

static int _test_rand(int (* trand)(void)) {
	int rd = 0;
	for (int i = 0; i < _INT_TEST; ++i)
		rd = trand();
	return rd;
}

//
// test sh_rand 和 rand 速度比较
//
void test_scrand(void) {

	// stdlib.h rand test
	TIME_PRINT({
		_test_rand(rand);
	});

	// scrand.h sh_rand test
	TIME_PRINT({
		_test_rand(sh_rand);
	});
}

/*
 describe:
	1亿的数据量, 测试随机生成函数
	front system rand, back redis rand
 
 test code

 // 1亿的数据测试
 #define _INT_TEST	(100000000)
 
 static int _test_rand(int (* trand)(void)) {
 	 int rd = 0;
 	 for (int i = 0; i < _INT_TEST; ++i)
 	 	rd = trand();
 	 return rd;
 }

 winds test :
	cl version 14 Visual Studio 2015 旗舰版(Window 10 专业版)

	Debug
	The current code block running time:1.743000 seconds
	The current code block running time:4.408000 seconds

	Release
	The current code block running time:1.649000 seconds
	The current code block running time:0.753000 seconds

 linux test : 
	gcc version 6.3.0 20170406 (Ubuntu 6.3.0-12ubuntu2)
	-g -O2
	The current code block running time:0.775054 seconds
	The current code block running time:0.671887 seconds
 */