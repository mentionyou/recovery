#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <ext2fs/ext2fs.h>
#include <ext2fs/ext2_io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <assert.h>		
#include <libgen.h>

#define o_O(msg) \
do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define BUF_SIZE 1024

#define TRUE 1
#define FALSE 0


struct found
{

  int header_found, footer_found;
  unsigned long header_blk;
  unsigned long footer_blk;
  char dotpart[4];

};

int EXT2_BLOCK_SIZE;
int DIRECT_BLKS=12;

void do_dump_unused ();

char restore_device_dir[100];
char device[75];

int main (int argc, char *argv[])
{
  ext2_filsys current_fs = NULL;
  int open_flags = EXT2_FLAG_SOFTSUPP_FEATURES;	
  blk_t superblock = 0;
  blk_t blocksize = 0;
  int sign = 0;
  int i = 0;
  int ans = 0;

//  printf ("\nPlease enter the device name:");
//  scanf ("%s", device);
//  printf ("\n Please enter the output directory - (You must provide  separate partition/drive\'s directory :");
//  scanf ("%s", restore_device_dir);

	strcpy(restore_device_dir,"/home/drc/Documents/recover");
	strcpy(device,"/dev/sdb");

  sign = ext2fs_open (device, open_flags, superblock, blocksize, unix_io_manager,&current_fs);

  if (sign)
    {
      current_fs = NULL;
      o_O ("Error while opening filesystem.");
    }

  sign = ext2fs_read_inode_bitmap (current_fs);

  if (sign)
    {
      o_O ("Error while reading inode bitmap");
    }

  sign = ext2fs_read_block_bitmap (current_fs);

  if (sign)
    {
      o_O ("Error while reading block bitmap");
    }


  EXT2_BLOCK_SIZE = current_fs->blocksize;

      do_dump_unused (current_fs);

  ext2fs_close (current_fs);
  return 1;
}


void do_dump_unused (ext2_filsys current_fs)
{
  unsigned long blk, last_searched_blk;
  unsigned char buf[EXT2_BLOCK_SIZE];
  unsigned int i;

  errcode_t sign, f_sign;

  struct found needle = { 0 };

  for (blk = current_fs->super->s_first_data_block;
       blk < current_fs->super->s_blocks_count; blk++)
    {
      if (ext2fs_test_block_bitmap (current_fs->block_map, blk))
	continue;
      sign = io_channel_read_blk (current_fs->io, blk, 1, buf);
      if (sign)
	{
	  o_O ("Error while reading block");
	  return;
	}

      for (i = 0; i < current_fs->blocksize; i++)
	if (buf[i])
	  break;
      //      if (i >= current_fs->blocksize){
      //              printf("\nUnused block %lu contains zero data:\n\n",blk);
      //              continue;
      //              }

      printf
	("\nSearching Unused block %lu which contains non-zero data:\n\n",
	 blk);

      if ((blk - last_searched_blk) != 1)
	{
	  //some blks are skipped b'cause they are in-use or empty.so reset.
	  memset ((void *) &needle, '\0', sizeof (struct found));
	}

      sign = find_header (buf, &needle, blk);
      last_searched_blk = blk;

      if (sign == 1 || needle.header_found == 1)
	{		
		sleep(1000);	
	  f_sign = find_footer (buf, &needle, blk);
	  if (f_sign == -1)
	    {
	      //footer mismatch.reset.

	      memset ((void *) &needle, '\0', sizeof (struct found));
	    }
	  else if (f_sign == 1)
	    {
	      printf ("perfect match.recover from %lu to %lu",
		      needle.header_blk, needle.footer_blk);
		
	     write_back (current_fs, &needle);

	      memset ((void *) &needle, '\0', sizeof (struct found));
	    }
	  else
{
	    printf ("No footer at all");

}
	  if ((blk - needle.header_blk) >= DIRECT_BLKS)
	    {
	      //unable to find footer in DIRECT_BLKS blks so reset
	      memset ((void *) &needle, '\0', sizeof (struct found));
	    }

	  //for (i=0; i < current_fs->blocksize; i++)
	  //      fputc(buf[i], stdout);
	}
      else if (sign == -1)
	{
	  //header found again. reset.
	  memset ((void *) &needle, '\0', sizeof (struct found));
	}
      else
	printf ("no header found.at all");
    }
}

