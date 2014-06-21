// DGMVCCombine: stand-alone combiner for MVC streams.
// Copyright (C) 2014 Donald A. Graft, All Rights Reserved
// 
// This is free software. You may re-use it or derive new code from it
// on the condition that my original copyright notice is retained in your
// source code and the documentation of your program credits my original
// contribution.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 3000000

unsigned char base_buffer[BUF_SIZE];
unsigned char dependent_buffer[BUF_SIZE];
unsigned char *base_au, *dependent_au;
int base_au_size, dependent_au_size;
FILE *base_fp, *dependent_fp, *combined_fp;

int get_next_au(unsigned char *buf, FILE *fp)
{
	unsigned char *p;
	int hit_eof = 0;
	size_t read;

	read = fread(buf, 1, BUF_SIZE, fp);
	p = &buf[3];
	while (1)
	{
		p++;
		if (p[-4] == 0 && p[-3] == 0 && p[-2] == 1 && (p[-1] & 0x1f) == (buf == base_buffer ? 9 : 24))
			break;
	}
	if (buf == base_buffer)
		base_au = &p[-4];
	else
		dependent_au = &p[-4];
	while (1)
	{
		p++;
		if (p >= buf + read)
		{
			hit_eof = 1;
			break;
		}
		if (p[-4] == 0 && p[-3] == 0 && p[-2] == 1 && (p[-1] & 0x1f) == (buf == base_buffer ? 9 : 24))
			break;
	}
	if (buf == base_buffer)
	{
		base_au_size = (int) (p - base_au - (hit_eof ? 0 : 4));
	}
	else
	{
		dependent_au_size = (int) (p - dependent_au - (hit_eof ? 0 : 4));
	}
	_fseeki64(fp, -(buf + read - p + (hit_eof ? 0 : 4)), SEEK_CUR);
	return (hit_eof ? 1 : 0);
}

int main(int argc, char **argv)
{
	int count;
	int status;

	if (argc < 4)
	{
		printf("DGMVCCombine 1.0.2 by Donald A. Graft, Copyright (C) 2014, All Rights Reserved\n");
		printf("usage: dgmvccombine base dependent combined\n");
		return 1;
	}
	fopen_s(&base_fp, argv[1], "rb");
	if (base_fp == NULL)
	{
		printf("Cannot open base stream file\n");
		return 1;
	}
	fopen_s(&dependent_fp, argv[2], "rb");
	if (dependent_fp == NULL)
	{
		printf("Cannot open dependent stream file\n");
		fclose(base_fp);
		return 1;
	}
	fopen_s(&combined_fp, argv[3], "wb");
	if (dependent_fp == NULL)
	{
		printf("Cannot open combined stream file\n");
		fclose(base_fp);
		fclose(dependent_fp);
		return 1;
	}
	count = 0;
	// Extra leading 0 for the first 9 NALU.
	fputc(0, combined_fp);
	// Loop through the access units.
	while (1)
	{
		status = get_next_au(base_buffer, base_fp);
		status |= get_next_au(dependent_buffer, dependent_fp);
		count++;
		if (!(count % 50))
			printf("%d frames processed\r", count);
		fwrite(base_au, 1, base_au_size, combined_fp);
		fwrite(dependent_au, 1, dependent_au_size, combined_fp);
		if (status == 1)
			break;
	}
	fclose(base_fp);
	fclose(dependent_fp);
	fclose(combined_fp);
	return 0;
}