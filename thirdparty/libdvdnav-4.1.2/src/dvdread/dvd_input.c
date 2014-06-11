/*
 * Copyright (C) 2002 Samuel Hocevar <sam@zoy.org>,
 *                    Håkan Hjort <d95hjort@dtek.chalmers.se>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <signal.h>

#include "dvd_reader.h"
#include "dvd_input.h"

//#define DEBUG

/* The function pointers that is the exported interface of this file. */
#if 0
THREAD_SAFE dvd_input_t (*dvdinput_open)  (const char *) = NULL;
THREAD_SAFE int         (*dvdinput_close) (dvd_input_t)  = NULL;
THREAD_SAFE int         (*dvdinput_seek)  (dvd_input_t, int) = NULL;
THREAD_SAFE int         (*dvdinput_title) (dvd_input_t, int) = NULL;
THREAD_SAFE int         (*dvdinput_read)  (dvd_input_t, void *, int, int) = NULL;
THREAD_SAFE char *      (*dvdinput_error) (dvd_input_t) = NULL;
#endif
THREAD_SAFE char *      dvdinput_unrar_cmd  = NULL;
THREAD_SAFE char *      dvdinput_unrar_file = NULL;
THREAD_SAFE int         dvdinput_firstrun = 1;

#ifdef HAVE_DVDCSS_DVDCSS_H
/* linking to libdvdcss */
#include <dvdcss/dvdcss.h>
#define DVDcss_open(a) dvdcss_open((char*)(a))
#define DVDcss_close   dvdcss_close
#define DVDcss_seek    dvdcss_seek
#define DVDcss_title   dvdcss_title
#define DVDcss_read    dvdcss_read
#define DVDcss_error   dvdcss_error
#else

/* dlopening libdvdcss */
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
/* Only needed on MINGW at the moment */
#include "../../msvc/contrib/dlfcn.c"
#endif

typedef struct dvdcss_s *dvdcss_handle;
static dvdcss_handle (*DVDcss_open)  (const char *);
static int           (*DVDcss_close) (dvdcss_handle);
static int           (*DVDcss_seek)  (dvdcss_handle, int, int);
static int           (*DVDcss_title) (dvdcss_handle, int);
static int           (*DVDcss_read)  (dvdcss_handle, void *, int, int);
static char *        (*DVDcss_error) (dvdcss_handle);
#endif

/* When is it faster to respawn unrar, rather than seek by eating bytes? */
#define MAXIMUM_SEEK_SIZE 1048576

/* The DVDinput handle, add stuff here for new input methods. */
struct dvd_input_s {
  /* libdvdcss handle */
  dvdcss_handle dvdcss;

  /* dummy file input */
  int fd;

	/* unrar support */
	char *unrar_archive;
	char *unrar_file;
	FILE *unrar_stream;
	off_t seek_pos;
	off_t current_pos;
};


/**
 * initialize and open a DVD device or file.
 */
static dvd_input_t css_open(const char *target)
{
  dvd_input_t dev;

  /* Allocate the handle structure */
  dev = (dvd_input_t) malloc(sizeof(*dev));
  if(dev == NULL) {
    fprintf(stderr, "libdvdread: Could not allocate memory.\n");
    return NULL;
  }

  /* Really open it with libdvdcss */
  dev->dvdcss = DVDcss_open(target);
  if(dev->dvdcss == 0) {
    fprintf(stderr, "libdvdread: Could not open %s with libdvdcss.\n", target);
    free(dev);
    return NULL;
  }

  return dev;
}

/**
 * return the last error message
 */
static char *css_error(dvd_input_t dev)
{
  return DVDcss_error(dev->dvdcss);
}

/**
 * seek into the device.
 */
static int css_seek(dvd_input_t dev, int blocks)
{
  /* DVDINPUT_NOFLAGS should match the DVDCSS_NOFLAGS value. */
  return DVDcss_seek(dev->dvdcss, blocks, DVDINPUT_NOFLAGS);
}

/**
 * set the block for the begining of a new title (key).
 */
static int css_title(dvd_input_t dev, int block)
{
  return DVDcss_title(dev->dvdcss, block);
}

/**
 * read data from the device.
 */
static int css_read(dvd_input_t dev, void *buffer, int blocks, int flags)
{
  return DVDcss_read(dev->dvdcss, buffer, blocks, flags);
}

/**
 * close the DVD device and clean up the library.
 */
static int css_close(dvd_input_t dev)
{
  int ret;

  ret = DVDcss_close(dev->dvdcss);

  if(ret < 0)
    return ret;

  free(dev);

  return 0;
}






