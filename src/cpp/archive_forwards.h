#ifndef ARCHIVE_FORWARDS_H
#define ARCHIVE_FORWARDS_H


#ifdef __cplusplus
extern "C" {
#endif
    
/*
 * Minimal forward declares that qgrep needs from libarchive to ease
 * MSVC port
 */

#include <sys/types.h>  /* Linux requires this for off_t */
#include <inttypes.h> /* For int64_t */
#include <stdio.h> /* For FILE * */
#ifndef _WIN32
#include <unistd.h>  /* For ssize_t and size_t */
#else
#include "libarchive-nonposix.h"
#endif

#if (defined __WIN32__) || (defined _WIN32)
# ifdef BUILD_LIBARCHIVE_DLL
#  define LIBARCHIVE_DLL_IMPEXP    __DLL_EXPORT__
# elif defined(LIBARCHIVE_STATIC)
#  define LIBARCHIVE_DLL_IMPEXP     
# elif defined (USE_LIBARCHIVE_DLL)
#  define LIBARCHIVE_DLL_IMPEXP    __DLL_IMPORT__
# elif defined (USE_LIBARCHIVE_STATIC)
#  define LIBARCHIVE_DLL_IMPEXP     
# else /* assume USE_LIBARCHIVE_DLL */
#  define LIBARCHIVE_DLL_IMPEXP    __DLL_IMPORT__
# endif
#else /* _WIN32 */
# define LIBARCHIVE_DLL_IMPEXP
#endif /* _WIN32 */      

/*
 * Error codes: Use archive_errno() and archive_error_string()
 * to retrieve details.  Unless specified otherwise, all functions
 * that return 'int' use these codes.
 */
#define	ARCHIVE_EOF	  1	/* Found end of archive. */
#define	ARCHIVE_OK	  0	/* Operation was successful. */
#define	ARCHIVE_RETRY	(-10)	/* Retry might succeed. */
#define	ARCHIVE_WARN	(-20)	/* Partial success. */
/* For example, if write_header "fails", then you can't push data. */
#define	ARCHIVE_FAILED	(-25)	/* Current operation cannot complete. */
#define	ARCHIVE_FATAL	(-30)	/* No more operations are possible. */


// from archive.h
struct archive;
struct archive_entry;
LIBARCHIVE_DLL_IMPEXP ssize_t		 archive_read_data(struct archive *, void *, size_t);
LIBARCHIVE_DLL_IMPEXP struct archive	*archive_read_new(void);
LIBARCHIVE_DLL_IMPEXP int		 archive_read_support_compression_all(struct archive *);
LIBARCHIVE_DLL_IMPEXP int		 archive_read_support_format_all(struct archive *);
LIBARCHIVE_DLL_IMPEXP int		 archive_read_open_filename(struct archive *,const char *_filename, size_t _block_size);
LIBARCHIVE_DLL_IMPEXP const char	*archive_error_string(struct archive *);
LIBARCHIVE_DLL_IMPEXP int		 archive_read_next_header(struct archive *, struct archive_entry **);
LIBARCHIVE_DLL_IMPEXP const char	*archive_error_string(struct archive *);
LIBARCHIVE_DLL_IMPEXP int		 archive_read_data_skip(struct archive *);
LIBARCHIVE_DLL_IMPEXP int		 archive_read_finish(struct archive *);
LIBARCHIVE_DLL_IMPEXP struct archive	*archive_write_new(void);
LIBARCHIVE_DLL_IMPEXP int		 archive_write_set_compression_gzip(struct archive *);
LIBARCHIVE_DLL_IMPEXP int		 archive_write_set_format_pax_restricted(struct archive *);
LIBARCHIVE_DLL_IMPEXP int		 archive_write_open_file(struct archive *, const char *_file);
LIBARCHIVE_DLL_IMPEXP int		 archive_write_header(struct archive *, struct archive_entry *);
LIBARCHIVE_DLL_IMPEXP ssize_t		 archive_write_data(struct archive *, const void *, size_t);
LIBARCHIVE_DLL_IMPEXP int		 archive_write_close(struct archive *);
LIBARCHIVE_DLL_IMPEXP int		 archive_write_finish(struct archive *);


// from archive_entry.h
#define	AE_IFREG	0100000
LIBARCHIVE_DLL_IMPEXP const char		*archive_entry_pathname(struct archive_entry *);
LIBARCHIVE_DLL_IMPEXP struct archive_entry	*archive_entry_new(void);
LIBARCHIVE_DLL_IMPEXP void	archive_entry_set_pathname(struct archive_entry *, const char *);
LIBARCHIVE_DLL_IMPEXP void	archive_entry_set_size(struct archive_entry *, int64_t);
LIBARCHIVE_DLL_IMPEXP void	archive_entry_set_filetype(struct archive_entry *, unsigned int);
LIBARCHIVE_DLL_IMPEXP void	archive_entry_set_perm(struct archive_entry *, mode_t);
LIBARCHIVE_DLL_IMPEXP void			 archive_entry_free(struct archive_entry *);

    
#ifdef __cplusplus
}
#endif

#endif