int write_back (ext2_filsys current_fs, struct found *needle)
{
  unsigned long blk;
  unsigned char buf[EXT2_BLOCK_SIZE];
  unsigned int i;
  errcode_t sign, f_sign;
  char fname[] = "extcarveXXXXXX";
  char tmp_dname[100];
  int temfd;
 
  temfd = mkstemp (fname);
  close (temfd);
  unlink (fname);
  strcpy (tmp_dname, restore_device_dir);

  strcat (tmp_dname, "/");
  strcat (tmp_dname, fname);
  strcat (tmp_dname, needle->dotpart);
  temfd=open(tmp_dname,O_WRONLY|O_CREAT|O_TRUNC);
  // temfd = creat (tmp_dname, 0777);
  if (temfd < 0)
    {
      printf
	("Please check permission for destination directory %s.Something wrong",
	 restore_device_dir);
      exit (0);
    }

   //printf("\nRestording files dir : %s %u %u",restore_device_dir,needle->header_blk,needle->footer_blk);

  for (blk = needle->header_blk; blk <= needle->footer_blk; blk++)
    {
      sign = io_channel_read_blk (current_fs->io, blk, 1, buf);
      if (sign)
	{

	  o_O ("Error while reading block");
	  return;
	}
      write (temfd, buf, EXT2_BLOCK_SIZE);
	

    }
  close (temfd);

  return 1;
}

int find_header (unsigned char buf[EXT2_BLOCK_SIZE], struct found *needle, unsigned long blk)
{
  //gif 
  	if (buf[0] == 0x47 && buf[1] == 0x49 && buf[2] == 0x46 && buf[3] == 0x38)
    	{
      		printf ("gif header found!!!");
		sleep(1);
      		if (needle->header_found != 1)
		{
	  		needle->header_found = 1;
	  		needle->header_blk = blk;
	  		strcpy (needle->dotpart, ".gif");
	  		return 1;
		}
      	needle->header_found = -1;
      	return -1;
    	}

  //png 
  	if (	buf[0] == 0x89 && buf[1] == 0x50 && buf[2] == 0x4E && buf[3] == 0x47
      		&& buf[4] == 0x0D && buf[5] == 0x0A && buf[6] == 0x1A && buf[7] == 0x0A)
    	{
      		printf ("png header found!!!");
		sleep(1);
      		if (needle->header_found != 1)
		{
	  	needle->header_found = 1;
	  	needle->header_blk = blk;
	  	strcpy (needle->dotpart, ".png");
	  	return 1;
		}
      	needle->header_found = -1;
      	return -1;
    }

  //jpg 
 	 if ((buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 0xE1)
      		|| (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF
	  	&& buf[3] == 0xE0))
    	{
      		printf ("jpg header found!!!");
		sleep(1);
      		if (needle->header_found != 1)
		{
	  		needle->header_found = 1;
	  		needle->header_blk = blk;
	  		strcpy (needle->dotpart, ".jpg");
	  	return 1;
		}
      needle->header_found = -1;
      return -1;
    	}

  //pdf 
  	if ((buf[0] == 0x25 && buf[1] == 0x50 && buf[2] == 0x44 && buf[3] == 0x46
      		&& buf[4] == 0x2D && buf[5] == 0x31 && buf[6] == 0x2E))
    	{
      		printf ("pdf header found!!!");
		sleep(1);
      		if (needle->header_found != 1)
		{
	  		needle->header_found = 1;
	  		needle->header_blk = blk;
	  		strcpy (needle->dotpart, ".pdf");
	  		return 1;
		}
     	 needle->header_found = -1;
     	 return -1;
    	}

  //cpp
  	if (memcmp (buf, "#include", 8) == 0)
    	{
      		printf ("C/C++  header found!!!");
		sleep(1);
      		if (needle->header_found != 1)
		{
	  		needle->header_found = 1;
	  		needle->header_blk = blk;
	  		strcpy (needle->dotpart, ".cpp");
	 		return 1;
		}
      	needle->header_found = -1;
      	return -1;
    	}

  //php
  	if (memcmp (buf, "<?php>", 6) == 0)
    	{
      		printf ("php  header found!!!");
		sleep(1);
      		if (needle->header_found != 1)
		{
	  		needle->header_found = 1;
	  		needle->header_blk = blk;
	  		strcpy (needle->dotpart, ".php");
	  		return 1;
		}
      	needle->header_found = -1;
      	return -1;
    	}

	return 0;
	}