/**
 * initialize and open a DVD device or file.
 */
dvd_input_t file_open(const char *target)
{
  dvd_input_t dev;

  /* Allocate the library structure */
  dev = (dvd_input_t) malloc(sizeof(*dev));
  if(dev == NULL) {
    fprintf(stderr, "libdvdread: Could not allocate memory.\n");
    return NULL;
  }

  /* Open the device */
#ifndef WIN32
  dev->fd = open(target, O_RDONLY);
#else
  dev->fd = open(target, O_RDONLY | O_BINARY);
#endif
  if(dev->fd < 0) {
    perror("libdvdread: Could not open input");
    free(dev);
    return NULL;
  }

  return dev;
}

/**
 * return the last error message
 */
char *file_error(dvd_input_t dev)
{
  /* use strerror(errno)? */
  return (char *)"unknown error";
}

/**
 * seek into the device.
 */
int file_seek(dvd_input_t dev, int blocks)
{
  off_t pos;

  pos = lseek(dev->fd, (off_t)blocks * (off_t)DVD_VIDEO_LB_LEN, SEEK_SET);
  if(pos < 0) {
      return pos;
  }
  /* assert pos % DVD_VIDEO_LB_LEN == 0 */
  return (int) (pos / DVD_VIDEO_LB_LEN);
}

/**
 * set the block for the begining of a new title (key).
 */
int file_title(dvd_input_t dev, int block)
{
  return -1;
}

/**
 * read data from the device.
 */
int file_read(dvd_input_t dev, void *buffer, int blocks, int flags)
{
  size_t len;
  ssize_t ret;

  len = (size_t)blocks * DVD_VIDEO_LB_LEN;

  while(len > 0) {

    ret = read(dev->fd, buffer, len);

    if(ret < 0) {
      /* One of the reads failed, too bad.  We won't even bother
       * returning the reads that went ok, and as in the posix spec
       * the file postition is left unspecified after a failure. */
      return ret;
    }

    if(ret == 0) {
      /* Nothing more to read.  Return the whole blocks, if any, that we got.
	 and adjust the file possition back to the previous block boundary. */
      size_t bytes = (size_t)blocks * DVD_VIDEO_LB_LEN - len;
      off_t over_read = -(bytes % DVD_VIDEO_LB_LEN);
      /*off_t pos =*/ lseek(dev->fd, over_read, SEEK_CUR);
      /* should have pos % 2048 == 0 */
      return (int) (bytes / DVD_VIDEO_LB_LEN);
    }

    len -= ret;
  }

  return blocks;
}

/**
 * close the DVD device and clean up.
 */
int file_close(dvd_input_t dev)
{
  int ret;

  ret = close(dev->fd);

  if(ret < 0)
    return ret;

  free(dev);

  return 0;
}






/**
 * initialize and open a DVD device or file.
 */
static dvd_input_t rarfile_open(const char *target)
{
  dvd_input_t dev;
#ifdef DEBUG
  int ret;
  char buffer[1024];
#endif

  /* Allocate the library structure */
  dev = (dvd_input_t) malloc(sizeof(*dev));
  if(dev == NULL) {
    fprintf(stderr, "libdvdread: Could not allocate memory.\n");
    return NULL;
  }

  dev->unrar_stream = NULL;
  dev->unrar_file   = NULL;
  dev->seek_pos     = 0;  /* Assume start of file */
  dev->current_pos  = 0;

  /* This is kind of sad, to use setupRAR to pass the rarfile name, then
	 hope that dvdinput_open is called after. We could also do some sort
	 of filename separator parsing perhaps. */

  /* Allocated in setupRAR, move it over. */
  dev->unrar_archive  = dvdinput_unrar_file;
  dvdinput_unrar_file = NULL;

  /* The filename inside the RAR archive to operate on */
  dev->unrar_file = strdup(target);

  /* Lets call unrar, to list the release, as a way to quickly check that
	 all the volumes are present, and the unrar_cmd works. */
#ifdef DEBUGX

  if (dvdinput_firstrun) {

	  dvdinput_firstrun = 0;

	  snprintf(buffer, sizeof(buffer), "%s v -v -c- -p- -y -cfg- -- \"%s\"",
			   dvdinput_unrar_cmd, dev->unrar_archive);

	  dev->unrar_stream = popen(buffer,
#ifdef WIN32
								"rb"
#else
								"r" /* Curse the person who made 'b' be an error. */
#endif
								);

	  if(!dev->unrar_stream) {
		  perror("libdvdread: Could not spawn unrar: ");
		  fprintf(stderr, "cmd: '%s'\r\n", buffer);
		  free(dev);
		  return NULL;
	  }

	  while(fgets(buffer, sizeof(buffer), dev->unrar_stream)) ;

	  ret = pclose(dev->unrar_stream);
	  dev->unrar_stream = NULL;

	  /* Only allow returncode 0? Or also allow "missing archives" for those
		 who want to start watching while it is still downloading? */
	  fprintf(stderr, "libdvdread: unrar test complete: %d\r\n",
			  WEXITSTATUS(ret));

	//	  if (WIFEXITED(ret) && (WEXITSTATUS(ret) == 0)) {
	  if (WEXITSTATUS(ret) == 0) {
		  return dev;
	  }

	  perror("libdvdread: Could not test rar archive: ");
	  free(dev);
	  return NULL;

  } /* firstrun */

#endif


  return dev;
}


