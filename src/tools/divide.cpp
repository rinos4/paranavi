#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
	int size;
	if(argc != 3 || (size = atoi(argv[1])) == 0) {
		fprintf(stderr, "usage: %s size file\n", *argv);
		return 1;
	}
	char* fname = argv[2];
	FILE* in = fopen(fname, "rb");
	if(!in){
		fprintf(stderr, "Can' open %s\n", fname);
		return 2;
	}
	int index = 0, count = 0;
	FILE* out = 0;
	int ch;
	while((ch = fgetc(in)) != EOF){
		if(!out){
			char buf[256];
			sprintf(buf, "%s%d.bin", fname, index++);
			out = fopen(buf, "wb");
			if(!out){
				fprintf(stderr, "Can't create %s\n", buf);
				return 2;
			}
		}
		fputc(ch, out);
		if(++count == size){
			count = 0;
			fclose(out);
			out = 0;
		}
	}
	if(out) fclose(out);
	fclose(in);
	return 0;
}
