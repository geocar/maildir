# normally you'll call this with make args...
all: maildir
maildir: unix/maildir.c common/hostname.c
