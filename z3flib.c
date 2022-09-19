/*
 * z3lib (c)2005,2006/GPL,BSD Oskar Schirmer, schirmer@scara.com
 * combined huffman/lz compression library
 * rfc1952 compliant
 */

#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "z3lib.h"
#include "z3liblib.h"
#include "z3crc32.h"
#include "z3blib.h"
#include "z3dlib.h"
#include "z3flib.h"

static __u32 crc_32_table[256];
static int crc_32_table_filled = 0;

static void z3f_fill_crc32table(void)
{
  if (!crc_32_table_filled) {
    crc_32_table_filled = 1;
    gen_crc32_table(&crc_32_table[0]);
  }
}

#ifndef Z3LIB_DECODE_ONLY

int z3f_encode_init(int filedescr, struct z3fe_handle *zh, int level,
                        time_t mtime, int os,
                        int xlen, __u8 *fextra,
                        char *fname, char *fcomment)
{
  __u32 i;
  if (zh == NULL) {
    return -EINVAL;
  }
  if (filedescr < 0) {
    return -ENOENT;
  }
  if (level <= 0) {
    level = 6;
  } else if (level > 9) {
    level = 9;
  }
  if (z3d_encode_init(&zh->z.z3de,
        sizeof(zh->z.s.space) + ((level > 5) ? sizeof(zh->z.s.space3) : 0),
        (level > 3) ? 4096 : 0, 76, 76, 13,
        (level > 7) ? 1 : 0, (level > 5) ? 1 : 0) == NULL) {
    return -EFAULT;
  }
  zh->filedescr = filedescr;
  zh->buffer[0] = 0x1f;
  zh->buffer[1] = 0x8b;
  zh->buffer[2] = 8;
  zh->buffer[3] = ((fextra == NULL) ? 0 : (1 << 2))
                | ((fname == NULL) ? 0 : (1 << 3))
                | ((fcomment == NULL) ? 0 : (1 << 4));
  zh->buffer[4] = (i = (__u32)mtime);
  zh->buffer[5] = (i >>= 8);
  zh->buffer[6] = (i >>= 8);
  zh->buffer[7] = i >> 8;
  zh->buffer[8] = 0;
  zh->buffer[9] = (__u8)os;
  if (fextra != NULL) {
    if (xlen >> 16) {
      return -EINVAL;
    }
    zh->buffer[10] = (__u8)xlen;
    zh->buffer[11] = (__u8)(xlen >> 8);
    zh->bin = 12;
  } else {
    zh->bin = 10;
  }
  zh->bout = 0;
  zh->state = (fextra != NULL) ? z3fe_state_fextra :
                (fname != NULL) ? z3fe_state_fname :
                (fcomment != NULL) ? z3fe_state_fcomment : z3fe_state_data;
  zh->total = 0;
  zh->crc = CRC_INIT_32;
  z3f_fill_crc32table();
  return 0;
}

int z3f_encode_header(struct z3fe_handle *zh, int xlen, __u8 *fextra,
                        char *fname, char *fcomment)
{
  int r;
  __u32 flen;
  __u8 *fdat;
  enum z3fe_state next;
  while (zh->bin < Z3FE_BUFFER_SIZE) {
    switch (zh->state) {
    case z3fe_state_fextra:
      next = z3fe_state_fname;
      fdat = fextra;
      flen = (fdat != NULL) ? (__u32)xlen : 0;
      break;
    case z3fe_state_fname:
      next = z3fe_state_fcomment;
      fdat = (__u8*)fname;
      flen = (fdat != NULL) ? strlen(fdat) + 1 : 0;
      break;
    case z3fe_state_fcomment:
      next = z3fe_state_data;
      fdat = (__u8*)fcomment;
      flen = (fdat != NULL) ? strlen(fdat) + 1 : 0;
      break;
    case z3fe_state_data:
      return 0;
    default:
      return -EPERM;
    }
    r = flen - zh->total;
    if (r > 0) {
      if (r > (Z3FE_BUFFER_SIZE - zh->bin)) {
        r = Z3FE_BUFFER_SIZE - zh->bin;
      }
      memcpy(&zh->buffer[zh->bin], &fdat[zh->total], r);
      zh->bin += r;
      zh->total += r;
    } else {
      zh->state = next;
      zh->total = 0;
    }
  }
  r = write(zh->filedescr, &zh->buffer[zh->bout], zh->bin - zh->bout);
  if (r < 0) {
    return -errno;
  } else {
    zh->bout += r;
  }
  if (zh->bin == zh->bout) {
    zh->bin = zh->bout = 0;
  }
  return -EAGAIN;
}

