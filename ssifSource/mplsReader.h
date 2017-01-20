/* 
 * File:   parse_mpls.h
 * Author: tkmax82
 *
 * Created on January 22, 2013, 10:48 AM
 */

#pragma once

#include <libfileread.h>

#ifdef _MSC_VER
#include <stdint.h>
#else
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
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


/*
 * Structs - BD-ROM
 */

typedef struct {
    char* path;
    char* name;
    UDFFILE file;
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
	int32_t raw_duration;
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

void die(const char* filename, int line_number, const char* format, ...);
void format_duration_to(double length_sec, char* str);


//Struct initialization
void init_mpls_file_t(mpls_file_t* mpls_file);
void init_stream_clip_t(stream_clip_t* stream_clip);
playlist_t create_playlist_t();
void init_playlist_t(playlist_t* playlist);

// Struct freeing
void free_mpls_file_members(mpls_file_t* mpls_file);
void free_playlist_members(playlist_t* playlist);

// Main parsing functions
mpls_file_t init_mpls(char* path);
void parse_stream_clips(mpls_file_t* mpls_file, playlist_t* playlist);

// printing functions 
void print_stream_clips_header(playlist_t* playlist);
void print_stream_clips(playlist_t* playlist);

#ifdef	__cplusplus
}
#endif
