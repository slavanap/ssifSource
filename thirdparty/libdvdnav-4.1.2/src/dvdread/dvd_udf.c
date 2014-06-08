/*
 * This code is based on dvdudf by:
 *   Christian Wolff <scarabaeus@convergence.de>.
 *
 * Modifications by:
 *   Billy Biggs <vektor@dumbterm.net>.
 *   Björn Englund <d4bjorn@dtek.chalmers.se>.
 *
 * dvdudf: parse and read the UDF volume information of a DVD Video
 * Copyright (C) 1999 Christian Wolff for convergence integrated media
 * GmbH The author can be reached at scarabaeus@convergence.de, the
 * project's page is at http://linuxtv.org/dvd/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  Or, point your browser to
 * http://www.gnu.org/copyleft/gpl.html
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>

#include "dvd_reader.h"
#define DVD_UFD_INTERNAL
#include "dvd_udf.h"


//#define DEBUG


/* Private but located in/shared with dvd_reader.c */
extern int UDFReadBlocksRaw( dvd_reader_t *device, uint32_t lb_number,
				size_t block_count, unsigned char *data,
				int encrypted );

/* It's required to either fail or deliver all the blocks asked for. */
static int DVDReadLBUDF( dvd_reader_t *device, uint32_t lb_number,
			 size_t block_count, unsigned char *data,
			 int encrypted )
{
  int ret;
  size_t count = block_count;

  while(count > 0) {

    ret = UDFReadBlocksRaw(device, lb_number, count, data, encrypted);

    if(ret <= 0) {
      /* One of the reads failed or nothing more to read, too bad.
       * We won't even bother returning the reads that went ok. */
      return ret;
    }

    count -= (size_t)ret;
    lb_number += (uint32_t)ret;
  }

  return block_count;
}


#ifndef NULL
#define NULL ((void *)0)
#endif

struct Partition {
    int valid;
    char VolumeDesc[128];
    uint16_t Flags;
    uint16_t Number;
    char Contents[32];
    uint32_t AccessType;
    uint32_t Start;
    uint32_t Length;
    uint32_t Forward;  // Filetype 250
};

// libdvdread also uses AD as a way to carry File information back, like
// filesize. Ick. So we have a 32bit-64bit problem. Lund
struct AD {
    uint32_t Location;
    uint32_t Length;
    uint8_t  Flags;
    uint16_t Partition;
};

//
// Lets no longer re-use AD for Files, but create its own structure.
// Lund
// Is there a real max number of chains? What can fit in 2048?
// Here is the problem we need to fix. Currently the UDF API returns the
// start block as the "file handle", and it assumes that all blocks are
// then sequential.
// We need to change this to return a "UDF *" type, (perhaps FileAD *),
// where we can store the chains, and compute the "actual" block-position
// to seek to.
//
// My 4.4GB file example uses 5 chains. A BD ISO can store 50GB of data, so
// potentially we could have a 50GB file, so we would need to be able to
// support 62 chains.
#define UDF_MAX_AD_CHAINS 50

struct FileAD {
    uint64_t Length;
	uint32_t num_AD;
	uint32_t Partition_Start;
	struct AD AD_chain[UDF_MAX_AD_CHAINS];
};



struct extent_ad {
  uint32_t location;
  uint32_t length;
};

struct avdp_t {
  struct extent_ad mvds;
  struct extent_ad rvds;
};

struct pvd_t {
  uint8_t VolumeIdentifier[32];
  uint8_t VolumeSetIdentifier[128];
};

struct lbudf {
  uint32_t lb;
  uint8_t *data;
  /* needed for proper freeing */
  uint8_t *data_base;
};

struct icbmap {
  uint32_t lbn;
  struct FileAD file;
  uint8_t filetype;
};

struct udf_cache {
  int avdp_valid;
  struct avdp_t avdp;
  int pvd_valid;
  struct pvd_t pvd;
  int partition_valid;
  struct Partition partition;
  int rooticb_valid;
  struct AD rooticb;
  int lb_num;
  struct lbudf *lbs;
  int map_num;
  struct icbmap *maps;
};

typedef enum {
  PartitionCache, RootICBCache, LBUDFCache, MapCache, AVDPCache, PVDCache
} UDFCacheType;

void FreeUDFCache(void *cache)
{
  struct udf_cache *c = (struct udf_cache *)cache;
  if(c == NULL) {
    return;
  }
  if(c->lbs) {
    int n;
    for(n = 0; n < c->lb_num; n++)
      free(c->lbs[n].data_base);
    free(c->lbs);
  }
  if(c->maps) {
    free(c->maps);
  }
  free(c);
}



static int GetUDFCache(dvd_reader_t *device, UDFCacheType type,
		       uint32_t nr, void *data)
{
  int n;
  struct udf_cache *c;

  if(DVDUDFCacheLevel(device, -1) <= 0) {
    return 0;
  }

  c = (struct udf_cache *)GetUDFCacheHandle(device);

  if(c == NULL) {
    return 0;
  }

  switch(type) {
  case AVDPCache:
    if(c->avdp_valid) {
      *(struct avdp_t *)data = c->avdp;
      return 1;
    }
    break;
  case PVDCache:
    if(c->pvd_valid) {
      *(struct pvd_t *)data = c->pvd;
      return 1;
    }
    break;
  case PartitionCache:
    if(c->partition_valid) {
      *(struct Partition *)data = c->partition;
      return 1;
    }
    break;
  case RootICBCache:
    if(c->rooticb_valid) {
      *(struct AD *)data = c->rooticb;
      return 1;
    }
    break;
  case LBUDFCache:
    for(n = 0; n < c->lb_num; n++) {
      if(c->lbs[n].lb == nr) {
      *(uint8_t **)data = c->lbs[n].data;
      return 1;
      }
    }
    break;
  case MapCache:
    for(n = 0; n < c->map_num; n++) {
      if(c->maps[n].lbn == nr) {
       *(struct icbmap *)data = c->maps[n];
       return 1;
      }
    }
    break;
  default:
    break;
  }

  return 0;
}

