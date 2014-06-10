#define _CRT_SECURE_NO_WARNINGS

/* 
 * File:   parse_mpls.c
 * Author: Andrew C. Dvorak <andy@andydvorak.net>
 *
 * Created on January 21, 2013, 2:43 PM
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _SYSLIMITS_H_
#include <syslimits.h>
#endif
#ifdef _SYS_SYSLIMITS_H_
#include <sys/syslimits.h>
#endif

#include "stdint.h"
#include <libdvdnav-4.1.2\dvdfopen\dvdfopen.h>
#include "mpls_parse.h"



#ifdef _MSC_VER

#include <assert.h>
#include <Windows.h>
#define MAXPATHLEN MAX_PATH
/*---------------------------------------------------------------------------*/
static
char *dirname(char *path)
{
	static char buffer[260];
	size_t len;
	if (path==NULL) {
		strcpy(buffer, ".");
		return buffer;
	}
	len = strlen(path);
	assert(len<sizeof(buffer));
	if (len!=0 && (path[len-1]=='/' || path[len-1]=='\\')) {
		--len;
	}
	while (len!=0 && path[len-1]!='/' && path[len-1]!='\\') {
		--len;
	}
	if (len==0) {
		strcpy(buffer, ".");
	} else if (len==1) {
		if (path[0]=='/' || path[0]=='\\') {
			strcpy(buffer, "/");
		} else {
			strcpy(buffer, ".");
		}
	} else {
		memcpy(buffer, path, len-1);
	}
	return buffer;
}
/*---------------------------------------------------------------------------*/
static
char *basename(char *path)
{
	static char buffer[260];
	size_t len_start, len;
	if (path==NULL) {
		strcpy(buffer, ".");
		return buffer;
	}
	len = strlen(path);
	assert(len<sizeof(buffer));
	if (len!=0 && (path[len-1]=='/' || path[len-1]=='\\')) {
		if (len==1) {
			strcpy(buffer, "/");
			return buffer;
		}
		--len;
	}
	len_start = len;
	while (len!=0 && path[len-1]!='/' && path[len-1]!='\\') {
		--len;
	}
	if (len==0) {
		strcpy(buffer, ".");
	} else {
		memcpy(buffer, path + len, len_start - len);
	}

	return buffer;
}
/*---------------------------------------------------------------------------*/
static
char *realpath(const char *path, char *resolved_path)
{
	char *pszFilePart;
	if (resolved_path == NULL)
		resolved_path = (char*)malloc(MAX_PATH);
	if (GetFullPathNameA(path, MAXPATHLEN, resolved_path, &pszFilePart)==0)
		return NULL;
	return resolved_path;
}
/*---------------------------------------------------------------------------*/

#endif


/*
 * Utility functions
 */


void
die (const char* filename, int line_number, const char * format, ...)
{
    va_list vargs;
    va_start (vargs, format);
    fprintf (stderr, "%s:%d: ", filename, line_number);
    vfprintf (stderr, format, vargs);
    fprintf (stderr, "\n");
    exit (EXIT_FAILURE);
}

double
timecode_to_sec(int32_t timecode)
{
    return (double)timecode / TIMECODE_DIV;
}

char*
format_duration(double length_sec)
{
    char* str = (char*) calloc(15, sizeof(char));
    format_duration_to(length_sec, str);
    return str;
}

void
format_duration_to(double length_sec, char* dest)
{
    sprintf(dest, "%02.0f:%02.0f:%06.3f", floor(length_sec / 3600), floor(fmod(length_sec, 3600) / 60), fmod(length_sec, 60));
}


/*
 * Linked list functions
 */


void
add_stream_clip(stream_clip_list_t* list, stream_clip_t* clip)
{
    clip->next = NULL;
    clip->index = list->count;

    if (list->first == NULL)
    {
        list->first = clip;
    }
    else
    {
        list->last->next = clip;
    }

    list->last = clip;
    list->count++;
}

stream_clip_t*
get_stream_clip_at(stream_clip_list_t* list, int index)
{
    stream_clip_t* clip = list->first;
    while (clip != NULL)
    {
        if (clip->index == index)
            break;
        clip = clip->next;
    }
    return clip;
}


