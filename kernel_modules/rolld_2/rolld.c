/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * As regards GPLv3: https://www.gnu.org/licenses/gpl-3.0.html
*/

/*
 * Create a character device, roll a d_size die (<= 9) and print the output to the user.
 * Minimal example to show driver and uiomove operation,
 *
 * 1) partially based on the echo pseudo-device KLD presented in the FreeBSD
 * Architecture Handbook:
 * https://docs.freebsd.org/en/books/arch-handbook/driverbasics/
 *
 * 2) but also written with the fundamental suggestions of this message:
 * https://lists.freebsd.org/archives/freebsd-hackers/2024-April/003146.html
 *
 */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/libkern.h>

static d_open_t      rolld_open;
static d_close_t     rolld_close;
static d_read_t      rolld_read;

static struct cdevsw rolld_cdevsw = {
	.d_version = D_VERSION,
	.d_open = rolld_open,
	.d_close = rolld_close,
	.d_read = rolld_read,
	.d_name = "rolld",
};

/* vars */
static struct cdev *rolld_dev;
static uint32_t d_size = 6;

static int
rolld_loader(struct module *m __unused, int what, void *arg __unused)
{
	int error = 0;

	switch (what) {
	case MOD_LOAD:
		error = make_dev_p(MAKEDEV_CHECKNAME | MAKEDEV_WAITOK,
		    &rolld_dev,
		    &rolld_cdevsw,
		    0,
		    UID_ROOT,
		    GID_WHEEL,
		    0444,
		    "rolld");
		if (error != 0)
			break;

		printf("Roll device loaded.\n");
		break;
	case MOD_UNLOAD:
		destroy_dev(rolld_dev);
		printf("Roll device unloaded.\n");
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}
	return (error);
}

static int
rolld_open(struct cdev *dev __unused, int oflags __unused, int devtype __unused,
    struct thread *td __unused)
{
	int error = 0;

	uprintf("Opened device \"rolld\" successfully.\n");
	return (error);
}

static int
rolld_close(struct cdev *dev __unused, int fflag __unused, int devtype __unused,
    struct thread *td __unused)
{
	uprintf("Closing device \"rolld\".\n");
	return (0);
}

static int
rolld_read(struct cdev *dev __unused, struct uio *uio, int ioflag __unused)
{
	uprintf("Hello.\n");
	uprintf("uio_offset: %ld\n", uio->uio_offset);
	uprintf("uio_resid: %ld\n", uio->uio_resid);
	uint8_t random_out;

	random_out = arc4random() % d_size;

	if (uio->uio_offset > 0) {
		uprintf("I'm zero!\n");
		return 0;
	}
	uprintf("Returned value is: %d\n", (uiomove(&random_out, 1, uio)));
	return 0;
}

DEV_MODULE(rolld, rolld_loader, NULL);