static int SetUDFCache(dvd_reader_t *device, UDFCacheType type,
		       uint32_t nr, void *data)
{
  int n;
  struct udf_cache *c;

  if(DVDUDFCacheLevel(device, -1) <= 0) {
    return 0;
  }

  c = (struct udf_cache *)GetUDFCacheHandle(device);

  if(c == NULL) {
    c = calloc(1, sizeof(struct udf_cache));
    /* fprintf(stderr, "calloc: %d\n", sizeof(struct udf_cache)); */
    if(c == NULL) {
      return 0;
    }
    SetUDFCacheHandle(device, c);
  }


  switch(type) {
  case AVDPCache:
    c->avdp = *(struct avdp_t *)data;
    c->avdp_valid = 1;
    break;
  case PVDCache:
    c->pvd = *(struct pvd_t *)data;
    c->pvd_valid = 1;
    break;
  case PartitionCache:
    c->partition = *(struct Partition *)data;
    c->partition_valid = 1;
    break;
  case RootICBCache:
    c->rooticb = *(struct AD *)data;
    c->rooticb_valid = 1;
    break;
  case LBUDFCache:
    for(n = 0; n < c->lb_num; n++) {
      if(c->lbs[n].lb == nr) {
       /* replace with new data */
       c->lbs[n].data_base = ((uint8_t **)data)[0];
       c->lbs[n].data = ((uint8_t **)data)[1];
       c->lbs[n].lb = nr;
       return 1;
      }
    }
    c->lb_num++;
    c->lbs = realloc(c->lbs, c->lb_num * sizeof(struct lbudf));
    /*
    fprintf(stderr, "realloc lb: %d * %d = %d\n",
	    c->lb_num, sizeof(struct lbudf),
	    c->lb_num * sizeof(struct lbudf));
    */
    if(c->lbs == NULL) {
      c->lb_num = 0;
      return 0;
    }
    c->lbs[n].data_base = ((uint8_t **)data)[0];
    c->lbs[n].data = ((uint8_t **)data)[1];
    c->lbs[n].lb = nr;
    break;
  case MapCache:
    for(n = 0; n < c->map_num; n++) {
      if(c->maps[n].lbn == nr) {
       /* replace with new data */
       c->maps[n] = *(struct icbmap *)data;
       c->maps[n].lbn = nr;
       return 1;
      }
    }
    c->map_num++;
    c->maps = realloc(c->maps, c->map_num * sizeof(struct icbmap));
    /*
    fprintf(stderr, "realloc maps: %d * %d = %d\n",
	    c->map_num, sizeof(struct icbmap),
	    c->map_num * sizeof(struct icbmap));
    */
    if(c->maps == NULL) {
      c->map_num = 0;
      return 0;
    }
    c->maps[n] = *(struct icbmap *)data;
    c->maps[n].lbn = nr;
    break;
  default:
    return 0;
  }

  return 1;
}


/* For direct data access, LSB first */
#define GETN1(p) ((uint8_t)data[p])
#define GETN2(p) ((uint16_t)data[p] | ((uint16_t)data[(p) + 1] << 8))
#define GETN3(p) ((uint32_t)data[p] | ((uint32_t)data[(p) + 1] << 8) \
		  | ((uint32_t)data[(p) + 2] << 16))
#define GETN4(p) ((uint32_t)data[p] \
		  | ((uint32_t)data[(p) + 1] << 8) \
		  | ((uint32_t)data[(p) + 2] << 16) \
		  | ((uint32_t)data[(p) + 3] << 24))
#define GETN8(p) ((uint64_t)data[p]		    \
		  | ((uint64_t)data[(p) + 1] << 8)  \
		  | ((uint64_t)data[(p) + 2] << 16) \
		  | ((uint64_t)data[(p) + 3] << 24) \
		  | ((uint64_t)data[(p) + 4] << 32) \
		  | ((uint64_t)data[(p) + 5] << 40) \
		  | ((uint64_t)data[(p) + 6] << 48) \
		  | ((uint64_t)data[(p) + 7] << 56))
/* This is wrong with regard to endianess */
#define GETN(p, n, target) memcpy(target, &data[p], n)

static int Unicodedecode( uint8_t *data, int len, char *target )
{
    int p = 1, i = 0;

    if( ( data[ 0 ] == 8 ) || ( data[ 0 ] == 16 ) ) do {
        if( data[ 0 ] == 16 ) p++;  /* Ignore MSB of unicode16 */
        if( p < len ) {
            target[ i++ ] = data[ p++ ];
        }
    } while( p < len );

    target[ i ] = '\0';
    return 0;
}

static int UDFDescriptor( uint8_t *data, uint16_t *TagID )
{
    *TagID = GETN2(0);
    /* TODO: check CRC 'n stuff */
	//fprintf(stderr, "UDFDescriptor(%d)\n", *TagID);
    return 0;
}

static int UDFExtentAD( uint8_t *data, uint32_t *Length, uint32_t *Location )
{
    *Length   = GETN4(0);
    *Location = GETN4(4);
    return 0;
}

