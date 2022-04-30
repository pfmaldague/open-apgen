#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>	/* for printf etc. */
#include <string.h>	/* for strlen() etc. */
#include <stdlib.h> 	/* for malloc() */

#include <XmlRpc.H>


extern "C" {

#ifdef have_xml_reader
#	include <gdome_wrapper.h>

#endif // have_xml_reader

extern char*	xmtext;
extern int	ap_linenumber(const char* Origin);
extern int	xmparse(void);
extern void	ap_init_parser();
extern void	ap_lex_buffers_initialize_to(const char* b, const char** Origin);
extern int	generic_error(const char* s);
extern void	apcmd_install_symbols();
int		(*linenumber_handle)(const char* Or) = NULL;
char**		text_handle;
const char*	origin;
} // extern "C"

// GLOBALS
bool		ap_verbose = false;
const char*	theClientName = NULL;
int		thePortNumber = -1;

// LOCALS
static bool		std_input = false;
static GdomeDocument*	theDoc = NULL;
static GdomeElement*	theEl = NULL;
static const char*	prog_name;
static buf_struct	In = {NULL, 0, 0};

extern void	build_xmlrpc_object(GdomeDocument*, GdomeElement*, XmlRpc::XmlRpcValue&);
// extern int	parse(int argc, char* argv[], XmlRpc::XmlRpcValue& args);
extern int	ap_spice(int argc, char* argv[], const char* Pname);

static const char* commands[] = {
	"Compute_Ang_Sep_Times",
	"Compute_Boresight_Angle",
	"Compute_Closest_Approach_Times",
	"compute_comet_state",
	"compute_earth_state",
	"compute_flyby_orientation",
	"compute_flyby_state",
	"compute_FSW_state",
	"compute_ground_station_state",
	"compute_impactor_state",
	"compute_lander_orbiter_az_el",
	"compute_lander_sun_earth_az_el",
	"compute_light_time",
	"compute_moon_state",
	"Compute_Occultation_Times",
	"Compute_Orb_Elements",
	"compute_sep_angle",
	"compute_state",
	"compute_state_wframe",
	"compute_station_angles",
	"compute_sun_state",
	"Compute_Surface_Intercept",
	"ConvertMPFTime",
	"et2utcs",
	"ET_to_VTC",
	"formatCPM",
	"formatCart",
	"generateCK",
	"get_Current_UTC_time",
	"get_Earth_Azimuth",
	"get_Earth_Elevation",
	"get_ET_time",
	"get_Flyby_q",
	"get_FSW_state",
	"get_horizon_elevation",
	"get_Orbiter_Elevation",
	"get_Orbiter_Azimuth",
	"get_Orbiter_Range",
	"get_Orb_Elements",
	"get_Sun_Azimuth",
	"get_Sun_Elevation",
	"init_spice",
	"reinit_spice",
	"scet2sclk",
	"sclk2scet",
	"spkez_api",
	"Time_to_ET",
	"is_an_act_type",
	"get_all_act_types",
	""};

void initialize_xml_system(buf_struct* inbuf, GdomeDocument** doc, GdomeElement** elem) {
	initialize_DOM( "XMLRPC",	/* name of the document that will be created */
			doc,		/* will contain a ptr to the document node */
			elem,		/*  "      "       "   "  "  top-level element */
			0,		/* whether to add certain subnodes (unused) */
			NULL		/* optional attribute to be added to the top-level element */
		      );
	set_default_document_to(*doc, *elem);
	ap_lex_buffers_initialize_to(inbuf->buf, &origin);
	apcmd_install_symbols(); }

int usage(const char* s) {
	int i;
	printf("Usage: %s [-v] <cmd> [-v] [cmd option 1 [cmd option 2 ...]]\n", s);
	printf("  where <cmd> is one of:\n");
	for(i = 0; commands[i][0] != '\0'; i++) {
		printf("          %s\n", commands[i]); }
	printf("       If '-' is used as the last cmd option, it should be followed by expression data terminated by ^D.\n");
	printf("       For help on a specific command: %s <cmd> -h\n", s);
	return -1; }

bool is_a_valid_command(const char* c, int& j) {
	int i;
	for(i = 0; commands[i][0] != '\0'; i++) {
		if(!strcmp(c, commands[i])) {
			j = i;
			return true; } }
	j = -1;
	return false; }

int main(int argc, char* argv[]) {
	int		i = 1, j, ret;
	buf_struct	progplus = {NULL, 0, 0};

	prog_name = argv[0];
	if(argc == 1) {
		return usage(prog_name); }
	if(!strncmp(argv[1], "-verbose", 2)) {
		i++;
		XmlRpc::setVerbosity(5); }
	concatenate(&progplus, prog_name);
	concatenate(&progplus, " ");

	if(is_a_valid_command(argv[i], j)) {
		/* progplus is provided just to improve the clarity of any
		 * error messages emanating from ap_spice: */
		ret = ap_spice(argc - i, argv + i, progplus.buf); }
	else {
		ret = -1;
		printf("Did not understand command \"%s\".\n", argv[i]); }

	destroy_buf_struct(&progplus);
	return ret; }

#ifdef OBSOLETE
int parse(int argc, char* argv[], XmlRpc::XmlRpcValue& Args) {
#ifdef have_xml_reader

	int			i, ret;

	if(argc == 1) {
		return usage(prog_name); }

	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-v")) {
			ap_verbose = true; }
		else if(!strcmp(argv[i], "-")) {
			std_input = true; } }

	if(std_input) {
		char byte[2];
		int c;
		byte[1] = '\0';
		while((c = getchar()) != EOF) {
			byte[0] = c;
			concatenate(&In, byte); } }
	else {
		concatenate(&In, argv[argc - 1]); }

	initialize_xml_system(&In, &theDoc, &theEl);
	/* should check return */
	ret = xmparse();
	if(ap_verbose) {
		printf("Return from xmparse() = %d\n", ret); }
	destroy_buf_struct(&In);
	if(!ret) {
		/* no errors */
		saveAPDOM("XmlRpcValue.xml", theDoc);
		build_xmlrpc_object(theDoc, theEl, Args);
		cleanupDocument(&theDoc, &theEl);
		cleanupImplementation();
		return 0; }
	else {
		return -1; }
#else
	return -1;
#endif
}
#endif /* OBSOLETE */

extern "C" {
int generic_error(const char* s) {
	char* buffer;

	buffer = (char *) malloc(strlen(s) + strlen(*text_handle) + 120);
	sprintf(buffer, "%s on line %d near \"%s\" #", s, linenumber_handle(origin),
		(const char *) *text_handle);
	fprintf(stderr, "Error: %s\n", buffer);

	// debug
	// printf("%s on line %d near \"%s\" #", s, linenumber_handle(*buffer_origin_handle), (const char *) *text_handle);

	free(buffer);
	return 0; }
} // extern "C"

void apcmd_install_symbols() {
	linenumber_handle = ap_linenumber;
	text_handle = (char **) &xmtext;
	}