/*
 * Binary file handling
 */


int16_t
get_int16(char* bytes)
{
    uint8_t b1 = (uint8_t) bytes[0];
    uint8_t b2 = (uint8_t) bytes[1];
//    printf("%02x %02x \n", b1, b2);
    return (b1 << 8) |
           (b2 << 0);
}

int32_t
get_int32(char* bytes)
{
    uint8_t b1 = (uint8_t) bytes[0];
    uint8_t b2 = (uint8_t) bytes[1];
    uint8_t b3 = (uint8_t) bytes[2];
    uint8_t b4 = (uint8_t) bytes[3];
//    printf("%02x %02x %02x %02x \n", b1, b2, b3, b4);
    return (b1 << 24) |
           (b2 << 16) |
           (b3 <<  8) |
           (b4 <<  0);
}

int16_t
get_int16_cursor(char* bytes, int* cursor)
{
    int16_t val = get_int16(bytes + *cursor);
    *cursor += 2;
    return val;
}

int32_t
get_int32_cursor(char* bytes, int* cursor)
{
    int32_t val = get_int32(bytes + *cursor);
    *cursor += 4;
    return val;
}

char*
file_read_string(common_file_reader* file, int offset, int length)
{
    // Allocate an empty string initialized to all NULL chars
    char* chars = (char*) calloc(length + 1, sizeof(char));
    
    // Jump to the requested position (byte offset) in the file
    file->seek(offset, -1);
    
    // Read 4 bytes of data into a char array
    // e.g., fread(chars, 1, 8) = first eight chars
    int br = (int)file->read(chars, length);
    
    if(br != length)
    {
        DIE("Wrong number of chars read in file_get_chars(): expected %i, found %i", length, br);
    }
    
    // Reset cursor to beginning of file
    file->seek(0, -1);

    return chars;
}

char*
copy_string_cursor(char* bytes, int* offset, int length)
{
    char* str = (char*) calloc(length + 1, sizeof(char));
    strncpy(str, bytes + *offset, length);
    *offset += length;
    return str;
}


/*
 * Struct initialization
 */


mpls_file_t
create_mpls_file_t()
{
    mpls_file_t mpls_file;
    init_mpls_file_t(&mpls_file);
    return mpls_file;
}

void
init_mpls_file_t(mpls_file_t* mpls_file)
{
    int i;
    mpls_file->path = NULL;
    mpls_file->name = NULL;
    mpls_file->file = NULL;
    mpls_file->size = 0;
    mpls_file->data = NULL;
    for (i = 0; i < 9; i++)
        mpls_file->header[i] = 0;
    mpls_file->pos = 0;
    mpls_file->playlist_pos = 0;
    mpls_file->chapter_pos = 0;
    mpls_file->total_chapter_count = 0;
    mpls_file->time_in = 0;
    mpls_file->time_out = 0;
}

stream_clip_t
create_stream_clip_t()
{
    stream_clip_t stream_clip;
    init_stream_clip_t(&stream_clip);
    return stream_clip;
}

void
init_stream_clip_t(stream_clip_t* stream_clip)
{
    int i;
    for (i = 0; i < 11; i++)
        stream_clip->filename[i] = 0;
    stream_clip->time_in_sec = 0;
    stream_clip->time_out_sec = 0;
    stream_clip->duration_sec = 0;
    stream_clip->relative_time_in_sec = 0;
    stream_clip->relative_time_out_sec = 0;
    stream_clip->track_count = 0;
    stream_clip->video_count = 0;
    stream_clip->audio_count = 0;
    stream_clip->subtitle_count = 0;
    stream_clip->interactive_menu_count = 0;
    stream_clip->secondary_video_count = 0;
    stream_clip->secondary_audio_count = 0;
    stream_clip->pip_count = 0;
    stream_clip->index = 0;
    stream_clip->next = NULL;
}

void
init_stream_clip_list_t(stream_clip_list_t* list)
{
    list->first = NULL;
    list->count = 0;
}