/**
 * return the last error message
 */
static char *rarfile_error(dvd_input_t dev)
{
  /* use strerror(errno)? */
  return (char *)"unknown error";
}

/**
 * seek into the device.
 */
static int rarfile_seek(dvd_input_t dev, int blocks)
{
#ifdef DEBUGX
    fprintf(stderr, "libdvdread: rarfile_seek(block -> %d)\r\n", blocks);
#endif

  dev->seek_pos = (off_t)blocks * (off_t)DVD_VIDEO_LB_LEN;

  /* assert pos % DVD_VIDEO_LB_LEN == 0 */
  return (int) (dev->seek_pos / DVD_VIDEO_LB_LEN);
}

/**
 * set the block for the begining of a new title (key).
 */
static int rarfile_title(dvd_input_t dev, int block)
{
  return -1;
}

/**
 * read data from the device.
 *
 * Unfortunately, the UDF layer asks for the same block MANY times, and
 * we have to "seek back" one block by restarting unrar. This should be
 * fixed. Please do so if you have time.
 *
 */
static int rarfile_read(dvd_input_t dev, void *buffer, int blocks, int flags)
{
	size_t len, read_size;
	ssize_t ret;
	char ibuffer[DVD_VIDEO_LB_LEN];

#ifdef DEBUGX
	  fprintf(stderr, "libdvdread: rarfile_read (%d blocks)\r\n", blocks);
#endif


  /* First, position ourselves where the API wants us. This may mean
   * spawning new unrar, or possibly eating data until the correct
   * position. */

  /* We already have a stream, here, we can be exactly at the right place,
   * or, need to eat data until the correct position, or
   * if the seek is too far, close this unrar, and spawn a new.
   */
  if (dev->unrar_stream) {

	  /* eat data? */
	  if (dev->seek_pos > dev->current_pos) {

		  /* Seek too far? Better to spawn new unrar? */
		  if ((dev->seek_pos - dev->current_pos) >=
              (off_t)MAXIMUM_SEEK_SIZE) {
#ifdef DEBUG
			  fprintf(stderr, "libdvdread: seek too large, re-spawning unrar\r\n");
#endif
			  pclose(dev->unrar_stream);
			  dev->unrar_stream = NULL;

		  } else {

			  /* Not too far, read and eat bytes. */
			  while (dev->seek_pos > dev->current_pos) {

				  /* Work out how much we need to read, but no larger than
				   * the size of our buffer.*/

				  read_size = dev->seek_pos - dev->current_pos;
				  if (read_size > sizeof(ibuffer))
					  read_size = sizeof(ibuffer);

				  if ((fread(ibuffer, read_size, 1, dev->unrar_stream)) != 1) {
					  /* Something failed, lets close, and let's try respawn */
					  pclose(dev->unrar_stream);
					  dev->unrar_stream = NULL;
					  break;
				  }

				  dev->current_pos += read_size;
			  } /* while seek > current */
		  } /* NOT >= max seek */
	  } /* NOT seek > current */

	  /* Also check if seek < current, then we must restart unrar */
	  if (dev->seek_pos < dev->current_pos) {

		  pclose(dev->unrar_stream);
		  dev->unrar_stream = NULL;

	  }

  } /* we have active stream */

  /* Spawn new unrar? */
  if (!dev->unrar_stream) {
	  snprintf(ibuffer, sizeof(ibuffer),
			   "%s p -inul -c- -p- -y -cfg- -sk%"PRIu64" -- \"%s\" \"%s\"",
			   dvdinput_unrar_cmd,
			   dev->seek_pos,
			   dev->unrar_file,
			   dev->unrar_archive);

#ifdef DEBUGX
	  fprintf(stderr, "libdvdvread: spawning '%s' for %"PRIu64"\r\n",
              ibuffer, dev->seek_pos);
#endif

#ifndef WIN32
      // We must make sure PIPE is set to terminate process when we close
      // the handle to the unrar child
      {
          static struct sigaction sa, restore_chld_sa, restore_pipe_sa;
          sa.sa_flags = 0;
          sigemptyset (&sa.sa_mask);
          sa.sa_handler = SIG_DFL;
          sigaction (SIGPIPE, &sa, &restore_pipe_sa);
#endif

          dev->unrar_stream = popen(ibuffer,
#ifdef WIN32
                                    "rb"
#else
                                    "r"
#endif
                                    );
#ifndef WIN32
          // Restore signal
          sigaction(SIGPIPE, &restore_pipe_sa, NULL);
      }
#endif

	  if (!dev->unrar_stream) {
		  return -1;
	  }

	  /* Update ptr */
	  dev->current_pos = dev->seek_pos;

  }

  /* Assert current == seek ? */
#ifdef DEBUG
  if (dev->current_pos != dev->seek_pos)
	  fprintf(stderr, "libdvdread: somehow, current_pos != seek_pos!?\r\n");
#endif

  len = (size_t)blocks * DVD_VIDEO_LB_LEN;

  while(len > 0) {

	  /* ret = read(dev->fd, buffer, len); */
	  ret = fread(buffer, 1, len, dev->unrar_stream);

#ifdef DEBUGX
	  fprintf(stderr, "libdvdread: fread(%d) -> %d\r\n", (int)len, (int)ret);
#endif

	  if((ret != len) && ferror(dev->unrar_stream)) {
		  /* One of the reads failed, too bad.  We won't even bother
		   * returning the reads that went ok, and as in the posix spec
		   * the file postition is left unspecified after a failure. */
		  return -1;
	  }

	  if (ret < 0)
		  return -1;

	  if ((ret != len) || feof(dev->unrar_stream)) {
		  /* Nothing more to read.  Return the whole blocks, if any,
		   * that we got. and adjust the file position back to the
		   * previous block boundary. */
		  size_t bytes = (size_t)blocks * DVD_VIDEO_LB_LEN - len; /* 'ret'? */
		  off_t over_read = -(bytes % DVD_VIDEO_LB_LEN);
		  /*off_t pos =*/ /*lseek(dev->fd, over_read, SEEK_CUR);*/
		  dev->current_pos += (off_t)len;
		  dev->seek_pos = dev->current_pos + (off_t)len + over_read;
		  /* minus over_read, I did not touch the code above, but I wonder
		   * if it is correct. It does not even use "ret" in the math. */

		  /* should have pos % 2048 == 0 */
		  return (int) (bytes / DVD_VIDEO_LB_LEN);
	  }

	  dev->current_pos += (off_t) ret;
	  dev->seek_pos += (off_t) ret;
	  len -= ret;
  }

  return blocks;
}