int find_footer (unsigned char buf[EXT2_BLOCK_SIZE],struct found *needle, unsigned long blk)
{
  int i = 0;
  while (i <= EXT2_BLOCK_SIZE - 1)
    {
	if(i<EXT2_BLOCK_SIZE - 1)
	{
      		if (buf[i] == 0x00 && buf[i + 1] == 0x3b)
	{
	  	if (strcmp (needle->dotpart, ".gif") == 0)
	    	{
	      		needle->footer_blk = blk;
	     		 needle->footer_found = 1;
	      		return 1;
	    	}
	  	else
	    		return -1;
	}
	}
      //png
	if(i<EXT2_BLOCK_SIZE - 9)
	{
      		if (	buf[i] == 0x00 && buf[i + 1] == 0x00 && buf[i + 2] == 0x49
	  		&& buf[i + 3] == 0x45 && buf[i + 4] == 0x4e && buf[i + 5] == 0x44
	  		&& buf[i + 6] == 0xae && buf[i + 7] == 0x42 && buf[i + 8] == 0x60
	  		&& buf[i + 9] == 0x82)
	{
	  	if (strcmp (needle->dotpart, ".png") == 0)
	    	{
	      		needle->footer_blk = blk;
	      		needle->footer_found = 1;
	      		return 1;
	   	}
	  	else
	    		return -1;
	}
	}
      //jpg
	if(i<EXT2_BLOCK_SIZE - 1)
	{
      		if (buf[i] == 0xFF && buf[i + 1] == 0xD9)
	{
	  	if (strcmp (needle->dotpart, ".jpg") == 0)
	    	{
	      		needle->footer_blk = blk;
	      		needle->footer_found = 1;
	      		return 1;
	    	}
	  	else
	    		return -1;
	}
	}
      	//pdf 
	if(i<EXT2_BLOCK_SIZE - 5)
	{
      		if (buf[i] == 0x25 && buf[i + 1] == 0x25 && buf[i + 2] == 0x45
	  		&& buf[i + 3] == 0x4F && buf[i + 4] == 0x46 && buf[i + 5] == 0x0A)
	{
	  	if (strcmp (needle->dotpart, ".pdf") == 0)
	    	{
	     		needle->footer_blk = blk;
	      		needle->footer_found = 1;
	     		return 1;
	    	}
	  	else
	    		return -1;
	}
	}
      	//C/C++ 
	if(i<EXT2_BLOCK_SIZE - 6)
	{
      		if ((memcmp (&buf[i], "return", 6) == 0))
	{
	  	if (strcmp (needle->dotpart, ".cpp") == 0)
	    	{
	      		needle->footer_blk = blk;
	      		needle->footer_found = 1;
	      		return 1;
	    	}
	  	else
	    		return -1;
	}
	}
	//php 
	if(i<EXT2_BLOCK_SIZE - 6)
	{
	      	if ((memcmp (&buf[i], "</php>", 6) == 0))
	{
	  	if (strcmp (needle->dotpart, ".php") == 0)
		{
	      		needle->footer_blk = blk;
	      		needle->footer_found = 1;
	      		return 1;
	    	}
	  else
	  	return -1;
	}
     	}

      i++;
    }
  return 0;
}