playlist_t
create_playlist_t()
{
    playlist_t playlist;
    init_playlist_t(&playlist);
    return playlist;
}

void
init_playlist_t(playlist_t* playlist)
{
    int i;
    for (i = 0; i < 11; i++)
        playlist->filename[i] = 0;
    playlist->time_in_sec = 0;
    playlist->time_out_sec = 0;
    playlist->duration_sec = 0;
    for (i = 0; i < 15; i++)
        playlist->duration_formatted[i] = 0;
    playlist->chapters = NULL;
    playlist->chapter_count = 0;
    init_stream_clip_list_t(&playlist->stream_clip_list);
    init_stream_clip_list_t(&playlist->chapter_stream_clip_list);
}


/*
 * Struct freeing
 */


void
free_stream_clip_list(stream_clip_list_t* list)
{
    stream_clip_t* clip = list->first;
    stream_clip_t* next = NULL;
    while (clip != NULL)
    {
        next = clip->next;
        free(clip);
        clip = next;
    }
    list->first = list->last = NULL;
    list->count = 0;
}

void
free_mpls_file_members(mpls_file_t* mpls_file)
{
    free(mpls_file->path); mpls_file->path = NULL;
    free(mpls_file->data); mpls_file->data = NULL;
}

void
free_playlist_members(playlist_t* playlist)
{
    // playlist.stream_clip_list contains ALL stream clips;
    // playlist.chapter_stream_clip_list only contains a subset.
    // So we only need to free .stream_clip_list nodes.
    free_stream_clip_list(&playlist->stream_clip_list);
    free(playlist->chapters); playlist->chapters = NULL;
}


/*
 * Private inline functions
 */


static void
copy_header(char* header, char* data, int* offset)
{
    strncpy(header, data + *offset, 8);
    *offset += 8;
}


/*
 * Main parsing functions
 */


mpls_file_t
init_mpls(char* path)
{
    mpls_file_t mpls_file = create_mpls_file_t();

    mpls_file.path = realpath(path, NULL);
    if (mpls_file.path == NULL)
    {
        DIE("Unable to get the full path (realpath) of \"%s\".", path);
    }
    
    mpls_file.name = basename(mpls_file.path);
    if (mpls_file.name == NULL)
    {
        DIE("Unable to get the file name (basename) of \"%s\".", path);
    }
    
    mpls_file.file = universal_fopen(mpls_file.path);
    if (mpls_file.file == NULL)
    {
        DIE("Unable to open \"%s\" for reading.", mpls_file.path);
    }

    mpls_file.size = (long)mpls_file.file->filesize();
    if (mpls_file.size < 90)
    {
        DIE("Invalid MPLS file (too small): \"%s\".", mpls_file.path);
    }
    
    mpls_file.data = file_read_string(mpls_file.file, 0, mpls_file.size);
    
    char* data = mpls_file.data;
    int* pos_ptr = &(mpls_file.pos);
    
    // Verify header
    copy_header(mpls_file.header, data, pos_ptr);
    if (strncmp("MPLS0100", mpls_file.header, 8) != 0 &&
        strncmp("MPLS0200", mpls_file.header, 8) != 0)
    {
        DIE("Invalid header in \"%s\": expected MPLS0100 or MPLS0200, found \"%s\".", mpls_file.path, mpls_file.header);
    }
    
    // Verify playlist offset
    mpls_file.playlist_pos = get_int32_cursor(data, pos_ptr);
    if (mpls_file.playlist_pos <= 8)
    {
        DIE("Invalid playlists offset: %i.", mpls_file.playlist_pos);
    }
    
    // Verify chapter offset
    int32_t chaptersPos = get_int32_cursor(data, pos_ptr);
    if (chaptersPos <= 8)
    {
        DIE("Invalid chapters offset: %i.", chaptersPos);
    }
    
    int chapterCountPos = chaptersPos + 4;
    mpls_file.chapter_pos = chaptersPos + 6;
    
    // Verify chapter count
    mpls_file.total_chapter_count = get_int16(data + chapterCountPos);
    if (mpls_file.total_chapter_count < 0)
    {
        DIE("Invalid chapter count: %i.", mpls_file.total_chapter_count);
    }
    
    // Verify Time IN
    mpls_file.time_in = get_int32(data + TIME_IN_POS);
    if (mpls_file.time_in < 0)
    {
        DIE("Invalid playlist time in: %i.", mpls_file.time_in);
    }
    
    // Verify Time OUT
    mpls_file.time_out = get_int32(data + TIME_OUT_POS);
    if (mpls_file.time_out < 0)
    {
        DIE("Invalid playlist time out: %i.", mpls_file.time_out);
    }
    
    return mpls_file;
}

