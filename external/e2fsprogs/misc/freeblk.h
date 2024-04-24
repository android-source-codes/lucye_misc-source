
#ifndef FREEBLK_H
#define FREEBLK_H

#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#ifndef MMC_RSP_SPI_R1
#define MMC_RSP_SPI_S1  (1 << 7)
#define MMC_RSP_SPI_R1  (MMC_RSP_SPI_S1)
#endif

#ifndef BLKDISCARD
#define BLKDISCARD _IO(0x12,119)
#endif

#ifndef BLKSECDISCARD
#define BLKSECDISCARD _IO(0x12,125)
#endif

#ifndef USE_MINGW /* O_BINARY is windows-specific flag */
#define O_BINARY 0
#endif

struct mmc_ioc_cmd {
    /* Implies direction of data.  true = write, false = read */
    int write_flag;

    /* Application-specific command.  true = precede with CMD55 */
    int is_acmd;

    __u32 opcode;
    __u32 arg;
    __u32 response[4];  /* CMD response */
    unsigned int flags;
    unsigned int blksz;
    unsigned int blocks;

    /*
     * Sleep at least postsleep_min_us useconds, and at most
     * postsleep_max_us useconds *after* issuing command.  Needed for
     * some read commands for which cards have no other way of indicating
     * they're ready for the next command (i.e. there is no equivalent
     * a "busy" indicator for read operations).
     */
    unsigned int postsleep_min_us;
    unsigned int postsleep_max_us;

    /*
     * Override driver-computed timeouts.  Note the difference in units!
     */
    unsigned int data_timeout_ns;
    unsigned int cmd_timeout_ms;

    /*
     * For 64-bit machines, the next member, ``__u64 data_ptr``, wants to
     * be 8-byte aligned.  Make sure this struct is the same size when
     * built for 32-bit.
     */
    __u32 __pad;

    /* DAT buffer */
    __u64 data_ptr;
};

#define mmc_ioc_cmd_set_data(ic, ptr) ic.data_ptr = (__u64)(unsigned long) ptr
#define MMC_IOC_CMD _IOWR(MMC_BLOCK_MAJOR, 0, struct mmc_ioc_cmd)
#define MMC_SEND_EXT_CSD          8
#define MMC_RSP_R1     (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136 (1 << 1)        /* 136 bit response */
#define MMC_RSP_CRC (1 << 2)        /* expect valid crc */
#define MMC_RSP_BUSY    (1 << 3)        /* card may send busy */
#define MMC_RSP_OPCODE  (1 << 4)        /* response contains opcode */
#define MMC_CMD_MASK    (3 << 5)        /* non-SPI command type */
#define MMC_CMD_AC  (0 << 5)
#define MMC_CMD_ADTC    (1 << 5)
#define MMC_CMD_BC  (2 << 5)
#define MMC_CMD_BCR (3 << 5)

#define MMC_BLOCK_MAJOR     179

#endif //FREEBLK_H
