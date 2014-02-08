// merge.cpp : Defines the entry point for the console application.
//
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <tchar.h>
#include <cstring>

int find_delim(char* buffer, int size, bool flag_base) {
	char pattern_base[] = {0x00, 0x00, 0x00, 0x01, 0x09};
	char pattern_dept[] = {0x00, 0x00, 0x00, 0x01, 0x18};

	int pattern_size = 5;
	char *pattern = flag_base ? pattern_base : pattern_dept;
	for(int i=0; i<=size-pattern_size; i++) {
		if (memcmp(pattern, buffer + i, pattern_size) == 0) {
			return i;
		}
	}
	return -1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	char* def_argv[] = {
		NULL,
		"c:\\Users\\Vyacheslav\\Projects\\Utils\\test\\somic_demux_base.h264",
		"c:\\Users\\Vyacheslav\\Projects\\Utils\\test\\sonic_demux_dept.h264",
		"c:\\Users\\Vyacheslav\\Projects\\Utils\\test\\mux.result.h264"
	};
//	argv = def_argv;


	FILE* fbase = fopen(argv[1], "rb");
	FILE* fdept = fopen(argv[2], "rb");
	FILE *fout = fopen(argv[3], "wb");

	int size = 1024;
	char* buffer_base = new char[size];
	char* buffer_dept = new char[size];

	int base_size, dept_size;
	base_size = fread(buffer_base, 1, size, fbase);
	dept_size = fread(buffer_dept, 1, size, fdept);

	bool flag_base = true;

	while (base_size != 0 && dept_size != 0) {
		char *buffer = flag_base ? buffer_base : buffer_dept;
		int *buff_size = flag_base ? &base_size : &dept_size;

		int search = find_delim(buffer, *buff_size, flag_base);
		if (search > 0) {
			fwrite(buffer, 1, search, fout);
			memcpy(buffer, buffer + search, *buff_size - search);
			*buff_size = *buff_size - search;

			flag_base = !flag_base;
			buffer = flag_base ? buffer_base : buffer_dept;
			buff_size = flag_base ? &base_size : &dept_size;
		}
		else if (*buff_size > 8) {
			fwrite(buffer, 1, *buff_size - 8, fout);
			memcpy(buffer, buffer + *buff_size - 8, 8);
			*buff_size = 8;
		}
		else {
			fwrite(buffer, 1, *buff_size, fout);
			*buff_size = 0;

			flag_base = !flag_base;
			buffer = flag_base ? buffer_base : buffer_dept;
			buff_size = flag_base ? &base_size : &dept_size;
		}
		*buff_size += fread(buffer + *buff_size, 1, size - *buff_size, flag_base ? fbase : fdept);
	}


	return 0;
}

