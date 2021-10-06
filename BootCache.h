
/*
 * Files.
 */
#define BC_BOOT_PLAYLIST	"/var/db/BootCache.playlist"
#define BC_BOOT_FLAGFILE	"/var/db/BootCache.flag"
#define BC_BOOT_STATFILE	"/tmp/BootCache.statistics"
#define BC_BOOT_HISTFILE	"/tmp/BootCache.history"
#define BC_BOOT_LOGFILE		"/tmp/BootCache.log"
#define BC_CONTROL_TOOL		"/System/Library/Extensions/BootCache.kext/Contents/Resources/BootCacheControl"

/*
 * Command stucture, passed via sysctl.
 */
struct BC_command {
	/* magic number identifies the structure */
	int	bc_magic;
#define	BC_MAGIC_0	0x10102021	/* old version with DEV_BSIZE blocks */
#define	BC_MAGIC_1	0x10102021	/* all disk addresses in bytes */
#define	BC_MAGIC	0x10102021	/* added flags field to playlist entry */

	/* opcode determines function of this command */
	int	bc_opcode;
#define	BC_OP_START	0x01
#define BC_OP_STOP	0x02
#define BC_OP_HISTORY	0x03
#define BC_OP_STATS	0x04
#define BC_OP_TAG	0x05

	/* optional parameter */
	int	bc_param;
	
	/* user-space data buffer, use varies with opcode */
	void	*bc_data;
	size_t	bc_length;
};

#define BC_SYSCTL	"kern.BootCache"

/*
 * BC_OP_START - start the cache engine with an optional playlist.
 *
 * If bc_data is not NULL, it points to bc_length bytes of
 * BC_playlist_entry structures.
 *
 * bc_param is set to the blocksize for which the playlist has
 * been computed.
 */

/*
 * Playlist entries represent regions of disk to be cached.
 */
struct BC_playlist_entry {
	u_int64_t	pce_offset;	/* block address */
	u_int64_t	pce_length;	/* size */
	u_int32_t	pce_flags;
#define PCE_PREFETCH	(1<<0)
};

/* number of entries we copyin at a time */
#define BC_PLC_CHUNK	512

/* sanity check for the upper bound on number of entries */
#define BC_MAXENTRIES	100000

/*
 * BC_OP_STOP - stop the cache engine.
 *
 * The history list size (in bytes) is returned in bc_length.  If the history
 * list was truncated, bc_length will be 0 and BC_OP_HISTORY must still be
 * called to clear the buffer.
 */

/*
 * BC_OP_HISTORY - copy out history and clear.
 *
 * The kernel's copy of the cache history is copied into the array of
 * BC_history_entry structures at bc_data.  The cache must previously have been
 * stopped before calling this interface out.
 */

struct BC_history_entry {
	u_int64_t	he_offset;	/* data offset on device */
	u_int64_t	he_length;	/* length of data */
	int		he_flags;
#define	BC_HE_MISS	0		/* read was not satisfied from cache */
#define	BC_HE_HIT	1		/* read was satisfied from cache */
#define BC_HE_TAG	2		/* userland-set tag */
#define BC_HE_WRITE	3		/* write-through operation */
};


/*
 * BC_OP_STATS - return statistics.
 */
struct BC_statistics {
	/* device */
	u_int	ss_blocksize;		/* underlying device bloksize */

	/* readahead */
	u_int	ss_initiated_reads;	/* read operations we initiated */
	u_int	ss_read_blocks;		/* number of blocks read */
	u_int	ss_read_errors;		/* read errors encountered */
	u_int	ss_error_discards;	/* blocks discarded due to read errors */
	struct timeval ss_cache_start;	/* time reads & cache started */
	struct timeval ss_pfetch_stop;	/* time prefetch stopped */
	struct timeval ss_read_stop;	/* time reads stopped */
	struct timeval ss_cache_stop;	/* time cache stopped */
	struct timeval ss_wait_time;	/* total time spent blocked for blocks */

	/* inbound strategy calls */
	u_int	ss_strategy_calls;	/* total strategy calls we received */
	u_int	ss_strategy_nonread;	/* strategy calls that were not reads */
	u_int	ss_strategy_bypassed;	/* strategy calls we bypassed */
	u_int	ss_strategy_bypass_active; /* strategy calls we bypassed reading */
	u_int	ss_strategy_blocked;	/* strategy calls that blocked */

	/* extents */
	u_int	ss_total_extents;	/* number of extents in the cache */
	u_int	ss_extent_lookups;	/* number of extents searched for */
	u_int	ss_extent_hits;		/* number of extents matched */
	u_int	ss_hit_blkmissing;	/* cache hits not filled due to missing blocks */

	/* block/page activity */
	u_int	ss_requested_blocks;
	u_int	ss_hit_blocks;		/* number of blocks vacated due to read hits */
	u_int	ss_write_discards;	/* blocks discarded due to overwriting */
	u_int	ss_spurious_blocks;	/* number of blocks not consumed */
	u_int	ss_spurious_pages;	/* number of pages not consumed */

	/* history activity */
	u_int	ss_history_clusters;	/* number of allocated history clusters */

	/* current status */
	u_int	ss_cache_flags;		/* current cache flags */
};


#ifndef KERNEL
/*
 * Support library functions.
 */
extern int	BC_blocksize;
extern int	BC_read_playlist(const char *, struct BC_playlist_entry **, int *);
extern int	BC_write_playlist(const char *, const struct BC_playlist_entry *, int);
extern int	BC_merge_playlists(struct BC_playlist_entry **, int *,
    const struct BC_playlist_entry *, int);
extern void	BC_sort_playlist(struct BC_playlist_entry *, int);
extern int	BC_coalesce_playlist(struct BC_playlist_entry **, int *);
extern int	BC_fetch_statistics(struct BC_statistics **);
extern int	BC_convert_history(const struct BC_history_entry *,
    struct BC_playlist_entry **, int *);
extern int	BC_start(struct BC_playlist_entry *, int);
extern int	BC_stop(struct BC_history_entry **, int *);
extern int	BC_print_statistics(char *, struct BC_statistics *);
extern int	BC_print_history(char *, struct BC_history_entry *, int);
extern int	BC_tag_history(void);
#endif
