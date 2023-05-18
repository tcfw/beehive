#ifndef _KERNEL_FS_CPIO_H
#define _KERNEL_FS_CPIO_H

#include "stdint.h"

#define CPIO_HEADER_MAGIC (0xC771)

#define CPIO_MODE_TYPE_MASK (0170000)	  //  This masks the file type bits
#define CPIO_MODE_TYPE_SOCKET (0140000)	  //  File type value for sockets
#define CPIO_MODE_TYPE_SYMBOLIC (0120000) //  File type value for symbolic links
#define CPIO_MODE_TYPE_FILE (0100000)	  //  File type value for regular files
#define CPIO_MODE_TYPE_BLOCK (0060000)	  //  File type value for block special devices
#define CPIO_MODE_TYPE_DIR (0040000)	  //  File type value for directories
#define CPIO_MODE_TYPE_CHAR (0020000)	  //  File type value for character special devices
#define CPIO_MODE_TYPE_FIFO (0010000)	  //  File type value for named pipes or FIFOs
#define CPIO_MODE_FLAG_SUID (0004000)	  //  SUID bit
#define CPIO_MODE_FLAG_SGID (0002000)	  //  SGID bit
#define CPIO_MODE_FLAG_STICKY (0001000)	  //  Sticky bit
#define CPIO_MODE_PERM_MASK (0000777)	  //  The lower 9 bits specify read/write/execute permissions

struct cpio_header_t
{
	uint16_t magic;	   // Magic number octal 070707
	uint16_t dev;	   // Device where file resides
	uint16_t ino;	   // I-number of file
	uint16_t mode;	   // File mode
	uint16_t uid;	   // Owner user ID
	uint16_t gid;	   // Owner group ID
	uint16_t nlink;	   // Number of links to file
	uint16_t rdev;	   // Device major/minor for special file
	uint32_t mtime;	   // Modify time of file
	uint16_t namesize; // Length of filename
	uint32_t filesize; // Length of file
};

#endif