static int UDFShortAD( uint8_t *data, struct AD *ad,
		       struct Partition *partition )
{
    ad->Length = GETN4(0);
    ad->Flags = ad->Length >> 30;
    ad->Length &= 0x3FFFFFFF;
    ad->Location = GETN4(4);
    ad->Partition = partition->Number; /* use number of current partition */
    return 0;
}

static int UDFLongAD( uint8_t *data, struct AD *ad )
{
    ad->Length = GETN4(0);
    ad->Flags = ad->Length >> 30;
    ad->Length &= 0x3FFFFFFF;
    ad->Location = GETN4(4);
    ad->Partition = GETN2(8);
    /* GETN(10, 6, Use); */
    return 0;
}

static int UDFExtAD( uint8_t *data, struct AD *ad )
{
    ad->Length = GETN4(0);
    ad->Flags = ad->Length >> 30;
    ad->Length &= 0x3FFFFFFF;
    ad->Location = GETN4(12);
    ad->Partition = GETN2(16);
    /* GETN(10, 6, Use); */
    return 0;
}

static int UDFICB( uint8_t *data, uint8_t *FileType, uint16_t *Flags )
{
    *FileType = GETN1(11);
    *Flags = GETN2(18);
    return 0;
}


static int UDFPartition( uint8_t *data, uint16_t *Flags, uint16_t *Number,
			 char *Contents, uint32_t *Start, uint32_t *Length )
{
    *Flags = GETN2(20);
    *Number = GETN2(22);
    GETN(24, 32, Contents);
    *Start = GETN4(188);
    *Length = GETN4(192);
#ifdef DEBUG
	fprintf(stderr, "UDFPartiton found Number %d, Contents %5.5s, start-end %d-%d\n",
			*Number, Contents, *Start, *Length);
#endif
	return 0;
}

/**
 * Reads the volume descriptor and checks the parameters.  Returns 0 on OK, 1
 * on error.
 */
static int UDFLogVolume( uint8_t *data, char *VolumeDescriptor )
{
    uint32_t lbsize, MT_L, N_PM;
    Unicodedecode(&data[84], 128, VolumeDescriptor);
    lbsize = GETN4(212);  /* should be 2048 */
    MT_L = GETN4(264);    /* should be 6 */
    N_PM = GETN4(268);    /* should be 1 */
#ifdef DEBUG
	fprintf(stderr, "UDFLogVolume: We found %d partitions.\n", N_PM);
#endif
	if (lbsize != DVD_VIDEO_LB_LEN) return 1;
    return 0;
}

static int UDFFileEntry( uint8_t *data, uint8_t *FileType,
			 struct Partition *partition, struct FileAD *fad )
{
    uint16_t flags;
    uint32_t L_EA, L_AD;
    unsigned int p;
	unsigned int curr_ad;

    UDFICB( &data[ 16 ], FileType, &flags );

    /* Init ad for an empty file (i.e. there isn't a AD, L_AD == 0 ) */
    //ad->Length = GETN4( 60 ); /* Really 8 bytes a 56 */
	fad->Length = GETN8(56); // Lund
    //ad->Flags = 0;
    //ad->Location = 0; /* what should we put here?  */
    //ad->Partition = partition->Number; /* use number of current partition */

    L_EA = GETN4( 168 );
    L_AD = GETN4( 172 );
    p = 176 + L_EA;
	curr_ad = 0;
    while( p < 176 + L_EA + L_AD ) {
		struct AD *ad;

		if (curr_ad >= UDF_MAX_AD_CHAINS) return 0;

		ad =  &fad->AD_chain[curr_ad];
		ad->Partition = partition->Number;
		ad->Flags = 0;
		// Increase AD chain ptr
		curr_ad++;

        switch( flags & 0x0007 ) {
            case 0:
				UDFShortAD( &data[ p ], ad, partition );
				p += 8;
				break;
            case 1:
				UDFLongAD( &data[ p ], ad );
				p += 16;
				break;
            case 2:
				UDFExtAD( &data[ p ], ad );
				p += 20;
				break;
            case 3:
                switch( L_AD ) {
				case 8:
					UDFShortAD( &data[ p ], ad, partition );
					break;
				case 16:
					UDFLongAD( &data[ p ],  ad );
					break;
				case 20:
					UDFExtAD( &data[ p ], ad );
					break;
                }
                p += L_AD;
                break;
		default:
			p += L_AD;
			break;
        }
    }
    return 0;
}

// TagID 266
// The differences in ExtFile are indicated below:
// struct extfile_entry {
//         struct desc_tag         tag;
//         struct icb_tag          icbtag;
//         uint32_t                uid;
//         uint32_t                gid;
//         uint32_t                perm;
//         uint16_t                link_cnt;
//         uint8_t                 rec_format;
//         uint8_t                 rec_disp_attr;
//         uint32_t                rec_len;
//         uint64_t                inf_len;
//         uint64_t                obj_size;        // XXXX
//         uint64_t                logblks_rec;
//         struct timestamp        atime;
//         struct timestamp        mtime;
//         struct timestamp        ctime;           // XXXX
//         struct timestamp        attrtime;
//         uint32_t                ckpoint;
//         uint32_t                reserved1;       // XXXX
//         struct long_ad          ex_attr_icb;
//         struct long_ad          streamdir_icb;   // XXXX
//         struct regid            imp_id;
//         uint64_t                unique_id;
//         uint32_t                l_ea;   /* Length of extended attribute area */
//         uint32_t                l_ad;   /* Length of allocation descriptors */
//         uint8_t                 data[1];
// } __packed;
//
// So old "l_ea" is now +216
//
// Lund
//
static int UDFExtFileEntry( uint8_t *data, uint8_t *FileType,
			 struct Partition *partition, struct FileAD *fad )
{
    uint16_t flags;
    uint32_t L_EA, L_AD;
    unsigned int p;
	unsigned int curr_ad;