void
parse_stream_clips(mpls_file_t* mpls_file, playlist_t* playlist)
{
    char* data = mpls_file->data;
    int* pos_ptr = &(mpls_file->pos);
    *pos_ptr = mpls_file->playlist_pos;

    playlist->time_in_sec = mpls_file->time_in;
    playlist->time_out_sec = mpls_file->time_out;
    
    /*int32_t playlist_size = */ get_int32_cursor(data, pos_ptr);
    /*int16_t playlist_reserved = */ get_int16_cursor(data, pos_ptr);
    int16_t stream_clip_count = get_int16_cursor(data, pos_ptr);
    /*int16_t playlist_subitem_count = */ get_int16_cursor(data, pos_ptr);
    
    int streamClipIndex;
    
    playlist->duration_sec = 0;
    
    for(streamClipIndex = 0; streamClipIndex < stream_clip_count; streamClipIndex++)
    {
        stream_clip_t* streamClip = (stream_clip_t*) calloc(1, sizeof(stream_clip_t));
        init_stream_clip_t(streamClip);

        add_stream_clip(&playlist->stream_clip_list, streamClip);
        add_stream_clip(&playlist->chapter_stream_clip_list, streamClip);

        int itemStart = *pos_ptr;
        int itemLength = get_int16_cursor(data, pos_ptr);
        char* itemName = copy_string_cursor(data, pos_ptr, 5); /* e.g., "00504" */
        char* itemType = copy_string_cursor(data, pos_ptr, 4); /* "M2TS" */
        
        // Will always be exactly ten (10) chars
        sprintf(streamClip->filename, "%s.%s", itemName, itemType);
        
        *pos_ptr += 1;
        int multiangle = (data[*pos_ptr] >> 4) & 0x01;
//        int condition  = (data[pos] >> 0) & 0x0F;
        *pos_ptr += 2;

        int32_t inTime = get_int32_cursor(data, pos_ptr);
        if (inTime < 0) inTime &= 0x7FFFFFFF;
        double timeIn = timecode_to_sec(inTime);

        int32_t outTime = get_int32_cursor(data, pos_ptr);
        if (outTime < 0) outTime &= 0x7FFFFFFF;
        double timeOut = timecode_to_sec(outTime);

//		printf("duration: %u\n", outTime - inTime);
		streamClip->raw_duration = outTime - inTime;


        streamClip->time_in_sec = timeIn;
        streamClip->time_out_sec = timeOut;
        streamClip->duration_sec = timeOut - timeIn;
        streamClip->relative_time_in_sec = playlist->duration_sec;
        streamClip->relative_time_out_sec = streamClip->relative_time_in_sec + streamClip->duration_sec;
        
#ifdef DEBUG
        printf("time in: %8.3f.  time out: %8.3f.  duration: %8.3f.  relative time in: %8.3f.\n",
                streamClip->time_in_sec, streamClip->time_out_sec,
                streamClip->duration_sec, streamClip->relative_time_in_sec);
#endif
        
        playlist->duration_sec += (timeOut - timeIn);
        
#ifdef DEBUG
        printf("Stream clip %2i: %s (type = %s, length = %i, multiangle = %i)\n", streamClipIndex, streamClip->filename, itemType, itemLength, multiangle);
#endif
        
        free(itemName); itemName = NULL;
        free(itemType); itemType = NULL;

        *pos_ptr += 12;
        
        if (multiangle > 0)
        {
            int angles = data[*pos_ptr];
            *pos_ptr += 2;
            int angle;
            for (angle = 0; angle < angles - 1; angle++)
            {
                /* char* angleName = */ copy_string_cursor(data, pos_ptr, 5);
                /* char* angleType = */ copy_string_cursor(data, pos_ptr, 4);
                *pos_ptr += 1;

                // TODO
                /*
                TSStreamFile angleFile = null;
                string angleFileName = string.Format(
                    "{0}.M2TS", angleName);
                if (streamFiles.ContainsKey(angleFileName))
                {
                    angleFile = streamFiles[angleFileName];
                }
                if (angleFile == null)
                {
                    throw new Exception(string.Format(
                        "Playlist {0} referenced missing angle file {1}.",
                        _fileInfo.Name, angleFileName));
                }

                TSStreamClipFile angleClipFile = null;
                string angleClipFileName = string.Format(
                    "{0}.CLPI", angleName);
                if (streamClipFiles.ContainsKey(angleClipFileName))
                {
                    angleClipFile = streamClipFiles[angleClipFileName];
                }
                if (angleClipFile == null)
                {
                    throw new Exception(string.Format(
                        "Playlist {0} referenced missing angle file {1}.",
                        _fileInfo.Name, angleClipFileName));
                }

                TSStreamClip angleClip =
                    new TSStreamClip(angleFile, angleClipFile);
                angleClip.AngleIndex = angle + 1;
                angleClip.TimeIn = streamClip->TimeIn;
                angleClip.TimeOut = streamClip->TimeOut;
                angleClip.RelativeTimeIn = streamClip->RelativeTimeIn;
                angleClip.RelativeTimeOut = streamClip->RelativeTimeOut;
                angleClip.Length = streamClip->Length;

                add_stream_clip(&playlist->stream_clip_list, angleClip);
                */
            }
            // TODO
//            if (angles - 1 > AngleCount) AngleCount = angles - 1;
        }

        /* int streamInfoLength = */ get_int16_cursor(data, pos_ptr);
        *pos_ptr += 2;
        int streamCountVideo = data[(*pos_ptr)++];
        int streamCountAudio = data[(*pos_ptr)++];
        int streamCountPG = data[(*pos_ptr)++];
        int streamCountIG = data[(*pos_ptr)++];
        int streamCountSecondaryAudio = data[(*pos_ptr)++];
        int streamCountSecondaryVideo = data[(*pos_ptr)++];
        int streamCountPIP = data[(*pos_ptr)++];
        *pos_ptr += 5;
        
        int i;

        for (i = 0; i < streamCountVideo; i++)
        {
//            TSStream stream = CreatePlaylistStream(data, ref pos);
//            if (stream != null) PlaylistStreams[stream.PID] = stream;
        }
        for (i = 0; i < streamCountAudio; i++)
        {
//            TSStream stream = CreatePlaylistStream(data, ref pos);
//            if (stream != null) PlaylistStreams[stream.PID] = stream;
        }
        for (i = 0; i < streamCountPG; i++)
        {
//            TSStream stream = CreatePlaylistStream(data, ref pos);
//            if (stream != null) PlaylistStreams[stream.PID] = stream;
        }
        for (i = 0; i < streamCountIG; i++)
        {
//            TSStream stream = CreatePlaylistStream(data, ref pos);
//            if (stream != null) PlaylistStreams[stream.PID] = stream;
        }
        for (i = 0; i < streamCountSecondaryAudio; i++)
        {
//            TSStream stream = CreatePlaylistStream(data, ref pos);
//            if (stream != null) PlaylistStreams[stream.PID] = stream;
            *pos_ptr += 2;
        }
        for (i = 0; i < streamCountSecondaryVideo; i++)
        {
//            TSStream stream = CreatePlaylistStream(data, ref pos);
//            if (stream != null) PlaylistStreams[stream.PID] = stream;
            *pos_ptr += 6;
        }
        /*
         * TODO
         * 
        for (i = 0; i < streamCountPIP; i++)
        {
            TSStream stream = CreatePlaylistStream(data, ref pos);
            if (stream != null) PlaylistStreams[stream.PID] = stream;
        }
        */

        *pos_ptr += itemLength - (*pos_ptr - itemStart) + 2;
        
        streamClip->track_count =
                streamCountVideo + streamCountAudio +
                streamCountPG + streamCountIG +
                streamCountSecondaryVideo + streamCountSecondaryAudio;
        streamClip->video_count = streamCountVideo;
        streamClip->audio_count = streamCountAudio;
        streamClip->subtitle_count = streamCountPG;
        streamClip->interactive_menu_count = streamCountIG;
        streamClip->secondary_video_count = streamCountSecondaryVideo;
        streamClip->secondary_audio_count = streamCountSecondaryAudio;
        streamClip->pip_count = streamCountPIP;
        
#ifdef DEBUG
        printf("\t\t #V: %i, #A: %i, #PG: %i, #IG: %i, #2A: %i, #2V: %i, #PiP: %i \n",
                streamCountVideo, streamCountAudio, streamCountPG, streamCountIG,
                streamCountSecondaryAudio, streamCountSecondaryVideo, streamCountPIP);
#endif
    }

    format_duration_to(playlist->duration_sec, playlist->duration_formatted);
}

