/*
    idle3ctl - Disable, get or set the idle3 timer of Western Digital HDD
    Copyright (C) 2011  Christophe Bothamy

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    For more information, please contact cbothamy at users.sf.net
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <linux/types.h>
#include <errno.h>

#include "sgio.h"
int prefer_ata12 = 0;
int verbose = 0;

int vscenabled = 0;
int force = 0;

char *device;
char *progname;
int fd=0;

#define VERSION "0.9.1"

#define VSC_KEY_WRITE 0x02
#define VSC_KEY_READ 0x01

int check_WDC_drive()
{
  static __u8 atabuffer[4+512];
  int i;

  if (verbose) {
    printf("Checking if Drive is a Western Digital Drive\n");
  }

  memset(atabuffer, 0, sizeof(atabuffer));
  atabuffer[0] = ATA_OP_IDENTIFY;
  atabuffer[3] = 1;
  if (do_drive_cmd(fd, atabuffer)) {
    perror(" HDIO_DRIVE_CMD(identify) failed");
    return errno;
  } 

  if (!force) {
    /* Check for a Western Digital Drive  3 first characters : WDC*/
    if ( (atabuffer[4+(27*2)+1] != 'W')
      || (atabuffer[4+(27*2)] != 'D')
      || (atabuffer[4+(28*2)+1] != 'C')) {
      fprintf(stderr, "The drive %s does not seem to be a Western Digital Drive ",device);
      fprintf(stderr, "but a ");
      for (i=27; i<47; i++) {
        if(atabuffer[4+(i*2)+1]==0)break;
        putchar(atabuffer[4+(i*2)+1]);
        if(atabuffer[4+(i*2)+0]==0)break;
        putchar(atabuffer[4+(i*2)+0]);
      }
      printf("\n");

      fprintf(stderr, "Use the --force option if you know what you're doing\n");
      return 1;
    }
  }

  return 0;
}

/* Enable Vendor Specific Commands */
int VSC_enable()
{
  if (verbose) {
    printf("Enabling Vendor Specific ATA commands\n");
  }

  int err = 0;
  struct ata_tf tf;
  tf_init(&tf, ATA_OP_VENDOR_SPECIFIC, 0, 0);
  tf.lob.feat = 0x45;
  tf.lob.lbam = 0x44;
  tf.lob.lbah = 0x57;
  tf.dev = 0xa0;

  if(sg16(fd, SG_WRITE, SG_PIO, &tf, NULL, 0, 5)) {
    err = errno;
    perror("sg16(VSC_ENABLE) failed");
    return err;
  }
  vscenabled = 1;
  return 0;
}

/* Disable Vendor Specific Commands */
int VSC_disable()
{
  if (verbose) {
    printf("Disabling Vendor Specific ATA commands\n");
  }

  int err = 0;
  struct ata_tf tf;
  tf_init(&tf, ATA_OP_VENDOR_SPECIFIC, 0, 0);
  tf.lob.feat = 0x44;
  tf.lob.lbam = 0x44;
  tf.lob.lbah = 0x57;
  tf.dev = 0xa0;

  if(sg16(fd, SG_WRITE, SG_PIO, &tf, NULL, 0, 5)) {
    err = errno;
    perror("sg16(VSC_DISABLE) failed");
    return err;
  }
  vscenabled = 0;
  return 0;
}

/* Vendor Specific Commands sendkey */
int VSC_send_key(char rw)
{
  int err = 0;
  char buffer[512];
  struct ata_tf tf;

  tf_init(&tf, ATA_OP_SMART, 0, 0);
  tf.lob.feat = 0xd6;
  tf.lob.nsect = 0x01;
  tf.lob.lbal = 0xbe;
  tf.lob.lbam = 0x4f;
  tf.lob.lbah = 0xc2;
  tf.dev = 0xa0;

  memset(buffer,0,sizeof(buffer));
  buffer[0]=0x2a;
  buffer[2]=rw;
  buffer[4]=0x02;
  buffer[6]=0x0d;
  buffer[8]=0x16;
  buffer[10]=0x01;

  if(sg16(fd, SG_WRITE, SG_PIO, &tf, buffer, 512, 5)) {
    err = errno;
    perror("sg16(VSC_SENDKEY) failed");
    return err;
  }
  vscenabled = 0;
  return 0;
}
int VSC_send_write_key()
{
  if (verbose) {
    printf("Sending WRITE key\n");
  }
  return VSC_send_key(VSC_KEY_WRITE);
}
int VSC_send_read_key()
{
  if (verbose) {
    printf("Sending READ key\n");
  }
  return VSC_send_key(VSC_KEY_READ);
}

/* Vendor Specific Commands get timer value */
int VSC_get_timer(unsigned char *timer)
{
  if (verbose) {
    printf("Getting Idle3 timer value\n");
  }

  int err = 0;
  char buffer[512];
  struct ata_tf tf;

  tf_init(&tf, ATA_OP_SMART, 0, 0);
  tf.lob.feat = 0xd5;
  tf.lob.nsect = 0x01;
  tf.lob.lbal = 0xbf;
  tf.lob.lbam = 0x4f;
  tf.lob.lbah = 0xc2;
  tf.dev = 0xa0;

  memset(buffer,0,sizeof(buffer));

  if(sg16(fd, SG_READ, SG_PIO, &tf, buffer, 512, 5)) {
    err = errno;
    perror("sg16(VSC_GET_TIMER) failed");
    return err;
  }
  
  *timer=buffer[0];
  return 0;
}

