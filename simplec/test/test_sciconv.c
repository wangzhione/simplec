#include <sciconv.h>
#include <stdio.h>
#include <stdlib.h>

#define _INT_STR	(35)

void test_sciconv(void) {
	// name 地址足够大
	char name[_INT_STR] = "王志 - simplec";

	printf("[%s] is utf8 = %d.\n", name, si_isutf8(name));
	si_gbktoutf8s(name, sizeof name);
	printf("[%s] is utf8 = %d.\n", name, si_isutf8(name));
	si_utf8togbks(name);
	printf("[%s] is utf8 = %d.\n", name, si_isutf8(name));
}