/**
 * close the DVD device and clean up.
 */
static int rarfile_close(dvd_input_t dev)
{

#ifdef DEBUG
    fprintf(stderr, "libdvdread: rarfile_close\r\n");
#endif

  if (dev->unrar_stream)
	  pclose(dev->unrar_stream);
  dev->unrar_stream = NULL;

  if (dev->unrar_archive)
	  free(dev->unrar_archive);
  dev->unrar_archive = NULL;

  if (dev->unrar_file)
	  free(dev->unrar_file);
  dev->unrar_file = NULL;

  free(dev);

  return 0;
}



//
// This function should be renamed to something generic, with method ENUMS
// to set DEFAULT or UNRAR.
// This function takes the path to unrar "/usr/local/bin/unrar" and the
// name of the ISO file inside the rar archive. "/movie.iso"
//
int dvdinput_setupRAR(const char *unrar_path, const char *rarfile)
{

#ifdef DEBUG
    fprintf(stderr, "libdvdread: Configuring for unrar.\n");
#endif
	/* Search for unrar in $PATH, common places etc, as well as "unrar_path" *
	 * But we really need unrar with special SEEK -sk option for this to
	   work */

    if (dvdinput_unrar_cmd) {
        free(dvdinput_unrar_cmd);
        dvdinput_unrar_cmd = NULL;
    }

    if (dvdinput_unrar_file) {
        free(dvdinput_unrar_file);
        dvdinput_unrar_file = NULL;
    }

    // Did they ask to disable RAR support?
    if (!unrar_path || !rarfile) return 0;

	dvdinput_unrar_cmd  = strdup(unrar_path);
	dvdinput_unrar_file = strdup(rarfile);

	/* Check it is present, and executable? */

	return 0;
}





