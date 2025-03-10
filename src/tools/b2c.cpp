// gcc -O -Wa b2c.cpp -o b2c.exe    (mingw32/gcc3.4.2)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned long u32;
typedef unsigned char u8;
int main(int argc, char** argv){
	if(argc < 3){
		fprintf(stderr, "usage: %s file name\n", *argv);
		return 1;
	}
	FILE* fp = fopen(argv[1], "rb");
	if(!fp){
		fprintf(stderr, "Can't open %s\n", argv[1]);
		return 1;
	}
	printf("const u32 %s[] = {\n", argv[2]);
	int ch, n = 0;
	u32 v = 0;
	while((ch = fgetc(fp)) != EOF){
		
		v |= ((u32)ch) << ((n & 3) * 8);
		if((n & 3) == 3){
			printf("0x%08x,", v);
			v = 0;
		}
		if(++n == 16){
			printf("\n");
			n = 0;
		}
	}
	if(n){
		if(n & 3) printf("0x%08x,", v);
		printf("\n");
	}
	printf("};\n");
	fclose(fp);
	return 0;
}