    UDFICB( &data[ 16 ], FileType, &flags );

    /* Init ad for an empty file (i.e. there isn't a AD, L_AD == 0 ) */
	fad->Length = GETN8(56); // Lund
    //ad->Flags = 0;

    L_EA = GETN4( 208);
    L_AD = GETN4( 212);
    p = 216 + L_EA;
	curr_ad = 0;
    while( p < 216 + L_EA + L_AD ) {
		struct AD *ad;

		if (curr_ad >= UDF_MAX_AD_CHAINS) return 0;

#ifdef DEBUG
		fprintf(stderr, "libdvdread: reading AD chain %d\r\n", curr_ad);
#endif

		ad =  &fad->AD_chain[curr_ad];
		ad->Partition = partition->Number;
		ad->Flags = 0;
		// Increase AD chain ptr
		curr_ad++;

        switch( flags & 0x0007 ) {
            case 0:
				UDFShortAD( &data[ p ], ad, partition );
				p += 8;
				break;
            case 1:
				UDFLongAD( &data[ p ], ad );
				p += 16;
				break;
            case 2:
				UDFExtAD( &data[ p ], ad );
				p += 20;
				break;
            case 3:
                switch( L_AD ) {
				case 8:
					UDFShortAD( &data[ p ], ad, partition );
					break;
				case 16:
					UDFLongAD( &data[ p ],  ad );
					break;
				case 20:
					UDFExtAD( &data[ p ], ad );
					break;
                }
                p += L_AD;
                break;
		default:
			p += L_AD;
			break;
        }
    }
    return 0;
}


static int UDFFileIdentifier( uint8_t *data, uint8_t *FileCharacteristics,
			      char *FileName, struct AD *FileICB )
{
    uint8_t L_FI;
    uint16_t L_IU;

    *FileCharacteristics = GETN1(18);
    L_FI = GETN1(19);
    UDFLongAD(&data[20], FileICB);
    L_IU = GETN2(36);
    if (L_FI) Unicodedecode(&data[38 + L_IU], L_FI, FileName);
    else FileName[0] = '\0';
    return 4 * ((38 + L_FI + L_IU + 3) / 4);
}

/**
 * Maps ICB to FileAD
 * ICB: Location of ICB of directory to scan
 * FileType: Type of the file
 * File: Location of file the ICB is pointing to
 * return 1 on success, 0 on error;
 */
