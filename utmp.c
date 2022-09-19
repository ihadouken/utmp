

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>


struct passwd *pw;
gid_t egid, gid;


void
die(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);
	va_end(va);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int status;
	uid_t uid;
	sigset_t set;
	extern void addutmp(void), delutmp(void);

	egid = getegid();
	gid = getgid();
	setgid(gid);

	errno = 0;
	if ((pw = getpwuid(uid = getuid())) == NULL) {
		if(errno)
			die("utmp:getpwuid:%s", strerror(errno));
		else
			die("utmp:who are you?");
	}

	setenv("LOGNAME", pw->pw_name, 1);
	setenv("USER", pw->pw_name, 1);
	setenv("SHELL", pw->pw_shell, 1);
	setenv("HOME", pw->pw_dir, 1);

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);

	switch (fork()) {
	case 0:
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		argv[0] = getenv("SHELL");
		execv(argv[0], argv);
		die("error executing shell:%s", strerror(errno));
	case -1:
		die("error spawning child:%s", strerror(errno));
	default:
		addutmp();
		signal(SIGINT, SIG_IGN);
		signal(SIGTERM, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		sigprocmask(SIG_UNBLOCK, &set, NULL);

		if (wait(&status) == -1)
			perror("utmp:error waiting child");
		delutmp();
	}
	return (WIFEXITED(status)) ? WEXITSTATUS(status) : EXIT_FAILURE;
}
