#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>

static char *__get_sys_hostname(void)
{
	static char s[512];
	unsigned int j = 1024;
	struct utsname me;
	char *q, *p;
	
	p = getenv("HOSTNAME");
	if (p) return p;

	if (uname(&me) == 0) {
		for (q = me.nodename; q < s+(sizeof(s)-2); q++) {
			if (*q == '.') *q = '\0';
			if (*q == '\0') {
				strncpy(s, me.nodename, sizeof(s)-2);
				return s;
			}
		}
		/* hrm, a more draconian method is required... */
	}

	if (gethostname(s, sizeof(s)-1) == 0) {
		for (q = s; q < s+(sizeof(s)-2); q++) {
			if (*q == '.') *q = '\0';
			if (*q == '\0') return s;
		}
	}

	for (p = NULL;;) {
		p = (char *)realloc(p, j);
		if (!p) return 0;
		if (gethostname(p, j-1) == 0) {
			for (q = p; q < p+(j-2); q++) {
				if (*q == '.') *q = '\0';
				if (*q == '\0') return p;
			}
		}
		j += 16;
	}
}
char *hostname(void)
{
	static char *cx = 0;
	if (!cx) cx = __get_sys_hostname();
	return cx;
}
