#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

extern char *hostname(void);

static int cwrite(int fd, const char *buffer, unsigned int r)
{
	unsigned int i;
	int p;
	for (i = 0; i < r;) {
		do {
			p = write(fd, buffer+i, r-i);
		} while (p == -1 && (errno == EINTR
#ifdef EAGAIN
				|| errno == EAGAIN
#endif
				));
		if (p < 1) return 0;
		i += p;
	}
	return 1;
}

extern char *rp_srs_fix(char *rp);


int main(int argc, char *argv[])
{
	char buffer[32770];
	char *p, *x, *y, *rp, *dt;
	int fd, r;

	if (argc == 1) {
		fprintf(stderr, "Usage: %s dir/. < message\n", argv[0]);
		exit(1);
	}
	if (chdir(argv[1]) == -1) {
		fprintf(stderr, "Can't chdir(%s): %s\n", argv[1], strerror(errno));
		exit(111);
	}

	rp = rp_srs_fix(getenv("RPLINE"));
	dt = getenv("DTLINE");

	x = hostname();
	if (!x) {
		fprintf(stderr, "Can't determine hostname: %s\n", strerror(errno));
		exit(111);
	}
	/* sanity check */
	for (p = x; *p; p++) {
		if (*p <= 32 || *p >= 127) {
			*p = 0;
			break;
		}
	}

	y = malloc(strlen(x) + 128);
	if (!y) {
		fprintf(stderr, "malloc(%d): %s\n", (int)(strlen(x)+128), strerror(errno));
		exit(111);
	}

	sprintf(y, "tmp/%lu.%ld.%s", (unsigned long)time(0), (long int)getpid(), x);

	x = malloc(strlen(y));
	if (!x) {
		fprintf(stderr, "malloc(%d): %s\n", (int)strlen(y), strerror(errno));
		exit(111);
	}

	sprintf(x, "new%s", y+3);

	fd = open(y, O_CREAT|O_RDWR|O_NOCTTY|O_EXCL
#ifdef O_LARGEFILE
			| O_LARGEFILE
#endif
			, 0666);
	if (fd == -1) {
		fprintf(stderr, "creat(%s): %s\n", y, strerror(errno));
		exit(111); /* eh... we're lazy */
	}

	if (rp && !cwrite(fd, rp, strlen(rp))) {
		fprintf(stderr, "rpwrite(): %s\n", strerror(errno));
		close(fd);
		unlink(y);
		exit(111);
	}
	if (dt && !cwrite(fd, dt, strlen(dt))) {
		fprintf(stderr, "rpwrite(): %s\n", strerror(errno));
		close(fd);
		unlink(y);
		exit(111);
	}

	for (;;) {
		do {
			r = read(0, buffer, sizeof(buffer)-2);
		} while (r == -1 && (errno == EINTR
#ifdef EAGAIN
					|| errno == EAGAIN
#endif
				));
		if (r == -1) {
			fprintf(stderr, "read(): %s\n", strerror(errno));
			close(fd);
			unlink(y);
			exit(111);
		}
		if (r == 0) break;

		if (!cwrite(fd, buffer, r)) {
			fprintf(stderr, "write(): %s\n", strerror(errno));
			close(fd);
			unlink(y);
			exit(111);
		}
	}
	if (fsync(fd) == -1) {
		fprintf(stderr, "fsync(): %s\n", strerror(errno));
		close(fd);
		unlink(y);
		exit(111);
	}
	if (close(fd) == -1) {
		fprintf(stderr, "close(): %s\n", strerror(errno));
		/* NFS junk */
		unlink(y);
		exit(111);
	}
	/* sanity check */
	for (p = x; *p; p++) {
		if (*p <= 32 || *p >= 127) {
			*p = 0;
			break;
		}
	}
	for (p = y; *p; p++) {
		if (*p <= 32 || *p >= 127) {
			*p = 0;
			break;
		}
	}
	if (rename(y, x) == -1) {
		fprintf(stderr, "rename(): %s\n", strerror(errno));
		unlink(y);
		exit(111);
	}
	exit(0);
}
