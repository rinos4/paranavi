// gcc -O -Wa add8kb.cpp -o add8kb.exe

#include <stdio.h>
#include <stdlib.h>

const size_t ADD_SIZE = 8 * 1024;

int main(int argc, char** argv){
	if(argc != 2){
		fprintf(stderr, "usage: %s file\n", *argv);
		return 1;
	}

	// ÉçÉOì«Ç›çûÇ›
	FILE* fp = fopen(argv[1], "ab");
	if(!fp){
		fprintf(stderr, "Can' open %s\n", argv[1]);
		return 2;
	}
	fseek(fp, 0, SEEK_END);
	for(size_t i = ftell(fp) ; i < ADD_SIZE ; ++i) fputc(0, fp);
	
//	printf("%d", ftell(fp));
	fclose(fp);

	return 0;
}