int z3f_encode_write(struct z3fe_handle *zh, __u8 *data, __u32 count)
{
  int r;
  __u32 i;
  i = zh->bin - zh->bout;
  if (i > 0) {
    r = write(zh->filedescr, &zh->buffer[zh->bout], i);
    if (r < 0) {
      return -errno;
    } else {
      zh->bout += r;
    }
  }
  if (zh->bin == zh->bout) {
    zh->bout = 0;
    switch (zh->state) {
    case z3fe_state_data:
      if (z3d_encode(&zh->z.z3de, data, count, &i,
                  &zh->buffer[0], Z3FE_BUFFER_SIZE, &zh->bin) == NULL) {
        zh->state = z3fe_state_tail;
      } else if (i != 0) {
        zh->crc = update_crc_32_block(&crc_32_table[0], zh->crc, data, i);
        zh->total += i;
        return i;
      }
      return -EAGAIN;
    case z3fe_state_tail:
      zh->bin = 8;
      i = ~zh->crc;
      r = 0;
      do {
        zh->buffer[r] = i;
        i >>= 8;
      } while (++r < 4);
      i = zh->total;
      do {
        zh->buffer[r] = i;
        i >>= 8;
      } while (++r < 8);
      zh->state = z3fe_state_end;
      return -EAGAIN;
    case z3fe_state_end:
      return 0;
    default:
      return -EPERM;
    }
  }
  return -EAGAIN;
}

#endif /* Z3LIB_DECODE_ONLY */

#ifndef Z3LIB_ENCODE_ONLY

int z3f_decode_init(int filedescr, struct z3fd_handle *zh)
{
  if (zh == NULL) {
    return -EINVAL;
  }
  if (filedescr < 0) {
    return -ENOENT;
  }
  if (z3d_decode_init(0, 0, &zh->z3dd, sizeof(zh->z3dd)) == NULL) {
    return -EFAULT;
  }
  zh->filedescr = filedescr;
  zh->bin = zh->bout = 0;
  zh->state = z3fd_state_init;
  zh->given = 0;
  zh->total = 0;
  zh->crc = CRC_INIT_32;
  z3f_fill_crc32table();
  return 0;
}

static __u8 z3fd_minbufsize[z3fd_nb_state] = {
  18, 10, 9, 9, 9, 10, 9, 9
};

