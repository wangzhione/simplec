#include <sciconv.h>
#include <stdio.h>
#include <stdlib.h>

#define _INT_STR	(35)

void test_sciconv(void) {
	
	char name[_INT_STR] = "王志 - simplec";
	printf("[%s] is utf8 = %d.\n", name, si_isutf8(name));
	si_gbktoutf8(name);
	printf("[%s] is utf8 = %d.\n", name, si_isutf8(name));
}