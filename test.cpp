

///// EXAMPLE PROGRAM //////

#include "FakeFibre/FakeFibre.h"

#include<stdio.h>
#include<unistd.h>

static ff_handle hmain = NULL;
static ff_handle h1 = NULL;
static ff_handle h2 = NULL;

void * ff1(void * data) {

	int i = 0;
	int n = 0;
	const char * s[] = {"A","B","C","\n"};
	for(n=0;n<2;n++) {
		for(i=0;i<4;i++)
			printf("%s", s[i]);

		// Switch to fibre 2
		ff_yield_to(h2);
	}

	h1 = NULL;

	return NULL;
}

void * ff2(void * data) {

	int i = 0;
	int n = 0;
	const char * s[] = {"1","2","3","\n"};
	for(n=0;n<2;n++) {
		for(i=0;i<4;i++)
			printf("%s", s[i]);

		// Switch to fibre 1
		ff_yield_to(h1);
	}

	h2 = NULL;

	return NULL;
}

int main() {

	ff_convert_this(&hmain);

	ff_create(&h1, &ff1, NULL, NULL);
	ff_create(&h2, &ff2, NULL, NULL);

	ff_yield_to(h1);

	ff_exit();

	return 0;
}
