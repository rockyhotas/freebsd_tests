/*
 * Copyright (C) 2021 Rocky Hotas
 * 
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
 * Based on the simple Echo pseudo-device KLD presented in the FreeBSD Architecture Handbook:
 * https://docs.freebsd.org/en/books/arch-handbook/driverbasics/
 *
 * Rocky Hotas
 */

#include <sys/systm.h>		/* uprintf */
#include <sys/param.h>		/* defines used in kernel.h */
#include <sys/module.h>		/* necessary to create the kernel module itself */
#include <sys/kernel.h>		/* types used in module initialization */
#include <sys/conf.h>		/* cdevsw struct */
#include <sys/uio.h>		/* uio struct */
#include <sys/malloc.h>		/* malloc() will be used within this module */
#include <sys/libkern.h>	/* arc4random(). See random(9) */

#define BUFFERSIZE 2

/* Function prototypes */
static d_open_t      rolld_open;
static d_close_t     rolld_close;
static d_read_t      rolld_read;

/* Character device entry points */
static struct cdevsw rolld_cdevsw = {
	.d_version = D_VERSION,
	.d_open = rolld_open,
	.d_close = rolld_close,
	.d_read = rolld_read,
	.d_name = "rolld",
};

struct s_roll {
	char msg[BUFFERSIZE + 1];
	int len;
};

/* vars */
static struct cdev *rolld_dev;
// it must be <= 9. See rolld_read
static uint32_t d_size = 6;
static struct s_roll *rolldmsg;

MALLOC_DECLARE(M_ROLLDBUF);
MALLOC_DEFINE(M_ROLLDBUF, "rolldbuffer", "minimal buffer for rolld module");

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
		    0444,		/* write is not implemented, so no need to write to this device */
		    "rolld");
		if (error != 0)
			break;

		rolldmsg = malloc(sizeof(*rolldmsg), M_ROLLDBUF, M_WAITOK |
		    M_ZERO);
		printf("Roll device loaded.\n");
		break;
	case MOD_UNLOAD:
		destroy_dev(rolld_dev);
		free(rolldmsg, M_ROLLDBUF);
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
	return (error);
}

static int
rolld_close(struct cdev *dev __unused, int fflag __unused, int devtype __unused,
    struct thread *td __unused)
{
	return (0);
}

static int
rolld_read(struct cdev *dev __unused, struct uio *uio, int ioflag __unused)
{
	size_t amt;
	rolldmsg->len = 2;
	char random_item;
	int error;

	random_item = (char) arc4random();
	rolldmsg->msg[0] = 49 + (random_item % d_size);
	rolldmsg->msg[1] = '\n';
	rolldmsg->msg[rolldmsg->len] = 0;

	amt = MIN(uio->uio_resid, uio->uio_offset >= rolldmsg->len + 1 ? 0 :
	    rolldmsg->len + 1 - uio->uio_offset);

	if ((error = uiomove(rolldmsg->msg, amt, uio)) != 0)
		uprintf("uiomove failed!\n");

	return (error);
}

DEV_MODULE(rolld, rolld_loader, NULL);