static int UDFMapICB( dvd_reader_t *device, struct AD ICB, uint8_t *FileType,
					  struct Partition *partition, struct FileAD *File )
{
    uint8_t LogBlock_base[DVD_VIDEO_LB_LEN + 2048];
    uint8_t *LogBlock = (uint8_t *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
    uint32_t lbnum;
    uint16_t TagID;
    struct icbmap tmpmap;

    lbnum = partition->Start + ICB.Location + partition->Forward;

    tmpmap.lbn = lbnum;
    if(GetUDFCache(device, MapCache, lbnum, &tmpmap)) {
		*FileType = tmpmap.filetype;
		*File = tmpmap.file;
		return 1;
    }

    do {
        if( DVDReadLBUDF( device, lbnum++, 1, LogBlock, 0 ) <= 0 ) {
            TagID = 0;
        } else {
            UDFDescriptor( LogBlock, &TagID );
        }

#ifdef DEBUG
		fprintf(stderr,
				"libdvdread: reading block %d TagID %d\r\n",
				lbnum, TagID);
#endif

        if( TagID == 261 ) {
            UDFFileEntry( LogBlock, FileType, partition, File );
			tmpmap.file = *File;
			tmpmap.filetype = *FileType;
			SetUDFCache(device, MapCache, tmpmap.lbn, &tmpmap);
            return 1;
        };

        if( TagID == 266 ) {
            UDFExtFileEntry( LogBlock, FileType, partition, File );
			tmpmap.file = *File;
			tmpmap.filetype = *FileType;
			SetUDFCache(device, MapCache, tmpmap.lbn, &tmpmap);
            return 1;
        }

	} while( ( lbnum <= partition->Start + ICB.Location + ( ICB.Length - 1 )
			   / DVD_VIDEO_LB_LEN ) && ( TagID != 261 ) && (TagID != 266) );

    return 0;
}

/**
 * Dir: Location of directory to scan
 * FileName: Name of file to look for
 * FileICB: Location of ICB of the found file
 * return 1 on success, 0 on error;
 */
static int UDFScanDir( dvd_reader_t *device, struct FileAD Dir, char *FileName,
                       struct Partition *partition, struct AD *FileICB,
		       int cache_file_info)
{
    char filename[ MAX_UDF_FILE_NAME_LEN ];
    uint8_t directory_base[ 2 * DVD_VIDEO_LB_LEN + 2048];
    uint8_t *directory = (uint8_t *)(((uintptr_t)directory_base & ~((uintptr_t)2047)) + 2048);
    uint32_t lbnum;
    uint16_t TagID;
    uint8_t filechar;
    unsigned int p;
    uint8_t *cached_dir_base = NULL, *cached_dir;
    uint32_t dir_lba;
    struct AD tmpICB;
    int found = 0;
    int in_cache = 0;

    /* Scan dir for ICB of file */
    //lbnum = partition->Start + Dir.AD_chain[0].Location;
    lbnum = partition->Start + Dir.AD_chain[0].Location + partition->Forward;

    if(DVDUDFCacheLevel(device, -1) > 0) {
      /* caching */

      if(!GetUDFCache(device, LBUDFCache, lbnum, &cached_dir)) {
	dir_lba = (Dir.AD_chain[0].Length + DVD_VIDEO_LB_LEN) / DVD_VIDEO_LB_LEN;
	if((cached_dir_base = malloc(dir_lba * DVD_VIDEO_LB_LEN + 2048)) == NULL) {
	  return 0;
	}
	cached_dir = (uint8_t *)(((uintptr_t)cached_dir_base & ~((uintptr_t)2047)) + 2048);
	if( DVDReadLBUDF( device, lbnum, dir_lba, cached_dir, 0) <= 0 ) {
	  free(cached_dir_base);
	  cached_dir = NULL;
	}
	/*
	if(cached_dir) {
	  fprintf(stderr, "malloc dir: %d\n",
		  dir_lba * DVD_VIDEO_LB_LEN);
	}
	*/
	{
	  uint8_t *data[2];
	  data[0] = cached_dir_base;
	  data[1] = cached_dir;
	  SetUDFCache(device, LBUDFCache, lbnum, data);
	}
      } else {
	in_cache = 1;
      }

      if(cached_dir == NULL) {
	return 0;
      }

      p = 0;

      while( p < Dir.AD_chain[0].Length ) {
		  UDFDescriptor( &cached_dir[ p ], &TagID );
		  if( TagID == 257 ) {
			  p += UDFFileIdentifier( &cached_dir[ p ], &filechar,
									  filename, &tmpICB );
			  if(cache_file_info && !in_cache) {
				  uint8_t tmpFiletype;
				  struct FileAD tmpFile;

				  memset(&tmpFile, 0, sizeof(tmpFile));

				  //fprintf(stderr, "libdvdread: UDFScanDir(%s)\r\n", filename);
				  if( !strcasecmp( FileName, filename ) ) {
					  *FileICB = tmpICB;
					  found = 1;

				  }
				  UDFMapICB(device, tmpICB, &tmpFiletype,
							partition, &tmpFile);
			  } else {
				  //fprintf(stderr, "libdvdread: UDFScanDir(%s)\r\n", filename);
				  if( !strcasecmp( FileName, filename ) ) {
					  *FileICB = tmpICB;
					  return 1;
				  }
			  }
		  } else {
			  if(cache_file_info && (!in_cache) && found) {
				  return 1;
			  }
			  return 0;
		  }
      }
      if(cache_file_info && (!in_cache) && found) {
		  return 1;
      }
      return 0;
    }

    if( DVDReadLBUDF( device, lbnum, 2, directory, 0 ) <= 0 ) {
        return 0;
    }

    p = 0;
    while( p < Dir.AD_chain[0].Length ) {
        if( p > DVD_VIDEO_LB_LEN ) {
            ++lbnum;
            p -= DVD_VIDEO_LB_LEN;
            Dir.AD_chain[0].Length -= DVD_VIDEO_LB_LEN;
            if( DVDReadLBUDF( device, lbnum, 2, directory, 0 ) <= 0 ) {
                return 0;
            }
        }
        UDFDescriptor( &directory[ p ], &TagID );
        if( TagID == 257 ) {
            p += UDFFileIdentifier( &directory[ p ], &filechar,
                                    filename, FileICB );
			//fprintf(stderr, "libdvdread: UDFScanDir(%s)\r\n", filename);
            if( !strcasecmp( FileName, filename ) ) {
                return 1;
            }
        } else {
            return 0;
        }
    }

    return 0;
}



/**
 * Low-level function used by DVDReadDir to simulate readdir().
 * Dir: Location of directory to scan
 * FileName: Name of file to look for
 * FileICB: Location of ICB of the found file
 * return 1 on success, 0 on error;
 */
int UDFScanDirX( dvd_reader_t *device,
				 dvd_dir_t *dirp )
{
    char filename[ MAX_UDF_FILE_NAME_LEN ];
    uint8_t directory_base[ 2 * DVD_VIDEO_LB_LEN + 2048];
    uint8_t *directory = (uint8_t *)(((uintptr_t)directory_base & ~((uintptr_t)2047)) + 2048);
    uint32_t lbnum;
    uint16_t TagID;
    uint8_t filechar;
    unsigned int p;
	struct AD FileICB;
	struct FileAD File;
    struct Partition partition;
    uint8_t filetype;

#ifdef DEBUG
    fprintf(stderr, "libdvdread: ScanDirX (current %u: len %u)\r\n",
            dirp->dir_current, dirp->dir_length);
#endif


    //partition.Forward = 0;

    if(!(GetUDFCache(device, PartitionCache, 0, &partition)))
		return 0;

    /* Scan dir for ICB of file */
    //lbnum = partition->Start + Dir.Location;
	lbnum = dirp->dir_current;


	// I have cut out the caching part of the original UDFScanDir() function.
	// one day we should probably bring it back.
	memset(&File, 0, sizeof(File));
	// Not cached...

    if( DVDReadLBUDF( device, lbnum + partition.Forward, 2, directory, 0 ) <= 0 ) {
        return 0;
    }


    //p = 0;
    p = dirp->current_p;

    //while( p < Dir.Length ) {
    while( p < dirp->dir_length ) {
        if( p > DVD_VIDEO_LB_LEN ) {
            ++lbnum;
            p -= DVD_VIDEO_LB_LEN;

            //Dir.Length -= DVD_VIDEO_LB_LEN;
			if (dirp->dir_length >= DVD_VIDEO_LB_LEN)
				dirp->dir_length -= DVD_VIDEO_LB_LEN;
			else
				dirp->dir_length = 0;

            if( DVDReadLBUDF( device, lbnum, 2, directory, 0 ) <= 0 ) {
                return 0;
            }
        }
        UDFDescriptor( &directory[ p ], &TagID );

        if( TagID == 257 ) {

            p += UDFFileIdentifier( &directory[ p ], &filechar,
                                    filename, &FileICB );
			//fprintf(stderr, "libdvdread: UDFScanDir(%s)\r\n", filename);
            //if( !strcasecmp( FileName, filename ) ) {
            //    return 1;
            //}

			// We have a valid entry, return it.
#ifdef DEBUG
			fprintf(stderr, "libdvdread: readdir block %d p %d '%s'\r\n",
					lbnum, p, filename);
#endif
			dirp->current_p = p;
			dirp->dir_current = lbnum;

			if (!*filename)  // No filename, simulate "." dirname
				strcpy(dirp->entry.d_name, ".");
			else {
				// Bah, MSVC don't have strlcpy()
				strncpy(dirp->entry.d_name, filename,
						sizeof(dirp->entry.d_name)-1);
				dirp->entry.d_name[ sizeof(dirp->entry.d_name) -1 ] = 0;
			}

			// Look up the Filedata
			if( !UDFMapICB( device, FileICB, &filetype, &partition, &File))
				return 0;
			if (filetype == 4)
				dirp->entry.d_type = DVD_DT_DIR;
			else
				dirp->entry.d_type = DVD_DT_REG; // Add more types?

			dirp->entry.d_filesize = File.Length;

			return 1;

        } else {
			// Not TagID 257
            return 0;
        }
    }
	// End of DIR
	// As it turns out, UDFScanDir() flips out if it has cache enabled, but
	// not everything is in the cache.
    return 0;
}


static int UDFGetAVDP( dvd_reader_t *device,
		       struct avdp_t *avdp)
{
  uint8_t Anchor_base[ DVD_VIDEO_LB_LEN + 2048 ];
  uint8_t *Anchor = (uint8_t *)(((uintptr_t)Anchor_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum, MVDS_location, MVDS_length;
  uint16_t TagID;
  uint32_t lastsector;
  int terminate;
  struct avdp_t;

  if(GetUDFCache(device, AVDPCache, 0, avdp)) {
    return 1;
  }

  /*
    + At sector 256 (100h)
    + At the final sector of the disk
    + 256 blocks before the final sector of the disk
  */
  /* Find Anchor */
  lastsector = 0;
  lbnum = 256;   /* Try #1, prime anchor */
  terminate = 0;

  for(;;) {
    if( DVDReadLBUDF( device, lbnum, 1, Anchor, 0 ) > 0 ) {
      UDFDescriptor( Anchor, &TagID );
    } else {
      TagID = 0;
    }
    if (TagID != 2) {
      /* Not an anchor */
      if( terminate ) return 0; /* Final try failed */

      if( lastsector ) {

	/* We already found the last sector.  Try #3, alternative
	 * backup anchor.  If that fails, don't try again.
	 */
	lbnum = lastsector - 256;
	terminate = 1;
      } else {
	/* TODO: Find last sector of the disc (this is optional). */
	if( lastsector ) {
	  /* Try #2, backup anchor */
	  lbnum = lastsector - 256;
	} else {

	  /* Unable to find last sector */
        return 0;
	}
      }
    } else {
      /* It's an anchor! We can leave */
      break;
    }
  }

  /* Main volume descriptor */
  UDFExtentAD( &Anchor[ 16 ], &MVDS_length, &MVDS_location );
  avdp->mvds.location = MVDS_location;
  avdp->mvds.length = MVDS_length;

  /* Backup volume descriptor */
  UDFExtentAD( &Anchor[ 24 ], &MVDS_length, &MVDS_location );
  avdp->rvds.location = MVDS_location;
  avdp->rvds.length = MVDS_length;

  SetUDFCache(device, AVDPCache, 0, avdp);

  return 1;
}

/**
 * Looks for partition on the disc.  Returns 1 if partition found, 0 on error.
 *   partnum: Number of the partition, starting at 0.
 *   part: structure to fill with the partition information
 */
static int UDFFindPartition( dvd_reader_t *device, int partnum,
			     struct Partition *part )
{
    uint8_t LogBlock_base[ DVD_VIDEO_LB_LEN + 2048 ];
    uint8_t *LogBlock = (uint8_t *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
    uint32_t lbnum, MVDS_location, MVDS_length;
    uint16_t TagID;
    int i, volvalid;
    struct avdp_t avdp;


    if(!UDFGetAVDP(device, &avdp)) {
      return 0;
    }

    /* Main volume descriptor */
    MVDS_location = avdp.mvds.location;
    MVDS_length = avdp.mvds.length;

    part->valid = 0;
    volvalid = 0;
    part->VolumeDesc[ 0 ] = '\0';
    i = 1;
    do {
        /* Find Volume Descriptor */
        lbnum = MVDS_location;
        do {

            if( DVDReadLBUDF( device, lbnum++, 1, LogBlock, 0 ) <= 0 ) {
                TagID = 0;
            } else {
                UDFDescriptor( LogBlock, &TagID );
            }

            if( ( TagID == 5 ) && ( !part->valid ) ) {
                /* Partition Descriptor */
                UDFPartition( LogBlock, &part->Flags, &part->Number,
                              part->Contents, &part->Start, &part->Length );
                part->valid = ( partnum == part->Number );
            } else if( ( TagID == 6 ) && ( !volvalid ) ) {
                /* Logical Volume Descriptor */
                if( UDFLogVolume( LogBlock, part->VolumeDesc ) ) {
                    /* TODO: sector size wrong! */
                } else {
                    volvalid = 1;
                }
            }

        } while( ( lbnum <= MVDS_location + ( MVDS_length - 1 )
                 / DVD_VIDEO_LB_LEN ) && ( TagID != 8 )
                 && ( ( !part->valid ) || ( !volvalid ) ) );

        if( ( !part->valid) || ( !volvalid ) ) {
	  /* Backup volume descriptor */
	  MVDS_location = avdp.mvds.location;
	  MVDS_length = avdp.mvds.length;
        }
    } while( i-- && ( ( !part->valid ) || ( !volvalid ) ) );

    /* We only care for the partition, not the volume */
    return part->valid;
}



void UDFFreeFile(dvd_reader_t *device, struct FileAD *File)
{

	free(File);

}


//
// The API users will refer to block 0 as start of file and going up
// we need to convert that to actual disk block;
// partition_start + file_start + offset
// but keep in mind that file_start is chained, and not contiguous.
//
// We return "0" as error, since a File can not start at physical block 0
//
// Who made Location be blocks, but Length be bytes? Can bytes be uneven
// blocksize in the middle of a chain?
//
uint32_t UDFFileBlockPos(struct FileAD *File, uint32_t file_block)
{
	uint32_t result, i, offset;

	if (!File) return 0;

	// Look through the chain to see where this block would belong.
	for (i = 0, offset = 0;
		 i < File->num_AD;
		 i++) {

		// Is "file_block" inside this chain? Then use this chain.
		if (file_block < (offset +
						  (File->AD_chain[i].Length /  DVD_VIDEO_LB_LEN)))
			break;

		offset += (File->AD_chain[i].Length /  DVD_VIDEO_LB_LEN);

	}

	if (i >= File->num_AD)
		i = 0; // This was the default behavior before I fixed things.


	result = File->Partition_Start
		+ File->AD_chain[i].Location
		+ file_block
		- offset;


#ifdef DEBUG
	if (!(file_block % 10000))
		fprintf(stderr, "libdvdread: File block %d -> %d (chain %d + offset %d : %d)\r\n", file_block, result,
				i,
				file_block - offset,
				(File->AD_chain[i].Length / DVD_VIDEO_LB_LEN));
#endif
	return result;

}



UDF_FILE UDFFindFile( dvd_reader_t *device, char *filename,
					  uint64_t *filesize )
{
    uint8_t LogBlock_base[ DVD_VIDEO_LB_LEN + 2048 ];
    uint8_t *LogBlock = (uint8_t *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
    uint32_t lbnum;
    uint16_t TagID;
    struct Partition partition;
    struct AD RootICB, ICB;
	struct FileAD File;
    char tokenline[ MAX_UDF_FILE_NAME_LEN ];
    char *token;
    uint8_t filetype;
	struct FileAD *result;

fprintf(stderr,"libdvdread: UDFFindFile(%s)\r\n", filename);
    partition.Forward = 0;
	memset(&File, 0, sizeof(File));

    *filesize = 0;
    tokenline[0] = '\0';
    strcat( tokenline, filename );


    if(!(GetUDFCache(device, PartitionCache, 0, &partition) &&
		 GetUDFCache(device, RootICBCache, 0, &RootICB))) {
		/* Find partition, 0 is the standard location for DVD Video.*/
		if( !UDFFindPartition( device, 0, &partition ) ) return 0;

		SetUDFCache(device, PartitionCache, 0, &partition);

		/* Find root dir ICB */
		lbnum = partition.Start;
		do {
			if( DVDReadLBUDF( device, lbnum++, 1, LogBlock, 0 ) <= 0 ) {
				TagID = 0;
			} else {
				UDFDescriptor( LogBlock, &TagID );
			}


#ifdef DEBUG
			fprintf(stderr,
					"libdvdread: read block %d TagID %d\r\n", lbnum,
					TagID);
#endif


			// It seems that someone added a FileType of 250, which seems to be
			// a "redirect" style file entry. If we discover such an entry, we
			// add on the "location" to partition->Start, and try again.
			// Who added this? Is there any official guide somewhere?
			// 2008/09/17 lundman
			// Should we handle 261(250) as well? FileEntry+redirect
            // 2009/03/01 lundman
            // Actually no, adding to "location" means all the files are
            // also read offset. Only the dirs should be offset.
			if( TagID == 266 ) {
				UDFExtFileEntry( LogBlock, &filetype, &partition, &File );
				if (filetype == 250) {
#ifdef DEBUG
					fprintf(stderr,
							"libdvdread: redirect block filetype 250 found at block %d, for location +%d\r\n",
							lbnum, File.AD_chain[0].Location);
#endif
					//partition.Start += File.AD_chain[0].Location;
					partition.Forward = File.AD_chain[0].Location;
					lbnum = partition.Start + partition.Forward;
					SetUDFCache(device, PartitionCache, 0, &partition);
					continue;
				}
			}

			/* File Set Descriptor */
			if( TagID == 256 ) {  /* File Set Descriptor */
				UDFLongAD( &LogBlock[ 400 ], &RootICB );
			}

		} while( ( lbnum < partition.Start + partition.Forward + partition.Length )
				 && ( TagID != 8 ) && ( TagID != 256 ) );

		/* Sanity checks. */
		if( TagID != 256 ) return NULL;

		// Lund: this test will fail on newer UDF2.50 images.
		//if( RootICB.Partition != 0 ) return 0;
		SetUDFCache(device, RootICBCache, 0, &RootICB);
    }

    /* Find root dir */
    if( !UDFMapICB( device, RootICB, &filetype, &partition, &File ) )
		return NULL;

    if( filetype != 4 ) return NULL;  /* Root dir should be dir */

    {
		int cache_file_info = 0;
		/* Tokenize filepath */
		token = strtok(tokenline, "/");

		while( token != NULL ) {

			if( !UDFScanDir( device, File, token, &partition, &ICB,
							 cache_file_info)) {
				return NULL;
			}
			if( !UDFMapICB( device, ICB, &filetype, &partition, &File ) ) {
				return NULL;
			}

			if(!strcmp(token, "VIDEO_TS")) {
				cache_file_info = 1;
			}
			token = strtok( NULL, "/" );
		}
    }

    /* Sanity check. */
    if( File.AD_chain[0].Partition != 0 ) return NULL;

    *filesize = File.Length;
    /* Hack to not return partition.Start for empty files. */
    if( !File.AD_chain[0].Location )
		return NULL;

	// Allocate a UDF_FILE node for the API user
	result = (struct FileAD *) malloc(sizeof(*result));
	if (!result) return NULL;

	// Copy over the information
	memcpy(result, &File, sizeof(*result));

	result->Partition_Start = partition.Start;

	return result;
	//return partition.Start + File.AD_chain[0].Location;
}



/**
 * Gets a Descriptor .
 * Returns 1 if descriptor found, 0 on error.
 * id, tagid of descriptor
 * bufsize, size of BlockBuf (must be >= DVD_VIDEO_LB_LEN).
 */
static int UDFGetDescriptor( dvd_reader_t *device, int id,
			     uint8_t *descriptor, int bufsize)
{
  uint32_t lbnum, MVDS_location, MVDS_length;
  struct avdp_t avdp;
  uint16_t TagID;
  uint32_t lastsector;
  int i, terminate;
  int desc_found = 0;
  /* Find Anchor */
  lastsector = 0;
  lbnum = 256;   /* Try #1, prime anchor */
  terminate = 0;
  if(bufsize < DVD_VIDEO_LB_LEN) {
    return 0;
  }

  if(!UDFGetAVDP(device, &avdp)) {
    return 0;
  }

  /* Main volume descriptor */
  MVDS_location = avdp.mvds.location;
  MVDS_length = avdp.mvds.length;

  i = 1;
  do {
    /* Find  Descriptor */
    lbnum = MVDS_location;
    do {

      if( DVDReadLBUDF( device, lbnum++, 1, descriptor, 0 ) <= 0 ) {
	TagID = 0;
      } else {
	UDFDescriptor( descriptor, &TagID );
      }

      if( (TagID == id) && ( !desc_found ) ) {
	/* Descriptor */
	desc_found = 1;
      }
    } while( ( lbnum <= MVDS_location + ( MVDS_length - 1 )
	       / DVD_VIDEO_LB_LEN ) && ( TagID != 8 )
	     && ( !desc_found) );

    if( !desc_found ) {
      /* Backup volume descriptor */
      MVDS_location = avdp.rvds.location;
      MVDS_length = avdp.rvds.length;
    }
  } while( i-- && ( !desc_found )  );

  return desc_found;
}


static int UDFGetPVD(dvd_reader_t *device, struct pvd_t *pvd)
{
  uint8_t pvd_buf_base[DVD_VIDEO_LB_LEN + 2048];
  uint8_t *pvd_buf = (uint8_t *)(((uintptr_t)pvd_buf_base & ~((uintptr_t)2047)) + 2048);

  if(GetUDFCache(device, PVDCache, 0, pvd)) {
    return 1;
  }

  if(!UDFGetDescriptor( device, 1, pvd_buf, sizeof(pvd_buf))) {
    return 0;
  }

  memcpy(pvd->VolumeIdentifier, &pvd_buf[24], 32);
  memcpy(pvd->VolumeSetIdentifier, &pvd_buf[72], 128);
  SetUDFCache(device, PVDCache, 0, pvd);

  return 1;
}

/**
 * Gets the Volume Identifier string, in 8bit unicode (latin-1)
 * volid, place to put the string
 * volid_size, size of the buffer volid points to
 * returns the size of buffer needed for all data
 */
int UDFGetVolumeIdentifier(dvd_reader_t *device, char *volid,
			   unsigned int volid_size)
{
  struct pvd_t pvd;
  unsigned int volid_len;

  /* get primary volume descriptor */
  if(!UDFGetPVD(device, &pvd)) {
    return 0;
  }

  volid_len = pvd.VolumeIdentifier[31];
  if(volid_len > 31) {
    /* this field is only 32 bytes something is wrong */
    volid_len = 31;
  }
  if(volid_size > volid_len) {
    volid_size = volid_len;
  }
  Unicodedecode(pvd.VolumeIdentifier, volid_size, volid);

  return volid_len;
}

/**
 * Gets the Volume Set Identifier, as a 128-byte dstring (not decoded)
 * WARNING This is not a null terminated string
 * volsetid, place to put the data
 * volsetid_size, size of the buffer volsetid points to
 * the buffer should be >=128 bytes to store the whole volumesetidentifier
 * returns the size of the available volsetid information (128)
 * or 0 on error
 */
int UDFGetVolumeSetIdentifier(dvd_reader_t *device, uint8_t *volsetid,
			      unsigned int volsetid_size)
{
  struct pvd_t pvd;

  /* get primary volume descriptor */
  if(!UDFGetPVD(device, &pvd)) {
    return 0;
  }


  if(volsetid_size > 128) {
    volsetid_size = 128;
  }

  memcpy(volsetid, pvd.VolumeSetIdentifier, volsetid_size);

  return 128;
}

