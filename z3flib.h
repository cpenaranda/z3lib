/*
 * z3lib (c)2005,2006/GPL,BSD Oskar Schirmer, schirmer@scara.com
 * combined huffman/lz compression library
 * rfc1952 compliant
 */

#ifndef __Z3FLIB_H__
#define __Z3FLIB_H__

/* private stuff: */

#define Z3F_FLG_FHCRC    (1 << 1)
#define Z3F_FLG_FEXTRA   (1 << 2)
#define Z3F_FLG_FNAME    (1 << 3)
#define Z3F_FLG_FCOMMENT (1 << 4)

#ifndef Z3LIB_DECODE_ONLY

#define Z3FE_BUFFER_SIZE (1 << 12)
#define Z3FE_EXTRA_MEMSIZE 65402

enum z3fe_state {
  z3fe_state_fextra,
  z3fe_state_fname,
  z3fe_state_fcomment,
  z3fe_state_data,
  z3fe_state_tail,
  z3fe_state_end,
  z3fe_nb_state
};

struct z3fe_handle {
  int filedescr;
  __u32 bin, bout;
  enum z3fe_state state;
  __u32 total;
  __u32 crc;
  __u8 buffer[Z3FE_BUFFER_SIZE];
  union {
    struct z3de_handle z3de;
    struct {
      char space[Z3FE_EXTRA_MEMSIZE + sizeof(struct z3de_handle)];
      char space3[Z3BE_MEMSIZE_EXTRA3];
    } s;
  } z;
};
#endif /* Z3LIB_DECODE_ONLY */

#ifndef Z3LIB_ENCODE_ONLY

#define Z3FD_BUFFER_SIZE (1 << 12)

enum z3fd_state {
  z3fd_state_init,
  z3fd_state_fxlen,
  z3fd_state_fextra,
  z3fd_state_fname,
  z3fd_state_fcomment,
  z3fd_state_fhcrc,
  z3fd_state_data,
  z3fd_state_end,
  z3fd_nb_state
};

struct z3fd_handle {
  int filedescr;
  __u32 bin, bout;
  enum z3fd_state state;
  __u32 given;
  __u32 index;
  __u32 total;
  __u32 pbits;
  int pbits_nb;
  __u32 crc;
  __u8 buffer[Z3FD_BUFFER_SIZE];
  __u8 flg, os;
  __u16 xlen;
  time_t mtime;
  struct z3dd_handle z3dd;
};
#endif /* Z3LIB_ENCODE_ONLY */

/* public stuff: */

#ifndef Z3LIB_DECODE_ONLY

/*
 * initialise compression
 * input:
 *    filedescr: unix file descriptor, referring to a file previously opened
 *      for writing
 *    level: compression level desired, in range 1..9, or 0 for default
 *    mtime: time_t seconds since 1970/01/01 00:00:00
 *    os: operating system indicator byte, -1 when unknown
 *    xlen: number of bytes in field fextra, when given
 *    fextra: array of xlen bytes to be included in header, or NULL
 *    fname: zero terminated name string, or NULL
 *    fcomment: zero terminated comment string, or NULL
 * return:
 *    0 on success, negative on error (see errno.h)
 */
int z3f_encode_init(int filedescr, struct z3fe_handle *zh, int level,
                        time_t mtime, int os,
                        int xlen, __u8 *fextra,
                        char *fname, char *fcomment);

/*
 * insert header fields, when needed, where the parameters should be
 * those given to z3f_encode_init before and stay constant;
 * call this function repeated, as long as it returns -EAGAIN
 * return:
 *   0 on success,
 *   -EAGAIN when the function needs to be called again
 *   other negative codes on error (see errno.h)
 */
int z3f_encode_header(struct z3fe_handle *zh, int xlen, __u8 *fextra,
                        char *fname, char *fcomment);

/*
 * write compressed data to file, compress according to rfc1952
 * input:
 *    data: a pointer to uncompressed data, to be compressed and written
 *    count: number of bytes available at data, which must be non-zero
 *      as long as the file shall not be closed. When no more data is to
 *      be written, it must be zero, and the function must be called
 *      again, until it returns 0, too
 * return:
 *    number of bytes written from data buffer,
 *    0 on end of file,
 *    -EAGAIN when function needs to be called again,
 *    other negative codes on error (see errno.h)
 */
int z3f_encode_write(struct z3fe_handle *zh, __u8 *data, __u32 count);

#endif /* Z3LIB_DECODE_ONLY */

#ifndef Z3LIB_ENCODE_ONLY

/*
 * initialise decompression
 * input:
 *    filedescr: unix file descriptor, referring to a file previously opened
 *      for reading
 * return:
 *    0 on success, negative on error (see errno.h)
 */
int z3f_decode_init(int filedescr, struct z3fd_handle *zh);

/*
 * read compressed data from file, decompress according to rfc1952
 * input:
 *    count: space available in read buffer, in bytes
 * output:
 *    data: a pointer where to store decompressed bytes, not more than count
 * return:
 *    number of bytes read into data buffer,
 *    0 on end of file,
 *    -EAGAIN when function needs to be called again,
 *    other negative codes on error (see errno.h)
 */
int z3f_decode_read(struct z3fd_handle *zh, __u8 *data, __u32 count);

#endif /* Z3LIB_ENCODE_ONLY */
#endif /* __Z3FLIB_H__ */
