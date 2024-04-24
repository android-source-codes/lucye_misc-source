/*
 * freeblk.c - List the control structures of a second
 *			  extended filesystem
 *
 */

/*
 * History:
 * 14/03/19	- Creation
 */

#include "config.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
extern char *optarg;
extern int optind;
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ext2fs/ext2fs.h"
#include "e2p/e2p.h"
#include "jfs_user.h"
#include <uuid/uuid.h>
#include "freeblk.h"

#include "../version.h"
#include "nls-enable.h"

#define in_use(m, x)	(ext2fs_test_bit ((x), (m)))

const char * program_name = "freeblk";
char device_name[256];
int hex_format = 0;
int blocks64 = 0;
int print_list=0;
int print_debug=0;

int wipe_block_device(int fd, unsigned long long start, unsigned long long size)
{
    unsigned long long range[2];
    int ret;

// READ ME!!
// We remove Secure Discard option because we don't use below eMMC_4_5 anymore.
// If you want to use secure discard option, fix here.

    range[0] = start;
    range[1] = size;
    ret = ioctl(fd, BLKDISCARD, &range);
    if (ret < 0) {
        printf("Discard failed %d(%s)\n", errno, strerror(errno));
        return 1;
    } else {
        return 0;
    }

    return 0;
}

static void usage(void)
{
    fprintf (stderr, _("Usage: %s [-lpq] device\n"), program_name);
    fprintf (stderr, _("Usage: -l for list only\n"));
    fprintf (stderr, _("Usage: -p for print debug\n"));
    fprintf (stderr, _("Usage: -q for quiet\n"));
    exit (1);
}

static void free_block(ext2_filsys fs, unsigned long group, char * bitmap,
        unsigned long num, unsigned long offset, int ratio)
{
    int p = 0;
    unsigned long i;
    unsigned long j;
    int fd;
    unsigned long long start, end;

    fd = open(device_name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
    if (fd < 0) {
        printf("open error\n");
        return;
    }

    offset /= ratio;
    offset += group * num;
    for (i = 0; i < num; i++)
        if (!in_use (bitmap, i))
        {
            start = end = (i + offset) * ratio;
            for (j = i; j < num && !in_use (bitmap, j); j++)
                ;
            if (--j != i) {
                end = (j + offset) * ratio;
                i = j;
            }

            if ( print_list ) {
                printf("G%03ld Start=%10lld Size=%08lld\n", group, start*fs->blocksize, (end-start+1)*fs->blocksize);
            } else {
                if ( print_debug ) {
                    printf("G%03ld Start=%10lld Size=%08lld\n", group, start*fs->blocksize, (end-start+1)*fs->blocksize);
                }
                wipe_block_device( fd, start*fs->blocksize , (end-start+1)*fs->blocksize );
            }
            p = 1;
        }

    close(fd);

}

static void list_free_blocks (ext2_filsys fs)
{
    unsigned long i;
    blk64_t	first_block, last_block;
    blk64_t	super_blk, old_desc_blk, new_desc_blk;
    char *block_bitmap=NULL, *inode_bitmap=NULL;
    int inode_blocks_per_group, old_desc_blocks, reserved_gdt;
    int	block_nbytes, inode_nbytes;
    blk64_t		blk_itr = EXT2FS_B2C(fs, fs->super->s_first_data_block);

    block_nbytes = EXT2_CLUSTERS_PER_GROUP(fs->super) / 8;
    inode_nbytes = EXT2_INODES_PER_GROUP(fs->super) / 8;

    if (fs->block_map)
        block_bitmap = malloc(block_nbytes);
    if (fs->inode_map)
        inode_bitmap = malloc(inode_nbytes);

    inode_blocks_per_group = ((fs->super->s_inodes_per_group *
                EXT2_INODE_SIZE(fs->super)) +
            EXT2_BLOCK_SIZE(fs->super) - 1) /
        EXT2_BLOCK_SIZE(fs->super);
    reserved_gdt = fs->super->s_reserved_gdt_blocks;
    first_block = fs->super->s_first_data_block;
    if (fs->super->s_feature_incompat & EXT2_FEATURE_INCOMPAT_META_BG)
        old_desc_blocks = fs->super->s_first_meta_bg;
    else
        old_desc_blocks = fs->desc_blocks;
    for (i = 0; i < fs->group_desc_count; i++) {
        first_block = ext2fs_group_first_block2(fs, i);
        last_block = ext2fs_group_last_block2(fs, i);

        ext2fs_super_and_bgd_loc2(fs, i, &super_blk,
                &old_desc_blk, &new_desc_blk, 0);

        if (block_bitmap) {
            ext2fs_get_block_bitmap_range2(fs->block_map,
                    blk_itr, block_nbytes << 3, block_bitmap);
            free_block(fs, i, block_bitmap,
                    fs->super->s_clusters_per_group,
                    fs->super->s_first_data_block,
                    EXT2FS_CLUSTER_RATIO(fs));
            blk_itr += fs->super->s_clusters_per_group;
        }
    }
    if (block_bitmap)
        free(block_bitmap);
    if (inode_bitmap)
        free(inode_bitmap);
}

int main (int argc, char ** argv)
{
    errcode_t	retval;
    ext2_filsys	fs;
    int		print_badblocks = 0;
    blk64_t		use_superblock = 0;
    int		use_blocksize = 0;
    int		flags;
    int     c;

#ifdef ENABLE_NLS
    setlocale(LC_MESSAGES, "");
    setlocale(LC_CTYPE, "");
    bindtextdomain(NLS_CAT_NAME, LOCALEDIR);
    textdomain(NLS_CAT_NAME);
    set_com_err_gettext(gettext);
#endif
    add_error_table(&et_ext2_error_table);
    if (argc && *argv)
        program_name = *argv;

    while ((c = getopt (argc, argv, "lpq")) != EOF) {
        switch (c) {
            case 'l':
                print_list=1;
                break;
            case 'p':
                print_debug=1;
                break;
            case 'q':
                print_debug=0;
                print_list=0;
                break;
            default:
                usage();
        }
    }

    if (optind > argc - 1)
        usage();
    strcpy( device_name, argv[optind++] );
    flags = EXT2_FLAG_JOURNAL_DEV_OK | EXT2_FLAG_SOFTSUPP_FEATURES | EXT2_FLAG_64BITS;

    retval = ext2fs_open (device_name, flags, use_superblock,
            use_blocksize, unix_io_manager, &fs);
    if (retval) {
        com_err (program_name, retval, _("while trying to open %s"),
                device_name);
        printf (_("Couldn't find valid filesystem superblock.\n"));
        exit (1);
    }
    fs->default_bitmap_type = EXT2FS_BMAP64_RBTREE;
    if (fs->super->s_feature_incompat & EXT4_FEATURE_INCOMPAT_64BIT)
        blocks64 = 1;
    if (print_badblocks) {
        printf("Find bad blcoks\n");
        return -1;
    } else {
        if (fs->super->s_feature_incompat &
                EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) {
            ext2fs_close(fs);
            exit(0);
        }
        retval = ext2fs_read_bitmaps (fs);
        list_free_blocks (fs);
        if (retval) {
            printf(_("\n%s: %s: error reading bitmaps: %s\n"),
                    program_name, device_name,
                    error_message(retval));
        }
    }
    ext2fs_close (fs);
    remove_error_table(&et_ext2_error_table);
    exit (0);
}
