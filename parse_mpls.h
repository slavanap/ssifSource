/* 
 * File:   parse_mpls.h
 * Author: tkmax82
 *
 * Created on January 22, 2013, 10:48 AM
 */

#ifndef PARSE_MPLS_H
#define	PARSE_MPLS_H

#include <libgen.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _SYSLIMITS_H_
#include <syslimits.h>
#endif
#ifdef _SYS_SYSLIMITS_H_
#include <sys/syslimits.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif


/*
 * Constants
 */


#define TIME_IN_POS  82
#define TIME_OUT_POS 86

#define CHAPTER_TYPE_ENTRY_MARK 1 /* standard chapter */
#define CHAPTER_TYPE_LINK_POINT 2 /* unsupported ??? */

#define CHAPTER_SIZE 14 /* number of bytes per chapter entry */

#define TIMECODE_DIV 45000.00 /* divide timecodes (int32) by this value to get
                                 the number of seconds (double) */


/*
 * Macros
 */


#define DIE(...) die(__FILE__, __LINE__, __VA_ARGS__);
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))


/*
 * Structs - BD-ROM
 */

typedef struct {
    char* path;
    char* name;
    FILE* file;
    long size;
    char* data;
    char header[9];              /* "MPLS0100" or "MPLS0200" */
    int32_t pos;                 /* cursor containing the current byte offset in the file during parsing */
    int32_t playlist_pos;        /* byte offset of the playlist and stream clip information */
    int32_t chapter_pos;         /* byte offset of the chapter list */
    int16_t total_chapter_count; /* total number of chapters (including both supported (0x1) and unsupported (0x2) chapter types) */
    int32_t time_in;
    int32_t time_out;
} mpls_file_t; /* raw data extracted from .mpls file */

typedef struct stream_clip_s {
    char filename[11]; /* uppercase - e.g., "12345.M2TS" */
    double time_in_sec;
    double time_out_sec;
    double duration_sec;
    double relative_time_in_sec;
    double relative_time_out_sec;
    int track_count;
    int video_count;
    int audio_count;
    int subtitle_count; /* Presentation Graphics Streams (subtitles) */
    int interactive_menu_count; /* Interactive Graphics Streams (in-movie popup menus) */
    int secondary_video_count;
    int secondary_audio_count;
    int pip_count; /* Picture-in-Picture (PiP) */
    int index;
    struct stream_clip_s* next;
} stream_clip_t; /* parsed data from .m2ts + .cpli files */

typedef struct {
    stream_clip_t* first;
    stream_clip_t* last;
    int count;
} stream_clip_list_t;

typedef struct {
    char filename[11]; /* uppercase - e.g., "00801.MPLS" */
    double time_in_sec;
    double time_out_sec;
    double duration_sec;
    char duration_formatted[15]; /* HH:MM:SS.mmm */
    stream_clip_list_t stream_clip_list;
    stream_clip_list_t chapter_stream_clip_list;
    double* chapters;
    size_t chapter_count;
} playlist_t;


/*
 * Utility functions
 */


/**
 * From http://www.lemoda.net/c/die/index.html
 * @param filename name of the .c file that called die()
 * @param line_number
 * @param format format passed to printf()
 * @param ... arguments passed to printf()
 */
void
die (const char* filename, int line_number, const char * format, ...);

/**
 * Converts the specified timecode to seconds.
 * @param timecode
 * @return Number of seconds (integral part) and milliseconds (fractional part) represented by the timecode
 */
double
timecode_to_sec(int32_t timecode);

/**
 * Converts a duration in seconds to a human-readable string in the format HH:MM:SS.mmm
 * @param length_sec
 * @return 
 */
char*
format_duration(double length_sec);

void
format_duration_to(double length_sec, char* str);


/*
 * Linked list functions
 */


void
add_stream_clip(stream_clip_list_t* list, stream_clip_t* clip);

stream_clip_t*
get_stream_clip_at(stream_clip_list_t* list, int index);


/*
 * Binary file handling
 */


long
file_get_length(FILE* file);

int16_t
get_int16(char* bytes);

int32_t
get_int32(char* bytes);

/**
 * Converts a sequence of bytes to an int16_t and advances the cursor position by +2.
 * @param bytes
 * @param cursor
 * @return 
 */
int16_t
get_int16_cursor(char* bytes, int* cursor);

/**
 * Converts a sequence of bytes to an int32_t and advances the cursor position by +4.
 * @param bytes
 * @param cursor
 * @return 
 */
int32_t
get_int32_cursor(char* bytes, int* cursor);

char*
file_read_string(FILE* file, int offset, int length);

/**
 * Reads a sequence of bytes into a C string and advances the cursor position by #{length}.
 * @param file
 * @param offset
 * @param length
 * @return 
 */
char*
file_read_string_cursor(FILE* file, int* offset, int length);

/**
 * Copies a sequence of bytes into a newly alloc'd C string and advances the cursor position by #{length}.
 * @param bytes
 * @param offset
 * @param length
 * @return 
 */
char*
copy_string_cursor(char* bytes, int* offset, int length);


/*
 * Struct initialization
 */


mpls_file_t
create_mpls_file_t();

void
init_mpls_file_t(mpls_file_t* mpls_file);

stream_clip_t
create_stream_clip_t();

void
init_stream_clip_t(stream_clip_t* stream_clip);

void
init_stream_clip_list_t(stream_clip_list_t* list);

playlist_t
create_playlist_t();

void
init_playlist_t(playlist_t* playlist);


/*
 * Struct freeing
 */


void
free_stream_clip_list(stream_clip_list_t* list);

void
free_mpls_file_members(mpls_file_t* mpls_file);

void
free_playlist_members(playlist_t* playlist);


/*
 * Main parsing functions
 */


mpls_file_t
init_mpls(char* path);

void
parse_stream_clips(mpls_file_t* mpls_file, playlist_t* playlist);

void
parse_stream_clip();

void
parse_chapters(mpls_file_t* mpls_file, playlist_t* playlist);

void
parse_chapter();

void
parse_mpls(char* path);



#ifdef	__cplusplus
}
#endif

#endif	/* PARSE_MPLS_H */

