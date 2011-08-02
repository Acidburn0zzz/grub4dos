/* builtins.c - the GRUB builtin commands */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Include stdio.h before shared.h, because we can't define
   WITHOUT_LIBC_STUBS here.  */
#ifdef GRUB_UTIL
# include <stdio.h>
#endif

#include <shared.h>
#include <filesys.h>
#include <term.h>
#include "iamath.h"

#ifdef SUPPORT_NETBOOT
# define GRUB	1
# include <etherboot.h>
#endif

#ifdef SUPPORT_SERIAL
# include <serial.h>
# include <terminfo.h>
#endif

#ifdef GRUB_UTIL
# include <device.h>
#else /* ! GRUB_UTIL */
# include <apic.h>
//# include <smp-imps.h>
#endif /* ! GRUB_UTIL */

#ifdef USE_MD5_PASSWORDS
# include <md5.h>
#endif

#include "freebsd.h"

/* The type of kernel loaded.  */
kernel_t kernel_type;
static kernel_t kernel_type_orig;

/* The boot device.  */
static int bootdev = 0;

#ifdef GRUB_UTIL
/* 0 for silent, 1 for normal, 2..MAX_UINT for verbose.
 * if debug == MAX_UINT or debug < 0, then filesystem debug mode is on.
 * (MAX_UINT is 0x7FFFFFFF)
 */
int debug = 1;

int use_config_file = 1;
unsigned long free_mem_start;
unsigned long free_mem_end;
#endif

/* The default entry.  */
int default_entry = 0;
/* The fallback entry.  */
int fallback_entryno;
int fallback_entries[MAX_FALLBACK_ENTRIES];
/* The number of current entry.  */
int current_entryno;
#ifdef SUPPORT_GFX
/* graphics file */
char graphics_file[64];
#endif
/* The address for Multiboot command-line buffer.  */
static char *mb_cmdline;// = (char *) MB_CMDLINE_BUF;
static char kernel_option_video[64] = {0};/* initialize the first byte to 0 */
/* The password.  */
char *password;
/* The password type.  */
password_t password_type;
/* The flag for indicating that the user is authoritative.  */
int auth = 0;
/* The timeout.  */
int grub_timeout = -1;
/* Whether to show the menu or not.  */
int show_menu = 1;
/* Don't display a countdown message for the hidden menu */
int silent_hiddenmenu = 0;

unsigned long pxe_restart_config = 0;
#ifndef GRUB_UTIL
unsigned long configfile_in_menu_init = 0;
#endif /* ! GRUB_UTIL */

//extern unsigned long i;
//extern char *mbr;
#if !defined(STAGE1_5) && !defined(GRUB_UTIL)
/* The first sector of stage2 can be reused as a tmp buffer.
 * Do NOT write more than 512 bytes to this buffer!
 * The stage2-body, i.e., the pre_stage2, starts at 0x8200!
 * Do NOT overwrite the pre_stage2 code at 0x8200!
 */
extern char *mbr /* = (char *)0x8000 */; /* 512-byte buffer for any use. */
#else
extern char mbr[SECTOR_SIZE];
#endif /* STAGE1_5 || GRUB_UTIL */

#ifndef STAGE1_5
extern int dir (char *dirname);
#endif /* STAGE1_5 */

#ifdef SUPPORT_GRAPHICS
extern int outline;
#endif /* SUPPORT_GRAPHICS */

#ifndef GRUB_UTIL

/* Whether the drive map hook is on.  */
//static int int13_on_hook = 0;

//static int floppy_not_inserted[4] = {0, 0, 0, 0};

/* The BIOS drive map.  */
int drive_map_slot_empty (struct drive_map_slot item);

/* backup of original BIOS floppy-count byte in 0x410 */
extern char floppies_orig;

/* backup of original BIOS harddrive-count byte in 0x475 */
extern char harddrives_orig;

extern unsigned short chain_load_segment;//0x0000;
extern unsigned short chain_load_offset;//0x7c00;
extern unsigned int chain_load_length;//0x200;
extern unsigned short chain_boot_CS;//0x0000;
extern unsigned short chain_boot_IP;//0x7c00;
extern unsigned long chain_ebx;
extern unsigned long chain_ebx_set;
extern unsigned long chain_edx;
extern unsigned long chain_edx_set;
extern unsigned long chain_enable_gateA20;
extern unsigned long chain_bx;
extern unsigned long chain_bx_set;
extern unsigned long chain_cx;
extern unsigned long chain_cx_set;
extern char HMA_start[];
#endif /* ! GRUB_UTIL */

static int chainloader_load_segment = 0;
static int chainloader_load_segment_orig = 0;
static int chainloader_load_offset = 0;
static int chainloader_load_offset_orig = 0;
static int chainloader_load_length = 0;
static int chainloader_load_length_orig = 0;
static int chainloader_skip_length = 0;
static int chainloader_skip_length_orig = 0;
static int chainloader_boot_CS = 0;
static int chainloader_boot_CS_orig = 0;
static int chainloader_boot_IP = 0;
static int chainloader_boot_IP_orig = 0;
static long chainloader_ebx = 0;
static long chainloader_ebx_orig = 0;
static int chainloader_ebx_set = 0;
static int chainloader_ebx_set_orig = 0;
static long chainloader_edx = 0;
static long chainloader_edx_orig = 0;
static int chainloader_edx_set = 0;
static int chainloader_edx_set_orig = 0;
static long chainloader_bx = 0;
static long chainloader_bx_orig = 0;
static int chainloader_bx_set = 0;
static int chainloader_bx_set_orig = 0;
static long chainloader_cx = 0;
static long chainloader_cx_orig = 0;
static int chainloader_cx_set = 0;
static int chainloader_cx_set_orig = 0;
static int chainloader_disable_A20 = 0;
static int chainloader_disable_A20_orig = 0;
static int is_sdi = 0;
static int is_sdi_orig = 0;
static int is_raw = 0;
static int is_raw_orig = 0;
static int is_isolinux = 0;
static int is_isolinux_orig = 0;
static int is_grldr = 0;
static int is_grldr_orig = 0;
static int is_io = 0;
static int is_io_orig = 0;
static char chainloader_file[256];
static char chainloader_file_orig[256];


static const char *warning_defaultfile = "# WARNING: If you want to edit this file directly, do not remove any line";

#ifndef GRUB_UTIL
int
drive_map_slot_empty (struct drive_map_slot item)
{
	unsigned long *array = (unsigned long *)&item;
	
	unsigned long n = sizeof (struct drive_map_slot) / sizeof (unsigned long);
	
	while (n)
	{
		if (*array)
			return 0;
		array++;
		n--;
	}

	return 1;
	//if (*(unsigned long *)(&(item.from_drive))) return 0;
	//if (item.start_sector) return 0;
	//if (item.sector_count) return 0;
	//return 1;
}

static int
drive_map_slot_equal (struct drive_map_slot a, struct drive_map_slot b)
{
	//return ! grub_memcmp ((void *)&(a.from_drive), (void *)&(b.from_drive), sizeof(struct drive_map_slot));
	return ! grub_memcmp ((void *)&a, (void *)&b, sizeof(struct drive_map_slot));
	//if (*(unsigned long *)(&(a.from_drive)) != *(unsigned long *)(&(b.from_drive))) return 0;
	//if (a.start_sector != b.start_sector) return 0;
	//if (a.sector_count != b.sector_count) return 0;
	//return 1;
}
int map_func (char *arg, int flags);
int disable_map_info = 0;
#endif /* ! GRUB_UTIL */

/* Prototypes for allowing straightfoward calling of builtins functions
   inside other functions.  */
static int configfile_func (char *arg, int flags);
void lba_to_chs (unsigned long lba, unsigned long *cl, unsigned long *ch, unsigned long *dh);
//static char *set_device (char *device);
//static int real_open_partition (int flags);
//static int open_partition (void);
int command_func (char *arg, int flags);
int commandline_func (char *arg, int flags);
int errnum_func (char *arg, int flags);
int checkrange_func (char *arg, int flags);

/* Check a password for correctness.  Returns 0 if password was
   correct, and a value != 0 for error, similarly to strcmp. */
int
check_password (char* expected, password_t type)
{
	/* Do password check! */
	char entered[32];

	/* Wipe out any previously entered password */
	//   entered[0] = 0;
	memset(entered,0,sizeof(entered));
	get_cmdline_str.prompt = "Password: ";
	get_cmdline_str.maxlen = sizeof (entered) - 1;
	get_cmdline_str.echo_char = '*';
	get_cmdline_str.readline = 0;
	get_cmdline_str.cmdline = entered;
	get_cmdline (get_cmdline_str);
	
  switch (type)
    {
    case PASSWORD_PLAIN:
      return strcmp (entered, expected);

#ifdef USE_MD5_PASSWORDS
    case PASSWORD_MD5:
      return check_md5_password (entered, expected);
#endif
    default: 
      /* unsupported password type: be secure */
      return 1;
    }
}

/* Print which sector is read when loading a file.  */
static void
disk_read_print_func (unsigned long sector, unsigned long offset, unsigned long length)
{
  grub_printf ("[%d,%d,%d]", sector, offset, length);
}

extern int rawread_ignore_memmove_overflow; /* defined in disk_io.c */
long query_block_entries;
unsigned long map_start_sector;
static unsigned long map_num_sectors = 0;

static unsigned long blklst_start_sector;
static unsigned long blklst_num_sectors;
static unsigned long blklst_num_entries;
static unsigned long blklst_last_length;
  
static void disk_read_blocklist_func (unsigned long sector, unsigned long offset, unsigned long length);
  
  /* Collect contiguous blocks into one entry as many as possible,
     and print the blocklist notation on the screen.  */
static void
disk_read_blocklist_func (unsigned long sector, unsigned long offset, unsigned long length)
{
      if (blklst_num_sectors > 0)
	{
	  if (blklst_start_sector + blklst_num_sectors == sector
	      && offset == 0 && blklst_last_length == buf_geom.sector_size)
	    {
	      blklst_num_sectors++;
	      blklst_last_length = length;
	      return;
	    }
	  else
	    {
	      if (query_block_entries >= 0)
	        {
		  if (blklst_last_length == buf_geom.sector_size)
		    grub_printf ("%s%ld+%d", (blklst_num_entries ? "," : ""),
			     (unsigned long long)(blklst_start_sector - part_start), blklst_num_sectors);
		  else if (blklst_num_sectors > 1)
		    grub_printf ("%s%ld+%d,%ld[0-%d]", (blklst_num_entries ? "," : ""),
			     (unsigned long long)(blklst_start_sector - part_start), (blklst_num_sectors-1),
			     (unsigned long long)(blklst_start_sector + blklst_num_sectors-1 - part_start), 
			     blklst_last_length);
		  else
		    grub_printf ("%s%ld[0-%d]", (blklst_num_entries ? "," : ""),
			     (unsigned long long)(blklst_start_sector - part_start), blklst_last_length);
	        }
	      blklst_num_entries++;
	      blklst_num_sectors = 0;
	    }
	}

      if (offset > 0)
	{
	  if (query_block_entries >= 0)
	  grub_printf("%s%ld[%d-%d]", (blklst_num_entries ? "," : ""),
		      (unsigned long long)(sector - part_start), offset, (offset + length));
	  blklst_num_entries++;
	}
      else
	{
	  blklst_start_sector = sector;
	  blklst_num_sectors = 1;
	  blklst_last_length = length;
	}
}

/* blocklist */
static int
blocklist_func (char *arg, int flags)
{
//#ifdef GRUB_UTIL
//  char *dummy = (char *) RAW_ADDR (0x100000);
//#else
//  char *dummy = (char *) RAW_ADDR (0x400000);
//#endif
  char *dummy = NULL;
  int err;
#ifndef NO_DECOMPRESSION
  int no_decompression_bak = no_decompression;
#endif
  
  blklst_start_sector = 0;
  blklst_num_sectors = 0;
  blklst_num_entries = 0;
  blklst_last_length = 0;

  map_start_sector = 0;
  map_num_sectors = 0;

  /* Open the file.  */
  if (! grub_open (arg))
    goto fail_open;

#ifndef NO_DECOMPRESSION
  if (compressed_file)
  {
    if (query_block_entries < 0)
    {
	/* compressed files are not considered contiguous. */
	query_block_entries = 3;
	goto fail_read;
    }

    grub_close ();
    no_decompression = 1;
    if (! grub_open (arg))
	goto fail_open;
  }
#endif /* NO_DECOMPRESSION */

  /* Print the device name.  */
  if (query_block_entries >= 0)
  {
	grub_printf ("(%cd%d", ((current_drive & 0x80) ? 'h' : 'f'), (current_drive & ~0x80));
  
	if ((current_partition & 0xFF0000) != 0xFF0000)
	    grub_printf (",%d", ((unsigned char)(current_partition >> 16)));
  
	if ((current_partition & 0x00FF00) != 0x00FF00)
	    grub_printf (",%c", ('a' + ((unsigned char)(current_partition >> 8))));
  
	grub_printf (")");
  }

  rawread_ignore_memmove_overflow = 1;
  /* Read in the whole file to DUMMY.  */
  disk_read_hook = disk_read_blocklist_func;
  err = grub_read ((unsigned long long)(unsigned int)dummy, (query_block_entries < 0 ? buf_geom.sector_size : -1ULL), 0xedde0d90);
  disk_read_hook = 0;
  rawread_ignore_memmove_overflow = 0;
  if (! err)
    goto fail_read;

  /* The last entry may not be printed yet.  Don't check if it is a
   * full sector, since it doesn't matter if we read too much. */
  if (blklst_num_sectors > 0)
    {
      if (query_block_entries >= 0)
        grub_printf ("%s%ld+%d", (blklst_num_entries ? "," : ""),
		 (unsigned long long)(blklst_start_sector - part_start), blklst_num_sectors);
      blklst_num_entries++;
    }

  if (query_block_entries >= 0)
    grub_putchar ('\n');
#if 0
  if (num_entries > 1)
    query_block_entries = num_entries;
  else
    {
      query_block_entries = 1;
      map_start_sector = start_sector;
      map_num_sectors = num_sectors;
    }
#endif
  if (query_block_entries < 0)
    {
      map_start_sector = blklst_start_sector;
      blklst_start_sector = 0;
      blklst_num_sectors = 0;
      blklst_num_entries = 0;
      blklst_last_length = 0;
      rawread_ignore_memmove_overflow = 1;
      /* read in the last sector to DUMMY */
      filepos = filemax ? (filemax - 1) & (-(unsigned long long)buf_geom.sector_size) : filemax;
      disk_read_hook = disk_read_blocklist_func;
      err = grub_read ((unsigned long long)(unsigned int)dummy, -1ULL, 0xedde0d90);
      disk_read_hook = 0;
      rawread_ignore_memmove_overflow = 0;
      if (! err)
        goto fail_read;
      map_num_sectors = blklst_start_sector - map_start_sector + 1;
      query_block_entries = filemax ? 
	      map_num_sectors - ((filemax - 1) >> log2_tmp (buf_geom.sector_size)/*>> SECTOR_BITS*/) : 2;
    }

fail_read:

  grub_close ();

fail_open:

#ifndef NO_DECOMPRESSION
  no_decompression = no_decompression_bak;
#endif

  if (query_block_entries < 0)
    query_block_entries = 0;
  return ! errnum;
}

static struct builtin builtin_blocklist =
{
  "blocklist",
  blocklist_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "blocklist FILE",
  "Print the blocklist notation of the file FILE."
};

#if ! defined(GRUB_UTIL) && ! defined (STAGE1_5)
/* WinME support by bean. Thanks! */

// The following routines are used to decompress IO.SYS from WinME

static int ibuf_pos;
static char *ibuf_ptr,*obuf_ptr;
static unsigned short ibuf_tab[16];

static void ibuf_init(char* buf);
static unsigned short ibuf_read(int nbits);
static int expand_block(int nsec);
static long expand_file(char* src,char* dst);

static void ibuf_init(char* buf)
{
  int i;

  ibuf_ptr=buf;
  ibuf_pos=0;

  // ibuf_tab[i]=(1<<(i+1))-1
  ibuf_tab[0]=1;
  for (i=1;i<16;i++)
    ibuf_tab[i]=ibuf_tab[i-1]*2+1;
}

static unsigned short ibuf_read(int nbits)
{
  unsigned short res;

  res=(unsigned short)((*((unsigned long*)ibuf_ptr)>>ibuf_pos) & ibuf_tab[nbits-1]);
  ibuf_pos+=nbits;
  if (ibuf_pos>=8)
    {
      ibuf_ptr+=(ibuf_pos>>3);
      ibuf_pos&=7;
    }
  return res;
}

#define obuf_init(buf)		obuf_ptr=buf
#define obuf_putc(ch)		*(obuf_ptr++)=ch
#define obuf_copy(ofs,len)	do { for (;len>0;obuf_ptr++,len--) *(obuf_ptr)=*(obuf_ptr-ofs); } while (0)
// Don't use memcpy, otherwise we could be screwed !

static int expand_block(int nsec)
{
  while (nsec>0)
    {
      int cnt;

      cnt=0x200;
      while (1)
        {
          int flg,ofs,bts,bse,del,len;

          flg=ibuf_read(2);

          if (flg==0) ofs=ibuf_read(6); else
            if (flg==3)
              {
                if (ibuf_read(1))
                  {
                    ofs=ibuf_read(12)+0x140;
                    if (ofs==0x113F)
                      break;
                  }
                else
                  ofs=ibuf_read(8)+0x40;
              } else
                {
                  char ch;

                  cnt--;
                  if (cnt<0)
                    {
                      grub_putstr("Data corrupted");
                      return 1;
                    }
                  ch=ibuf_read(7);
                  if (flg & 1)
                    ch|=0x80;
                  obuf_putc(ch);
                  continue;
                }
          if (ofs==0)
            {
              grub_putstr("Data corrupted");
              return 1;
            }
          bse=2;
          del=0;
          for (bts=0;bts<9;bts++)
            {
              if (ibuf_read(1))
                break;
              bse+=del+1;
              del=del*2+1;
            }
          if (bts==9)
            {
              grub_putstr("Data corrupted");
              return 1;
            }
          len=(bts)?bse+ibuf_read(bts):bse;
          if ((cnt=cnt-len)<0)
            {
              grub_putstr("Data corrupted");
              return 1;
            }
          obuf_copy(ofs,len);
        }
      nsec--;
      if ((cnt) && (nsec))
        {
          grub_putstr("Data corrupted");
          return 1;
        }
    }
  return 0;
}

static long expand_file(char* src,char* dst)
{
  ibuf_init(src);
  obuf_init(dst);

  if (ibuf_read(16)!=0x4D43)
    {
      grub_putstr("First CM signature not found");
      return -1;
    }
  while (1)
    {
      unsigned short flg,len;

      flg=ibuf_read(8);
      len=ibuf_read(16);
      if (len==0)
        {
          int n;

          n=(ibuf_ptr-src) & 0xF;
          if ((n) || (ibuf_pos))
            {
              ibuf_ptr+=16-n;
              ibuf_pos=0;
            }
          if (ibuf_read(16)!=0x4D43)
            {
              grub_putstr("Second CM signature not found");
              return -1;
            }
          return obuf_ptr-dst;
        }
      if (flg==0)
        {
          memcpy(obuf_ptr,ibuf_ptr,len);
          ibuf_ptr+=len;
          obuf_ptr+=len;
        }
      else
        {
          char* save_ptr;
          unsigned short sec;

          sec=(ibuf_read(16)+511)>>9;
          save_ptr=ibuf_ptr;
          if (ibuf_read(16)!=0x5344)
            {
              grub_putstr("0x5344 signature not found");
              return -1;
            }
          ibuf_read(16);
          if (expand_block(sec))
            return -1;
          ibuf_ptr=save_ptr+len;
          ibuf_pos=0;
        }
    }
}
#endif /* ! defined(GRUB_UTIL) && ! defined (STAGE1_5) */

#ifndef GRUB_UTIL
/* boot */
static int
boot_func (char *arg, int flags)
{
  int old_cursor, old_errnum;
  struct term_entry *prev_term = current_term;

  /* Clear the int15 handler if we can boot the kernel successfully.
     This assumes that the boot code never fails only if KERNEL_TYPE is
     not KERNEL_TYPE_NONE. Is this assumption is bad?  */
  if (kernel_type != KERNEL_TYPE_NONE)
    unset_int15_handler ();

   /* if our terminal needed initialization, we should shut it down
    * before booting the kernel, but we want to save what it was so
    * we can come back if needed */
  if (current_term->shutdown) 
    {
      (*current_term->shutdown)();
      current_term = term_table; /* assumption: console is first */
    }

#ifdef FSYS_PXE
  if (kernel_type!=KERNEL_TYPE_CHAINLOADER)
    pxe_unload();
#endif

#ifdef SUPPORT_NETBOOT
  /* Shut down the networking.  */
  cleanup_net ();
#endif
  
  old_cursor = setcursor (1);
  errnum = 0;

  //errorcheck = 1;
  /* clear keyboard buffer before boot */
  while (console_checkkey () != -1) console_getkey ();
  
  switch (kernel_type)
    {
    case KERNEL_TYPE_FREEBSD:
    case KERNEL_TYPE_NETBSD:
      /* *BSD */
      bsd_boot (kernel_type, /*0*/bootdev, (char *) mbi.cmdline);
      break;

    case KERNEL_TYPE_LINUX:
      /* Linux */
      map_func ("(0x22) (0x22)", flags);	/* delete mapping for INITRD_DRIVE */
      map_func ("--rehook", flags);
      linux_boot ();
      break;

    case KERNEL_TYPE_BIG_LINUX:
      /* Big Linux */

      {
	unsigned int p;
	/* check grub.exe */
	for (p = (unsigned int)linux_bzimage_tmp_addr; p < (unsigned int)linux_bzimage_tmp_addr + 0x8000; p += 0x200)
	{
		if (((*(long long *)(void *)p & 0xFFFF00FFFFFFFFFFLL) == 0x02030000008270EALL) &&
			((*(long long *)(void *)(p + 0x12) & 0xFFFFFFFFFFLL) == 0x0037392E30LL))
		{
#ifdef FSYS_PXE
		    if (pxe_entry == 0)
			*(char *)(void *)(p + 5) |= 0x01;	// disable pxe
#endif
		    if (*(long *)(void *)(p + 0x80) == 0xFFFFFFFF)//boot_drive
		    {
			*(long *)(void *)(p + 0x80) = saved_drive;
			*(long *)(void *)(p + 0x08) = saved_partition;
		    }
			break;
		}
	}
      }

      map_func ("(0x22) (0x22)", flags);	/* delete mapping for INITRD_DRIVE */
      map_func ("--rehook", flags);
      big_linux_boot ();
      break;

    case KERNEL_TYPE_CHAINLOADER:
      /* Chainloader */
      
      /* set boot drive to the root device.
       * the boot drive should be either (fd0) or (hd0) for normal
       * MS-style boot sectors. the BIOS only passes (fd0) or (hd0)
       * to DL, because these two are the only devices that can be
       * booted by BIOS.
       *
       * You should set boot drive using the rootnoverify command just
       * before the boot command.
       */
      boot_drive = saved_drive;

#ifndef GRUB_UTIL
      /* extended chainloader */
      if (chainloader_load_length > 512 || chainloader_boot_CS > 0 ||
		chainloader_skip_length ||
		chainloader_ebx_set ||
		chainloader_edx_set ||
		chainloader_bx_set ||
		chainloader_cx_set ||
		is_sdi || is_raw || //chainloader_disable_A20 ||
		(chainloader_boot_IP >= 0 && chainloader_boot_IP != 0x7c00) ||
		(((chainloader_load_segment >= 0)? chainloader_load_segment : 0) << 4) +
		((chainloader_load_offset >= 0)? chainloader_load_offset : 0x7c00) != 0x7c00
	 )
	{
		unsigned long read_length;

		/* load high */
		if (chainloader_load_segment == -1)
			chainloader_load_segment = 0;
		if (chainloader_load_offset == -1)
		{
		    if (chainloader_load_segment)
			chainloader_load_offset = 0;
		    else
			chainloader_load_offset = 0x7c00;
		}
		if (chainloader_boot_CS == -1)
			chainloader_boot_CS = chainloader_load_segment;
		if (chainloader_boot_IP == -1)
			chainloader_boot_IP = chainloader_load_offset;

		/* Open the file.  */
		if (! grub_open (chainloader_file))
		  {
		    kernel_type = KERNEL_TYPE_NONE;
		    return 0;
		  }
	  if (is_sdi)
	    {
	      unsigned long long bytes_needed = filemax;
	      unsigned long long base = 0;

	      if (mbi.flags & MB_INFO_MEM_MAP)
	        {
	          struct AddrRangeDesc *map = (struct AddrRangeDesc *) saved_mmap_addr;
	          unsigned long end_addr = saved_mmap_addr + saved_mmap_length;

	          for (; end_addr > (unsigned long) map; map = (struct AddrRangeDesc *) (((int) map) + 4 + map->size))
		    {
		      unsigned long long top_end;

		      if (map->Type != MB_ARD_MEMORY)
			  continue;
		      top_end =  map->BaseAddr + map->Length;
		      if (top_end > 0x100000000ULL)
			  top_end = 0x100000000ULL;

#define MIN_EMU_BASE 0x200000ULL
		      if (map->Length >= bytes_needed
			 && (base = (top_end - bytes_needed) & 0xfffff000) >= MIN_EMU_BASE /* page align */
			 && map->Length >= top_end - base)
				break; /* success */
		      base = 0;
		    }
	        }
	      else
		  grub_printf ("Address Map BIOS Interface is not activated.\n");

	      if (base < MIN_EMU_BASE)
		{
		  grub_close ();
		  return ! (errnum = ERR_WONT_FIT);
		}
      
#undef MIN_EMU_BASE
	      //filepos = 0;
	      if (grub_read (base, -1ULL, 0xedde0d90) != filemax)
		{
			grub_close ();
			if (errnum == ERR_NONE)
				errnum = ERR_READ;
			return 0;
		}
	      grub_close ();
      
	      unset_int13_handler (0);
      
#define BootCodeOffsetLow (*(unsigned long *)((int)base + 0x10))
#define BootCodeSizeLow  (*(unsigned long *)((int)base + 0x18))
	      read_length = BootCodeSizeLow;
	      if (read_length > 0x90000)
		  read_length = 0x90000;
	      grub_memmove((char *)0x200000, (char *)((unsigned int)base + BootCodeOffsetLow), read_length);
#undef BootCodeOffsetLow
#undef BootCodeSizeLow
	      if (! chainloader_edx_set)
		{
		  chainloader_edx = (unsigned int)base | 0x41;
		  chainloader_edx_set = 1;
		}
	    }else{
		/* Read the first 640K */
		read_length = filemax - chainloader_skip_length;
		filepos = ((*(unsigned short *)0x413)<<10) - (chainloader_load_segment<<4) - chainloader_load_offset;
		if (read_length > filepos)
		    read_length = filepos;
		filepos = chainloader_skip_length;

		/* read the new loader to physical address 2MB, overwriting
		 * the backup area of DOS memory.
		 */

		if (grub_read ((unsigned long long) 0x200000, read_length, 0xedde0d90) != read_length)
		  {
		    grub_close ();
		    kernel_type = KERNEL_TYPE_NONE;

		    if (errnum == ERR_NONE)
			errnum = ERR_EXEC_FORMAT;
      
		    break;
		  }
		grub_close ();

#ifdef FSYS_PXE
                pxe_unload();
#endif
		if (is_io)
		{
		  /* WinMe support by bean. Thanks! */
		
		  // Not a very neat way to test WinME, works anyway
		  if ((*(unsigned short*)0x200000==0x4D43) &&
		    // ((*(char*)0x110002==0) || (*(unsigned short*)0x110000==0x4D43)) &&
		    (chainloader_skip_length==0x800))
		  {
		    unsigned long len;
		    len=(unsigned long)expand_file((char*)0x200000,(char*)0x2A0000);
		    if (len==0xffffffff || len==0)
		      {
			kernel_type = KERNEL_TYPE_NONE;
			errnum = ERR_EXEC_FORMAT;
			break;
		      }
		    grub_memmove((char*)0x200000,(char*)0x2A0000,len);
		    chainloader_load_length=read_length=len;
		  }

		  /* import from WEE, to prevent MS from wiping int32-3F. */
		  /* search these 12 bytes ... */
		  /* 83 C7 08	add di,08	*/
		  /* B9 0E 00	mov cx,0E	*/
		  /* AB		stosw		*/
		  /* 83 C7 02	add di,02	*/
		  /* E2 FA	loop (-6)	*/
		  /* ... replace with NOPs */
		  {
		    unsigned long p = 0x200000;
		    unsigned long q = 0x200000 - 16 + read_length;
		    while (p < q)
		    {
			if ((*(long *)p == 0xB908C783) &&
			    (*(long long*)(p+4)==0xFAE202C783AB000ELL))
			{
				((long *)p)[2]=((long *)p)[1]=
					*(long *)p = 0x90909090;
				p += 12;
				continue; /* would occur 3 times */
			}
			p++;
		    }
		  }
		}

		/* create boot info table for isolinux */
		if (is_isolinux)
		{
			int p;
			query_block_entries = -1; /* query block list only */
			blocklist_func (chainloader_file, flags);
			if (errnum)
				break;
			if (query_block_entries != 1)
			{
				errnum = ERR_NON_CONTIGUOUS;
				break;
			}
			*(unsigned long*)0x20000C = map_start_sector;
			*(unsigned long*)0x200010 = chainloader_load_length;

			old_errnum = 0; /* init to calculate the checksum */
			*(unsigned long*)(0x200000 + chainloader_load_length) = 0;

			for (p = 0; p < (chainloader_load_length - 0x40 + 3) / 4; p++)
			{
				old_errnum += *(long *)(0x200040 + (p << 2));
			}

			*(unsigned long*)0x200014 = old_errnum; /* checksum */
		}
#ifdef FSYS_PXE
		if (is_grldr && pxe_entry == 0)
		{
			*(unsigned char*)0x202005 |= 0x01;	/* disable pxe */
		}
#endif

		/* Turn off A20 here if --disable-a20 was specified.
		 * Note: we won't have access to the odd megas of the memory
		 * when A20 is off.
		 */
		if (chainloader_disable_A20)
		{
			if (! gateA20 (0))
			{
				/* to asure A20 is on when we return to grub. */
				gateA20 (1);	/* turn on A20 in case it is off */
				errnum = ERR_DISABLE_A20;
				break;
			}
			if (debug > 0)
				grub_printf("\nGate A20 is turned off successfully.\n");
		}

		if (! chainloader_edx_set)
		{
			chainloader_edx_set = 1;
			chainloader_edx = boot_drive;
		}
		if (chainloader_edx & 0x80)
		{
		  if (! probe_bpb((struct master_and_dos_boot_sector *)0x200000))
			if (*((unsigned long *) (0x20001C)) == 0)
			{
				int err;
				if ((err = probe_mbr ((struct master_and_dos_boot_sector *)0x200000, 0, ((chainloader_load_length + 511) >> 9), 0)))
				{
					if (debug > 0)
						printf ("Partition table not recognized(chainloader_edx=0x%X, err=%d). ", chainloader_edx, err);
					errnum = ERR_HD_VOL_START_0;
					break;
				}
			}
		} else {
		  /* clear number of hidden sectors for floppy */
		  if (! probe_bpb((struct master_and_dos_boot_sector *)0x200000))
    			if (*((unsigned long *) (0x20001C)))
        		    *((unsigned long *) (0x20001C)) = 0;
		}
		
	    }
		
		if (chainloader_load_length == -1 || chainloader_load_length > read_length)
			chainloader_load_length = read_length;
		
		chain_load_segment = (unsigned short)chainloader_load_segment;
		chain_load_offset = (unsigned short)chainloader_load_offset;
		chain_boot_CS = (unsigned short)chainloader_boot_CS;
		chain_boot_IP = (unsigned short)chainloader_boot_IP;
		chain_load_length = chainloader_load_length;
		if (! chainloader_ebx_set)
		{
			chainloader_ebx_set = 1;
			chainloader_ebx = boot_drive;
		}
		chain_ebx = chainloader_ebx;
		chain_ebx_set = chainloader_ebx_set;
		chain_edx = chainloader_edx;
		chain_edx_set = chainloader_edx_set;
		chain_bx = chainloader_bx;
		chain_bx_set = chainloader_bx_set;
		chain_cx = chainloader_cx;
		chain_cx_set = chainloader_cx_set;
		chain_enable_gateA20 = ! chainloader_disable_A20;
		
		/* Check if we should set the int13 handler.  */
		if (unset_int13_handler (1) && ! drive_map_slot_empty (bios_drive_map[0]))
		{
		  /* Set the handler. This is somewhat dangerous.  */
		  set_int13_handler (bios_drive_map);
		  buf_drive = -1;
		  buf_track = -1;
		}

		/* move the code to a safe place at 0x2B0000 */
		grub_memmove((char *)HMA_ADDR, HMA_start, 0x200/*0xfff0*/);

		//typedef void (*HMA_FUNC)(void);
		//((HMA_FUNC)0x100000)();	/* no return */

		/* Jump to high memory area. This will move boot code at
		 * 0x200000 to the destination load-segment:load-offset;
		 * setup edx and ebx registers; switch to real mode;
		 * and jump to boot-cs:boot-ip.
		 */
		((void (*)(void))HMA_ADDR)();	/* no return */
		break;
	}
#endif
      if (chainloader_disable_A20)
      {
	if (gateA20 (0))
		grub_printf("\nGate A20 is turned off successfully.\n");
	else {
		/* to asure A20 is on when we return to grub. */
		gateA20 (1);	/* turn on A20 in case it is off */
		errnum = ERR_DISABLE_A20;
		break;
	}
      }
      /* Check if we should set the int13 handler.  */
      if (unset_int13_handler (1) && ! drive_map_slot_empty (bios_drive_map[0]))
	{
	  /* Set the handler. This is somewhat dangerous.  */
	  set_int13_handler (bios_drive_map);
	  buf_drive = -1;
	  buf_track = -1;
	}
      
	if ((boot_drive & 0x80) == 0)
	{
	  /* clear number of hidden sectors for floppy */
	  if (! probe_bpb((struct master_and_dos_boot_sector *)0x7C00))
	    if (*((unsigned long *) (0x7C00 + 0x1C)))
	        *((unsigned long *) (0x7C00 + 0x1C)) = 0;
	} else {
	  if (! probe_bpb((struct master_and_dos_boot_sector *)0x7C00))
	    if (*((unsigned long *) (0x7C00 + 0x1C)) == 0)
	    {
		int err;
		if ((err = probe_mbr ((struct master_and_dos_boot_sector *)0x7C00, 0, 1, 0)))
		{
			if (debug > 0)
				printf ("Partition table not recognized(boot_drive=0x%X, err=%d). ", boot_drive, err);
			errnum = ERR_HD_VOL_START_0;
		  	break;
		}
	    }
	}
#ifdef FSYS_PXE
      pxe_unload();
#endif
      chain_stage1 (0, 0x7C00, boot_part_addr); /* no return */
      break;

    case KERNEL_TYPE_MULTIBOOT:
      /* Multiboot */
      
      /* Set the boot loader name.  */
      mbi.boot_loader_name = (unsigned long) "GNU GRUB " VERSION;

      /* Get the ROM configuration table by INT 15, AH=C0h.  */
      mbi.config_table = get_rom_config_table ();

#if 0
      /* Get the APM BIOS table.  */
      get_apm_info ();
      if (apm_bios_info.version)
      {
	mbi.apm_table = (unsigned long) &apm_bios_info;
	mbi.flags |= MB_INFO_APM_TABLE;
      }
#endif

	/* Get the drive info.  */
	mbi.drives_length = 0;
	mbi.drives_addr = (unsigned long)(saved_mmap_addr + saved_mmap_length);

	/* XXX: Too many drives will possibly use a piece of memory starting at 0x10000(64K). */

#ifdef GRUB_UTIL
#define FIND_DRIVES 8
#else
#define FIND_DRIVES (*((char *)0x475))
#endif
      {
	unsigned int drive;
	unsigned long addr = mbi.drives_addr;

	for (drive = 0x80; drive < 0x80 + FIND_DRIVES; drive++)
	{
	    struct drive_info *info = (struct drive_info *) addr;
      
	    /* Get the geometry. This ensures that the drive is present.  */

	    if (debug > 1)
		grub_printf ("get_diskinfo(%X), ", drive);

	    if (get_diskinfo (drive, &tmp_geom))
		continue;//break;

	    if (debug > 1)
		grub_printf (" %sC/H/S=%d/%d/%d, Sector Count/Size=%ld/%d\n",
			((tmp_geom.flags & BIOSDISK_FLAG_LBA_EXTENSION) ? "LBA, " : ""),
			tmp_geom.cylinders, tmp_geom.heads, tmp_geom.sectors,
			(unsigned long long)tmp_geom.total_sectors, tmp_geom.sector_size);
      
	    /* Set the information.  */
	    info->drive_number = drive;
	    info->drive_mode = ((tmp_geom.flags & BIOSDISK_FLAG_LBA_EXTENSION)
				? MB_DI_LBA_MODE : MB_DI_CHS_MODE);
	    info->drive_cylinders = tmp_geom.cylinders;
	    info->drive_heads = tmp_geom.heads;
	    info->drive_sectors = tmp_geom.sectors;

	    addr += sizeof (struct drive_info);

	    info->size = addr - (unsigned long) info;
	    mbi.drives_length += info->size;
	}
      }
#undef FIND_DRIVES

      multi_boot ((int) entry_addr, (int) &mbi, 0, -1, 0, 0, 0);
      break;

    default:
      errnum = ERR_BOOT_COMMAND;
      //return 0;
    }

  old_errnum = errnum;
  /* if we get back here, we should go back to what our term was before */
  setcursor (old_cursor);
  current_term = prev_term;
  if (current_term->startup)
      /* if our terminal fails to initialize, fall back to console since
       * it should always work */
      if ((*current_term->startup)() == 0)
          current_term = term_table; /* we know that console is first */
  errnum = old_errnum;
  return ! errnum;
}

static struct builtin builtin_boot =
{
  "boot",
  boot_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "boot",
  "Boot the OS/chain-loader which has been loaded."
};
#endif /* ! GRUB_UTIL */


#ifdef SUPPORT_NETBOOT
#ifndef GRUB_UTIL
/* bootp */
static int
bootp_func (char *arg, int flags)
{
  int with_configfile = 0;

  if (grub_memcmp (arg, "--with-configfile", sizeof ("--with-configfile") - 1)
      == 0)
    {
      with_configfile = 1;
      arg = skip_to (0, arg);
    }
  
  if (! bootp ())
    {
      if (errnum == ERR_NONE)
	errnum = ERR_DEV_VALUES;

      return 0;
    }

  /* Notify the configuration.  */
  print_network_configuration ();

  /* XXX: this can cause an endless loop, but there is no easy way to
     detect such a loop unfortunately.  */
  if (with_configfile)
    configfile_func (config_file, flags);
  
  return 1;
}

static struct builtin builtin_bootp =
{
  "bootp",
  bootp_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "bootp [--with-configfile]",
  "Initialize a network device via BOOTP. If the option `--with-configfile'"
  " is given, try to load a configuration file specified by the 150 vendor"
  " tag."
};
#endif /* ! GRUB_UTIL */
#endif /* SUPPORT_NETBOOT */

void hexdump(unsigned long ofs,char* buf,int len)
{
  quit_print=0;
  while (len>0)
    {
      int cnt,k;

      grub_printf ("%08X: ", ofs);
      cnt=16;
      if (cnt>len)
        cnt=len;

      for (k=0;k<cnt;k++)
        {	  
          printf("%02X ", (unsigned long)(unsigned char)(buf[k]));
          if ((k!=15) && ((k & 3)==3))
            printf(" ");
        }

      for (;k<16;k++)
        {
          printf("   ");
          if ((k!=15) && ((k & 3)==3))
            printf(" ");
        }

      printf("; ");

      for (k=0;k<cnt;k++)
        printf("%c",(unsigned long)((((unsigned char)buf[k]>=32) && ((unsigned char)buf[k]!=127))?buf[k]:'.'));

      printf("\n");

      if (quit_print)
        break;

      ofs+=16;
      len-=cnt;
    }
}


/* cat */
static int
cat_func (char *arg, int flags)
{
  unsigned char c;
  unsigned char s[128];
  unsigned char r[128];
  unsigned long long Hex = 0;
  unsigned long long len, i, j;
  char *p;
  unsigned long long skip = 0;
  unsigned long long length = 0xffffffffffffffffULL;
  char *locate = 0;
  char *replace = 0;
  unsigned long long locate_align = 1;
  unsigned long long len_s;
  unsigned long long len_r = 0;
  unsigned long long ret = 0;
	unsigned long long number = -1ULL;
  quit_print = 0;

  for (;;)
  {
	if (grub_memcmp (arg, "--hex=", 6) == 0)
	{
		arg += 6;
		safe_parse_maxint (&arg, &Hex);
	}
	else if (grub_memcmp (arg, "--hex", 5) == 0)
	{
		Hex = 1;
	}
	else if (grub_memcmp (arg, "--skip=", 7) == 0)
	{
		arg += 7;
		safe_parse_maxint(&arg,&skip);
	}
	else if (grub_memcmp (arg, "--length=", 9) == 0)
	{
		arg += 9;
		safe_parse_maxint(&arg, &length);
	}
	else if (grub_memcmp (arg, "--locate=", 9) == 0)
	{
		locate = arg += 9;
	}
	else if (grub_memcmp (arg, "--replace=", 10) == 0)
	{
		replace = arg += 10;
	}
	else if (grub_memcmp (arg, "--locate-align=", 15) == 0)
	{
		arg += 15;
		if (! safe_parse_maxint (&p, &locate_align))
			return 0;
		if ((unsigned long)locate_align == 0)
			return ! (errnum = ERR_BAD_ARGUMENT);
	}
	else if (grub_memcmp (arg, "--number=",9) == 0)
	{
		arg += 9;
		safe_parse_maxint (&arg, &number);
	}
	else
		break;
	if (errnum)
		return 0;
	arg = skip_to (0, arg);
  }
  if (! length)
  {
    if (grub_memcmp (arg,"()-1\0",5) == 0 )
    {
        if (! grub_open ("()+1"))
            return 0;
        filesize = filemax*(unsigned long long)part_start;
    } 
    else if (grub_memcmp (arg,"()\0",3) == 0 )
    {
        if (! grub_open ("()+1"))
            return 0;
        filesize = filemax*(unsigned long long)part_length;
    }
    else 
    {
		int no_decompression_bak = no_decompression;
		no_decompression = 1;
       if (! grub_open (arg))
       {
			no_decompression = no_decompression_bak;
            return 0;
       }
       filesize = filemax;
       no_decompression = no_decompression_bak;
    }
	grub_close();
	if (debug > 0)
		grub_printf ("Filesize is 0x%lX\n", (unsigned long long)filesize);
	//ret = filemax;
	//return ret;
	return filesize;
  }

	if (replace)
	{
		p = replace;
		if (*p++ == '*')
		{
			safe_parse_maxint (&p, &len_r);
			errnum = 0;
		}
		if (! len_r)
		{
			if (*replace == '\"')
			{
				for (i = 0; i < 128 && (r[i] = *(++replace)) != '\"'; i++);
			}else{
				for (i = 0; i < 128 && (r[i] = *(replace++)) != ' ' && r[i] != '\t'; i++);
			}
			r[i] = 0;
			replace = (char*)r;
			len_r = parse_string (replace);
		}
		else
		{
			replace = (char*)(unsigned int)len_r;
			len_r = Hex?Hex:8;
		}
		if (len_r == 0 || len_r > 16)
		{
			return ! (errnum = ERR_BAD_ARGUMENT);
		}
	}
  
    if (! grub_open (arg))
    return 0; 
  if (length > filemax)
      length = filemax;
  filepos = skip;
  
  if (locate)
  {
    if (*locate == '\"')
    {
      for (i = 0; i < 128 && (s[i] = *(++locate)) != '\"'; i++);
    }else{
      for (i = 0; i < 128 && (s[i] = *(locate++)) != ' ' && s[i] != '\t'; i++);
    }
    s[i] = 0;
    len_s = parse_string ((char *)s);
    if (len_s == 0 || len_s > 16)
    {
		grub_close();
		return ! (errnum = ERR_BAD_ARGUMENT);
		}
    //j = skip;
    grub_memset ((char *)(SCRATCHADDR), 0, 32);
	c = (unsigned char)(length&0xf);
	for (i = 0,j = skip; ; j += 16)
	{
		len = 0;
		if (j - skip < length)
			len = grub_read ((unsigned long long)(SCRATCHADDR + 16), 16, 0xedde0d90);
		if (len < 16)
			grub_memset ((char *)(SCRATCHADDR + 16 + (unsigned long)len), 0, 16 - len);

		if (j != skip)
		{
			while (i < 16 && (len !=0 || !c || i + len_s <= c))
			{
					unsigned long long k = j - 16 + i;
					if ((locate_align == 1 || ! ((unsigned long)k % (unsigned long)locate_align))
						&& grub_memcmp ((char *)&s, (char *)(SCRATCHADDR + (unsigned long)i), len_s) == 0)
					{
						/* print the address */
						if (!replace || debug > 1)
							grub_printf (" %lX", (unsigned long long)k);
						/* replace strings */
						if (replace)
						{
							unsigned long long filepos_bak = filepos;
							filepos = k;
							/* write len_r bytes at string replace to file!! */
							grub_read ((unsigned long long)(unsigned int)replace,len_r, 0x900ddeed);
							i += len_r;
							filepos = filepos_bak;
						}
						else
							i += len_s;
						ret++;
						Hex = k;
						if (number <= ret)
						{
							len = 0;
							break;
						}
					}
					else
						i++;
			}
			if (len == 0)
			{
				sprintf(ADDR_RET_STR,"0x%x",Hex);
				break;
			}
			i -= 16;
		}
		grub_memmove ((char *)SCRATCHADDR, (char *)(SCRATCHADDR + 16), 16);
	}
  }else if (Hex == (++ret))	/* a trick for (ret = 1, Hex == 1) */
  {
    for (j = skip; j - skip < length && (len = grub_read ((unsigned long long)(unsigned long)&s, 16, 0xedde0d90)); j += 16)
    {
//	if (debug > 0) //chenall 2010-11-12 changed.always show.
		hexdump(j,(char*)&s,(len>length+skip-j)?(length+skip-j):len);
	if (quit_print)
		break;
    }
  }else
    for (j = 0; j < length && grub_read ((unsigned long long)(unsigned long)&c, 1, 0xedde0d90) && c; j++)
    {
#if 1
//	if (debug > 0)//chenall 2010-11-12 changed.always show.
		grub_putchar (c);
#else
	/* Because running "cat" with a binary file can confuse the terminal,
	   print only some characters as they are.  */
	if (grub_isspace (c) || (c >= ' ' && c <= '~'))
		grub_putchar (c);
	else
		grub_putchar ('?');
#endif
	if (quit_print)
		break;
    }
  
  grub_close ();
  return ret;
}

static struct builtin builtin_cat =
{
  "cat",
  cat_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "cat [--hex] [--skip=S] [--length=L] [--locate=STRING] [--locate-align=A] FILE",
  "Print the contents of the file FILE, or print the locations "
  "of the string STRING in FILE."
};


#if ! defined(GRUB_UTIL) && ! defined (STAGE1_5)
/* cdrom */
static int
cdrom_func (char *arg, int flags)
{
  for (;;)
  {
    if (grub_memcmp (arg, "--add-io-ports=", 15) == 0)
      {
	char *p;
	unsigned long long tmp;

	p = arg + 15;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	reg_base_addr_append = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--init", 6) == 0)
      {
	init_atapi();

	if (atapi_dev_count)
	{
	  if (debug > 0)
	    printf("\nFound %d CD-ROM%s. (Note: Further read could fail if the hardware does not\nfully support ATAPI).\n", atapi_dev_count, (atapi_dev_count > 1 ? "s" : ""));
	} else {
	  if (debug > 0)
	    printf("\nNo CD-ROMs found. Perhaps the hardware does not fully support ATAPI. If your\nCD-ROM uses unusual I/O ports, please specify them with \"--add-io-ports=P\".\n");
	}
	return atapi_dev_count;
      }
    else if (grub_memcmp (arg, "--stop", 6) == 0)
      {
	atapi_dev_count = 0;
	return 1;
      }
    else
      return ! (errnum = ERR_BAD_ARGUMENT);
    arg = skip_to (0, arg);
  }
  
  return 1;
}

static struct builtin builtin_cdrom =
{
  "cdrom",
  cdrom_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "cdrom --add-io-ports=P | --init | --stop",
  "Initialise/stop atapi cdroms or set additional I/O ports for a possible atapi cdrom device."
  " The high word of P specifies the base register of the control block registers, and"
  " the low word of P specifies the base register of the command block registers."
};
#endif /* ! defined(GRUB_UTIL) && ! defined (STAGE1_5) */


/* chainloader */
static int
chainloader_func (char *arg, int flags)
{
  char *p;

  int force = 0;
  char *filename;

  int is_pcdos = 0;
  int is_msdos = 0;
  int is_drmk = 0;
  int is_romdos = 0;
  int is_drdos = 0;

  chainloader_load_segment_orig = chainloader_load_segment;
  chainloader_load_offset_orig = chainloader_load_offset;
  chainloader_load_length_orig = chainloader_load_length;
  chainloader_skip_length_orig = chainloader_skip_length;
  chainloader_boot_CS_orig = chainloader_boot_CS;
  chainloader_boot_IP_orig = chainloader_boot_IP;
  chainloader_ebx_orig = chainloader_ebx;
  chainloader_ebx_set_orig = chainloader_ebx_set;
  chainloader_edx_orig = chainloader_edx;
  chainloader_edx_set_orig = chainloader_edx_set;
  chainloader_bx_orig = chainloader_bx;
  chainloader_bx_set_orig = chainloader_bx_set;
  chainloader_cx_orig = chainloader_cx;
  chainloader_cx_set_orig = chainloader_cx_set;
  chainloader_disable_A20_orig = chainloader_disable_A20;
  is_sdi_orig = is_sdi;
  is_raw_orig = is_raw;
  is_isolinux_orig = is_isolinux;
  is_grldr_orig = is_grldr;
  is_io_orig = is_io;
  kernel_type_orig = kernel_type;
  grub_memmove ((char *)chainloader_file_orig, (char *)chainloader_file, sizeof(chainloader_file));

  chainloader_load_segment = -1;//0x0000;
  chainloader_load_offset = -1;//0x7c00;
  chainloader_load_length = -1;//0x200;
  chainloader_skip_length = 0;
  chainloader_boot_CS = -1;//0x0000;
  chainloader_boot_IP = -1;//0x7c00;
  chainloader_ebx = 0;
  chainloader_ebx_set = 0;
  chainloader_edx = 0;
  chainloader_edx_set = 0;
  chainloader_bx = 0;
  chainloader_bx_set = 0;
  chainloader_cx = 0;
  chainloader_cx_set = 0;
  chainloader_disable_A20 = 0;
  is_sdi = is_raw = 0;
  is_isolinux = 0;
  is_grldr = 0;
  is_io = 0;
  kernel_type = KERNEL_TYPE_CHAINLOADER;

  for (;;)
  {
    if (grub_memcmp (arg, "--force", 7) == 0)
      {
	force = 1;
      }
    else if (grub_memcmp (arg, "--pcdos", 7) == 0)
      {
	is_pcdos = 1;
      }
    else if (grub_memcmp (arg, "--msdos", 7) == 0)
      {
	is_msdos = 1;
      }
    else if (grub_memcmp (arg, "--load-segment=", 15) == 0)
      {
	unsigned long long ull;
	p = arg + 15;
	if (! safe_parse_maxint (&p, &ull))
		goto failure;
	if (ull >= 0xA000)
	{
		errnum = ERR_INVALID_LOAD_SEGMENT;
		goto failure;
	}
	chainloader_load_segment = ull;
      }
    else if (grub_memcmp (arg, "--load-offset=", 14) == 0)
      {
	unsigned long long ull;
	p = arg + 14;
	if (! safe_parse_maxint (&p, &ull))
		goto failure;
	if (ull > 0xF800)
	{
		errnum = ERR_INVALID_LOAD_OFFSET;
		goto failure;
	}
	chainloader_load_offset = ull;
      }
    else if (grub_memcmp (arg, "--load-length=", 14) == 0)
      {
	unsigned long long ull;
	p = arg + 14;
	if (! safe_parse_maxint (&p, &ull))
		goto failure;
	if (ull < 512 || ull > 0xA0000)
	{
		errnum = ERR_INVALID_LOAD_LENGTH;
		goto failure;
	}
	chainloader_load_length = ull;
      }
    else if (grub_memcmp (arg, "--skip-length=", 14) == 0)
      {
	unsigned long long ull;
	p = arg + 14;
	if (! safe_parse_maxint (&p, &ull))
		goto failure;
	//if (chainloader_skip_length < 0)
	//{
	//	errnum = ERR_INVALID_SKIP_LENGTH;
	//	goto failure;
	//}
	chainloader_skip_length = ull;
      }
    else if (grub_memcmp (arg, "--boot-cs=", 10) == 0)
      {
	unsigned long long ull;
	p = arg + 10;
	if (! safe_parse_maxint (&p, &ull))
		goto failure;
	if (ull > 0xffff)
	{
		errnum = ERR_INVALID_BOOT_CS;
		goto failure;
	}
	chainloader_boot_CS = ull;
      }
    else if (grub_memcmp (arg, "--boot-ip=", 10) == 0)
      {
	unsigned long long ull;
	p = arg + 10;
	if (! safe_parse_maxint (&p, &ull))
		goto failure;
	if (ull > 0xffff)
	{
		errnum = ERR_INVALID_BOOT_IP;
		goto failure;
	}
	chainloader_boot_IP = ull;
      }
    else if (grub_memcmp (arg, "--ebx=", 6) == 0)
      {
	unsigned long long ull;
	p = arg + 6;
	if (! safe_parse_maxint (&p, &ull))
		goto failure;
	chainloader_ebx = ull;
	chainloader_ebx_set = 1;
      }
    else if (grub_memcmp (arg, "--edx=", 6) == 0)
      {
	unsigned long long ull;
	p = arg + 6;
	if (! safe_parse_maxint (&p, &ull))
		goto failure;
	chainloader_edx = ull;
	chainloader_edx_set = 1;
      }
    else if (grub_memcmp (arg, "--sdi", 5) == 0)
      {
	is_sdi = 1;
      }
    else if (grub_memcmp (arg, "--raw", 5) == 0)
      {
	is_raw = 1;
      }
    else if (grub_memcmp (arg, "--disable-a20", 13) == 0)
      {
	chainloader_disable_A20 = 1;
      }
    else
	break;
    arg = skip_to (0, arg);
  }
  
  if (grub_strlen(saved_dir) + grub_strlen(arg) + 20 >= sizeof(chainloader_file))
    {
	errnum = ERR_WONT_FIT;
	goto failure;
    }

  if (*arg == '/')
  {
    if (saved_partition != 0xFFFFFF)
    {
      if ((saved_partition & 0xFF00) == 0xFF00)
	grub_sprintf (chainloader_file, "(%d,%d)%s%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), saved_dir, arg);
      else
	if ((saved_partition & 0xFF0000) == 0xFF0000)
	  grub_sprintf (chainloader_file, "(%d,%c)%s%s", saved_drive, (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), saved_dir, arg);
	else
	  grub_sprintf (chainloader_file, "(%d,%d,%c)%s%s", saved_drive,  (unsigned long)(unsigned char)(saved_partition>>16), (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), saved_dir, arg);
    }
    else
      grub_sprintf (chainloader_file, "(%d)%s%s", saved_drive, saved_dir, arg);
  }
  else if (*arg == '(' && arg[1] == ')')
  {
    if (saved_partition != 0xFFFFFF)
    {
      if ((saved_partition & 0xFF00) == 0xFF00)
	grub_sprintf (chainloader_file, "(%d,%d)%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), (arg + 2));
      else
	if ((saved_partition & 0xFF0000) == 0xFF0000)
	  grub_sprintf (chainloader_file, "(%d,%c)%s", saved_drive, (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), (arg + 2));
	else
	  grub_sprintf (chainloader_file, "(%d,%d,%c)%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), (arg + 2));
    }
    else
      grub_sprintf (chainloader_file, "(%d)%s", saved_drive, (arg + 2));
  } else if (*arg != '(')
  {
    if (saved_partition != 0xFFFFFF)
    {
      if ((saved_partition & 0xFF00) == 0xFF00)
	grub_sprintf (chainloader_file, "(%d,%d)%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), arg);
      else
	if ((saved_partition & 0xFF0000) == 0xFF0000)
	  grub_sprintf (chainloader_file, "(%d,%c)%s", saved_drive, (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), arg);
	else
	  grub_sprintf (chainloader_file, "(%d,%d,%c)%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), arg);
    }
    else
      grub_sprintf (chainloader_file, "(%d)%s", saved_drive, arg);
  }
  else
	grub_memmove (chainloader_file, arg, 255);
  chainloader_file[255]=0;

  errnum = ERR_NONE;
  
  if ((is_sdi) || (is_raw))
    return 1;

//  if (debug > 0)
//	printf ("Debug: chainloader_func: set_device(%s) ...", arg);

  /* Get the drive number.  */
  filename = set_device (arg);

//  if (debug > 0)
//	/* wipe out debug message. */
//	printf ("\r                                                                             \r");

  if (errnum) {
	/* No device specified. Default to the root device. */
	current_drive = saved_drive;
	//current_partition = saved_partition;
	filename = arg;
	errnum = 0;
  }
  
  if (filename == 0)
	filename = arg;
	
	if (! chainloader_edx_set)
	{
		#ifdef FSYS_FB
	  if (current_drive == 0xFFFF || current_drive == ram_drive)
		{
			chainloader_edx = (saved_drive == FB_DRIVE ? fb_status >> 8 & 0xff :
								saved_drive | ((saved_partition >> 8) & 0xFF00));
			chainloader_edx_set=1;
		}
		else if (current_drive == FB_DRIVE)
		{
			chainloader_edx = fb_status >> 8 & 0xff;
			chainloader_edx_set=1;
		}
		#else
		if (current_drive == 0xFFFF || current_drive == ram_drive)
		{
			chainloader_edx = saved_drive | ((saved_partition >> 8) & 0xFF00);
			chainloader_edx_set=1;
		}
		#endif
	}


#ifndef GRUB_UTIL
  /* check bootable cdrom */
  if (*filename == 0 || *filename == ' ' || *filename == '\t')
  {
	//check_bootable_cdrom (current_drive);
	unsigned long i;
	unsigned long tmp;
	unsigned short tmp1;
	unsigned short tmp2;
//	struct geometry tmp_geom;
	
	tmp = current_drive;
	/* check bootable type of drive (tmp) */

	/* Get the geometry. This ensures that the drive is present.  */
	if (get_diskinfo (tmp, &tmp_geom))
	{
		errnum = ERR_NO_DISK;
		goto failure;
	}

	/* open the drive */
	
	grub_sprintf ((char *)SCRATCHADDR, "(0x%X)+0x%lX", tmp, (unsigned long long)tmp_geom.total_sectors);

	if (! grub_open ((char *)SCRATCHADDR))
		goto failure;
	
	/****************************************/
	/* read the EL Torito Volume Descriptor */
	/****************************************/

	//filemax = 0x12 * 0x800;
	filepos = 0x11 * 0x800;

	if (grub_read ((unsigned long long)SCRATCHADDR, 512, 0xedde0d90) != 512)
		goto failure_exec_format;

	/* check the EL Torito Volume Descriptor */

	if (memcmp ((char *)SCRATCHADDR, "\0CD001\1EL TORITO SPECIFICATION\0", 31))
		goto failure_exec_format;
	
	/* get absolut start sector number for boot catalog */
	tmp = *(unsigned long *)((char *)SCRATCHADDR + 0x47);

	/********************************************/
	/* read the first 512 bytes of Boot Catalog */
	/********************************************/

	//filemax = (tmp + 1) * 0x800;
	filepos = tmp * 0x800;
	
	if (grub_read ((unsigned long long)SCRATCHADDR, 512, 0xedde0d90) != 512)
		goto failure_exec_format;

	/*******************************/
	/* check the Validation Entry  */
	/*******************************/

	tmp1 = 0;
	for (i = 0; i < 16; i++)
	{
		tmp = *(((unsigned short *)(char *)SCRATCHADDR) + i);
		tmp1 += tmp;
		if ((i == 0 && tmp != 1) || (i == 15 && tmp != 0xAA55))
			break;
	}

	if (tmp1 || i < 16)
		goto failure_exec_format;

	/* Boot Record Volume Descriptor format
	 *
	 * Offset     Type	Description
	 * ------     ----	----------------------------------------------
	 *   0	      Byte	Boot Record Indicator, must be 0
	 *
	 *  1-5	   Characters	ISO-9660 Identifier, must be "CD001"
	 *
	 *   6	      Byte	Version of this descriptor, must be 1
	 *
	 *  7-26   Characters	Boot System Identifier, must be
	 *			"EL TORITO SPECIFICATION" padded with 0's.
	 *
	 * 27-46   Characters	Unused, must be 0.
	 *
	 * 47-4A     Dword	LBA of the first sector of Boot Catalog.
	 *
	 * 4B-7FF  Characters	Unused, must be 0.
	 *
	 */

	/* Validation Entry format
	 *
	 * Offset     Type	Description
	 * ------     ----	----------------------------------------------
	 *   0	      Byte	Header ID, must be 01
	 *
	 *   1	      Byte	Platform ID
	 *				0 = 80x86
	 *				1 = Power PC
	 *				2 = Mac
	 *
	 *  2-3	      Word	Reserved, must be 0
	 *
	 *  4-1B   Characters	ID string. This is intended to identify the
	 *			manufacturer/developer of the CD-ROM.
	 *
	 * 1C-1D      Word	Checksum Word. This sum of all the words in
	 *			this record should be 0.
	 *
	 *  1E        Byte	Key byte, must be 55. This value is included
	 *			in the checksum.
	 *
	 *  1F        Byte	Key byte, must be AA. This value is included
	 *			in the checksum.
	 *
	 */

	/* Initial/Default Entry format
	 *
	 * Offset	Type	Description
	 * ------	----	----------------------------------------------
	 *	0	Byte	Boot Indicator. 88=Bootable, 00=Not Bootable
	 *	
	 *	1	Byte	Boot media type.
	 *			0 = No Emulation
	 *			1 = 1.2 meg diskette
	 *			2 = 1.44 meg diskette
	 *			3 = 2.88 meg diskette
	 *			4 = Hard Disk(drive 80)
	 *
	 *	2	Word	Load Segment. 0 for 7C0
	 *
	 *	4	Byte	System Type.
	 *
	 *	5	Byte	Unused, must be 0.
	 *
	 *	6	Word	Sector Count. This is the number of virtual/
	 *			emulated sectors the system will store at Load
	 *			Segment during the initial boot procedure.
	 *
	 *	8	DWord	Load RBA. This is the start address of the
	 *			virtual disk. CD's use Relative/Logical block
	 *			addressing.
	 *
	 *	C	Bytes	Unused, must be 0's.
	 *
	 */

	/* Specification Packet format
	 *
	 * Offset	Type	Description
	 * ------	----	----------------------------------------------
	 *	0	Byte	Packet Size, currently 13
	 *
	 *	1	Byte	Boot media type. This specifies what media the
	 *			boot image is intended to emulate in bits 0-3.
	 *			Bits 6-7 are specific to the type of system.
	 *
	 *			Bits 0-3 count as follows:
	 *			
	 *				0:	No Emulation
	 *				1:	1.2  meg diskette
	 *				2:	1.44 meg diskette
	 *				3:	2.88 meg diskette
	 *				4:	Hard Disk (drive C)
	 *
	 *				5-F:	Reserved, invalid at this time
	 *
	 *			bits 4-5 - Reserved, must be 0
	 *			bit 6 - Image contains an ATAPI driver,
	 *				bytes 8 & 9 refer to IDE interface
	 *			bit 7 - Image contains SCSI drivers,
	 *				bytes 8 & 9 refer to SCSI interface
	 *
	 *	2	Byte	Drive Number. This is the drive number on
	 *			which emulation is being initiated or
	 *			terminated. This must be 0 for a floppy image,
	 *			80 for a bootable hard disk, and 81-FF for a
	 *			"non-bootable" or "no emulation" drive.
	 *
	 *	3	Byte	Controller Index. This specifies the controller
	 *			number of the specified CD drive.
	 *
	 *	4-7	Dword	Logical Block Address for the disk image to be
	 *			emulated.
	 *
	 *	8-9	Word	Device Specification. For SCSI controllers byte
	 *			8 is the LUN and PUN of the CD Drive, byte 9 is
	 *			the Bus number. For IDE controllers the low
	 *			order bit of byte 8 specifies master/slave
	 *			device, 0 = master.
	 *
	 *	A-B	Word	User Buffer Segment. If this field is non-zero
	 *			it specifies the segment of a user supplied 3k
	 *			buffer for caching CD reads. This buffer will
	 *			begin at Segment:0.
	 *
	 *	C-D	Word	Load Segment. This is the load address for the
	 *			initial boot image. If this value is 0, the
	 *			system will use the traditional segment of 7C0.
	 *			If this value is non-zero the system will use
	 *			the specified address. This field is only valid
	 *			for function 4C.
	 *
	 *	E-F	Word	Sector Count. This is the number of virtual
	 *			sectors the system will store at Load Segment
	 *			during the initial boot procedure. This field
	 *			is only valid for function 4C.
	 *
	 *	10	Byte	Bits 0-7 of the cylinder count. This should
	 *			match the value returned in CH when INT 13
	 *			function 08 is invoked.
	 *
	 *	11	Byte	This is the value returned in the CL register
	 *			when INT 13 function 08 is invoked. Bits 0-5
	 *			are the sector count. Bits 6 and 7 are the high
	 *			order 2 bits of the cylinder count.
	 *
	 *	12	Byte	This is the head count, it should match the
	 *			value in DH when INT 13 Function 08 is invoked.
	 *
	 */

	/***********************************/
	/* check the Initial/Default Entry */
	/***********************************/

	/* XXX: assume Initial/Default Entry is always at offset 0x20 */

	tmp = *(unsigned char *)(SCRATCHADDR + 0x20);	/* Boot Indicator. 88=Bootable, 00=Not Bootable */

	if (tmp != 0x88)
		goto failure_exec_format;

	tmp = *((unsigned char *)SCRATCHADDR + 0x25);	/* Unused byte, must be 0. */

	if (tmp)
		goto failure_exec_format;
	
	tmp2 = *((unsigned short *)((unsigned char *)SCRATCHADDR + 0x22));	/* Load Segment */

	if (debug > 0)
		printf ("\nLoad segment: 0x%X\t", (unsigned long)tmp2);

	if (tmp2 == 0)
		tmp2 = 0x7C0;

//	*(unsigned short *)&(chainloader_file[0x0C]) = tmp2;	/* 0C-0D: load segment */
	
	tmp = *((unsigned char *)SCRATCHADDR + 0x24);	/* System Type */

	if (debug > 0)
		printf ("System Type: 0x%X\t", (unsigned long)tmp);
	
	i = *((unsigned short *)((unsigned char *)SCRATCHADDR + 0x26));	/* Sector Count */

	if (debug > 0)
		printf ("Sector Count: 0x%X\n", (unsigned long)i);

//	*(unsigned short *)&(chainloader_file[0x0E]) = i;	/* 0E-0F: sector count in 512-byte sectors */
	
	tmp = *((unsigned long *)((unsigned char *)SCRATCHADDR + 0x28));	/* Load RBA */

	if (debug > 0)
		printf ("Load RBA: 0x%X\t", (unsigned long)tmp);

//	*(unsigned long *)&(chainloader_file[4]) = tmp;		/* 04-07: LBA of the image to be emulated */
	
	tmp1 = *((unsigned char *)SCRATCHADDR + 0x21);	/* Boot media type */

	if (tmp1 > 4)
		goto failure_exec_format;

	if (debug > 0)
		printf ("Boot Type: %s\n", 
			tmp1 == 0 ? "0 = No Emulation" :
			tmp1 == 1 ? "1 = 1.2M floppy"  :
			tmp1 == 2 ? "2 = 1.44M floppy" :
			tmp1 == 3 ? "3 = 2.88M floppy" : "Hard Disk");

//	chainloader_file[1] = tmp1;				/* 01: boot media type */
//	chainloader_file[2] = ((tmp1 == 0) ? current_drive : (tmp1 == 4) ? 0x80 : 0x00);	/* 02: drive number */

	if (tmp1 == 0)	/* no-emulation mode */
	{
		/* No emulation mode uses 2048-byte sector size. */
		if (buf_geom.sector_size != 2048)
			goto failure_exec_format;
		//kernel_type = KERNEL_TYPE_CHAINLOADER;
		sprintf (chainloader_file, "(%d)%d+%d", current_drive, tmp, (unsigned long)((i + 3) / 4));
		chainloader_load_segment = tmp2;//0x0000;
		chainloader_load_offset = 0;//0x7c00;
		chainloader_load_length = i * 512;//0x200;
		chainloader_skip_length = 0;
		chainloader_boot_CS = tmp2;//0x0000;
		chainloader_boot_IP = 0;//0x7c00;
		//chainloader_ebx = 0;
		//chainloader_ebx_set = 0;
		chainloader_edx = current_drive;
		chainloader_edx_set = 1;
		//chainloader_disable_A20 = 0;
		
		/* update lba_cd_boot if there are any maps */
		if (! unset_int13_handler (0))	/* a successful unset */
		{
			if (drive_map_slot_empty (bios_drive_map[0]))
			    if (atapi_dev_count == 0)
			    {
				errnum = ERR_NO_DRIVE_MAPPED;
				goto failure_exec_format;
			    }
			lba_cd_boot = tmp;

			set_int13_handler (bios_drive_map);
		}

		/* needn't clear disk buffer */
		/* buf_drive = -1; */
		/* buf_track = -1; */

		return ! (errnum = 0);
	}

	/* floppy or hard drive emulation. LBA=tmp */

	/*******************************************************/
	/* read the first 512 bytes of the image at 0000:7C00  */
	/*******************************************************/

	filepos = tmp * 0x800;
	
	/* we cannot use SCRATCHADDR because map_func and geometry_func may
	 * use it. So we use 1 sector below 0x2B0000 instead. */
	if (grub_read ((unsigned long long)(HMA_ADDR - 0x200), 512, 0xedde0d90) != 512)
		goto failure_exec_format;

	{
		unsigned long heads = 0;
		unsigned long sects = 0;

		if (tmp1 == 4)
		{
			int err;
			if ((err = probe_mbr ((struct master_and_dos_boot_sector *)(HMA_ADDR - 0x200), 0, 1, 0)))
			{
				if (debug > 0)
					printf ("Warning! Partition table of HD image is faulty(err=%d).\n", err);
				sects = (*(unsigned char *)(HMA_ADDR - 0x200 + 0x1C4)) & 0x3F;
				probed_total_sectors = *(unsigned long *)(HMA_ADDR - 0x200 + 0x1CA);
				if (sects < 2 || probed_total_sectors < 5)
				{
					errnum = ERR_BAD_PART_TABLE;
					goto failure_exec_format;
				}
				heads = *(unsigned char *)(HMA_ADDR - 0x200 + 0x1C3);
				//if (heads < 255)
					heads++;
				if (debug > 0)
					printf ("Use Heads=%d, SectorsPerTrack=%d, TotalSectors=%ld for HD emulation.\n", heads, sects, (unsigned long long)probed_total_sectors);
			}
		} else {
			if (probe_bpb((struct master_and_dos_boot_sector *)(HMA_ADDR - 0x200)))
			{
				probed_total_sectors = (tmp1 == 1 ? 2400 : tmp1 == 2 ? 2880 : 5760);
			}
		}

		/* map the image */
		sprintf (chainloader_file, "--read-only --heads=%d --sectors-per-track=%d (%d)%d+%d (%d)",
				(unsigned long)heads,
				(unsigned long)sects,
				(unsigned long)current_drive,
				(unsigned long)tmp,
				(unsigned long)((probed_total_sectors + 3) / 4), //0x3FFFFFFF,
				(unsigned long)((tmp1 == 4) ? 0x80 : 0x00));
	}

	tmp = ( (tmp1 == 4) ? (*(char *)0x475) : ((*(char*)0x410) & 1) );

	/* file must be closed before calling map_func() */
	grub_close ();

	disable_map_info = 1;
	map_func (chainloader_file, flags);
	disable_map_info = 0;

	if (errnum)
		goto failure;

	/* move the former fd0 or hd0 up. */
	if (tmp)
	{
		sprintf (chainloader_file, "(%d) (%d)", (unsigned long)((tmp1 == 4) ? 0x80 : 0x00), (unsigned long)((tmp1 == 4) ? (0x80 + tmp) : 0x01));

		map_func (chainloader_file, flags);

		if (errnum)
		{
			if (debug > 0)
			{
				printf ("Failed 'map %s'(error=%d). But you may ignore it and continue to 'boot' the CD.\n", chainloader_file, errnum);
			}
			errnum = 0;
		}
	}

	/* rehook */
	unset_int13_handler (0);
	if (drive_map_slot_empty (bios_drive_map[0]))
	    if (atapi_dev_count == 0)
	    {
		errnum = ERR_NO_DRIVE_MAPPED;
		goto failure;
	    }
	set_int13_handler (bios_drive_map);
	buf_drive = -1;
	buf_track = -1;

        grub_memmove ((char *)0x7C00, (char *)(HMA_ADDR - 0x200), 512);
	sprintf (chainloader_file, "(%d)+1", (unsigned long)((tmp1 == 4) ? 0x80 : 0x00));

	chainloader_load_segment = 0;//tmp2;
	chainloader_load_offset = 0x7c00;
	chainloader_load_length = 512;
	chainloader_skip_length = 0;
	chainloader_boot_CS = 0;//tmp2;
	chainloader_boot_IP = 0x7c00;
	//chainloader_ebx = 0;
	chainloader_ebx_set = 0;
	chainloader_edx_set = 0;
	chainloader_bx_set = 0;
	chainloader_cx_set = 0;
	//chainloader_disable_A20 = 0;
	saved_drive = ((tmp1 == 4) ? 0x80 : 0x00);
	chainloader_edx = saved_drive;
	saved_partition = ((tmp1 == 4) ? 0x00FFFF : 0xFFFFFF);
	return ! (errnum = 0);

  }
#endif /* ! GRUB_UTIL */

  /* Open the file.  */
  grub_open (arg);

  if (errnum)
	goto failure;

  /* Read the first block.  */
  {
    unsigned long len;
    len = grub_read ((unsigned long long) SCRATCHADDR, 512, 0xedde0d90);

    if (len != 512)
	goto failure_exec_format;
  }

  if (chainloader_skip_length > filemax)
  {
	errnum = ERR_INVALID_SKIP_LENGTH;
	goto failure_exec_format;
  }
  
  if (*((long *)SCRATCHADDR) == 0x49445324 /* $SDI */)
    {
	is_sdi = 1;
	if (debug > 0)
	    grub_printf("SDI signature: %s\n", (char *)(SCRATCHADDR));
    }
  else
  if ((*(long long *)SCRATCHADDR | 0xFF00LL) == 0x4749464E4F43FFEBLL && filemax > 0x4000)
    {

	if (chainloader_load_segment == -1)
		chainloader_load_segment = 0x0060;
drdos:
	if (chainloader_load_offset == -1)
		chainloader_load_offset = 0;
	if (chainloader_load_length == -1)
		chainloader_load_length = filemax;
	if (! chainloader_ebx_set)
	{
		chainloader_ebx = current_drive | ((current_partition >> 8) & 0xFF00);
		chainloader_ebx_set = 1;
	}

	grub_close ();

	/* FIXME: Where should the BPB be placed for FreeDOS's KERNEL.SYS?
	 *	  In the current implementation it is placed at 0000:7C00
	 *	  but has no effect, since the KERNEL.SYS body will later
	 *	  overwrite 0000:7C00 on issuing the `boot' command.
	 */

	if (((current_partition >> 8) & 0xFF00) == 0xFF00) /* check if partition number == 0xFF */
		grub_sprintf ((char *)SCRATCHADDR, "(%d)+1", (unsigned long)(unsigned char)current_drive);
	else
		grub_sprintf ((char *)SCRATCHADDR, "(%d,%d)+1", (unsigned long)(unsigned char)current_drive, (unsigned long)(unsigned char)(current_partition >> 16));

	if (! grub_open ((char *)SCRATCHADDR))
		goto failure;
	
	/* Read the boot sector of the partition onto 0000:7C00  */
	if (grub_read ((unsigned long long) SCRATCHADDR, 512, 0xedde0d90) != 512)
		goto failure_exec_format;

	/* modify the hidden sectors */
	/* FIXME: Does the boot drive number also need modifying? */
	
	if (*((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS)))
	    *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS)) = (unsigned long)part_start;

	if (is_pcdos||is_msdos) {
		/* Set data area location to *0x7BFC, root directory entry address to *0x7BF8, root directory entry size to *0x7BDE */
		if (*((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_FAT_NAME)) == 0x31544146) { /* FAT1(2|6) */
			*(unsigned long *)0x7BF8 = *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_RESERVED_SECTORS))
	    								+ *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_SECTORS_PER_FAT)) * 2; // root directory entry address
			*(unsigned short *)0x7BDE = *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_MAX_ROOT_ENTRIES)) * 32 / *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_BYTES_PER_SECTOR)); // root directory entry size
			*(unsigned long *)0x7BFC = (unsigned long)part_start
									+ *(unsigned long *)0x7BF8
	    							+ *(unsigned short *)0x7BDE;
		}
		else if (*((unsigned long *)(SCRATCHADDR + BOOTSEC_BPB_FAT32_NAME)) == 0x33544146) { /* FAT32 */
			*(unsigned long *)0x7BF8 = *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_RESERVED_SECTORS))
	    								+ *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_FAT32_SECTORS_PER_FAT)) * 2
	    								+ (*((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_FAT32_ROOT)) - 2) * *((unsigned char *) (SCRATCHADDR + BOOTSEC_BPB_SECTORS_PER_CLUSTER)); // root directory entry address
			*(unsigned short *)0x7BDE = *((unsigned char *) (SCRATCHADDR + BOOTSEC_BPB_SECTORS_PER_CLUSTER)); // root directory entry size = 1 cluster
	    	*(unsigned long *)0x7BFC = (unsigned long)part_start + *(unsigned long *)0x7BF8; // root directory entry is in cluster 2 !!
		}
		else {
			grub_printf("Error: Not FAT partition.");
			goto failure_exec_format;
		}

		/* copy directory entry of boot files to 0x500 */
		grub_close ();
		if (((current_partition >> 8) & 0xFF00) == 0xFF00) /* check if partition number == 0xFF */
			grub_sprintf ((char *)(HMA_ADDR - 0x20), "(%d)%d+%d", (unsigned long)(unsigned char)current_drive, *(unsigned long *)0x7BF8, *(unsigned short *)0x7BDE * *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_BYTES_PER_SECTOR)) / 512);
		else
			grub_sprintf ((char *)(HMA_ADDR - 0x20), "(%d,%d)%d+%d", (unsigned long)(unsigned char)current_drive, (unsigned long)(unsigned char)(current_partition >> 16), *(unsigned long *)0x7BF8, *(unsigned short *)0x7BDE * *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_BYTES_PER_SECTOR)) / 512);

		grub_open ((char *)(HMA_ADDR - 0x20));
		grub_read ((unsigned long long)(HMA_ADDR - 0x10000), (*(unsigned short *)0x7BD8 = *(unsigned short *)0x7BDE * *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_BYTES_PER_SECTOR))), 0xedde0d90);

		/* read 1st FAT(first 32K) to HMA_ADDR - 0x8000 */
		if (*((unsigned long *)(SCRATCHADDR + BOOTSEC_BPB_FAT32_NAME)) == 0x33544146) { /* FAT32 */
			*(unsigned short *)0x7BDC = 0; // read root directory cluster count
			*(unsigned long *)0x7BD0 = *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_FAT32_ROOT)); // last read root directory cluster
			if (((current_partition >> 8) & 0xFF00) == 0xFF00) /* check if partition number == 0xFF */
				grub_sprintf ((char *)(HMA_ADDR - 0x20), "(%d)%d+%d", (unsigned long)(unsigned char)current_drive, *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_RESERVED_SECTORS)), *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_FAT32_SECTORS_PER_FAT)));
			else
				grub_sprintf ((char *)(HMA_ADDR - 0x20), "(%d,%d)%d+%d", (unsigned long)(unsigned char)current_drive, (unsigned long)(unsigned char)(current_partition >> 16), *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_RESERVED_SECTORS)), *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_FAT32_SECTORS_PER_FAT)));

			grub_open ((char *)(HMA_ADDR - 0x20)); /* read 1st FAT table(first 0x8000 bytes only) */
			grub_read ((unsigned long long)(HMA_ADDR - 0x8000), (*((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_FAT32_SECTORS_PER_FAT)) > 40 ? 40 : *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_FAT32_SECTORS_PER_FAT))) * *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_BYTES_PER_SECTOR)), 0xedde0d90);
readroot:
			if ( (HMA_ADDR - 0x10000 + (1+*(unsigned short *)0x7BDC) * *(unsigned short *)0x7BD8) < HMA_ADDR && /* don't overrun */
				*(unsigned long *)0x7BD0 * sizeof(unsigned long) < 0x8000 &&
				(*(unsigned long *)0x7BD0 = *(unsigned long *)(HMA_ADDR - 0x8000 + *(unsigned long *)0x7BD0 * sizeof(unsigned long))) != 0xFFFFFFFF ) { /* root cluster count > 1 */
				*(unsigned long *)0x7BF8 = *((unsigned short *) (SCRATCHADDR + BOOTSEC_BPB_RESERVED_SECTORS))
		    								+ *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_FAT32_SECTORS_PER_FAT)) * 2
		    								+ (*(unsigned long *)0x7BD0 - 2) * *((unsigned char *) (SCRATCHADDR + BOOTSEC_BPB_SECTORS_PER_CLUSTER)); // root directory entry address
				if (((current_partition >> 8) & 0xFF00) == 0xFF00) /* check if partition number == 0xFF */
					grub_sprintf ((char *)(HMA_ADDR - 0x20), "(%d)%d+%d", (unsigned long)(unsigned char)current_drive, *(unsigned long *)0x7BF8, *(unsigned short *)0x7BDE);
				else
					grub_sprintf ((char *)(HMA_ADDR - 0x20), "(%d,%d)%d+%d", (unsigned long)(unsigned char)current_drive, (unsigned long)(unsigned char)(current_partition >> 16), *(unsigned long *)0x7BF8, *(unsigned short *)0x7BDE);
				grub_open ((char *)(HMA_ADDR - 0x20));
				grub_read ((unsigned long long)(HMA_ADDR - 0x10000 + (1+*(unsigned short *)0x7BDC) * *(unsigned short *)0x7BD8), *(unsigned short *)0x7BD8, 0xedde0d90);
				++(*(unsigned short *)0x7BDC);
				goto readroot;
			}
		}

		for ( *(unsigned long *)0x7BF4 = HMA_ADDR - 0x10000, *(unsigned long *)0x7BF0 = 0x500; *(char *)(*(unsigned long *)0x7BF4) && *(unsigned long *)0x7BF4 < HMA_ADDR; *(unsigned long *)0x7BF4 += 32) {
			if (*(long long *)(*(unsigned long *)0x7BF4) == *(long long *)0x7BE0) { /* BIO */
				grub_memmove((void *)*(unsigned long *)0x7BF0, (void *)*(unsigned long *)0x7BF4, 32);
				*(unsigned long *)0x7BF0 += 32;
			}
		}
		for ( *(unsigned long *)0x7BF4 = HMA_ADDR - 0x10000; *(char *)(*(unsigned long *)0x7BF4) && *(unsigned long *)0x7BF4 < HMA_ADDR; *(unsigned long *)0x7BF4 += 32) {
			if (*(long long *)(*(unsigned long *)0x7BF4) == *(long long *)0x7BE8) { /* DOS */
				grub_memmove((void *)*(unsigned long *)0x7BF0, (void *)*(unsigned long *)0x7BF4, 32);
				*(unsigned long *)0x7BF0 += 32;
			}
		}
		if (! chainloader_bx_set)
		{
			chainloader_bx = *(unsigned long *)0x7BFC;
			chainloader_bx_set = 1;
		}
		if (! chainloader_cx_set)
		{
			chainloader_cx = *((unsigned char *) (SCRATCHADDR + BOOTSEC_BPB_MEDIA_DESCRIPTOR)) << 8;
			chainloader_cx_set = 1;
		}
	}

	if (debug > 0)
	  grub_printf("Will boot %s from drive=0x%x, partition=0x%x(hidden sectors=0x%lx)\n",
			(is_pcdos ? "PC DOS" : (is_msdos ? "MS-DOS" : (is_drdos ? "DR-DOS" : (is_romdos ? "ROM-DOS" : (is_drmk ? "DRMK" : "FreeDOS"))))), current_drive, (unsigned long)(unsigned char)(current_partition >> 16), (unsigned long long)part_start);
    }
  else if ((*((long long *)SCRATCHADDR) == 0x501E0100122E802ELL && (is_drdos = 1)) /* packed with pack101 */ || 
  	  is_pcdos || is_msdos ||
  	  (*((long long *)(SCRATCHADDR)) == 0x0A079047EBLL && (is_pcdos = 1)) /* PC-DOS 7.1 */ || 
  	  (*((long long *)(SCRATCHADDR)) == 0x070135E9LL && (is_pcdos = 1)) /* PC-DOS 2000 */ || 
  	  (*((unsigned long *)(SCRATCHADDR)) == 0x060135E9 && (is_msdos = 1)) /* MS-DOS 6.x */ || 
  	  (*((long long *)(SCRATCHADDR+6)) == 0x646F4D206C616552LL && (is_drmk = 1)) /* DRMK */ || 
  	  ((*(long long *)SCRATCHADDR | 0xFFFF02LL) == 0x4F43000000FFFFEBLL && (*(((long long *)SCRATCHADDR)+1) == 0x706D6F435141504DLL) && (is_drdos = 1)))   /* DR-DOS */
    {
	/* contributor: Roy <roytam%gmail%com> */
	if (chainloader_load_segment == -1)
		chainloader_load_segment = 0x0070;
	if ((is_pcdos || is_drmk || is_msdos) && chainloader_load_length == -1)
		chainloader_load_length = filemax < 0x7400 ? filemax : 0x7400;
	if (is_pcdos) {
		*(long long *)0x7BE0 = 0x20204F49424D4249LL; /* IBMBIO.COM */
		*(long long *)0x7BE8 = 0x2020534F444D4249LL; /* IBMDOS.COM */
	}
	else if (is_msdos) {
		*(long long *)0x7BE0 = 0x2020202020204F49LL; /* IO.SYS */
		*(long long *)0x7BE8 = 0x202020534F44534DLL; /* MSDOS.SYS */
	}

	goto drdos;
    }
  else
  if (filemax >= 0x40000 && *((unsigned short *) (SCRATCHADDR)) == 0x5A4D && // MZ header
  	  *((unsigned short *) (SCRATCHADDR + 0x80)) == 0x4550 && // PE header
  	  *((unsigned short *) (SCRATCHADDR + 0xDC)) == 0x1 //&& // PE subsystem
//  	  (*((unsigned long *) (SCRATCHADDR + 0xA8))) == 0x1000 && // Entry address
//  	  (*((unsigned long *) (SCRATCHADDR + 0xB4))) == 0x8000 // Base address
  	 )
    {
	if (chainloader_load_segment == -1)
		chainloader_load_segment = 0;
	if (chainloader_load_offset == -1)
		chainloader_load_offset = (*((unsigned long *) (SCRATCHADDR + 0xB4)));
	if (chainloader_load_length == -1)
		chainloader_load_length = filemax;
	if (chainloader_boot_IP == -1)
		chainloader_boot_IP = (*((unsigned long *) (SCRATCHADDR + 0xB4))) + (*((unsigned long *) (SCRATCHADDR + 0xA8)));
	if (! chainloader_edx_set)
	{
		chainloader_edx = current_drive | ((current_partition >> 8) & 0xFF00);
		chainloader_edx_set = 1;
	}
	grub_close ();
	
	if (debug > 0)
	  grub_printf("Will boot FreeLDR from drive=0x%x, partition=0x%x(hidden sectors=0x%lx)\n", current_drive, (unsigned long)(unsigned char)(current_partition >> 16), (unsigned long long)part_start);
    }
  else
  if (*(short *)SCRATCHADDR == 0x5A4D && filemax > 0x10000 &&
       *((unsigned short *) (SCRATCHADDR + BOOTSEC_SIG_OFFSET)) == 0
      	/* && (*(long *)(SCRATCHADDR + 0xA2)) == 0 */ )
    {

      int err;
      
      /* Read the second sector.  */
      if (grub_read ((unsigned long long) SCRATCHADDR, 512, 0xedde0d90) != 512)
		goto failure_exec_format;

      err = (*(short *)SCRATCHADDR != 0x4A42);

      filepos += 0x200;	/* skip the third sector */
      
      /* Read the fourth sector.  */
      if (grub_read ((unsigned long long) SCRATCHADDR, 512, 0xedde0d90) != 512)
		goto failure_exec_format;

      err |= (*((unsigned short *) (SCRATCHADDR + BOOTSEC_SIG_OFFSET)) != 0x534D);

      /* Read the fifth sector.
       * check the compress signature "CM" of IO.SYS of WinME */
      if (grub_read ((unsigned long long) SCRATCHADDR, 512, 0xedde0d90) != 512)
		goto failure_exec_format;

      if (! err)
      {
	if (chainloader_load_segment == -1)
		chainloader_load_segment = 0x0070;
	if (chainloader_load_offset == -1)
		chainloader_load_offset = 0;
	if (chainloader_load_length == -1)
		chainloader_load_length = filemax;
	if (chainloader_skip_length == 0)
		chainloader_skip_length = 0x0800;

	/* WinME support by bean. Thanks! */

	// Input parameter for SYSINIT
	// BX,AX: Start sector for the data area of FAT. It doesn't needs to
	//        be set. However, we should at least clear BX, otherwise,
	//        it would have some minor problem when booting WinME.
	// DI:    Length of the boot code, don't need to be set.
	// BP:    0x7C00, boot sector pointer, don't need to be set.
	// DH:    Media ID (BS[0x15]) , 0xF0 for floppy, 0xF8 for harddisk
	// DL:    Drive number (BS[0x40] for FAT32)

	if (! chainloader_edx_set)
	{
		chainloader_edx = current_drive | ((current_partition >> 8) & 0xFF00);
		chainloader_edx_set = 1;
	}
	if (! chainloader_ebx_set)
	{
		chainloader_ebx = 0;	// clear BX for WinME
		chainloader_ebx_set = 1;
	}
	    
	/* save partition number to the high word */
	chainloader_edx = (chainloader_edx & 0xFF00FFFF) | ((chainloader_edx & 0xFF00) << 8);
	
	// set media descriptor in DH for WinME
	chainloader_edx = (chainloader_edx & 0xFFFF00FF) | 0xF000 | ((chainloader_edx & 0x80) << 4);
		
	grub_close ();
	
	/* FIXME: Where should the BPB be placed for MS-DOS's IO.SYS?
	 *	  In the current implementation it is placed at 0000:7C00
	 *	  but has no effect, since the IO.SYS body will later
	 *	  overwrite 0000:7C00 on issuing the `boot' command.
	 */
	
	if ((chainloader_edx & 0xFF0000) == 0xFF0000) /* check if partition number == 0xFF */
		grub_sprintf ((char *)SCRATCHADDR, "(%d)+1", (unsigned long)(unsigned char)chainloader_edx);
	else
		grub_sprintf ((char *)SCRATCHADDR, "(%d,%d)+1", (unsigned long)(unsigned char)chainloader_edx, (unsigned long)(unsigned char)(chainloader_edx >> 16));

	if (! grub_open ((char *)SCRATCHADDR))
		goto failure;
	
	/* Read the boot sector of the partition onto 0000:7C00  */
	if (grub_read ((unsigned long long) SCRATCHADDR, 512, 0xedde0d90) != 512)
		goto failure_exec_format;

	is_io = 1;
	/* modify the hidden sectors */
	/* FIXME: Does the boot drive number also need modifying? */
	
	if (*((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS)))
	    *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS)) = (unsigned long)part_start;
	if (debug > 0)
	  grub_printf("Will boot MS-DOS %c.x from drive=0x%x, partition=0x%x(hidden sectors=0x%lx)\n",
			(unsigned long)(((*(unsigned short *) SCRATCHADDR) == 0x4D43)? '8' : '7'),
			(unsigned long)current_drive, (unsigned long)(unsigned char)(current_partition >> 16), (unsigned long long)part_start);
      }
    }
  else
  if (((*(long *)SCRATCHADDR) & 0x00FF00FF) == 0x000100E9 && filemax > 0x30000 &&
       (*((unsigned short *) (SCRATCHADDR + BOOTSEC_SIG_OFFSET)) != BOOTSEC_SIGNATURE))
    {
	if (chainloader_load_segment == -1)
		chainloader_load_segment = 0x2000;
	if (chainloader_load_offset == -1)
		chainloader_load_offset = 0;
	if (chainloader_load_length == -1)
		chainloader_load_length = filemax;
	if (! chainloader_edx_set)
	{
		chainloader_edx = current_drive | ((current_partition >> 8) & 0xFF00);
		chainloader_edx_set = 1;
	}
	    
	grub_close ();
	
	if ((chainloader_edx & 0xFF00) == 0xFF00)
		grub_sprintf ((char *)SCRATCHADDR, "(%d)+1", (unsigned long)(unsigned char)chainloader_edx);
	else
		grub_sprintf ((char *)SCRATCHADDR, "(%d,%d)+1", (unsigned long)(unsigned char)chainloader_edx, (unsigned long)(unsigned char)(chainloader_edx >> 8));

	if (! grub_open ((char *)SCRATCHADDR))
		goto failure;
	
	/* Read the boot sector of the partition onto 0000:7C00  */
	if (grub_read ((unsigned long long) SCRATCHADDR, SECTOR_SIZE, 0xedde0d90) != SECTOR_SIZE)
		goto failure_exec_format;

	/* modify the hidden sectors */
	/* FIXME: Does the boot drive number also need modifying? */
	
	if (*((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS)))
	    *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS)) = (unsigned long)part_start;
	if (debug > 0)
	  grub_printf("Will boot NTLDR from drive=0x%x, partition=0x%x(hidden sectors=0x%lx)\n", current_drive, (unsigned long)(unsigned char)(current_partition >> 16), (unsigned long long)part_start);
    }
  else
  if (filemax >= 0x4000 && (*(short *)SCRATCHADDR) == 0x3EEB && //(*(long *)(SCRATCHADDR + 0x40)) == 0x5B0000E8 &&
       (*((unsigned short *) (SCRATCHADDR + BOOTSEC_SIG_OFFSET)) != BOOTSEC_SIGNATURE))
    {
	char tmp_buf[16];

	filepos = 0x1FF8;	/* grldr signature, pre_stage2, etc */
      
	if (grub_read ((unsigned long long)(unsigned long)tmp_buf, 16, 0xedde0d90) != 16)
		goto failure_exec_format;

	filepos = 0x200;
	if (*(short *)tmp_buf < 0x40 || *(short *)tmp_buf > 0x1B8)
		goto check_isolinux;
	if (*(long *)(tmp_buf + 8) != 0x008270EA)
		goto check_isolinux;
	if (*(long *)(SCRATCHADDR + *(short *)tmp_buf) != *(long *)(tmp_buf + 4))
		goto check_isolinux;
	if ((*(long long *)(void *)((int)*(short *)tmp_buf + (int)SCRATCHADDR - 5) & 0xFFFFFFFFFFLL) != 0xB8661FFCBBLL)
		goto check_isolinux;

//	if (chainloader_load_segment == -1)
//		chainloader_load_segment = 0x2000;
//	if (chainloader_load_offset == -1)
//		chainloader_load_offset = 0;
	if (chainloader_load_length == -1)
		chainloader_load_length = filemax;
	if (! chainloader_edx_set)
	{
		chainloader_edx = current_drive | ((current_partition >> 8) & 0xFF00);
		chainloader_edx_set = 1;
	}
	is_grldr = 1;
	    
	grub_close ();
	
	if (debug > 0)
	  grub_printf("Will boot GRLDR from drive=0x%x, partition=0x%x(hidden sectors=0x%lx)\n", current_drive, (unsigned long)(unsigned char)(current_partition >> 16), (unsigned long long)part_start);
    }
  else
    {

check_isolinux:

	if (filemax < 0x800)
		goto check_signature;

	/* Read the 2nd, 3rd and 4th sectors. */

	/**********************************************/
	/**** 4 sectors at SCRATCHADDR are used!!! ****/
	/**********************************************/

	filepos = 0x200;

	if (grub_read ((unsigned long long) SCRATCHADDR+0x200, 0x600, 0xedde0d90) != 0x600)
		goto check_signature;

	if (filemax >= 0x4000 && ((*(long *)SCRATCHADDR) & 0x80FFFFFF)== 0xEB5A4D && (*(long *)(SCRATCHADDR + 0x202)) == 0x53726448 &&
	       (*((unsigned short *) (SCRATCHADDR + BOOTSEC_SIG_OFFSET)) == BOOTSEC_SIGNATURE) &&
		(*(unsigned char *)(SCRATCHADDR + 0x200)) == 0xEB)	/* GRUB.EXE */
	{
		if (chainloader_load_segment == -1)
			chainloader_load_segment = 0x1000;	/* use address != 0x7C00, so that PXE is automatically disabled. */
		if (chainloader_load_offset == -1)
			chainloader_load_offset = 0;
		if (chainloader_boot_CS == -1)
			chainloader_boot_CS = chainloader_load_segment;
		if (chainloader_boot_IP == -1)
			chainloader_boot_IP = chainloader_load_offset + 2;	/* skip "MZ" */
		if (chainloader_load_length == -1)
			chainloader_load_length = filemax;
		if (! chainloader_edx_set)
		{
			chainloader_edx = current_drive | ((current_partition >> 8) & 0xFF00);
			chainloader_edx_set = 1;
		}
	    
		grub_close ();
	
		if (debug > 0)
		  grub_printf("Will boot GRUB.EXE from drive=0x%x, partition=0x%x(hidden sectors=0x%lx)\n", current_drive, (unsigned long)(unsigned char)(current_partition >> 16), (unsigned long long)part_start);
	}
	else
	if ((*(long long *)(SCRATCHADDR + 0x200)) == 0xCB5052C03342CA8CLL && (*(long *)(SCRATCHADDR + 0x208) == 0x5441464B))   /* ROM-DOS */
	{
		/* contributor: Roy <roytam%gmail%com> */
		if (chainloader_load_segment == -1)
			chainloader_load_segment = 0x1000;
		if (chainloader_load_offset == -1)
			chainloader_load_offset = 0;
		if (chainloader_load_length == -1)
			chainloader_load_length = filemax;
		if (chainloader_skip_length == 0)
			chainloader_skip_length = 0x0200;
		*(unsigned long *)0x84 = current_drive | 0xFFFF0000;
		is_romdos = 1;
		goto drdos;
	}
      else
      if (((*(long long *)SCRATCHADDR) & 0xFFFFFFFFFF00FFFFLL) == 0x909000007C00EAFALL && filemax > 0x2000 && filemax < 0x20000) /* ISOLINUX */
	{
	    for (p = (char *)(SCRATCHADDR + 0x40); p < (char *)(SCRATCHADDR + 0x7F3); p++)
	    {
		if (	*(unsigned long *)p == 0xBB0201B8 &&
			*(unsigned long *)(p - 4) == 0 &&
			*(unsigned long *)(p + 4) == 0x06B97C00 &&
			*(unsigned long *)(p + 8) == 0x0180BA00 &&
			*(unsigned short *)(p + 12) == 0x9A9C)
		{
			goto isolinux_ok;
		}
	    }
		/* comment out old code */
		//for (p = (char *)(SCRATCHADDR + 0x40); p < (char *)(SCRATCHADDR + 0x140); p++)
		//{
		//	if (*(unsigned long *)p == 0xD08EC031 &&
		//		*(unsigned short *)(p - 10) == 0x892E &&
		//		*(unsigned short *)(p - 5) == 0x8C2E &&
		//		*(unsigned char *)(p + 4) == 0xBC &&
		//		*(unsigned char *)(p - 8) == 0x26 &&
		//		*(unsigned char *)(p - 3) == 0x16)
		//	{
		//		goto isolinux_ok;
		//	}
		//}
		goto check_signature;	/* it is not isolinux. */
isolinux_ok:
	    if (buf_geom.sector_size != 2048)
	    {
		if (debug > 0)
			printf ("\nCannot chainload ISOLINUX from a non-CDROM device.\n");
		goto failure_exec_format;
	    }

	    chainloader_load_segment = 0;
	    chainloader_load_offset = 0x7c00;
	    chainloader_load_length = filemax;
	    chainloader_edx = current_drive;
	    chainloader_edx_set = 1;

	    is_isolinux = 1;
	}
      else
	{
check_signature:
	    /* If not loading it forcibly, check for the signature.  */
	    if (! force
		&& (*((unsigned short *) (SCRATCHADDR + BOOTSEC_SIG_OFFSET))
		!= BOOTSEC_SIGNATURE))
		goto failure_exec_format;
	}
    }

  grub_close ();

  /* if BPB exists, we can reliablly modify the hidden sectors. */
  if (! probe_bpb((struct master_and_dos_boot_sector *)SCRATCHADDR))
    if (*((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS)))
        *((unsigned long *) (SCRATCHADDR + BOOTSEC_BPB_HIDDEN_SECTORS))
          = (unsigned long)part_start;

  if (chainloader_load_length == -1)
	  chainloader_load_length = filemax;
  if (chainloader_load_length > 0xA0000)
	  chainloader_load_length = 0xA0000;
  
  grub_memmove ((char *)0x7C00, (char *)SCRATCHADDR, 512);
  errnum = ERR_NONE;
  
  return 1;

failure_exec_format:

  grub_close ();

  if (errnum == ERR_NONE)
	errnum = ERR_EXEC_FORMAT;

failure:

  chainloader_load_segment = chainloader_load_segment_orig;
  chainloader_load_offset = chainloader_load_offset_orig;
  chainloader_load_length = chainloader_load_length_orig;
  chainloader_skip_length = chainloader_skip_length_orig;
  chainloader_boot_CS = chainloader_boot_CS_orig;
  chainloader_boot_IP = chainloader_boot_IP_orig;
  chainloader_ebx = chainloader_ebx_orig;
  chainloader_ebx_set = chainloader_ebx_set_orig;
  chainloader_edx = chainloader_edx_orig;
  chainloader_edx_set = chainloader_edx_set_orig;
  chainloader_disable_A20 = chainloader_disable_A20_orig;
  is_sdi = is_sdi_orig;
  is_raw = is_raw_orig;
  is_isolinux = is_isolinux_orig;
  is_grldr = is_grldr_orig;
  is_io = is_io_orig;
  kernel_type = kernel_type_orig;
  force = errnum;	/* backup the errnum */
  grub_memmove ((char *)chainloader_file, (char *)chainloader_file_orig, sizeof(chainloader_file));
  errnum = force;	/* restore the errnum */
  return 0;		/* return failure */

}

static struct builtin builtin_chainloader =
{
  "chainloader",
  chainloader_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "chainloader [--force] [--load-segment=LS] [--load-offset=LO]"
  "\n[--load-length=LL] [--skip-length=SL] [--boot-cs=CS] [--boot-ip=IP]"
  "\n[--ebx=EBX] [--edx=EDX] [--sdi] [--disable-a20] [--pcdos] [--msdos] FILE",
  "Load the chain-loader FILE. If --force is specified, then load it"
  " forcibly, whether the boot loader signature is present or not."
  " LS:LO specifies the load address other than 0000:7C00. LL specifies"
  " the length of the boot image(between 512 and 640K). CS:IP specifies"
  " the address where the boot image will gain control. EBX/EDX specifies"
  " the EBX/EDX register value when the boot image gets control. Use --sdi"
  " if FILE is a System Deployment Image, which is of the Windows XP"
  " RAM boot file format. Use --disable-a20 if you wish to turn off"
  " A20 when transferring control to the boot image."
  " SL specifies length in bytes at the beginning of the image to be"
  " skipped when loading."
};


/* This function could be used to debug new filesystem code. Put a file
   in the new filesystem and the same file in a well-tested filesystem.
   Then, run "cmp" with the files. If no output is obtained, probably
   the code is good, otherwise investigate what's wrong...  */
/* cmp FILE1 FILE2 */
static int
cmp_func (char *arg, int flags)
{
  /* The filenames.  */
  char *file1, *file2;
  /* The addresses.  */
  char *addr1, *addr2;
  int i, Hex;
  /* The size of the file.  */
  unsigned long long size;
  unsigned long long cur_pos = 0;
    quit_print=0;
    Hex = 0;

  for (;;)
  {
		if (grub_memcmp (arg, "--hex", 5) == 0)
		{
			Hex = 1;
			arg=skip_to (0, arg);
		}
		else if (grub_memcmp (arg, "--skip=", 7) == 0)
		{
			arg += 7;
			if (! safe_parse_maxint_with_suffix (&arg, &cur_pos, 0))
				return 0;
		  while (*arg == ' ' || *arg == '\t') arg++;
		}
		else
			break;
   }
  /* Get the filenames from ARG.  */
  file1 = arg;
  file2 = skip_to (0, arg);
  if (! *file1 || ! *file2)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  /* Terminate the filenames for convenience.  */
  nul_terminate (file1);
  nul_terminate (file2);

  /* Read the whole data from FILE1.  */
  #define CMP_BUF_SIZE 0x100000ULL
  addr1 = (char *) RAW_ADDR (0x600000);
  addr2 = addr1 + CMP_BUF_SIZE;
  if (! grub_open (file1))
    return 0;
  
  /* Get the size.  */
  size = filemax;
  grub_close();
 
  if (! grub_open (file2))
    return 0;
  grub_close();

  if ((size != filemax) && !Hex)
    {
      grub_printf ("Differ in size: 0x%lx [%s], 0x%lx [%s]\n", size, file1, filemax, file2);
      grub_close ();
      return 0;
    }
    
  if (Hex)
  {
	if (current_term->setcolorstate)
		current_term->setcolorstate (COLOR_STATE_HEADING);
		
	grub_printf("Compare FILE1:%s <--> FILE2:%s\t\n",file1,file2);
	
	if (current_term->setcolorstate)
		current_term->setcolorstate (COLOR_STATE_STANDARD);
  }
  
 
  if (! grub_open (file1))
	return 0;

  unsigned long long size1, size2;
  
  if (Hex)
	filepos = cur_pos & -16ULL;
  else
    filepos = cur_pos;

  while ((size1 = grub_read ((unsigned long long)(unsigned long)addr1, CMP_BUF_SIZE, 0xedde0d90)))
  {
  		cur_pos = filepos;
		grub_close();
		if (! grub_open (file2))
			return 0;
		
		filepos = cur_pos - size1;
		if (! (size2 = grub_read ((unsigned long long)(unsigned long)addr2, size1, 0xedde0d90)))
		{
			grub_close ();
			return 0;
		}
		grub_close();
		
		if (Hex)
		{
			for (i = 0; i < size2; i+=16)
			{
				int k,cnt;
				unsigned char c;
				unsigned long cur_offset;
				cur_offset = (unsigned long)(cur_pos - size1 + i);
				if (current_term->setcolorstate)
					current_term->setcolorstate (COLOR_STATE_NORMAL);
				grub_printf("0x%X\t0x%lX/0x%lX\n",cur_offset, size, filemax);
				if (current_term->setcolorstate)
					current_term->setcolorstate (COLOR_STATE_STANDARD);
				grub_printf ("%08X: ",cur_offset);
				cnt = 16;
				if (cnt+i > size2)
					cnt=size2 - i;
				for (k=0;k<cnt;k++)
				{
					c = (unsigned char)(addr1[i+k]);
					if ((addr1[i+k] != addr2[i+k]) && (current_term->setcolorstate))
						current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);
					printf("%02X", c);
					if ((addr1[i+k] != addr2[i+k]) && (current_term->setcolorstate))
						current_term->setcolorstate (COLOR_STATE_STANDARD);
					if ((k != 15) && ((k & 3)==3))
						printf(" ");
					printf(" ");
				}
				for (;k<16;k++)
				{
					printf("   ");
					if ((k!=15) && ((k & 3)==3))
						printf(" ");
				}
				printf("; ");

				for (k=0;k<cnt;k++)
				{
					c=(unsigned char)(addr1[i+k]);
					printf("%c",((c>=32) && (c != 127))?c:'.');
				}
				printf("\n");
				grub_printf ("%08X: ",cur_offset);
				for (k=0;k<cnt;k++)
				{
					c = (unsigned char)(addr2[i+k]);
					if ((addr1[i+k] != addr2[i+k]) && (current_term->setcolorstate))
						current_term->setcolorstate (COLOR_STATE_HIGHLIGHT);
					printf("%02X", c);
					if ((addr1[i+k] != addr2[i+k]) && (current_term->setcolorstate))
						current_term->setcolorstate (COLOR_STATE_STANDARD);
					if ((k!=15) && ((k & 3)==3))
						printf(" ");
					printf(" ");
				}

				for (;k<16;k++)
				{
					printf("   ");
					if ((k!=15) && ((k & 3)==3))
						printf(" ");
				}
				printf("; ");

				for (k=0;k<cnt;k++)
				{
					c = (unsigned char)(addr2[i+k]);
					printf("%c",((c>=32) && (c != 127))?c:'.');
				}
				printf("\n");
				if (quit_print)
					return 1;
			}
		}
		else
		{
			  /* Now compare ADDR1 with ADDR2.  */
			for (i = 0; i < size2; i++)
			{
				if (addr1[i] != addr2[i])
				{
					grub_printf ("Differ at the offset %d: 0x%x [%s], 0x%x [%s]\n",
						(unsigned long)(cur_pos - size1 + i), (unsigned long) addr1[i], file1,
						(unsigned long) addr2[i], file2);
					return 0;
				}
			}
		}
		
		if (! grub_open (file1))
			return 0;	
		filepos = cur_pos;
  }
	grub_close();
	return 1;
}

static struct builtin builtin_cmp =
{
  "cmp",
  cmp_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "cmp [--hex] FILE1 FILE2",
  "Compare the file FILE1 with the FILE2 and inform the different values"
  " if any."
};


static char *color_list[16] =
{
    "black",
    "blue",
    "green",
    "cyan",
    "red",
    "magenta",
    "brown",
    "light-gray",
    "dark-gray",
    "light-blue",
    "light-green",
    "light-cyan",
    "light-red",
    "light-magenta",
    "yellow",
    "white"
};

static int color_number (char *str);
int blinking = 1;
  
  /* Convert the color name STR into the magical number.  */
static int
color_number (char *str)
{
      char *ptr;
      int i;
      int color = 0;
      int tmp_blinking = blinking;
      
      /* Find the separator.  */
      for (ptr = str; *ptr && *ptr != '/'; ptr++)
	;

      /* If not found, return -1.  */
      if (! *ptr)
	return -1;

      /* Terminate the string STR.  */
      *ptr++ = 0;

      /* If STR contains the prefix "blink-", then set the `blink' bit
	 in COLOR.  */
      if (substring ("blink-", str, 0) <= 0)
	{
	  if (tmp_blinking == 0)
		return -1;
	  tmp_blinking = 0x80;
	  color = 0x80;
	  str += 6;
	}
      
      /* Search for the color name.  */
      for (i = 0; i < 16; i++)
	if (grub_strcmp (color_list[i], str) == 0)
	  {
	    color |= i;
	    break;
	  }

      if (i == 16)
	return -1;

      str = ptr;
      nul_terminate (str);

      /* Search for the color name.  */      
      for (i = 0; i < 16; i++)
	if (grub_strcmp (color_list[i], str) == 0)
	  {
	    if (i >= 8)
	    {
		if (tmp_blinking == 0x80)
			return -1;
		tmp_blinking = 0;
	    }
	    color |= i << 4;
	    break;
	  }

      if (i == 16)
	return -1;

      blinking = tmp_blinking;
      return color;
}

/* color */
/* Set new colors used for the menu interface. Support two methods to
   specify a color name: a direct integer representation and a symbolic
   color name. An example of the latter is "blink-light-gray/blue".  */
static int
color_func (char *arg, int flags)
{
  char *normal;
  char *highlight;
  char *helptext;
  char *heading;
  unsigned long long new_normal_color;
  unsigned long long new_highlight_color;
  unsigned long long new_helptext_color;
  unsigned long long new_heading_color;
      
  blinking = 1;
  normal = arg;
  highlight = skip_to (0, arg);
  helptext = skip_to (0, highlight);
  heading = skip_to (0, helptext);

  new_normal_color = (unsigned long long)(long long)color_number (normal);
  if ((long long)new_normal_color < 0 && ! safe_parse_maxint (&normal, &new_normal_color))
    return 0;
  
  if (new_normal_color & 0xFF00)	/* disable blinking */
	blinking = 0;
  new_helptext_color = new_normal_color;
  new_heading_color = new_normal_color;
  /* The second argument is optional, so set highlight_color
     to inverted NORMAL_COLOR.  */
  if (! *highlight)
    new_highlight_color = (((new_normal_color >> 4) & 0xf)
			   | ((new_normal_color & 0xf) << 4));
  else
    {
      new_highlight_color = (unsigned long long)(long long)color_number (highlight);
      if ((long long)new_highlight_color < 0
	  && ! safe_parse_maxint (&highlight, &new_highlight_color))
	return 0;
      if (new_highlight_color & 0xFF00)	/* disable blinking */
      {
	if (blinking == 0x80)
	{
		errnum = ERR_BAD_ARGUMENT;
		return 0;
	}
	blinking = 0;
      }
      if (*helptext)
      {
	new_helptext_color = (unsigned long long)(long long)color_number (helptext);
	if ((long long)new_helptext_color < 0
	    && ! safe_parse_maxint (&helptext, &new_helptext_color))
		return 0;
	if (new_helptext_color & 0xFF00)	/* disable blinking */
	{
		if (blinking == 0x80)
		{
			errnum = ERR_BAD_ARGUMENT;
			return 0;
		}
		blinking = 0;
	}
	if (*heading)
	{
		new_heading_color = (unsigned long long)(long long)color_number (heading);
		if ((long long)new_heading_color < 0
		    && ! safe_parse_maxint (&heading, &new_heading_color))
			return 0;
		if (new_heading_color & 0xFF00)	/* disable blinking */
		{
			if (blinking == 0x80)
			{
				errnum = ERR_BAD_ARGUMENT;
				return 0;
			}
			blinking = 0;
		}
	}
      }
    }

  if (current_term->setcolor)
    current_term->setcolor (new_normal_color, new_highlight_color, new_helptext_color, new_heading_color);
  
  return 1;
}

static struct builtin builtin_color =
{
  "color",
  color_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "color NORMAL [HIGHLIGHT [HELPTEXT [HEADING]]]",
  "Change the menu colors. The color NORMAL is used for most"
  " lines in the menu, and the color HIGHLIGHT is used to highlight the"
  " line where the cursor points. If you omit HIGHLIGHT, then the"
  " inverted color of NORMAL is used for the highlighted line. If you"
  " omit HELPTEXT and/or HEADING, then NORMAL is used."
  " The format of a color is \"FG/BG\". FG and BG are symbolic color names."
  " A symbolic color name must be one of these: black, blue, green,"
  " cyan, red, magenta, brown, light-gray, dark-gray, light-blue,"
  " light-green, light-cyan, light-red, light-magenta, yellow and white."
  " You can prefix \"blink-\" to FG if you want a blinking foreground color."
};


/* configfile */
static int
configfile_func (char *arg, int flags)
{
	if (flags & BUILTIN_BAT_SCRIPT)
	{
		if (! grub_open (arg))
				return 0;
		grub_close();
		return (errnum = -2);
	}
  char *new_config = config_file;

#ifndef GRUB_UTIL
	if (*arg == 0 && *config_file && pxe_restart_config == 0)
	{
		if (configfile_in_menu_init == 0)
			pxe_restart_config = configfile_in_menu_init = 1;
		return 1;
	}
#endif /* ! GRUB_UTIL */

  if (grub_strlen(saved_dir) + grub_strlen(arg) + 20 >= sizeof(chainloader_file_orig))
	return ! (errnum = ERR_WONT_FIT);

  if (*arg == '/')
  {
    if (saved_partition != 0xFFFFFF)
    {
      if ((saved_partition & 0xFF00) == 0xFF00)
	grub_sprintf (chainloader_file_orig, "(%d,%d)%s%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), saved_dir, arg);
      else
	if ((saved_partition & 0xFF0000) == 0xFF0000)
	  grub_sprintf (chainloader_file_orig, "(%d,%c)%s%s", saved_drive, (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), saved_dir, arg);
	else
	  grub_sprintf (chainloader_file_orig, "(%d,%d,%c)%s%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), saved_dir, arg);
    }
    else
      grub_sprintf (chainloader_file_orig, "(%d)%s%s", saved_drive, saved_dir, arg);
  }
  else if (*arg == '(' && arg[1] == ')')
  {
    if (saved_partition != 0xFFFFFF)
    {
      if ((saved_partition & 0xFF00) == 0xFF00)
	grub_sprintf (chainloader_file_orig, "(%d,%d)%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), (arg + 2));
      else
	if ((saved_partition & 0xFF0000) == 0xFF0000)
	  grub_sprintf (chainloader_file_orig, "(%d,%c)%s", saved_drive, (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), (arg + 2));
	else
	  grub_sprintf (chainloader_file_orig, "(%d,%d,%c)%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), (arg + 2));
    }
    else
      grub_sprintf (chainloader_file_orig, "(%d)%s", saved_drive, (arg + 2));
  } else if (*arg != '(')
  {
    if (saved_partition != 0xFFFFFF)
    {
      if ((saved_partition & 0xFF00) == 0xFF00)
	grub_sprintf (chainloader_file_orig, "(%d,%d)%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), arg);
      else
	if ((saved_partition & 0xFF0000) == 0xFF0000)
	  grub_sprintf (chainloader_file_orig, "(%d,%c)%s", saved_drive, (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), arg);
	else
	  grub_sprintf (chainloader_file_orig, "(%d,%d,%c)%s", saved_drive, (unsigned long)(unsigned char)(saved_partition>>16), (unsigned long)(unsigned char)((saved_partition>>8) + 'a'), arg);
    }
    else
      grub_sprintf (chainloader_file_orig, "(%d)%s", saved_drive, arg);
  }
  else
	grub_memmove (chainloader_file_orig, arg, sizeof(chainloader_file_orig));
  //chainloader_file_orig[sizeof(chainloader_file_orig) - 1] = 0;

  arg = chainloader_file_orig;

  nul_terminate (arg);

  /* check possible filename overflow */
#ifdef GRUB_UTIL
  if (grub_strlen (arg) >= 0x70 /*sizeof (config_file)*/)
	return ! (errnum = ERR_WONT_FIT);
#else
  if (grub_strlen (arg) >= ((char *)0x8270 - new_config))
	return ! (errnum = ERR_WONT_FIT);
#endif
  
  /* Check if the file ARG is present.  */
  if (! grub_open (arg))
    return 0;

#ifndef GRUB_UTIL
  if (current_drive == cdrom_drive)
	configfile_opened = 1;
  else
#endif /* ! GRUB_UTIL */
	grub_close ();
  
//#ifndef GRUB_UTIL
//	if (pxe_restart_config == 0)
//	{
//		if (configfile_in_menu_init)
//			return 1;
//	}
//#endif /* ! GRUB_UTIL */

  /* Copy ARG to CONFIG_FILE.  */
  while ((*new_config++ = *arg++) != 0)
    ;

//#ifdef GRUB_UTIL
  /* Force to load the configuration file.  */
  use_config_file = 1;
//#endif

#ifndef GRUB_UTIL
	if (pxe_restart_config == 0)
	{
		pxe_restart_config = /* configfile_in_menu_init = */ 1;
		return 1;
	}
#endif /* ! GRUB_UTIL */

  /* Make sure that the user will not be authoritative.  */
  auth = 0;
  
  saved_entryno = 0;
  //force_cdrom_as_boot_device = 0;
  if (current_drive != 0xFFFF)
  {
    boot_drive = current_drive;
    install_partition = current_partition;
  }
#ifdef GRUB_UTIL
  buf_drive = -1;	/* invalidate disk cache. */
  buf_track = -1;	/* invalidate disk cache. */
  current_drive = GRUB_INVALID_DRIVE;
  current_partition = 0xFFFFFF;
  fsys_type = NUM_FSYS;
  boot_part_addr = 0;
  current_slice = 0;
  /* Restart cmain.  */
  grub_longjmp (restart_env, 0);
#else
  ///* Restart pre_stage2.  */
  //(*(char *)0x8205) |= 2;	/* disable keyboard intervention */
  //chain_stage1(0, 0x8200, boot_part_addr);
  /* Restart cmain.  */
  asm volatile ("movl $0x7000, %esp");	/* set stack to STACKOFF */
#ifdef HAVE_ASM_USCORE
  asm volatile ("call _cmain");
  asm volatile ("jmp _stop");
#else
  asm volatile ("call cmain");
  asm volatile ("jmp stop");
#endif
#endif /* ! GRUB_UTIL */

  /* Never reach here.  */
  return 1;
}

static struct builtin builtin_configfile =
{
  "configfile",
  configfile_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "configfile FILE",
  "Load FILE as the configuration file."
};


#ifndef GRUB_UTIL
/* dd if=IF of=OF */
static int
dd_func (char *arg, int flags)
{
  char *p;
  char *in_file = NULL, *out_file = NULL;
  unsigned long long bs = 0;
  unsigned long long count = 0;
  unsigned long long skip = 0;
  unsigned long long seek = 0;
  unsigned long long old_part_start = part_start;
  unsigned long long old_part_length = part_length;
//  int in_fsys_type;
  unsigned long in_drive;
  unsigned long in_partition;
//  int out_fsys_type;
  unsigned long out_drive;
  unsigned long out_partition;
  unsigned long long in_filepos;
  unsigned long long in_filemax;
  unsigned long long out_filepos;
  unsigned long long out_filemax;
  unsigned long long buf_addr = 0x20000ULL;
  unsigned long long buf_size = 0x10000ULL;
  char tmp_in_file[16];
  char tmp_out_file[16];

  for (;;)
  {
    if (grub_memcmp (arg, "if=", 3) == 0)
      {
	if (in_file)
		return !(errnum = ERR_BAD_ARGUMENT);
	in_file = arg + 3;
	if (/* *in_file != '/' &&*/ *in_file != '(')
		return !(errnum = ERR_DEV_FORMAT);
      }
    else if (grub_memcmp (arg, "of=", 3) == 0)
      {
	if (out_file)
		return !(errnum = ERR_BAD_ARGUMENT);
	out_file = arg + 3;
	if (/* *out_file != '/' &&*/ *out_file != '(')
		return !(errnum = ERR_DEV_FORMAT);
      }
    else if (grub_memcmp (arg, "bs=", 3) == 0)
      {
	if (bs)
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 3;
	if (*p == '-')
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &bs))
		return 0;
	if (bs == 0 /*|| 0x100000 % bs*/)
		return !(errnum = ERR_BAD_ARGUMENT);
      }
    else if (grub_memcmp (arg, "count=", 6) == 0)
      {
	if (count)
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 6;
	if (*p == '-')
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &count))
		return 0;
	if (count == 0)
		return !(errnum = ERR_BAD_ARGUMENT);
      }
    else if (grub_memcmp (arg, "skip=", 5) == 0)
      {
	if (skip)
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 5;
	if (*p == '-')
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &skip))
		return 0;
      }
    else if (grub_memcmp (arg, "seek=", 5) == 0)
      {
	if (seek)
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 5;
	if (*p == '-')
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &seek))
		return 0;
      }
    else if (grub_memcmp (arg, "buf=", 4) == 0)
      {
	if (buf_addr >= 0x100000)	/* already at above 1M, so it is set previously. */
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 4;
	if (*p == '-')			/* negative */
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &buf_addr))
		return 0;
	if (buf_addr < 0x100000)	/* cannot set buffer at below 1M */
		return !(errnum = ERR_BAD_ARGUMENT);
      }
    else if (grub_memcmp (arg, "buflen=", 7) == 0)
      {
	if (buf_size > 0x10000)		/* already above 640K, so it is set previously. */
		return !(errnum = ERR_BAD_ARGUMENT);
	p = arg + 7;
	if (*p == '-')			/* negative */
		return !(errnum = ERR_BAD_ARGUMENT);
	if (! safe_parse_maxint (&p, &buf_size))
		return 0;
	if (buf_size <= 0x10000 || buf_size > 0xFFFFFFFF)	/* cannot set buffer size below 640K */
		return !(errnum = ERR_BAD_ARGUMENT);
      }
    else if (*arg)
		return !(errnum = ERR_BAD_ARGUMENT);
    else
	break;
    arg = skip_to (0, arg);
  }
  
  if (! in_file || ! out_file)
	return !(errnum = ERR_BAD_ARGUMENT);
  if (bs == 0)
	bs = 512;

//  if (*in_file == '/')
//  {
//	in_drive = saved_drive;
//	in_partition = saved_partition;
//	in_file--;
//	*in_file = ')';
//	in_file--;
//	*in_file = '(';
//	if (! grub_open (in_file))
//		goto fail;
//	in_filemax = filemax;
//	grub_close ();
//  }
//  else
  {
	p = set_device (in_file);
	if (errnum)
		goto fail;
	if (! p)
	{
		if (errnum == 0)
			errnum = ERR_BAD_ARGUMENT;
		goto fail;
	}
	in_drive = current_drive;
	in_partition = current_partition;
	/* if only the device portion is specified */
	if ((unsigned char)*p <= ' ')
	{
		in_file = p = tmp_in_file;
		*p++ = '(';
		*p++ = ')';
		*p++ = '+';
		*p++ = '1';
		*p = 0;
		current_drive = saved_drive;
		current_partition = saved_partition;
		saved_drive = in_drive;
		saved_partition = in_partition;
		in_drive = current_drive;
		in_partition = current_partition;
		grub_open (in_file);
		current_drive = saved_drive;
		current_partition = saved_partition;
		saved_drive = in_drive;
		saved_partition = in_partition;
		in_drive = current_drive;
		in_partition = current_partition;
		if (errnum)
			goto fail;
		in_filemax = (unsigned long long)(buf_geom.sector_size) * part_length;
		grub_sprintf (in_file + 3, "0x%lx", (unsigned long long)part_length);
		grub_close ();
	}
	else
	{
		//in_file--;
		//*in_file = ')';
		//in_file--;
		//*in_file = '(';
		//current_drive = saved_drive;
		//current_partition = saved_partition;
		//saved_drive = in_drive;
		//saved_partition = in_partition;
		//in_drive = current_drive;
		//in_partition = current_partition;
		grub_open (in_file);
		//current_drive = saved_drive;
		//current_partition = saved_partition;
		//saved_drive = in_drive;
		//saved_partition = in_partition;
		//in_drive = current_drive;
		//in_partition = current_partition;
		in_filemax = filemax;
		if (errnum)
			goto fail;
		grub_close ();
	}
  }
//  in_fsys_type = fsys_type;

//  if (*out_file == '/')
//  {
//	out_drive = saved_drive;
//	out_partition = saved_partition;
//	out_file--;
//	*out_file = ')';
//	out_file--;
//	*out_file = '(';
//	if (! grub_open (out_file))
//		goto fail;
//	out_filemax = filemax;
//	grub_close ();
//  }
//  else
  {
	p = set_device (out_file);
	if (errnum)
		goto fail;
	if (! p)
	{
		if (errnum == 0)
			errnum = ERR_BAD_ARGUMENT;
		goto fail;
	}
	out_drive = current_drive;
	out_partition = current_partition;
	/* if only the device portion is specified */
	if ((unsigned char)*p <= ' ')
	{
		out_file = p = tmp_out_file;
		*p++ = '(';
		*p++ = ')';
		*p++ = '+';
		*p++ = '1';
		*p = 0;
		current_drive = saved_drive;
		current_partition = saved_partition;
		saved_drive = out_drive;
		saved_partition = out_partition;
		out_drive = current_drive;
		out_partition = current_partition;
		grub_open (out_file);
		current_drive = saved_drive;
		current_partition = saved_partition;
		saved_drive = out_drive;
		saved_partition = out_partition;
		out_drive = current_drive;
		out_partition = current_partition;
		if (errnum)
			goto fail;
		out_filemax = (unsigned long long)(buf_geom.sector_size) * part_length;
		grub_sprintf (out_file + 3, "0x%lx", (unsigned long long)part_length);
		grub_close ();
	}
	else
	{
		//out_file--;
		//*out_file = ')';
		//out_file--;
		//*out_file = '(';
		//current_drive = saved_drive;
		//current_partition = saved_partition;
		//saved_drive = out_drive;
		//saved_partition = out_partition;
		//out_drive = current_drive;
		//out_partition = current_partition;
		grub_open (out_file);
		//current_drive = saved_drive;
		//current_partition = saved_partition;
		//saved_drive = out_drive;
		//saved_partition = out_partition;
		//out_drive = current_drive;
		//out_partition = current_partition;
		out_filemax = filemax;
		if (errnum)
			goto fail;
		grub_close ();
	}
  }
//  out_fsys_type = fsys_type;

  /* calculate in_filepos and out_filepos */
  in_filepos = skip * bs;
  out_filepos = seek * bs;
  if (count)
  {
	if (in_filemax > ((count + skip) * bs))
	    in_filemax = ((count + skip) * bs);
	if (out_filemax > ((count + seek) * bs))
	    out_filemax = ((count + seek) * bs);
  }

  if (in_drive == 0xFFFF && in_file == tmp_in_file &&	/* in_file is (md) */
      out_drive == 0xFFFF && out_file == tmp_out_file)	/* out_file is (md) */
  {
	unsigned long long tmp_part_start = part_start;
	unsigned long long tmp_part_length = part_length;

	count = in_filemax - in_filepos;
	if (count > out_filemax - out_filepos)
	    count = out_filemax - out_filepos;

	part_start = old_part_start;
	part_length = old_part_length;
	grub_memmove64 (out_filepos, in_filepos, count);
	part_start = tmp_part_start;
	part_length = tmp_part_length;
	if (debug > 0)
	{
		grub_printf ("\nMoved 0x%lX bytes from 0x%lX to 0x%lX\n", (unsigned long long)count, (unsigned long long)in_filepos, (unsigned long long)out_filepos);
	}
	errnum = 0;
	return count;
  }

  /* (*p == '/') indicates out_file is not a block file */
  /* (*p != '/') indicates out_file is a block file */

#if 0
  if (out_drive != ram_drive && out_drive != 0xFFFF && *p != '/')
  {
	unsigned long j;

	/* check if it is a mapped memdrive */
	j = DRIVE_MAP_SIZE;		/* real drive */
#ifndef GRUB_UTIL
	if (! unset_int13_handler (1))	/* map is hooked */
	    for (j = 0; j < DRIVE_MAP_SIZE; j++)
	    {
		if (drive_map_slot_empty (hooked_drive_map[j]))
		{
			j = DRIVE_MAP_SIZE;	/* real drive */
			break;
		}

		if (out_drive == hooked_drive_map[j].from_drive && hooked_drive_map[j].to_drive == 0xFF && !(hooked_drive_map[j].to_cylinder & 0x4000))
			break;			/* memdrive */
	    }
#endif /* ! GRUB_UTIL */

	if (j == DRIVE_MAP_SIZE)	/* real drive */
	{
	    /* this command is intended for running in command line and inhibited from running in menu.lst */
	    if (flags & (BUILTIN_MENU | BUILTIN_SCRIPT))
	    {
		errnum = ERR_WRITE_TO_NON_MEM_DRIVE;
		goto fail;
	    }
	}
  }
#endif

  {
    unsigned long long in_pos = in_filepos;
    unsigned long long out_pos = out_filepos;
    unsigned long long tmp_size = buf_size;

    if (debug > 0)
    {
	count = in_filemax - in_filepos;
	if (count > out_filemax - out_pos)
	    count = out_filemax - out_pos;
	count = ((unsigned long)(count + buf_size - 1) / (unsigned long)buf_size);
	grub_printf ("buf_size=0x%lX, loops=0x%lX. in_pos=0x%lX, out_pos=0x%lX\n", (unsigned long long)buf_size, (unsigned long long)count, (unsigned long long)in_pos, (unsigned long long)out_pos);
    }
    count = 0;
    while (in_pos < in_filemax && out_pos < out_filemax)
    {
	if (debug > 0)
	{
		if (!((char)count & 7))
			grub_printf ("\r");
		grub_printf ("%08X ", (unsigned long)(count));
	}
	/* open in_file */
	current_drive = saved_drive;
	current_partition = saved_partition;
	saved_drive = in_drive;
	saved_partition = in_partition;
	in_drive = current_drive;
	in_partition = current_partition;
	current_drive = saved_drive;
	current_partition = saved_partition;
//	fsys_type = in_fsys_type;
	if (grub_open (in_file))
	{
		filepos = in_pos;
		//tmp_size = buf_size;
		if (tmp_size > in_filemax - in_pos)
		    tmp_size = in_filemax - in_pos;
		if (grub_read (buf_addr, tmp_size, 0xedde0d90) != tmp_size)	/* read */
		{
			if (errnum == 0)
				errnum = ERR_READ;
		}
		{
			int err = errnum;
			grub_close ();
			errnum = err;
		}
	}
	current_drive = saved_drive;
	current_partition = saved_partition;
	saved_drive = in_drive;
	saved_partition = in_partition;
	in_drive = current_drive;
	in_partition = current_partition;
	if (errnum)
		goto end;

	in_pos += tmp_size;
	
	/* open out_file */
	current_drive = saved_drive;
	current_partition = saved_partition;
	saved_drive = out_drive;
	saved_partition = out_partition;
	out_drive = current_drive;
	out_partition = current_partition;
	current_drive = saved_drive;
	current_partition = saved_partition;
//	fsys_type = out_fsys_type;
	if (grub_open (out_file))
	{
		filepos = out_pos;
		if (tmp_size > out_filemax - out_pos)
		    tmp_size = out_filemax - out_pos;
		if (grub_read (buf_addr, tmp_size, 0x900ddeed) != tmp_size)	/* write */
		{
			if (errnum == 0)
				errnum = ERR_WRITE;
		}
		{
			int err = errnum;
			grub_close ();
			errnum = err;
		}
	}
	current_drive = saved_drive;
	current_partition = saved_partition;
	saved_drive = out_drive;
	saved_partition = out_partition;
	out_drive = current_drive;
	out_partition = current_partition;
	if (errnum)
		goto end;

	out_pos += tmp_size;
	count++;
    }

end:

    in_pos -= in_filepos;
    out_pos -= out_filepos;

    if (debug > 0)
    {
	int err = errnum;
	grub_printf ("\nBytes read / written = 0x%lX / 0x%lX\n", (unsigned long long)in_pos, (unsigned long long)out_pos);
	errnum = err;
    }
  }

fail:

//  if (*(long *)(in_file - 1) == 0x2F292869)	/* i()/ */
//	*(unsigned short *)in_file = 0x3D66;		/* f= */
//  if (*(long *)(out_file - 1) == 0x2F29286F)	/* o()/ */
//	*(unsigned short *)out_file = 0x3D66;		/* f= */

  return !(errnum);
}

static struct builtin builtin_dd =
{
  "dd",
  dd_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "dd if=IF of=OF [bs=BS] [count=C] [skip=IN] [seek=OUT] [buf=ADDR] [buflen=SIZE]",
  "Copy file IF to OF. BS is blocksize, default to 512. C is blocks to copy,"
  " default is total blocks in IF. IN specifies number of blocks to skip when"
  " read, default is 0. OUT specifies number of blocks to skip when write,"
  " default is 0. Skipped blocks are not touched. Both IF and OF must exist."
  " dd can neither enlarge nor reduce the size of OF, the leftover tail"
  " of IF will be discarded. OF cannot be a gzipped file. If IF is a gzipped"
  " file, it will be decompressed automatically when copying. dd is dangerous,"
  " use at your own risk. To be on the safe side, you should only use dd to"
  " write a file in memory. ADDR and SIZE are used for user-defined buffer."
};


#endif	/* ! GRUB_UTIL */
/* debug */
static int bat_script_debug = 0;
static int
debug_func (char *arg, int flags)
{
  unsigned long long tmp_debug;

  if (! *arg)
  {
    ///* If ARG is empty, toggle the flag.  */
    //debug = ! debug;
  }
  else if (grub_memcmp (arg, "on", 2) == 0)
    debug = 2;
  else if (grub_memcmp (arg, "normal", 6) == 0)
    debug = 1;
  else if (grub_memcmp (arg, "off", 3) == 0)
    debug = 0;
  else if (grub_memcmp (arg, "status", 6) == 0)
    grub_printf (" Debug is now %d\n", (unsigned long)debug);
  else if (safe_parse_maxint (&arg, &tmp_debug))
  {
    debug = tmp_debug;
    switch (debug)
    {
		case 3:
			bat_script_debug = 1;
			break;
		case 4:
			bat_script_debug = 0;
			break;
    }
  }
  else
  {
    if (errnum == 0)
      errnum = ERR_BAD_ARGUMENT;
    return 0;
  }

  return debug;
}

static struct builtin builtin_debug =
{
  "debug",
  debug_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "debug [on | off | normal | status | INTEGER]",
  "Turn on/off or display/set the debug level."
};


/* default */
static int
default_func (char *arg, int flags)
{
  unsigned long long ull;
  int len;
  char *p;
#if defined(STAGE1_5) || defined(GRUB_UTIL)
//  char mbr[SECTOR_SIZE];
#endif
  char *default_file = (char *) DEFAULT_FILE_BUF;

#ifndef SUPPORT_DISKLESS
  if (grub_memcmp (arg, "saved", 5) == 0)
    {
      default_entry = saved_entryno;
      return 1;
    }
#endif /* SUPPORT_DISKLESS */
  
  if (safe_parse_maxint (&arg, &ull))
    {
      default_entry = ull;
      return 1;
    }

  errnum = ERR_NONE;
  
//printf("default_func open %s\n", arg);
  /* Open the file.  */
  if (! grub_open (arg))
    return ! errnum;

  if (compressed_file)
  {
    grub_close ();
    return ! (errnum = ERR_DEFAULT_FILE);
  }

  len = grub_read ((unsigned long long)(unsigned long)mbr, SECTOR_SIZE, 0xedde0d90);
  grub_close ();
  
//printf("default_func errnum=%d\n", errnum);

  if (len < 180 || filemax > 2048)
    return ! (errnum = ERR_DEFAULT_FILE);

//printf("default_func errnum=%d\n", errnum);
  /* check file content for safety */
  p = mbr;
  while (p < mbr + len - 100 && grub_memcmp (++p, warning_defaultfile, 73));

//printf("default_func errnum=%d\n", errnum);
  //grub_printf ("p=%x, mbr=%x, len=%x, mbr + len - 160 = %x\n", p, mbr, len, mbr + len - 160);
  if (p > mbr + len - 160)
    return ! (errnum = ERR_DEFAULT_FILE);

//printf("default_func errnum=%d\n", errnum);
  len = grub_strlen (arg);
  if (len >= DEFAULT_FILE_BUFLEN)
    return ! (errnum = ERR_WONT_FIT);
  
//printf("default_func errnum=%d\n", errnum);
  grub_memmove (default_file, arg, len);
  default_file[len] = 0;
  boot_drive = current_drive;
  install_partition = current_partition;
  
  p = mbr;
  if (safe_parse_maxint (&p, &ull))
    {
      default_entry = ull;
      return 1;
    }

//printf("default_func errnum=%d\n", errnum);
  errnum = 0;		/* ignore error */
  return errnum;	/* return false */
}

static struct builtin builtin_default =
{
  "default",
  default_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "default [NUM | `saved' | FILE]",
  "Set the default entry to entry number NUM (if not specified, it is"
  " 0, the first entry), or to the entry number saved by savedefault if"
  " the key word `saved\' is specified, or to the entry number previously"
  " saved in the specified file FILE. When FILE is specified, all subsequent"
  " `savedefault\' commands will save default entry numbers into"
  " FILE."
#if 0
  "default [NUM | `saved']",
  "Set the default entry to entry number NUM (if not specified, it is"
  " 0, the first entry) or the entry number saved by savedefault."
#endif
};


#ifdef GRUB_UTIL
/* device */
static int
device_func (char *arg, int flags)
{
  char *drive = arg;
  char *device;

  /* Get the drive number from DRIVE.  */
  if (! set_device (drive))
    return 0;

  /* Get the device argument.  */
  device = skip_to (0, drive);
  
  /* Terminate DEVICE.  */
  nul_terminate (device);

  if (! *device || ! check_device (device))
    {
      errnum = ERR_FILE_NOT_FOUND;
      return 0;
    }

  assign_device_name (current_drive, device);
  
  return 1;
}

static struct builtin builtin_device =
{
  "device",
  device_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "device DRIVE DEVICE",
  "Specify DEVICE as the actual drive for a BIOS drive DRIVE. This command"
  " can be used only in the grub shell."
};
#endif /* GRUB_UTIL */


#ifdef SUPPORT_NETBOOT
/* dhcp */
static int
dhcp_func (char *arg, int flags)
{
  /* For now, this is an alias for bootp.  */
  return bootp_func (arg, flags);
}

static struct builtin builtin_dhcp =
{
  "dhcp",
  dhcp_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "dhcp",
  "Initialize a network device via DHCP."
};
#endif /* SUPPORT_NETBOOT */


static int terminal_func (char *arg, int flags);

#ifdef SUPPORT_GRAPHICS
extern char splashimage[64];

static int
splashimage_func(char *arg, int flags)
{
    //char splashimage[64];
//    int i;

    /* If ARG is empty, we reset SPLASHIMAGE.  */
    if (*arg)
    {
	/* filename can only be 64 characters due to our buffer size */
	if (strlen(arg) > 63)
		return ! (errnum = ERR_WONT_FIT);
    
//    if (flags == BUILTIN_CMDLINE) {
	if (! grub_open(arg))
		return 0;
	grub_close();
//    }
    }

    strcpy(splashimage, arg);

//    /* get rid of TERM_NEED_INIT from the graphics terminal. */
//    for (i = 0; term_table[i].name; i++) {
//	if (grub_strcmp (term_table[i].name, "graphics") == 0) {
//	    term_table[i].flags &= ~TERM_NEED_INIT;
//	    break;
//	}
//    }
    
    //graphics_set_splash (splashimage);

    if (/* flags == BUILTIN_CMDLINE && */ graphics_inited) {
	graphics_end();
	if (! graphics_init())
		return ! (errnum = ERR_EXEC_FORMAT);
	graphics_cls();
    }

    /* FIXME: should we be explicitly switching the terminal as a 
     * side effect here? */
    terminal_func("graphics", flags);

    return 1;
}

static struct builtin builtin_splashimage =
{
  "splashimage",
  splashimage_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "splashimage FILE",
  "Load FILE as the background image when in graphics mode."
};


/* foreground */
static int
foreground_func(char *arg, int flags)
{
    if (grub_strlen(arg) == 6) {
	int r = ((hex(arg[0]) << 4) | hex(arg[1])) >> 2;
	int g = ((hex(arg[2]) << 4) | hex(arg[3])) >> 2;
	int b = ((hex(arg[4]) << 4) | hex(arg[5])) >> 2;

	foreground = (r << 16) | (g << 8) | b;
	if (graphics_inited)
	    graphics_set_palette(15, r, g, b);

	return 1;
    }

    return 0;
}

static struct builtin builtin_foreground =
{
  "foreground",
  foreground_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "foreground RRGGBB",
  "Sets the foreground color when in graphics mode."
  "RR is red, GG is green, and BB blue. Numbers must be in hexadecimal."
};


/* background */
static int
background_func(char *arg, int flags)
{
    if (grub_strlen(arg) == 6) {
	int r = ((hex(arg[0]) << 4) | hex(arg[1])) >> 2;
	int g = ((hex(arg[2]) << 4) | hex(arg[3])) >> 2;
	int b = ((hex(arg[4]) << 4) | hex(arg[5])) >> 2;

	background = (r << 16) | (g << 8) | b;
	if (graphics_inited)
	    graphics_set_palette(0, r, g, b);
	return 1;
    }

    return 0;
}

static struct builtin builtin_background =
{
  "background",
  background_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "background RRGGBB",
  "Sets the background color when in graphics mode."
  "RR is red, GG is green, and BB blue. Numbers must be in hexadecimal."
};

#endif /* SUPPORT_GRAPHICS */

static int
in_range (char *range, unsigned long long val)
{
  unsigned long long start_num;
  unsigned long long end_num;

  for (;;)
  {
    if (! safe_parse_maxint (&range, &start_num))
	break;
    if (val == start_num)
	return 1;
    if (*range == ',')
    {
	range++;
	continue;
    }
    if (*range != ':')
	break;

    range++;
    if (! safe_parse_maxint (&range, &end_num))
	break;
    if ((long long)val > (long long)start_num && (long long)val <= (long long)end_num)
	return 1;
    if (val > start_num && val <= end_num)
	return 1;
    if (*range != ',')
	break;

    range++;
  }

  errnum = 0;
  return 0;
}

/* checkrange */
int
checkrange_func(char *arg, int flags)
{
  struct builtin *builtin1;
  unsigned long ret;
  char *arg1;

  arg1 = skip_to (0, arg);	/* the command */

  builtin1 = find_command (arg1);

  if ((int)builtin1 != -1)
  if (! builtin1 || ! (builtin1->flags & flags))
  {
	errnum = ERR_UNRECOGNIZED;
	return 0;
  }

  if ((int)builtin1 == -1 || ((builtin1->func) != errnum_func && (builtin1->func) != checkrange_func))
	errnum = 0;

  if ((int)builtin1 != -1)
	ret = (builtin1->func) (skip_to (1, arg1), flags);
  else
	ret = command_func (arg1, flags);

  if ((int)builtin1 != -1)
  if ((builtin1->func) == errnum_func /*|| (builtin1->func) == checkrange_func*/)
	errnum = 0;
  if (errnum)
	return 0;

  return in_range (arg, ret);
}

static struct builtin builtin_checkrange =
{
  "checkrange",
  checkrange_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "checkrange RANGE COMMAND",
  "Return true if the return value of COMMAND is in RANGE and false otherwise."
};
#ifndef GRUB_UTIL
/* checktime */
static int
checktime_func(char *arg, int flags)
{
  unsigned long date,time;
  int day, month, year, sec, min, hour, dow, ii;
  int limit[5][2] = {{0, 59}, {0, 23}, {1, 31}, {1, 12}, {0, 7}};
  int field[5];

  auto int get_day_of_week (void);
  int get_day_of_week (void)
    {
      int a, y, m;

      a = (14 - month) / 12;
      y = year - a;
      m = month + 12 * a - 2;
      return (day + y + y / 4 - y / 100 + y / 400 + (31 * m / 12)) % 7;
    }

  get_datetime(&date, &time);

  day = ((date >> 4) & 0xF) * 10 + (date & 0xF);
  date >>= 8;

  month = ((date >> 4) & 0xF) * 10 + (date & 0xF);
  date >>= 8;

  year = ((date >> 4) & 0xF) * 10 + (date & 0xF);
  date >>= 8;
  year += (((date >> 4) & 0xF) * 10 + (date & 0xF)) * 100;

  time >>= 8;

  sec = ((time >> 4) & 0xF) * 10 + (time & 0xF);
  time >>= 8;

  min = ((time >> 4) & 0xF) * 10 + (time & 0xF);
  time >>= 8;

  hour = ((time >> 4) & 0xF) * 10 + (time & 0xF);

  dow = get_day_of_week();

  field[0] = min;
  field[1] = hour;
  field[2] = day;
  field[3] = month;
  field[4] = dow;

  if (! arg[0])
    {
      grub_printf ("%d-%02d-%02d %02d:%02d:%02d %d\n", year, month, day, hour, min, sec, dow);
      return time;
    }

  for (ii = 0; ii < 5; ii++)
    {
      char *p;
      int ok = 0;

      if (! arg[0])
        break;

      p = arg;
      while (1)
        {
          unsigned long long m1, m2, m3;
	  int j;

          if (*p == '*')
            {
              m1 = limit[ii][0];
              m2 = limit[ii][1];
              p++;
            }
          else
            {
              if (! safe_parse_maxint (&p, &m1))
		return 0;

              if (*p == '-')
                {
                  p++;
                  if (! safe_parse_maxint (&p, &m2))
                    return 0;
                }
              else
                m2 = m1;
            }

          if ((m1 < limit[ii][0]) || (m2 > limit[ii][1]) || (m1 > m2))
            return 0;

          if (*p == '/')
            {
              p++;
              if (! safe_parse_maxint (&p, &m3))
                return 0;
            }
          else
            m3 = 1;

          for (j = m1; j <= m2; j+= m3)
            {
              if (j == field[ii])
                {
                  ok = 1;
                  break;
                }
            }

          if (ok)
            break;

          if (*p == ',')
            p++;
          else
            break;
        }

      if (! ok)
        break;

      arg = skip_to (0, arg);
    }

  return (ii == 5);
}

static struct builtin builtin_checktime =
{
  "checktime",
  checktime_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "checktime min hour dom month dow",
  "Check time."
};
#endif

/* clear */
static int 
clear_func() 
{
  if (current_term->cls)
    current_term->cls();
  if (use_pager)
    count_lines = 0;
  return 1;
}

static struct builtin builtin_clear =
{
  "clear",
  clear_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "clear",
  "Clear the screen"
};


#if 0
/* displayapm */
static int
displayapm_func (char *arg, int flags)
{
  if (mbi.flags & MB_INFO_APM_TABLE)
    {
      grub_printf ("APM BIOS information:\n"
		   " Version:          0x%x\n"
		   " 32-bit CS:        0x%x\n"
		   " Offset:           0x%x\n"
		   " 16-bit CS:        0x%x\n"
		   " 16-bit DS:        0x%x\n"
		   " 32-bit CS length: 0x%x\n"
		   " 16-bit CS length: 0x%x\n"
		   " 16-bit DS length: 0x%x\n",
		   (unsigned) apm_bios_info.version,
		   (unsigned) apm_bios_info.cseg,
		   apm_bios_info.offset,
		   (unsigned) apm_bios_info.cseg_16,
		   (unsigned) apm_bios_info.dseg_16,
		   (unsigned) apm_bios_info.cseg_len,
		   (unsigned) apm_bios_info.cseg_16_len,
		   (unsigned) apm_bios_info.dseg_16_len);
      return 1;
    }

  grub_printf ("No APM BIOS found or probe failed\n");

  return 0;
}

static struct builtin builtin_displayapm =
{
  "displayapm",
  displayapm_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "displayapm",
  "Display APM BIOS information."
};
#endif


/* displaymem */
static int
displaymem_func (char *arg, int flags)
{
  if (get_eisamemsize () != -1)
    grub_printf (" EISA Memory BIOS Interface is present\n");
  if (get_mmap_entry ((void *) SCRATCHADDR, 0) != 0
      || *((int *) SCRATCHADDR) != 0)
    grub_printf (" Address Map BIOS Interface is present\n");

  grub_printf (" Lower memory: %uK, "
	       "Upper memory (to first chipset hole): %uK\n",
	       (unsigned long)saved_mem_lower, (unsigned long)saved_mem_upper);

  if (mbi.flags & MB_INFO_MEM_MAP)
    {
      struct AddrRangeDesc *map = (struct AddrRangeDesc *) saved_mmap_addr;
      unsigned long end_addr = saved_mmap_addr + saved_mmap_length;

      grub_printf (" [Address Range Descriptor entries "
		   "immediately follow (values are 64-bit)]\n");
      while (end_addr > (unsigned long) map)
	{
	  char *str;

	  if (map->Type == MB_ARD_MEMORY)
	    str = "Usable RAM";
	  else
	    str = "Reserved";
	  grub_printf ("  %s: Base: 0x%lX, Length: 0x%lX\n",
		       str,
		       (unsigned long long)(map->BaseAddr),
		       (unsigned long long)(map->Length));

	  map = ((struct AddrRangeDesc *) (((int) map) + 4 + map->size));
	}
    }

  return 1;
}

static struct builtin builtin_displaymem =
{
  "displaymem",
  displaymem_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "displaymem",
  "Display what GRUB thinks the system address space map of the"
  " machine is, including all regions of physical RAM installed."
};


/* dump FROM TO */
#ifdef GRUB_UTIL
static int
dump_func (char *arg, int flags)
{
  char *from, *to;
  FILE *fp;
  char c;
  
  from = arg;
  to = skip_to (0, arg);
  if (! *from || ! *to)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  nul_terminate (from);
  nul_terminate (to);
  
  if (! grub_open (from))
    return 0;

  fp = fopen (to, "w");
  if (! fp)
    {
      errnum = ERR_WRITE;
      return 0;
    }

  while (grub_read ((unsigned long long)(unsigned int)&c, 1, 0xedde0d90))
    if (fputc (c, fp) == EOF)
      {
	errnum = ERR_WRITE;
	fclose (fp);
	return 0;
      }

  if (fclose (fp) == EOF)
    {
      errnum = ERR_WRITE;
      return 0;
    }

  grub_close ();
  return 1;
}

static struct builtin builtin_dump =
  {
    "dump",
    dump_func,
    BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT,
    "dump FROM TO",
    "Dump the contents of the file FROM to the file TO. FROM must be"
    " a GRUB file and TO must be an OS file."
  };
#endif /* GRUB_UTIL */


#ifdef GRUB_UTIL
static char embed_info[32];
/* embed */
/* Embed a Stage 1.5 in the first cylinder after MBR or in the
   bootloader block in a FFS.  */
static int
embed_func (char *arg, int flags)
{
  char *stage1_5;
  char *device;
  char *stage1_5_buffer = (char *) RAW_ADDR (0x100200);
  unsigned long len, size;
  unsigned long sector;
  
  stage1_5 = arg;
  device = skip_to (0, stage1_5);

  /* Open a Stage 1.5.  */
  if (! grub_open (stage1_5))
    return 0;

  /* Read the whole of the Stage 1.5.  */
  len = grub_read ((unsigned long long)(unsigned int)stage1_5_buffer, -1ULL, 0xedde0d90);
  grub_close ();
  
  if (errnum)
    return 0;
  
  size = (len + SECTOR_SIZE - 1) / SECTOR_SIZE;
  
  /* Get the device where the Stage 1.5 will be embedded.  */
  set_device (device);
  if (errnum)
    return 0;

  if (current_partition == 0xFFFFFF)
    {
      /* Embed it after the MBR.  */

//#if defined(STAGE1_5) || defined(GRUB_UTIL)	    
      //char mbr[SECTOR_SIZE];
#define mbr ((char *) RAW_ADDR (0x100000))
//#endif
#if 0	/* !!!! The obsolete EZ-BIOS no longer supported !!!! */
      char ezbios_check[2*SECTOR_SIZE];
#endif
      int i;
      
      /* Open the partition.  */
      if (! open_partition ())
	return 0;

      /* No floppy has MBR.  */
      if (! (current_drive & 0x80))
	{
	  errnum = ERR_DEV_VALUES;
	  return 0;
	}
      
      /* Read the MBR of CURRENT_DRIVE.  */
      //printf ("current_drive=%X, mbr=%X\n", current_drive, mbr);
      if (! rawread (current_drive, PC_MBR_SECTOR, 0, SECTOR_SIZE, (unsigned long long)(unsigned int)mbr, 0xedde0d90))
	return 0;
      
      //printf ("current_drive=%X, mbr=%X\n", current_drive, mbr);
      /* Sanity check.  */
      if (! PC_MBR_CHECK_SIG (mbr))
	{
	  errnum = ERR_BAD_PART_TABLE;
	  return 0;
	}

      /* Check if the disk can store the Stage 1.5.  */
      for (i = 0; i < 4; i++)
	if (PC_SLICE_TYPE (mbr, i) && PC_SLICE_START (mbr, i) - 1 < size)
	  {
	    errnum = ERR_NO_DISK_SPACE;
	    return 0;
	  }
      
#if 0	/* !!!! The obsolete EZ-BIOS no longer supported !!!! */
      
      /* Check for EZ-BIOS signature. It should be in the third
       * sector, but due to remapping it can appear in the second, so
       * load and check both.  
       */
      if (! rawread (current_drive, 1, 0, 2 * SECTOR_SIZE, ezbios_check))
	return 0;

      if (! memcmp (ezbios_check + 3, "AERMH", 5)
	  || ! memcmp (ezbios_check + 512 + 3, "AERMH", 5))
	{
	  /* The space after the MBR is used by EZ-BIOS which we must 
	   * not overwrite.
	   */
	  errnum = ERR_NO_DISK_SPACE;
	  return 0;
	}
#endif

      sector = 1;
#undef mbr
    }
  else
    {
      /* Embed it in the bootloader block in the filesystem.  */
      unsigned long start_sector;
      
      /* Open the partition.  */
      if (! open_device ())
	return 0;

      /* Check if the current slice supports embedding.  */
      if (fsys_table[fsys_type].embed_func == 0
	  || ! fsys_table[fsys_type].embed_func (&start_sector, size))
	{
	  errnum = ERR_DEV_VALUES;
	  return 0;
	}

      sector = part_start + start_sector;
    }

  /* Clear the cache.  */
  buf_track = -1;

  /* Now perform the embedding.  */
  if (! devwrite ((unsigned long)(sector - part_start), size, stage1_5_buffer))
    return 0;
  
  grub_printf (" %d sectors are embedded.\n", (unsigned long)size);
  grub_sprintf (embed_info, "%d+%d", (unsigned long)(sector - part_start), (unsigned long)size);
  return 1;
}

static struct builtin builtin_embed =
{
  "embed",
  embed_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "embed STAGE1_5 DEVICE",
  "Embed the Stage 1.5 STAGE1_5 in the sectors after MBR if DEVICE"
  " is a drive, or in the \"bootloader\" area if DEVICE is a FFS partition."
  " Print the number of sectors which STAGE1_5 occupies if successful."
};
#endif /* GRUB_UTIL */


/* errnum */
int
errnum_func (char *arg, int flags)
{
  if (debug > 0)
      grub_printf (" ERRNUM is %d\n", errnum);
  return errnum;
}

static struct builtin builtin_errnum =
{
  "errnum",
  errnum_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "errnum",
  "Return the error number."
};


/* errorcheck */
static int
errorcheck_func (char *arg, int flags)
{

  /* If ARG is empty, toggle the flag.  */
  if (! *arg)
  {
    errorcheck = ! errorcheck;
    if (debug > 0)
      grub_printf (" Error check is toggled now to be %s\n", (errorcheck ? "on" : "off"));
  }
  else if (grub_memcmp (arg, "on", 2) == 0)
    errorcheck = 1;
  else if (grub_memcmp (arg, "off", 3) == 0)
    errorcheck = 0;
  else if (grub_memcmp (arg, "status", 6) == 0)
  {
    if (debug > 0)
      grub_printf (" Error check is now %s\n", (errorcheck ? "on" : "off"));
  }
  else
      errnum = ERR_BAD_ARGUMENT;

  return errorcheck;
}

static struct builtin builtin_errorcheck =
{
  "errorcheck",
  errorcheck_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "errorcheck [on | off | status]",
  "Turn on/off or display the error check mode, or toggle it if no argument."
};


/* fallback */
static int
fallback_func (char *arg, int flags)
{
  unsigned long i = 0;
  int go=0;
  if (memcmp(arg,"--go",4) == 0)
	{
		go = 1;
		arg = skip_to (0, arg);
	}
  while (*arg)
    {
      unsigned long long entry;
      unsigned long j;
      
      if (! safe_parse_maxint (&arg, &entry))
	return 0;

      /* Remove duplications to prevent infinite looping.  */
      for (j = 0; j < i; j++)
	if (entry == fallback_entries[j])
	  break;
      if (j != i)
	continue;
      
      fallback_entries[i++] = entry;
      if (i == MAX_FALLBACK_ENTRIES)
	break;
      
      arg = skip_to (0, arg);
    }

  if (i < MAX_FALLBACK_ENTRIES)
    fallback_entries[i] = -1;

  fallback_entryno = (i == 0) ? -1 : 0;
  if (go) return (errnum = MAX_ERR_NUM);
  return 1;
}

static struct builtin builtin_fallback =
{
  "fallback",
  fallback_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
#if 1
  "fallback NUM...",
  "Go into unattended boot mode: if the default boot entry has any"
  " errors, instead of waiting for the user to do anything, it"
  " immediately starts over using the NUM entry (same numbering as the"
  " `default' command). This obviously won't help if the machine"
  " was rebooted by a kernel that GRUB loaded."
#endif
};


/* command */
static char command_path[128]="(bd)/BOOT/GRUB/";
static int command_path_len = 15;
#define GRUB_MOD_ADDR 0x1200000
#define UTF8_BAT_SIGN 0x54414221BFBBEFULL
#define BAT_SIGN 0x54414221UL
struct exec_array
{
	char name[12];
	unsigned long len;
	char data[];
} *p_exec;

unsigned int mod_end = GRUB_MOD_ADDR;

static struct exec_array *grub_mod_find(const char *name)
{
	struct exec_array *p_mod = (struct exec_array *)GRUB_MOD_ADDR;
	while ((unsigned int)p_mod < mod_end)
	{
		if (substring(name,p_mod->name,1) == 0)
			return p_mod;
		p_mod = (struct exec_array *)((unsigned int)(p_mod->data + p_mod->len + 0xf) & ~0xf);
	}
	return 0;
}

static int grub_mod_add (struct exec_array *mod)
{
   if (grub_mod_find(mod->name) == NULL)
   {
      if (debug > 1)
         grub_printf("insmod:%s...\n",mod->name);
      unsigned long long rd_base_bak = rd_base;
      unsigned long long rd_size_bak = rd_size;
      rd_base = (unsigned long long)(unsigned int)mod->data;
      rd_size = (unsigned long long)mod->len;
      buf_drive = -1;
      grub_open("(rd)+1");
      if ((mod_end + filemax) >= GRUB_MOD_ADDR + 0xF0000)
      {
         grub_close();
         errnum = ERR_WONT_FIT;
         return 0;
      }
      struct exec_array *p_mod = (struct exec_array *)mod_end;
      grub_strcpy(p_mod->name,mod->name);
      p_mod->len = filemax;
      grub_read((unsigned long long)(unsigned int)p_mod->data,-1,GRUB_READ);
      grub_close();
      rd_base = rd_base_bak;
      rd_size = rd_size_bak;
      if (*(unsigned long long *)(int)(p_mod->data + p_mod->len - 8) != 0xBCBAA7BA03051805ULL
         && *(unsigned long *)(int)p_mod->data != BAT_SIGN
         && (*(unsigned long long *)(int)p_mod->data & 0xFFFFFFFFFFFFFFULL) != UTF8_BAT_SIGN) //!BAT with utf-8 BOM 0xBFBBEF
      {
         errnum = ERR_EXEC_FORMAT;
         return 0;
      }
      mod_end = ((unsigned int)p_mod->data + p_mod->len + 0xf) & ~0xf;
      return debug?grub_printf("%s loaded\n",mod->name):1;
   }
   else
      return debug?grub_printf("%s already loaded\n",mod->name):1;
}

static int grub_mod_list(const char *name)
{
   struct exec_array *p_mod = (struct exec_array *)GRUB_MOD_ADDR;
   int ret = 0;
   while ((unsigned int)p_mod < mod_end)
   {
      if (*name == '\0' || substring(name,p_mod->name,1) == 0)
      {
         if (debug)
            grub_printf(" %s\n",p_mod->name);
         ret++;
      }
      p_mod = (struct exec_array *)((unsigned int)(p_mod->data + p_mod->len + 0xf) & ~0xf);
   }
   return ret;
}

static int grub_mod_del(const char *name)
{
   struct exec_array *p_mod;
   for (p_mod = (struct exec_array *)GRUB_MOD_ADDR; (unsigned int)p_mod < mod_end; p_mod = (struct exec_array *)((unsigned int)(p_mod->data + p_mod->len + 0xf) & ~0xf))
   {
      if (substring(name,p_mod->name,1) == 0)
      {
         unsigned int next_mod = ((unsigned int)p_mod->data + p_mod->len + 0xf) & ~0xf;
         if (next_mod == mod_end)
            mod_end = (unsigned int)p_mod;
         else
         {
            memmove(p_mod,(char *)next_mod,mod_end - next_mod);
            mod_end -= next_mod - (unsigned int)p_mod;
         }
         return debug?grub_printf("%s unloaded.\n",name):1;
      }
   }
   return 0;
}

static int grub_exec_run(char *program, int flags);
static int command_open(char *arg,int flags)
{
   if (*arg == '(' || *arg == '/')
      return grub_open(arg);

   if (skip_to(0,arg) - arg > 120)
      return 0;
   if (flags == 0 && (p_exec = grub_mod_find(arg)))
      return 2;
   char t_path[512];
   grub_sprintf(t_path,"%s%s",command_path,arg);
   if (grub_open(t_path + command_path_len - 1))
      return 1;
   return grub_open(t_path)?3:0;
}

int
command_func (char *arg, int flags)
{
  while (*arg == ' ' || *arg == '\t') arg++;

  if (! flags)	/* check syntax only */
  {
    if (*arg == '/' || *arg == '(' || *arg == '+' || *arg=='%')
	return 1;
    if (*arg >= '0' && *arg <= '9')
	return 1;
    if (*arg >= 'a' && *arg <= 'z')
	return 1;
    if (*arg >= 'A' && *arg <= 'Z')
	return 1;
    return 0;
  }
  
   if (*arg <= ' ')
   {
      if (debug > 0)
	 printf("Current default path: %s\n",command_path);
      return 20;
   }
	
   if (grub_memcmp (arg, "--set-path=", 11) == 0)
	{
		arg += 11;

		if (! *arg)
		{
			command_path_len = 15;
			return grub_sprintf(command_path,"(bd)/BOOT/GRUB/");
		}

		int j = grub_strlen(arg);

		if (j >= 0x60)
		{
			if (debug > 0)
				printf("Set default command path error: PATH is too long \n");
			return 0;
		}

		grub_memmove(command_path, arg, j + 1);
		if (command_path[j-1] != '/')
			command_path[j++] = '/';
		command_path[j] = 0;
		command_path_len = j;
		return 1;
	}

  /* open the command file. */
  char *filename = arg;
  char file_path[512];
  unsigned long arg_len = grub_strlen(arg);/*get length for build psp */
  char *cmd_arg = skip_to(SKIP_WITH_TERMINATE,arg);/* get argument of command */
  p_exec = NULL;
  switch(command_open(filename,0))
  {
     case 2:
        sprintf(file_path,"(md)/");
        filemax = p_exec->len;
		break;
	 case 3:
		{
		char *p=command_path;
		print_root_device(file_path,1);
		while (*p != '/')
			++p;
		sprintf(file_path+strlen(file_path),"%s",p);
	    break;
		}
	 case 1:
	 	if (*filename != '(')
		{
			print_root_device(file_path,0);
			sprintf(file_path+strlen(file_path),"%s/",saved_dir);
		}
		else
		{
			file_path[0] = '/';
			file_path[1] = 0;
		}
	    break;
     default:
        grub_printf ("Warning! No such command: %s\n", arg);
        errnum = 0;	/* No error, so that old menus will run smoothly. */
        //return 0;
        return 0;/* return 0 indicating a failure or a false case. */
  }
#if 1
	if (filemax < 9ULL)
	{
		errnum = ERR_EXEC_FORMAT;
		goto fail;
	}
#endif


#if 1
	char *psp;
	unsigned long psp_len;
	unsigned long prog_len;
	char *program;
	prog_len = filemax;
	psp_len = ((arg_len + strlen(file_path)+ 16) & ~0xF) + 0x10 + 0x20;
	psp = (char *)grub_malloc(prog_len * 2 + psp_len);

	if (psp == NULL)
	{
		goto fail;
	}

	program = psp + psp_len;//(psp + psp_len) = entry point of program.

	if (p_exec == NULL)
	{
			/* read file to buff and check exec signature. */
		unsigned long long *end_signature = (unsigned long long *)(program + (unsigned long)filemax - 8);
		if ((grub_read ((unsigned long long)(int)program, -1ULL, 0xedde0d90) != filemax))
		{
			if (! errnum)
				errnum = ERR_EXEC_FORMAT;
		}
		#ifndef GRUB_UTIL
		else if (*end_signature == 0x85848F8D0C010512ULL)
		{
			if (filemax < 512 || filemax > 0x80000)
				errnum = ERR_EXEC_FORMAT;
			else if (filemax + 0x10100 > ((*(unsigned short *)0x413) << 10))
				errnum = ERR_WONT_FIT;
			else
			{
				unsigned long ret;
				struct realmode_regs {
					unsigned long edi; // input and output
					unsigned long esi; // input and output
					unsigned long ebp; // input and output
					unsigned long esp; //stack pointer, input
					unsigned long ebx; // input and output
					unsigned long edx; // input and output
					unsigned long ecx; // input and output
					unsigned long eax;// input and output
					unsigned long gs; // input and output
					unsigned long fs; // input and output
					unsigned long es; // input and output
					unsigned long ds; // input and output
					unsigned long ss; //stack segment, input
					unsigned long eip; //instruction pointer, input
					unsigned long cs; //code segment, input
					unsigned long eflags; // input and output
				};

				struct realmode_regs regs;
				ret = grub_strlen (cmd_arg);
				/* first, backup low 640K memory to address 2M */
				grub_memmove ((char *)0x200000, 0, 0xA0000);
				/* copy command-tail */
				if (ret > 126)
					ret = 126;
				if (ret)
					grub_memmove ((char *)0x10081, cmd_arg, ret);
				/* setup offset 0x80 for command-tail count */
				*(char *)0x10080 = ret;
				/* end the command-tail with CR */
				*(char *)(0x10081 + ret) = 0x0D;

				/* clear the beginning word of DOS PSP. the program
				 * check it and see it is running under grub4dos.
				 * a normal DOS PSP should begin with "CD 20".
				 */
				*(short *)0x10000 = 0;

				/* copy program to 1000:0100 */
				grub_memmove ((char *)0x10100, (char *)program, filemax);

				/* setup DS, ES, CS:IP */
				regs.cs = regs.ds = regs.es = 0x1000;
				regs.eip = 0x100;

				/* setup FS, GS, EFLAGS and stack */
				regs.ss = regs.esp = regs.fs = regs.gs = regs.eflags = -1;

				/* for 64K .com style command, setup stack */
				if (filemax < 0xFF00)
				{
					regs.ss = 0x1000;
					regs.esp = 0xFFFE;
				}
				grub_free(psp);
				grub_close();
				ret = realmode_run ((unsigned long)&regs);
				/* restore memory 0x10000 - 0xA0000 */
				grub_memmove ((char *)0x10000, (char *)0x210000, ((*(unsigned short *)0x413) << 10) - 0x10000);
				return ret;
			}
		}
		#endif
		else if (*end_signature != 0xBCBAA7BA03051805ULL && 
				  *(unsigned long *)program != BAT_SIGN &&
				  (*(unsigned long long *)program & 0xFFFFFFFFFFFFFFULL) != UTF8_BAT_SIGN)
		{
			errnum = ERR_EXEC_FORMAT;
		}
		grub_close ();
		if (errnum)
		{
		   grub_free(psp);
		   return 0;
		}
	}
	else
	{
		grub_memmove(program,p_exec->data,prog_len);
	}

	program[prog_len] = '\0';
	grub_memset(psp, 0, psp_len);
	grub_memmove (psp + 16, arg , arg_len + 1);/* copy args into somewhere in PSP. */
	filename = psp + 16 + arg_len + 1;
	grub_strcpy(filename,file_path);
	*(unsigned long *)psp = psp_len;
	*(unsigned long *)(program - 4) = psp_len;		/* PSP length in bytes. it is in both the starting dword and the ending dword of the PSP. */
	*(unsigned long *)(program - 8) = psp_len - 16 - (cmd_arg - arg);	/* args is here. */
	*(unsigned long *)(program - 12) = flags;		/* flags is here. */
	*(unsigned long *)(program - 16) = psp_len - 16;/*program filename here.*/
	*(unsigned long *)(program - 20) = prog_len;//program length
	*(unsigned long *)(program - 24) = program - filename; 
	/* (free_mem_start + pid - 16) is reserved for full pathname of the program file. */
	int pid;
	pid = grub_exec_run(program, flags);
	/* on exit, release the memory. */
	grub_free(psp);
	if (errnum == -1)//errnum = -1 on exit run.
	{
		errnum = 0;
		pid = run_line(CMD_RUN_ON_EXIT,flags);
	}
	return pid;
#endif
#if 0
	/* check if we have enough memory. */
  {
	unsigned long long memory_needed;

	arg = skip_to (0, arg);		/* get argument of command */
	memory_needed = filemax + ((grub_strlen (arg) + 16) & ~0xF) + 16 + 16;

	/* The amount of free memory is (free_mem_end - free_mem_start) */

	if (memory_needed > (free_mem_end - free_mem_start))
	{
		errnum = ERR_WONT_FIT;
		goto fail;
	}
  }

  /* Where is the free memory? build PSP, load the executable image. */
  {
	unsigned long pid = 255;
	unsigned long j;
	unsigned long psp_len;
	for (j = 1; &mem_alloc_array_start[j] < mem_alloc_array_end && mem_alloc_array_start[j].addr; j++)
	    if (pid < mem_alloc_array_start[j].pid)
		pid = mem_alloc_array_start[j].pid;
	pid++;	/* new pid. */

	/* j - 1 is possibly for free memory start */
	--j;

	if ((mem_alloc_array_start[j].addr & 0x01))	/* no free memory. */
	{
		errnum = ERR_WONT_FIT;
		goto fail;
	}

	/* check sanity */
	if ((mem_alloc_array_start[j].addr & 0xFFFFFFF0) != free_mem_start)
	{
		errnum = ERR_INTERNAL_CHECK;
		goto fail;
	}

	((unsigned long *)free_mem_start)[0] = psp_len = ((grub_strlen (arg) + 16) & ~0xF) + 16 + 16;
	grub_memmove ((char *)(free_mem_start + 16), arg, grub_strlen (arg) + 1);	/* copy args into somewhere in PSP. */
	*(unsigned long *)(free_mem_start + psp_len - 4) = psp_len;		/* PSP length in bytes. it is in both the starting dword and the ending dword of the PSP. */
	*(unsigned long *)(free_mem_start + psp_len - 8) = psp_len - 16;	/* args is here. */
	*(unsigned long *)(free_mem_start + psp_len - 12) = flags;		/* flags is here. */
	/* (free_mem_start + pid - 16) is reserved for full pathname of the program file. */

	/* kernel image is destroyed, so invalidate the kernel */
	if (kernel_type < KERNEL_TYPE_CHAINLOADER)
	    kernel_type = KERNEL_TYPE_NONE;

	if (grub_read ((unsigned long long)(free_mem_start + psp_len), -1ULL, 0xedde0d90) != filemax)
	{
		if (! errnum)
			errnum = ERR_EXEC_FORMAT;
		goto fail;
	}
	grub_close ();

	/*Is a batch file? */
	if (*(unsigned long *)(free_mem_start + psp_len) == BAT_SIGN)//!BAT
	{
		mem_alloc_array_start[j].addr |= 0x01;
		mem_alloc_array_start[j].pid = pid;
		psp_len += free_mem_start;
		free_mem_start = psp_len + filemax + 0x1f;
		*(int *)(int)(psp_len + filemax) = 0;
		free_mem_start &= 0xFFFFFFF0;
		mem_alloc_array_start[j+1].addr = free_mem_start;/*next mem_alloc_array_start*/
		mem_alloc_array_start[j+2].addr = 0;/* end the mem_alloc_array */
		/*run batch script*/
		pid = script_run((char *)(int)psp_len, flags);
		/*release memory. */
		mem_alloc_array_start[j].pid = 0;
		mem_alloc_array_start[j+1].addr = 0;
		free_mem_start = (mem_alloc_array_start[j].addr &= 0xFFFFFFF0);
		return pid;
	}

	/* check exec signature. */
	if (*(unsigned long long *)(int)(free_mem_start + psp_len + filemax - 8) != 0xBCBAA7BA03051805ULL)
		return ! (errnum = ERR_EXEC_FORMAT);

	/* allocate all memory to the program. */
	mem_alloc_array_start[j].addr |= 0x01;	/* the memory is now in use. */
	mem_alloc_array_start[j].pid = pid;	/* with this pid. */

	psp_len += free_mem_start;
	free_mem_start = free_mem_end;		/* no free memory for other programs. */

	/* call the new program.  (free_mem_start + psp_len) = entry point of program. */
	pid = ((int (*)(void))(psp_len))();	/* pid holds return value. */

	/* on exit, release the memory. */
	for (j = 1; &mem_alloc_array_start[j] < mem_alloc_array_end && mem_alloc_array_start[j].addr; j++)
		;
	--j;
	mem_alloc_array_start[j].pid = 0;
	free_mem_start = (mem_alloc_array_start[j].addr &= 0xFFFFFFF0);
	return pid;
  }
#endif
  //return 1;

fail:

  grub_close ();
  return 0;
}

static struct builtin builtin_command =
{
  "command",
  command_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "command [--set-path=PATH] FILE [ARGS]",
  "Run executable file FILE with arguments ARGS."
  "--set-path sets a search PATH for executable files,default is (bd)/boot/grub."
};

static int insmod_func(char *arg,int flags)
{
   if (arg == NULL || *arg == '\0')
      return 0;
   if (substring(skip_to(0,arg) - 4,".mod",1) == 0)
   {
      if (!command_open(arg,1))
         return 0;
      char *buff=grub_malloc(filemax);
      if (!buff)
      {
        grub_close();
        return 0;
      }
      #if 0
      if (grub_read((unsigned long long)(unsigned int)buff,16,GRUB_READ) != 16
        || *(unsigned long long *)(unsigned int)buff != 0xBCBAA7BA03051805LL
        || grub_read((unsigned long long)(unsigned int)buff,-1,GRUB_READ) == 0 )
      {
         grub_close();
         grub_free(buff);
         errnum = ERR_EXEC_FORMAT;
         return 0;
      }
      #else
      if (grub_read((unsigned long long)(unsigned int)buff,-1,GRUB_READ) != filemax)
      {
        grub_close();
        grub_free(buff);
        return 0;
      }
      #endif
      grub_close();
      char *buff_end = buff+filemax;
      struct exec_array *p_mod = (struct exec_array *)buff;
      //skip grub4dos moduld head.
      if (strcmp(p_mod->name,"\x05\x18\x05\x03\xBA\xA7\xBA\xBC") == 0)
        ++p_mod;
      while ((char *)p_mod < buff_end && grub_mod_add(p_mod))
      {
         p_mod = (struct exec_array *)(p_mod->data + p_mod->len);
      }
      grub_free(buff);
      return 1;
   }
   switch(command_open(arg,0))
   {
      case 2:
         return debug?grub_printf("%s already loaded\n",arg):1;
      case 0:
      case -1:
         return 0;
      default:
         {
			 struct exec_array *p_mod = grub_malloc(filemax + sizeof(struct exec_array));
			 char *filename = arg;
			 int ret = 0;
			 if (p_mod == NULL || grub_read((unsigned long long)(unsigned int)p_mod->data,-1,GRUB_READ) != filemax)
				goto exit;
			 p_mod->len = filemax;
			 if (*arg == '(' || *arg == '/')
			 {
				while (*arg)
				{
				   if (*arg++ == '/')
					  filename = arg;
				}
			 }
			 if (strlen(filename) > 11)
			 {
				grub_printf("Err filename\n");
			 }
			 else
			 {
				grub_strcpy(p_mod->name,filename);
				ret = grub_mod_add(p_mod);
			 }

			 exit:
			 grub_close();
			 grub_free(p_mod);
			 return ret;
         }
   }
}

static struct builtin builtin_insmod =
{
   "insmod",
   insmod_func,
   BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
   "insmod MODFILE|FILE.MOD",
   "FILE.MOD is MODFILE package, it has multiple MODFILE"
};

static int delmod_func(char *arg,int flags)
{
   if (*arg == '\0')
      return grub_mod_list(arg);
   if (grub_memcmp(arg,"-l",2) == 0)
   {
      arg = skip_to(0,arg);
      return grub_mod_list(arg);
   }

   if (*arg == '*')
   {
      mod_end = GRUB_MOD_ADDR;
      return 1;
   }

   return grub_mod_del(arg);
}

static struct builtin builtin_delmod =
{
   "delmod",
   delmod_func,
   BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
   "delmod modname|*",
};


/* commandline */
int
commandline_func (char *arg, int flags)
{
  int forever = 0;
  char *config_entries = arg;

  enter_cmdline(config_entries, forever);

  return 1;
}

static struct builtin builtin_commandline =
{
  "commandline",
  commandline_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
#if 1
  "commandline",
  "Enter command-line prompt mode."
#endif
};


extern int bsd_evil_hack;

extern unsigned long next_partition_drive;
extern unsigned long next_partition_dest;
extern unsigned long *next_partition_partition;
extern unsigned long *next_partition_type;
extern unsigned long *next_partition_start;
extern unsigned long *next_partition_len;
extern unsigned long *next_partition_offset;
extern unsigned long *next_partition_entry;
extern unsigned long *next_partition_ext_offset;
extern char *next_partition_buf;

//extern unsigned long dest_partition;
//static unsigned long entry;
//static unsigned long ext_offset;

/* Hide/Unhide CURRENT_PARTITION.  */
static int
set_partition_hidden_flag (int hidden)
{
  unsigned long part = 0xFFFFFF;
  unsigned long start, len, offset, ext_offset1;
  unsigned long type, entry1;
#ifdef GRUB_UTIL
//  char mbr[512];
#endif
  
  /* The drive must be a hard disk.  */
  if (! (current_drive & 0x80))
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  
  /* The partition must be a PC slice.  */
  if ((current_partition >> 16) == 0xFF
      || (current_partition & 0xFFFF) != 0xFFFF)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  
  /* Look for the partition.  */
  while ((	next_partition_drive		= current_drive,
		next_partition_dest		= 0xFFFFFF,
		next_partition_partition	= &part,
		next_partition_type		= &type,
		next_partition_start		= &start,
		next_partition_len		= &len,
		next_partition_offset		= &offset,
		next_partition_entry		= &entry1,
		next_partition_ext_offset	= &ext_offset1,
		next_partition_buf		= mbr,
		next_partition ()))
//  while (next_partition (current_drive, 0xFFFFFF, &part, &type,
//			 &start, &len, &offset, &entry,
//			 &ext_offset, mbr))
    {                                                                       
      if (part == current_partition)
	{
	  int is_hidden = (PC_SLICE_TYPE (mbr, entry1) & PC_SLICE_TYPE_HIDDEN_FLAG);
	  /* Found.  */
	  if (hidden == -1)	/* status only */
	  {
		if (debug > 0)
			grub_printf ("Partition (hd%d,%d) is %shidden.\n", (current_drive & 0x7F), (unsigned long)(unsigned char)(part>>16), (is_hidden ? "" : "not "));
		
		return is_hidden;
	  }

	  /* Check if the hidden flag need change.  */
	  if ((!hidden) != (!is_hidden))
	  {
		if (hidden)
			PC_SLICE_TYPE (mbr, entry1) |= PC_SLICE_TYPE_HIDDEN_FLAG;
		else
			PC_SLICE_TYPE (mbr, entry1) &= ~PC_SLICE_TYPE_HIDDEN_FLAG;       
	  
		/* Write back the MBR to the disk.  */
		buf_track = -1;
		if (! rawwrite (current_drive, offset, mbr))
			return 0;

		if (debug > 0)
			grub_printf ("Partition (hd%d,%d) successfully set %shidden.\n", (current_drive & 0x7F), (unsigned long)(unsigned char)(part>>16), (hidden ? "" : "un"));
	  } else {
		if (debug > 0)
			grub_printf ("Partition (hd%d,%d) was already %shidden.\n", (current_drive & 0x7F), (unsigned long)(unsigned char)(part>>16), (hidden ? "" : "un"));

	  }
	  
	  /* Succeed.  */
	  return 1;
	}
    }
  
  return 0;
}



static int real_root_func (char *arg1, int attempt_mnt);
/* find */
/* Search for the filename ARG in all of partitions and optionally make that
 * partition root("--set-root", Thanks to Chris Semler <csemler@mail.com>).
 */
//static void print_root_device (char *buffer);
static int find_check(char *filename,struct builtin *builtin1,char *arg,int flags)
{
	saved_drive = current_drive;
	saved_partition = current_partition;
	if (filename == NULL || (open_device() && grub_open (filename)))
	{
		grub_close ();
		if (builtin1)
		{
			int ret = strlen(arg) + 1;
			char *_tmp;
			if ((_tmp = grub_malloc(ret)) == NULL)
				return 0;
			memmove(_tmp,arg,ret );
			buf_drive = -1;
			ret = (builtin1->func) (_tmp, flags);
			grub_free(_tmp);
			if (ret == 0)
				return 0;
		}

		if (debug > 0)
		{
			print_root_device(NULL,0);
			putchar('\n');
		}
		return 1;
	}

	errnum = ERR_NONE;
	return 0;
}

static int
find_func (char *arg, int flags)
{
  struct builtin *builtin1 = 0;
//  int ret;
  char *filename;
  unsigned long drive;
  unsigned long tmp_drive = saved_drive;
  unsigned long tmp_partition = saved_partition;
  unsigned long got_file = 0;
  char *set_root = 0;
  unsigned long ignore_cd = 0;
  unsigned long ignore_floppies = 0;
  unsigned long ignore_oem = 0;
  char find_devices[8]="upnhcf";//find order:ud->pd->nd->hd->cd->fd
  //char *in_drives = NULL;	/* search in drive list */
//  char root_found[16];
  
	for (;;)
	{
		if (grub_memcmp (arg, "--set-root=", 11) == 0)
      {
	set_root = arg + 11;
	if (*set_root && *set_root != ' ' && *set_root != '\t' && *set_root != '/')
		return ! (errnum = ERR_FILENAME_FORMAT);
      }
    else if (grub_memcmp (arg, "--set-root", 10) == 0)
      {
	set_root = "";
      }
    else if (grub_memcmp (arg, "--ignore-cd", 11) == 0)
      {
	ignore_cd = 1;
      }
    else if (grub_memcmp (arg, "--ignore-floppies", 17) == 0)
      {
	ignore_floppies = 1;
      }
	else if (grub_memcmp (arg, "--ignore-oem", 12) == 0)
      {
	ignore_oem = 1;
      }
		else if (grub_memcmp(arg, "--devices=", 10) == 0)
		{
			int i = 0;
			arg += 10;
			while (i < 7 && *arg >= 'a')
			{
				find_devices[i++] = *arg++;
			}
			find_devices[i] = '\0';
		}
		else
			break;
    arg = skip_to (0, arg);
  }
  
  /* The filename should NOT have a DEVICE part. */
  filename = set_device (arg);
  if (filename)
	return ! (errnum = ERR_FILENAME_FORMAT);

  if (*arg == '/' || *arg == '+' || (*arg >= '0' && *arg <= '9'))
  {
    filename = arg;
    arg = skip_to (0, arg);
  } else {
    filename = 0;
  }

  /* arg points to command. */

//  if (*arg >= 'a' && *arg <= 'z')
  if (*arg)
  {
    builtin1 = find_command (arg);
    if ((int)builtin1 != -1)
    if (! builtin1 || ! (builtin1->flags & flags))
    {
	errnum = ERR_UNRECOGNIZED;
	return 0;
    }
    if ((int)builtin1 != -1)
	arg = skip_to (1, arg);	/* get argument of command */
    else
	builtin1 = &builtin_command;
  }
//  else if (*arg)
//  {
//	errnum = ERR_UNRECOGNIZED;
//	return 0;
//  }

  errnum = 0;

	char *devtype = find_devices;
	unsigned int FIND_DRIVES = 0;
	/*check if current root in find_devices list*/
	for (; *devtype; devtype++)
	{
		switch(*devtype)
		{
			case 'h':
				if (tmp_drive >= 0x80 && tmp_drive < 0xA0 && tmp_partition != 0xFFFFFF)
					FIND_DRIVES = 1;
				break;
			case 'u':
				if (tmp_drive == FB_DRIVE)
					FIND_DRIVES = 1;
				break;
			case 'p':
				if (PXE_DRIVE == tmp_drive)
					FIND_DRIVES = 1;
				break;
			case 'c':
				if (ignore_cd)
					*devtype = ' ';
				else if (current_drive == cdrom_drive || (tmp_drive >= 0xa0 && tmp_drive <= 0xff))
					FIND_DRIVES = 1;
				break;
			case 'f':
				if (ignore_floppies)
					*devtype = ' ';
				else if (tmp_drive < 8)
					FIND_DRIVES = 1;
				break;
		}
	}
	/*search in current root device*/
	if (FIND_DRIVES)
	{
		current_drive = saved_drive;
		current_partition = saved_partition;
		if (find_check(filename,builtin1,arg,flags) == 1)
		{
			got_file = 1;
			if (set_root)
				goto found;
		}
	}
	/*search other devices*/
	for (devtype = find_devices; *devtype; devtype++)
	{
		current_partition = 0xFFFFFF;
		switch(*devtype)
		{
#ifdef FSYS_FB
			case 'u':
				if (fb_status)
					current_drive = FB_DRIVE;
				else
					continue;
				break;
#endif
#ifdef SUPPORT_NETBOOT
			case 'n':
				if (network_ready)
					current_drive = NETWORK_DRIVE;
				else
					continue;
				break;
#endif /* SUPPORT_NETBOOT */
#ifdef FSYS_PXE
			case 'p':
				if (pxe_entry)
					current_drive = PXE_DRIVE;
				else
					continue;
				break;
#endif
			case 'c':/*Only search first cdrom*/
				if (cdrom_drive != GRUB_INVALID_DRIVE)
					current_drive = cdrom_drive;
#ifndef GRUB_UTIL
				else if (atapi_dev_count)
					current_drive = min_cdrom_id;
#endif
				else
					continue;
				break;
			case 'h':
			case 'f':
				#ifdef GRUB_UTIL
				#define FIND_HD_DRIVES 8
				#define FIND_FD_DRIVES 8
				#else
				#define FIND_HD_DRIVES  (*((char *)0x475))
				#define FIND_FD_DRIVES  (((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0)
				#endif
				FIND_DRIVES = (*devtype == 'h') ? 0x80 + FIND_HD_DRIVES : FIND_FD_DRIVES;
				for (drive = (*devtype == 'h')?0x80:0; drive < FIND_DRIVES; drive++)
				{
					unsigned long part = 0xFFFFFF;
					unsigned long start, len, offset, ext_offset1;
					unsigned long type, entry1;

					saved_drive = current_drive = drive;
					saved_partition = current_partition = part;
					if ((*devtype == 'f') && open_device()) //if is a partition 
					{
						if ((tmp_drive != current_drive || tmp_partition != current_partition) && find_check(filename,builtin1,arg,flags) == 1)
						{
							got_file = 1;
							if (set_root)
								goto found;
						}
						//continue;
					}

					saved_drive = current_drive = drive;
					while ((	next_partition_drive		= drive,
							next_partition_dest		= 0xFFFFFF,
							next_partition_partition	= &part,
							next_partition_type		= &type,
							next_partition_start		= &start,
							next_partition_len		= &len,
							next_partition_offset		= &offset,
							next_partition_entry		= &entry1,
							next_partition_ext_offset	= &ext_offset1,
							next_partition_buf		= mbr,
							next_partition ()))
					{
						if (type != PC_SLICE_TYPE_NONE
							&& ! (ignore_oem == 1 && (type & ~PC_SLICE_TYPE_HIDDEN_FLAG) == 0x02) 
							&& ! IS_PC_SLICE_TYPE_BSD (type)
							&& ! IS_PC_SLICE_TYPE_EXTENDED (type))
						{
							current_drive = drive;
							current_partition = part;

							if ((tmp_drive != current_drive || tmp_partition != current_partition) && find_check(filename,builtin1,arg,flags) == 1)
							{
								got_file = 1;
								if (set_root)
									goto found;
							}
						} /*end if*/
					} /*while next_partition*/

					#if 0
					saved_drive = current_drive = drive;
					saved_partition = current_partition = 0xFFFFFF;
					if ((*devtype == 'h') && open_device()) //if is a partition 
					{
						if ((tmp_drive != current_drive || tmp_partition != current_partition) && find_check(filename,builtin1,arg,flags) == 1)
						{
							got_file = 1;
							if (set_root)
								goto found;
						}
						//continue;
					}
					#endif

				/* next_partition always sets ERRNUM in the last call, so clear it.  */
					errnum = ERR_NONE;
				}
				#undef FIND_HD_DRIVES
				#undef FIND_FD_DRIVES
				//h,f. no break;default continue;
			default:
				continue;
		}
		if (tmp_drive == current_drive)
			continue;
		if (find_check(filename,builtin1,arg,flags) == 1)
		{
			got_file = 1;
			if (set_root)
				goto found;
		}
	}
#if 0
#ifdef FSYS_FB
	if (fb_status && tmp_drive != FB_DRIVE)
	{
		current_drive = FB_DRIVE;
		current_partition = 0xFFFFFF;
		if (find_check(filename,builtin1,arg,flags) == 1)
		{
			got_file = 1;
			if (set_root)
					goto found;
		}
	}
#endif

	/* CD-ROM.  */
	if (cdrom_drive != GRUB_INVALID_DRIVE && ! ignore_cd && tmp_drive != cdrom_drive)
	{
		current_drive = cdrom_drive;
		current_partition = 0xFFFFFF;

		if (find_check(filename,builtin1,arg,flags) == 1)
		{
			got_file = 1;
			if (set_root)
				goto found;
		}
	}

  /* Hard disks. Search in hard disks first, since floppies are slow */
#ifdef GRUB_UTIL
#define FIND_HD_DRIVES 8
#define FIND_FD_DRIVES 8
#else
#define FIND_HD_DRIVES  (*((char *)0x475))
#define FIND_FD_DRIVES  (((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0)
#endif
	for (current_drive = 0x80; /*drive < 0x80 + FIND_DRIVES*/; current_drive++)
	{
		unsigned long part = 0xFFFFFF;
		unsigned long start, len, offset, ext_offset1;
		unsigned long type, entry1;

		/* Floppies.  */
		if (current_drive >= 0x80 + FIND_HD_DRIVES)
		{
			if (ignore_floppies)
				break;
			current_drive = 0;	/* begin floppy */
		}

		if (current_drive < 0x80 && current_drive >= FIND_FD_DRIVES)
		{
				break;	/* end floppy */
		}

		current_partition = part;
		if (open_device()) //if is a partition 
		{
			if (find_check(filename,builtin1,arg,flags) == 1)
			{
				got_file = 1;
				if (set_root)
					goto found;
			}
			continue;
		}

		while ((	next_partition_drive		= current_drive,
			next_partition_dest		= 0xFFFFFF,
			next_partition_partition	= &part,
			next_partition_type		= &type,
			next_partition_start		= &start,
			next_partition_len		= &len,
			next_partition_offset		= &offset,
			next_partition_entry		= &entry1,
			next_partition_ext_offset	= &ext_offset1,
			next_partition_buf		= mbr,
			next_partition ()))
		{
			if (type != PC_SLICE_TYPE_NONE
				&& ! (ignore_oem == 1 && (type & ~PC_SLICE_TYPE_HIDDEN_FLAG) == 0x02) 
				&& ! IS_PC_SLICE_TYPE_BSD (type)
				&& ! IS_PC_SLICE_TYPE_EXTENDED (type))
			{
				current_partition = part;
				if (find_check(filename,builtin1,arg,flags) == 1)
				{
					got_file = 1;
					if (set_root)
						goto found;
				}
			} /*end if*/
		} /*while next_partition*/

		/* next_partition always sets ERRNUM in the last call, so clear it.  */
		errnum = ERR_NONE;
	}
#endif
	saved_drive = tmp_drive;
	saved_partition = tmp_partition;
found:
	if (got_file)
	{
		errnum = ERR_NONE;
		if (set_root)
		{
			int j;

			//return real_root_func (root_found, 1);
			//saved_drive = current_drive;
			//saved_partition = current_partition;
			/* copy root prefix to saved_dir */
			for (j = 0; j < sizeof (saved_dir); j++)
			{
				 char ch;

				 ch = set_root[j];
				 if (ch == 0 || ch == 0x20 || ch == '\t')
				break;
				 if (ch == '\\')
				 {
				saved_dir[j] = ch;
				j++;
				ch = set_root[j];
				if (! ch || j >= sizeof (saved_dir))
				{
					j--;
					saved_dir[j] = 0;
					break;
				}
				 }
				 saved_dir[j] = ch;
			}

			if (saved_dir[j-1] == '/')
			{
				 saved_dir[j-1] = 0;
			} else
				 saved_dir[j] = 0;
		} //if set_root
		
		return 1;
	}

  errnum = ERR_FILE_NOT_FOUND;
  return 0;
}

static struct builtin builtin_find =
{
  "find",
  find_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "find [--set-root[=DIR]] [--devices=DEVLIST] [--ignore-floppies] [--ignore-cd] [FILENAME] [CONDITION]",
  "Search for the filename FILENAME in all of partitions and print the list of"
  " the devices which contain the file and suffice CONDITION. CONDITION is a"
  " normal grub command, which return non-zero for TRUE and zero for FALSE."
  " DEVLIST specify the search devices and order,the default DEVLIST is upnhcf."
  " DEVLIST must be a combination of these letters (u, p, n, h, c, f)."
  " If the option --set-root is used and FILENAME is found on a device, then"
  " stop the find immediately and set the device as new root."
  " If the option --ignore-floppies is present, the search will bypass all"
  " floppies. And --ignore-cd will skip (cd)."
};


/* uuid */
/* List filesystem UUID in all of partitions or search for filesystem
 * with specified UUID and set the partition as root.
 * Contributed by Jing Liu ( fartersh-1@yahoo.com )
 */
//static void print_root_device (char *buffer);
static void get_uuid (char* uuid_found);
static int
uuid_func (char *arg, int flags)
{
  unsigned long drive;
//  unsigned long tmp_drive = saved_drive;
//  unsigned long tmp_partition = saved_partition;
  char root_found[16] = "";
  char uuid_found[100];

	if (*arg == '(')
	{
		set_device (arg);
		if (errnum)
			return 0;
		if (! open_device ())
			return 0;
//		saved_drive = current_drive;
//		saved_partition = current_partition;
		grub_memset (uuid_found, 0, 100);
                if (errnum != ERR_FSYS_MOUNT && fsys_type < NUM_FSYS)
			get_uuid (uuid_found);
		arg = skip_to (0, arg);
		if (! *arg )
		{
			/* Print the type of the filesystem.  */
			if (debug > 0)
			{
				print_root_device (NULL,1);
				grub_printf (": UUID is %s\n\t", ((*uuid_found) ? uuid_found : "(unsupported)"));
				print_fsys_type ();
			}
//			saved_drive = tmp_drive;
//			saved_partition = tmp_partition;
			errnum = ERR_NONE;
			sprintf(ADDR_RET_STR,uuid_found);
			return (*uuid_found);
		}
//		saved_drive = tmp_drive;
//		saved_partition = tmp_partition;
		errnum = ERR_NONE;
		return ! grub_strcmp ((char*)uuid_found, arg);
	}

//  if (! *arg && (flags & (BUILTIN_MENU | BUILTIN_SCRIPT)))
//    {
//      if (debug > 0)
//        grub_printf ("uuid must be used with UUID argument in menu.lst.\n");
//	return ! (errnum = ERR_BAD_ARGUMENT);
//    }

  errnum = 0;
  /* Search in hard disks first, since floppies are slow */
#ifdef GRUB_UTIL
#define FIND_DRIVES 8
#else
#define FIND_DRIVES (*((char *)0x475))
#endif
  for (drive = 0x80; drive < 0x80 + FIND_DRIVES; drive++)
#undef FIND_DRIVES
    {
      unsigned long part = 0xFFFFFF;
      unsigned long start, len, offset, ext_offset1;
      unsigned long type, entry1;

      current_drive = drive;
      while ((	next_partition_drive		= drive,
		next_partition_dest		= 0xFFFFFF,
		next_partition_partition	= &part,
		next_partition_type		= &type,
		next_partition_start		= &start,
		next_partition_len		= &len,
		next_partition_offset		= &offset,
		next_partition_entry		= &entry1,
		next_partition_ext_offset	= &ext_offset1,
		next_partition_buf		= mbr,
		next_partition ()))
	{
	  if (type != PC_SLICE_TYPE_NONE
	      && ! IS_PC_SLICE_TYPE_BSD (type)
	      && ! IS_PC_SLICE_TYPE_EXTENDED (type))
	    {
	      current_partition = part;
	      if (open_device ())
		{
		  int bsd_part = (part >> 8) & 0xFF;
		  int pc_slice = part >> 16;
//		  saved_drive = current_drive;
//		  saved_partition = current_partition;
		  grub_memset(uuid_found, 0, 100);
                  if (errnum != ERR_FSYS_MOUNT && fsys_type < NUM_FSYS)
                    get_uuid(uuid_found);
		  if (*uuid_found)
                    {
                      if (! *arg )
                        {
		          if (bsd_part == 0xFF)
			    grub_printf("(hd%d,%d): UUID is %s\n\t", (drive - 0x80), (unsigned long)(unsigned char)(part >> 16), uuid_found);
		          else
			    grub_printf ("(hd%d,%d,%c): UUID is %s\n\t",
                                          (drive - 0x80), pc_slice, (bsd_part + 'a'), uuid_found);
                          print_fsys_type();
		        }
                      else if (grub_strcmp((char*)uuid_found,arg) == 0)
                        {
		          if (bsd_part == 0xFF)
                            grub_sprintf(root_found,"(hd%d,%d)", (drive - 0x80), (unsigned long)(unsigned char)(part >> 16));
                          else
                            grub_sprintf(root_found,"(hd%d,%d,%c)",
			                  (drive - 0x80), pc_slice, (bsd_part + 'a'));
                          goto found;
                        }
                    }
                  else if (debug > 0)
                    {
		      if (bsd_part == 0xFF)
                        grub_printf("(hd%d,%d): UUID is (unsupported)\n\t", (drive - 0x80), (unsigned long)(unsigned char)(part >> 16));
                      else
                        grub_printf("(hd%d,%d,%c): UUID is (unsupported)\n\t",
                                     (drive - 0x80), pc_slice, (bsd_part + 'a'));
                      print_fsys_type();
                    }
                }
	    }

	  /* We want to ignore any error here.  */
	  errnum = ERR_NONE;
	}

      /* next_partition always sets ERRNUM in the last call, so clear it.  */
      errnum = ERR_NONE;
    }

  /* Floppies.  */
#ifdef GRUB_UTIL
#define FIND_DRIVES 8
#else
#define FIND_DRIVES (((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0)
#endif
  for (drive = 0; drive < 0 + FIND_DRIVES; drive++)
#undef FIND_DRIVES
    {
      current_drive = drive;
      current_partition = 0xFFFFFF;
      if (open_device ())
	{
//	  saved_drive = current_drive;
//	  saved_partition = current_partition;
          grub_memset(uuid_found, 0, 100);
          if (errnum != ERR_FSYS_MOUNT && fsys_type < NUM_FSYS)
            get_uuid(uuid_found);
          if (*uuid_found)
            {
              if (! *arg )
                {
                  grub_printf("(fd%d): UUID is %s\n\t", drive, uuid_found);
                  print_fsys_type();
                }
              else if (grub_strcmp((char*)uuid_found,arg) == 0)
                {
                  grub_sprintf(root_found,"(fd%d)", drive);
                  goto found;
                }
            }
          else if (debug > 0)
            {
              grub_printf("(fd%d): UUID is (unsupported)\n\t", drive);
              print_fsys_type();
            }
	}
      errnum = ERR_NONE;
  }

found:
//  saved_drive = tmp_drive;
//  saved_partition = tmp_partition;
  errnum = ERR_NONE;
  if (! *arg)
    return 1;
        
  if (*root_found)
    {
      if (debug > 0)
        grub_printf("setting root to %s\n", root_found);
      return real_root_func(root_found,1);
    }

  errnum = ERR_NO_PART;
  return 0;
}

static struct builtin builtin_uuid =
{
  "uuid",
  uuid_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "uuid [DEVICE] [UUID]",
  "If DEVICE is not specified, search for filesystem with UUID in all"
  " partitions and set the partition containing the filesystem as new"
  " root(if UUID is specified), or just list uuid's of all filesystems"
  " on all devices(if UUID is not specified). If DEVICE is specified," 
  " return true or false according to whether or not the DEVICE matches"
  " the specified UUID(if UUID is specified), or just list the uuid of"
  " DEVICE(if UUID is not specified)."
};

static void
get_uuid (char* uuid_found)
{
  unsigned char uuid[32] = "";
    {
#ifdef FSYS_FAT
      if (grub_memcmp(fsys_table[fsys_type].name, "fat", 3) == 0)
        {
          char fat_length[2];
          devread(0, 0x16, 2, (unsigned long long)(unsigned long)fat_length, 0xedde0d90);
          if (*(unsigned short*)fat_length)
            // FAT12/16
            devread(0, 0x27, 4, (unsigned long long)(unsigned long)uuid, 0xedde0d90);
          else
            // FAT32
            devread(0, 0x43, 4, (unsigned long long)(unsigned long)uuid, 0xedde0d90);
          grub_sprintf(uuid_found, "%02X%02X-%02X%02X", uuid[3], uuid[2], uuid[1], uuid[0]);
          return;
        }  
#endif
#ifdef FSYS_NTFS
      if (grub_memcmp(fsys_table[fsys_type].name, "ntfs", 4) == 0)
        {
          devread(0, 0x48, 8, (unsigned long long)(unsigned long)uuid, 0xedde0d90);
          grub_sprintf(uuid_found, "%02X%02X%02X%02X%02X%02X%02X%02X", uuid[7], uuid[6], uuid[5], uuid[4], uuid[3], uuid[2], uuid[1], uuid[0]);
          return;
        }
#endif
#ifdef FSYS_EXT2FS
      if (grub_memcmp(fsys_table[fsys_type].name, "ext2fs", 6) == 0)
        {
          devread(2, 0x68, 16, (unsigned long long)(unsigned long)uuid, 0xedde0d90);
          grub_sprintf(uuid_found, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
          return;
        }
#endif
#ifdef FSYS_REISERFS
      if (grub_memcmp(fsys_table[fsys_type].name, "reiserfs", 8) == 0)
        {
          char version[9];
          devread(0x10, 52, 9, (unsigned long long)(unsigned long)version, 0xedde0d90);
          if (grub_memcmp(version, "ReIsEr2Fs", 9) == 0 || grub_memcmp(version, "ReIsEr3Fs", 9) == 0)
            devread(0x10, 84, 16, (unsigned long long)(unsigned long)uuid, 0xedde0d90);
          else
            {
              devread(0x10, 0, 7, (unsigned long long)(unsigned long)version, 0xedde0d90);
              if (grub_memcmp(version, "ReIsEr4", 7) == 0)
                devread(0x10, 20, 16, (unsigned long long)(unsigned long)uuid, 0xedde0d90);
              else
                return;
            }
          grub_sprintf(uuid_found, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
          return;
        }
#endif
#ifdef FSYS_JFS
      if (grub_memcmp(fsys_table[fsys_type].name, "jfs", 3) == 0)
        {
          devread(0x40, 136, 16, (unsigned long long)(unsigned long)uuid, 0xedde0d90);
          grub_sprintf(uuid_found, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
          return;
        }
#endif
#ifdef FSYS_XFS
      if (grub_memcmp(fsys_table[fsys_type].name, "xfs", 3) == 0)
        {
          devread(2, 32, 16, (unsigned long long)(unsigned long)uuid, 0xedde0d90);
          grub_sprintf(uuid_found, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
          return;
        }
#endif
    }
}


/* fstest */
static int
fstest_func (char *arg, int flags)
{

  /* If ARG is empty, toggle the flag.  */
  if (! *arg)
  {
    if (disk_read_hook)
    {
      disk_read_hook = NULL;
      if (debug > 0)
	printf (" Filesystem tracing is now off\n");
    }else{
      disk_read_hook = disk_read_print_func;
      if (debug > 0)
	printf (" Filesystem tracing is now on\n");
    }
  }
  else if (grub_memcmp (arg, "on", 2) == 0)
    disk_read_hook = disk_read_print_func;
  else if (grub_memcmp (arg, "off", 3) == 0)
    disk_read_hook = NULL;
  else if (grub_memcmp (arg, "status", 6) == 0)
  {
    if (debug > 0)
      grub_printf (" Filesystem tracing is now %s\n", (disk_read_hook ? "on" : "off"));
  }
  else
      errnum = ERR_BAD_ARGUMENT;

  return (int)disk_read_hook;
}

static struct builtin builtin_fstest =
{
  "fstest",
  fstest_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "fstest [on | off | status]",
  "Turn on/off or display the fstest mode, or toggle it if no argument."
};

#ifdef SUPPORT_GFX

/* graphics */
static int
gfxmenu_func (char *arg, int flags)
{
    //if (*arg)
    //{
	/* filename can only be 64 characters due to our buffer size */
	if (strlen(arg) > 63)
		return ! (errnum = ERR_WONT_FIT);
    
	if (! grub_open(arg))
		return 0;
	grub_close();
    //}

    strcpy(graphics_file, arg);

  //memmove(graphics_file, arg, sizeof graphics_file - 1);  
  //graphics_file[sizeof graphics_file - 1] = 0;
  gfx_drive = saved_drive;
  gfx_partition = saved_partition;

  return 1;
}

static struct builtin builtin_gfxmenu =
{
  "gfxmenu",
  gfxmenu_func,
  BUILTIN_MENU | BUILTIN_HELP_LIST,
  "gfxmenu FILE",
  "Use the graphical menu from FILE."
};
#endif


/* geometry */
static int
geometry_func (char *arg, int flags)
{
//  struct geometry tmp_geom;
  char *msg;
//  char *device = arg;
//#ifdef GRUB_UTIL
//  char *ptr;
//#endif

#ifndef GRUB_UTIL
  int sync = 0;

  force_geometry_tune = 0;
  for (;;)
  {
    if (grub_memcmp (arg, "--tune", 6) == 0)
    {
      if (force_geometry_tune) 
	  return (errnum = ERR_BAD_ARGUMENT, 0);
      force_geometry_tune = 1;
    }
    else if (grub_memcmp (arg, "--bios", 6) == 0)
    {
      if (force_geometry_tune) 
	  return (errnum = ERR_BAD_ARGUMENT, 0);
      force_geometry_tune = 2;
    }
    else if (grub_memcmp (arg, "--sync", 6) == 0)
    {
      sync = 1;
    }
    else
	break;
    arg = skip_to (0, arg);
  }
#endif

  /* Get the drive and the partition.  */
  if (! *arg || *arg == ' ' || *arg == '\t')
    {
	current_drive = saved_drive;
	current_partition = saved_partition;
    }
  else
    {
      if (! set_device (arg))
	return 0;
    }

//  /* Get the device number.  */
//  set_device (device);
//  if (errnum)
//    return 0;

  /* Check for the geometry.  */
  if (get_diskinfo (current_drive, &tmp_geom))
    {
#ifndef GRUB_UTIL
      force_geometry_tune = 0;
#endif
      errnum = ERR_NO_DISK;
      return 0;
    }
#ifndef GRUB_UTIL
  force_geometry_tune = 0;
#endif

#if 0
	/* The situation described as follows should never occur. So comment
	 * out. Another reason to comment out: the SCRATCHSEG has only 512
	 * byte room and it cannot hold a large 2048-byte cdrom sector.
	 *
	 *		Commented out 2008-09-30 by Tinybit.
	 */

  /* Attempt to read the first sector, because some BIOSes turns out not
     to support LBA even though they set the bit 0 in the support
     bitmap, only after reading something actually.  */
#ifdef GRUB_UTIL
  if (current_drive != cdrom_drive)
#else
  if (current_drive != cdrom_drive && (current_drive < (unsigned char)min_cdrom_id || current_drive >= (unsigned char)(min_cdrom_id + atapi_dev_count)))
#endif /* GRUB_UTIL */
  {
    if (biosdisk (BIOSDISK_READ, current_drive, &tmp_geom, 0, 1, SCRATCHSEG))
    {
	if (debug > 1)
		printf("biosdisk read first sector of drive 0x%X: failure! errnum=%d\n", current_drive, errnum);
	errnum = 0;
//	errnum = ERR_READ;
//	return 0;
    }
  }
#endif

#ifdef GRUB_UTIL
  arg/*ptr*/ = skip_to (0, arg/*device*/);
  if (*arg/*ptr*/)
    {
      char *cylinder, *head, *sector, *total_sector;
      unsigned long long num_cylinder, num_head, num_sector, num_total_sector;

      cylinder = arg/*ptr*/;
      head = skip_to (0, cylinder);
      sector = skip_to (0, head);
      total_sector = skip_to (0, sector);
      if (! safe_parse_maxint (&cylinder, &num_cylinder)
	  || ! safe_parse_maxint (&head, &num_head)
	  || ! safe_parse_maxint (&sector, &num_sector))
	return 0;

      disks[current_drive].cylinders = num_cylinder;
      disks[current_drive].heads = num_head;
      disks[current_drive].sectors = num_sector;

      if (safe_parse_maxint (&total_sector, &num_total_sector))
	disks[current_drive].total_sectors = num_total_sector;
      else
	disks[current_drive].total_sectors
	  = num_cylinder * num_head * num_sector;
      errnum = 0;

      tmp_geom = disks[current_drive];
      buf_drive = -1;
    }
#endif /* GRUB_UTIL */

#ifdef GRUB_UTIL
  msg = device_map[current_drive];
#else
  if (tmp_geom.flags & BIOSDISK_FLAG_BIFURCATE)
    msg = "BIF";
  else if (tmp_geom.flags & BIOSDISK_FLAG_LBA_EXTENSION)
    msg = "LBA";
  else
    msg = "CHS";
#endif

  grub_printf ("drive 0x%02X(%s): C/H/S=%d/%d/%d, Sector Count/Size=%ld/%d\n",
	       current_drive, msg,
	       tmp_geom.cylinders, tmp_geom.heads, tmp_geom.sectors,
	       (unsigned long long)tmp_geom.total_sectors, tmp_geom.sector_size);

  if (tmp_geom.sector_size != 512)
  {
#ifndef GRUB_UTIL
	if (sync)
	{
		grub_printf ("Cannot sync CD-ROM.\n");
		errnum = ERR_BAD_ARGUMENT;
		return 0; /* failure */
	}
#endif
	return 1; /* success */
  }

#ifndef GRUB_UTIL
  if (sync)
  {
#define	BS	((struct master_and_dos_boot_sector *)mbr)
    
    // Make sure rawread will not call get_diskinfo again after force_geometry_tune is reset.
    if (buf_drive != current_drive)
    {
	buf_drive = current_drive;
	buf_track = -1; // invalidate track buffer
    }
    buf_geom = tmp_geom;

    /* Read MBR or the floppy boot sector.  */
    if (! rawread (current_drive, 0, 0, SECTOR_SIZE, (unsigned long long)(unsigned long)mbr, 0xedde0d90))
	return 0;

//    if (current_drive == cdrom_drive || (current_drive >= (unsigned char)min_cdrom_id && current_drive < (unsigned char)(min_cdrom_id + atapi_dev_count)))
//    {
//	grub_printf ("Cannot sync CD-ROM.\n");
//	errnum = ERR_BAD_ARGUMENT;
//	return 0;
//    }

    if (current_drive & 0x80)
    {
	unsigned long start_cl, start_ch, start_dh, start_lba[4];
	unsigned long end_cl, end_ch, end_dh, end_lba[4];
	unsigned long entry1;

	if (current_drive >= 0x88 || current_drive >= 0x80 + (*(unsigned char *)0x475))
	{
	    errnum = ERR_NO_DISK;
	    return 0;
	}

	/* repair partition table. */

	for (entry1 = 0; entry1 < 4; entry1++)
	{
		/* get absolute starting and ending sector number. */
		start_lba[entry1] = PC_SLICE_START (mbr, entry1);
		end_lba[entry1] = start_lba[entry1] + PC_SLICE_LENGTH (mbr, entry1) - 1;
		/* skip null entry. */
		if (! start_lba[entry1] || ! end_lba[entry1])
			continue;
		/* calculate the new CHS starting and ending values. */
		lba_to_chs (start_lba[entry1], &start_cl, &start_ch, &start_dh);
		lba_to_chs (end_lba[entry1], &end_cl, &end_ch, &end_dh);
		/* update values of the partition table in memory */
		PC_SLICE_HEAD (mbr, entry1) = start_dh;
		PC_SLICE_SEC (mbr, entry1) = start_cl;
		PC_SLICE_CYL (mbr, entry1) = start_ch;
		PC_SLICE_EHEAD (mbr, entry1) = end_dh;
		PC_SLICE_ESEC (mbr, entry1) = end_cl;
		PC_SLICE_ECYL (mbr, entry1) = end_ch;
	}
	if (debug > 0)
	    grub_printf ("Writing MBR for drive 0x%X ... ", current_drive);
	/* Write back/update the MBR.  */
	if (! rawwrite (current_drive, 0, mbr))
	{
	    if (debug > 0)
		grub_printf ("failure.\n");
            return 0;
	} else {
	    if (debug > 0)
		grub_printf ("success.\n");
	}

	/* repair BPB of each primary partition. */

	for (entry1 = 0; entry1 < 4; entry1++)
	{
		/* skip null entry. */
		if (! start_lba[entry1] || ! end_lba[entry1])
			continue;
		/* Read the first sector of the partition.  */
		if (! rawread (current_drive, start_lba[entry1], 0, SECTOR_SIZE, (unsigned long long)(unsigned long)mbr, 0xedde0d90))
			continue;	/* on read failure, try next entry */

		/* try to find out the filesystem type */
		if (BS->boot_signature == 0xAA55 && ! probe_bpb(BS) && filesystem_type > 0)
		{
		    if (BS->total_heads != tmp_geom.heads || BS->sectors_per_track != tmp_geom.sectors)
		    {
			if (debug > 0)
			    grub_printf ("(hd%d,%d): Changing H/S=%d/%d to H/S=%d/%d ... ", (current_drive & 0x7F), entry1, BS->total_heads, BS->sectors_per_track, tmp_geom.heads, tmp_geom.sectors);
			BS->total_heads = tmp_geom.heads;
			BS->sectors_per_track = tmp_geom.sectors;

			/* Write back/update the floppy boot sector.  */
			if (! rawwrite (current_drive, start_lba[entry1], mbr))
			{
			    if (debug > 0)
				grub_printf ("failure.\n");
		            return 0;
			} else {
			    if (debug > 0)
				grub_printf ("success.\n");
			}
		    }
		}
	}
    }
    else
    {
	if (current_drive >= 2)
		return 1;

	/* repair floppy BPB */

	/* try to find out the filesystem type */
	if (BS->boot_signature == 0xAA55 && ! probe_bpb(BS) && filesystem_type > 0)
	{
	    if (BS->total_heads != tmp_geom.heads || BS->sectors_per_track != tmp_geom.sectors)
	    {
		if (debug > 0)
		    grub_printf ("Floppy %d: Changing H/S=%d/%d to H/S=%d/%d ... ", current_drive, BS->total_heads, BS->sectors_per_track, tmp_geom.heads, tmp_geom.sectors);
		BS->total_heads = tmp_geom.heads;
		BS->sectors_per_track = tmp_geom.sectors;

		/* Write back/update the floppy boot sector.  */
		if (! rawwrite (current_drive, 0, mbr))
		{
		    if (debug > 0)
			grub_printf ("failure.\n");
	            return 0;
		} else {
		    if (debug > 0)
			grub_printf ("success.\n");
		}
	    }
	}
    }
    //return 1;
#undef BS
  }
#endif

//#ifdef GRUB_UTIL
//  if (current_drive != cdrom_drive)
//#else
//  if (current_drive != cdrom_drive && (current_drive < (unsigned char)min_cdrom_id || current_drive >= (unsigned char)(min_cdrom_id + atapi_dev_count)))
//#endif /* GRUB_UTIL */
    if (current_drive & 0x80)
      real_open_partition (1);

  return 1;
}

static struct builtin builtin_geometry =
{
  "geometry",
  geometry_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
#ifdef GRUB_UTIL
  "geometry [DRIVE] [CYLINDER HEAD SECTOR [TOTAL_SECTOR]]",
  "Print the information for drive DRIVE or the current root device if DRIVE"
  " is not specified. In the grub shell, you can"
  " set the geometry of the drive arbitrarily. The number of the cylinders,"
  " the one of the heads, the one of the sectors and the one of the total"
  " sectors are set to CYLINDER, HEAD, SECTOR and TOTAL_SECTOR,"
  " respectively. If you omit TOTAL_SECTOR, then it will be calculated based"
  " on the C/H/S values automatically."
#else
  "geometry [--tune] [--bios] [--sync] [DRIVE]",
  "Print the information for drive DRIVE or the current root device if DRIVE"
  " is not specified."
  " If --tune is specified, the geometry will change to the tuned value."
  " If --bios is specified, the geometry will change to BIOS reported value."
  " If --sync is specified, the C/H/S values in partition table"
  " of DRIVE and H/S values in BPB of each primary partition of DRIVE"
  "(or BPB of floppy DRIVE) will be updated according to the current"
  " geometry of DRIVE in use."
#endif /* GRUB_UTIL */
};


/* halt */
static int
halt_func (char *arg, int flags)
{
  int skip_flags = 0;

  for (;;)
  {
    if (grub_memcmp (arg, "--no-apm", 8) == 0)
      {
	skip_flags |= 1;
      }
    else if (grub_memcmp (arg, "--no-acpi", 9) == 0)
      {
	skip_flags |= 2;
      }
	else if (grub_memcmp (arg, "--force-sci", 11) ==0)
	  {
	skip_flags |= 4;
      }
    else
	break;
    arg = skip_to (0, arg);
  }
  
  grub_halt (skip_flags);
  
  /* Never reach here.  */
  return 0;
}

static struct builtin builtin_halt =
{
  "halt",
  halt_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "halt [--no-apm] [--no-acpi]",
  "Halt the system using ACPI and APM. If --no-acpi is specified, only APM is"
  "\n\tto be tried. If --no-apm is specified, only ACPI is to be tried. if"
  "\n\tboth options are specified, return to grub4dos with failure."
};


/* help */
#define MAX_SHORT_DOC_LEN	39
#define MAX_LONG_DOC_LEN	72

static int
help_func (char *arg, int flags)
{
  int all = 0;
  
  if (grub_memcmp (arg, "--all", sizeof ("--all") - 1) == 0)
    {
      all = 1;
      arg = skip_to (0, arg);
    }
  
  if (! *arg)
    {
      /* Invoked with no argument. Print the list of the short docs.  */
      struct builtin **builtin;
      int left = 1;

      for (builtin = builtin_table; *builtin != 0; builtin++)
	{
	//  int len;
	//  int i;
	  #if 0
	  /* If this cannot be used in the command-line interface,
	     skip this.  */
	  if (! ((*builtin)->flags & BUILTIN_CMDLINE))
	    continue;
	  #endif
	  /* If this doesn't need to be listed automatically and "--all"
	     is not specified, skip this.  */
	  if (! all && ! ((*builtin)->flags & BUILTIN_HELP_LIST))
	    continue;
#if 0
	  len = grub_strlen ((*builtin)->short_doc);
	  /* If the length of SHORT_DOC is too long, truncate it.  */
	  
	  if (len > MAX_SHORT_DOC_LEN - 1)
	    len = MAX_SHORT_DOC_LEN - 1;

	  for (i = 0; i < len; i++)
	    grub_putchar ((*builtin)->short_doc[i]);

	  for (; i < MAX_SHORT_DOC_LEN; i++)
	    grub_putchar (' ');
#else
	    printf("%-*.*s",MAX_SHORT_DOC_LEN,MAX_SHORT_DOC_LEN-1,(*builtin)->short_doc);
#endif
	  if (! left)
	    grub_putchar ('\n');

	  left = ! left;
	}

      /* If the last entry was at the left column, no newline was printed
	 at the end.  */
      if (! left)
	grub_putchar ('\n');
    }
  else
    {
      /* Invoked with one or more patterns.  */
      do
	{
	  struct builtin **builtin;
	  char *next_arg;

	  /* Get the next argument.  */
	  next_arg = skip_to (0, arg);

	  /* Terminate ARG.  */
	  nul_terminate (arg);

	  for (builtin = builtin_table; *builtin; builtin++)
	    {
	      #if 0
	      /* Skip this if this is only for the configuration file.  */
	      if (! ((*builtin)->flags & BUILTIN_CMDLINE))
		continue;
		#endif
	      if (substring (arg, (*builtin)->name, 0) < 1 && (*builtin)->short_doc)
		{
		  char *doc = (*builtin)->long_doc;

		  /* At first, print the name and the short doc.  */
		  grub_printf ("%s: %s\n",
			       (*builtin)->name, (*builtin)->short_doc);

		  /* Print the long doc.  */
		  while (*doc)
		    {
		      int len = grub_strlen (doc);
//		      int i;

		      /* If LEN is too long, fold DOC.  */
		      if (len > MAX_LONG_DOC_LEN)
			{
			  /* Fold this line at the position of a space.  */
			  for (len = MAX_LONG_DOC_LEN; len > 0; len--)
			    if (doc[len - 1] == ' ')
			      break;
			}
			#if 0
		      grub_printf ("    ");
		      for (i = 0; i < len; i++)
			grub_putchar (*doc++);
		      grub_putchar ('\n');
		  #else
				grub_printf("    %.*s\n",len,doc);
				doc += len;
		  #endif
		    }
		}
	    }

	  arg = next_arg;
	}
      while (*arg);
    }

  return 1;
}

static struct builtin builtin_help =
{
  "help",
  help_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "help [--all] [PATTERN ...]",
  "Display helpful information about builtin commands. Not all commands"
  " aren't shown without the option `--all'."
};


/* hiddenmenu */
static int
hiddenmenu_func (char *arg, int flags)
{
  show_menu = 0;

  while (*arg)
  {
    if (grub_memcmp (arg, "--silent", 8) == 0)
      {
        silent_hiddenmenu = 1;
      }
    else if (grub_memcmp (arg, "--off", 5) == 0)
      {
	/* set to the default values. */
	show_menu = 1;
        silent_hiddenmenu = 0;
      }
      else if (grub_memcmp (arg, "--chkpass", 9) == 0)
      {
			unsigned long long t = 0x11bLL;
			arg = skip_to(1,arg);
			safe_parse_maxint(&arg,&t);
			errnum = 0;
      	silent_hiddenmenu = t << 16;
      }
    arg = skip_to (0, arg);
  }

  return 1;
}

static struct builtin builtin_hiddenmenu =
{
  "hiddenmenu",
  hiddenmenu_func,
  BUILTIN_MENU,
#if 0
  "hiddenmenu [--silent]",
  "Hide the menu."
#endif
};


/* hide */
static int
hide_func (char *arg, int flags)
{
  /* If no arguments, hide current partition in the current drive. */
  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	current_drive = saved_drive;
	current_partition = saved_partition;
  }
  else if (! set_device (arg))
    return 0;

  return set_partition_hidden_flag (1);
}

static struct builtin builtin_hide =
{
  "hide",
  hide_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "hide [PARTITION]",
  "Hide PARTITION by setting the \"hidden\" bit in"
  " its partition type code. The default partition is the current"
  " root device."
};


/* hiddenflag [--set | --clear] [PARTITION] */
static int
hiddenflag_func (char *arg, int flags)
{
  int hidden = -1;	/* -1 for status report */

  for (;;)
  {
    if (grub_memcmp (arg, "--set", 5) == 0)
      {
	hidden = 1;
      }
    else if (grub_memcmp (arg, "--clear", 7) == 0)
      {
	hidden = 0;
      }
    else
	break;
    arg = skip_to (0, arg);
  }
  
  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	current_drive = saved_drive;
	current_partition = saved_partition;
  }
  else if (! set_device (arg))
    return 0;

  return set_partition_hidden_flag (hidden);
}

static struct builtin builtin_hiddenflag =
{
  "hiddenflag",
  hiddenflag_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "hiddenflag [--set | --clear] [PARTITION]",
  "Hide/unhide PARTITION by setting/clearing the \"hidden\" bit in"
  " its partition type code, or report the hidden status."
  " The default partition is the current root device."
};


#ifdef SUPPORT_NETBOOT
/* ifconfig */
static int
ifconfig_func (char *arg, int flags)
{
  char *svr = 0, *ip = 0, *gw = 0, *sm = 0;
  
  if (! eth_probe ())
    {
      grub_printf ("No ethernet card found.\n");
      errnum = ERR_DEV_VALUES;
      return 0;
    }
  
  while (*arg) 
    {
      if (! grub_memcmp ("--server=", arg, sizeof ("--server=") - 1))
	svr = arg + sizeof("--server=") - 1;
      else if (! grub_memcmp ("--address=", arg, sizeof ("--address=") - 1))
	ip = arg + sizeof ("--address=") - 1;
      else if (! grub_memcmp ("--gateway=", arg, sizeof ("--gateway=") - 1))
	gw = arg + sizeof ("--gateway=") - 1;
      else if (! grub_memcmp ("--mask=", arg, sizeof("--mask=") - 1))
	sm = arg + sizeof ("--mask=") - 1;
      else
	{
	  errnum = ERR_BAD_ARGUMENT;
	  return 0;
	}
      
      arg = skip_to (0, arg);
    }
  
  if (! ifconfig (ip, sm, gw, svr))
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  
  print_network_configuration ();
  return 1;
}

static struct builtin builtin_ifconfig =
{
  "ifconfig",
  ifconfig_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "ifconfig [--address=IP] [--gateway=IP] [--mask=MASK] [--server=IP]",
  "Configure the IP address, the netmask, the gateway and the server"
  " address or print current network configuration."
};
#endif /* SUPPORT_NETBOOT */


#if 0
/* impsprobe */
static int
impsprobe_func (char *arg, int flags)
{
#ifdef GRUB_UTIL
  /* In the grub shell, we cannot probe IMPS.  */
  errnum = ERR_UNRECOGNIZED;
  return 0;
#else /* ! GRUB_UTIL */
  if (!imps_probe ())
  {
    if (debug > 0)
      printf (" No MPS information found or probe failed\n");
    return 0;
  }

  return 1;
#endif /* ! GRUB_UTIL */
}

static struct builtin builtin_impsprobe =
{
  "impsprobe",
  impsprobe_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT,
  "impsprobe",
  "Probe the Intel Multiprocessor Specification 1.1 or 1.4"
  " configuration table and boot the various CPUs which are found into"
  " a tight loop."
};
#endif


/* initrd */
static int
initrd_func (char *arg, int flags)
{
  switch (kernel_type)
    {
    case KERNEL_TYPE_LINUX:
    case KERNEL_TYPE_BIG_LINUX:
      if (! load_initrd (arg))
	return 0;
      break;

    default:
      errnum = ERR_NEED_LX_KERNEL;
      return 0;
    }

  return 1;
}

static struct builtin builtin_initrd =
{
  "initrd",
  initrd_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "initrd FILE [FILE ...]",
  "Load an initial ramdisk FILE for a Linux format boot image and set the"
  " appropriate parameters in the Linux setup area in memory. For Linux"
  " 2.6+ kernels, multiple cpio files can be loaded."
};


#ifdef GRUB_UTIL
//#ifdef GRUB_UTIL
//#define installaddr (*(unsigned long *)RAW_ADDR (0x100000))
//#define installlist (*(unsigned long *)RAW_ADDR (0x100004))
//#define stage2_first_buffer (*(char **)RAW_ADDR (0x100008))
//#else
static unsigned long installaddr, installlist;
static char *stage2_first_buffer;
//#endif
static void disk_read_blocklist_func1 (unsigned long sector, unsigned long offset, unsigned long length);

  /* Write SECTOR to INSTALLLIST, and update INSTALLADDR and
     INSTALLSECT.  */
static void
disk_read_blocklist_func1 (unsigned long sector, unsigned long offset, unsigned long length)
{
      if (((unsigned long)debug) >= 0x7FFFFFFF)
	printf("[%d]", sector);

      if (offset != 0 || blklst_last_length != SECTOR_SIZE)
	{
	  /* We found a non-sector-aligned data block. */
	  errnum = ERR_UNALIGNED;
	  return;
	}

      blklst_last_length = length;

      if (*((unsigned long *) (installlist - 4))
	  + *((unsigned short *) installlist) != sector
	  || installlist == (unsigned long) stage2_first_buffer + SECTOR_SIZE + 4)
	{
	  installlist -= 8;

	  if (*((unsigned long *) (installlist - 8)))
	    errnum = ERR_WONT_FIT;
	  else
	    {
	      *((unsigned short *) (installlist + 2)) = (installaddr >> 4);
	      *((unsigned long *) (installlist - 4)) = sector;
	    }
	}

      *((unsigned short *) installlist) += 1;
      installaddr += 512;
}

//#ifdef GRUB_UTIL
//#define saved_sector (*(unsigned long *)RAW_ADDR (0x10000C))
//#else
static unsigned long saved_sector;
//#endif
static void disk_read_savesect_func (unsigned long sector, unsigned long offset, unsigned long length);
  
  /* Save the first sector of Stage2 in STAGE2_SECT.  */
static void
disk_read_savesect_func (unsigned long sector, unsigned long offset, unsigned long length)
{
      if (((unsigned long)debug) >= 0x7FFFFFFF)
	printf ("[%d]", sector);

      /* ReiserFS has files which sometimes contain data not aligned
         on sector boundaries.  Returning an error is better than
         silently failing. */
      if (offset != 0 || length != SECTOR_SIZE)
	errnum = ERR_UNALIGNED;

      saved_sector = sector;
}

/* install */
static int
install_func (char *arg, int flags)
{
  char *stage1_file, *dest_dev, *file, *addr;
  char *stage1_buffer = (char *) RAW_ADDR (0x100200);
  char *stage2_buffer = stage1_buffer + SECTOR_SIZE;
  char *old_sect = stage2_buffer + SECTOR_SIZE;
  char *stage2_second_buffer;
  /* XXX: Probably SECTOR_SIZE is reasonable.  */
  char *config_filename;
  char *dummy;
  unsigned long new_drive = GRUB_INVALID_DRIVE;
  unsigned long dest_drive;
  unsigned long dest_partition;
  unsigned long dest_sector;
  unsigned long src_drive, src_partition, src_part_start;
  unsigned long i;
//struct geometry dest_geom, src_geom;
  unsigned long stage2_first_sector, stage2_second_sector;
  char *ptr;
  /* Point to the location of the name of a configuration file in Stage 2.  */
  char *config_file_location;
  /* If FILE is a Stage 1.5?  */
  int is_stage1_5 = 0;
  /* Must call grub_close?  */
  int is_open = 0;
  /* If LBA is forced?  */
  int is_force_lba = 0;
#ifndef NO_DECOMPRESSION
  int no_decompression_bak = no_decompression;
#endif
  /* Was the last sector full? */
  blklst_last_length = SECTOR_SIZE;
  
  stage2_first_buffer = old_sect + SECTOR_SIZE;
  stage2_second_buffer = stage2_first_buffer + SECTOR_SIZE;
  config_filename = stage2_second_buffer + SECTOR_SIZE;
  dummy = config_filename + SECTOR_SIZE;
  
#ifdef GRUB_UTIL
  /* If the Stage 2 is in a partition mounted by an OS, this will store
     the filename under the OS.  */
  char *stage2_os_file = 0;
#endif /* GRUB_UTIL */
  
  /* First, check the GNU-style long option.  */
  while (1)
    {
      if (grub_memcmp ("--force-lba", arg, sizeof ("--force-lba") - 1) == 0)
	{
	  is_force_lba = 1;
	  arg = skip_to (0, arg);
	}
#ifdef GRUB_UTIL
      else if (grub_memcmp ("--stage2=", arg, sizeof ("--stage2=") - 1) == 0)
	{
	  stage2_os_file = arg + sizeof ("--stage2=") - 1;
	  arg = skip_to (0, arg);
	  nul_terminate (stage2_os_file);
	}
#endif /* GRUB_UTIL */
      else
	break;
    }
  
  stage1_file = arg;
  dest_dev = skip_to (0, stage1_file);
  if (*dest_dev == 'd')
    {
      new_drive = 0;
      dest_dev = skip_to (0, dest_dev);
    }
  file = skip_to (0, dest_dev);
  addr = skip_to (0, file);

  /* Get the installation address.  */
  {
	unsigned long long ull;
	if (! safe_parse_maxint (&addr, &ull))
	{
		/* ADDR is not specified.  */
		installaddr = 0;
		ptr = addr;
		errnum = 0;
	}
	else
	{
		installaddr = ull;
		ptr = skip_to (0, addr);
	}
  }

#ifndef NO_DECOMPRESSION
  /* Do not decompress Stage 1 or Stage 2.  */
  no_decompression = 1;
#endif

  /* Read Stage 1.  */
  is_open = grub_open (stage1_file);
  if (! is_open
      || ! grub_read ((unsigned long long)(unsigned int)stage1_buffer, SECTOR_SIZE, 0xedde0d90) == SECTOR_SIZE)
    goto fail;

  /* Read the old sector from DEST_DEV.  */
  if (! set_device (dest_dev)
      || ! open_partition ()
      || ! devread (0, 0, SECTOR_SIZE, (unsigned long long)(unsigned int)old_sect, 0xedde0d90))
    goto fail;

  /* Store the information for the destination device.  */
  dest_drive = current_drive;
  dest_partition = current_partition;
//dest_geom = buf_geom;
  dest_sector = (unsigned long)part_start;

  /* Copy the possible DOS BPB, 59 bytes at byte offset 3.  */
  grub_memmove (stage1_buffer + BOOTSEC_BPB_OFFSET,
		old_sect + BOOTSEC_BPB_OFFSET,
		BOOTSEC_BPB_LENGTH);

  /* If for a hard disk, copy the possible MBR/extended part table.  */
  if (dest_drive & 0x80)
    grub_memmove (stage1_buffer + STAGE1_WINDOWS_NT_MAGIC,
		  old_sect + STAGE1_WINDOWS_NT_MAGIC,
		  STAGE1_PARTEND - STAGE1_WINDOWS_NT_MAGIC);

  /* Check for the version and the signature of Stage 1.  */
  if (*((short *)(stage1_buffer + STAGE1_VER_MAJ_OFFS)) != COMPAT_VERSION
      || (*((unsigned short *) (stage1_buffer + BOOTSEC_SIG_OFFSET))
	  != BOOTSEC_SIGNATURE))
    {
      errnum = ERR_BAD_VERSION;
      goto fail;
    }

  /* This below is not true any longer. But should we leave this alone?  */
  
  /* If DEST_DRIVE is a floppy, Stage 2 must have the iteration probe
     routine.  */
  if (! (dest_drive & 0x80)
      && (*((unsigned char *) (stage1_buffer + BOOTSEC_PART_OFFSET)) == 0x80
	  || stage1_buffer[BOOTSEC_PART_OFFSET] == 0))
    {
      errnum = ERR_BAD_VERSION;
      goto fail;
    }

  grub_close ();
  
  /* Open Stage 2.  */
  is_open = grub_open (file);
  if (! is_open)
    goto fail;

  src_drive = current_drive;
  src_partition = current_partition;
  src_part_start = (unsigned long)part_start;
//src_geom = buf_geom;
  
  if (! new_drive)
    new_drive = src_drive;
  else if (src_drive != dest_drive)
    grub_printf ("Warning: the option `d' was not used, but the Stage 1 will"
		 " be installed on a\ndifferent drive than the drive where"
		 " the Stage 2 resides.\n");

  /* Set the boot drive.  */
  *((unsigned char *) (stage1_buffer + STAGE1_BOOT_DRIVE)) = new_drive;

  /* Set the "force LBA" flag.  */
  *((unsigned char *) (stage1_buffer + STAGE1_FORCE_LBA)) = is_force_lba;

  /* If DEST_DRIVE is a hard disk, enable the workaround, which is
     for buggy BIOSes which don't pass boot drive correctly. Instead,
     they pass 0x00 or 0x01 even when booted from 0x80.  */
  if (dest_drive & BIOS_FLAG_FIXED_DISK)
    /* Replace the jmp (2 bytes) with double nop's.  */
    *((unsigned short *) (stage1_buffer + STAGE1_BOOT_DRIVE_CHECK))
      = 0x9090;
  
  /* Read the first sector of Stage 2.  */
  disk_read_hook = disk_read_savesect_func;
  if (grub_read ((unsigned long long)(unsigned int)stage2_first_buffer, SECTOR_SIZE, 0xedde0d90) != SECTOR_SIZE)
    goto fail;

  stage2_first_sector = saved_sector;
  
  /* Read the second sector of Stage 2.  */
  if (grub_read ((unsigned long long)(unsigned int)stage2_second_buffer, SECTOR_SIZE, 0xedde0d90) != SECTOR_SIZE)
    goto fail;

  stage2_second_sector = saved_sector;
  
  /* Check for the version of Stage 2.  */
  if (*((short *) (stage2_second_buffer + STAGE2_VER_MAJ_OFFS))
      != COMPAT_VERSION)
    {
      errnum = ERR_BAD_VERSION;
      goto fail;
    }

  /* Check for the Stage 2 id.  */
  if (stage2_second_buffer[STAGE2_STAGE2_ID] != STAGE2_ID_STAGE2)
    is_stage1_5 = 1;

  /* If INSTALLADDR is not specified explicitly in the command-line,
     determine it by the Stage 2 id.  */
  if (! installaddr)
    {
      if (! is_stage1_5)
	/* Stage 2.  */
	installaddr = 0x8000;
      else
	/* Stage 1.5.  */
	installaddr = 0x2000;
    }

  *((unsigned long *) (stage1_buffer + STAGE1_STAGE2_SECTOR))
    = stage2_first_sector;
  *((unsigned short *) (stage1_buffer + STAGE1_STAGE2_ADDRESS))
    = installaddr;
  *((unsigned short *) (stage1_buffer + STAGE1_STAGE2_SEGMENT))
    = installaddr >> 4;

  i = (unsigned long) stage2_first_buffer + SECTOR_SIZE - 4;
  while (*((unsigned long *) i))
    {
      if (i < (int) stage2_first_buffer
	  || (*((int *) (i - 4)) & 0x80000000)
	  || *((unsigned short *) i) >= 0xA00
	  || *((short *) (i + 2)) == 0)
	{
	  errnum = ERR_BAD_VERSION;
	  goto fail;
	}

      *((int *) i) = 0;
      *((int *) (i - 4)) = 0;
      i -= 8;
    }

  installlist = (int) stage2_first_buffer + SECTOR_SIZE + 4;
  installaddr += SECTOR_SIZE;
  
  /* Read the whole of Stage2 except for the first sector.  */
  filepos = SECTOR_SIZE;

  disk_read_hook = disk_read_blocklist_func1;
  if (! grub_read ((unsigned long long)(unsigned int)dummy, -1ULL, 0xedde0d90))
    goto fail;
  
  disk_read_hook = 0;
  
  /* Find a string for the configuration filename.  */
  config_file_location = stage2_second_buffer + STAGE2_VER_STR_OFFS;
  while (*(config_file_location++))
    ;

  /* Set the "force LBA" flag for Stage2.  */
  *((unsigned char *) (stage2_second_buffer + STAGE2_FORCE_LBA))
    = is_force_lba;
  
  if (*ptr == 'p')
    {
      *((long *) (stage2_second_buffer + STAGE2_INSTALLPART))
	= src_partition;
      if (is_stage1_5)
	{
	  /* Reset the device information in FILE if it is a Stage 1.5.  */
	  unsigned long device = 0xFFFFFFFF;

	  grub_memmove (config_file_location, (char *) &device,
			sizeof (device));
	}

      ptr = skip_to (0, ptr);
    }

  if (*ptr)
    {
      grub_strcpy (config_filename, ptr);
      nul_terminate (config_filename);
	
      if (! is_stage1_5)
	/* If it is a Stage 2, just copy PTR to CONFIG_FILE_LOCATION.  */
	grub_strcpy (config_file_location, ptr);
      else
	{
	  char *real_config;
	  unsigned long device;

	  /* Translate the external device syntax to the internal device
	     syntax.  */
	  if (! (real_config = set_device (ptr)))
	    {
	      /* The Stage 2 PTR does not contain the device name, so
		 use the root device instead.  */
	      errnum = ERR_NONE;
	      current_drive = saved_drive;
	      current_partition = saved_partition;
	      real_config = ptr;
	    }
	  
	  if (current_drive == src_drive)
	    {
	      /* If the drive where the Stage 2 resides is the same as
		 the one where the Stage 1.5 resides, do not embed the
		 drive number.  */
	      current_drive = GRUB_INVALID_DRIVE;
	    }

	  device = (current_drive << 24) | current_partition;
	  grub_memmove (config_file_location, (char *) &device,
			sizeof (device));
	  grub_strcpy (config_file_location + sizeof (device),
		       real_config);
	}

      /* If a Stage 1.5 is used, then we need to modify the Stage2.  */
      if (is_stage1_5)
	{
	  char *real_config_filename = skip_to (0, ptr);
	  
	  is_open = grub_open (config_filename);
	  if (! is_open)
	    goto fail;

	  /* Skip the first sector.  */
	  filepos = SECTOR_SIZE;
	  
	  disk_read_hook = disk_read_savesect_func;
	  if (grub_read ((unsigned long long)(unsigned int)stage2_buffer, SECTOR_SIZE, 0xedde0d90) != SECTOR_SIZE)
	    goto fail;
	  
	  disk_read_hook = 0;
	  grub_close ();
	  is_open = 0;
	  
	  /* Sanity check.  */
	  if (*(stage2_buffer + STAGE2_STAGE2_ID) != STAGE2_ID_STAGE2)
	    {
	      errnum = ERR_BAD_VERSION;
	      goto fail;
	    }

	  /* Set the "force LBA" flag for Stage2.  */
	  *(stage2_buffer + STAGE2_FORCE_LBA) = is_force_lba;

	  /* If REAL_CONFIG_FILENAME is specified, copy it to the Stage2.  */
	  if (*real_config_filename)
	    {
	      /* Specified */
	      char *location;
	      
	      /* Find a string for the configuration filename.  */
	      location = stage2_buffer + STAGE2_VER_STR_OFFS;
	      while (*(location++))
		;
	      
	      /* Copy the name.  */
	      grub_strcpy (location, real_config_filename);
	    }
	  
	  /* Write it to the disk.  */
	  buf_track = -1;

#ifdef GRUB_UTIL
	  /* In the grub shell, access the Stage 2 via the OS filesystem
	     service, if possible.  */
	  if (stage2_os_file)
	    {
	      FILE *fp;

	      fp = fopen (stage2_os_file, "r+");
	      if (! fp)
		{
		  errnum = ERR_FILE_NOT_FOUND;
		  goto fail;
		}

	      if (fseek (fp, SECTOR_SIZE, SEEK_SET) != 0)
		{
		  fclose (fp);
		  errnum = ERR_BAD_VERSION;
		  goto fail;
		}

	      if (fwrite (stage2_buffer, 1, SECTOR_SIZE, fp)
		  != SECTOR_SIZE)
		{
		  fclose (fp);
		  errnum = ERR_WRITE;
		  goto fail;
		}

	      fclose (fp);
	    }
	  else
#endif /* GRUB_UTIL */
	    {
	      if (! devwrite ((unsigned long)(saved_sector - part_start), 1, stage2_buffer))
		goto fail;
	    }
	}
    }

  /* Clear the cache.  */
  buf_track = -1;

  /* Write the modified sectors of Stage2 to the disk.  */
#ifdef GRUB_UTIL
  if (! is_stage1_5 && stage2_os_file)
    {
      FILE *fp;

      fp = fopen (stage2_os_file, "r+");
      if (! fp)
	{
	  errnum = ERR_FILE_NOT_FOUND;
	  goto fail;
	}

      if (fwrite (stage2_first_buffer, 1, SECTOR_SIZE, fp) != SECTOR_SIZE)
	{
	  fclose (fp);
	  errnum = ERR_WRITE;
	  goto fail;
	}

      if (fwrite (stage2_second_buffer, 1, SECTOR_SIZE, fp) != SECTOR_SIZE)
	{
	  fclose (fp);
	  errnum = ERR_WRITE;
	  goto fail;
	}

      fclose (fp);
    }
  else
#endif /* GRUB_UTIL */
    {
      /* The first.  */
      current_drive = src_drive;
      current_partition = src_partition;

      if (! open_partition ())
	goto fail;

      if (! devwrite (stage2_first_sector - src_part_start, 1,
		      stage2_first_buffer))
	goto fail;

      if (! devwrite (stage2_second_sector - src_part_start, 1,
		      stage2_second_buffer))
	goto fail;
    }
  
  /* Write the modified sector of Stage 1 to the disk.  */
  current_drive = dest_drive;
  current_partition = dest_partition;
  if (! open_partition ())
    goto fail;

  devwrite (0, 1, stage1_buffer);

 fail:
  if (is_open)
    grub_close ();
  
  disk_read_hook = 0;
  
#ifndef NO_DECOMPRESSION
  no_decompression = no_decompression_bak;
#endif

//  if (debug > 0)
//    grub_printf ("\n\nNotice: the two commands SETUP and INSTALL will be removed soon! Please use the\nOS utility of BOOTLACE.COM to install GRLDR boot record to the MBR or to the\nboot area of a specified partition.\n\n");
  return ! errnum;
}

static struct builtin builtin_install =
{
  "install",
  install_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "install [--stage2=STAGE2_FILE] [--force-lba] STAGE1 [d] DEVICE\n STAGE2 [ADDR] [p] [CONFIG_FILE] [REAL_CONFIG_FILE]",
  "Install STAGE1 on DEVICE, and install a blocklist for loading STAGE2"
  " as a Stage 2. If the option `d' is present, the Stage 1 will always"
  " look for the disk where STAGE2 was installed, rather than using"
  " the booting drive. The Stage 2 will be loaded at address ADDR, which"
  " will be determined automatically if you don't specify it. If"
  " the option `p' or CONFIG_FILE is present, then the first block"
  " of Stage 2 is patched with new values of the partition and name"
  " of the configuration file used by the true Stage 2 (for a Stage 1.5,"
  " this is the name of the true Stage 2) at boot time. If STAGE2 is a Stage"
  " 1.5 and REAL_CONFIG_FILE is present, then the Stage 2 CONFIG_FILE is"
  " patched with the configuration filename REAL_CONFIG_FILE."
  " If the option `--force-lba' is specified, disable some sanity checks"
  " for LBA mode. If the option `--stage2' is specified, rewrite the Stage"
  " 2 via your OS's filesystem instead of the raw device."
};
#endif /* GRUB_UTIL */


/* is64bit */
static int
is64bit_func (char *arg, int flags)
{
  return is64bit;
}

static struct builtin builtin_is64bit =
{
  "is64bit",
  is64bit_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "is64bit",
  "Return true if CPU is 64-bit and false if not."
};


/* kernel */
static int
kernel_func (char *arg, int flags)
{
  int len;
  kernel_t suggested_type = KERNEL_TYPE_NONE;
  unsigned long load_flags = 0;

#ifndef AUTO_LINUX_MEM_OPT
  load_flags |= KERNEL_LOAD_NO_MEM_OPTION;
#endif

  /* Deal with GNU-style long options.  */
  while (1)
    {
      /* If the option `--type=TYPE' is specified, convert the string to
	 a kernel type.  */
      if (grub_memcmp (arg, "--type=", 7) == 0)
	{
	  arg += 7;
	  
	  if (grub_memcmp (arg, "netbsd", 6) == 0)
	    suggested_type = KERNEL_TYPE_NETBSD;
	  else if (grub_memcmp (arg, "freebsd", 7) == 0)
	    suggested_type = KERNEL_TYPE_FREEBSD;
	  else if (grub_memcmp (arg, "openbsd", 7) == 0)
	    /* XXX: For now, OpenBSD is identical to NetBSD, from GRUB's
	       point of view.  */
	    suggested_type = KERNEL_TYPE_NETBSD;
	  else if (grub_memcmp (arg, "linux", 5) == 0)
	    suggested_type = KERNEL_TYPE_LINUX;
	  else if (grub_memcmp (arg, "biglinux", 8) == 0)
	    suggested_type = KERNEL_TYPE_BIG_LINUX;
	  else if (grub_memcmp (arg, "multiboot", 9) == 0)
	    suggested_type = KERNEL_TYPE_MULTIBOOT;
	  else
	    {
	      errnum = ERR_BAD_ARGUMENT;
	      return 0;
	    }
	}
      /* If the `--no-mem-option' is specified, don't pass a Linux's mem
	 option automatically. If the kernel is another type, this flag
	 has no effect.  */
      else if (grub_memcmp (arg, "--no-mem-option", 15) == 0)
	load_flags |= KERNEL_LOAD_NO_MEM_OPTION;
      else
	break;

      /* Try the next.  */
      arg = skip_to (0, arg);
    }
      
  len = grub_strlen (arg) + grub_strlen (kernel_option_video);

  /* Reset MB_CMDLINE.  */
  mb_cmdline = (char *) MB_CMDLINE_BUF;
  if (len + 1 > MB_CMDLINE_BUFLEN)
    {
      errnum = ERR_WONT_FIT;
      return 0;
    }

  /* Copy the command-line to MB_CMDLINE and append the kernel_option_video
   * which might have been set by `setvbe'.
   */
  grub_sprintf (mb_cmdline, "%s%s", arg, kernel_option_video);

  suggested_type = load_image (arg, mb_cmdline, suggested_type, load_flags);
  if (suggested_type == KERNEL_TYPE_NONE)
    return 0;

  kernel_type = suggested_type;
  mb_cmdline += len + 1;
  return 1;
}

static struct builtin builtin_kernel =
{
  "kernel",
  kernel_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "kernel [--no-mem-option] [--type=TYPE] FILE [ARG ...]",
  "Attempt to load the primary boot image from FILE. The rest of the"
  " line is passed verbatim as the \"kernel command line\".  Any modules"
  " must be reloaded after using this command. The option --type is used"
  " to suggest what type of kernel to be loaded. TYPE must be either of"
  " \"netbsd\", \"freebsd\", \"openbsd\", \"linux\", \"biglinux\" and"
  " \"multiboot\". The option --no-mem-option tells GRUB not to pass a"
  " Linux's mem option automatically."
};


/* lock */
static int
lock_func (char *arg, int flags)
{
  if (! auth && password)
    {
      errnum = ERR_PRIVILEGED;
      return 0;
    }

  return 1;
}

static struct builtin builtin_lock =
{
  "lock",
  lock_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "lock",
  "Break a command execution unless the user is authenticated."
};
  

#ifndef STAGE1_5
/* ls */
static int
ls_func (char *arg, int flags)
{
  /* If no arguments, list root dir of current root device. */
  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	return dir ("/");
  }
  else if (substring(arg,"dev",1) == 0)
  {
  	buf_drive = -1;
	sprintf((char *)COMPLETION_BUF,"(");
	return print_completions(1,0);
  }

  return dir (arg);
}

static struct builtin builtin_ls =
{
  "ls",
  ls_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "ls [FILE_OR_DIR]",
  "List file or directory."
};
#endif /* STAGE1_5 */


/* makeactive */
static int
makeactive_func (char *arg, int flags)
{
  int status = 0;
  int part = 0;

  if (grub_memcmp (arg, "--status", 8) == 0)
    {
      status = 1;
      arg = skip_to (0, arg);
    }

  /* Get the drive and the partition.  */
  if (! *arg || *arg == ' ' || *arg == '\t')
    {
	current_drive = saved_drive;
	current_partition = saved_partition;
    }
  else
    {
      if (! set_device (arg))
	return 0;
    }

  /* The drive must be a hard disk.  */
  if (! (current_drive & 0x80))
    {
      errnum = ERR_DEV_VALUES;
      return 0;
    }

  /* The partition must be a primary partition.  */
  if ((part = (current_partition >> 16)) > 3
      /*|| (current_partition & 0xFFFF) != 0xFFFF*/)
    {
      errnum = ERR_DEV_VALUES;
      return 0;
    }

  /* Read the MBR in the scratch space.  */
  if (! rawread (current_drive, 0, 0, SECTOR_SIZE, (unsigned long long)(unsigned long)mbr, 0xedde0d90))
	return 0;

  /* If the partition is an extended partition, setting the active
     flag violates the specification by IBM.  */
  if (IS_PC_SLICE_TYPE_EXTENDED (PC_SLICE_TYPE (mbr, part)))
    {
	errnum = ERR_DEV_VALUES;
	return 0;
    }

  if (status)
    {
	int active = (PC_SLICE_FLAG (mbr, part) == PC_SLICE_FLAG_BOOTABLE);
	if (debug > 0)
		grub_printf ("Partition (hd%d,%d) is %sactive.\n", (current_drive & 0x7F), part, ((active ? "" : "not ")));
		
	return active;
    }

  /* Check if the active flag is disabled.  */
  if (PC_SLICE_FLAG (mbr, part) != PC_SLICE_FLAG_BOOTABLE)
    {
	/* Clear all the active flags in this table.  */
	{
	  int j;
	  for (j = 0; j < 4; j++)
	    PC_SLICE_FLAG (mbr, j) = 0;
	}

	/* Set the flag.  */
	PC_SLICE_FLAG (mbr, part) = PC_SLICE_FLAG_BOOTABLE;

	/* Write back the MBR.  */
	if (! rawwrite (current_drive, 0, mbr))
	    return 0;

	if (debug > 0)
		grub_printf ("Partition (hd%d,%d) successfully set active.\n", (current_drive & 0x7F), part);
    }
  else
    {
	if (debug > 0)
		grub_printf ("Partition (hd%d,%d) was already active.\n", (current_drive & 0x7F), part);
    }

  return 1;
}

static struct builtin builtin_makeactive =
{
  "makeactive",
  makeactive_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "makeactive [--status] [PART]",
  "Activate the partition PART. PART defaults to the current root device."
  " This command is limited to _primary_ PC partitions on a hard disk."
};


#ifndef GRUB_UTIL
static unsigned long long start_sector, sector_count;
unsigned long long initrd_start_sector;

  /* Get the start sector number of the file.  */
static void disk_read_start_sector_func (unsigned long sector, unsigned long offset, unsigned long length);
static void
disk_read_start_sector_func (unsigned long sector, unsigned long offset, unsigned long length)
{
      if (sector_count < 1)
	{
	  start_sector = sector;
	}
      sector_count++;
}

static void print_bios_total_drives(void);
static void
print_bios_total_drives(void)
{	
	grub_printf ("\nfloppies_orig=%d, harddrives_orig=%d, floppies_curr=%d, harddrives_curr=%d\n", 
			((floppies_orig & 1)?(floppies_orig >> 6) + 1 : 0), harddrives_orig,
			(((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0), (*(char*)0x475));
}

#endif
  
unsigned long probed_total_sectors;
unsigned long probed_total_sectors_round;
unsigned long probed_heads;
unsigned long probed_sectors_per_track;
unsigned long probed_cylinders;
unsigned long sectors_per_cylinder;
#ifdef GRUB_UTIL
int filesystem_type = 0;
#endif

#if 0
    /* matrix of coefficients of linear equations
     * 
     *   C[n] * (H_count * S_count) + H[n] * S_count = LBA[n] - S[n] + 1
     *
     * where n = 1, 2, 3, 4, 5, 6, 7, 8
     */
	 /* This starts at offset 0x130 */ 
long long L[9]; /* L[n] == LBA[n] - S[n] + 1 */
long H[9];
short C[9];
short X;
short Y;
short Cmax;
long Hmax;
short Smax;
unsigned long Z;
#endif

/*  
 * return:
 * 		0		success
 *		otherwise	failure
 *			1	no "55 AA" at the end
 *
 */

int
probe_bpb (struct master_and_dos_boot_sector *BS)
{
  unsigned long i;

  /* first, check ext2 grldr boot sector */
  probed_total_sectors = BS->total_sectors_long;

  /* at 0D: (byte)Sectors per block. Valid values are 2, 4, 8, 16 and 32. */
  if (BS->sectors_per_cluster < 2 || 32 % BS->sectors_per_cluster)
	goto failed_ext2_grldr;

  /* at 0E: (word)Bytes per block.
   * Valid values are 0x400, 0x800, 0x1000, 0x2000 and 0x4000.
   */
  if (BS->reserved_sectors != BS->sectors_per_cluster * 0x200)
	goto failed_ext2_grldr;
  
  i = *(unsigned long *)((char *)BS + 0x2C);
  if (BS->reserved_sectors == 0x400 && i != 2)
	goto failed_ext2_grldr;

  if (BS->reserved_sectors != 0x400 && i != 1)
	goto failed_ext2_grldr;

  /* at 14: Pointers per block(number of blocks covered by an indirect block).
   * Valid values are 0x100, 0x200, 0x400, 0x800, 0x1000.
   */

  i = *(unsigned long *)((char *)BS + 0x14);
  if (i < 0x100 || 0x1000 % i)
	goto failed_ext2_grldr;
  
  /* at 10: Pointers in pointers-per-block blocks, that is, number of
   *        blocks covered by a double-indirect block.
   * Valid values are 0x10000, 0x40000, 0x100000, 0x400000 and 0x1000000.
   */

  if (*(unsigned long *)((char *)BS + 0x10) != i * i)
	goto failed_ext2_grldr;
  
  if (! BS->sectors_per_track || BS->sectors_per_track > 63)
	goto failed_ext2_grldr;
  
  if (BS->total_heads > 256 /* (BS->total_heads - 1) >> 8 */)
	goto failed_ext2_grldr;
  
  probed_heads = BS->total_heads;
  probed_sectors_per_track = BS->sectors_per_track;
  sectors_per_cylinder = probed_heads * probed_sectors_per_track;
  probed_cylinders = (probed_total_sectors + sectors_per_cylinder - 1) / sectors_per_cylinder;

  filesystem_type = 5;
  
  /* BPB probe success */
  return 0;

failed_ext2_grldr:
  probed_total_sectors = BS->total_sectors_short ? BS->total_sectors_short : (BS->total_sectors_long ? BS->total_sectors_long : (unsigned long)BS->total_sectors_long_long);
#if 0
  if (probed_total_sectors != sector_count && sector_count != 1 && (! (probed_total_sectors & 1) || probed_total_sectors != sector_count - 1))
	goto failed_probe_BPB;
#endif
  if (BS->bytes_per_sector != 0x200)
	return 1;
  if (! BS->sectors_per_cluster || 128 % BS->sectors_per_cluster)
	return 1;
//  if (! BS->reserved_sectors)	/* NTFS reserved_sectors is 0 */
//	goto failed_probe_BPB;
  if (BS->number_of_fats > (unsigned char)2 /* BS->number_of_fats && ((BS->number_of_fats - 1) >> 1) */)
	return 1;
  if (! BS->sectors_per_track || BS->sectors_per_track > 63)
	return 1;
  if (BS->total_heads > 256 /* (BS->total_heads - 1) >> 8 */)
	return 1;
  if (BS->media_descriptor < (unsigned char)0xF0)
	return 1;
  if (! BS->root_dir_entries && ! BS->total_sectors_short && ! BS->sectors_per_fat) /* FAT32 or NTFS */
	if (BS->number_of_fats && BS->total_sectors_long && BS->sectors_per_fat32)
	{
		filesystem_type = 3;
	}
	else if (! BS->number_of_fats && ! BS->total_sectors_long && ! BS->reserved_sectors && BS->total_sectors_long_long)
	{
		filesystem_type = 4;
	} else
		return 1;	/* unknown NTFS-style BPB */
  else if (BS->number_of_fats && BS->sectors_per_fat) /* FAT12 or FAT16 */
	if ((probed_total_sectors - BS->reserved_sectors - BS->number_of_fats * BS->sectors_per_fat - (BS->root_dir_entries * 32 + BS->bytes_per_sector - 1) / BS->bytes_per_sector) / BS->sectors_per_cluster < 0x0ff8 )
	{
		filesystem_type = 1;
	} else {
		filesystem_type = 2;
	}
  else
	return 1;	/* unknown BPB */
  
  probed_heads = BS->total_heads;
  probed_sectors_per_track = BS->sectors_per_track;
  sectors_per_cylinder = probed_heads * probed_sectors_per_track;
  probed_cylinders = (probed_total_sectors + sectors_per_cylinder - 1) / sectors_per_cylinder;

  /* BPB probe success */
  return 0;

}

#if 0
/* on call:
 * 		BS		points to the bootsector
 * 		start_sector1	is the start_sector of the bootimage in the real disk, if unsure, set it to 0
 * 		sector_count1	is the sector_count of the bootimage in the real disk, if unsure, set it to 1
 * 		part_start1	is the part_start of the partition in which the bootimage resides, if unsure, set it to 0
 *  
 * on return:
 * 		0		success
 *		otherwise	failure
 *
 */

int
probe_mbr (struct master_and_dos_boot_sector *BS, unsigned long start_sector1, unsigned long sector_count1, unsigned long part_start1)
{
  unsigned long i, j;
  unsigned long lba_total_sectors = 0;
  
  /* probe the partition table */
  
  Cmax = 0; Hmax = 0; Smax = 0;
  for (i = 0; i < 4; i++)
    {
      int *part_entry;
      /* the boot indicator must be 0x80 (for bootable) or 0 (for non-bootable) */
      if ((unsigned char)(BS->P[i].boot_indicator << 1))/* if neither 0x80 nor 0 */
	return 1;
      /* check if the entry is empty, i.e., all the 16 bytes are 0 */
      part_entry = (int *)&(BS->P[i].boot_indicator);
      if (*part_entry++ || *part_entry++ || *part_entry++ || *part_entry)
      //if (*(long long *)&BS->P[i] || ((long long *)&BS->P[i])[1])
        {
	  /* valid partitions never start at 0, because this is where the MBR
	   * lives; and more, the number of total sectors should be non-zero.
	   */
	  if (! BS->P[i].start_lba || ! BS->P[i].total_sectors)
		return 2;
	  if (lba_total_sectors < BS->P[i].start_lba+BS->P[i].total_sectors)
	      lba_total_sectors = BS->P[i].start_lba+BS->P[i].total_sectors;
	  /* the partitions should not overlap each other */
	  for (j = 0; j < i; j++)
	  {
	    if ((BS->P[j].start_lba <= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors >= BS->P[i].start_lba + BS->P[i].total_sectors))
		continue;
	    if ((BS->P[j].start_lba >= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors <= BS->P[i].start_lba + BS->P[i].total_sectors))
		continue;
	    if ((BS->P[j].start_lba < BS->P[i].start_lba) ?
		(BS->P[i].start_lba - BS->P[j].start_lba < BS->P[j].total_sectors) :
		(BS->P[j].start_lba - BS->P[i].start_lba < BS->P[i].total_sectors))
		return 3;
	  }
	  /* the cylinder number */
	  C[i] = (BS->P[i].start_sector_cylinder >> 8) | ((BS->P[i].start_sector_cylinder & 0xc0) << 2);
	  if (Cmax < C[i])
		  Cmax = C[i];
	  H[i] = BS->P[i].start_head;
	  if (Hmax < H[i])
		  Hmax = H[i];
	  X = BS->P[i].start_sector_cylinder & 0x3f;/* the sector number */
	  if (Smax < X)
		  Smax = X;
	  /* the sector number should not be 0. */
	  ///* partitions should not start at the first track, the MBR-track */
	  if (! X /* || BS->P[i].start_lba < Smax */)
		return 4;
	  L[i] = BS->P[i].start_lba - X + 1;
	  if (start_sector1 == part_start1)/* extended partition is pretending to be a whole drive */
		L[i] +=(unsigned long) part_start1;
	
	  C[i+4] = (BS->P[i].end_sector_cylinder >> 8) | ((BS->P[i].end_sector_cylinder & 0xc0) << 2);
	  if (Cmax < C[i+4])
		  Cmax = C[i+4];
	  H[i+4] = BS->P[i].end_head;
	  if (Hmax < H[i+4])
		  Hmax = H[i+4];
	  Y = BS->P[i].end_sector_cylinder & 0x3f;
	  if (Smax < Y)
		  Smax = Y;
	  if (! Y)
		return 5;
	  L[i+4] = BS->P[i].start_lba + BS->P[i].total_sectors;
	  if (L[i+4] < Y)
		return 6;
	  L[i+4] -= Y;
	  if (start_sector1 == part_start1)/* extended partition is pretending to be a whole drive */
		L[i+4] +=(unsigned long) part_start1;
	  
   /* C[n] * (H_count * S_count) + H[n] * S_count = LBA[n] - S[n] + 1 = L[n] */

		/* C[n] * (H * S) + H[n] * S = L[n] */

	  /* Check the large disk partition -- Win98 */
	  if (Y == 63 && H[i+4] == Hmax && C[i+4] == Cmax
		&& (Hmax >= 254 || Cmax >= 1022)
		/* && C[i+4] == 1023 */
		&& (Cmax + 1) * (Hmax + 1) * 63 < L[i+4] + Y
		/* && C[i] * (Hmax + 1) * 63 + H[i] * 63 + X - 1 == BS->P[i].start_lba */
	     )
	  {
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 > L[i])
			return 7;
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 < L[i])
		{
		  /* calculate CHS numbers from start LBA */
		  if (X != ((unsigned long)L[i] % 63)+1 && X != 63)
			return 8;
		  if (H[i]!=((unsigned long)L[i]/63)%(Hmax+1) && H[i]!=Hmax)
			return 9;
		  if (C[i] != (((unsigned long)L[i]/63/(Hmax+1)) & 0x3ff) && C[i] != Cmax)
			return 10;
		}
		C[i] = 0; 
		H[i] = 1; 
		L[i] = 63;
		C[i+4] = 1; 
		H[i+4] = 0; 
		L[i+4] = (Hmax + 1) * 63;
	  }

	  /* Check the large disk partition -- Win2K */
	  else if (Y == 63 && H[i+4] == Hmax /* && C[i+4] == Cmax */
		&& (C[i+4] + 1) * (Hmax + 1) * 63 < L[i+4] + Y
		&& ! (((unsigned long)L[i+4] + Y) % ((Hmax + 1) * 63))
		&& ((((unsigned long)L[i+4] + Y) / ((Hmax + 1) * 63) - 1) & 0x3ff) == C[i+4]
	     )
	  {
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 > L[i])
			return 11;
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 < L[i])
		{
			if (((unsigned long)L[i] - H[i] * 63) % ((Hmax+1) * 63))
				return 12;
			if (((((unsigned long)L[i] - H[i] * 63) / ((Hmax+1) * 63)) & 0x3ff) != C[i])
				return 13;
		}
		C[i] = 0; 
		H[i] = 1; 
		L[i] = 63;
		C[i+4] = 1; 
		H[i+4] = 0; 
		L[i+4] = (Hmax + 1) * 63;
	  }

	  /* Maximum of C[n] * (H * S) + H[n] * S = 1023 * 255 * 63 + 254 * 63 = 0xFB03C1 */

	  else if (L[i+4] > 0xFB03C1) /* Large disk */
	  {
		/* set H/S to max */
		if (Hmax < 254)
		    Hmax = 254;
		Smax = 63;
		if ((unsigned long)L[i+4] % Smax)
		    return 114;
		if (H[i+4]!=((unsigned long)L[i+4]/63)%(Hmax+1) && H[i+4]!=Hmax)
		    return 115;
		if (C[i+4] != (((unsigned long)L[i+4]/63/(Hmax+1)) & 0x3ff) && C[i+4] != Cmax)
		    return 116;

		if (C[i] * (Hmax+1) * 63 + H[i] * 63 > L[i])
			return 117;
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 < L[i])
		{
		  /* calculate CHS numbers from start LBA */
		  if (X != ((unsigned long)L[i] % 63)+1 && X != 63)
			return 118;
		  if (H[i]!=((unsigned long)L[i]/63)%(Hmax+1) && H[i]!=Hmax)
			return 119;
		  if (C[i] != (((unsigned long)L[i]/63/(Hmax+1)) & 0x3ff) && C[i] != Cmax)
			return 120;
		}
		C[i] = 0;
		H[i] = 1;
		L[i] = 63;
		C[i+4] = 1;
		H[i+4] = 0;
		L[i+4] = (Hmax + 1) * 63;
	  }
        }
      else
        {
	  /* empty entry, zero all the coefficients */
	  C[i] = 0;
	  H[i] = 0;
	  L[i] = 0;
	  C[i+4] = 0;
	  H[i+4] = 0;
	  L[i+4] = 0;
        }
    }

//  for (i = 0; i < 4; i++)
//    {
//grub_printf ("%d   \t%d   \t%d\n%d   \t%d   \t%d\n", C[i], H[i],(int)(L[i]), C[i+4], H[i+4],(int)(L[i+4]));
//    }
  for (i = 0; i < 8; i++)
    {
      if (C[i])
	break;
    }
  if (i == 8) /* all C[i] == 0 */
    {
      for (i = 0; i < 8; i++)
	{
	  if (H[i])
	    break;
	}
      if (i == 8) /* all H[i] == 0 */
	return 14;
      for (j = 0; j < i; j++)
	if (L[j])
	  return 15;
      if (! L[i])
	  return 16;
      //if (*(long *)((char *)&(L[i]) + 4))
      if (L[i] > 0x7fffffff)
	  return 17;
      if ((long)L[i] % H[i])
	  return 18;
      probed_sectors_per_track = (long)L[i] / H[i];
      if (probed_sectors_per_track > 63 || probed_sectors_per_track < Smax)
	  return 19;
      Smax = probed_sectors_per_track;
      for (j = i + 1; j < 8; j++)
        {
	  if (H[j])
	    {
              if (probed_sectors_per_track * H[j] != L[j])
	        return 20;
	    }
	  else if (L[j])
	        return 21;
	}
      probed_heads = Hmax + 1;
#if 0
      if (sector_count1 == 1)
	  probed_cylinders = 1;
      else
#endif
        {
	  L[8] = sector_count1; /* just set to a number big enough */
	  Z = sector_count1 / probed_sectors_per_track;
	  for (j = probed_heads; j <= 256; j++)
	    {
	      H[8] = Z % j;/* the remainder */
	      if (L[8] > H[8])
		{
		  L[8] = H[8];/* the least residue */
		  probed_heads = j;/* we got the optimum value */
		}
	    }
	  probed_cylinders = Z / probed_heads;
	  if (! probed_cylinders)
		  probed_cylinders = 1; 
	}
      sectors_per_cylinder = probed_heads * probed_sectors_per_track;
      probed_total_sectors_round = sectors_per_cylinder * probed_cylinders;
      probed_total_sectors = lba_total_sectors;
    }
  else
    {
	    if (i > 0)
	    {
		C[8] = C[i]; H[8] = H[i]; L[8] = L[i];
		C[i] = C[0]; H[i] = H[0]; L[i] = L[0];
		C[0] = C[8]; H[0] = H[8]; L[0] = L[8];
	    }
	    H[8] = 0; /* will store sectors per track */
	    for (i = 1; i < 8; i++)
	    {
		H[i] = C[0] * H[i] - C[i] * H[0];
		L[i] = C[0] * L[i] - C[i] * L[0];
		if (H[i])
		{
			if (H[i] < 0)
			  {
			    H[i] = - H[i];/* H[i] < 0x080000 */
			    L[i] = - L[i];
			  }
			/* Note: the max value of H[i] is 1024 * 256 * 2 = 0x00080000, 
			 * so L[i] should be less than 0x00080000 * 64 = 0x02000000 */
			if (L[i] <= 0 || L[i] > 0x7fffffff)
				return 22;
			L[8] = ((long)L[i]) / H[i]; /* sectors per track */
			if (L[8] * H[i] != L[i])
				return 23;
			if (L[8] > 63 || L[8] < Smax)
				return 24;
			Smax = L[8];
			if (H[8])
			  {
				/* H[8] is the old L[8] */
				if (L[8] != H[8])
					return 25;
			  }
			else /* H[8] is empty, so store L[8] for the first time */
				H[8] = L[8];
		}
		else if (L[i])
			return 26;
	    }
	    if (H[8])
	    {
		/* H[8] is sectors per track */
		L[0] -= H[0] * H[8];
		/* Note: the max value of H[8] is 63, the max value of C[0] is 1023,
		 * so L[0] should be less than 64 * 1024 * 256 = 0x01000000	 */
		if (L[0] <= 0 || L[0] > 0x7fffffff)
			return 27;
		
		/* L[8] is number of heads */
		L[8] = ((long)L[0]) / H[8] / C[0];
		if (L[8] * H[8] * C[0] != L[0])
			return 28;
		if (L[8] > 256 || L[8] <= Hmax)
			return 29;
		probed_sectors_per_track = H[8];
	    }
	    else /* fail to set L[8], this means all H[i]==0, i=1,2,3,4,5,6,7 */
	    {
		/* Now the only equation is: C[0] * H * S + H[0] * S = L[0] */
		for (i = 63; i >= Smax; i--)
		{
			L[8] = L[0] - H[0] * i;
			if (L[8] <= 0 || L[8] > 0x7fffffff)
				continue;
			Z = L[8];
			if (Z % (C[0] * i))
				continue;
			L[8] = Z / (C[0] * i);
			if (L[8] <= 256 && L[8] > Hmax)
				break;/* we have got the PROBED_HEADS */
		}
		if (i < Smax)
			return 30;
		probed_sectors_per_track = i;
	    }
	    probed_heads = L[8];
      sectors_per_cylinder = probed_heads * probed_sectors_per_track;
      probed_cylinders = (sector_count1 + sectors_per_cylinder - 1) / sectors_per_cylinder;
      if (probed_cylinders < Cmax + 1)
	      probed_cylinders = Cmax + 1;
      probed_total_sectors_round = sectors_per_cylinder * probed_cylinders;
      probed_total_sectors = lba_total_sectors;
    }

  
  filesystem_type = 0;	/* MBR device */
  
  /* partition table probe success */
  return 0;
}
#endif

static unsigned long long L[8];
static unsigned long S[8];
static unsigned long H[8];
static unsigned long C[8];
static unsigned long X;
static unsigned long Y;
static unsigned long Cmax;
static unsigned long Hmax;
static unsigned long Smax;
static unsigned long Lmax;

/* on call:
 * 		BS		points to the bootsector
 * 		start_sector1	is the start_sector of the bootimage in the
 *				real disk, if unsure, set it to 0
 * 		sector_count1	is the sector_count of the bootimage in the
 *				real disk, if unsure, set it to 1
 * 		part_start1	is the part_start of the partition in which
 *				the bootimage resides, if unsure, set it to 0
 *  
 * on return:
 * 		0		success
 *		otherwise	failure
 *
 */
int
probe_mbr (struct master_and_dos_boot_sector *BS, unsigned long start_sector1, unsigned long sector_count1, unsigned long part_start1)
{
  unsigned long i, j;
  unsigned long lba_total_sectors = 0;
  unsigned long non_empty_entries;
  unsigned long HPC;
  unsigned long SPT;
  unsigned long solutions = 0;
  unsigned long ret_val;
  unsigned long active_partitions = 0;
  unsigned long best_HPC = 0;
  unsigned long best_SPT = 0;
  unsigned long best_bad_things = 0xFFFFFFFF;
  
  /* probe the partition table */
  
  Cmax = 0; Hmax = 0; Smax = 0; Lmax = 0;

#if 0
  if (filemax < 512)
  {
	if (debug > 1)
		printf ("Error: filesize(=%d) less than 512.\n"
			, (unsigned long)filemax);
	return 1;
  }
#endif

#if 0
  /* check signature */
  if (BS->boot_signature != 0xAA55)
  {
	if (debug > 1)
		printf ("Warning!!! No boot signature 55 AA.\n");
  }
#endif

  /* check boot indicator (0x80 or 0) */
  non_empty_entries = 0; /* count non-empty entries */
  for (i = 0; i < 4; i++)
    {
      int *part_entry;
      /* the boot indicator must be 0x80 (bootable) or 0 (non-bootable) */
      if ((unsigned char)(BS->P[i].boot_indicator << 1))/* if neither 0x80 nor 0 */
      {
	if (debug > 1)
		printf ("Error: invalid boot indicator(0x%X) for entry %d.\n"
			, (unsigned char)(BS->P[i].boot_indicator), i);
	ret_val = 2;
	goto err_print_hex;
      }
      if ((unsigned char)(BS->P[i].boot_indicator) == 0x80)
	active_partitions++;
      if (active_partitions > 1)
      {
	if (debug > 1)
		printf ("Error: duplicate active flag at entry %d.\n", i);
	ret_val = 3;
	goto err_print_hex;
      }
      /* check if the entry is empty, i.e., all the 16 bytes are 0 */
      part_entry = (int *)&(BS->P[i].boot_indicator);
      if (*part_entry++ || *part_entry++ || *part_entry++ || *part_entry)
      {
	  non_empty_entries++;
	  /* valid partitions never start at 0, because this is where the MBR
	   * lives; and more, the number of total sectors should be non-zero.
	   */
	  if (! BS->P[i].start_lba)
	  {
		if (debug > 1)
			printf ("Error: partition %d should not start at sector 0(the MBR sector).\n", i);
		ret_val = 4;
		goto err_print_hex;
	  }
	  if (! BS->P[i].total_sectors)
	  {
		if (debug > 1)
			printf ("Error: number of total sectors in partition %d should not be 0.\n", i);
		ret_val = 5;
		goto err_print_hex;
	  }
	  if (lba_total_sectors < BS->P[i].start_lba+BS->P[i].total_sectors)
	      lba_total_sectors = BS->P[i].start_lba+BS->P[i].total_sectors;
	  /* the partitions should not overlap each other */
	  for (j = 0; j < i; j++)
	  {
	    if ((BS->P[j].start_lba <= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors >= BS->P[i].start_lba + BS->P[i].total_sectors))
		continue;
	    if ((BS->P[j].start_lba >= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors <= BS->P[i].start_lba + BS->P[i].total_sectors))
		continue;
	    if ((BS->P[j].start_lba < BS->P[i].start_lba) ?
		(BS->P[i].start_lba - BS->P[j].start_lba < BS->P[j].total_sectors) :
		(BS->P[j].start_lba - BS->P[i].start_lba < BS->P[i].total_sectors))
	    {
		if (debug > 1)
			printf ("Error: overlapped partitions %d and %d.\n", j, i);
		ret_val = 6;
		goto err_print_hex;
	    }
	  }
	  /* the starting cylinder number */
	  C[i] = (BS->P[i].start_sector_cylinder >> 8) | ((BS->P[i].start_sector_cylinder & 0xc0) << 2);
	  if (Cmax < C[i])
	      Cmax = C[i];
	  /* the starting head number */
	  H[i] = BS->P[i].start_head;
	  if (Hmax < H[i])
	      Hmax = H[i];
	  /* the starting sector number */
	  X = ((BS->P[i].start_sector_cylinder) & 0x3f);
	  if (Smax < X)
	      Smax = X;
	  /* the sector number should not be 0. */
	  ///* partitions should not start at the first track, the MBR-track */
	  if (! X /* || BS->P[i].start_lba < Smax */)
	  {
		if (debug > 1)
			printf ("Error: starting S of entry %d should not be 0.\n", i);
		ret_val = 7;
		goto err_print_hex;
	  }
	  S[i] = X;
	  L[i] = BS->P[i].start_lba;// - X + 1;
	  if (start_sector1 == part_start1)/* extended partition is pretending to be a whole drive */
		L[i] +=(unsigned long) part_start1;
	  if (Lmax < L[i])
	      Lmax = L[i];
	
	  /* the ending cylinder number */
	  C[i+4] = (BS->P[i].end_sector_cylinder >> 8) | ((BS->P[i].end_sector_cylinder & 0xc0) << 2);
	  if (Cmax < C[i+4])
	      Cmax = C[i+4];
	  /* the ending head number */
	  H[i+4] = BS->P[i].end_head;
	  if (Hmax < H[i+4])
	      Hmax = H[i+4];
	  /* the ending sector number */
	  Y = ((BS->P[i].end_sector_cylinder) & 0x3f);
	  if (Smax < Y)
	      Smax = Y;
	  if (! Y)
	  {
		if (debug > 1)
			printf ("Error: ending S of entry %d should not be 0.\n", i);
		ret_val = 8;
		goto err_print_hex;
	  }
	  S[i+4] = Y;
	  L[i+4] = BS->P[i].start_lba + BS->P[i].total_sectors;
	  if (start_sector1 == part_start1)/* extended partition is pretending to be a whole drive */
		L[i+4] +=(unsigned long) part_start1;
	  if (L[i+4] < Y)
	  {
		if (debug > 1)
			printf ("Error: partition %d ended too near.\n", i);
		ret_val = 9;
		goto err_print_hex;
	  }
	  if (L[i+4] > 0x100000000ULL)
	  {
		if (debug > 1)
			printf ("Error: partition %d ended too far.\n", i);
		ret_val = 10;
		goto err_print_hex;
	  }
	  //L[i+4] -= Y;
	  //L[i+4] ++;
	  L[i+4] --;
	  if (Lmax < L[i+4])
	      Lmax = L[i+4];
      }
      else
      {
	  /* empty entry, zero out all the coefficients */
	  C[i] = 0;
	  H[i] = 0;
	  S[i] = 0;
	  L[i] = 0;
	  C[i+4] = 0;
	  H[i+4] = 0;
	  S[i+4] = 0;
	  L[i+4] = 0;
      }
    }	/* end for */
  if (non_empty_entries == 0)
  {
	if (debug > 1)
		printf ("Error: partition table is empty.\n");
	ret_val = 11;
	goto err_print_hex;
  }

  /* This can serve as a solution if there would be no solution. */
  if (debug > 1)
	printf ("Initial estimation: Cmax=%d, Hmax=%d, Smax=%d\n", Cmax, Hmax, Smax);

  /* Try each HPC in Hmax+1 .. 256 and each SPT in Smax .. 63 */

  for (SPT = Smax; SPT <= 63; SPT++)
  {
    for (HPC = (Hmax == 255) ? Hmax : Hmax + 1; HPC <= 256; HPC++)
    {
      unsigned long bad_things = 0;

      /* Check if this combination of HPC and SPT is OK */
      for (i = 0; i < 8; i++)
      {
	if (L[i])
	{
	  unsigned long C1, H1, S1;

	  /* Calculate C/H/S from LBA */
	  S1 = (((unsigned long)L[i]) % SPT) + 1;
	  H1 = (((unsigned long)L[i]) / SPT) % HPC;
	  C1 = ((unsigned long)L[i]) / (SPT * HPC);
	  /* check sanity */
#if 1
	  if (C1 <= 1023)
	  {
		if (C1 == C[i] && H1 == H[i] && S1 == S[i])
		{
			continue; /* this is OK */
		}
		if (/*C1 > C[i]*/ C1 == C[i]+1 && C[i] == Cmax && L[i] == Lmax && (H1 != HPC-1 || S1 != SPT) && (((H[i] == HPC-1 || (HPC == 255 && H[i] == 255)) && S[i] == SPT) || (H[i] == H1 && S[i] == S1)))
		{
			/* HP USB Disk Storage Format Tool. Bad!! */
			bad_things++;
			continue; /* accept it. */
		}
	  }
	  else
	  {
		if ((((C1 & 1023) == C[i] || 1023 == C[i]) && (S1 == S[i] || SPT == S[i]) && (H1 == H[i] || (HPC-1) == H[i])) || (1023 == C[i] && 255 == H[i] && 63 == S[i]))
		continue; /* this is OK */
	  }
#else
	  if ((C1 <= 1023) ?
		((C1 == C[i] && H1 == H[i] && S1 == S[i]) || (/*C1 > C[i]*/ C1 == C[i]+1 && C[i] == Cmax && (((H[i] == HPC-1 || (HPC == 255 && H[i] == 255)) && S[i] == SPT) || (H[i] == H1 && S[i] == S1))) )
		:
		((((C1 & 1023) == C[i] || 1023 == C[i]) && (S1 == S[i] || SPT == S[i]) && (H1 == H[i] || (HPC-1) == H[i])) || (1023 == C[i] && 255 == H[i] && 63 == S[i])))
		continue; /* this is OK */
#endif
	  /* failed, try next combination */
	  break;
	}
      }
      if (i >= 8) /* passed */
      {
	solutions++;
	if (HPC == 256)
		bad_things += 16;	/* not a good solution. */
	if (debug > 1)
		printf ("Solution %d(bad_things=%d): H=%d, S=%d.\n", solutions, bad_things, HPC, SPT);
	if (bad_things <= best_bad_things)
	{
		best_HPC = HPC;
		best_SPT = SPT;
		best_bad_things = bad_things;
	}
      }
    }
  }
  ret_val = 0;	/* initial value: partition table probe success */
  if (solutions == 0)
  {
    if ((Hmax == 254 || Hmax == 255) && Smax == 63)
    {
      if (debug > 1)
	printf ("Partition table is NOT GOOD and there is no solution.\n"
		"But there is a fuzzy solution: H=255, S=63.\n");
      best_HPC = 255;
      best_SPT = 63;
    }
    else
    {
      if (debug > 1)
	printf ("Sorry! No solution. Bad! Please report it.\n");
      ret_val = -1;	/* non-zero: partition table probe failure */
      goto err_print_decimal;	/* will not touch filesystem_type */
    }
  }
  else if (solutions == 1)
  {
    if (debug > 1)
    {
      if (best_bad_things == 0)
      {
	printf ("Perfectly Good!\n");
      //filesystem_type = 0;	/* MBR device */
      //return 0;	/* partition table probe success */
      }
      else
      {
	printf ("Found 1 solution, but the partition table has problems.\n");
      }
    }
  }
  else
  {
    if (debug > 1)
	printf ("Total solutions: %d (too many). The best one is:\n"
		"H=%d, S=%d.\n", solutions, best_HPC, best_SPT);
  }

  probed_sectors_per_track = best_SPT;
  probed_heads = best_HPC;
  sectors_per_cylinder = probed_heads * probed_sectors_per_track;
  probed_cylinders = (Lmax / sectors_per_cylinder) + 1;
  if (probed_cylinders < Cmax + 1)
      probed_cylinders = Cmax + 1;
  probed_total_sectors_round = sectors_per_cylinder * probed_cylinders;
  probed_total_sectors = Lmax + 1;

  filesystem_type = 0;	/* MBR device */
  if (solutions == 1 && best_bad_things == 0)
	return 0;	/* partition table probe success */

err_print_decimal:

  /* print the partition table in calculated decimal LBA C H S */
  if (debug > 1)
    for (i = 0; i < 4; i++)
    {
	printf ("%10ld %4d %3d %2d    %10ld %4d %3d %2d\n"
		, (unsigned long long)L[i], C[i], H[i], S[i]
		, (unsigned long long)L[i+4], C[i+4], H[i+4], S[i+4]);
    }

err_print_hex:

  /* print the partition table in Hex */
  if (debug > 1)
    for (i = 0; i < 4; i++)
    {
	printf ("%02X, %02X %02X %02X   %02X, %02X %02X %02X   %08X   %08X\n"
		, (unsigned char)(BS->P[i].boot_indicator)
		, BS->P[i].start_head
		, (unsigned char)(BS->P[i].start_sector_cylinder)
		, (unsigned char)(BS->P[i].start_sector_cylinder >> 8)
		, (unsigned char)(BS->P[i].system_indicator)
		, BS->P[i].end_head
		, (unsigned char)(BS->P[i].end_sector_cylinder)
		, (unsigned char)(BS->P[i].end_sector_cylinder >> 8)
		, BS->P[i].start_lba
		, BS->P[i].total_sectors);
    }

  return ret_val;
}

//#include "fat.h"

#ifndef GRUB_UTIL

unsigned long long map_mem_min = 0x100000;
unsigned long long map_mem_max = (-4096ULL);

/* map */
/* Map FROM_DRIVE to TO_DRIVE.  */
int
map_func (char *arg, int flags)
{
  char *to_drive;
  char *from_drive;
  unsigned long to, from, i = 0;
  int j;
  char *filename;
  char *p;
  unsigned long extended_part_start;
  unsigned long extended_part_length;

  int err;

  //struct master_and_dos_boot_sector *BS = (struct master_and_dos_boot_sector *) RAW_ADDR (0x8000);
#define	BS	((struct master_and_dos_boot_sector *)mbr)

  unsigned long long mem = -1ULL;
  int read_Only = 0;
  int fake_write = 0;
  int unsafe_boot = 0;
  int disable_lba_mode = 0;
  int disable_chs_mode = 0;
  unsigned long long sectors_per_track = -1ULL;
  unsigned long long heads_per_cylinder = -1ULL;
  unsigned long long to_filesize = 0;
  unsigned long BPB_H = 0;
  unsigned long BPB_S = 0;
  int in_situ = 0;
  int add_mbt = -1;
  int prefer_top = 0;
  filesystem_type = -1;
  start_sector = sector_count = 0;
  
  for (;;)
  {
    if (grub_memcmp (arg, "--status", 8) == 0)
      {
#ifndef GRUB_UTIL
//	if (debug > 0)
	{
	  print_bios_total_drives();
	  printf ("\nNumber of ATAPI CD-ROMs: %d\n", atapi_dev_count);
	}
#endif

	if (rd_base != -1ULL)
	    grub_printf ("\nram_drive=0x%X, rd_base=0x%lX, rd_size=0x%lX\n", ram_drive, rd_base, rd_size); 
	if (unset_int13_handler (1) && drive_map_slot_empty (bios_drive_map[0]))
	  {
	    grub_printf ("\nThe int13 hook is off. The drive map table is currently empty.\n");
	    return 1;
	  }

//struct drive_map_slot
//{
	/* Remember to update DRIVE_MAP_SLOT_SIZE once this is modified.
	 * The struct size must be a multiple of 4.
	 */

	  /* X=max_sector bit 7: read only or fake write */
	  /* Y=to_sector  bit 6: safe boot or fake write */
	  /* ------------------------------------------- */
	  /* X Y: meaning of restrictions imposed on map */
	  /* ------------------------------------------- */
	  /* 1 1: read only=0, fake write=1, safe boot=0 */
	  /* 1 0: read only=1, fake write=0, safe boot=0 */
	  /* 0 1: read only=0, fake write=0, safe boot=1 */
	  /* 0 0: read only=0, fake write=0, safe boot=0 */

//	unsigned char from_drive;
//	unsigned char to_drive;		/* 0xFF indicates a memdrive */
//	unsigned char max_head;
//	unsigned char max_sector;	/* bit 7: read only */
					/* bit 6: disable lba */

//	unsigned short to_cylinder;	/* max cylinder of the TO drive */
					/* bit 15:  TO  drive support LBA */
					/* bit 14:  TO  drive is CDROM(with big 2048-byte sector) */
					/* bit 13: FROM drive is CDROM(with big 2048-byte sector) */

//	unsigned char to_head;		/* max head of the TO drive */
//	unsigned char to_sector;	/* max sector of the TO drive */
					/* bit 7: in-situ */
					/* bit 6: fake-write or safe-boot */

//	unsigned long start_sector;
//	unsigned long start_sector_hi;	/* hi dword of the 64-bit value */
//	unsigned long sector_count;
//	unsigned long sector_count_hi;	/* hi dword of the 64-bit value */
//};

	/* From To MaxHead MaxSector ToMaxCylinder ToMaxHead ToMaxSector StartLBA_lo StartLBA_hi Sector_count_lo Sector_count_hi Hook Type */
//	if (debug > 0)
	  grub_printf ("\nFr To Hm Sm To_C _H _S   Start_Sector     Sector_Count   DHR"
		       "\n-- -- -- -- ---- -- -- ---------------- ---------------- ---\n");
	if (! unset_int13_handler (1))
	  for (i = 0; i < DRIVE_MAP_SIZE; i++)
	    {
		if (drive_map_slot_empty (hooked_drive_map[i]))
			break;
		for (j = 0; j < DRIVE_MAP_SIZE; j++)
		  {
		    if (drive_map_slot_equal(bios_drive_map[j], hooked_drive_map[i]))
			break;
		  }
//		if (debug > 0)
		  grub_printf ("%02X %02X %02X %02X %04X %02X %02X %016lX %016lX %c%c%c\n", hooked_drive_map[i].from_drive, hooked_drive_map[i].to_drive, hooked_drive_map[i].max_head, hooked_drive_map[i].max_sector, hooked_drive_map[i].to_cylinder, hooked_drive_map[i].to_head, hooked_drive_map[i].to_sector, (unsigned long long)hooked_drive_map[i].start_sector, (unsigned long long)hooked_drive_map[i].sector_count, ((hooked_drive_map[i].to_cylinder & 0x4000) ? 'C' : hooked_drive_map[i].to_drive < 0x80 ? 'F' : hooked_drive_map[i].to_drive == 0xFF ? 'M' : 'H'), ((j < DRIVE_MAP_SIZE) ? '=' : '>'), ((hooked_drive_map[i].max_sector & 0x80) ? ((hooked_drive_map[i].to_sector & 0x40) ? 'F' : 'R') :((hooked_drive_map[i].to_sector & 0x40) ? 'S' : 'U')));
	    }
	for (i = 0; i < DRIVE_MAP_SIZE; i++)
	  {
	    if (drive_map_slot_empty (bios_drive_map[i]))
		break;
	    if (! unset_int13_handler (1))
	      {
		for (j = 0; j < DRIVE_MAP_SIZE; j++)
		  {
		    if (drive_map_slot_equal(hooked_drive_map[j], bios_drive_map[i]))
			break;
		  }
		if (j < DRIVE_MAP_SIZE)
			continue;
	      }
//	    if (debug > 0)
		  grub_printf ("%02X %02X %02X %02X %04X %02X %02X %016lX %016lX %c<%c\n", bios_drive_map[i].from_drive, bios_drive_map[i].to_drive, bios_drive_map[i].max_head, bios_drive_map[i].max_sector, bios_drive_map[i].to_cylinder, bios_drive_map[i].to_head, bios_drive_map[i].to_sector, (unsigned long long)bios_drive_map[i].start_sector, (unsigned long long)bios_drive_map[i].sector_count, ((bios_drive_map[i].to_cylinder & 0x4000) ? 'C' : bios_drive_map[i].to_drive < 0x80 ? 'F' : bios_drive_map[i].to_drive == 0xFF ? 'M' : 'H'), ((bios_drive_map[i].max_sector & 0x80) ? ((bios_drive_map[i].to_sector & 0x40) ? 'F' : 'R') :((bios_drive_map[i].to_sector & 0x40) ? 'S' : 'U')));
	  }
	return 1;
      }
    else if (grub_memcmp (arg, "--hook", 6) == 0)
      {
	unsigned long long first_entry = -1ULL;

	p = arg + 6;
	if (*p == '=')
	{
		p++;
		if (! safe_parse_maxint (&p, &first_entry))
			return 0;
		if (first_entry > 0xFF)
			return ! (errnum = ERR_BAD_ARGUMENT);
	}
	unset_int13_handler (0);
	//if (! unset_int13_handler (1))
	//	return ! (errnum = ERR_INT13_ON_HOOK);
	if (drive_map_slot_empty (bios_drive_map[0]))
	    if (atapi_dev_count == 0)
		return ! (errnum = ERR_NO_DRIVE_MAPPED);
	if (first_entry <= 0xFF)
	{
	    /* setup first entry */
	    /* find first_entry in bios_drive_map */
	    for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
	    {
		if (drive_map_slot_empty (bios_drive_map[i]))
			break;	/* not found */
		if (bios_drive_map[i].from_drive == first_entry)
		{
			/* found */
			/* nothing to do if it is already the first entry */
			if (i == 0)
				break;
			/* backup this entry onto hooked_drive_map[0] */
			grub_memmove ((char *) &hooked_drive_map[0], (char *) &bios_drive_map[i], sizeof (struct drive_map_slot));
			/* move top entries downward */
			grub_memmove ((char *) &bios_drive_map[1], (char *) &bios_drive_map[0], sizeof (struct drive_map_slot) * i);
			/* restore this entry onto bios_drive_map[0] from hooked_drive_map[0] */
			grub_memmove ((char *) &bios_drive_map[0], (char *) &hooked_drive_map[0], sizeof (struct drive_map_slot));
			break;
		}
	    }
	}
	set_int13_handler (bios_drive_map);
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else if (grub_memcmp (arg, "--unhook", 8) == 0)
      {
	//if (unset_int13_handler (1))
	//	return ! (errnum = ERR_INT13_OFF_HOOK);
	unset_int13_handler (0);
//	int13_on_hook = 0;
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else if (grub_memcmp (arg, "--unmap=", 8) == 0)
      {
	int drive;
	char map_tmp[32];

	p = arg + 8;
	for (drive = 0xFF; drive >= 0; drive--)
	{
		if (drive != INITRD_DRIVE && in_range (p, drive)
		#ifdef FSYS_FB
		&& drive != FB_DRIVE
		#endif
		)
		{
			/* unmap drive */
			sprintf (map_tmp, "(0x%X) (0x%X)", drive, drive);
			map_func (map_tmp, flags);
		}
	}
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else if (grub_memcmp (arg, "--rehook", 8) == 0)
      {
	//if (unset_int13_handler (1))
	//	return ! (errnum = ERR_INT13_OFF_HOOK);
	unset_int13_handler (0);
//	int13_on_hook = 0;
	if (drive_map_slot_empty (bios_drive_map[0]))
	    if (atapi_dev_count == 0)
		return 1;//! (errnum = ERR_NO_DRIVE_MAPPED);
//	set_int13_handler (bios_drive_map);	/* backup bios_drive_map onto hooked_drive_map */
//	unset_int13_handler (0);	/* unhook it to avoid further access of hooked_drive_map by the call to map_func */
//	/* delete all memory mappings in hooked_drive_map */
//	for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
//	{
//	    while (hooked_drive_map[i].to_drive == 0xFF && !(hooked_drive_map[i].to_cylinder & 0x4000))
//	    {
//		grub_memmove ((char *) &hooked_drive_map[i], (char *) &hooked_drive_map[i + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - i));
//	    }
//	}
	/* clear hooked_drive_map */
	grub_memset ((char *) hooked_drive_map, 0, sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE));

	/* re-create (topdown) all memory map entries in hooked_drive_map from bios_drive_map */
	for (j = 0; j < DRIVE_MAP_SIZE - 1; j++)
	{
	    unsigned long long top_start = 0;
	    unsigned long top_entry = DRIVE_MAP_SIZE;
	    /* find the top memory mapping in bios_drive_map */
	    for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
	    {
		if (bios_drive_map[i].to_drive == 0xFF && !(bios_drive_map[i].to_cylinder & 0x4000))
		{
			if (top_start < bios_drive_map[i].start_sector)
			{
			    top_start = bios_drive_map[i].start_sector;
			    top_entry = i;
			}
		}
	    }
	    if (top_entry >= DRIVE_MAP_SIZE)	/* not found */
		break;				/* end */
	    /* move it to hooked_drive_map, by a copy and a delete */
	    grub_memmove ((char *) &hooked_drive_map[j], (char *) &bios_drive_map[top_entry], sizeof (struct drive_map_slot));
	    grub_memmove ((char *) &bios_drive_map[top_entry], (char *) &bios_drive_map[top_entry + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - top_entry));
	}
	/* now there should be no memory mappings in bios_drive_map. */

//	/* delete all memory mappings in bios_drive_map */
//	for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
//	{
//	    while (bios_drive_map[i].to_drive == 0xFF && !(bios_drive_map[i].to_cylinder & 0x4000))
//	    {
//		grub_memmove ((char *) &bios_drive_map[i], (char *) &bios_drive_map[i + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - i));
//	    }
//	}
	/* re-create all memory mappings stored in hooked_drive_map */
	for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
	{
	    if (hooked_drive_map[i].to_drive == 0xFF && !(hooked_drive_map[i].to_cylinder & 0x4000))
	    {
		char tmp[128];
#ifndef NO_DECOMPRESSION
		int no_decompression_bak = no_decompression;
		int is64bit_bak = is64bit;
#endif
		sprintf (tmp, "--heads=%d --sectors-per-track=%d (md)0x%lX+0x%lX (0x%X)", (hooked_drive_map[i].max_head + 1), ((hooked_drive_map[i].max_sector) & 63), (unsigned long long)hooked_drive_map[i].start_sector, (unsigned long long)hooked_drive_map[i].sector_count, hooked_drive_map[i].from_drive);

		if (debug > 1)
		{
			printf ("Re-map the memdrive (0x%X):\n\tmap %s\n", hooked_drive_map[i].from_drive, tmp);
		}
		errnum = 0;
		disable_map_info = 1;
#ifndef NO_DECOMPRESSION
		if (hooked_drive_map[i].from_drive == INITRD_DRIVE)
		{
			no_decompression = 1;
			is64bit = 0;
		}
#endif
		map_func (tmp, flags);
#ifndef NO_DECOMPRESSION
		if (hooked_drive_map[i].from_drive == INITRD_DRIVE)
		{
			no_decompression = no_decompression_bak;
			is64bit = is64bit_bak;
		}
#endif
		disable_map_info = 0;

		if (errnum)
		{
			if (debug > 0)
			{
				printf ("Fatal: Error %d occurred while 'map %s'. Please report this bug.\n", errnum, tmp);
			}
			return 0;
		}

		if (hooked_drive_map[i].from_drive == INITRD_DRIVE)
			linux_header->ramdisk_image = RAW_ADDR (initrd_start_sector << 9);
		/* change the map options */
		for (j = 0; j < DRIVE_MAP_SIZE - 1; j++)
		{
			if (bios_drive_map[j].from_drive == hooked_drive_map[i].from_drive && bios_drive_map[j].to_drive == 0xFF && !(bios_drive_map[j].to_cylinder & 0x4000))
			{
				bios_drive_map[j].max_head	= hooked_drive_map[i].max_head;
				bios_drive_map[j].max_sector	= hooked_drive_map[i].max_sector;
				bios_drive_map[j].to_cylinder	= hooked_drive_map[i].to_cylinder;
				bios_drive_map[j].to_head	= hooked_drive_map[i].to_head;
				bios_drive_map[j].to_sector	= hooked_drive_map[i].to_sector;
				break;
			}
		}
	    }
	}
	set_int13_handler (bios_drive_map);	/* hook it */
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else if (grub_memcmp (arg, "--floppies=", 11) == 0)
      {
	unsigned long long floppies;
	p = arg + 11;
	if (! safe_parse_maxint (&p, &floppies))
		return 0;
	if (floppies > 2)
		return ! (errnum = ERR_INVALID_FLOPPIES);
#ifndef GRUB_UTIL
	*((char *)0x410) &= 0x3e;
	if (floppies)
		*((char *)0x410) |= (floppies == 1) ? 1 : 0x41;
#endif
	return 1;
      }
    else if (grub_memcmp (arg, "--harddrives=", 13) == 0)
      {
	unsigned long long harddrives;
	p = arg + 13;
	if (! safe_parse_maxint (&p, &harddrives))
		return 0;
	if (harddrives > 127)
		return ! (errnum = ERR_INVALID_HARDDRIVES);
#ifndef GRUB_UTIL
	*((char *)0x475) = harddrives;
#endif
	return 1;
      }
    else if (grub_memcmp (arg, "--ram-drive=", 12) == 0)
      {
	unsigned long long tmp;
	p = arg + 12;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	if (tmp > 254)
		return ! (errnum = ERR_INVALID_RAM_DRIVE);
	ram_drive = tmp;
	
	return 1;
      }
    else if (grub_memcmp (arg, "--rd-base=", 10) == 0)
      {
	unsigned long long tmp;
	p = arg + 10;
	if (! safe_parse_maxint_with_suffix (&p, &tmp, 9))
		return 0;
//	if (tmp == 0xffffffff)
//		return ! (errnum = ERR_INVALID_RD_BASE);
	rd_base = tmp;
	
	return 1;
      }
    else if (grub_memcmp (arg, "--rd-size=", 10) == 0)
      {
	unsigned long long tmp;
	p = arg + 10;
	if (! safe_parse_maxint_with_suffix (&p, &tmp, 9))
		return 0;
//	if (tmp == 0)
//		return ! (errnum = ERR_INVALID_RD_SIZE);
	rd_size = tmp;
	
	return 1;
      }
    else if (grub_memcmp (arg, "--int15nolow=", 13) == 0)
      {
	unsigned long long tmp;
	p = arg + 13;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	int15nolow = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--memdisk-raw=", 14) == 0)
      {
	unsigned long long tmp;
	p = arg + 14;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	memdisk_raw = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--a20-keep-on=", 14) == 0)
      {
	unsigned long long tmp;
	p = arg + 14;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	a20_keep_on = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--safe-mbr-hook=", 16) == 0)
      {
	unsigned long long tmp;
	p = arg + 16;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	safe_mbr_hook = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--int13-scheme=", 15) == 0)
      {
	unsigned long long tmp;
	p = arg + 15;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	int13_scheme = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--mem-max=", 10) == 0)
      {
	unsigned long long num;
	p = arg + 10;
	if (! safe_parse_maxint_with_suffix (&p, &num, 9))
		return 0;
	map_mem_max = (num<<9) & (-4096ULL); // convert to bytes, 4KB alignment, round down
	if (is64bit) {
	  if (map_mem_max==0)
	      map_mem_max = (-4096ULL);
	} else {
	  if (map_mem_max==0 || map_mem_max>(1ULL<<32))
	      map_mem_max = (1ULL<<32); // 4GB
	}
	grub_printf("map_mem_max = 0x%lX sectors = 0x%lX bytes\n",map_mem_max>>9,map_mem_max);
	return 1;
      }
    else if (grub_memcmp (arg, "--mem-min=", 10) == 0)
      {
	unsigned long long num;
	p = arg + 10;
	if (! safe_parse_maxint_with_suffix (&p, &num, 9))
		return 0;
	map_mem_min = (((num<<9)+4095)&(-4096ULL)); // convert to bytes, 4KB alignment, round up
	if (map_mem_min < (1ULL<<20))
	    map_mem_min = (1ULL<<20); // 1MB
	grub_printf("map_mem_min = 0x%lX sectors = 0x%lX bytes\n",map_mem_min>>9,map_mem_min);
	return 1;
      }
    else if (grub_memcmp (arg, "--mem=", 6) == 0)
      {
	p = arg + 6;
	if (! safe_parse_maxint_with_suffix (&p, &mem, 9))
		return 0;
	if (mem == -1ULL)
		mem = -2ULL;//return errnum = ERR_INVALID_MEM_RESERV;
      }
    else if (grub_memcmp (arg, "--mem", 5) == 0)
      {
	mem = 0;
      }
    else if (grub_memcmp (arg, "--top", 5) == 0)
      {
	prefer_top = 1;
      }
    else if (grub_memcmp (arg, "--read-only", 11) == 0)
      {
	if (read_Only || fake_write || unsafe_boot)
		return !(errnum = ERR_SPECIFY_RESTRICTION);
	read_Only = 1;
	unsafe_boot = 1;
      }
    else if (grub_memcmp (arg, "--fake-write", 12) == 0)
      {
	if (read_Only || fake_write || unsafe_boot)
		return !(errnum = ERR_SPECIFY_RESTRICTION);
	fake_write = 1;
	unsafe_boot = 1;
      }
    else if (grub_memcmp (arg, "--unsafe-boot", 13) == 0)
      {
	if (unsafe_boot)
		return !(errnum = ERR_SPECIFY_RESTRICTION);
	unsafe_boot = 1;
      }
    else if (grub_memcmp (arg, "--disable-chs-mode", 18) == 0)
      {
	disable_chs_mode = 1;
      }
    else if (grub_memcmp (arg, "--disable-lba-mode", 18) == 0)
      {
	disable_lba_mode = 1;
      }
    else if (grub_memcmp (arg, "--in-place", 10) == 0)
      {
	in_situ = 2;
      }
    else if (grub_memcmp (arg, "--in-situ", 9) == 0)
      {
	in_situ = 1;
      }
    else if (grub_memcmp (arg, "--heads=", 8) == 0)
      {
	p = arg + 8;
	if (! safe_parse_maxint (&p, &heads_per_cylinder))
		return 0;
	if ((unsigned long long)heads_per_cylinder > 256)
		return ! (errnum = ERR_INVALID_HEADS);
      }
    else if (grub_memcmp (arg, "--sectors-per-track=", 20) == 0)
      {
	p = arg + 20;
	if (! safe_parse_maxint (&p, &sectors_per_track))
		return 0;
	if ((unsigned long long)sectors_per_track > 63)
		return ! (errnum = ERR_INVALID_SECTORS);
      }
    else if (grub_memcmp (arg, "--add-mbt=", 10) == 0)
      {
	unsigned long long num;
	p = arg + 10;
	if (! safe_parse_maxint (&p, &num))
		return 0;
	add_mbt = num;
	if (add_mbt < -1 || add_mbt > 1)
		return 0;
      }
    else
	break;
    arg = skip_to (0, arg);
  }
  
  to_drive = arg;
  from_drive = skip_to (0, arg);

//  if (grub_memcmp (from_drive, "(hd+)", 5) == 0)
//  {
//	from = (unsigned char)(0x80 + (*(unsigned char *)0x475));
//  }
//  else
//  {
  /* Get the drive number for FROM_DRIVE.  */
  set_device (from_drive);
  if (errnum)
    return 0;
  from = current_drive;
//  }

  if (! (from & 0x80) && in_situ)
	return ! (errnum = ERR_IN_SITU_FLOPPY);
  /* Get the drive number for TO_DRIVE.  */
  filename = set_device (to_drive);

  if (errnum) {
	/* No device specified. Default to the root device. */
	current_drive = saved_drive;
	//current_partition = saved_partition;
	filename = 0;
	errnum = 0;
  }
  
  to = current_drive;
  if (to == FB_DRIVE)
    to = (unsigned char)(fb_status >> 8)/* & 0xff*/;

  if (! (to & 0x80) && in_situ)
	return ! (errnum = ERR_IN_SITU_FLOPPY);

  /* if mem device is used, assume the --mem option */
  if (to == 0xffff || to == ram_drive || from == ram_drive)
  {
	if (mem == -1ULL)
		mem = 0;
  }

  if (mem != -1ULL && in_situ)
	return ! (errnum = ERR_IN_SITU_MEM);

  if (in_situ == 1)
  {
	unsigned long current_partition_bak;
       	char *filename_bak;
	char buf[16];

	current_partition_bak = current_partition;
	filename_bak = filename;
	
	/* read the first sector of drive TO */
	grub_sprintf (buf, "(%d)+1", to);
	if (! grub_open (buf))
		return 0;

	if (grub_read ((unsigned long long)SCRATCHADDR, SECTOR_SIZE, 0xedde0d90) != SECTOR_SIZE)
	{
		grub_close ();

		/* This happens, if the file size is less than 512 bytes. */
		if (errnum == ERR_NONE)
			errnum = ERR_EXEC_FORMAT;
 
		return 0;
	}
	grub_close ();

	current_partition = current_partition_bak;
	filename = filename_bak;

	/* try to find an empty entry in the partition table */
	for (i = 0; i < 4; i++)
	{
		if (! *(((char *)SCRATCHADDR) + 0x1C2 + i * 16))
			break;	/* consider partition type 00 as empty */
		if (! (*(((char *)SCRATCHADDR) + 0x1C0 + i * 16) & 63))
			break;	/* invalid start sector number of 0 */
		if (! (*(((char *)SCRATCHADDR) + 0x1C4 + i * 16) & 63))
			break;	/* invalid end sector number of 0 */
		if (! *(unsigned long *)(((char *)SCRATCHADDR) + 0x1C6 + i * 16))
			break;	/* invalid start LBA of 0 */
		if (! *(unsigned long *)(((char *)SCRATCHADDR) + 0x1CA + i * 16))
			break;	/* invalid sector count of 0 */
	}
	
	if (i >= 4)	/* No empty entry. The partition table is full. */
		return ! (errnum = ERR_PARTITION_TABLE_FULL);
  }
  
  if ((current_partition == 0xFFFFFF || (to >= 0x80 && to <= 0xFF)) && filename && (*filename == 0x20 || *filename == 0x09))
    {
      if (to == 0xffff /* || to == ram_drive */)
      {
	if (((long long)mem) <= 0)
	{
	  //grub_printf ("When mapping whole mem device at a fixed location, you must specify --mem to a value > 0.\n");
	  return ! (errnum = ERR_MD_BASE);
	}
	start_sector = (unsigned long long)mem;
	sector_count = 1;
	//goto map_whole_drive;
      }else if (to == ram_drive)
      {
	/* always consider this to be a fixed memory mapping */
	if ((rd_base & 0x1ff) || ! rd_base)
		return ! (errnum = ERR_RD_BASE);
	to = 0xffff;
	mem = (rd_base >> SECTOR_BITS);
	start_sector = (unsigned long long)mem;
	sector_count = 1;
	//goto map_whole_drive;
      }else{
        /* when whole drive is mapped, the geometry should not be specified. */
        if ((long long)heads_per_cylinder > 0 || (long long)sectors_per_track > 0)
	  return ! (errnum = ERR_SPECIFY_GEOM);
        /* when whole drive is mapped, the mem option should not be specified. 
         * but when we delete a drive map slot, the mem option means force.
         */
        if (mem != -1ULL && to != from)
	  return ! (errnum = ERR_SPECIFY_MEM);

        sectors_per_track = 1;/* 1 means the specified geometry will be ignored. */
        heads_per_cylinder = 1;/* can be any value but ignored since #sectors==1. */
        /* Note: if the user do want to specify geometry for whole drive map, then
         * use a command like this:
         * 
         * map --heads=H --sectors-per-track=S (hd0)+1 (hd1)
         * 
         * where S > 1
         */
        goto map_whole_drive;
      }
    }

  if (mem == -1ULL)
  {
    query_block_entries = -1; /* query block list only */
    blocklist_func (to_drive, flags);
    if (errnum)
	return 0;
    if (query_block_entries != 1 && mem == -1ULL)
	return ! (errnum = ERR_NON_CONTIGUOUS);

    start_sector = map_start_sector; /* in big 2048-byte sectors */
    //sector_count = map_num_sectors;
    sector_count = (filemax + 0x1ff) >> SECTOR_BITS; /* in small 512-byte sectors */

    if (start_sector == part_start && part_start && sector_count == 1)
	sector_count = part_length;
  }

  if ((to == 0xffff /* || to == ram_drive */) && sector_count == 1)
  {
    /* fixed memory mapping */
    grub_memmove64 ((unsigned long long)(unsigned long) BS, (start_sector << 9), SECTOR_SIZE);
  }else{
    if (! grub_open (to_drive))
	return 0;
    start_sector = sector_count = 0;
    rawread_ignore_memmove_overflow = 1;
    disk_read_hook = disk_read_start_sector_func;
    /* Read the first sector of the emulated disk.  */
    err = grub_read ((unsigned long long)(unsigned long) BS, SECTOR_SIZE, 0xedde0d90);
    disk_read_hook = 0;
    rawread_ignore_memmove_overflow = 0;
    if (err != SECTOR_SIZE && from != ram_drive)
    {
      grub_close ();
      /* This happens, if the file size is less than 512 bytes. */
      if (errnum == ERR_NONE)
	  errnum = ERR_EXEC_FORMAT;
      return 0;
    }

    to_filesize = gzip_filemax;
    sector_count = (filemax + 0x1ff) >> SECTOR_BITS; /* in small 512-byte sectors */
    if (part_length && start_sector == part_start /* && part_start */ && sector_count == 1)
    {
      if (mem != -1ULL)
      {
	char buf[32];

	grub_close ();
	sector_count = part_length;
        grub_sprintf (buf, "(%d)%ld+%ld", to, (unsigned long long)part_start, (unsigned long long)part_length);
        if (! grub_open (buf))
		return 0;
        filepos = SECTOR_SIZE;
      } else if (part_start) {
	sector_count = part_length;
      }
    }

    if (mem == -1ULL)
      grub_close ();
    if (to == 0xffff && sector_count == 1)
    {
      grub_printf ("For mem file in emulation, you should not specify sector_count to 1.\n");
      return 0;
    }
  }

  /* measure start_sector in small 512-byte sectors */
  if (buf_geom.sector_size == 2048)
	start_sector *= 4;

  if (filemax < 512 || BS->boot_signature != 0xAA55)
	goto geometry_probe_failed;
  
  /* probe the BPB */
  
  if (probe_bpb(BS))
	goto failed_probe_BPB;
  
  if (debug > 0 && ! disable_map_info)
    grub_printf ("%s BPB found %s 0xEB (jmp) leading the boot sector.\n", 
		filesystem_type == 1 ? "FAT12" :
		filesystem_type == 2 ? "FAT16" :
		filesystem_type == 3 ? "FAT32" :
		filesystem_type == 4 ? "NTFS"  :
		/*filesystem_type == 5 ?*/ "EXT2 GRLDR",
		(BS->dummy1[0] == (char)0xEB ? "with" : "but WITHOUT"));
  
  if (sector_count != 1)
    {
      if (debug > 0 && ! disable_map_info)
      {
	if (probed_total_sectors > sector_count)
	  grub_printf ("Warning: BPB total_sectors(%ld) is greater than the number of sectors in the whole disk image (%ld). The int13 handler will disable any read/write operations across the image boundary. That means you will not be able to read/write sectors (in absolute address, i.e., lba) %ld - %ld, though they are logically inside your file system.\n", (unsigned long long)probed_total_sectors, (unsigned long long)sector_count, (unsigned long long)sector_count, (unsigned long long)(probed_total_sectors - 1));
	else if (probed_total_sectors < sector_count && ! disable_map_info)
	  grub_printf ("info: BPB total_sectors(%ld) is less than the number of sectors in the whole disk image(%ld).\n", (unsigned long long)probed_total_sectors, (unsigned long long)sector_count);
      }
    }

  if (mem == -1ULL && (from & 0x80) && (to & 0x80) && (start_sector == part_start && part_start && sector_count == part_length)/* && BS->hidden_sectors >= probed_sectors_per_track */)
  {
	if (BS->hidden_sectors <= probed_sectors_per_track)
		BS->hidden_sectors = (unsigned long)part_start;
	extended_part_start = BS->hidden_sectors - probed_sectors_per_track;
	extended_part_length = probed_total_sectors + probed_sectors_per_track;
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nBPB probed C/H/S = %d/%d/%d, BPB probed total sectors = %ld\n", probed_cylinders, probed_heads, probed_sectors_per_track, (unsigned long long)probed_total_sectors);
	BPB_H = probed_heads;
	BPB_S = probed_sectors_per_track;
	if (in_situ)
	{
		//start_sector += probed_sectors_per_track;
		//sector_count -= probed_sectors_per_track;
		goto geometry_probe_ok;
	}
		
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("Try to locate extended partition (hd%d)%d+%d for the virtual (hd%d).\n", (to & 0x7f), extended_part_start, extended_part_length, (from & 0x7f));
	grub_sprintf ((char *)BS, "(hd%d)%d+%d", (to & 0x7f), extended_part_start, extended_part_length);
	//j = filesystem_type;	/* save filesystem_type */
	if (! grub_open ((char *)BS))
		return 0;

	//filesystem_type = j;	/* restore filesystem_type */
	/* Read the first sector of the emulated disk.  */
	if (grub_read ((unsigned long long)(unsigned int) BS, SECTOR_SIZE, 0xedde0d90) != SECTOR_SIZE)
	{
		grub_close ();

		/* This happens, if the file size is less than 512 bytes. */
		if (errnum == ERR_NONE)
			errnum = ERR_EXEC_FORMAT;
 
		return 0;
	}
	grub_close ();
	for (i = 0; i < 4; i++)
	{
		unsigned char sys = BS->P[i].system_indicator;
		if (sys == 0x05 || sys == 0x0f || sys == 0)
		{
			*(long long *)&(BS->P[i].boot_indicator) = 0LL;
			//*(long long *)&(BS->P[i].start_lba) = 0LL;
			((long long *)&(BS->P[i]))[1] = 0LL;
		}
	}
	part_start = extended_part_start;
	part_length = extended_part_length;
	start_sector = extended_part_start;
	sector_count = extended_part_length;
	
	/* when emulating a hard disk using a logical partition, the geometry should not be specified. */
	if ((long long)heads_per_cylinder > 0 || (long long)sectors_per_track > 0)
	  return ! (errnum = ERR_SPECIFY_GEOM);

	goto failed_probe_BPB;
	//if (probe_mbr (BS, start_sector, sector_count, (unsigned long)part_start) > 0)
	//	goto geometry_probe_failed;
  }
  //if (probed_cylinders * sectors_per_cylinder == probed_total_sectors)
	goto geometry_probe_ok;

failed_probe_BPB:
  /* probe the partition table */
  
  if (probe_mbr (BS, start_sector, sector_count, (unsigned long)part_start))
	goto geometry_probe_failed;

  if (sector_count != 1)
    {
      if (probed_total_sectors < probed_total_sectors_round && probed_total_sectors_round <= sector_count)
	  probed_total_sectors = probed_total_sectors_round;
      if (debug > 0 && ! disable_map_info)
      {
	if (probed_total_sectors > sector_count)
	  grub_printf ("Warning: total_sectors calculated from partition table(%ld) is greater than the number of sectors in the whole disk image (%ld). The int13 handler will disable any read/write operations across the image boundary. That means you will not be able to read/write sectors (in absolute address, i.e., lba) %ld - %ld, though they are logically inside your emulated virtual disk(according to the partition table).\n", (unsigned long long)probed_total_sectors, (unsigned long long)sector_count, (unsigned long long)sector_count, (unsigned long long)(probed_total_sectors - 1));
	else if (probed_total_sectors < sector_count)
	  grub_printf ("info: total_sectors calculated from partition table(%ld) is less than the number of sectors in the whole disk image(%ld).\n", (unsigned long long)probed_total_sectors, (unsigned long long)sector_count);
      }
    }
  
  goto geometry_probe_ok;

geometry_probe_failed:

  if (BPB_H || BPB_S)
  {
	if (mem != -1ULL)
		grub_close ();
	return ! (errnum = ERR_EXTENDED_PARTITION);
  }

  /* possible ISO9660 image */
  if (filemax < 512 || BS->boot_signature != 0xAA55 || (unsigned char)from >= (unsigned char)0xA0)
  {
	if ((long long)heads_per_cylinder < 0)
		heads_per_cylinder = 0;
	if ((long long)sectors_per_track < 0)
		sectors_per_track = 0;
  }

  if ((long long)heads_per_cylinder < 0)
    {
	if (from & 0x80)
	{
		if (mem != -1ULL)
			grub_close ();
		return ! (errnum = ERR_NO_HEADS);
	}
	/* floppy emulation */
	switch (sector_count)
	  {
		/* for the 5 standard floppy disks, offer a value */
		case  720: /*  360K */
		case 1440: /*  720K */
		case 2400: /* 1200K */
		case 2880: /* 1440K */
		case 5760: /* 2880K */
			heads_per_cylinder = 2; break;
		default:
			if (sector_count > 1 && sector_count < 5760 )
			  {
				heads_per_cylinder = 2; break;
			  }
			else
			  {
				if (mem != -1ULL)
					grub_close ();
				return ! (errnum = ERR_NO_HEADS);
			  }
	  }
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nAutodetect number-of-heads failed. Use default value %d\n", heads_per_cylinder);
    }
  else if (heads_per_cylinder == 0)
    {
	if (from & 0x80)
	  {
		/* hard disk emulation */
		heads_per_cylinder = 255; /* 256 */
	  }
	else
	  {
		/* floppy emulation */
		switch (sector_count)
		  {
			case 320: /* 160K */
			case 360: /* 180K */
			case 500: /* 250K */
				heads_per_cylinder = 1; break;
			case 1: /* a whole-disk emulation */
				heads_per_cylinder = 255; break;
			default:
				if (sector_count <= 63 * 1024 * 2 + 1)
					heads_per_cylinder = 2;
				else
					heads_per_cylinder = 255;
		  }
	  }
	if (debug > 0 && ! disable_map_info)
	  if (from < 0xA0)
	    grub_printf ("\nAutodetect number-of-heads failed. Use default value %d\n", heads_per_cylinder);
    }
  else
    {
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nAutodetect number-of-heads failed. Use the specified %d\n", heads_per_cylinder);
    }

  if ((long long)sectors_per_track < 0)
    {
	if (from & 0x80)
	{
		if (mem != -1ULL)
			grub_close ();
		return ! (errnum = ERR_NO_SECTORS);
	}
	/* floppy emulation */
	switch (sector_count)
	  {
		/* for the 5 standard floppy disks, offer a value */
		case  720: /*  360K */
		case 1440: /*  720K */
			sectors_per_track = 9; break;
		case 2400: /* 1200K */
			sectors_per_track = 15; break;
		case 2880: /* 1440K */
			sectors_per_track = 18; break;
		case 5760: /* 2880K */
			sectors_per_track = 36; break;
		default:
			if (sector_count > 1 && sector_count < 2880 )
			  {
				sectors_per_track = 18; break;
			  }
			else if (sector_count > 2880 && sector_count < 5760 )
			  {
				sectors_per_track = 36; break;
			  }
			else
			  {
				if (mem != -1ULL)
					grub_close ();
				return ! (errnum = ERR_NO_SECTORS);
			  }
	  }
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nAutodetect sectors-per-track failed. Use default value %d\n", sectors_per_track);
    }
  else if (sectors_per_track == 0)
    {
	if (sector_count == 1)
		sectors_per_track = 1;
	else if (from & 0x80)
	  {
		/* hard disk emulation */
		sectors_per_track = 63;
	  }
	else
	  {
		/* floppy emulation */
		switch (sector_count)
		  {
			case  400: /*  200K */
				sectors_per_track = 5; break;
			case  320: /*  160K */
			case  640: /*  320K */
			case 1280: /*  640K */
				sectors_per_track = 8; break;
			case  360: /*  180K */
			case  720: /*  360K */
			case 1440: /*  720K */
			case 1458: /*  729K */
			case 1476: /*  738K */
			case 1494: /*  747K */
			case 1512: /*  756K */
				sectors_per_track = 9; break;
			case  500: /*  250K */
			case  800: /*  400K */
			case  840: /*  420K */
			case 1000: /*  500K */
			case 1600: /*  800K */
			case 1620: /*  810K */
			case 1640: /*  820K */
			case 1660: /*  830K */
			case 1680: /*  840K */
				sectors_per_track = 10; break;
			case 1804: /*  902K */
				sectors_per_track = 11; break;
			case 1968: /*  984K */
				sectors_per_track = 12; break;
			case 2132: /* 1066K */
				sectors_per_track = 13; break;
			case 2400: /* 1200K */
			case 2430: /* 1215K */
			case 2460: /* 1230K */
			case 2490: /* 1245K */
			case 2520: /* 1260K */
				sectors_per_track = 15; break;
			case 2720: /* 1360K */
			case 2754: /* 1377K */
			case 2788: /* 1394K */
			case 2822: /* 1411K */
			case 2856: /* 1428K */
				sectors_per_track = 17; break;
			case 2880: /* 1440K */
			case 2916: /* 1458K */
			case 2952: /* 1476K */
			case 2988: /* 1494K */
			case 3024: /* 1512K */
				sectors_per_track = 18; break;
			case 3116: /* 1558K */
				sectors_per_track = 19; break;
			case 3200: /* 1600K */
			case 3240: /* 1620K */
			case 3280: /* 1640K */
			case 3320: /* 1660K */
				sectors_per_track = 20; break;
			case 3360: /* 1680K */
			case 3402: /* 1701K */
			case 3444: /* 1722K */
			case 3486: /* 1743K */
			case 3528: /* 1764K */
				sectors_per_track = 21; break;
			case 3608: /* 1804K */
				sectors_per_track = 22; break;
			case 3772: /* 1886K */
				sectors_per_track = 23; break;
			case 5760: /* 2880K */
				sectors_per_track = 36; break;
			case 6396: /* 3198K */
				sectors_per_track = 39; break;
			case 7216: /* 3608K */
				sectors_per_track = 44; break;
			case 7380: /* 3690K */
				sectors_per_track = 45; break;
			case 7544: /* 3772K */
				sectors_per_track = 46; break;
			default:
				if (sector_count <= 2881)
					sectors_per_track = 18;
				else if (sector_count <= 5761)
					sectors_per_track = 36;
				else
					sectors_per_track = 63;
		  }
	  }
	if (debug > 0 && ! disable_map_info)
	  if (from < 0xA0)
	    grub_printf ("\nAutodetect sectors-per-track failed. Use default value %d\n", sectors_per_track);
    }
  else
    {
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nAutodetect sectors-per-track failed. Use the specified %d\n", sectors_per_track);
    }
  goto map_whole_drive;

geometry_probe_ok:
  
  if (! disable_map_info)
  {
    if (debug > 0)
      grub_printf ("\nprobed C/H/S = %d/%d/%d, probed total sectors = %ld\n", probed_cylinders, probed_heads, probed_sectors_per_track, (unsigned long long)probed_total_sectors);
    if (mem != -1ULL && ((long long)mem) <= 0)
    {
      if (((unsigned long long)(-mem)) < probed_total_sectors && probed_total_sectors > 1 && filemax >= 512)
	mem = - (unsigned long long)probed_total_sectors;
    }
  }
  if (BPB_H || BPB_S)
	if (BPB_H != probed_heads || BPB_S != probed_sectors_per_track)
	{
		//if (debug > 0)
		//	grub_printf ("\nWarning!!! geometry (H/S=%d/%d) from the (extended) partition table\nconflict with geometry (H/S=%d/%d) in the BPB. The boot could fail!\n", probed_heads, probed_sectors_per_track, BPB_H, BPB_S);
		if (mem != -1ULL)
			grub_close ();
		return ! (errnum = ERR_EXTENDED_PARTITION);
	}
  
  if ((long long)heads_per_cylinder <= 0)
	heads_per_cylinder = probed_heads;
  else if (heads_per_cylinder != probed_heads)
    {
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nWarning!! Probed number-of-heads(%d) is not the same as you specified.\nThe specified value %d takes effect.\n", probed_heads, heads_per_cylinder);
    }

  if ((long long)sectors_per_track <= 0)
	sectors_per_track = probed_sectors_per_track;
  else if (sectors_per_track != probed_sectors_per_track)
    {
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nWarning!! Probed sectors-per-track(%d) is not the same as you specified.\nThe specified value %d takes effect.\n", probed_sectors_per_track, sectors_per_track);
    }

map_whole_drive:

  if (from != ram_drive)
  {
    /* Search for an empty slot in BIOS_DRIVE_MAP.  */
    for (i = 0; i < DRIVE_MAP_SIZE; i++)
    {
      /* Perhaps the user wants to override the map.  */
      if (bios_drive_map[i].from_drive == from)
	break;
      
      if (drive_map_slot_empty (bios_drive_map[i]))
	break;
    }

    if (i == DRIVE_MAP_SIZE)
    {
      if (mem != -1ULL)
        grub_close ();
      errnum = ERR_WONT_FIT;
      return 0;
    }

    /* If TO == FROM and whole drive is mapped, and, no map options occur, then delete the entry.  */
    if (to == from && read_Only == 0 && fake_write == 0 && disable_lba_mode == 0
      && disable_chs_mode == 0 && start_sector == 0 && (sector_count == 0 ||
	/* sector_count == 1 if the user uses a special method to map a whole drive, e.g., map (hd1)+1 (hd0) */
     (sector_count == 1 && (long long)heads_per_cylinder <= 0 && (long long)sectors_per_track <= 1)))
	goto delete_drive_map_slot;
  }

  /* check whether TO is being mapped */
  //if (mem == -1)
  for (j = 0; j < DRIVE_MAP_SIZE; j++)
    {
      if (unset_int13_handler (1))
	  continue;	/* not hooked */
      if (to != hooked_drive_map[j].from_drive)
	  continue;
#if 0
	  if (! (hooked_drive_map[j].max_sector & 0x3F))
	      disable_chs_mode = 1;

	  /* X=max_sector bit 7: read only or fake write */
	  /* Y=to_sector  bit 6: safe boot or fake write */
	  /* ------------------------------------------- */
	  /* X Y: meaning of restrictions imposed on map */
	  /* ------------------------------------------- */
	  /* 1 1: read only=0, fake write=1, safe boot=0 */
	  /* 1 0: read only=1, fake write=0, safe boot=0 */
	  /* 0 1: read only=0, fake write=0, safe boot=1 */
	  /* 0 0: read only=0, fake write=0, safe boot=0 */

	  if (!(read_Only | fake_write | unsafe_boot))	/* no restrictions specified */
	  switch ((hooked_drive_map[j].max_sector & 0x80) | (hooked_drive_map[j].to_sector & 0x40))
	  {
		case 0xC0: read_Only = 0; fake_write = 1; unsafe_boot = 1; break;
		case 0x80: read_Only = 1; fake_write = 0; unsafe_boot = 1; break;
		case 0x00: read_Only = 0; fake_write = 0; unsafe_boot = 1; break;
	      /*case 0x40:*/
		default:   read_Only = 0; fake_write = 0; unsafe_boot = 0;
	  }

	  if (hooked_drive_map[j].max_sector & 0x40)
	      disable_lba_mode = 1;
#endif

	  to = hooked_drive_map[j].to_drive;
	  if (to == 0xFF && !(hooked_drive_map[j].to_cylinder & 0x4000))
		to = 0xFFFF;		/* memory device */
	  if (start_sector == 0 && (sector_count == 0 || (sector_count == 1 && (long long)heads_per_cylinder <= 0 && (long long)sectors_per_track <= 1)))
	  {
		sector_count = hooked_drive_map[j].sector_count;
		heads_per_cylinder = hooked_drive_map[j].max_head + 1;
		sectors_per_track = (hooked_drive_map[j].max_sector) & 0x3F;
	  }
	  start_sector += hooked_drive_map[j].start_sector;
      
	  /* If TO == FROM and whole drive is mapped, and, no map options occur, then delete the entry.  */
	  if (to == from && read_Only == 0 && fake_write == 0 && disable_lba_mode == 0
	      && disable_chs_mode == 0 && start_sector == 0 && (sector_count == 0 ||
	  /* sector_count == 1 if the user uses a special method to map a whole drive, e.g., map (hd1)+1 (hd0) */
	     (sector_count == 1 && (long long)heads_per_cylinder <= 0 && (long long)sectors_per_track <= 1)))
	    {
		/* yes, delete the FROM drive(with slot[i]), not the TO drive(with slot[j]) */
		if (from != ram_drive)
			goto delete_drive_map_slot;
	    }

	  break;
    }

  j = i;	/* save i into j */
//grub_printf ("\n debug 4 start_sector=%lX, part_start=%lX, part_length=%lX, sector_count=%lX, filemax=%lX\n", start_sector, part_start, part_length, sector_count, filemax);
  
#ifndef GRUB_UTIL
  /* how much memory should we use for the drive emulation? */
  if (mem != -1ULL)
    {
      unsigned long long start_byte;
      unsigned long long bytes_needed;
      unsigned long long base;
      unsigned long long top_end;
      
      bytes_needed = base = top_end = 0ULL;
//if (to == 0xff)
//{
//	
//}else{
      if (start_sector == part_start && part_start == 0 && sector_count == 1)
		sector_count = part_length;

      /* For GZIP disk image if uncompressed size >= 4GB, 
         high bits of filemax is wrong, sector_count is also wrong. */
      #if 0
      if ( compressed_file && (sector_count < probed_total_sectors) && filesystem_type > 0 )
      {   /* adjust high bits of filemax and sector_count */
	  unsigned long sizehi = (unsigned long)(probed_total_sectors >> (32-SECTOR_BITS));
	  if ( (unsigned long)filemax < (unsigned long)(probed_total_sectors << SECTOR_BITS) )
	      ++sizehi;
	  fsmax = filemax += (unsigned long long)sizehi << 32;
	  sector_count = filemax >> SECTOR_BITS;
	  grub_printf("Uncompressed size is probably %ld sectors\n",sector_count);
      }
		#endif
      if ( (long long)mem < 0LL && sector_count < (-mem) )
	bytes_needed = (-mem) << SECTOR_BITS;
      else
	bytes_needed = sector_count << SECTOR_BITS;

      /* filesystem_type
       *	 0		an MBR device
       *	 1		FAT12
       *	 2		FAT16
       *	 3		FAT32
       *	 4		NTFS
       *	-1		unknown filesystem(do not care)
       *	
       * Note: An MBR device is a whole disk image that has a partition table.
       */

      if (add_mbt<0)
	  add_mbt = (filesystem_type > 0 && (from & 0x80) && (from < 0xA0))? 1: 0; /* known filesystem without partition table */

      if (add_mbt)
	bytes_needed += sectors_per_track << SECTOR_BITS;	/* build the Master Boot Track */

      bytes_needed = ((bytes_needed+4095)&(-4096ULL));	/* 4KB alignment */
//}
      if ((to == 0xffff || to == ram_drive) && sector_count == 1)
	/* mem > 0 */
	bytes_needed = 0;
      //base = 0;

      start_byte = start_sector << SECTOR_BITS;
      if (to == ram_drive)
	start_byte += rd_base;
  /* we can list the total memory thru int15/E820. on the other hand, we can
   * see thru our drive map slots how much memory have already been used for
   * drive emulation. */
      /* the available memory can be calculated thru int15 and the slots. */
      /* total memory can always be calculated thru unhooked int15 */
      /* used memory can always be calculated thru drive map slots */
      /* the int15 handler:
       *   if not E820, return to the original int15
       *   for each E820 usable memory, check how much have been used in slot,
       *     and return the usable memory minus used memory.
       */
      /* once int13 hooked and at least 1 slot uses mem==1, also hook int15
       */

      if (map_mem_max > 0x100000000ULL && ! is64bit)
	  map_mem_max = 0x100000000ULL;
      if (map_mem_min < 0x100000ULL)
	  map_mem_min = 0x100000ULL;

      if (mbi.flags & MB_INFO_MEM_MAP)
        {
          struct AddrRangeDesc *map = (struct AddrRangeDesc *) saved_mmap_addr;
          unsigned long end_addr = saved_mmap_addr + saved_mmap_length;

          for (; end_addr > (unsigned long) map; map = (struct AddrRangeDesc *) (((int) map) + 4 + map->size))
	    {
	      unsigned long long tmpbase, tmpend, tmpmin;
	      if (map->Type != MB_ARD_MEMORY)
		  continue;
	      tmpmin = (map->BaseAddr > map_mem_min) ?
			map->BaseAddr : map_mem_min;
	      tmpmin = ((tmpmin+4095)&(-4096ULL));/* 4KB alignment, round up */
	      tmpend = (map->BaseAddr + map->Length < map_mem_max) ?
			map->BaseAddr + map->Length : map_mem_max;
	      tmpend &= (-4096ULL);	/* 4KB alignment, round down */
	      if (tmpend < bytes_needed)
		  continue;
	      tmpbase = tmpend - bytes_needed; // maximum possible base for this region
	      //grub_printf("range b %lx l %lx -- tm %lx te %lx tb %lx\n",map->BaseAddr,map->Length,tmpmin,tmpend,tmpbase);
	      if (((long long)mem) > 0)//force the base value
	      {
		  if (tmpbase >= (mem << 9))
		      tmpbase =  (mem << 9); // move tmpbase down to forced value
		  else // mem base exceed maximum possible base for this region
		      continue;
	      }
	      if (tmpbase < tmpmin) // base fall below minimum address for this region
		  continue;

	      for (i = 0; i < DRIVE_MAP_SIZE; i++)
	        {
      
		  if (drive_map_slot_empty (bios_drive_map[i]))
			break;

		  /* TO_DRIVE == 0xFF indicates a MEM map */
		  if (bios_drive_map[i].to_drive == 0xFF && !(bios_drive_map[i].to_cylinder & 0x4000))
	            {
		      unsigned long long drvbase, drvend;
		      drvbase = (bios_drive_map[i].start_sector << SECTOR_BITS);
		      drvend  = (bios_drive_map[i].sector_count << SECTOR_BITS) + drvbase;
		      drvend  = ((drvend+4095)&(-4096ULL));/* 4KB alignment, round up */
		      drvbase &= (-4096ULL);	/* 4KB alignment, round down */
		      //grub_printf("drv %02x: db %lx de %lx -- tb %lx te %lx\n",bios_drive_map[i].from_drive,drvbase,drvend,tmpbase,tmpend);
		      if (tmpbase < drvend && drvbase < tmpend)
			{ // overlapped address, move tmpend and tmpbase down
			  tmpend = drvbase;
			  if (tmpend >= bytes_needed)
			  {
			      if ((long long)mem <= 0)
			      { // decrease tmpbase
				  tmpbase = tmpend - bytes_needed;
				  if (tmpbase >= tmpmin)
				  { // recheck new tmpbase 
				      i = -1; continue;
				  }
			      }
			      else 
			      { // force base value, cannot decrease tmpbase
				  /* changed "<" to "<=". -- tinybit */
				  if (tmpbase <= tmpend - bytes_needed)
				  { // there is still enough space below drvbase
				      continue;
				  }
			      }
			  }
			  /* not enough room for this memory area */
			//  grub_printf("region can't be used bn %lx te %lx  tb %lx\n",bytes_needed,tmpend,tmpbase);
			  tmpbase = 0; break;
			}
		    }
	        } /* for (i = 0; i < DRIVE_MAP_SIZE; i++) */

	      if (tmpbase >= tmpmin)
	      {
		/* check if the memory area overlaps the (md)... or (rd)... file. */
		if ( (to != 0xffff && to != ram_drive)	/* TO is not in memory */
		  || ((long long)mem) > 0	/* TO is in memory but is mapped at a fixed location */
		  || !compressed_file		/* TO is in memory and is normally mapped and uncompressed */
		/* Now TO is in memory and is normally mapped and compressed */
		  || tmpbase + bytes_needed <= start_byte  /* XXX: destination is below the gzip image */
		  || tmpbase >= start_byte + to_filesize) /* XXX: destination is above the gzip image */
		{
		    if (prefer_top)
		    {
			if (base < tmpbase)
			{
			    base = tmpbase; top_end = tmpend;
			}
			continue; // find available region with highest address
		    }
		    else
		    {
			base = tmpbase; top_end = tmpend;
			break; // use the first available region
		    }
		}
		else
		    /* destination overlaps the gzip image */
		    /* fail and try next memory block. */
		    continue;
	      }
	      //if (to == 0xff)
		//bytes_needed = 0;

	    } /* for (; end_addr > (int) map; map = (struct AddrRangeDesc *) (((int) map) + 4 + map->size)) */

        } /* if (mbi.flags & MB_INFO_MEM_MAP) */
      else
	  grub_printf ("\nFatal: Your BIOS has no support for System Memory Map(INT15/EAX=E820h).\nAs a result you cannot use the --mem option.\n");

      if (base < map_mem_min)
      {
	  grub_close ();
	  return ! (errnum = ERR_WONT_FIT);
      }

      if (((long long)mem) > 0)
      {
	  base = ((unsigned long long)mem << 9);
	  sector_count = (top_end - base) >> SECTOR_BITS;
      }else{
	  sector_count = bytes_needed >> SECTOR_BITS;
	  // now sector_count may be > part_length, reading so many sectors could cause failure
      }

      bytes_needed = base;
      if (add_mbt)	/* no partition table */
	bytes_needed += sectors_per_track << SECTOR_BITS;	/* build the Master Boot Track */

      /* bytes_needed points to the first partition, base points to MBR */
      
//#undef MIN_EMU_BASE
      //      if (! grub_open (filename1))
      //	return 0;
	
      /* the first sector already read at BS */
      #if 0
      if ((to != 0xffff && to != ram_drive) || ((long long)mem) <= 0)
	{
		#endif
	  /* if image is in memory and not compressed, we can simply move it. */
	  if ((to == 0xffff || to == ram_drive) && !compressed_file)
	  {
	    if (bytes_needed != start_byte)
		grub_memmove64 (bytes_needed, start_byte, filemax);
	  } else {
	    unsigned long long read_result;
	    grub_memmove64 (bytes_needed, (unsigned long long)(unsigned int)BS, SECTOR_SIZE);
	    /* read the rest of the sectors */
	    if (filemax > SECTOR_SIZE)
	    {
	      read_result = grub_read ((bytes_needed + SECTOR_SIZE), -1ULL, 0xedde0d90);
	      if (read_result != filemax - SECTOR_SIZE)
	      {
		//if ( !probed_total_sectors || read_result<(probed_total_sectors<<SECTOR_BITS) )
		//{
		unsigned long long required = (probed_total_sectors << SECTOR_BITS) - SECTOR_SIZE;
		/* read again only required sectors */
		if ( ! probed_total_sectors 
		     || required >= filemax - SECTOR_SIZE
		     || ( (filepos = SECTOR_SIZE), /* re-read from the second sector. */
			  grub_read (bytes_needed + SECTOR_SIZE, required, 0xedde0d90) != required
			)
		   )
		{
		    grub_close ();
		    if (errnum == ERR_NONE)
			errnum = ERR_READ;
		    return 0;
		}
		//}
	      }
	    }
	  }
	  grub_close ();
	 #if 0
	}
      else if (/*(to == 0xffff || to == ram_drive) && */!compressed_file)
	{
	    if (bytes_needed != start_byte)
		grub_memmove64 (bytes_needed, start_byte, filemax);
		grub_close ();
	}
	else
     {
		grub_memmove64 (bytes_needed, (unsigned long long)(unsigned int)BS, SECTOR_SIZE);
        grub_read (bytes_needed + SECTOR_SIZE, -1ULL, 0xedde0d90);
        grub_close ();
      }
	#endif
      start_sector = base >> SECTOR_BITS;
      to = 0xFFFF/*GRUB_INVALID_DRIVE*/;

      if (add_mbt)	/* no partition table */
      {
	unsigned long sectors_per_cylinder1, cylinder, sector_rem;
	unsigned long head, sector;

	/* First sector of disk image has already been read to buffer BS/mbr */
	
	/* modify the BPB drive number. required for FAT12/16/32/NTFS */
	if (filesystem_type != -1)
	if (!*(char *)((int)BS + ((filesystem_type == 3) ? 0x41 : 0x25)))
		*(char *)((int)BS + ((filesystem_type == 3) ? 0x40 : 0x24)) = from;
	
	/* modify the BPB hidden sectors. required for FAT12/16/32/NTFS/EXT2 */
	if (filesystem_type != -1 || *(unsigned long *)((int)BS + 0x1c) == (unsigned long)part_start)
	    *(unsigned long *)((int)BS + 0x1c) = sectors_per_track;
	
	grub_memmove64 (bytes_needed, (unsigned long long)(unsigned int)BS, SECTOR_SIZE);
	
	/* clear BS/mbr buffer */
	grub_memset(mbr, 0, SECTOR_SIZE);
	
	/* clear MS magic number */
	//*(long *)((int)mbr + 0x1b8) = 0; 
	/* Let MS magic number = virtual BIOS drive number */
	*(long *)((int)mbr + 0x1b8) = (unsigned char)from; 
	/* build the partition table */
	*(long *)((int)mbr + 0x1be) = 0x00010180L; 
	*(char *)((int)mbr + 0x1c2) = //((filesystem_type == -1 || filesystem_type == 5)? 0x83 : filesystem_type == 4 ? 0x07 : 0x0c); 
		filesystem_type == 1 ? 0x0E /* FAT12 */ :
		filesystem_type == 2 ? 0x0E /* FAT16 */ :
		filesystem_type == 3 ? 0x0C /* FAT32 */ :
		filesystem_type == 4 ? 0x07 /* NTFS */  :
		/*filesystem_type == 5 ?*/ 0x83 /* EXT2 */;

	//sector_count += sectors_per_track;	/* already incremented above */

	/* calculate the last sector's C/H/S value */
	sectors_per_cylinder1 = sectors_per_track * heads_per_cylinder;
	cylinder = ((unsigned long)(sector_count - 1)) / sectors_per_cylinder1;	/* XXX: 64-bit div */
	sector_rem = ((unsigned long)(sector_count - 1)) % sectors_per_cylinder1; /* XXX: 64-bit mod */
	head = sector_rem / (unsigned long)sectors_per_track;
	sector = sector_rem % (unsigned long)sectors_per_track;
	sector++;
	*(char *)((int)mbr + 0x1c3) = head;
	*(unsigned short *)((int)mbr + 0x1c4) = sector | (cylinder << 8) | ((cylinder >> 8) << 6);
	*(unsigned long *)((int)mbr + 0x1c6) = sectors_per_track; /* start LBA */
	*(unsigned long *)((int)mbr + 0x1ca) = sector_count - sectors_per_track; /* sector count */
	*(long long *)((int)mbr + 0x1ce) = 0LL; 
	*(long long *)((int)mbr + 0x1d6) = 0LL; 
	*(long long *)((int)mbr + 0x1de) = 0LL; 
	*(long long *)((int)mbr + 0x1e6) = 0LL; 
	*(long long *)((int)mbr + 0x1ee) = 0LL; 
	*(long long *)((int)mbr + 0x1f6) = 0LL; 
	*(unsigned short *)((int)mbr + 0x1fe) = 0xAA55;
	
	/* compose a master boot record routine */
	*(char *)((int)mbr) = 0xFA;	/* cli */
	*(unsigned short *)((int)mbr + 0x01) = 0xC033; /* xor AX,AX */
	*(unsigned short *)((int)mbr + 0x03) = 0xD08E; /* mov SS,AX */
	*(long *)((int)mbr + 0x05) = 0xFB7C00BC; /* mov SP,7C00 ; sti */
	*(long *)((int)mbr + 0x09) = 0x07501F50; /* push AX; pop DS */
						  /* push AX; pop ES */
	*(long *)((int)mbr + 0x0d) = 0x7C1CBEFC; /* cld; mov SI,7C1C */
	*(long *)((int)mbr + 0x11) = 0x50061CBF; /* mov DI,061C ; push AX */
	*(long *)((int)mbr + 0x15) = 0x01E4B957; /* push DI ; mov CX, 01E4 */
	*(long *)((int)mbr + 0x19) = 0x1ECBA4F3; /* repz movsb;retf;push DS */
	*(long *)((int)mbr + 0x1d) = 0x537C00BB; /* mov BX,7C00 ; push BX */
	*(long *)((int)mbr + 0x21) = 0x520180BA; /* mov DX,0180 ; push DX */
	*(long *)((int)mbr + 0x25) = 0x530201B8; /* mov AX,0201 ; push BX */
	*(long *)((int)mbr + 0x29) = 0x5F13CD41; /* inc CX; int 13; pop DI */
	*(long *)((int)mbr + 0x2d) = 0x5607BEBE; /* mov SI,07BE ; push SI */
	*(long *)((int)mbr + 0x31) = 0xCBFA5A5D; /* pop BP;pop DX;cli;retf */
	grub_memmove64 (base, (unsigned long long)(unsigned int)mbr, SECTOR_SIZE);
      }
      else if ((from & 0x80) && (from < 0xA0))	/* the virtual drive is hard drive */
      {
	/* First sector of disk image has already been read to buffer BS/mbr */
	if (*(long *)((int)mbr + 0x1b8) == 0)
	  /* Let MS magic number = virtual BIOS drive number */
	  *(long *)((int)mbr + 0x1b8) = (unsigned char)from; 
	else if ((*(long *)((int)mbr + 0x1b8) & 0xFFFFFF00) == 0)
	  *(long *)((int)mbr + 0x1b8) |= (from << 8); 
	grub_memmove64 (base, (unsigned long long)(unsigned int)mbr, SECTOR_SIZE);
      }

      /* if FROM is (rd), no mapping is established. but the image will be
       * loaded into memory, and (rd) will point to it. Note that Master
       * Boot Track and MBR code have been built as above when needed
       * for ram_drive > 0x80.
       */
      if (from == ram_drive)
      {
	rd_base = base;
	rd_size = filemax;//(sector_count << SECTOR_BITS);
	if (add_mbt)
		rd_size += sectors_per_track << SECTOR_BITS;	/* build the Master Boot Track */
	return 1;
      }
    }
#endif	/* GRUB_UTIL */

  if (in_situ)
	bios_drive_map[j].to_cylinder = 
		filesystem_type == 1 ? 0x0E /* FAT12 */ :
		filesystem_type == 2 ? 0x0E /* FAT16 */ :
		filesystem_type == 3 ? 0x0C /* FAT32 */ :
		filesystem_type == 4 ? 0x07 /* NTFS */  :
		/*filesystem_type == 5 ?*/ 0x83 /* EXT2 */;
  
  /* if TO_DRIVE is whole floppy, skip the geometry lookup. */
  if (start_sector == 0 && sector_count == 0 && to < 0x04)
  {
	tmp_geom.flags &= ~BIOSDISK_FLAG_LBA_EXTENSION;
	tmp_geom.sector_size = 512;
	tmp_geom.cylinders = 80;	/* does not care */
	tmp_geom.heads = 2;		/* does not care */
	tmp_geom.sectors = 18;		/* does not care */
  }
  else
  /* Get the geometry. This ensures that the drive is present.  */
  if (to != PXE_DRIVE && get_diskinfo (to, &tmp_geom))
  {
	return ! (errnum = ERR_NO_DISK);
  }

  i = j;	/* restore i from j */
  
  bios_drive_map[i].from_drive = from;
  bios_drive_map[i].to_drive = (unsigned char)to; /* to_drive = 0xFF if to == 0xffff */

  /* if CHS disabled, let MAX_HEAD != 0 to ensure a non-empty slot */
  bios_drive_map[i].max_head = disable_chs_mode | (heads_per_cylinder - 1);
  bios_drive_map[i].max_sector = (disable_chs_mode ? 0 : in_situ ? 1 : sectors_per_track) | ((read_Only | fake_write) << 7) | (disable_lba_mode << 6);
  if (from >= 0xA0 && tmp_geom.sector_size != 2048) /* FROM is cdrom and TO is not cdrom. */
	bios_drive_map[i].max_sector |= 0x0F; /* can be any value > 1, indicating an emulation. */

  /* bit 12 is for "TO drive is bifurcate" */

  if (! in_situ)
	bios_drive_map[i].to_cylinder =
		((tmp_geom.flags & BIOSDISK_FLAG_BIFURCATE) ? ((to & 0x100) >> 1) :
		((tmp_geom.flags & BIOSDISK_FLAG_LBA_EXTENSION) << 15)) |
		((tmp_geom.sector_size == 2048) << 14) |
		((from >= 0xA0) << 13) |	/* assume cdrom if from_drive is 0xA0 or greater */
		((!!(tmp_geom.flags & BIOSDISK_FLAG_BIFURCATE)) << 12) |
		((filesystem_type > 0) << 11) |	/* has a known boot sector type */
		((tmp_geom.cylinders - 1 > 0x3FF) ? 0x3FF : (tmp_geom.cylinders - 1));
  
  bios_drive_map[i].to_head = tmp_geom.heads - 1;

  bios_drive_map[i].to_sector = tmp_geom.sectors | ((fake_write | ! unsafe_boot) << 6);
  if (in_situ)
	bios_drive_map[i].to_sector |= 0x80;
  
  bios_drive_map[i].start_sector = start_sector;
  //bios_drive_map[i].start_sector_hi = 0;	/* currently only low 32 bits are used. */
  initrd_start_sector = start_sector;

  bios_drive_map[i].sector_count = sector_count;//(sector_count & 0xfffffffe) | fake_write | ! unsafe_boot;
  //bios_drive_map[i].sector_count_hi = 0;	/* currently only low 32 bits are used. */

#ifndef GRUB_UTIL
  /* increase the floppies or harddrives if needed */
  if (from & 0x80)
  {
	if (*((char *)0x475) == (char)(from - 0x80))
	{
		*((char *)0x475) = (char)(from - 0x80 + 1);
		if (debug > 0 && ! disable_map_info)
			print_bios_total_drives();
	}
  }else{
	if ((((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0) == ((char)from))
	{
		*((char *)0x410) &= 0x3e;
		*((char *)0x410) |= (((char)from) ? 0x41 : 1);
		if (debug > 0 && ! disable_map_info)
			print_bios_total_drives();
	}
  }
#endif	/* GRUB_UTIL */
  return 1;
  
delete_drive_map_slot:
  
//  if (bios_drive_map[i].to_drive == 0xFF && !(bios_drive_map[i].to_cylinder & 0x4000))
//  for (j = DRIVE_MAP_SIZE - 1; j > i ; j--)
//    {
//      if (bios_drive_map[j].to_drive == 0xFF && !(bios_drive_map[j].to_cylinder & 0x4000))
//	{
//	  if (mem == -1)
//		return ! (errnum = ERR_DEL_MEM_DRIVE);
//	  else /* force delete all subsequent MEM map slots */
//		grub_memmove ((char *) &bios_drive_map[j], (char *) &bios_drive_map[j + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - j));
//	}
//    }

  grub_memmove ((char *) &bios_drive_map[i], (char *) &bios_drive_map[i + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - i));
  if (mem != -1ULL)
	  grub_close ();

#undef	BS

#ifndef GRUB_UTIL
  /* Search for the Max floppy drive number in the drive map table.  */
  from = 0;
  for (i = 0; i < DRIVE_MAP_SIZE; i++)
    {
      if (drive_map_slot_empty (bios_drive_map[i]))
	break;
      if (! (bios_drive_map[i].from_drive & 0xE0) && from < bios_drive_map[i].from_drive + 1)
	from = bios_drive_map[i].from_drive + 1;
    }

  /* max possible floppies that can be handled in BIOS Data Area is 4 */
  if (from > 4)
	from = 4;
  if (from <= ((floppies_orig & 1) ? (floppies_orig >> 6) + 1 : 0))
	if ((((*(char*)0x410) & 1) ? ((*(char*)0x410) >> 6) + 1 : 0) > ((floppies_orig & 1) ? (floppies_orig >> 6) + 1 : 0))
	{
		/* decrease the floppies */
		(*(char*)0x410) = floppies_orig;
		if (debug > 0)
			print_bios_total_drives();
	}

  /* Search for the Max hard drive number in the drive map table.  */
  from = 0;
  for (i = 0; i < DRIVE_MAP_SIZE; i++)
    {
      if (drive_map_slot_empty (bios_drive_map[i]))
	break;
      if ((bios_drive_map[i].from_drive & 0xE0) == 0x80 && from < bios_drive_map[i].from_drive - 0x80 + 1)
	from = bios_drive_map[i].from_drive - 0x80 + 1;
    }

  if (from <= harddrives_orig)
	if ((*(char*)0x475) > harddrives_orig)
	{
		/* decrease the harddrives */
		(*(char*)0x475) = harddrives_orig;
		if (debug > 0)
			print_bios_total_drives();
	}

#endif	/* ! GRUB_UTIL */
  return 1;
}

static struct builtin builtin_map =
{
  "map",
  map_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "map [--status] [--mem[=RESERV]] [--hook] [--unhook] [--unmap=DRIVES]\n [--rehook] [--floppies=M] [--harddrives=N] [--memdisk-raw=RAW]\n [--a20-keep-on=AKO] [--safe-mbr-hook=SMH] [--int13-scheme=SCH]\n [--ram-drive=RD] [--rd-base=ADDR] [--rd-size=SIZE] [[--read-only]\n [--fake-write] [--unsafe-boot] [--disable-chs-mode] [--disable-lba-mode]\n [--heads=H] [--sectors-per-track=S] TO_DRIVE FROM_DRIVE]",
  "Map the drive FROM_DRIVE to the drive TO_DRIVE. This is necessary"
  " when you chain-load some operating systems, such as DOS, if such an"
  " OS resides at a non-first drive. TO_DRIVE can be a disk file, this"
  " indicates a disk emulation."
  " If --read-only is given, the"
  " emulated drive will be write-protected. If --fake-write is given,"
  " any write operations to the emulated drive are allowed but the data"
  " written will be discarded. The --unsafe-boot switch enables the write"
  " to the Master and DOS boot sectors of the emulated disk."
  " If --disable-chs-mode is given, CHS access to the emulated drive will"
  " be refused. If --disable-lba-mode is given, LBA access to the emulated"
  " drive will be refused."
  " if RAW=1,"
  " all memdrives will be accessed without using int15/ah=87h. if RAW=0, then"
  " int15/ah=87h will be used to access memdrives."
  " If one of --status, --hook, --unhook, --rehook, --floppies, --harddrives,"
  " --memdisk-raw, --a20-keep-on, --safe-mbr-hook, --int13-scheme,"
  " --ram-drive, --rd-base or --rd-size is"
  " given, then any other command-line arguments will be ignored."
  " The --mem option indicates a drive in"
  " memory. if RESERV is used and <= 0, the minimum memory occupied by the"
  " memdrive is (-RESERV) in 512-byte-sectors. if RESERV is used and > 0,"
  " the memdrive will occupy the mem area starting at absolute physical"
  " address RESERV in 512-byte-sectors and ending at the end of this mem"
  " block(usually the end of physical mem)."
};
#endif	/* ! GRUB_UTIL */


#ifdef USE_MD5_PASSWORDS
/* md5crypt */
static int
md5crypt_func (char *arg, int flags)
{
  char crypted[36];
  char key[32];
  unsigned int seed;
  int i;
  const char *const seedchars =
    "./0123456789ABCDEFGHIJKLMNOPQRST"
    "UVWXYZabcdefghijklmnopqrstuvwxyz";
  
  /* First create a salt.  */

  /* The magical prefix.  */
  grub_memset (crypted, 0, sizeof (crypted));
  grub_memmove (crypted, "$1$", 3);

  /* Create the length of a salt.  */
#ifdef GRUB_UTIL
  seed = currticks ();
#else
  seed = *(unsigned int *)0x46C;
#endif

  /* Generate a salt.  */
  for (i = 0; i < 8 && seed; i++)
    {
      /* FIXME: This should be more random.  */
      crypted[3 + i] = seedchars[seed & 0x3f];
      seed >>= 6;
    }

  /* A salt must be terminated with `$', if it is less than 8 chars.  */
  crypted[3 + i] = '$';

#ifdef DEBUG_MD5CRYPT
  grub_printf ("salt = %s\n", crypted);
#endif
  if (*arg)
	sprintf(key,"%.30s",arg);
  else
  {
	  /* Get a password.  */
	  grub_memset (key, 0, sizeof (key));
	  get_cmdline_str.prompt = "Password: ";
	  get_cmdline_str.maxlen = sizeof (key) - 1;
	  get_cmdline_str.echo_char = '*';
	  get_cmdline_str.readline = 0;
	  get_cmdline_str.cmdline = key;
	  get_cmdline (get_cmdline_str);
  }
  /* Crypt the key.  */
  make_md5_password (key, crypted);

  grub_printf ("Encrypted: %s\n", crypted);
  return 1;
}

static struct builtin builtin_md5crypt =
{
  "md5crypt",
  md5crypt_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "md5crypt",
  "Generate a password in MD5 format."
};
#endif /* USE_MD5_PASSWORDS */


/* module */
static int
module_func (char *arg, int flags)
{
  int len = grub_strlen (arg);

  switch (kernel_type)
    {
    case KERNEL_TYPE_MULTIBOOT:
      if (mb_cmdline + len + 1 > (char *) MB_CMDLINE_BUF + MB_CMDLINE_BUFLEN)
	{
	  errnum = ERR_WONT_FIT;
	  return 0;
	}
      grub_memmove (mb_cmdline, arg, len + 1);
      if (! load_module (arg, mb_cmdline))
	return 0;
      mb_cmdline += len + 1;
      break;

    case KERNEL_TYPE_LINUX:
    case KERNEL_TYPE_BIG_LINUX:
      if (! load_initrd (arg))
	return 0;
      break;

    default:
      errnum = ERR_NEED_MB_KERNEL;
      return 0;
    }

  return 1;
}

static struct builtin builtin_module =
{
  "module",
  module_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "module FILE [ARG ...]",
  "Load a boot module FILE for a Multiboot format boot image (no"
  " interpretation of the file contents is made, so users of this"
  " command must know what the kernel in question expects). The"
  " rest of the line is passed as the \"module command line\", like"
  " the `kernel' command."
};


/* modulenounzip */
static int
modulenounzip_func (char *arg, int flags)
{
  int ret;
#ifndef NO_DECOMPRESSION
  int no_decompression_bak = no_decompression;
#endif

#ifndef NO_DECOMPRESSION
  no_decompression = 1;
#endif

  ret = module_func (arg, flags);

#ifndef NO_DECOMPRESSION
  no_decompression = no_decompression_bak;
#endif

  return ret;
}

static struct builtin builtin_modulenounzip =
{
  "modulenounzip",
  modulenounzip_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "modulenounzip FILE [ARG ...]",
  "The same as `module', except that automatic decompression is"
  " disabled."
};

#ifdef SUPPORT_GRAPHICS

/* outline [on | off | status] */
static int
outline_func (char *arg, int flags)
{

  /* If ARG is empty, toggle the flag.  */
  if (! *arg)
    outline = ! outline;
  else if (grub_memcmp (arg, "on", 2) == 0)
    outline = 1;
  else if (grub_memcmp (arg, "off", 3) == 0)
    outline = 0;
  else if (grub_memcmp (arg, "status", 6) == 0)
  {
    if (debug > 0)
      grub_printf (" Character outline is now %s\n", (outline ? "on" : "off"));
  }
  else
      errnum = ERR_BAD_ARGUMENT;

  return outline;
}

static struct builtin builtin_outline =
{
  "outline",
  outline_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "outline [on | off | status]",
  "Turn on/off or display the outline mode, or toggle it if no argument."
};
#endif /* SUPPORT_GRAPHICS */


/* pager [on|off] */
static int
pager_func (char *arg, int flags)
{

  /* If ARG is empty, toggle the flag.  */
  if (! *arg)
    use_pager = ! use_pager;
  else if (grub_memcmp (arg, "on", 2) == 0)
    use_pager = 1;
  else if (grub_memcmp (arg, "off", 3) == 0)
    use_pager = 0;
  else if (grub_memcmp (arg, "status", 6) == 0)
  {
    if (debug > 0)
      grub_printf (" Internal pager is now %s\n", (use_pager ? "on" : "off"));
  }
  else
      errnum = ERR_BAD_ARGUMENT;
  if (use_pager == 0)
    count_lines = -1;
  else if (count_lines == -1)
    count_lines = 0;
  return use_pager;
}

static struct builtin builtin_pager =
{
  "pager",
  pager_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "pager [on | off | status]",
  "Turn on/off or display the pager mode, or toggle it if no argument."
};


  /* Convert a LBA address to a CHS address in the INT 13 format.  */
void
lba_to_chs (unsigned long lba, unsigned long *cl, unsigned long *ch, unsigned long *dh)
{
      unsigned long cylinder, head, sector;

      sector = lba % buf_geom.sectors + 1;
      head = (lba / buf_geom.sectors) % buf_geom.heads;
      cylinder = lba / (buf_geom.sectors * buf_geom.heads);

      if (cylinder > 0x3FF)
	cylinder = 0x3FF;
      
      if (cylinder >= buf_geom.cylinders)
	cylinder = buf_geom.cylinders - 1;
      
      *cl = sector | ((cylinder & 0x300) >> 2);
      *ch = cylinder & 0xFF;
      *dh = head;
}
      
/* partnew PART TYPE START LEN */
static int
partnew_func (char *arg, int flags)
{
  unsigned long long new_type, new_start, new_len;
  unsigned long start_cl, start_ch, start_dh;
  unsigned long end_cl, end_ch, end_dh;
  unsigned long current_drive_bak;
  unsigned long current_partition_bak;
  char *filename;
  unsigned long entry1, i;
  unsigned long active = -1;
#if defined(STAGE1_5) || defined(GRUB_UTIL)	    
//  char mbr[512];
#endif

  if (grub_memcmp (arg, "--active", 8) == 0)
    {
      active = 0x80;
      arg = skip_to (0, arg);
    }

  /* Get the drive and the partition.  */
  if (! set_device (arg))
    return 0;

  /* The drive must be a hard disk.  */
  if (! (current_drive & 0x80))
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  entry1 = current_partition >> 16;
  
  /* The partition must a primary partition.  */
  if (entry1 > 3 || (current_partition & 0xFFFF) != 0xFFFF)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  /* Get the new partition type.  */
  arg = skip_to (0, arg);
  if (! safe_parse_maxint (&arg, &new_type))
    return 0;

  /* The partition type is unsigned char.  */
  if (new_type > 0xFF)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  /* Get the new partition start and length.  */
  arg = skip_to (0, arg);
  filename = arg;

  current_drive_bak = 0;
  if ((! safe_parse_maxint (&arg, &new_start))
      || ((arg = skip_to (0, arg)), (! safe_parse_maxint (&arg, &new_len))))
  {
      current_drive_bak = current_drive;
      current_partition_bak = current_partition;
      arg = filename;
      filename = set_device (filename);

      if (errnum)
      {
	/* No device specified. Default to the root device. */
	current_drive = saved_drive;
	current_partition = saved_partition;
	filename = 0;
	errnum = 0;
      }
  
      if (current_drive != current_drive_bak)
      {
	if (debug > 0)
	    grub_printf ("Cannot create a partition in drive %X from a file in drive %X.\n", current_drive_bak, current_drive);
	errnum = ERR_BAD_ARGUMENT;
	return 0;
      }

      if (current_partition == 0xFFFFFF)
      {
	if (debug > 0)
	    grub_printf ("Cannot create a partition with a blocklist of a whole drive.\n");
	errnum = ERR_BAD_ARGUMENT;
	return 0;
      }

      query_block_entries = -1; /* query block list only */
      blocklist_func (arg, flags);
      if (errnum == 0)
      {
	if (query_block_entries != 1)
		return ! (errnum = ERR_NON_CONTIGUOUS);
	new_start = map_start_sector;
	new_len = (filemax + 0x1ff) >> SECTOR_BITS;
      }
      else
	return ! errnum;

      if (new_start == part_start && part_start && new_len == 1)
	new_len = part_length;

      if (new_start < part_start || new_start + new_len > (unsigned long)(part_start + part_length))
      {
	if (debug > 0)
	    grub_printf ("Cannot create a partition that exceeds the partition boundary.\n");
	return ! (errnum = ERR_BAD_ARGUMENT);
      }
    
      /* Read the first sector.  */
      if (! rawread (current_drive, new_start, 0, SECTOR_SIZE, (unsigned long long)(unsigned int)mbr, 0xedde0d90))
        return 0;

#define	BS	((struct master_and_dos_boot_sector *)mbr)
      /* try to find out the filesystem type */
      if (BS->boot_signature == 0xAA55 && ! probe_bpb(BS))
      {
	if ((new_type & 0xFFFFFFEF) == 0)	/* auto filesystem type */
	{
		new_type |= 
			filesystem_type == 1 ? 0x0E /* FAT12 */ :
			filesystem_type == 2 ? 0x0E /* FAT16 */ :
			filesystem_type == 3 ? 0x0C /* FAT32 */ :
			filesystem_type == 4 ? 0x07 /* NTFS */  :
			/*filesystem_type == 5 ?*/ 0x83 /* EXT2 */;
		if (filesystem_type == 5)
			new_type = 0x83; /* EXT2 */
	}
	if (debug > 0)
	    grub_printf ("%s BPB found %s the leading 0xEB (jmp). Hidden sectors=0x%X\n",
		(filesystem_type == 1 ? "FAT12" :
		filesystem_type == 2 ? "FAT16" :
		filesystem_type == 3 ? "FAT32" :
		filesystem_type == 4 ? "NTFS"  :
		/*filesystem_type == 5 ?*/ "EXT2 GRLDR"),
		(BS->dummy1[0] == (char)0xEB ? "with" : "but WITHOUT"),
		BS->hidden_sectors);
	if (BS->hidden_sectors != new_start)
	{
	    if (debug > 0)
		grub_printf ("Changing hidden sectors 0x%X to 0x%lX... ", BS->hidden_sectors, (unsigned long long)new_start);
	    BS->hidden_sectors = new_start;
	    /* Write back/update the boot sector.  */
	    if (! rawwrite (current_drive, new_start, mbr))
	    {
		if (debug > 0)
			grub_printf ("failure.\n");
	        return 0;
	    } else {
		if (debug > 0)
			grub_printf ("success.\n");
	    }
	}
      }
#undef BS

      current_drive = current_drive_bak;
      current_partition = current_partition_bak;
  }
#if 0
  else
  {
      /* this command is intended for running in command line and inhibited from running in menu.lst */
      if (flags & (BUILTIN_MENU | BUILTIN_SCRIPT))
      {
	if (debug > 0)
	    grub_printf ("This form of partnew is inhibited from running in a script.\n");
	return ! (errnum = ERR_BAD_ARGUMENT);
      }
  }
#endif

  /* Read the MBR.  */
  if (! rawread (current_drive, 0, 0, SECTOR_SIZE, (unsigned long long)(unsigned int)mbr, 0xedde0d90))
    return 0;

  if (current_drive_bak)	/* creating a partition from a file */
  {
	/* if the entry is not empty, it should be a part of another
	 * partition, that is, it should be covered by another partition. */
    if (PC_SLICE_START (mbr, entry1) != 0 || PC_SLICE_LENGTH (mbr, entry1) != 0)
    {
	for (i = 0; i < 4; i++)
	{
		if (i == entry1)
			continue;
		if (PC_SLICE_START (mbr, entry1) < PC_SLICE_START (mbr, i))
			continue;
		if (PC_SLICE_START (mbr, entry1) + PC_SLICE_LENGTH (mbr, entry1)
		    > PC_SLICE_START (mbr, i) + PC_SLICE_LENGTH (mbr, i))
			continue;
		break;	/* found */
	}
	if (i >= 4)
	{
		/* not found */
		if (debug > 0)
		    grub_printf ("Cannot overwrite an independent partition.\n");
		return ! (errnum = ERR_BAD_ARGUMENT);
	}
    }
  }

#if 0
  /* Do not check this. The total_sectors might be too small
   * for some buggy BIOSes. */
  /* Check if the new partition will fit in the disk.  */
  if (new_start + new_len > buf_geom.total_sectors)
    {
      errnum = ERR_GEOM;
      return 0;
    }
#endif

  if (new_type == 0 && new_start == 0 && new_len == 0)
  {
    /* empty the entry */
    start_dh = start_cl = start_ch = end_dh = end_cl = end_ch = 0;
  }else{
    if (new_start == 0 || new_len == 0)
    {
      errnum = ERR_BAD_PART_TABLE;
      return 0;
    }
    /* Store the partition information in the MBR.  */
    lba_to_chs (new_start, &start_cl, &start_ch, &start_dh);
    lba_to_chs (new_start + new_len - 1, &end_cl, &end_ch, &end_dh);
  }

  if (active == 0x80)
  {
    /* Activate this partition */
    PC_SLICE_FLAG (mbr, entry1) = 0x80;
  } else {
    if (PC_SLICE_FLAG (mbr, entry1) != 0x80)
        PC_SLICE_FLAG (mbr, entry1) = 0;
  }

  /* Deactivate other partitions */
  if (PC_SLICE_FLAG (mbr, entry1) == 0x80)
  {
	for (i = 0; i < 4; i++)
	{
		if (i == entry1)
			continue;
		if (PC_SLICE_FLAG (mbr, i) != 0)
		{
		    if (debug > 0)
		    {
		      if (PC_SLICE_FLAG (mbr, i) == 0x80)
		      {
			grub_printf ("The active flag(0x80) of partition %d was changed to 0.\n", (unsigned long)i);
		      } else {
			grub_printf ("The invalid active flag(0x%X) of partition %d was changed to 0.\n", (unsigned long)(PC_SLICE_FLAG (mbr, i)), (unsigned long)i);
		      }
		    }

		    PC_SLICE_FLAG (mbr, i) = 0;
		}
	}
  }
  PC_SLICE_HEAD (mbr, entry1) = start_dh;
  PC_SLICE_SEC (mbr, entry1) = start_cl;
  PC_SLICE_CYL (mbr, entry1) = start_ch;
  PC_SLICE_TYPE (mbr, entry1) = new_type;
  PC_SLICE_EHEAD (mbr, entry1) = end_dh;
  PC_SLICE_ESEC (mbr, entry1) = end_cl;
  PC_SLICE_ECYL (mbr, entry1) = end_ch;
  PC_SLICE_START (mbr, entry1) = new_start;
  PC_SLICE_LENGTH (mbr, entry1) = new_len;

  /* Make sure that the MBR has a valid signature.  */
  PC_MBR_SIG (mbr) = PC_MBR_SIGNATURE;
  
  /* Write back the MBR to the disk.  */
  buf_track = -1;
  if (! rawwrite (current_drive, 0, mbr))
    return 0;

  return 1;
}

static struct builtin builtin_partnew =
{
  "partnew",
  partnew_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "partnew [--active] PART TYPE START [LEN]",
  "Create a primary partition at the starting address START with the"
  " length LEN, with the type TYPE. START and LEN are in sector units."
  " If --active is used, the new partition will be active. START can be"
  " a contiguous file that will be used as the content/data of the new"
  " partition, in which case the LEN parameter is ignored, and TYPE can"
  " be either 0x00 for auto or 0x10 for hidden-auto."
};


/* parttype PART TYPE */
static int
parttype_func (char *arg, int flags)
{
  unsigned long long new_type = -1;
  unsigned long part = 0xFFFFFF;
  unsigned long start, len, offset, ext_offset1;
  unsigned long type, entry1;
#if defined(STAGE1_5) || defined(GRUB_UTIL)	    
//  char mbr[512];
#endif

  /* Get the drive and the partition.  */

  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	current_drive = saved_drive;
	current_partition = saved_partition;
  } else if (! set_device (arg))
  {
    if (! safe_parse_maxint (&arg, &new_type))
      return 0;
    current_drive = saved_drive;
    current_partition = saved_partition;
  } else {
    /* Get the new partition type.  */
    arg = skip_to (0, arg);
    if (*arg && *arg != ' ' && *arg != '\t')
      if (! safe_parse_maxint (&arg, &new_type))
        return 0;
  }

  /* The drive must be a hard disk.  */
  if (! (current_drive & 0x80))
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  
  /* The partition must be a PC slice.  */
  if ((current_partition >> 16) == 0xFF
      || (current_partition & 0xFFFF) != 0xFFFF)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  /* The partition type is unsigned char.  */
  if (new_type != -1 && new_type > 0xFF)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  /* Look for the partition.  */
  while ((	next_partition_drive		= current_drive,
		next_partition_dest		= 0xFFFFFF,
		next_partition_partition	= &part,
		next_partition_type		= &type,
		next_partition_start		= &start,
		next_partition_len		= &len,
		next_partition_offset		= &offset,
		next_partition_entry		= &entry1,
		next_partition_ext_offset	= &ext_offset1,
		next_partition_buf		= mbr,
		next_partition ()))
    {
      if (part == current_partition)
	{
	  /* Found.  */

	  errnum = 0;
	  if (new_type == -1)	/* return the current type */
	  {
		new_type = PC_SLICE_TYPE (mbr, entry1);
		if (debug > 0)
			printf ("Partition type for (hd%d,%d) is 0x%X.\n", (current_drive & 0x7F), (unsigned long)(unsigned char)(current_partition >> 16), (unsigned long)new_type);
		return new_type;
	  }

	  /* Set the type to NEW_TYPE.  */
	  PC_SLICE_TYPE (mbr, entry1) = new_type;
	  
	  /* Write back the MBR to the disk.  */
	  buf_track = -1;
	  if (! rawwrite (current_drive, offset, mbr))
	    break;	/* failure */

	  if (debug > 0)
		printf ("Partition type for (hd%d,%d) set to 0x%X successfully.\n", (current_drive & 0x7F), (unsigned long)(unsigned char)(current_partition >> 16), (unsigned long)new_type);
	  /* Succeed.  */
	  errnum = 0;
	  return 1;
	}
    }

  /* The partition was not found.  ERRNUM was set by next_partition.  */
  return 0;
}

static struct builtin builtin_parttype =
{
  "parttype",
  parttype_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "parttype [PART] [TYPE]",
  "Change the type of the partition PART to TYPE. If TYPE is omitted, return "
  "the partition type of the specified device(instead of changing it). PART "
  "default to the current root device."
};


/* password */
static int
password_func (char *arg, int flags)
{
  int len;
  password_t type = PASSWORD_PLAIN;

#ifdef USE_MD5_PASSWORDS
  if (grub_memcmp (arg, "--md5", 5) == 0)
    {
      type = PASSWORD_MD5;
      arg = skip_to (0, arg);
    }
#endif
  if (grub_memcmp (arg, "--", 2) == 0)
    {
      type = PASSWORD_UNSUPPORTED;
      arg = skip_to (0, arg);
    }

  if ((flags & (BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_BAT_SCRIPT)) != 0)
    {
    	#if 0
      /* Do password check! */
      char entered[32];
      
      /* Wipe out any previously entered password */
      entered[0] = 0;
      get_cmdline_str.prompt = "Password: ";
      get_cmdline_str.maxlen = sizeof (entered) - 1;
      get_cmdline_str.echo_char = '*';
      get_cmdline_str.readline = 0;
      get_cmdline_str.cmdline = entered;
      get_cmdline (get_cmdline_str);
	#endif
      nul_terminate (arg);
      if ((len = check_password (arg, type)) != 0)
	{
	  errnum = (len == 0xFFFF ? ERR_MD5_FORMAT : ERR_PRIVILEGED);
	  return 0;
	}
    }
  else
    {
      len = grub_strlen (arg);
      
      /* PASSWORD NUL NUL ... */
      if (len + 2 > PASSWORD_BUFLEN)
	{
	  errnum = ERR_WONT_FIT;
	  return 0;
	}
      
      /* Copy the password and clear the rest of the buffer.  */
      password = (char *) PASSWORD_BUF;
      grub_memmove (password, arg, len);
      grub_memset (password + len, 0, PASSWORD_BUFLEN - len);
      password_type = type;
    }
  return 1;
}

static struct builtin builtin_password =
{
  "password",
  password_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_NO_ECHO,
  "password [--md5] PASSWD [FILE]",
  "If used in the first section of a menu file, disable all"
  " interactive editing control (menu entry editor and"
  " command line). If the password PASSWD is entered, it loads the"
  " FILE as a new config file and restarts the GRUB Stage 2. If you"
  " omit the argument FILE, then GRUB just unlocks privileged"
  " instructions.  You can also use it in the script section, in"
  " which case it will ask for the password, before continueing."
  " The option --md5 tells GRUB that PASSWD is encrypted with"
  " md5crypt."
};


/* pause */
static int
pause_func (char *arg, int flags)
{
//  char *p;
  unsigned long long wait = -1;
  int time1;
  int time2 = -1;
  int testkey = 0;

	for (;;)
	{
		if (grub_memcmp (arg, "--test-key", 10) == 0)
		{
			testkey = 1;
		}
		else if (grub_memcmp (arg, "--wait=", 7) == 0)
		{
			arg += 7;
			if (! safe_parse_maxint (&arg, &wait))
				return 0;
		}
		else
			break;
		arg = skip_to (0, arg);
	}
  
  if (*arg)
    printf("%s\n", arg);

  /* Get current time.  */
  int ret = 1;
  while ((time2 = getrtsecs ()) == 0xFF);
   while (wait != 0)
   {
      /* Check if there is a key-press.  */
      if (checkkey () != -1)
      {
      	ret = getkey ();
      	if (testkey)
      	{
      		if (debug > 0)
      			printf("%04x",ret);
      		return ret;
      	}
         ret &= 0xFF;
         /* Check the special ESC key  */
         if (ret == '\e')
            return 0;	/* abort this entry */
         break;
      }

      if (wait != -1 && (time1 = getrtsecs ()) != time2 && time1 != 0xFF)
      {
         if (debug >= 0)
         	printf("\t%d\t\r",wait);
         time2 = time1;
         wait--;
      }
   }
   return ret;
}

static struct builtin builtin_pause =
{
  "pause",
  pause_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_NO_ECHO,
  "pause [--wait=T] [MESSAGE ...]",
  "Print MESSAGE, then wait until a key is pressed or T seconds has passed."
};


#ifdef FSYS_PXE
/* pxe */
static struct builtin builtin_pxe =
{
  "pxe",
  pxe_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "pxe [cmd] [parameters]",
  "Call PXE command."
};
#endif


#ifdef GRUB_UTIL
/* quit */
static int
quit_func (char *arg, int flags)
{
  stop ();
  
  /* Never reach here.  */
  return 1;
}

static struct builtin builtin_quit =
{
  "quit",
  quit_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "quit",
  "Exit from the GRUB shell."
};
#else
/* quit */
static int
quit_func (char *arg, int flags)
{
  unsigned long i;
  register long Sum;
  
  /* check if we were launched from DOS.  */
  if (*(long *)0x2A0000 != 0x50554B42)	/* "BKUP" */
  {
	return ! (errnum = ERR_DOS_BACKUP);
  }
  
  for (i = 0, Sum = 0; i < (0xA0000 + 16) / 4; i++)
  {
	Sum += *(long *)(0x200000 + (i << 2));
  }
  
  if (Sum)
	return ! (errnum = ERR_DOS_BACKUP);
  
  chainloader_disable_A20 = 0;
  //grub_memmove((char *)0x110000, (char *)0x200000, 0xA0000);

  for (;;)
  {
    if (grub_memcmp (arg, "--disable-a20", 13) == 0)
      {
	chainloader_disable_A20 = 1;
      }
    else
	break;
    arg = skip_to (0, arg);
  }
  chain_load_segment = 0;
  chain_load_offset = 0;
  chain_boot_CS = *(unsigned short*)(0x2A0000 + 10);
  chain_boot_IP = *(unsigned short*)(0x2A0000 + 8);
  chain_load_length = 0xA0000;
  chain_ebx = 0;
  chain_ebx_set = 0;
  chain_edx = 0;
  chain_edx_set = 0;
  chain_bx = 0;
  chain_bx_set = 0;
  chain_cx = 0;
  chain_cx_set = 0;
  chain_enable_gateA20 = ((*(short *)(0x2A0000 + 4)) != 0);
  if (chainloader_disable_A20)
	chain_enable_gateA20 = 0;
		
  /* move the code to a safe place at 0x2B0000 */
  grub_memmove((char *)HMA_ADDR, HMA_start, 0x200/*0xfff0*/);

  /* Turn off A20 here if the DOS image told us to do so. Note: we won't have
   * access to the odd megas of the memory when A20 is off.
   */
  if (!chain_enable_gateA20)
  {
	if (gateA20 (0))
	{
		if (debug > 0)
		  grub_printf("\nGate A20 is turned off successfully.\n");
	}
	else {
		/* to asure A20 is on when we return to grub. */
		gateA20 (1);	/* turn on A20 in case it is off */
		if (chainloader_disable_A20)
			return ! (errnum = ERR_DISABLE_A20);
		else
		{
			if (debug > 0)
			  grub_printf("\nFailed to turn off Gate A20!\n");
			chain_enable_gateA20 = 1;
		}
	}
  }

		/* Jump to high memory area. This will move boot code at
		 * 0x110000 to the destination load-segment:load-offset;
		 * setup edx and ebx registers; switch to real mode;
		 * and jump to boot-cs:boot-ip.
		 */
  ((void (*)(void))HMA_ADDR)();	/* no return */
  
  return 0;
}

static struct builtin builtin_quit =
{
  "quit",
  quit_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "quit [--disable-a20]",
  "Go back to DOS if GRUB was previously launched from DOS."
};
#endif /* GRUB_UTIL */


#ifdef SUPPORT_NETBOOT
#ifndef GRUB_UTIL
/* rarp */
static int
rarp_func (char *arg, int flags)
{
  if (! rarp ())
    {
      if (errnum == ERR_NONE)
	errnum = ERR_DEV_VALUES;

      return 0;
    }

  /* Notify the configuration.  */
  print_network_configuration ();
  return 1;
}

static struct builtin builtin_rarp =
{
  "rarp",
  rarp_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "rarp",
  "Initialize a network device via RARP."
};
#endif /* ! GRUB_UTIL */
#endif /* SUPPORT_NETBOOT */


#ifndef GRUB_UTIL
static int
read_func (char *arg, int flags)
{
  unsigned long long addr, val;

  if (! safe_parse_maxint (&arg, &addr))
    return 0;

  val = *(unsigned long *)(unsigned long)(RAW_ADDR (addr));
  if (debug > 0)
    grub_printf ("Address 0x%lx: Value 0x%lx\n", addr, val);
  return val;
}

static struct builtin builtin_read =
{
  "read",
  read_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "read ADDR",
  "Read a 32-bit value from memory at address ADDR and"
  " display it in hex format."
};
#endif /* ! GRUB_UTIL */


int
parse_string (char *arg)
{
  int len;
  char *p;
  char ch;
  //int quote;

  //nul_terminate (arg);
	#if 1
	for (len = 0,p = arg;(ch = *p);p++)
	{
		if (ch == '\\' && (ch = *++p))
		{
			switch(ch)
			{
				case 't':
					*arg++ = '\t';
					break;
				case 'r':
					*arg++ = '\r';
					break;
				case 'n':
					*arg++ = '\n';
					break;
				case 'a':
					*arg++ = '\a';
					break;
				case 'b':
					*arg++ = '\b';
					break;
				case 'f':
					*arg++ = '\f';
					break;
				case 'v':
					*arg++ = '\v';
					break;
				case 'x':
				{
					/* hex */
					int val;

					p++;
					ch = *p;
					if (ch >= '0' && ch <= '9')
						val = ch - '0';
					else if (ch >= 'A' && ch <= 'F')
						val = ch - 'A' + 10;
					else if (ch >= 'a' && ch <= 'f')
						val = ch - 'a' + 10;
					else
						return len;	/* error encountered */
					
					p++;
					ch = *p;
					
					if (ch >= '0' && ch <= '9')
						val = (val << 4) + ch - '0';
					else if (ch >= 'A' && ch <= 'F')
						val = (val << 4) + ch - 'A' + 10;
					else if (ch >= 'a' && ch <= 'f')
						val = (val << 4) + ch - 'a' + 10;
					else
						p--;
					
					*arg++ = val;
				}
					break;
				default:
					if (ch >= '0' && ch <= '7')
					{
						/* octal */
						int val = ch - '0';
						int i;
						
						for (i=0;i<2 && p[1] >= '0' && p[1] <= '7';i++)
						{
							p++;
							val <<= 3;
							val += *p - '0';
						}
						
						*arg++ = val;
						break;
					} else *arg++ = ch;
			}
		} else *arg++ = ch;
		
		len++;
	}
	#else
  for (quote = len = 0, p = arg; (ch = *p); p++)
  {
	if (ch == '\\')
	{
		if (quote)
		{
			*arg++ = ch;
			len++;
			quote = 0;
			continue;
		}
		quote = 1;
		continue;
	}
	if (quote)
	{
		if (ch == 't')
		{
			*arg++ = '\t';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'r')
		{
			*arg++ = '\r';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'n')
		{
			*arg++ = '\n';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'a')
		{
			*arg++ = '\a';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'b')
		{
			*arg++ = '\b';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'f')
		{
			*arg++ = '\f';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'v')
		{
			*arg++ = '\v';
			len++;
			quote = 0;
			continue;
		}
		if (ch >= '0' && ch <= '7')
		{
			/* octal */
			int val = ch - '0';

			if (p[1] >= '0' && p[1] <= '7')
			{
				val *= 8;
				p++;
				val += *p -'0';
				if (p[1] >= '0' && p[1] <= '7')
				{
					val *= 8;
					p++;
					val += *p -'0';
				}
			}
			*arg++ = val;
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'x')
		{
			/* hex */
			int val;

			p++;
			ch = *p;
			if (ch >= '0' && ch <= '9')
				val = ch - '0';
			else if (ch >= 'A' && ch <= 'F')
				val = ch - 'A' + 10;
			else if (ch >= 'a' && ch <= 'f')
				val = ch - 'a' + 10;
			else
				return len;	/* error encountered */
			p++;
			ch = *p;
			if (ch >= '0' && ch <= '9')
				val = val * 16 + ch - '0';
			else if (ch >= 'A' && ch <= 'F')
				val = val * 16 + ch - 'A' + 10;
			else if (ch >= 'a' && ch <= 'f')
				val = val * 16 + ch - 'A' + 10;
			else
				p--;
			*arg++ = val;
			len++;
			quote = 0;
			continue;
		}
		if (ch)
		{
			*arg++ = ch;
			len++;
			quote = 0;
			continue;
		}
		return len;
	}
	*arg++ = ch;
	len++;
	quote = 0;
  }
  #endif
//  if (*arg) *arg = 0;
  return len;
}

#ifndef GRUB_UTIL
static int
write_func (char *arg, int flags)
{
  unsigned long long addr;
  unsigned long long val;
  char *p;
  unsigned long tmp_drive;
  unsigned long tmp_partition;
  unsigned long long offset;
  unsigned long long len;
  char tmp_file[16];
  //int block_file = 0;

  tmp_drive = saved_drive;
  tmp_partition = saved_partition;
  offset = 0;
  for (;;)
  {
    if (grub_memcmp (arg, "--offset=", 9) == 0)
      {
	p = arg + 9;
	if (! safe_parse_maxint (&p, &offset))
		return 0;
      }
    else
	break;
    arg = skip_to (0, arg);
  }
  
  p = NULL;
  addr = -1;
  if (*arg == '/' || *arg == '(')
  {
	/* destination is device or file. */
  	if (*arg == '(')
	{
		p = set_device (arg);
		if (errnum)
			goto fail;
		if (! p)
		{
			if (errnum == 0)
				errnum = ERR_BAD_ARGUMENT;
			goto fail;
		}
		//if (*p != '/')
		//	block_file = 1;
		saved_drive = current_drive;
		saved_partition = current_partition;
		/* if only the device portion is specified */
		if ((unsigned char)*p <= ' ')
		{
			p = tmp_file;
			*p++ = '(';
			*p++ = ')';
			*p++ = '+';
			*p++ = '1';
			*p = 0;
			p = tmp_file;
			grub_open (p);
			if (errnum)
				goto fail;
			grub_sprintf (p + 3, "0x%lx", (unsigned long long)part_length);
			grub_close ();
		}
	}
	if (p != tmp_file)
	    p = arg;
	grub_open (p);
	current_drive = saved_drive;
	current_partition = saved_partition;
	if (errnum)
		goto fail;

#if 0
	if (current_drive != ram_drive && current_drive != 0xFFFF && block_file)
	{
		unsigned long j;

		/* check if it is a mapped memdrive */
		j = DRIVE_MAP_SIZE;		/* real drive */
#ifndef GRUB_UTIL
		if (! unset_int13_handler (1))	/* map is hooked */
		    for (j = 0; j < DRIVE_MAP_SIZE; j++)
		    {
			if (drive_map_slot_empty (hooked_drive_map[j]))
			{
				j = DRIVE_MAP_SIZE;	/* real drive */
				break;
			}

			if (current_drive == hooked_drive_map[j].from_drive && hooked_drive_map[j].to_drive == 0xFF && !(hooked_drive_map[j].to_cylinder & 0x4000))
				break;			/* memdrive */
		    }
#endif /* ! GRUB_UTIL */

		if (j == DRIVE_MAP_SIZE)	/* real drive */
		{
		    /* this command is intended for running in command line and inhibited from running in menu.lst */
		    if (flags & (BUILTIN_MENU | BUILTIN_SCRIPT))
		    {
			grub_close ();
			errnum = ERR_WRITE_TO_NON_MEM_DRIVE;
			goto fail;
		    }
		}
	}
#endif

	filepos = offset;
  }
  else
  {
	/* destination is memory address. */
	if (*arg < '0' || *arg > '9')
	{
		errnum = ERR_BAD_ARGUMENT;
		goto fail;
	}
	if (! safe_parse_maxint (&arg, &addr))
		goto fail;
	if (addr == -1)
	{
		errnum = ERR_BAD_ARGUMENT;
		goto fail;
	}
  }

  /* destination is device or file if addr == -1 */
  /* destination is memory address if addr != -1 */

  arg = skip_to (0, arg);	/* INTEGER_OR_STRING */

  if (addr == -1)
  {
	/* string */
	if (! *arg)
	{
		grub_close ();
		errnum = ERR_BAD_ARGUMENT;
		goto fail;
	}
	len = parse_string (arg);
	if (saved_drive == 0xFFFF && p == tmp_file)	/* (md) */
	{
		grub_close ();
		grub_memmove64 (offset, (unsigned long long)(unsigned int)arg, len);
		if ((unsigned long)&saved_drive + 3 >= offset && (unsigned long)&saved_drive < offset + len)
			tmp_drive = saved_drive;
		if ((unsigned long)&saved_partition + 3 >= offset && (unsigned long)&saved_partition < offset + len)
			tmp_partition = saved_partition;
		errnum = 0;
		goto succ;
	}
	/* write file */
	if (len > filemax - filepos)
	    len = filemax - filepos;
	if (grub_read ((unsigned long long)(unsigned int)arg, len, 0x900ddeed) != len)	/* write */
	{
		if (errnum == 0)
			errnum = ERR_WRITE;
	}
	{
		int err = errnum;
		grub_close ();
		errnum = err;
	}
succ:
	if (errnum == 0 && debug > 0)
		grub_printf ("0x%lX bytes written at offset 0x%lX.\n", (unsigned long long)len, (unsigned long long)offset);
  }
  else
  {
	/* integer */
	p = arg;
	if (! safe_parse_maxint (&p, &val))
		goto fail;
	*((unsigned *)(unsigned int) RAW_ADDR (addr += offset)) = (unsigned)val;
	if (debug > 0)
		grub_printf ("Address 0x%lx: Value 0x%x\n", (unsigned long long)addr, (*((unsigned *)(unsigned int) RAW_ADDR (addr))));
	if (addr != (int)&saved_drive)
		saved_drive = tmp_drive;
	if (addr != (int)&saved_partition)
		saved_partition = tmp_partition;
	errnum = 0;
	return val;
  }

fail:

  saved_drive = tmp_drive;
  saved_partition = tmp_partition;
  return !(errnum);
}

static struct builtin builtin_write =
{
  "write",
  write_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "write [--offset=SKIP] ADDR_OR_FILE INTEGER_OR_STRING",
  "Write a 32-bit value to memory or write a string to file(or device!)."
};


/* reboot */
static int
reboot_func (char *arg, int flags)
{
  grub_reboot ();

  /* Never reach here.  */
  return 0;
}

static struct builtin builtin_reboot =
{
  "reboot",
  reboot_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "reboot",
  "Reboot your system."
};
#endif /* ! GRUB_UTIL */


/* Print the root device information to buffer.
   flag 0	saved device.
   flag 1  current device.
*/
void
print_root_device (char *buffer,int flag)
{
	unsigned long long st_bak = putchar_st.status;
	unsigned long tmp_drive = flag?current_drive:saved_drive;
	unsigned long tmp_partition = flag?current_partition:saved_partition;
	if (buffer)
		putchar_st.status = ((unsigned long long)(int)buffer << 32) | 1;
	else
		putchar(' ');
	switch(tmp_drive)
	{
	#ifdef FSYS_FB
		case FB_DRIVE:
			grub_printf("(ud)");
			break;
	#endif /* FSYS_FB */
	#ifdef FSYS_PXE
		case PXE_DRIVE:
			grub_printf("(pd)");
			break;
	#endif /* PXE drive. */
	#ifdef SUPPORT_NETBOOT
		case NETWORK_DRIVE:
			grub_printf ("(nd)");
			break;
	#endif /* SUPPORT_NETBOOT */
		default:
			if (tmp_drive == cdrom_drive)
			{
				grub_printf("(cd)");
				break;
			}
			if (tmp_drive & 0x80)
			{
				/* Hard disk drive.  */
				grub_printf ("(hd%d", (tmp_drive - 0x80));
			}
			else
			{
				/* Floppy disk drive.  */
				grub_printf ("(fd%d", tmp_drive);
			}

			if ((tmp_partition & 0xFF0000) != 0xFF0000)
				grub_printf (",%d", (unsigned long)(unsigned char)(tmp_partition >> 16));

			if ((tmp_partition & 0x00FF00) != 0x00FF00)
				grub_printf (",%c", (unsigned long)(unsigned char)((tmp_partition >> 8) + 'a'));

			grub_printf (")");
			break;
	}
	if (buffer)
	{
		*putchar_st.addr = 0;
		putchar_st.status = st_bak;
	}
	return;
}

static int
real_root_func (char *arg, int attempt_mnt)
{
  char *next;
  unsigned long i, tmp_drive = 0;
  unsigned long tmp_partition = 0;
  char ch;

  /* Get the drive and the partition.  */
  if (! *arg || *arg == ' ' || *arg == '\t')
    {
	current_drive = saved_drive;
	current_partition = saved_partition;
	next = 0; /* If ARG is empty, just print the current root device.  */
    }
  else if (grub_memcmp (arg, "endpart", 7) == 0)
    {
	unsigned long part = 0xFFFFFF;
	unsigned long start, len, offset, ext_offset1;
	unsigned long type, entry1;
	
	/* find MAX/END partition of the current root drive */
	
	if (! (saved_drive & 0x80))
	{
		grub_printf ("Cannot use 'endpart' with the current root device (fd%d).\n", saved_drive);
		errnum = ERR_DEV_VALUES;
		return 0;
	}

	tmp_partition = saved_partition;
	tmp_drive = saved_drive;

	current_partition = saved_partition;
	current_drive = saved_drive;
	next = arg + 7;
	
	while ((next_partition_drive		= current_drive,
		next_partition_dest		= 0xFFFFFF,
		next_partition_partition	= &part,
		next_partition_type		= &type,
		next_partition_start		= &start,
		next_partition_len		= &len,
		next_partition_offset		= &offset,
		next_partition_entry		= &entry1,
		next_partition_ext_offset	= &ext_offset1,
		next_partition_buf		= mbr,
		next_partition ()))
	{
	  if (type != PC_SLICE_TYPE_NONE
	      && ! IS_PC_SLICE_TYPE_BSD (type)
	      && ! IS_PC_SLICE_TYPE_EXTENDED (type))
	    {
		saved_partition = current_partition;
		current_partition = part;
		if (attempt_mnt)
		{
		   if (! open_device ())
			current_partition = saved_partition;
		}
	    }

	  /* We want to ignore any error here.  */
	  errnum = ERR_NONE;
	}

	saved_drive = tmp_drive;
	saved_partition = tmp_partition;
	errnum = ERR_NONE;

    }
  else if (grub_memcmp (arg, "bootdev", 7) == 0)
    {
	/* use original boot device */
	current_partition = install_partition;
	current_drive = boot_drive;
	next = arg + 7;
    }
  else
    {
	/* Call set_device to get the drive and the partition in ARG.  */
	if (! (next = set_device (arg)))
	    return 0;
    }

  if (next)
  {
	/* check the length of the root prefix, i.e., NEXT */
	for (i = 0; i < sizeof (saved_dir); i++)
	{
		ch = next[i];
		if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		if (ch == '\\')
		{
			//saved_dir[i] = ch;
			i++;
			ch = next[i];
			if (! ch || i >= sizeof (saved_dir))
			{
				i--;
				//saved_dir[i] = 0;
				break;
			}
		}
	}

	if (i >= sizeof (saved_dir))
	{
		errnum = ERR_WONT_FIT;
		return 0;
	}

	tmp_partition = current_partition;
	tmp_drive = current_drive;
  }

  errnum = ERR_NONE;

  /* Ignore ERR_FSYS_MOUNT.  */
  if (attempt_mnt)
    {
      if (! open_device () && errnum != ERR_FSYS_MOUNT)
	return 0;

#if 1
      if (next)
      {
	unsigned long long hdbias = 0;
	char *biasptr;

	/* BSD and chainloading evil hacks !!  */
	biasptr = skip_to (0, next);
	safe_parse_maxint (&biasptr, &hdbias);
	errnum = 0;
	bootdev = set_bootdev (hdbias);
      }
      if (errnum)
	return 0;
#endif
      
      if (fsys_type != NUM_FSYS || ! next)
        /* Print the type of the filesystem.  */
      {
	    if (! next)
			print_root_device (NULL,0);
		if (! next || debug )
        print_fsys_type ();
      }
      else
	return ! (errnum = ERR_FSYS_MOUNT);
    }
#if 1
  else if (next)
    {
      /* This is necessary, because the location of a partition table
	 must be set appropriately.  */
      if (open_partition ())
	{
	  set_bootdev (0);
	  if (errnum)
	    return 0;
	}
    }
#endif
  
  if (next)
  {
	if (kernel_type == KERNEL_TYPE_CHAINLOADER)
	{
	  if (is_io)
	  {
		/* DL=drive, DH=media descriptor: 0xF0=floppy, 0xF8=harddrive */
		chainloader_edx = (tmp_drive & 0xFF) | 0xF000 | ((tmp_drive & 0x80) << 4);
		chainloader_edx_set = 1;

		/* the user might wrongly set these argument, so force them to be correct */

		chainloader_ebx = 0;    // clear BX for WinME
		chainloader_ebx_set = 1;
		chainloader_load_segment = 0x0070;
		chainloader_load_offset = 0;
		chainloader_skip_length = 0x0800;
	  } else {
	    if (chainloader_edx_set)
	    {
		chainloader_edx &= 0xFFFF0000;
		chainloader_edx |= tmp_drive | ((tmp_partition >> 8) & 0xFF00);
	    }

	    if (chainloader_ebx_set && chainloader_ebx)
	    {
		chainloader_ebx &= 0xFFFF0000;
		chainloader_ebx |= tmp_drive | ((tmp_partition >> 8) & 0xFF00);
	    }
	  }
	}

	saved_partition = tmp_partition;
	saved_drive = tmp_drive;

	/* copy root prefix to saved_dir */
	for (i = 0; i < sizeof (saved_dir); i++)
	{
		ch = next[i];
		if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		if (ch == '\\')
		{
			saved_dir[i] = ch;
			i++;
			ch = next[i];
			if (! ch || i >= sizeof (saved_dir))
			{
				i--;
				saved_dir[i] = 0;
				break;
			}
		}
		saved_dir[i] = ch;
	}

	if (saved_dir[i-1] == '/')
	{
		saved_dir[i-1] = 0;
	} else
		saved_dir[i] = 0;
  }

  if (debug > 0 && *saved_dir)
	grub_printf ("The current working directory(i.e., the relative path) is %s\n", saved_dir);

  /* Clear ERRNUM.  */
  errnum = 0;
  /* If ARG is empty, then return TRUE for harddrive, and FALSE for floppy */
  return next ? 1 : (saved_drive & 0x80);
}

static int
root_func (char *arg, int flags)
{
  return real_root_func (arg, 1);
}

static struct builtin builtin_root =
{
  "root",
  root_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "root [DEVICE [HDBIAS]]",
  "Set the current \"root device\" to the device DEVICE, then"
  " attempt to mount it to get the partition size (for passing the"
  " partition descriptor in `ES:ESI', used by some chain-loaded"
  " bootloaders), the BSD drive-type (for booting BSD kernels using"
  " their native boot format), and correctly determine "
  " the PC partition where a BSD sub-partition is located. The"
  " optional HDBIAS parameter is a number to tell a BSD kernel"
  " how many BIOS drive numbers are on controllers before the current"
  " one. For example, if there is an IDE disk and a SCSI disk, and your"
  " FreeBSD root partition is on the SCSI disk, then use a `1' for HDBIAS."
};


/* rootnoverify */
static int
rootnoverify_func (char *arg, int flags)
{
  return real_root_func (arg, 0);
}

static struct builtin builtin_rootnoverify =
{
  "rootnoverify",
  rootnoverify_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "rootnoverify [DEVICE [HDBIAS]]",
  "Similar to `root', but don't attempt to mount the partition. This"
  " is useful for when an OS is outside of the area of the disk that"
  " GRUB can read, but setting the correct root device is still"
  " desired. Note that the items mentioned in `root' which"
  " derived from attempting the mount will NOT work correctly."
};


static int time1;
static int time2;

#if !defined(SUPPORT_DISKLESS) && !defined(GRUB_UTIL)
static unsigned long saved_sectors[2];
static unsigned long saved_offsets[2];
static unsigned long saved_lengths[2];
static unsigned long long wait;
static unsigned long long entryno;
static char *default_file;
//static int c;
static int deny_write;

  /* Save sector information about at most two sectors.  */
static void disk_read_savesect_func1 (unsigned long sector, unsigned long offset, unsigned long length);
static void
disk_read_savesect_func1 (unsigned long sector, unsigned long offset, unsigned long length)
{
      if (blklst_num_sectors < 2)
	{
	  saved_sectors[blklst_num_sectors] = sector;
	  saved_offsets[blklst_num_sectors] = offset;
	  saved_lengths[blklst_num_sectors] = length;
	}
      blklst_num_sectors++;
}

static void prompt_user (void);

static void
prompt_user (void)
{
	int wait1;
	int c;
	wait1 = wait;

	printf("\nAbout to write the entry number %d to file %s\n\n"
		"Press Y to allow or N to deny.\n", entryno, default_file);

	/* Get current time.  */
	while ((time1 = getrtsecs ()) == 0xFF);

	for (;;)
	{
	  /* Check if ESC is pressed.  */
	  if (checkkey () != -1 /*&& ASCII_CHAR (getkey ()) == '\e'*/)
	    {
	      if ((c = ASCII_CHAR (getkey ())) == '\e')
	      {
		deny_write = 2;	/* abort this entry */
		break;
	      }
	      if (c == 'Y' || c == 'y')
	      {
		deny_write = -1;
		break;		/* allow the write */
	      }
	      if (c == 'N' || c == 'n')
	      {
		deny_write = 1;
		break;		/* deny the write */
	      }
	      
	      /* any other key restore the wait */
	      wait1 = wait;
	    }

	  if (wait1 != -1
	      && (time1 = getrtsecs ()) != time2
	      && time1 != 0xFF)
	    {
	      if (wait1 == 0)
	      {
		deny_write = 1;
		break;	/* timed out, deny the write */
	      }

	      time2 = time1;
	      wait1--;
	    }
	}

}
	    
#endif /* ! SUPPORT_DISKLESS && ! GRUB_UTIL */
  
/* savedefault */
static int
savedefault_func (char *arg, int flags)
{
#if !defined(SUPPORT_DISKLESS) && !defined(GRUB_UTIL)
  unsigned long tmp_drive = saved_drive;
  unsigned long tmp_partition = saved_partition;

  char *p;
//  int ignore_error = 0;

  blklst_num_sectors = 0;
  wait = 0;
  default_file = (char *) DEFAULT_FILE_BUF;
  time2 = -1;
  deny_write = 0;
  
  /* This command is only useful when you boot an entry from the menu
     interface.  */
//  if (! (flags & BUILTIN_SCRIPT))
//    {
//      errnum = ERR_UNRECOGNIZED;
//      return 0;
//    }

  for (;;)
  {
    /*if (grub_memcmp (arg, "--ignore-error", 14) == 0)
      {
	ignore_error = 1;
      }
    else*/ if (grub_memcmp (arg, "--wait=", 7) == 0)
      {
	p = arg + 7;
	if (! safe_parse_maxint (&p, &wait))
		return 0;
      }
    else
	break;
    arg = skip_to (0, arg);
  }
  
  /* Determine a saved entry number.  */
  if (*arg)
    {
      if (grub_memcmp (arg, "fallback", sizeof ("fallback") - 1) == 0)
	{
	  int i;
	  int index = 0;
	  
	  for (i = 0; i < MAX_FALLBACK_ENTRIES; i++)
	    {
	      if (fallback_entries[i] < 0)
		break;
	      if (fallback_entries[i] == current_entryno)
		{
		  index = i + 1;
		  break;
		}
	    }
	  
	  if (index >= MAX_FALLBACK_ENTRIES || fallback_entries[index] < 0)
	    {
	      /* This is the last.  */
	      errnum = ERR_BAD_ARGUMENT;
	      return 0;
	    }

	  entryno = fallback_entries[index];
	}
      else if (! safe_parse_maxint (&arg, &entryno))
	return 0;
    }
  else
    entryno = current_entryno;

  /* Open the default file.  */
  saved_drive = boot_drive;
  saved_partition = install_partition;
  if (grub_open (default_file))
    {
      unsigned long len, len1;
      char buf[12];
#ifdef STAGE1_5
      char mbr[512];
#endif
      
      if (compressed_file)
	{
	  errnum = ERR_DEFAULT_FILE;
	  goto fail;
	}

      saved_lengths[0] = 0;
      disk_read_hook = disk_read_savesect_func1;
      len = grub_read ((unsigned long long)(unsigned int)mbr, 512, 0xedde0d90);
      disk_read_hook = 0;
      grub_close ();
      
      if (len != ((filemax <= 512) ? filemax : 512))
	{
	  /* Read file failure  */
	  errnum = ERR_READ;
	  goto fail;
	}

      if (len < 180 || filemax > 2048)
	{
	  /* This is too small or too large. Do not modify the file manually!  */
	  errnum = ERR_DEFAULT_FILE;
	  goto fail;
	}

      /* check file content for safety */
      p = mbr;
      while (p < mbr + len - 100 &&
	  grub_memcmp (++p, warning_defaultfile, 73));

      if (p > mbr + len - 160)
	{
	  errnum = ERR_DEFAULT_FILE;
	  goto fail;
	}

      if (blklst_num_sectors > 2 || blklst_num_sectors <= 0 || saved_lengths[0] <= 0 || saved_lengths[0] > SECTOR_SIZE)
	{
	  /* Is this possible?! Too fragmented!  */
	  errnum = ERR_FSYS_CORRUPT;
	  goto fail;
	}
      
      /* Set up a string to be written.  */
      //grub_memset (mbr, '\n', 11);
      len = grub_sprintf (buf, "%d", entryno);
      len++;	/* including the ending null. */
      
//      if (saved_lengths[0] < len)
//	{
	  //char sect[512];
	  if (! rawread (current_drive, saved_sectors[0], 0, SECTOR_SIZE, (unsigned long long)(unsigned int)mbr, 0xedde0d90))
	    goto fail;
	  
	  len1 = saved_lengths[0] < len ? saved_lengths[0] : len;

	  /* it is not good to write a sector frequently, so check if we
	   * can skip the write. */
	  
	  if (grub_memcmp (mbr + saved_offsets[0], buf, len1))
	  {
	    grub_memmove (mbr + saved_offsets[0], buf, len1);

	    /* confirm the write */
	    if (wait && deny_write == 0)
		prompt_user();
	    
	    if (deny_write == 2)
		return 0;

	    if (deny_write <= 0 )
	    {
	      if (! rawwrite (current_drive, saved_sectors[0], mbr))
		goto fail;
	    }
	  }

	  /* The file is anchored to another file and the first few bytes
	     are spanned in two sectors. Uggh...  */

	  //len = strlen(buf) + 1; /* entryno and the ending NULL */
	  
	  /* only LEN bytes need to be written */
	  if (saved_lengths[0] < len)
	  {
	    /* write the rest bytes to the second sector */
	    if (! rawread (current_drive, saved_sectors[1], 0, 512, (unsigned long long)(unsigned int)mbr, 0xedde0d90))
		goto fail;
	    
	    /* skip the write if possible. */
	    if (grub_memcmp (mbr + saved_offsets[1],
			buf + saved_lengths[0],
			len - saved_lengths[0]))
	    {
	      grub_memmove (mbr + saved_offsets[1],
			buf + saved_lengths[0],
			len - saved_lengths[0]);
	      
	      if (wait && deny_write == 0)
		prompt_user();
	
	      if (deny_write == 2)
		return 0;

	      if (deny_write <= 0 )
	      {
		if (! rawwrite (current_drive, saved_sectors[1], mbr))
			goto fail;
	      }
	    }
	  }
//	}
//      else
//	{
//	  /* This is a simple case. It fits into a single sector.  */
//	  if (! rawread (current_drive, saved_sectors[0], 0, SECTOR_SIZE,
//			 sect))
//	    goto fail;
//	  grub_memmove (sect + saved_offsets[0], buf, len);
//	  if (! rawwrite (current_drive, saved_sectors[0], sect))
//	    goto fail;
//	}

      /* Clear the cache.  */
      buf_track = -1;
    }

 fail:
  saved_drive = tmp_drive;
  saved_partition = tmp_partition;

  if (errnum)
  {
	if (debug > 0)
	  grub_printf ("\nError occurred while savedefault.\n");
	/* ignore all errors, but return false. */
	return (errnum = 0);
  }

  return ! errnum;
#else /* ! SUPPORT_DISKLESS && ! GRUB_UTIL */
  errnum = ERR_UNRECOGNIZED;
  return 0;
#endif /* ! SUPPORT_DISKLESS && ! GRUB_UTIL */
}

static struct builtin builtin_savedefault =
{
  "savedefault",
  savedefault_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "savedefault [--wait=T] [NUM | `fallback']",
  "Save the current entry as the default boot entry if no argument is"
  " specified. If a number is specified, this number is saved. If"
  " `fallback' is used, next fallback entry is saved."
  " If T is not 0, prompt the user to confirm the write operation by"
  " pressing the Y key, and if no key-press detected within T seconds,"
  " the write will be discarded."
};


#ifdef SUPPORT_SERIAL
/* serial */
static int
serial_func (char *arg, int flags)
{
  unsigned short port = serial_hw_get_port (0);
  unsigned int speed = 9600;
  int word_len = UART_8BITS_WORD;
  int parity = UART_NO_PARITY;
  int stop_bit_len = UART_1_STOP_BIT;

  /* Process GNU-style long options.
     FIXME: We should implement a getopt-like function, to avoid
     duplications.  */
  while (1)
    {
      if (grub_memcmp (arg, "--unit=", sizeof ("--unit=") - 1) == 0)
	{
	  char *p = arg + sizeof ("--unit=") - 1;
	  unsigned long long unit;
	  
	  if (! safe_parse_maxint (&p, &unit))
	    return 0;
	  
	  if (unit < 0 || unit > 3)
	    {
	      errnum = ERR_DEV_VALUES;
	      return 0;
	    }

	  port = serial_hw_get_port (unit);
	}
      else if (grub_memcmp (arg, "--speed=", sizeof ("--speed=") - 1) == 0)
	{
	  char *p = arg + sizeof ("--speed=") - 1;
	  unsigned long long num;
	  
	  if (! safe_parse_maxint (&p, &num))
	    return 0;

	  speed = (unsigned int) num;
	}
      else if (grub_memcmp (arg, "--port=", sizeof ("--port=") - 1) == 0)
	{
	  char *p = arg + sizeof ("--port=") - 1;
	  unsigned long long num;
	  
	  if (! safe_parse_maxint (&p, &num))
	    return 0;

	  port = (unsigned short) num;
	}
      else if (grub_memcmp (arg, "--word=", sizeof ("--word=") - 1) == 0)
	{
	  char *p = arg + sizeof ("--word=") - 1;
	  unsigned long long len;
	  
	  if (! safe_parse_maxint (&p, &len))
	    return 0;

	  switch (len)
	    {
	    case 5: word_len = UART_5BITS_WORD; break;
	    case 6: word_len = UART_6BITS_WORD; break;
	    case 7: word_len = UART_7BITS_WORD; break;
	    case 8: word_len = UART_8BITS_WORD; break;
	    default:
	      errnum = ERR_BAD_ARGUMENT;
	      return 0;
	    }
	}
      else if (grub_memcmp (arg, "--stop=", sizeof ("--stop=") - 1) == 0)
	{
	  char *p = arg + sizeof ("--stop=") - 1;
	  unsigned long long len;
	  
	  if (! safe_parse_maxint (&p, &len))
	    return 0;

	  switch (len)
	    {
	    case 1: stop_bit_len = UART_1_STOP_BIT; break;
	    case 2: stop_bit_len = UART_2_STOP_BITS; break;
	    default:
	      errnum = ERR_BAD_ARGUMENT;
	      return 0;
	    }
	}
      else if (grub_memcmp (arg, "--parity=", sizeof ("--parity=") - 1) == 0)
	{
	  char *p = arg + sizeof ("--parity=") - 1;

	  if (grub_memcmp (p, "no", sizeof ("no") - 1) == 0)
	    parity = UART_NO_PARITY;
	  else if (grub_memcmp (p, "odd", sizeof ("odd") - 1) == 0)
	    parity = UART_ODD_PARITY;
	  else if (grub_memcmp (p, "even", sizeof ("even") - 1) == 0)
	    parity = UART_EVEN_PARITY;
	  else
	    {
	      errnum = ERR_BAD_ARGUMENT;
	      return 0;
	    }
	}
# ifdef GRUB_UTIL
      /* In the grub shell, don't use any port number but open a tty
	 device instead.  */
      else if (grub_memcmp (arg, "--device=", sizeof ("--device=") - 1) == 0)
	{
	  char *p = arg + sizeof ("--device=") - 1;
	  char dev[256];	/* XXX */
	  char *q = dev;
	  
	  while (*p && ! grub_isspace (*p))
	    *q++ = *p++;
	  
	  *q = 0;
	  serial_set_device (dev);
	}
# endif /* GRUB_UTIL */
      else
	break;

      arg = skip_to (0, arg);
    }

  /* Initialize the serial unit.  */
  if (! serial_hw_init (port, speed, word_len, parity, stop_bit_len))
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  
  return 1;
}

static struct builtin builtin_serial =
{
  "serial",
  serial_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "serial [--unit=UNIT] [--port=PORT] [--speed=SPEED] [--word=WORD]\n [--parity=PARITY] [--stop=STOP] [--device=DEV]",
  "Initialize a serial device. UNIT is a digit that specifies which serial"
  " device is used (e.g. 0 == COM1). If you need to specify the port number,"
  " set it by --port. SPEED is the DTE-DTE speed. WORD is the word length,"
  " PARITY is the type of parity, which is one of `no', `odd' and `even'."
  " STOP is the length of stop bit(s). The option --device can be used only"
  " in the grub shell, which specifies the file name of a tty device. The"
  " default values are COM1, 9600, 8N1."
};
#endif /* SUPPORT_SERIAL */


/* setkey */
struct keysym
{
  char *unshifted_name;			/* the name in unshifted state */
  char *shifted_name;			/* the name in shifted state */
  unsigned char unshifted_ascii;	/* the ascii code in unshifted state */
  unsigned char shifted_ascii;		/* the ascii code in shifted state */
  unsigned char keycode;		/* keyboard scancode */
};

/* The table for key symbols. If the "shifted" member of an entry is
   NULL, the entry does not have shifted state.  */
static struct keysym keysym_table[] =
{
  {"escape",		0,		0x1b,	0,	0x01},
  {"1",			"exclam",	'1',	'!',	0x02},
  {"2",			"at",		'2',	'@',	0x03},
  {"3",			"numbersign",	'3',	'#',	0x04},
  {"4",			"dollar",	'4',	'$',	0x05},
  {"5",			"percent",	'5',	'%',	0x06},
  {"6",			"caret",	'6',	'^',	0x07},
  {"7",			"ampersand",	'7',	'&',	0x08},
  {"8",			"asterisk",	'8',	'*',	0x09},
  {"9",			"parenleft",	'9',	'(',	0x0a},
  {"0",			"parenright",	'0',	')',	0x0b},
  {"minus",		"underscore",	'-',	'_',	0x0c},
  {"equal",		"plus",		'=',	'+',	0x0d},
  {"backspace",		0,		'\b',	0,	0x0e},
  {"tab",		0,		'\t',	0,	0x0f},
  {"q",			"Q",		'q',	'Q',	0x10},
  {"w",			"W",		'w',	'W',	0x11},
  {"e",			"E",		'e',	'E',	0x12},
  {"r",			"R",		'r',	'R',	0x13},
  {"t",			"T",		't',	'T',	0x14},
  {"y",			"Y",		'y',	'Y',	0x15},
  {"u",			"U",		'u',	'U',	0x16},
  {"i",			"I",		'i',	'I',	0x17},
  {"o",			"O",		'o',	'O',	0x18},
  {"p",			"P",		'p',	'P',	0x19},
  {"bracketleft",	"braceleft",	'[',	'{',	0x1a},
  {"bracketright",	"braceright",	']',	'}',	0x1b},
  {"enter",		0,		'\n',	0,	0x1c},
  {"control",		0,		0,	0,	0x1d},
  {"a",			"A",		'a',	'A',	0x1e},
  {"s",			"S",		's',	'S',	0x1f},
  {"d",			"D",		'd',	'D',	0x20},
  {"f",			"F",		'f',	'F',	0x21},
  {"g",			"G",		'g',	'G',	0x22},
  {"h",			"H",		'h',	'H',	0x23},
  {"j",			"J",		'j',	'J',	0x24},
  {"k",			"K",		'k',	'K',	0x25},
  {"l",			"L",		'l',	'L',	0x26},
  {"semicolon",		"colon",	';',	':',	0x27},
  {"quote",		"doublequote",	'\'',	'"',	0x28},
  {"backquote",		"tilde",	'`',	'~',	0x29},
  {"shift",		0,		0,	0,	0x2a},
  {"backslash",		"bar",		'\\',	'|',	0x2b},
  {"z",			"Z",		'z',	'Z',	0x2c},
  {"x",			"X",		'x',	'X',	0x2d},
  {"c",			"C",		'c',	'C',	0x2e},
  {"v",			"V",		'v',	'V',	0x2f},
  {"b",			"B",		'b',	'B',	0x30},
  {"n",			"N",		'n',	'N',	0x31},
  {"m",			"M",		'm',	'M',	0x32},
  {"comma",		"less",		',',	'<',	0x33},
  {"period",		"greater",	'.',	'>',	0x34},
  {"slash",		"question",	'/',	'?',	0x35},
  {"alt",		0,		0,	0,	0x38},
  {"space",		0,		' ',	0,	0x39},
  {"capslock",		0,		0,	0,	0x3a},
  {"F1",		0,		0,	0,	0x3b},
  {"F2",		0,		0,	0,	0x3c},
  {"F3",		0,		0,	0,	0x3d},
  {"F4",		0,		0,	0,	0x3e},
  {"F5",		0,		0,	0,	0x3f},
  {"F6",		0,		0,	0,	0x40},
  {"F7",		0,		0,	0,	0x41},
  {"F8",		0,		0,	0,	0x42},
  {"F9",		0,		0,	0,	0x43},
  {"F10",		0,		0,	0,	0x44},
  /* Caution: do not add NumLock here! we cannot deal with it properly.  */
  {"delete",		0,		0x7f,	0,	0x53}
};

static int find_key_code (char *key);
static int find_ascii_code (char *key);
  
static int
find_key_code (char *key)
{
      int i;

      for (i = 0; i < sizeof (keysym_table) / sizeof (keysym_table[0]); i++)
	{
	  if (keysym_table[i].unshifted_name &&
	      grub_strcmp (key, keysym_table[i].unshifted_name) == 0)
	    return keysym_table[i].keycode;
	  else if (keysym_table[i].shifted_name &&
		   grub_strcmp (key, keysym_table[i].shifted_name) == 0)
	    return keysym_table[i].keycode;
	}
      
      return 0;
}
  
static int
find_ascii_code (char *key)
{
      int i;
      
      for (i = 0; i < sizeof (keysym_table) / sizeof (keysym_table[0]); i++)
	{
	  if (keysym_table[i].unshifted_name &&
	      grub_strcmp (key, keysym_table[i].unshifted_name) == 0)
	    return keysym_table[i].unshifted_ascii;
	  else if (keysym_table[i].shifted_name &&
		   grub_strcmp (key, keysym_table[i].shifted_name) == 0)
	    return keysym_table[i].shifted_ascii;
	}
      
      return 0;
}
  
static int
setkey_func (char *arg, int flags)
{
  char *to_key, *from_key;
  int to_code, from_code;
  int map_in_interrupt = 0;
  
  to_key = arg;
  from_key = skip_to (0, to_key);

  if (! *to_key)
    {
      /* If the user specifies no argument, reset the key mappings.  */
      grub_memset (bios_key_map, 0, KEY_MAP_SIZE * sizeof (unsigned short));
      grub_memset (ascii_key_map, 0, KEY_MAP_SIZE * sizeof (unsigned short));

      return 1;
    }
  else if (! *from_key)
    {
      /* The user must specify two arguments or zero argument.  */
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  
  nul_terminate (to_key);
  nul_terminate (from_key);
  
  to_code = find_ascii_code (to_key);
  from_code = find_ascii_code (from_key);
  if (! to_code || ! from_code)
    {
      map_in_interrupt = 1;
      to_code = find_key_code (to_key);
      from_code = find_key_code (from_key);
      if (! to_code || ! from_code)
	{
	  errnum = ERR_BAD_ARGUMENT;
	  return 0;
	}
    }
  
  if (map_in_interrupt)
    {
      int i;
      
      /* Find an empty slot.  */
      for (i = 0; i < KEY_MAP_SIZE; i++)
	{
	  if ((bios_key_map[i] & 0xff) == from_code)
	    /* Perhaps the user wants to overwrite the map.  */
	    break;
	  
	  if (! bios_key_map[i])
	    break;
	}
      
      if (i == KEY_MAP_SIZE)
	{
	  errnum = ERR_WONT_FIT;
	  return 0;
	}
      
      if (to_code == from_code)
	/* If TO is equal to FROM, delete the entry.  */
	grub_memmove ((char *) &bios_key_map[i],
		      (char *) &bios_key_map[i + 1],
		      sizeof (unsigned short) * (KEY_MAP_SIZE - i));
      else
	bios_key_map[i] = (to_code << 8) | from_code;
      
#ifndef GRUB_UTIL
      /* Ugly but should work.  */
      unset_int15_handler ();
      set_int15_handler ();
#endif
    }
  else
    {
      int i;
      
      /* Find an empty slot.  */
      for (i = 0; i < KEY_MAP_SIZE; i++)
	{
	  if ((ascii_key_map[i] & 0xff) == from_code)
	    /* Perhaps the user wants to overwrite the map.  */
	    break;
	  
	  if (! ascii_key_map[i])
	    break;
	}
      
      if (i == KEY_MAP_SIZE)
	{
	  errnum = ERR_WONT_FIT;
	  return 0;
	}
      
      if (to_code == from_code)
	/* If TO is equal to FROM, delete the entry.  */
	grub_memmove ((char *) &ascii_key_map[i],
		      (char *) &ascii_key_map[i + 1],
		      sizeof (unsigned short) * (KEY_MAP_SIZE - i));
      else
	ascii_key_map[i] = (to_code << 8) | from_code;
    }
      
  return 1;
}

static struct builtin builtin_setkey =
{
  "setkey",
  setkey_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "setkey [TO_KEY FROM_KEY]",
  "Change the keyboard map. The key FROM_KEY is mapped to the key TO_KEY."
  " A key must be an alphabet, a digit, or one of these: escape, exclam,"
  " at, numbersign, dollar, percent, caret, ampersand, asterisk, parenleft,"
  " parenright, minus, underscore, equal, plus, backspace, tab, bracketleft,"
  " braceleft, bracketright, braceright, enter, control, semicolon, colon,"
  " quote, doublequote, backquote, tilde, shift, backslash, bar, comma,"
  " less, period, greater, slash, question, alt, space, capslock, FX (X"
  " is a digit), and delete. If no argument is specified, reset key"
  " mappings."
};


#ifdef GRUB_UTIL
struct stage1_5_map {
    char *fsys;
    char *name;
};
static struct stage1_5_map stage1_5_map[] =
{
    {"ext2fs",   "/e2fs_stage1_5"},
    {"fat",      "/fat_stage1_5"},
    {"ntfs",     "/ntfs_stage1_5"},
    {"ufs2",     "/ufs2_stage1_5"},
    {"ffs",      "/ffs_stage1_5"},
    {"iso9660",  "/iso9660_stage1_5"},
    {"jfs",      "/jfs_stage1_5"},
    {"minix",    "/minix_stage1_5"},
    {"reiserfs", "/reiserfs_stage1_5"},
    {"vstafs",   "/vstafs_stage1_5"},
    {"xfs",      "/xfs_stage1_5"}
};

#ifdef GRUB_UTIL
static int check_file (char *file);
static void sprint_device (int drive, int partition);
static int embed_stage1_5 (char *stage1_5, int drive, int partition, int flags);
static char device[16];
static char tmp[16];
static char stage15[64];
static char cmd_arg[256];
static char stage1[64];
static char stage2[64];
static char config_filename[64];
static char real_config_filename[64];
#else
static char *device = (char *)0x700;	// use only 16 bytes at 0x700
static char *tmp = (char *)0x710;	// use only 16 bytes at 0x710
static char *stage15 = (char *)0x780;	// use only 64 bytes at 0x780
/* we cannot use 0x8000 because it is for mbr. So we use 1 sector below 0x2B0000 instead. */
static char *cmd_arg = (char *)(HMA_ADDR - 0x200);//(char *)0x8000;	// use 256 bytes at 0x8000
static char *stage1 = (char *)(HMA_ADDR - 0x100);//(char *)0x8100;	// use 64 bytes at 0x8100
static char *stage2 = (char *)(HMA_ADDR - 0xC0);//(char *)0x8140;	// use 64 bytes at 0x8140
static char *config_filename = (char *)(HMA_ADDR - 0x80);//(char *)0x8180;	// use 64 bytes at 0x8180
static char *real_config_filename = (char *)(HMA_ADDR - 0x40);//(char *)0x81C0;	// use 64 bytes at 0x81C0
#endif
static char *buffer;

  /* Check if the file FILE exists like Autoconf.  */
static int
check_file (char *file)
{
      int ret;
      
      grub_printf (" Checking if \"%s\" exists... ", file);
      ret = grub_open (file);
      if (ret)
	{
	  grub_close ();
	  grub_printf ("yes\n");
	}
      else
	grub_printf ("no\n");

      return ret;
}
  
  /* Construct a device name in DEVICE.  */
static void
sprint_device (int drive, int partition)
{
      grub_sprintf (device, "(%cd%d",
		    ((drive & 0x80) ? 'h' : 'f'),
		    (drive & ~0x80));
      if ((partition & 0xFF0000) != 0xFF0000)
	{
	  //char tmp[16];
	  grub_sprintf (tmp, ",%d", (unsigned long)(unsigned char)(partition >> 16));
	  grub_strncat (device, tmp, 16);
	}
      if ((partition & 0x00FF00) != 0x00FF00)
	{
	  //char tmp[16];
	  grub_sprintf (tmp, ",%c", (unsigned long)(unsigned char)('a' + (partition >> 8)));
	  grub_strncat (device, tmp, 16);
	}
      grub_strncat (device, ")", 16);
}
  
static int
embed_stage1_5 (char *stage1_5, int drive, int partition, int flags)
{
      /* We install GRUB into the MBR, so try to embed the
	 Stage 1.5 in the sectors right after the MBR.  */
      sprint_device (drive, partition);
      grub_sprintf (cmd_arg, "%s %s", stage1_5, device);
	      
      /* Notify what will be run.  */
      grub_printf (" Running \"embed %s\"... ", cmd_arg);
      
      embed_func (cmd_arg, flags);
      if (! errnum)
	{
	  /* Construct the blocklist representation.  */
	  grub_sprintf (buffer, "%s%s", device, embed_info);
	  grub_printf ("succeeded\n");
	  return 1;
	}
      else
	{
	  grub_printf ("failed (this is not fatal)\n");
	  return 0;
	}
}
	  
/* setup */
static int
setup_func (char *arg, int flags)
{
  /* Point to the string of the installed drive/partition.  */
  char *install_ptr;
  /* Point to the string of the drive/parition where the GRUB images
     reside.  */
  char *image_ptr;
  unsigned long installed_drive, installed_partition;
  unsigned long image_drive, image_partition;
  unsigned long tmp_drive, tmp_partition;
  int is_force_lba = 0;
  char *stage2_arg = 0;
  char *prefix = 0;

  tmp_drive = saved_drive;
  tmp_partition = saved_partition;

  buffer = (char *) RAW_ADDR (0x100000);

  /* Check if the user specifies --force-lba.  */
  while (1)
    {
      if (grub_memcmp ("--force-lba", arg, sizeof ("--force-lba") - 1) == 0)
	{
	  is_force_lba = 1;
	  arg = skip_to (0, arg);
	}
      else if (grub_memcmp ("--prefix=", arg, sizeof ("--prefix=") - 1) == 0)
	{
	  prefix = arg + sizeof ("--prefix=") - 1;
	  arg = skip_to (0, arg);
	  nul_terminate (prefix);
	}
#ifdef GRUB_UTIL
      else if (grub_memcmp ("--stage2=", arg, sizeof ("--stage2=") - 1) == 0)
	{
	  stage2_arg = arg;
	  arg = skip_to (0, arg);
	  nul_terminate (stage2_arg);
	}
#endif /* GRUB_UTIL */
      else
	break;
    }
  
  install_ptr = arg;
  image_ptr = skip_to (0, install_ptr);

  /* Make sure that INSTALL_PTR is valid.  */
  set_device (install_ptr);
  if (errnum)
    goto fail;//    return 1;

  installed_drive = current_drive;
  installed_partition = current_partition;
  
  /* Mount the drive pointed by IMAGE_PTR.  */
  if (*image_ptr)
    {
      /* If the drive/partition where the images reside is specified,
	 get the drive and the partition.  */
      set_device (image_ptr);
      if (errnum)
	goto fail;//    return 1;
    }
  else
    {
      /* If omitted, use SAVED_PARTITION and SAVED_DRIVE.  */
      current_drive = saved_drive;
      current_partition = saved_partition;
    }

  image_drive = saved_drive = current_drive;
  image_partition = saved_partition = current_partition;

  /* Open it.  */
  if (! open_device ())
    goto fail;

  /* Check if stage1 exists. If the user doesn't specify the option
     `--prefix', attempt /boot/grub and /grub.  */
  /* NOTE: It is dangerous to run this command without `--prefix' in the
     grub shell, since that affects `--stage2'.  */
  if (! prefix)
    {
      prefix = "/boot/grub";
      grub_sprintf (stage1, "%s%s", prefix, "/stage1");
      if (! check_file (stage1))
	{
	  errnum = ERR_NONE;
	  prefix = "/grub";
	  grub_sprintf (stage1, "%s%s", prefix, "/stage1");
	  if (! check_file (stage1))
	    goto fail;
	}
    }
  else
    {
      grub_sprintf (stage1, "%s%s", prefix, "/stage1");
      if (! check_file (stage1))
	goto fail;
    }

  /* The prefix was determined.  */
  grub_sprintf (stage2, "%s%s", prefix, "/stage2");
  grub_sprintf (config_filename, "%s%s", prefix, "/menu.lst");
  *real_config_filename = 0;

  /* Check if stage2 exists.  */
  if (! check_file (stage2))
    goto fail;

  {
    char *fsys = fsys_table[fsys_type].name;
    int i;
    int size = sizeof (stage1_5_map) / sizeof (stage1_5_map[0]);
    
    /* Iterate finding the same filesystem name as FSYS.  */
    for (i = 0; i < size; i++)
      if (grub_strcmp (fsys, stage1_5_map[i].fsys) == 0)
	{
	  /* OK, check if the Stage 1.5 exists.  */
	  //char stage15[64];
	  
	  grub_sprintf (stage15, "%s%s", prefix, stage1_5_map[i].name);
	  if (check_file (stage15))
	    {
	      if (embed_stage1_5 (stage15, installed_drive, installed_partition, flags)
		  || embed_stage1_5 (stage15, image_drive, image_partition, flags))
		{
		  grub_strcpy (real_config_filename, config_filename);
		  sprint_device (image_drive, image_partition);
		  grub_sprintf (config_filename, "%s%s", device, stage2);
		  grub_strcpy (stage2, buffer);
		}
	    }
	  errnum = 0;
	  break;
	}
  }

  /* Construct a string that is used by the command "install" as its
     arguments.  */
  sprint_device (installed_drive, installed_partition);
  
#if 1
  /* Don't embed a drive number unnecessarily.  */
  grub_sprintf (cmd_arg, "%s%s%s%s %s%s %s p %s %s",
		(is_force_lba? "--force-lba " : ""),
		(stage2_arg? stage2_arg : ""),
		(stage2_arg? " " : ""),
		stage1,
		((installed_drive != image_drive) ? "d " : ""),
		device,
		stage2,
		config_filename,
		real_config_filename);
#else /* NOT USED */
  /* This code was used, because we belived some BIOSes had a problem
     that they didn't pass a booting drive correctly. It turned out,
     however, stage1 could trash a booting drive when checking LBA support,
     because some BIOSes modified the register %dx in INT 13H, AH=48H.
     So it becamed unclear whether GRUB should use a pre-defined booting
     drive or not. If the problem still exists, it would be necessary to
     switch back to this code.  */
  grub_sprintf (cmd_arg, "%s%s%s%s d %s %s p %s %s",
		(is_force_lba? "--force-lba " : ""),
		(stage2_arg? stage2_arg : ""),
		(stage2_arg? " " : ""),
		stage1,
		device,
		stage2,
		config_filename,
		real_config_filename);
#endif /* NOT USED */
  
  /* Notify what will be run.  */
  grub_printf (" Running \"install %s\"... ", cmd_arg);

  /* Make sure that SAVED_DRIVE and SAVED_PARTITION are identical
     with IMAGE_DRIVE and IMAGE_PARTITION, respectively.  */
  saved_drive = image_drive;
  saved_partition = image_partition;
  
  /* Run the command.  */
  if (install_func (cmd_arg, flags))
    grub_printf ("succeeded\nDone.\n");
  else
    grub_printf ("failed\n");

 fail:
  saved_drive = tmp_drive;
  saved_partition = tmp_partition;
//  if (debug > 0)
//    grub_printf ("\n\nNotice: the two commands SETUP and INSTALL will be removed soon! Please use the\nOS utility of BOOTLACE.COM to install GRLDR boot record to the MBR or to the\nboot area of a specified partition.\n\n");
  return ! errnum;
}

static struct builtin builtin_setup =
{
  "setup",
  setup_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST | BUILTIN_BOOTING,
  "setup [--prefix=DIR] [--stage2=STAGE2_FILE] [--force-lba]\n INSTALL_DEVICE [IMAGE_DEVICE]",
  "Set up the installation of GRUB automatically. This command uses"
  " the more flexible command \"install\" in the backend and installs"
  " GRUB into the device INSTALL_DEVICE. If IMAGE_DEVICE is specified,"
  " then find the GRUB images in the device IMAGE_DEVICE, otherwise"
  " use the current \"root device\", which can be set by the command"
  " \"root\". If you know that your BIOS should support LBA but GRUB"
  " doesn't work in LBA mode, specify the option `--force-lba'."
  " If you install GRUB under the grub shell and you cannot unmount the"
  " partition where GRUB images reside, specify the option `--stage2'"
  " to tell GRUB the file name under your OS."
};
#endif /* GRUB_UTIL */


#if defined(SUPPORT_SERIAL) || defined(SUPPORT_HERCULES) || defined(SUPPORT_GRAPHICS)
/* terminal */
static int
terminal_func (char *arg, int flags)
{
  /* The index of the default terminal in TERM_TABLE.  */
  int default_term = -1;
  struct term_entry *prev_term = current_term;
  unsigned long long to = -1;
  unsigned long long lines = 0;
  int no_message = 0;
  unsigned long term_flags = 0;
  /* XXX: Assume less than 32 terminals.  */
  unsigned long term_bitmap = 0;

  /* Get GNU-style long options.  */
  while (1)
    {
      if (grub_memcmp (arg, "--dumb", sizeof ("--dumb") - 1) == 0)
	term_flags |= TERM_DUMB;
      else if (grub_memcmp (arg, "--no-echo", sizeof ("--no-echo") - 1) == 0)
	/* ``--no-echo'' implies ``--no-edit''.  */
	term_flags |= (TERM_NO_ECHO | TERM_NO_EDIT);
      else if (grub_memcmp (arg, "--no-edit", sizeof ("--no-edit") - 1) == 0)
	term_flags |= TERM_NO_EDIT;
      else if (grub_memcmp (arg, "--timeout=", sizeof ("--timeout=") - 1) == 0)
	{
	  char *val = arg + sizeof ("--timeout=") - 1;
	  
	  if (! safe_parse_maxint (&val, &to))
	    return 0;
	}
      else if (grub_memcmp (arg, "--lines=", sizeof ("--lines=") - 1) == 0)
	{
	  char *val = arg + sizeof ("--lines=") - 1;

	  if (! safe_parse_maxint (&val, &lines))
	    return 0;

	  /* Probably less than four is meaningless....  */
	  if (lines < 4)
	    {
	      errnum = ERR_BAD_ARGUMENT;
	      return 0;
	    }
	}
      else if (grub_memcmp (arg, "--silent", sizeof ("--silent") - 1) == 0)
	no_message = 1;
      else
	break;

      arg = skip_to (0, arg);
    }
  
  /* If no argument is specified, show current setting.  */
  if (! *arg)
    {
      if (debug > 0)
	grub_printf ("%s%s%s%s\n",
		   current_term->name,
		   (current_term->flags & TERM_DUMB ? " (dumb)" : ""),
		   (current_term->flags & TERM_NO_EDIT ? " (no edit)" : ""),
		   (current_term->flags & TERM_NO_ECHO ? " (no echo)" : ""));
      return 1;
    }

  while (*arg)
    {
      int i;
      char *next = skip_to (0, arg);
      
      nul_terminate (arg);

      for (i = 0; term_table[i].name; i++)
	{
	  if (grub_strcmp (arg, term_table[i].name) == 0)
	    {
	      if (term_table[i].flags & TERM_NEED_INIT)
		{
		  errnum = ERR_DEV_NEED_INIT;
		  return 1;
		}
	      
	      if (default_term < 0)
		default_term = i;

	      term_bitmap |= (1 << i);
	      break;
	    }
	}

      if (! term_table[i].name)
	{
	  errnum = ERR_BAD_ARGUMENT;
	  return 0;
	}

      arg = next;
    }

  /* If multiple terminals are specified, wait until the user pushes any
     key on one of the terminals.  */
  if (term_bitmap & ~(1 << default_term))
    {
      time2 = -1;

      /* XXX: Disable the pager.  */
      count_lines = -1;
      
      /* Get current time.  */
      while ((time1 = getrtsecs ()) == 0xFF)
	;

      /* Wait for a key input.  */
      while (to)
	{
	  int i;

	  for (i = 0; term_table[i].name; i++)
	    {
	      if (term_bitmap & (1 << i))
		{
		  if (term_table[i].checkkey () >= 0)
		    {
		      (void) term_table[i].getkey ();
		      default_term = i;
		      
		      goto end;
		    }
		}
	    }
	  
	  /* Prompt the user, once per sec.  */
	  if ((time1 = getrtsecs ()) != time2 && time1 != 0xFF)
	    {
	      if (! no_message)
		{
		  /* Need to set CURRENT_TERM to each of selected
		     terminals.  */
		  for (i = 0; term_table[i].name; i++)
		    if (term_bitmap & (1 << i))
		      {
			current_term = term_table + i;
			grub_printf ("\rPress any key to continue.\n");
		      }
		  
		  /* Restore CURRENT_TERM.  */
		  current_term = prev_term;
		}
	      
	      time2 = time1;
	      if (to > 0)
		to--;
	    }
	}
    }

 end:
  current_term = term_table + default_term;
  current_term->flags = term_flags;
  
  if (lines)
    max_lines = lines;
  else
    max_lines = current_term->max_lines;
  
  /* If the interface is currently the command-line,
     restart it to repaint the screen.  */
  if (current_term != prev_term /*&& (flags & BUILTIN_CMDLINE)*/)
  {
    if (prev_term->shutdown)
      prev_term->shutdown();
    if (current_term->startup)
      current_term->startup();
//    grub_longjmp (restart_cmdline_env, 0);
  }
  
  return 1;
}

static struct builtin builtin_terminal =
{
  "terminal",
  terminal_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "terminal [--dumb] [--no-echo] [--no-edit] [--timeout=SECS]\n [--lines=LINES] [--silent] [console] [serial] [hercules] [graphics]",
  "Select a terminal. When multiple terminals are specified, wait until"
  " you push any key to continue. If both console and serial are specified,"
  " the terminal to which you input a key first will be selected. If no"
  " argument is specified, print current setting. The option --dumb"
  " specifies that your terminal is dumb, otherwise, vt100-compatibility"
  " is assumed. If you specify --no-echo, input characters won't be echoed."
  " If you specify --no-edit, the BASH-like editing feature will be disabled."
  " If --timeout is present, this command will wait at most for SECS"
  " seconds. The option --lines specifies the maximum number of lines."
  " The option --silent is used to suppress messages."
};
#endif /* SUPPORT_SERIAL || SUPPORT_HERCULES || SUPPORT_GRAPHICS */


#ifdef SUPPORT_SERIAL

#ifdef GRUB_UTIL
static struct terminfo term1;		//struct takes up 200 bytes!!
static struct terminfo *term = &term1;

static struct
      {
	const char *name;
	char *var;
      }
      options[] =
	{
	  {"--name=", term1.name},
	  {"--cursor-address=", term1.cursor_address},
	  {"--clear-screen=", term1.clear_screen},
	  {"--enter-standout-mode=", term1.enter_standout_mode},
	  {"--exit-standout-mode=", term1.exit_standout_mode}
	};
#else
static struct terminfo *term = (struct terminfo *)0x8000;	//use 200 bytes

static struct
      {
	const char *name;
	char *var;
      }
      options[] =
	{
	  {"--name=", ((struct terminfo *)0x8000)->name},
	  {"--cursor-address=", ((struct terminfo *)0x8000)->cursor_address},
	  {"--clear-screen=", ((struct terminfo *)0x8000)->clear_screen},
	  {"--enter-standout-mode=", ((struct terminfo *)0x8000)->enter_standout_mode},
	  {"--exit-standout-mode=", ((struct terminfo *)0x8000)->exit_standout_mode}
	};
#endif

static int
terminfo_func (char *arg, int flags)
{
  if (*arg)
    {
      grub_memset (term, 0, sizeof (struct terminfo));
      
      while (*arg)
	{
	  int i;
	  char *next = skip_to (0, arg);
	      
	  nul_terminate (arg);
	  
	  for (i = 0; i < sizeof (options) / sizeof (options[0]); i++)
	    {
	      const char *name = options[i].name;
	      int len = grub_strlen (name);
	      
	      if (! grub_memcmp (arg, name, len))
		{
		  grub_strcpy (options[i].var, ti_unescape_string (arg + len));
		  break;
		}
	    }

	  if (i == sizeof (options) / sizeof (options[0]))
	    {
	      errnum = ERR_BAD_ARGUMENT;
	      return ! errnum;
	    }

	  arg = next;
	}

      if (term->name[0] == 0 || term->cursor_address[0] == 0)
	{
	  errnum = ERR_BAD_ARGUMENT;
	  return ! errnum;
	}

      ti_set_term (term);
    }
  else
    {
      /* No option specifies printing out current settings.  */
      ti_get_term (term);

      grub_printf ("name=%s\n",
		   (ti_escape_string (term->name)));
      grub_printf ("cursor_address=%s\n",
		   (ti_escape_string (term->cursor_address)));
      grub_printf ("clear_screen=%s\n",
		   (ti_escape_string (term->clear_screen)));
      grub_printf ("enter_standout_mode=%s\n",
		   (ti_escape_string (term->enter_standout_mode)));
      grub_printf ("exit_standout_mode=%s\n",
		   (ti_escape_string (term->exit_standout_mode)));
    }

  return 1;
}

static struct builtin builtin_terminfo =
{
  "terminfo",
  terminfo_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "terminfo [--name=NAME --cursor-address=SEQ [--clear-screen=SEQ]"
  "\n [--enter-standout-mode=SEQ] [--exit-standout-mode=SEQ]]",
  
  "Define the capabilities of your terminal. Use this command to"
  " define escape sequences, if it is not vt100-compatible."
  " You may use \\e for ESC and ^X for a control character."
  " If no option is specified, the current settings are printed."
};
#endif /* SUPPORT_SERIAL */
	  

#if 0
/* testload */
static int
testload_func (char *arg, int flags)
{
  //int i;

  kernel_type = KERNEL_TYPE_NONE;

  if (! grub_open (arg))
    return 0;

  disk_read_hook = disk_read_print_func;

  /* Perform filesystem test on the specified file.  */
  /* Read whole file first. */
  grub_printf ("Whole file: ");

  grub_read ((char *) RAW_ADDR (0x300000), -1);

  /* Now compare two sections of the file read differently.  */

  for (i = 0; i < 0x10ac0; i++)
    {
      *((unsigned char *) RAW_ADDR (0x400000 + i)) = 0;
      *((unsigned char *) RAW_ADDR (0x500000 + i)) = 1;
    }

  /* First partial read.  */
  grub_printf ("\nPartial read 1: ");

  filepos = 0;
  grub_read ((char *) RAW_ADDR (0x400000), 0x7);
  grub_read ((char *) RAW_ADDR (0x400007), 0x100);
  grub_read ((char *) RAW_ADDR (0x400107), 0x10);
  grub_read ((char *) RAW_ADDR (0x400117), 0x999);
  grub_read ((char *) RAW_ADDR (0x400ab0), 0x10);
  grub_read ((char *) RAW_ADDR (0x400ac0), 0x10000);

  /* Second partial read.  */
  grub_printf ("\nPartial read 2: ");

  filepos = 0;
  grub_read ((char *) RAW_ADDR (0x500000), 0x10000);
  grub_read ((char *) RAW_ADDR (0x510000), 0x10);
  grub_read ((char *) RAW_ADDR (0x510010), 0x7);
  grub_read ((char *) RAW_ADDR (0x510017), 0x10);
  grub_read ((char *) RAW_ADDR (0x510027), 0x999);
  grub_read ((char *) RAW_ADDR (0x5109c0), 0x100);

  grub_printf ("\nHeader1 = 0x%x, next = 0x%x, next = 0x%x, next = 0x%x\n",
	       *((int *) RAW_ADDR (0x400000)),
	       *((int *) RAW_ADDR (0x400004)),
	       *((int *) RAW_ADDR (0x400008)),
	       *((int *) RAW_ADDR (0x40000c)));

  grub_printf ("Header2 = 0x%x, next = 0x%x, next = 0x%x, next = 0x%x\n",
	       *((int *) RAW_ADDR (0x500000)),
	       *((int *) RAW_ADDR (0x500004)),
	       *((int *) RAW_ADDR (0x500008)),
	       *((int *) RAW_ADDR (0x50000c)));

  for (i = 0; i < 0x10ac0; i++)
    if (*((unsigned char *) RAW_ADDR (0x400000 + i))
	!= *((unsigned char *) RAW_ADDR (0x500000 + i)))
      break;

  grub_printf ("Max is 0x10ac0: i=0x%x, filepos=0x%x\n", i, filepos);
  disk_read_hook = 0;
  grub_close ();
  return 1;
}

static struct builtin builtin_testload =
{
  "testload",
  testload_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT,
  "testload FILE",
  "Read the entire contents of FILE in several different ways and"
  " compares them, to test the filesystem code. The output is somewhat"
  " cryptic, but if no errors are reported and the final `i=X,"
  " filepos=Y' reading has X and Y equal, then it is definitely"
  " consistent, and very likely works correctly subject to a"
  " consistent offset error. If this test succeeds, then a good next"
  " step is to try loading a kernel."
};
#endif


#ifdef GRUB_UTIL
static struct vbe_controller controller1;	//struct size 512
static struct vbe_mode mode1;			//struct size 255
static struct vbe_controller *controller = &controller1;
static struct vbe_mode *mode = &mode1;
#else
static struct vbe_controller *controller = (struct vbe_controller *)0x8000;//use 512 bytes
static struct vbe_mode *mode = (struct vbe_mode *)0x700;//use 255 bytes
#endif
/* testvbe MODE */
static int
testvbe_func (char *arg, int flags)
{
  unsigned long long mode_number;
//  struct vbe_controller controller;	//struct size 512
//  struct vbe_mode mode;			//struct size 255
  
  if (! *arg)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  if (! safe_parse_maxint (&arg, &mode_number))
    return 0;

  /* Preset `VBE2'.  */
  grub_memmove (controller->signature, "VBE2", 4);

  /* Detect VBE BIOS.  */
  if (get_vbe_controller_info (controller) != 0x004F)
    {
      if (debug > 0)
	grub_printf (" VBE BIOS is not present.\n");
      return 0;
    }
  
  if (controller->version < 0x0200)
    {
      if (debug > 0)
	grub_printf (" VBE version %d.%d is not supported.\n",
		    (unsigned long)(controller->version >> 8),
		    (unsigned long)(unsigned char)(controller->version));
      return 0;
    }

  if (get_vbe_mode_info (mode_number, mode) != 0x004F
      || (mode->mode_attributes & 0x0091) != 0x0091)
    {
      if (debug > 0)
	grub_printf (" Mode 0x%x is not supported.\n", mode_number);
      return 0;
    }

  /* Now trip to the graphics mode.  */
  if (set_vbe_mode (mode_number | (1 << 14)) != 0x004F)
    {
      if (debug > 0)
	grub_printf (" Switching to Mode 0x%x failed.\n", mode_number);
      return 0;
    }

  /* Draw something on the screen...  */
  {
    unsigned char *base_buf = (unsigned char *) mode->phys_base;
    int scanline = controller->version >= 0x0300
      ? mode->linear_bytes_per_scanline : mode->bytes_per_scanline;
    /* FIXME: this assumes that any depth is a modulo of 8.  */
    int bpp = mode->bits_per_pixel / 8;
    int width = mode->x_resolution;
    int height = mode->y_resolution;
    int x, y;
    unsigned color = 0;

    /* Iterate drawing on the screen, until the user hits any key.  */
    while (checkkey () == -1)
      {
	for (y = 0; y < height; y++)
	  {
	    unsigned char *line_buf = base_buf + scanline * y;
	    
	    for (x = 0; x < width; x++)
	      {
		unsigned char *buf = line_buf + bpp * x;
		int i;

		for (i = 0; i < bpp; i++, buf++)
		  *buf = (color >> (i * 8)) & 0xff;
	      }

	    color++;
	  }
      }

    /* Discard the input.  */
    getkey ();
  }
  
  /* Back to the default text mode.  */
  if (set_vbe_mode (0x03) != 0x004F)
    {
#if 1
      if (debug > 0)
	grub_printf ("\nSwitching to text mode failed!!\n");
      return 0;
#else
      /* Why?!  */
      grub_reboot ();
#endif
    }

  return 1;
}

static struct builtin builtin_testvbe =
{
  "testvbe",
  testvbe_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "testvbe MODE",
  "Test the VBE mode MODE. Hit any key to return."
};

static inline unsigned long vbe_far_ptr_to_linear (unsigned long ptr)
{
    unsigned short seg = (ptr >> 16);
    unsigned short off = (ptr & 0xFFFF);

    return (seg << 4) + off;
}
  

static struct vbe_mode *
find_video_mode(/*struct vbe_controller *controller,*/ int width, int height, int depth, int *mode_number)
{
  // if height == depth == 0, then width is not a width but a mode number
//  static struct vbe_mode mode;
  int i;
  
  if (!height && !depth) {
    if (get_vbe_mode_info (width, mode) == 0x004F)
      if ((mode->mode_attributes & 0x0091) == 0x0091) {
        *mode_number = width;
	return mode;
      }
  }

  for (i = 0; i<0x200; i++)
    {
      if (get_vbe_mode_info (i, mode) != 0x004F)
	continue;

      /* Skip this, if this is not supported or linear frame buffer
	 mode is not support.  */
      if ((mode->mode_attributes & 0x0091) != 0x0091) 
	continue;

      if (debug > 0)
	printf("Mode (0x%x) %dx%dx%d %x %x\n",
		      (unsigned long)i,
		      (unsigned long)mode->x_resolution,
		      (unsigned long)mode->y_resolution,
		      (unsigned long)mode->bits_per_pixel,
		      (unsigned long)mode->mode_attributes,
		      (unsigned long)mode->phys_base);

      if ((mode->x_resolution == width) &&
	  (mode->y_resolution == height) &&
	  (mode->bits_per_pixel == depth)) {
	    *mode_number = i;
	    return mode;
      }
    }
  return NULL;
}

/* setvbe (Contributed by Gerardo Richarte. Thanks!) */
/* set the `video=' option for the next `kernel' command. */
static int
setvbe_func (char *arg, int flags)
{
  int mode_number;
  unsigned long long width = 10000;
  unsigned long long height = 10000;
  unsigned long long depth = 128;

//  struct vbe_controller controller;	//struct size 512
//  struct vbe_mode mode;			//struct size 255
  struct vbe_mode *mode2;

  if (!*arg)
  {
    *kernel_option_video = 0; /* initialize the string to be empty. */
    if (debug > 0)
      grub_printf ("\nVideo option cleared. Nothing will be added to subsequent kernel command-line.\n");
    errnum = 0;
    return 1;
  }

  errnum = ERR_BAD_ARGUMENT;
  if (! safe_parse_maxint (&arg, &width))
    return 0;
  if (*arg == 'x') {
    arg++;
    if (! safe_parse_maxint (&arg, &height))
      return 0;
    arg++;
    if (! safe_parse_maxint (&arg, &depth))
      return 0;
  } else {
    height = depth = 0;
  }

  if (debug > 0)
    grub_printf ("Trying setvbe for resolution %dx%dx%d\n", (unsigned long)width, (unsigned long)height, (unsigned long)depth);

  /* Preset `VBE2'.  */
  grub_memmove (controller->signature, "VBE2", 4);

  /* Detect VBE BIOS.  */
  if (get_vbe_controller_info (controller) != 0x004F)
    {
      if (debug > 0)
	grub_printf (" VBE BIOS is not present.\n");
      return 0;
    }
  
  if (controller->version < 0x0200)
    {
      if (debug > 0)
	grub_printf (" VBE version %d.%d is not supported.\n",
		   (unsigned long) (controller->version >> 8),
		   (unsigned long)(unsigned char) (controller->version));
      return 0;
    }

  mode2 = find_video_mode(/*controller,*/ width, height, depth, &mode_number);

  if (!mode2)
    {
      if (debug > 0)
	grub_printf (" Resolution %dx%dx%d not found. Try vbeprobe command to see available modes\n", (unsigned long)width, (unsigned long)height, (unsigned long)depth);
      return 0;
    }

  /* Now trip to the graphics mode.  */
  if (set_vbe_mode (mode_number | (1 << 14)) != 0x004F)
    {
      if (debug > 0)
	grub_printf (" Switching to Mode 0x%x failed.\n", (unsigned long)mode_number);
      return 0;
    }

  grub_sprintf (kernel_option_video, " video=%dx%dx%d@0x%x,%d", mode2->x_resolution, mode2->y_resolution, mode2->bits_per_pixel, mode2->phys_base, ((controller->version >= 0x0300) ? (mode2->linear_bytes_per_scanline) : (mode2->bytes_per_scanline)));

  if (debug > 0)
    grub_printf ("\nThe option \"%s\" will be automatically\nappended to each subsequent kernel command-line.\n", kernel_option_video);
  
  errnum = 0;
  return 1;
}

static struct builtin builtin_setvbe =
{
  "setvbe",
  setvbe_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "setvbe [MODE_3D]",
  "Set the VBE mode MODE_3D(which is of the form 1024x768x32) for each subsequent kernel command-line."
  " If no argument is specified, clear(nullify, invalidate) the video option"
  " string setup by the previous setvbe command."
};


#ifdef SUPPORT_NETBOOT
/* tftpserver */
static int
tftpserver_func (char *arg, int flags)
{
  if (! *arg || ! ifconfig (0, 0, 0, arg))
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }

  print_network_configuration ();
  return 1;
}

static struct builtin builtin_tftpserver =
{
  "tftpserver",
  tftpserver_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "tftpserver IPADDR",
  "Override the TFTP server address."
};
#endif /* SUPPORT_NETBOOT */


/* timeout */
static int
timeout_func (char *arg, int flags)
{
  unsigned long long ull;
  if (! safe_parse_maxint (&arg, &ull))
    return 0;

  grub_timeout = ull;
  return 1;
}

static struct builtin builtin_timeout =
{
  "timeout",
  timeout_func,
  BUILTIN_MENU,
#if 0
  "timeout SEC",
  "Set a timeout, in SEC seconds, before automatically booting the"
  " default entry (normally the first entry defined)."
#endif
};


/* title */
//static int
//title_func (char *arg, int flags)
//{
//  /* This function is not actually used at least currently.  */
//  return 1;
//}

struct builtin builtin_title =
{
  "title",
  NULL/*title_func*/,
  0/*BUILTIN_TITLE*/,
#if 0
  "title [NAME ...]",
  "Start a new boot entry, and set its name to the contents of the"
  " rest of the line, starting with the first non-space character."
#endif
};


#if ! defined(GRUB_UTIL) && ! defined (STAGE1_5)
extern int tpm_init(void);
/* tpm */
static int
tpm_func (char *arg, int flags)
{
  for (;;)
  {
    if (grub_memcmp (arg, "--init", 6) == 0)
      {
	return tpm_init();
      }
    else
      return ! (errnum = ERR_BAD_ARGUMENT);
    arg = skip_to (0, arg);
  }
  
  return 1;
}

static struct builtin builtin_tpm =
{
  "tpm",
  tpm_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "tpm --init",
  "Initialise TPM."
};
#endif /* ! defined(GRUB_UTIL) && ! defined (STAGE1_5) */


/* unhide */
static int
unhide_func (char *arg, int flags)
{
  /* If no arguments, unhide current partition in the current drive. */
  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	current_drive = saved_drive;
	current_partition = saved_partition;
  }
  else if (! set_device (arg))
    return 0;

  return set_partition_hidden_flag (0);
}

static struct builtin builtin_unhide =
{
  "unhide",
  unhide_func,
  BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_MENU | BUILTIN_HELP_LIST,
  "unhide [PARTITION]",
  "Unhide PARTITION by clearing the \"hidden\" bit in its"
  " partition type code. The default partition is the current"
  " root device."
};


#if 0
/* uppermem */
static int
uppermem_func (char *arg, int flags)
{
  if (! safe_parse_maxint (&arg, (int *)(void *) &mbi.mem_upper))
    return 0;

  mbi.flags &= ~MB_INFO_MEM_MAP;
  return 1;
}

static struct builtin builtin_uppermem =
{
  "uppermem",
  uppermem_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "uppermem KBYTES",
  "Force GRUB to assume that only KBYTES kilobytes of upper memory are"
  " installed.  Any system address range maps are discarded."
};
#endif


/* vbeprobe */
static int
vbeprobe_func (char *arg, int flags)
{
//  struct vbe_controller controller;	//struct size 512
  unsigned short *mode_list;
  unsigned long long mode_number = -1;
  
  if (*arg)
    {
      if (! safe_parse_maxint (&arg, &mode_number))
	return 0;
    }
  
  /* Set the signature to `VBE2', to obtain VBE 3.0 information.  */
  grub_memmove (controller->signature, "VBE2", 4);
  
  if (get_vbe_controller_info (controller) != 0x004F)
    {
      if (debug > 0)
	grub_printf (" VBE BIOS is not present.\n");
      return 0;
    }

  /* Check the version.  */
  if (controller->version < 0x0200)
    {
      if (debug > 0)
	grub_printf (" VBE version %d.%d is not supported.\n",
		   (unsigned long) (controller->version >> 8),
		   (unsigned long)(unsigned char) (controller->version));
      return 0;
    }

  /* Print some information.  */
  if (debug > 0)
    grub_printf (" VBE version %d.%d\n",
	       (unsigned long) (controller->version >> 8),
	       (unsigned long)(unsigned char) (controller->version));

  /* Iterate probing modes.  */
  for (mode_list
	 = (unsigned short *) vbe_far_ptr_to_linear (controller->video_mode);
       *mode_list != 0xFFFF;
       mode_list++)
    {
//      struct vbe_mode mode;			//struct size 255
      
      if (get_vbe_mode_info (*mode_list, mode) != 0x004F)
	continue;

      /* Skip this, if this is not supported or linear frame buffer
	 mode is not support.  */
      if ((mode->mode_attributes & 0x0081) != 0x0081)
	continue;

      if (mode_number == -1 || mode_number == *mode_list)
	{
	  char *model;
	  switch (mode->memory_model)
	    {
	    case 0x00: model = "Text"; break;
	    case 0x01: model = "CGA graphics"; break;
	    case 0x02: model = "Hercules graphics"; break;
	    case 0x03: model = "Planar"; break;
	    case 0x04: model = "Packed pixel"; break;
	    case 0x05: model = "Non-chain 4, 256 color"; break;
	    case 0x06: model = "Direct Color"; break;
	    case 0x07: model = "YUV"; break;
	    default: model = "Unknown"; break;
	    }
	  
	  if (debug > 0)
	    grub_printf ("  0x%x: %s, %ux%ux%u\n",
		       (unsigned long) *mode_list,
		       model,
		       (unsigned long) mode->x_resolution,
		       (unsigned long) mode->y_resolution,
		       (unsigned long) mode->bits_per_pixel);
	  
	  if (mode_number != -1)
	    break;
	}
    }

  if (mode_number != -1 && mode_number != *mode_list)
    if (debug > 0)
      grub_printf ("  Mode 0x%x is not found or supported.\n", (unsigned long)mode_number);
  
  return 1;
}

static struct builtin builtin_vbeprobe =
{
  "vbeprobe",
  vbeprobe_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "vbeprobe [MODE]",
  "Probe VBE information. If the mode number MODE is specified, show only"
  " the information about only the mode."
};
  

int
builtin_cmd (char *cmd, char *arg, int flags)
{
	struct builtin *builtin1 = 0;

	if (cmd == NULL)
	{
		return run_line (arg, flags);
	}

	if (substring(cmd,"exec",1) == 0)
		return command_func(arg, flags);

	builtin1 = find_command (cmd);

	if ((int)builtin1 != -1)
	{
		if (! builtin1 || ! (builtin1->flags & flags))
		{
			errnum = ERR_UNRECOGNIZED;
			return 0;
		}
		else
		{
			return (builtin1->func) (arg, flags);
		}
	}
	else
		return command_func (cmd, flags);
}

static int read_val(char **str_ptr,unsigned long long *val)
{
      char *p;
      char *arg = *str_ptr;
      while (*arg == ' ' || *arg == '\t') arg++;
      p = arg;
      if (*arg == '*') arg++;
      
      if (! safe_parse_maxint_with_suffix (&arg, val, 0))
      {
	 return 0;
      }
      
      if (*p == '*')
      {
	 *val = *((unsigned long long *)(int)*val);
      }
      
      while (*arg == ' ' || *arg == '\t') arg++;
      *str_ptr = arg;
      return 1;
}

static int
calc_func (char *arg, int flags)
{
   unsigned long long val1 = 0;
   unsigned long long val2 = 0;
   unsigned long long *p_result = &val1;
   char O;
   
   if (*arg == '*')
   {
      arg++;
      if (! safe_parse_maxint_with_suffix (&arg, &val1, 0))
      {
	 return 0;
      }
      p_result = (unsigned long long *)(int)val1;
      val1 = *p_result;
      while (*arg == ' ') arg++;
   }
   else
   {
      if (!read_val(&arg, &val1))
      {
	 return 0;
      }
   }

   if ((arg[0] == arg[1]) && (arg[0] == '+' || arg[0] == '-'))
   {
      if (arg[0] == '+')
         (*p_result)++;
      else
         (*p_result)--;
      arg += 2;
      while (*arg == ' ') arg++;
   }

   if (*arg == '=')
   {
      arg++;
      if (! read_val(&arg, &val1))
	 return 0;
   }
   else if (p_result != &val1)
   {
      p_result = &val1;
   }

   while (*arg)
   {
      val2 = 0ULL;
      O = *arg;
      arg++;

      if (O == '>' || O == '<')
      {
	 if (*arg != O)
		 return 0;
	 arg++;
      }
      
      if (! read_val(&arg, &val2))
	 return 0;

      switch(O)
      {
	 case '+':
		 val1 += val2;
		 break;
	 case '-':
		 val1 -= val2;
		 break;
	 case '*':
		 val1 *= val2;
		 break;
	 case '/':
		 val1 = (long)val1 / (long)val2;
		 break;
	 case '%':
		 val1 = (long)val1 % (long)val2;
		 break;
	 case '&':
		 val1 &= val2;
		 break;
	 case '|':
		 val1 |= val2;
		 break;
	 case '^':
		 val1 ^= val2;
		 break;
	 case '<':
		 val1 <<= val2;
		 break;
	 case '>':
		 val1 >>= val2;
		 break;
	 default:
		 return 0;
      }
   }
   
   if (debug > 0)
	  printf(" %ld (HEX:0x%lX)\n",val1,val1);
	if (p_result != &val1)
	   *p_result = val1;
   return val1;
}

static struct builtin builtin_calc =
{
  "calc",
  calc_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "calc [*INTEGER=] [*]INTEGER OPERATOR [[*]INTEGER]",
  "GRUB4DOS Simple Calculator.\n"
  "Available Operators: + - * / % << >> ^ & |"
  "\nNote: this is a Simple Calculator and From left to right only."
};

#ifndef GRUB_UTIL
/* graphicsmode */
static int
graphicsmode_func (char *arg, int flags)
{
#ifdef SUPPORT_GRAPHICS
  extern unsigned long graphics_mode;
  unsigned long long tmp_graphicsmode;
  int old_graphics_mode = graphics_mode;

  if (! *arg)
  {
    if (debug > 0)
	grub_printf (" Current graphics mode setting is 0x%X\n", graphics_mode);
  }
  else if (safe_parse_maxint (&arg, &tmp_graphicsmode))
  {
    if (tmp_graphicsmode != 0x12 && tmp_graphicsmode != 0x6A)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
    if (graphics_mode != (unsigned long)tmp_graphicsmode)
    {
      graphics_mode = tmp_graphicsmode;
       if (graphics_inited)
      {
	 current_term->shutdown();
	 current_term->startup();
      }
      if (debug > 0)
	grub_printf (" Graphics mode number set to 0x%X\n", graphics_mode);
    }
    else
    {
      if (debug > 0)
	grub_printf (" Graphics mode number was already 0x%X\n", graphics_mode);
    }
  }
  else
  {
    if (errnum == 0)
      errnum = ERR_BAD_ARGUMENT;
    return 0;
  }

  return old_graphics_mode;
#else
  return 0x12;
#endif
}

static struct builtin builtin_graphicsmode =
{
  "graphicsmode",
  graphicsmode_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "graphicsmode [0x12 | 0x6A]",
  "Display/set the graphics mode number for the next graphics init."
  "Return the current graphics mode setting."
};
#endif /* ! GRUB_UTIL */

#ifndef GRUB_UTIL
/* init_file */
char menu_init_script_file[32];
static int
initscript_func (char *arg, int flags)
{
	if (grub_strlen(arg) > 32 || ! grub_open (arg))
	{
		return 0;
	}
	grub_close();
	grub_strcpy (menu_init_script_file , arg);
	return 1;
}
 
static struct builtin builtin_initscript =
{
  "initscript",
  initscript_func,
  BUILTIN_MENU,
};

static int echo_func (char *arg,int flags)
{
   int saved_xy = 0;
   int x,y;
   x = y = getxy() & 0xffff;
   x >>= 8;
   y &= 0xff;
	char echo_ec = 0;
   for(;;)
   {
      if (grub_memcmp(arg,"-P:",3) == 0)
      {
	 arg += 3;
	 char c = 0;
	 if (*arg == '-') c=*arg++;
	 y = ((*arg++ - '0') & 15)*10;
	 y += ((*arg++ - '0') & 15);
	 if ( c != '\0' )
	 {
	    y = current_term->max_lines - y;
	    c = '\0';
	 }

	 if (*arg == '-') c = *arg++;
	 
	 x = ((*arg++ - '0') & 15)*10;
	 x += ((*arg++ - '0') & 15);
	 if (c != 0) x = current_term->chars_per_line - x;
	 saved_xy = getxy();
	 gotoxy(x,y);
      }
      else if (grub_memcmp(arg,"-h",2) == 0 )
      {
	 int i,j;
	 printf(" 0 1 2 3 4 5 6 7-L-0 1 2 3 4 5 6 7");
	 for (i=0;i<16;i++)
	 {
	    if (y < current_term->max_lines-1)
	       y++;
	    else
	       putchar('\n');

	    gotoxy(x,y);

	    for (j=0;j<16;j++)
	    {
	       if (j==8)
	       {
	         console_current_color = A_NORMAL;
	         printf(" L ");
	       }
       	       console_current_color = (i << 4) | j;
      	       printf("%02X",console_current_color);
	    }
	 }
	 console_current_color = A_NORMAL;
	 if (saved_xy) gotoxy((saved_xy >> 8) & 0xff,saved_xy & 0xff);//restor cursor
	 return 1;
      }
      else if (grub_memcmp(arg,"-n",2) == 0)
      {
			echo_ec |= 1;
      }
      else if (grub_memcmp(arg,"-e",2) == 0)
      {
			echo_ec |= 2;
      }
      else break;
   	 arg = skip_to (0,arg);
   }

	if (echo_ec & 2)
	{
		flags = parse_string(arg);
		arg[flags] = 0;
	}

   for(;*arg;arg++)
   {
      if (*arg == '$' && arg[1] == '[' && (arg[2] == ']' || arg[6] == ']'))
      {
	 if (arg[2] == ']')
	 {
	    if (current_term->setcolorstate)
	       current_term->setcolorstate (COLOR_STATE_STANDARD);
	 }
	 else
	 {
	    int char_attr = 0;
	    char_attr |= (arg[2] == '0')?0:0x80;
	    char_attr |= (arg[3] == '0')?0:8;
	    char_attr |= ((arg[4] - '0') & 7) << 4;
	    char_attr |= ((arg[5] - '0') & 7);
	    console_current_color = char_attr;
	    arg += 6;
	 }
      }
      else
      {
	    grub_putchar(*arg);
      }
   }

	if ((echo_ec & 1) == 0)
	{
		grub_putchar('\r');
		grub_putchar('\n');
	}
   if (current_term->setcolorstate)
	  current_term->setcolorstate (COLOR_STATE_STANDARD);
   if (saved_xy) gotoxy((saved_xy >> 8) & 0xff,saved_xy & 0xff);//restor cursor
   return 1;
}
static struct builtin builtin_echo =
{
   "echo",
   echo_func,
   BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT,
};
#endif

static int if_func(char *arg,int flags)
{
	char *str1,*str2;
	int cmp_flag = 0;
	int ret = 0;
	while(*arg)
	{
		if (substring("/i ", arg, 1) == -1)
			cmp_flag |= 1;
		else if(substring("not ", arg, 1) == -1)
			cmp_flag |= 4;
		else if (substring("exist ", arg, 1) == -1)
			cmp_flag |= 2;
		else
			break;
		arg = skip_to (0, arg);
	}
	if (*arg == '\0')
		return 0;
	if (cmp_flag & 2)
	{
		if (*arg < '@')
		{
			int no_decompression_bak = no_decompression;
			no_decompression = 1;
			ret = grub_open(arg);
			grub_close();
			errnum = 0;
			no_decompression = no_decompression_bak;
		}
		else
			ret = envi_cmd(arg,NULL,1);
		arg = skip_to(0,arg);
	}
	else
	{
		str1 = arg;
		str2 = arg = skip_to(1,arg);
		arg -= 2;
		if (*(unsigned short *)arg != 0x3D3D)
		{
			errnum = ERR_BAD_ARGUMENT;
			return 0;
		}
		skip_to (SKIP_WITH_TERMINATE | 1,str1);
		arg = skip_to (SKIP_WITH_TERMINATE,str2);
		ret = (substring(str1,str2,cmp_flag & 1) == 0);
	}

	if (ret ^ (cmp_flag >> 2))
	{
		return *arg?builtin_cmd(arg,skip_to(0,arg),flags):1;
	}
	return 0;
}

static struct builtin builtin_if =
{
   "if",
   if_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "if [/i] [not] STRING1==STRING2 [COMMAND]",
  "if [NOT] exist VARIABLE|FILENAME [COMMAND]"
};

#define MAX_USER_VARS 60
#define MAX_VARS 64
#define MAX_VAR_LEN	8
#define MAX_ENV_LEN	512
#define MAX_BUFFER	(MAX_VARS * (MAX_VAR_LEN + MAX_ENV_LEN))
#define BASE_ADDR 0x45000
#define VAR ((char (*)[MAX_VAR_LEN])BASE_ADDR)
#define ENVI ((char (*)[MAX_ENV_LEN])(BASE_ADDR + MAX_VARS * MAX_VAR_LEN))
#define _WENV_ 60
#define WENV_RANDOM (*(unsigned long *)(ENVI[_WENV_]+0x20))
#define QUOTE_CHAR (*(ENVI[_WENV_]+0x30))
#define WENV_TMP (ENVI[_WENV_]+0x40)
#define set_envi(var, val)			envi_cmd(var, val, 0)
//#define get_env(var, val)			envi_cmd(var, val, 1)
#define get_env_all()				envi_cmd(NULL, NULL, 2)
#define reset_env_all()				envi_cmd(NULL, NULL, 3)
/*
flags:
0 add or set
1 read	if env is NULL,return true when the variable var is exist.
2 show
3 reset
*/
int envi_cmd(const char *var,char * const env,int flags)
{
	if(flags == 3)
	{
		memset( (char *)BASE_ADDR, 0, MAX_BUFFER );
		sprintf(VAR[_WENV_], "?_WENV");
		sprintf(VAR[63], "?_WENV");
		QUOTE_CHAR = '\"';
		return 1;
	}

	int i, j;

	if (flags == 2)
	{
		int count=0;
		for(i=0; i < MAX_USER_VARS && VAR[i][0]; ++i)
		{
			if (VAR[i][0] < 'A')
				continue;
			if (var == NULL || substring(var,VAR[i],0) < 1 )
			{
				++count;
				printf("%.8s=%.60s\n",VAR[i],ENVI[i]);
			}
		}
		return count;
	}

	char ch[MAX_VAR_LEN +1] = "\0\0\0\0\0\0\0\0";
	char *p = (char *)var;
	int ou_start = 0;
	int ou_len = 0x200;
	if (*p == '%')
		p++;
	for (i=0;i<=MAX_VAR_LEN && (unsigned char)*p >='.';i++)
	{
		if (*p == '^')
			break;
		if (*(short*)p == 0x7E3A)//:~
		{
			unsigned long long t;
			p += 2;
			ou_start = safe_parse_maxint(&p,&t)?(int)t:0;
			if (*p == ',')
			{
				++p;
				ou_len = safe_parse_maxint(&p,&t)?(int)t:0;
			}
			break;
		}
		ch[i] = *p++;
	}
	if (flags == 4)
	{
		return (*p == '^' || *p== '%')?p-var:0;
	}
	if (*p && (flags != 1 || (*var == '%' && *p != '%')))
		return 0;

	/*
	i >= 60  system variables.
	'@' 	 Built-in variables or deleted.
	*/
	if (ch[0]=='@')
	{
		if (flags != 1)
			return 0;
		p = WENV_TMP;
		#ifndef GRUB_UTIL
		unsigned long date,time;
		get_datetime(&date, &time);

		if (substring(ch,"@date",1) == 0)
		{
			sprintf(p,"%04X-%02X-%02X",(date >> 16),(char)(date >> 8),(char)date);
		}
		else if (substring(ch,"@time",1) == 0)
		{
			sprintf(p,"%02X:%02X:%02X",(char)(time >> 24),(char)(time >> 16),(char)(time>>8));
		}
		else if (substring(ch,"@random",1) == 0)
		{
			WENV_RANDOM   =  (WENV_RANDOM * date + (*(int *)0x46c)) & 0x7fff;
			sprintf(p,"%d",WENV_RANDOM);
		}
		else if (substring(ch,"@root",1) == 0)
		{
			print_root_device(p,0);
			sprintf(p+strlen(p),saved_dir);
		}
		else if (substring(ch,"@path",1) == 0)
		{
			p = command_path;
		}
		else if (substring(ch,"@retval",1) == 0)
			sprintf(p,"%d",*(int*)0x4CB00);
		else
			return 0;
		j = i;
		#else
		j = 0xff;
		#endif
	}
	else 
	{
		if (ch[0] == '?' && (ch[1] == 0 || memcmp(ch,"?_UUID",7) == 0))
			j = i = _WENV_;
		else
		{
			j = 0xFF;
			for(i=(ch[0]=='?')?60:0;i < MAX_VARS && VAR[i][0];i++)
			{
				if (memcmp(VAR[i], ch, MAX_VAR_LEN) == 0)
				{
					j = i;
					break;
				}
				if (j == 0xFF && VAR[i][0] == '@') j = i;
			}
		}
		p = ENVI[i];
	}
	
	if (flags == 1)
	{
		if (j!=i)
			return 0;
		if (env == NULL)
			return 1;
		for(j=0;j<512 && p[j];j++)
		{
			;
		}
		if (ou_start < 0)
		{
			if (-ou_start < j)
			{
				ou_start += j;
			}
			else
			{
				ou_start = 0;
			}
		}
		else if (j - ou_start < 0)
			ou_start = j;
		j -= ou_start;
		if (ou_len < 0)
		{
			if (-ou_len <j)
				ou_len += j;
			else
				ou_len=0;
		}
		return sprintf(env,"%.*s",ou_len,p + ou_start);
	}
	//flags = 0 set/del variables 
	if (j == 0xFF && i >= MAX_VARS)//not variable space
	{
		return 0;
	}
	if (env == NULL || env[0] == '\0')//del
	{
		if(j == i)
		{
			VAR[i][0] = '@';
		}
		return 1;
	}

	if (j != i)
	{
		i = (j<i?j:i);
		memmove(VAR[i] ,ch ,MAX_VAR_LEN);
	}
	return sprintf(ENVI[i],"%.512s",env);
}

static void case_convert(char *ch,int flag)
{
	if (flag != 'a' && flag != 'A')
		return;
	while (*ch)
	{
		if ((unsigned char)(*ch-flag) < 26)
		{
			*ch ^= 0x20;
		}
		++ch;
	}
}

static int set_func(char *arg, int flags)
{
	if( *arg == '*' )
		return reset_env_all();
	else if ((strcmp(VAR[_WENV_], "?_WENV") != 0 && strcmp(VAR[63], "?_WENV") != 0))
		reset_env_all();
	if (*arg == '@')
		return 0;
	char value[512];
	int convert_flag=0;
	unsigned long long wait_t = 0xffffff00;
	while (*arg)
	{
		flags = *(short *)arg;
		if (flags == 0x612F) // set /a
		{
			convert_flag |= 0x100;
		}
		else if (flags == 0x412F) // set /A
		{
			convert_flag |= 0x500;
		}
		else if (flags == 0x702F) /* set /p */
		{
			convert_flag |= 0x200;
			if (arg[2] == ':')
			{
				char *p = arg + 3;
				safe_parse_maxint(&p,&wait_t);
				errnum = 0;
				wait_t <<= 8;
			}
		}
		else if (flags == 0x6C2F) /* set /l */
		{
			convert_flag |= 'A';
		}
		else if (flags == 0x752F) /* set /u */
		{
			convert_flag |= 'a';
		}
		else
			break;
		arg = skip_to(0, arg);
	}

	if ((unsigned char)*arg < '.')
		return get_env_all();
	char *var = arg;
	flags = strstr(var,"=")?0:2;
	arg = skip_to(SKIP_WITH_TERMINATE | 1,arg);
	if (convert_flag & 0x200)
	{
		value[0] = 0;
		get_cmdline_str.prompt = arg;
		get_cmdline_str.maxlen = sizeof (value) - 1;
		get_cmdline_str.echo_char = 0;
		get_cmdline_str.readline = 1 | wait_t;
		get_cmdline_str.cmdline = value;
		if (get_cmdline (get_cmdline_str) || !value[0])
			return 0;
		arg = value;
	}
	if (convert_flag & 0x100)
	{
		if (convert_flag & 0x400)
			sprintf(value,"0x%X",calc_func(arg,flags));
		else
			sprintf(value,"%d",calc_func(arg,flags));
		errnum = 0;
		arg = value;
	}

	if (*arg)
	{
		case_convert(arg,convert_flag&0xff);
		flags = 0;
	}
	skip_to(SKIP_LINE,arg);
	return envi_cmd(var,arg,flags);
}

static struct builtin builtin_set =
{
   "set",
   set_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT | BUILTIN_HELP_LIST,
  "set [/p] [/a|/A] [/l|/u] [VARIABLE=[STRING]]",
  "/p,Get a line of input;l|/u,lower/upper case;/a|/A,numerical expression that is evaluated(use calc)."
  "/a,set value to a Decimal;/A  to a HEX."
};

typedef struct _SETLOCAL {
	char var_name[480];//user var_names.
	struct _SETLOCAL *prev;
	unsigned long saved_drive;
	unsigned long saved_partition;
	unsigned long boot_drive;
	unsigned long install_partition;
	int debug;
	char reserved[8];//预留位置，同时也是为了凑足512字节。
	char var_str[MAX_USER_VARS<<9];//user vars only
	char saved_dir[256];
	char command_path[128];
} SETLOCAL;
static SETLOCAL *bc = NULL;
static SETLOCAL *cc = NULL;
static SETLOCAL *sc = NULL;

static int setlocal_func(char *arg, int flags)
{
	SETLOCAL *saved;
	if (*arg == '0')
		return printf("0x%X\n",cc);
	if ((saved=grub_malloc(sizeof(SETLOCAL)))== NULL)
		return 0;
	/* Create a copy of the current user environment */
	memmove(saved->var_name,(char *)BASE_ADDR,(MAX_USER_VARS + 1)<<9);
	sprintf(saved->saved_dir,saved_dir);
	sprintf(saved->command_path,command_path);
	saved->prev = cc;
	saved->saved_drive = saved_drive;
	saved->saved_partition = saved_partition;
	saved->boot_drive = boot_drive;
	saved->install_partition = install_partition;
	saved->debug = debug;
	cc = saved;
	if (*arg == '@')
	{
		if (flags & BUILTIN_BAT_SCRIPT)
		{
			saved = saved->prev;
			while (saved != bc)
			{
				sc = saved;
				saved = saved->prev;
				grub_free(sc);
			}
			cc->prev = bc;
			bc = cc;
		}
		sc = cc;
	}
	return 1;
}
static struct builtin builtin_setlocal =
{
   "setlocal",
   setlocal_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT,
  "setlocal",
};


static int endlocal_func(char *arg, int flags)
{
	SETLOCAL *saved = cc;
	if (*arg == '@')
	{
		sc = NULL;
	}
	if (cc == bc || cc == sc)
	{
		return 0;
	}

	/* Restore variables from the copy saved by setlocal_func */
	memmove(VAR[0],saved->var_name,MAX_USER_VARS<<3);
	memmove(ENVI[0],saved->var_str,MAX_USER_VARS<<9);
	sprintf(saved_dir,saved->saved_dir);
	sprintf(command_path,saved->command_path);
	saved_drive = saved->saved_drive;
	saved_partition = saved->saved_partition;
	boot_drive = saved->boot_drive;
	install_partition = saved->install_partition;
	debug = saved->debug;
	cc = cc->prev;
	grub_free(saved);
	return 1;
}
static struct builtin builtin_endlocal =
{
   "endlocal",
   endlocal_func,
  BUILTIN_MENU | BUILTIN_CMDLINE | BUILTIN_SCRIPT,
  "endlocal",
};

struct bat_label
{
	char *label;
	int line;
};

/*
bat_pid is current running batch script id.
it must the same of p_bat_prog->pid.
the first batch bat_pid is 1,max 10.so we can run 10 of batch script one time.
when all batch script is exit the bat_pid is 0;
*/
struct bat_array
{
	int pid;
	struct bat_label *entry;
	/*
	 (char **)(entry + 0x80) is the entry of bat_script to run.
	*/
	char *path;
} *p_bat_prog = NULL;
int bat_pid = 0;
static char **batch_args;
/*
find a label in current batch script.(p_bat_prog)
return line of the batch script.
if not find ,return -1;
*/
static int bat_find_label(char *label)
{
	struct bat_label *label_entry;
	if (*label == ':') label++;
	nul_terminate(label);
	for (label_entry = p_bat_prog->entry; label_entry->label ; label_entry++)//find label for goto/call.
	{
		if (substring(label_entry->label,label,1) == 0)
		{
			return label_entry->line;
		}
	}

	if (debug)
		printf(" cannot find the batch label specified - %s\n",label);
	return 0;
}

static int bat_get_args(char *arg,char *buff,int flags)
{
	char *p = ((char *)0x4CB08);
	char *s1 = buff;

	if (*arg == '(')
	{
		unsigned long tmp_partition = current_partition;
		unsigned long tmp_drive = current_drive;
		set_device (arg);
		print_root_device(p,1);
		current_partition = tmp_partition;
		current_drive = tmp_drive;
		p += strlen(p);
		while (*arg++ != ')')
			;
	}
	else if (flags & 0xff) // if is Param 0
	{
		p += sprintf(p,"%s",p_bat_prog->path) - 1;//use program run dir
	}
	else
	{
		print_root_device(p,0);
		p += strlen(p);
		p += sprintf(p,saved_dir);
	}
	if (*arg != '/')
		*p++ = '/';
	if (p + strlen(arg) >= (char *)0x4CC00)
		goto quit;
	sprintf(p,"%s",arg);
	p = ((char *)0x4CB08);
	flags >>= 8;
	if (flags & 0x10)
	{
		if (grub_open(p))
		{
			buff += sprintf(buff,"0x%lx",filemax);
			grub_close();
		}
		errnum = 0;
		flags &= 0xf;
	}

	if (flags == 0x2f)
	{
		buff += sprintf(buff, p);
		goto quit;
	}

	if (flags & 1)
	{
		while ( *p && *p != '/')
			*buff++ = *p++;
	}

	if (! (p = strstr(p,"/")))
		goto quit;
	char *p0,*p1,*p2 = NULL;
	p0 = p1 = p;

	while (*p)
	{
		if (*p++ == '/')
			p1 = p;
		if (*p == '.')
			p2 = p;
	}

	if (p2 < p1)
		p2 = p;

	if (flags & 2)
	{
		buff += sprintf(buff, "%.*s",p1 - p0,p0);
	}

	if (flags & 4)
	{
		buff += sprintf(buff,"%.*s",p2 - p1,p1);
	}

	if (flags & 8)
	{
		buff += sprintf(buff,p2);
	}

quit:
	return buff-s1;
}
/*
bat_run_script
run batch script.
if filename is NULL then is a call func.the first word of arg is a label.
*/
static int bat_run_script(char *filename,char *arg,int flags)
{
	if (bat_pid != p_bat_prog->pid)
	{
		errnum = ERR_FUNC_CALL;
		return 0;
	}

	char **bat_entry = (char **)(p_bat_prog->entry + 0x80);

	int i = 1;

	if (filename == NULL)
	{//filename is null is a call func;
		filename = arg;
		arg = skip_to(SKIP_WITH_TERMINATE | 1,arg);
		if ((i = bat_find_label(filename)) == 0)
			return 0;
	}

	char **p_entry = bat_entry + i;

	char *s[10];
	char *p_cmd;
	char *p_rep;
	char *p_buff;//buff for command_line
	char *cmd_buff;
	if ((cmd_buff = grub_malloc(0x1000)) == NULL)
	{
		return 0;
	}

	/*copy filename to buff*/
	i = grub_strlen(filename);
	grub_memmove(cmd_buff,filename,i+1);
	p_buff = cmd_buff + ((i+16) & ~0xf);
	s[0] = cmd_buff;
	/*copy arg to buff*/
	i = grub_strlen(arg);
	grub_memmove(p_buff, arg, i+1);
	arg = p_buff;
	p_buff = p_buff + ((i+16) & ~0xf);

	/*build args %1-%9*/
	for (i = 1;i < 9;i++)
	{
		s[i] = arg;
		if (*arg)
			arg = skip_to(SKIP_WITH_TERMINATE | 1,arg);
	}
	s[9] = arg;// %9 for other args.
	int ret = 0;
	char *p_bat;
	char **backup_args = batch_args;
	SETLOCAL *saved_bc = bc;
	batch_args = s;
	bc = cc; //saved for batch
	while ((p_bat = *p_entry))//copy cmd_line to p_buff and then run it;
	{
		p_cmd = p_buff;
		char *file_ext;
		while(*p_bat)
		{
			if (*p_bat != '%' || (file_ext = p_bat++,*p_bat == '%'))
			{//if *p_bat != '%' or p_bat[1] == '%'(*p_bat == p_bat[1] == '%');
				*p_cmd++ = *p_bat++;
				continue;
			}//file_ext now use for backup p_bat see the loop end.

			i = 0;

			if (*p_bat == '~')
			{
				p_bat++;
				i |= 0x20;
				while (*p_bat)
				{
					if (*p_bat == 'd')
						i |= 1;
					else if (*p_bat == 'p')
						i |= 2;
					else if (*p_bat == 'n')
						i |= 4;
					else if (*p_bat == 'x')
						i |= 8;
					else if (*p_bat == 'f')
						i |= 0xf;
					else if (*p_bat == 'z')
						i |= 0x10;
					else
						break;
					p_bat++;
				}
			}

			if (*p_bat <= '9' && *p_bat >= '0')
			{
				p_rep = s[*p_bat - '0'];
				if (*p_rep)
				{
					int len_c = 0;
					if ((i & 0x20) && *p_rep == '\"')
					{
						p_rep++;
					}
					if (i & 0x1f)
					{
						len_c = bat_get_args(p_rep,p_cmd,i << 8 | (s[*p_bat - '0'] == cmd_buff));
					}
					else
					{
						len_c = sprintf(p_cmd,p_rep);
					}

					if (len_c)
					{
						if ((i & 0x20) && p_cmd[len_c-1] == '\"')
						--len_c;
						p_cmd += len_c;
					}
				}
			}
			else if (*p_bat == '*')
			{
				for (i = 1;i< 10;++i)
				{
					if (s[i][0])
						p_cmd += sprintf(p_cmd,"%s ",s[i]);
					else
						break;
				}
			}
			else
			{
				p_bat = file_ext;
				*p_cmd++ = *p_bat;
			}
			++p_bat;
		}

		*p_cmd = '\0';
		if (bat_script_debug)
		{
			printf("%s\n",p_buff);
			char key=getkey() & 0xdf;
			if (key == 'Q')
			{
				errnum = 1001;
				break;
			}
			else if (key == 'C')
			{
				commandline_func((char *)0x1000000,0);
			}
		}

		ret = run_line (p_buff,flags);

		if ((*(short *)0x417 & 0x104) && checkkey() == 0x2E03)
		{
			getkey();
			char k;
			loop_yn:
			grub_printf("\nTerminate batch job (Y/N)? ");
			k = getkey() & 0xDF;
			putchar(k);
			if (k == 'Y')
			{
					errnum = 1255;
					break;
			}
			if (k != 'N')
				goto loop_yn;
		}

		if (errnum == ERR_BAT_GOTO)
		{
			if (ret == 0)
				break;
			p_entry = bat_entry + ret;
			errnum = ERR_NONE;
			continue;
		}
		if ((unsigned int)errnum >= 1000 || (errorcheck && errnum))
		{
			break;
		}
		p_entry++;
	}
	i = errnum; //save errnum.
	sprintf(ADDR_RET_STR,"%d",i);
	/*release memory. */
	while (bc != cc) //restore SETLOCAL
		endlocal_func(NULL,1);
	bc = saved_bc;
	batch_args = backup_args;
	grub_free(cmd_buff);
	errnum = (i == 1000) ? 0 : i;
	return errnum?0:1;
}


static int goto_func(char *arg, int flags)
{
	errnum = ERR_BAT_GOTO;
	if (flags & BUILTIN_BAT_SCRIPT)//batch script return arg addr.
	{
		return bat_find_label(arg);
	}
	else
		return fallback_func(arg,flags);//in menu script call fallback_func to jump next menu.
}

static struct builtin builtin_goto =
{
   "goto",
   goto_func,
   BUILTIN_SCRIPT | BUILTIN_BAT_SCRIPT,
};

static int call_func(char *arg,int flags)
{
	errnum = ERR_BAT_CALL;
	if (flags & BUILTIN_BAT_SCRIPT)
		return bat_run_script(NULL, arg, flags);
	else
		return 0;
}

static struct builtin builtin_call =
{
   "call",
   call_func,
  BUILTIN_BAT_SCRIPT,
};

static int exit_func(char *arg, int flags)
{
	unsigned long long t = 0;
	read_val(&arg, &t);
	errnum = 1000 + t;
	return errnum;
}

static struct builtin builtin_exit =
{
  "exit",
  exit_func,
  BUILTIN_BAT_SCRIPT,
  "exit [n]",
  "Exit batch script with a status of N."
};

static int shift_func(char *arg, int flags)
{
	char **s = batch_args;
	if (*arg == '/')
		++arg;
	unsigned int i = *arg - '0';
	if (i > 8)
		i = 0;
	while (i < 9 && s[i][0])
	{
		s[i] = s[i+1];
		++i;
	}
	if (i == 9)
	{
		s[9] = skip_to(SKIP_WITH_TERMINATE | 1,s[8]);
	}
	return 1;
}

static struct builtin builtin_shift =
{
  "shift",
  shift_func,
  BUILTIN_BAT_SCRIPT,
  "shift [[/]n]",
  "The positional parameters from %n+1 ... are renamed to %1 ...  If N is"
  " not given, it is assumed to be 0."
};

static int grub_exec_run(char *program, int flags)
{
	int pid;
	char *arg = program - (*(unsigned long *)(program - 8));
		/* kernel image is destroyed, so invalidate the kernel */
	if (kernel_type < KERNEL_TYPE_CHAINLOADER)
		kernel_type = KERNEL_TYPE_NONE;

	/*Is a batch file? */
	if (*(unsigned long *)program == BAT_SIGN || *(unsigned long *)program == 0x21BFBBEF)//!BAT
	{
		if (bat_pid >= 10)
		{
			return 0;
		}
		struct bat_array *p_bat_array = (struct bat_array *)grub_malloc(0x2600);
		if (p_bat_array == NULL)
			return 0;
		p_bat_array->path = program - (*(unsigned long *)(program - 24));
		struct bat_array *p_bat_array_orig = p_bat_prog;

		char *filename = program - (*(unsigned long *)(program - 16));
		char *p_bat;
		struct bat_label *label_entry =(struct bat_label *)((char *)p_bat_array + 0x200);
		char **bat_entry = (char **)(label_entry + 0x80);//0x400/sizeof(label_entry)
		unsigned long i_bat = 1,i_lab = 1;//i_bat:lines of script;i_lab=numbers of label.

		program = skip_to(SKIP_LINE,program);//skip head
		while ((p_bat = program))//scan batch file and make label and bat entry.
		{
			program = skip_to(SKIP_LINE,program);
			if (*p_bat == ':')
			{
				nul_terminate(p_bat);
				label_entry[i_lab].label = p_bat + 1;
				label_entry[i_lab].line = i_bat;
				i_lab++;
			}
			else
				bat_entry[i_bat++] = p_bat;
			if ((i_lab & 0x80) || (i_bat & 0x800))//max label 128,max script line 2048.
			{
				grub_free(p_bat_array);
				return 0;
			}
		}

		label_entry[i_lab].label = NULL;
		bat_entry[i_bat] = NULL;
		label_entry[0].label = "eof";
		label_entry[0].line = i_bat;
		p_bat_array->pid = ++bat_pid;
		p_bat_array->entry = label_entry;
		p_bat_prog = p_bat_array;
		flags |= BUILTIN_BAT_SCRIPT;
		pid = bat_run_script(filename, arg,flags);//run batch script from line 0;

		bat_pid--;
		p_bat_prog = p_bat_array_orig;
		grub_free(p_bat_array);
		return pid;
	}

	/* call the new program. */
	pid = ((int (*)(char *,int))program)(arg, flags | BUILTIN_USER_PROG);/* pid holds return value. */
	return pid;
}

/* The table of builtin commands. Sorted in dictionary order.  */
struct builtin *builtin_table[] =
{
#ifdef SUPPORT_GRAPHICS
  &builtin_background,
#endif
  &builtin_blocklist,
#ifndef GRUB_UTIL
  &builtin_boot,
#ifdef SUPPORT_NETBOOT
  &builtin_bootp,
#endif /* SUPPORT_NETBOOT */
#endif /* ! GRUB_UTIL */
  &builtin_calc,
  &builtin_call,
  &builtin_cat,
#if ! defined(GRUB_UTIL) && ! defined (STAGE1_5)
  &builtin_cdrom,
#endif
  &builtin_chainloader,
  &builtin_checkrange,
#ifndef GRUB_UTIL
  &builtin_checktime,
#endif
  &builtin_clear,
  &builtin_cmp,
  &builtin_color,
  &builtin_command,
  &builtin_commandline,
  &builtin_configfile,
#ifndef GRUB_UTIL
  &builtin_dd,
#endif
  &builtin_debug,
  &builtin_default,
  &builtin_delmod,
#ifdef GRUB_UTIL
  &builtin_device,
#endif /* GRUB_UTIL */
#ifdef SUPPORT_NETBOOT
  &builtin_dhcp,
#endif /* SUPPORT_NETBOOT */
//  &builtin_displayapm,
  &builtin_displaymem,
#ifdef GRUB_UTIL
  &builtin_dump,
#endif /* GRUB_UTIL */
#ifndef GRUB_UTIL
  &builtin_echo,
#endif
#ifdef GRUB_UTIL
  &builtin_embed,
#endif /* GRUB_UTIL */
  &builtin_endlocal,
  &builtin_errnum,
  &builtin_errorcheck,
  &builtin_exit,
  &builtin_fallback,
  &builtin_find,
#ifdef SUPPORT_GRAPHICS
  &builtin_foreground,
#endif
  &builtin_fstest,
  &builtin_geometry,
#ifdef SUPPORT_GFX
  &builtin_gfxmenu,
#endif
  &builtin_goto,
#ifndef GRUB_UTIL
  &builtin_graphicsmode,
#endif /* ! GRUB_UTIL */
  &builtin_halt,
  &builtin_help,
  &builtin_hiddenflag,
  &builtin_hiddenmenu,
  &builtin_hide,
  &builtin_if,
#ifdef SUPPORT_NETBOOT
  &builtin_ifconfig,
#endif /* SUPPORT_NETBOOT */
//  &builtin_impsprobe,
  &builtin_initrd,
#ifndef GRUB_UTIL
  &builtin_initscript,
#endif
  &builtin_insmod,
#ifdef GRUB_UTIL
  &builtin_install,
#endif /* GRUB_UTIL */
//  &builtin_ioprobe,
  &builtin_is64bit,
  &builtin_kernel,
  &builtin_lock,
#ifndef STAGE1_5
  &builtin_ls,
#endif /* STAGE1_5 */
  &builtin_makeactive,
#ifndef GRUB_UTIL
  &builtin_map,
#endif /* ! GRUB_UTIL */
#ifdef USE_MD5_PASSWORDS
  &builtin_md5crypt,
#endif /* USE_MD5_PASSWORDS */
  &builtin_module,
  &builtin_modulenounzip,
#ifdef SUPPORT_GRAPHICS
  &builtin_outline,
#endif /* SUPPORT_GRAPHICS */
  &builtin_pager,
  &builtin_partnew,
  &builtin_parttype,
  &builtin_password,
  &builtin_pause,
#ifdef FSYS_PXE
  &builtin_pxe,
#endif
//#ifdef GRUB_UTIL
  &builtin_quit,
//#endif /* GRUB_UTIL */
#ifndef GRUB_UTIL
#ifdef SUPPORT_NETBOOT
  &builtin_rarp,
#endif /* SUPPORT_NETBOOT */
  &builtin_read,
  &builtin_reboot,
#endif /* ! GRUB_UTIL */
  &builtin_root,
  &builtin_rootnoverify,
  &builtin_savedefault,
#ifdef SUPPORT_SERIAL
  &builtin_serial,
#endif /* SUPPORT_SERIAL */
  &builtin_set,
  &builtin_setkey,
  &builtin_setlocal,
#ifdef GRUB_UTIL
  &builtin_setup,
#endif /* GRUB_UTIL */
  &builtin_setvbe,
  &builtin_shift,
#ifdef SUPPORT_GRAPHICS
  &builtin_splashimage,
#endif /* SUPPORT_GRAPHICS */
#if defined(SUPPORT_SERIAL) || defined(SUPPORT_HERCULES) || defined(SUPPORT_GRAPHICS)
  &builtin_terminal,
#endif /* SUPPORT_SERIAL || SUPPORT_HERCULES SUPPORT_GRAPHICS */
#ifdef SUPPORT_SERIAL
  &builtin_terminfo,
#endif /* SUPPORT_SERIAL */
//  &builtin_testload,
  &builtin_testvbe,
#ifdef SUPPORT_NETBOOT
  &builtin_tftpserver,
#endif /* SUPPORT_NETBOOT */
  &builtin_timeout,
  &builtin_title,
#if ! defined(GRUB_UTIL) && ! defined (STAGE1_5)
  &builtin_tpm,
#endif /* ! defined(GRUB_UTIL) && ! defined (STAGE1_5) */
  &builtin_unhide,
//  &builtin_uppermem,
  &builtin_uuid,
  &builtin_vbeprobe,
#ifndef GRUB_UTIL
  &builtin_write,
#endif /* ! GRUB_UTIL */
  0
};