void
parse_chapters(mpls_file_t* mpls_file, playlist_t* playlist)
{
    char* data = mpls_file->data;
    int* pos_ptr = &(mpls_file->pos);
    *pos_ptr = mpls_file->chapter_pos;

    int i;
    int validChapterCount = 0;
    
    for (i = 0; i < mpls_file->total_chapter_count; i++)
    {
        char* chapter = data + *pos_ptr + (i * CHAPTER_SIZE);
        if (chapter[1] == CHAPTER_TYPE_ENTRY_MARK)
        {
            validChapterCount++;
        }
    }
    
    double* chapters = (double*) calloc(validChapterCount, sizeof(double));
    
    validChapterCount = 0;
    
    *pos_ptr = mpls_file->chapter_pos;
    
    for (i = 0; i < mpls_file->total_chapter_count; i++)
    {
        char* chapter = data + *pos_ptr;
        
        if (chapter[1] == CHAPTER_TYPE_ENTRY_MARK)
        {
            
            int streamFileIndex = ((int)data[*pos_ptr + 2] << 8) + data[*pos_ptr + 3];
            
            int32_t chapterTime = get_int32(chapter + 4);

            stream_clip_t* streamClip = get_stream_clip_at(&playlist->chapter_stream_clip_list, streamFileIndex);

			if (streamClip == NULL) {
#ifdef _DEBUG
				printf("ignore bad chapter\n");
#endif
				continue;
			}

            double chapterSeconds = timecode_to_sec(chapterTime);

            double relativeSeconds =
                chapterSeconds -
                streamClip->time_in_sec +
                streamClip->relative_time_in_sec;
            
#ifdef DEBUG
            printf("streamFileIndex %2i: (%9i / %f = %8.3f) - %8.3f + %8.3f = %8.3f\n", streamFileIndex, chapterTime, TIMECODE_DIV, chapterSeconds, streamClip->time_in_sec, streamClip->relative_time_in_sec, relativeSeconds);
#endif

            // Ignore short last chapter
            // If the last chapter is < 1.0 Second before end of film Ignore
            if (playlist->duration_sec - relativeSeconds > 1.0)
            {
//                streamClip->Chapters.Add(chapterSeconds);
//                this.Chapters.Add(relativeSeconds);
                chapters[validChapterCount++] = relativeSeconds;
            }
        }
        
        *pos_ptr += CHAPTER_SIZE;
    }
    
    chapters = (double*) realloc(chapters, validChapterCount * sizeof(double));

    playlist->chapters = chapters;
    playlist->chapter_count = validChapterCount;
}

