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
int base_has_9, base_has_7, base_has_8, base_has_6, base_has_5, base_has_1;
int dependent_has_6, dependent_has_8, dependent_has_15, dependent_has_20;

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
			hit_eof = 1;
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

void write_all(unsigned char *buf, int type)
{
	unsigned char *p = buf + 3;
	unsigned char *start, *end;
	unsigned char c;

	while (1)
	{
		while (1)
		{
			p++;
			if (p[-4] == 0 && p[-3] == 0 && p[-2] == 1)
			{
				c = (p[-1] & 0x1f);
				if (buf == base_buffer)
				{
					if (c == 7)
						base_has_7 = 1;
					else if (c == 8)
						base_has_8 = 1;
					else if (c == 6)
						base_has_6 = 1;
					else if (c == 5)
						base_has_5 = 1;
					else if (c == 1)
						base_has_1 = 1;
				}
				else
				{
					if (c == 6)
						dependent_has_6 = 1;
					else if (c == 8)
						dependent_has_8 = 1;
					else if (c == 20)
						dependent_has_20 = 1;
				}
				if (c == type)
					break;
			}
			if (p >= buf + (buf == base_buffer ? base_au_size : dependent_au_size))
				return;
		}
		start = p - 4;
		while (1)
		{
			p++;
			if (p >= buf + (buf == base_buffer ? base_au_size : dependent_au_size))
			{
				end = p;
				break;
			}
			if (p[-3] == 0 && p[-2] == 0 && p[-1] == 1)
			{
				end = p - 3;
				break;
			}
		}
		fwrite(start, 1, end - start, combined_fp);
	}
}

int main(int argc, char **argv)
{
	int count;
	int status;

	if (argc < 4)
	{
		printf("DGMVCCombine 1.0.1 by Donald A. Graft, Copyright (C) 2014, All Rights Reserved\n");
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
		base_has_9 = base_has_7 = base_has_8 = base_has_6 = base_has_5 = base_has_1 = 0;
		dependent_has_6 = dependent_has_8 = dependent_has_15 = dependent_has_20 = 0;
		status = get_next_au(base_buffer, base_fp);
		status |= get_next_au(dependent_buffer, dependent_fp);
		count++;
//		if (!(count % 100)) printf("%d frames processed\r", count);
		printf("%d frames processed\n", count);
		write_all(base_buffer, 9);
		if (base_has_7)
			write_all(base_buffer, 7);
		write_all(dependent_buffer, 15);
		if (base_has_8)
			write_all(base_buffer, 8);
		if (dependent_has_8)
			write_all(dependent_buffer, 8);
		if (base_has_6)
			write_all(base_buffer, 6);
		if (base_has_5)
			write_all(base_buffer, 5);
		if (base_has_1)
			write_all(base_buffer, 1);
		if (dependent_has_6)
			write_all(dependent_buffer, 6);
		if (dependent_has_20)
			write_all(dependent_buffer, 20);
		if (status == 1)
			break;
	}

	fclose(base_fp);
	fclose(dependent_fp);
	fclose(combined_fp);
	return 0;
}