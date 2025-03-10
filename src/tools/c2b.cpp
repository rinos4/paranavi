// gcc -O -Wa c2b.cpp -o c2b.exe    (mingw32/gcc3.4.2)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char gBuf[1024];

int GetHex(const char* p){
	if(!isxdigit(p[0]) || !isxdigit(p[1]) || isxdigit(p[2])) return -1;
	return strtoul(p, 0, 16);
}
int main(int argc, char** argv){
	if(argc < 2){
		fprintf(stderr, "usage: %s file\n", *argv);
		return 1;
	}
	FILE* fp = fopen(argv[1], "wb");
	if(!fp){
		fprintf(stderr, "Can't open %s\n", argv[1]);
		return 1;
	}

	while(fgets(gBuf, sizeof(gBuf), stdin)){
		
		for(char* p = gBuf ; strlen(p) >= 4 ; ++p){
			if(p[0] != '0') continue;
			if(p[1] != 'x' && p[1] != 'X') continue;
			int n = GetHex(p + 2);
			if(n < 0) continue;

			fputc(n, fp);
		}
	}
	fclose(fp);
	return 0;
}