int dvdinput_setup(void) {return 0;}

#if 0
/**
 * Setup read functions with either libdvdcss or minimal DVD access.
 */
int dvdinput_setup(void)
{
  void *dvdcss_library = NULL;
  char **dvdcss_version = NULL;


  // If we are currently in RAR mode, we do not need to load dvdcss.

  if (dvdinput_unrar_file) {
      fprintf(stderr, "libdvdread: configured to use RAR on '%s'\r\n",
              dvdinput_unrar_file);
      dvdinput_open  = rarfile_open;
      dvdinput_close = rarfile_close;
      dvdinput_seek  = rarfile_seek;
      dvdinput_title = rarfile_title;
      dvdinput_read  = rarfile_read;
      dvdinput_error = rarfile_error;
      return 0; // No CSS
  }


#ifdef HAVE_DVDCSS_DVDCSS_H
  /* linking to libdvdcss */
  dvdcss_library = &dvdcss_library;  /* Give it some value != NULL */
  /* the DVDcss_* functions have been #defined at the top */
  dvdcss_version = &dvdcss_interface_2;

#else
  /* dlopening libdvdcss */

#ifdef __APPLE__
  #define CSS_LIB "libdvdcss.2.dylib"
#elif defined(WIN32)
  #define CSS_LIB "libdvdcss.dll"
#else
  #define CSS_LIB "libdvdcss.so.2"
#endif
  dvdcss_library = dlopen(CSS_LIB, RTLD_LAZY);

  if(dvdcss_library != NULL) {
#if defined(__OpenBSD__) && !defined(__ELF__)
#define U_S "_"
#else
#define U_S
#endif
    DVDcss_open = (dvdcss_handle (*)(const char*))
      dlsym(dvdcss_library, U_S "dvdcss_open");
    DVDcss_close = (int (*)(dvdcss_handle))
      dlsym(dvdcss_library, U_S "dvdcss_close");
    DVDcss_title = (int (*)(dvdcss_handle, int))
      dlsym(dvdcss_library, U_S "dvdcss_title");
    DVDcss_seek = (int (*)(dvdcss_handle, int, int))
      dlsym(dvdcss_library, U_S "dvdcss_seek");
    DVDcss_read = (int (*)(dvdcss_handle, void*, int, int))
      dlsym(dvdcss_library, U_S "dvdcss_read");
    DVDcss_error = (char* (*)(dvdcss_handle))
      dlsym(dvdcss_library, U_S "dvdcss_error");

    dvdcss_version = (char **)dlsym(dvdcss_library, U_S "dvdcss_interface_2");

    if(dlsym(dvdcss_library, U_S "dvdcss_crack")) {
      fprintf(stderr,
	      "libdvdread: Old (pre-0.0.2) version of libdvdcss found.\n"
	      "libdvdread: You should get the latest version from "
	      "http://www.videolan.org/\n" );
      dlclose(dvdcss_library);
      dvdcss_library = NULL;
    } else if(!DVDcss_open  || !DVDcss_close || !DVDcss_title || !DVDcss_seek
	      || !DVDcss_read || !DVDcss_error || !dvdcss_version) {
      fprintf(stderr,  "libdvdread: Missing symbols in %s, "
	      "this shouldn't happen !\n", CSS_LIB);
      dlclose(dvdcss_library);
    }
  }
#endif /* HAVE_DVDCSS_DVDCSS_H */

  if(dvdcss_library != NULL) {
    /*
    char *psz_method = getenv( "DVDCSS_METHOD" );
    char *psz_verbose = getenv( "DVDCSS_VERBOSE" );
    fprintf(stderr, "DVDCSS_METHOD %s\n", psz_method);
    fprintf(stderr, "DVDCSS_VERBOSE %s\n", psz_verbose);
    */
    fprintf(stderr, "libdvdread: Using libdvdcss version %s for DVD access\n",
	    *dvdcss_version);

    /* libdvdcss wrapper functions */
    dvdinput_open  = css_open;
    dvdinput_close = css_close;
    dvdinput_seek  = css_seek;
    dvdinput_title = css_title;
    dvdinput_read  = css_read;
    dvdinput_error = css_error;
    return 1;

  } else {
    fprintf(stderr, "libdvdread: Encrypted DVD support unavailable.\n");

    /* libdvdcss replacement functions */
    dvdinput_open  = file_open;
    dvdinput_close = file_close;
    dvdinput_seek  = file_seek;
    dvdinput_title = file_title;
    dvdinput_read  = file_read;
    dvdinput_error = file_error;
    return 0;
  }
}

#endif