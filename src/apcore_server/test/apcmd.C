#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>	/* for printf etc. */
#include <string.h>	/* for strlen() etc. */
#include <stdlib.h> 	/* for malloc() */
#include <iostream>

#include <XmlRpc.H>

// GLOBALS
bool			ap_verbose = false;
const char*		theClientName = NULL;
int			thePortNumber = -1;

extern "C" {
#include <concat_util.h>
} // extern "C"

// LOCALS
static bool		std_input = false;
static const char*	prog_name;
static buf_struct	In = {NULL, 0, 0};

// extern void	build_xmlrpc_object(GdomeDocument*, GdomeElement*, XmlRpc::XmlRpcValue&);
extern int	parse(int argc, char* argv[]);
extern int	ap_checkplan(int argc, char* argv[], const char* Pname);
extern int	ap_consumption(int argc, char* argv[], const char* Pname);
extern int	ap_crash(int argc, char* argv[], const char* Pname);
extern int	ap_create(int argc, char* argv[], const char* Pname);
extern int	ap_createmany(int argc, char* argv[], const char* Pname);
extern int	ap_deleteact(int argc, char* argv[], const char* Pname);
extern int	ap_edit(int argc, char* argv[], const char* Pname);
extern int	ap_editableduration(int argc, char* argv[], const char* Pname);
extern int	ap_getapf(int argc, char* argv[], const char* Pname);
extern int	ap_getattr(int argc, char* argv[], const char* Pname);
extern int	ap_getparam(int argc, char* argv[], const char* Pname);
extern int	ap_getplan(int argc, char* argv[], const char* Pname);
extern int	ap_getresource(int argc, char* argv[], const char* Pname);
extern int	ap_getstdatts(int argc, char* argv[], const char* Pname);
extern int	ap_getviolations(int argc, char* argv[], const char* Pname);
extern int	ap_init(int argc, char* argv[], const char* Pname);
extern int	ap_integral(int argc, char* argv[], const char* Pname);
extern int	ap_json(int argc, char* argv[], const char* Pname);
extern int	ap_putfile(int argc, char* argv[], const char* Pname);
extern int	ap_setattr(int argc, char* argv[], const char* Pname);
extern int	ap_setparam(int argc, char* argv[], const char* Pname);
extern int	ap_terminate(int argc, char* argv[], const char* Pname);
extern int	ap_translate(int argc, char* argv[], const char* Pname);
extern int	ap_sitAndWait(int argc, char* argv[], const char* Pname);
extern int	ap_system(int argc, char* argv[], const char* Pname);

static const char* commands[] = {
	"checkplan",
	"consumption",
	"crash",
	"create",
	"createmany",
	"deleteact",
	"edit",
	"editableduration",
	"getapf",
	"getattr",
	"getparam",
	"getplan",
	"getresource",
	"getstdatts",
	"getviolations",
	"init",
	"integral",
	"json",
	"parse",
	"putfile",
	"setattr",
	"setparam",
	"sitAndWait",
	"system",
	"terminate",
	"translate",
	""};

int usage(const char* s) {
	int i;
	printf("Usage: %s [-v] <cmd> [-v] [cmd option 1 [cmd option 2 ...]]\n", s);
	printf("  where <cmd> is one of:\n");
	for(i = 0; commands[i][0] != '\0'; i++) {
		printf("          %s\n", commands[i]); }
	printf("       If '-' is used as the last cmd option, it should be followed by expression data terminated by ^D.\n");
	printf("       For help on a specific command: %s <cmd> -h\n", s);
	return -1; }

int main(int argc, char* argv[]) {
	int i = 1, ret;
	buf_struct progplus = {NULL, 0, 0};

	prog_name = argv[0];
	if(argc == i) {
		return usage(prog_name); }
	if(!strncmp(argv[1], "-verbose", 2)) {
		i++;
		XmlRpc::setVerbosity(5); }
	if(argc == i) {
		return usage(prog_name); }
	concatenate(&progplus, prog_name);
	concatenate(&progplus, " ");
	if(!strcmp(argv[i], "checkplan")) {
		ret = ap_checkplan(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "consumption")) {
		ret = ap_consumption(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "crash")) {
		ret = ap_crash(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "create")) {
		ret = ap_create(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "createmany")) {
		ret = ap_createmany(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "deleteact")) {
		ret = ap_deleteact(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "edit")) {
		ret = ap_edit(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "editableduration")) {
		ret = ap_editableduration(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "getapf")) {
		ret = ap_getapf(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "getattr")) {
		ret = ap_getattr(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "getparam")) {
		ret = ap_getparam(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "getplan")) {
		ret = ap_getplan(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "getresource")) {
		ret = ap_getresource(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "getstdatts")) {
		ret = ap_getstdatts(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "getviolations")) {
		ret = ap_getviolations(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "init")) {
		ret = ap_init(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "integral")) {
		ret = ap_integral(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "json")) {
		ret = ap_json(argc - i, argv + i, progplus.buf); }
	/* NOTE: the parse option is for exercising the interface. Normally,
	 * the parser is used to parse parameters for a command; the call to
	 * parse() should be initiated by the command handler (not in this
	 * file.) */
	else if(!strcmp(argv[i], "parse")) {
		ret = parse(argc - i, argv + i); }
	else if(!strcmp(argv[i], "putfile")) {
		ret = ap_putfile(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "setattr")) {
		ret = ap_setattr(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "setparam")) {
		ret = ap_setparam(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "sitAndWait")) {
		ret = ap_sitAndWait(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "system")) {
		ret = ap_system(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "terminate")) {
		ret = ap_terminate(argc - i, argv + i, progplus.buf); }
	else if(!strcmp(argv[i], "translate")) {
		ret = ap_translate(argc - i, argv + i, progplus.buf); }
	else {
		ret = -1;
		printf("Did not understand command \"%s\".\n", argv[i]); }
	destroy_buf_struct(&progplus);
	return ret; }

int parse(int argc, char* argv[]) {

	/* Initially I was going to switch over to libxml++. However, this is
	 * stupid because we don't really need to use XML for anything. It's
	 * just as well to use json or apgen's own TypedValue, for that
	 * matter. In fact, TypedValue has the huge advantage of being
	 * well-adapted to what we want. */

	return -1;
}
