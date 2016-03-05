#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "../uio_helper/uio_helper.h"
#include "../uio_helper/system.h"

#define UIO_NAME "/dev/uio0"
#define LEDBUTTON_OFFSET 0x4000

#define BAR0 0

#define BLINKSPEED	500000
#define BLINK_MODES_MAX	3

static volatile int keepRunning = 1;
static volatile int blinkState = 0;

static unsigned long *bar0; /* 32 bits access to full BAR0 */
volatile unsigned short *ledbutton; /* 16 bits access to ledbutton */

void ctrlc(int dummy) {
	keepRunning = 0;
	printf("\nCtrl-C captured, press button to end program\n");
}

void *blinkingledthread(void * arg)
{
	int pwmcount = 0;

	while(keepRunning) {
		switch(blinkState) {
			case 1: /* led less than 50% on */
				if (pwmcount == 10) {
					ledbutton[0] = 0x0001;
					pwmcount = 0;
				} else
					ledbutton[0] = 0;
					pwmcount++;
				break;
			case 2: /* led full on */
				ledbutton[0] = 0x0001;
				usleep(BLINKSPEED);
				break;
			case 3: /* led blinking */
				ledbutton[0] = ~ledbutton[0];
				usleep(BLINKSPEED);
				break;
			case 0:
			default:
				ledbutton[0] = 0; /* led off */
				usleep(BLINKSPEED);
		}
	}
	pthread_exit (0);
}

int main (int argc, char *argv[])
{
	struct uio_info_t* info;
	int fuio; /* /dev/uio0 */
	int ret;

	long ipending; /* number of interrupts occured */
	pthread_t th;

	signal(SIGINT, ctrlc);

	/* get  uio device 0 info */
	info = uio_find_devices(0);
	uio_get_all_info(info);
	uio_get_device_attributes(info);

	fuio = open(UIO_NAME, O_RDWR);
	if (fuio < 0) {
		printf("Can't open fuio\n");
		goto err_free_info;
	}

	/* mmap bar0 */
	bar0 = (unsigned long *)mmap(NULL, info->maps[BAR0].size,
				 PROT_READ|PROT_WRITE, MAP_SHARED, fuio,
				 BAR0*getpagesize());
	if (bar0 == NULL) {
		printf("Error can't allocate memory for bar0\n");
		goto err_close_fuio;
	}
	ledbutton = ((unsigned short *)&bar0[LEDBUTTON_OFFSET/4]);

	/* print info */
	printf("Name : %s\n", info->name);

	printf("Led identifier %d\n", ledbutton[1]);
	printf("Button identifier %d\n", ledbutton[2]);

	if (pthread_create (&th, NULL, blinkingledthread , "blinkthread") < 0) {
		fprintf (stderr, "pthread_create error\n");
		goto err_unmap;
	}

	while(keepRunning) {
		/* wait for interrupt */
		ret = read(fuio, &ipending, sizeof(long));
		printf("interrupt occurs\nret : %d\nipending : %ld\n",
		       ret, ipending);
		if (ledbutton[3] != 0) { /* if button pressed */
			blinkState = (blinkState < BLINK_MODES_MAX)?
					blinkState + 1 : 0;
			printf("Blink led mode %d\n", blinkState);
		}
	}

	(void)pthread_join (th, NULL);

	/* end */
err_unmap:
	munmap(bar0, info->maps[BAR0].size);
err_close_fuio:
	close(fuio);
err_free_info:
	uio_free_info(info);
	return 0;
}
