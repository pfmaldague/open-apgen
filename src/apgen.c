#include <config.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
	pid_t pid;
	const char *APCORE = "apbatch";
	const char *ATM = "atm";
	const char *errormsg = "Having trouble starting apgen...\n";
	const char *defaultmsg = "";
	const char *msg = defaultmsg;
	char **newargs;
	int i;
	int want_gui = 1;

	newargs = (char **) malloc(sizeof(char *) * (argc + 1));
	for(i = 0; i < argc; i++) {
		if(!strcmp(argv[i], "-nogui")) {
			want_gui = 0;
		}
		newargs[i] = argv[i];
	}
	newargs[argc] = NULL;
	fflush(stdout);
	if((pid = fork()) < 0) {
		msg = errormsg;
	} else if(!pid) {

		/* child process */
		if(want_gui) {
			if(execvp(ATM, newargs)) {
				char *path = getenv("PATH");
				if(!path) {
					fprintf(stderr, "apgen: child fork error. "
						"Environment var. PATH is not defined. You may want to\n"
						"define PATH so as to include the location of apgen executables.\n");
				} else {
					fprintf(stderr, "apgen: child fork error. "
						"Environment var. PATH is defined as \n%s.\n"
						"You may want to modify it to include the location of apgen executables.\n",
						path);
				}
			}
		} else {
			if(execvp(APCORE, newargs)) {
				char *path = getenv("PATH");
				if(!path) {
					fprintf(stderr, "apgen: child fork error. "
						"Environment var. PATH is not defined. You may want to\n"
						"define PATH so as to include the location of apgen executables.\n");
				} else {
					fprintf(stderr, "apgen: child fork error. "
						"Environment var. PATH is defined as \n%s.\n"
						"You may want to modify it to include the location of apgen executables.\n",
						path);
				}
			}
		}
		exit(0);
	} else {

		/* parent process */
		/* fprintf(stderr, "parent waiting for child..."); */
		if(waitpid(pid, NULL, 0) != pid) {
			msg = errormsg;
		}
		/* fprintf(stderr, " got it.\n"); */
	}
	fprintf(stderr, "%s", msg);
	return 0;
}