void
print_playlist_header(mpls_file_t* mpls_file, playlist_t* playlist)
{
    size_t i;
    size_t len = strlen(mpls_file->path);
    printf("%s\n", mpls_file->path);
    for (i = 0; i < len; i++)
        printf("%c", '=');
    printf("\n");
    printf("\n");
}

void
print_playlist_details(playlist_t* playlist)
{
    printf("Playlist duration: %s\n", playlist->duration_formatted);
    printf("\n");
}

void
print_tracks_header(playlist_t* playlist)
{
//    int i;
    char header[1024];
    sprintf(header, "Tracks (%i):", playlist->stream_clip_list.first->track_count);
//    size_t len = strlen(header);
    printf("%s\n", header);
//    for (i = 0; i < len; i++)
//        printf("%c", '-');
//    printf("\n");
    printf("\n");
}

void
print_tracks(playlist_t* playlist)
{
    stream_clip_t* first_clip = playlist->stream_clip_list.first;
    printf("\t type                        # \n");
    printf("\t ------------------------    --\n");
    printf("\t Primary Video:              %2i\n", first_clip->video_count);
    printf("\t Primary Audio:              %2i\n", first_clip->audio_count);
    printf("\t Subtitle (PGS):             %2i\n", first_clip->subtitle_count);
    printf("\t Interactive Menu:           %2i\n", first_clip->interactive_menu_count);
    printf("\t Secondary Video:            %2i\n", first_clip->secondary_video_count);
    printf("\t Secondary Audio:            %2i\n", first_clip->secondary_audio_count);
    printf("\t Picture-in-Picture (PiP):   %2i\n", first_clip->pip_count);
    printf("\n");
}