int z3f_decode_read(struct z3fd_handle *zh, __u8 *data, __u32 count)
{
  int r;
  __u32 i;
  __u8 *buf;
  i = zh->bin - zh->bout;
  if ((zh->given == 0)
   && (i < z3fd_minbufsize[zh->state])) {
    if (zh->bin >= Z3FD_BUFFER_SIZE) {
      z3lib_memmoveleft(&zh->buffer[0], &zh->buffer[zh->bout], i);
      zh->bin = i;
      zh->bout = 0;
    }
    r = read(zh->filedescr, &zh->buffer[zh->bin], Z3FD_BUFFER_SIZE - zh->bin);
    if (r < 0) {
      return -errno;
    } else if (zh->state == z3fd_state_end) {
      if (r != 0) {
        return -EOVERFLOW;
      }
    } else if (r == 0) {
      if (zh->state != z3fd_state_data) {
        return -ENODATA;
      }
    } else {
      zh->bin += r;
      return -EAGAIN;
    }
  }
  buf = &zh->buffer[zh->bout];
  switch (zh->state) {
  case z3fd_state_init:
    if ((buf[0] != 0x1f)
     || (buf[1] != 0x8b)
     || (buf[2] != 8)
     || (buf[3] & 0xe0)) {
      return -EPROTO;
    }
    zh->flg = buf[3];
    zh->os = buf[9];
    zh->mtime = (((((buf[7] << 8) + buf[6]) << 8) + buf[5]) << 8) + buf[4];
    zh->bout += 10;
    if (zh->flg & Z3F_FLG_FEXTRA) {
      zh->state = z3fd_state_fxlen;
    } else if (zh->flg & Z3F_FLG_FNAME) {
      zh->state = z3fd_state_fname;
    } else if (zh->flg & Z3F_FLG_FCOMMENT) {
      zh->state = z3fd_state_fcomment;
    } else if (zh->flg & Z3F_FLG_FHCRC) {
      zh->state = z3fd_state_fhcrc;
    } else {
      zh->state = z3fd_state_data;
    }
    return -EAGAIN;
  case z3fd_state_fxlen:
    zh->xlen = (buf[1] << 8) + buf[0];
    zh->bout += 2;
    zh->state = z3fd_state_fextra;
    return -EAGAIN;
  case z3fd_state_fextra:
    if (zh->xlen > 0) {
      if (i > zh->xlen) {
        i = zh->xlen;
      }
      zh->xlen -= i;
      zh->bout += i;
      return -EAGAIN;
    }
    if (zh->flg & Z3F_FLG_FNAME) {
      zh->state = z3fd_state_fname;
    } else if (zh->flg & Z3F_FLG_FCOMMENT) {
      zh->state = z3fd_state_fcomment;
    } else if (zh->flg & Z3F_FLG_FHCRC) {
      zh->state = z3fd_state_fhcrc;
    } else {
      zh->state = z3fd_state_data;
    }
    return -EAGAIN;
  case z3fd_state_fname:
    while (i-- > 0) {
      zh->bout += 1;
      if (*buf++ == 0) {
        if (zh->flg & Z3F_FLG_FCOMMENT) {
          zh->state = z3fd_state_fcomment;
        } else if (zh->flg & Z3F_FLG_FHCRC) {
          zh->state = z3fd_state_fhcrc;
        } else {
          zh->state = z3fd_state_data;
        }
        return -EAGAIN;
      }
    }
    return -EAGAIN;
  case z3fd_state_fcomment:
    while (i-- > 0) {
      zh->bout += 1;
      if (*buf++ == 0) {
        if (zh->flg & Z3F_FLG_FHCRC) {
          zh->state = z3fd_state_fhcrc;
        } else {
          zh->state = z3fd_state_data;
        }
        return -EAGAIN;
      }
    }
    return -EAGAIN;
  case z3fd_state_fhcrc:
    zh->bout += 2;
    zh->state = z3fd_state_data;
    return -EAGAIN;
  case z3fd_state_data:
    if (zh->given == 0) {
      if (z3d_decode_relative(&zh->z3dd, &r, &zh->pbits, &zh->pbits_nb,
            buf, i-8, &i, &zh->index, &zh->given) == NULL) {
        if (r == z3err_none) {
          zh->state = z3fd_state_end;
          return -EAGAIN;
        } else {
          return -EBADMSG;
        }
      }
      zh->bout += i;
    }
    i = zh->given;
    if (i > count) {
      i = count;
    }
    if (i == 0) {
      return -EAGAIN;
    }
    zh->crc = update_crc_32_block(&crc_32_table[0], zh->crc,
                        zh->index + (__u8*)&zh->z3dd, i);
    memcpy(data, zh->index + (__u8*)&zh->z3dd, i);
    zh->total += i;
    zh->given -= i;
    zh->index += i;
    return i;
  case z3fd_state_end:
    if (i != 8) {
      return -EMSGSIZE;
    }
    i = (((((buf[7] << 8) + buf[6]) << 8) + buf[5]) << 8) + buf[4];
    if (i != zh->total) {
      return -ENOEXEC;
    }
    *(__u32*)&buf[0] ^= (__u32)-1;
    if (update_crc_32_block(&crc_32_table[0], zh->crc, &buf[0], 4) != 0) {
      return -EILSEQ;
    }
    return 0;
  default:
    break;
  }
  return -EBADRQC;
}

#endif /* Z3LIB_ENCODE_ONLY */

