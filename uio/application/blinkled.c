#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "../uio_helper/uio_helper.h"
#include "../uio_helper/system.h"

#define UIO_NAME "/dev/uio0"

#define CRA_INTERRUPT_STATUS (0x40/4)
#define CRA_INTERRUPT_ENABLE (0x50/4)

int main (int argc, char *argv[])
{
    struct uio_info_t* info;
    int fuio;
    int ret;

    long ipending;
    unsigned long *bar0;

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
    bar0 = (unsigned long *)mmap(NULL, info->maps[1].size,
                                 PROT_READ|PROT_WRITE, MAP_SHARED, fuio,
                                 1*getpagesize());
    if (bar0 == NULL) {
        printf("Error can't allocate memory for bar0\n");
        goto err_close_fuio;
    }

    /* print info */
    printf("Name : %s\n", info->name);

    /* wait for interrupt */
    ret = read(fuio, &ipending, sizeof(long));
    printf("interrupt occurs\nret : %d\nipending : %ld\n", ret, ipending);


    printf("CRA_INTERRUPT_STATUS : 0x%08lX\n", bar0[CRA_INTERRUPT_STATUS]);
    printf("CRA_INTERRUPT_ENABLE : 0x%08lX\n", bar0[CRA_INTERRUPT_ENABLE]);

    /* end */

err_unmap_bar0:
    munmap(bar0, info->maps[1].size);
err_close_fuio:
    close(fuio);
err_free_info:
	uio_free_info(info);
    return 0;
}