/* Vendor Specific Commands set timer value */
int VSC_set_timer(unsigned char timer)
{
  if (verbose) {
    printf("Setting Idle3 timer value to %02x\n", timer);
  }

  int err = 0;
  char buffer[512];
  struct ata_tf tf;

  tf_init(&tf, ATA_OP_SMART, 0, 0);
  tf.lob.feat = 0xd6;
  tf.lob.nsect = 0x01;
  tf.lob.lbal = 0xbf;
  tf.lob.lbam = 0x4f;
  tf.lob.lbah = 0xc2;
  tf.dev = 0xa0;

  memset(buffer,0,sizeof(buffer));
  buffer[0]=timer;

  if(sg16(fd, SG_WRITE, SG_PIO, &tf, buffer, 512, 5)) {
    err = errno;
    perror("sg16(VSC_SET_TIMER) failed");
    return err;
  }
  
  return 0;
}

/* Called on exit, last chance do call VSC_disable if needed */
void cleanup(void)
{
  if (fd==0) return;
  if (vscenabled != 0) VSC_disable();
  close(fd);
}

/* Version */
void show_version(void)
{
  printf("%s v%s\n", progname, VERSION);
}

/* Usage */
void show_usage(void)
{
  printf("%s v%s - Read, Set or disable the idle3 timer of Western Digital drives\n", progname, VERSION);
  printf("Copyright (C) 2011  Christophe Bothamy\n");
  printf("\n");
  printf("Usage: %s [options] device\n", progname);
  printf("Options: \n");
  printf(" -h : displat help\n");
  printf(" -V : show version and exit immediately\n");
  printf(" -v : verbose output\n");
  printf(" --force : force even if no Western Digital HDD are detected\n");
  printf(" -g : get raw idle3 timer value\n");
  printf(" -g100 : get idle3 timer value as wdidle3 v1.00 value\n");
  printf(" -g103 : get idle3 timer value as wdidle3 v1.03 value\n");
  printf(" -g105 : get idle3 timer value as wdidle3 v1.05 value\n");
  printf(" -d : disable idle3 timer\n");
  printf(" -s<value> : set idle3 timer raw value\n");
}

int main(int argc, char **argv)
{
  int i;
  int action = 1;       /* 0: write, 1: read raw, 2: read as v1.00, 3: read as v1.03 */
  unsigned char timer;
  int display_usage = 1;

  if ((progname = (char *) strrchr(*argv, '/')) == NULL) progname = *argv;
  else progname++;

  // Cleanup on exit
  atexit(cleanup);
  
  // Check for options
  for (i=1; i<argc; i++) {
    if (strcmp(argv[i],"-h")==0) {
      show_usage();
      exit(1);
    }
    else if (strcmp(argv[i],"-V")==0) {
      show_version();
      exit(1);
    }
    else if (strcmp(argv[i],"--force")==0) force=1;
    else if (strcmp(argv[i],"-v")==0) verbose=1;
    else if (strcmp(argv[i],"-g")==0) action=1;
    else if (strcmp(argv[i],"-g100")==0) action=2;
    else if (strcmp(argv[i],"-g103")==0) action=3;
    else if (strcmp(argv[i],"-g105")==0) action=3;
    else if (strcmp(argv[i],"-d")==0) {
      action=0;
      timer=0;
    }
    else if (strncmp(argv[i],"-s",2)==0) {
      action=0;
      timer=0;
      if (strlen(argv[i])>2) timer=atoi(argv[i]+2);
      else {
        if (++i<argc) timer=atoi(argv[i]);
      }
      if (timer<=0) {
        fprintf(stderr,"Unable to parse a positive value to set timer\n");
        exit(1);
      }
    }
    else if (strncmp(argv[i],"-",1)==0) {
        fprintf(stderr,"Unknown option '%s'\n\n",argv[i]);
        show_usage();
        exit(1);
    }
    else {
      display_usage=0;

      // Open device
      device = argv[i];
      fd = open(device, O_RDONLY|O_NONBLOCK);
      if (fd<0) {
        perror("open failed");
        exit(1);
      }
    
      // Check if HDD is a WD
      if (check_WDC_drive() != 0) {
        exit(1);
      }
  
      if (VSC_enable()!=0) exit(1);
    
      if (action == 0) {
        if (VSC_send_write_key()!=0) exit(1);
        if (VSC_set_timer(timer)!=0) exit(1);

        if (timer==0) printf("Idle3 timer disabled\n");
        else printf("Idle3 timer set to %d (0x%02x)\n", timer, timer);

	printf("Please power cycle your drive off and on for the new "
               "setting to be taken into account. A reboot will not be enough!\n");
      }
      else if (action>=1) {
        if (VSC_send_read_key()!=0) exit(1);
        if (VSC_get_timer(&timer)!=0) exit(1);

        if (timer==0) {
          printf("Idle3 timer is disabled\n");
        }
        else switch (action) {
          case 1:
            printf("Idle3 timer set to %d (0x%02x)\n",timer, timer);
            break;
          case 2:
            printf("Idle3 timer set to %.1fs (0x%02x)\n",timer/10.0, timer);
            break;
          case 3:
            printf("Idle3 timer set to %.1fs (0x%02x)\n",(timer<129)?timer/10.0:(timer-128)*30.0, timer);
            break;
        }
      }
  
      VSC_disable();
  
      close(fd);
      fd=0;
    }
  }

  if (display_usage) {
    show_usage();
  }

  exit(0);
}