void
print_stream_clips_header(playlist_t* playlist)
{
//    int i;
    char header[1024];
    sprintf(header, "Stream Clips (%i):", playlist->stream_clip_list.count);
//    size_t len = strlen(header);
    printf("%s\n", header);
//    for (i = 0; i < len; i++)
//        printf("%c", '-');
//    printf("\n");
    printf("\n");
}

void
print_stream_clips(playlist_t* playlist)
{
    stream_clip_t* clip = playlist->stream_clip_list.first;
    char duration_human[15];
    printf("\t idx    filename     duration       frame_count\n");
    printf("\t ---    ----------   ------------   -----------\n");
    while (clip != NULL)
    {
        format_duration_to(clip->duration_sec, duration_human);
		int currect_framecount = (int)((double)clip->raw_duration * 24 / (45 * 1001) + 0.5);
        printf("\t %3i:   %s   %s      %8d\n", clip->index + 1, clip->filename, duration_human, currect_framecount);
        clip = clip->next;
    }
    printf("\n");
}

void
print_chapters_header(playlist_t* playlist)
{
//    int i;
    char header[1024];
    sprintf(header, "Chapters (%zu):", playlist->chapter_count);
//    size_t len = strlen(header);
    printf("%s\n", header);
//    for (i = 0; i < len; i++)
//        printf("%c", '-');
//    printf("\n");
    printf("\n");
}

void
print_chapters(playlist_t* playlist)
{
    size_t i;
    printf("\t idx    start time  \n");
    printf("\t ---    ------------\n");
    for(i = 0; i < playlist->chapter_count; i++)
    {
        double sec = playlist->chapters[i];
        char* chapter_start_human = format_duration(sec);
        printf("\t %3i:   %s\n", i + 1, chapter_start_human);
        free(chapter_start_human);
    }
    printf("\n");
}

void
parse_mpls(char* path)
{
	mpls_file_t mpls_file = init_mpls(path);
	playlist_t playlist = create_playlist_t();

	parse_stream_clips(&mpls_file, &playlist);
	parse_chapters(&mpls_file, &playlist);

	print_playlist_header(&mpls_file, &playlist);
	print_playlist_details(&playlist);
	print_tracks_header(&playlist);
	print_tracks(&playlist);
	print_stream_clips_header(&playlist);
	print_stream_clips(&playlist);
	print_chapters_header(&playlist);
	print_chapters(&playlist);

	free_playlist_members(&playlist);
	free_mpls_file_members(&mpls_file);
}



#if _CONSOLE
int main(int argc, char** argv) {
    if (argc < 2)
    {
        DIE("Usage: parse_mpls MPLS_FILE_PATH [ MPLS_FILE_PATH ... ]");
    }
    
    int i;
    for(i = 1; i < argc; i++)
    {
        parse_mpls(argv[i]);
    }
    system("pause");
    return (EXIT_SUCCESS);
}
#endif