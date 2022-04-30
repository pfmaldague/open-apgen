//////////////////////////////////////////////////////////////////////////
// BKL altered/added to the LOG class to complete the runlog processing //
// 9-25-97							      //
//////////////////////////////////////////////////////////////////////////

#include "action_request.H"
#include "apcoreWaiter.H"
#include "TOL_write.H"
#include "ActivityInstance.H"

static const char* logfilename = "apgen.log";

int	Action_request::script_execution_level = 0;

aoString	&Log::theList() {
			static aoString L;
			return L; }

void set_log_file_to(const char *L) {
	static	char *s = NULL;

	if(s) {
		free(s); }

	s = (char *) malloc(strlen(L) + 1);
	strcpy(s, L);
	logfilename = s; }

Log&	Log::LOG() {
	static Log theLog;

	return theLog; }

Log::Log() {
	if(serveropt()) {
		fout = NULL; }
	else {
		fout = new ofstream(logfilename);
		if(fout) {
			ostringstream	B;

			if(APcloptions::theCmdLineOptions().aafLog) {
				B << "APGEN version \"" << get_apgen_version_build_platform() << "\"\n"; }
			else {
				B << "APGEN script version \"" << get_apgen_version_build_platform() << "\"\n"; }

			//
			// This interface sucks - replace when we have a little time
			//
			/* NOTE: get_header_info() returns a string ending with a newline */
			B << get_tol_header_info("", false);
			string header = B.str();
			string::size_type idx = header.find('\n');

			*fout << header.substr(0, idx) << "\n";
			idx++;
			string::size_type idx2 = header.find('\n', idx);
			while(idx2 != header.npos) {
				string chunk = header.substr(idx, idx2 - idx);
				*fout << "# " << chunk << endl;

				// debug
				// cout << "Log::Log: writing line # " << chunk << endl;

				idx = idx2 + 1;
				idx2 = header.find('\n', idx); }
			if(APcloptions::theCmdLineOptions().aafLog) {
				*fout << "script AAF_log() {\n"; }
			}
		else {
			cout << "APGEN script version \"" << get_apgen_version_build_platform() << "\"\n"; } } }

Log::~Log() {
	if(fout) {
		if(APcloptions::theCmdLineOptions().aafLog) {
			*fout << "}\n"; }
		fout->close();
		delete fout; } }

Log& Log::operator << (const char* buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << buf; }
	return *this; }

Log& Log::operator << (char buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		char b[2] = {'\0', '\0'};
		b[0] = buf;
		theList() << b; }
	return *this; }

Log& Log::operator << (short buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << Cstring(buf); }
	return *this; }

Log& Log::operator << (int buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << Cstring(buf); }
	return *this; }

Log& Log::operator << (long buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << Cstring((int) buf); }
	return *this; }

Log& Log::operator << (float buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << Cstring(buf); }
	return *this; }

Log& Log::operator << (double buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << Cstring(buf); }
	return *this; }

Log& Log::operator << (unsigned char buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << Cstring((unsigned long) buf); }
	return *this; }

Log& Log::operator << (unsigned short buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << Cstring(buf); }
	return *this; }

Log& Log::operator << (unsigned int buf) {
	if (fout) {
		*fout << ((long) buf) << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << ((long) buf) << flush; }
	else {
		theList() << Cstring((long) buf); }
	return *this; }

Log& Log::operator << (unsigned long buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << Cstring(buf); }
	return *this; }

Log& Log::operator << (void* buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	else {
		theList() << Cstring(buf); }
	return *this; }

Log& Log::operator << (streambuf* buf) {
	if (fout) {
		*fout << buf << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << buf << flush; }
	return *this; }

Log& Log::operator << (ios& (*func)(ios&)) {
	if (fout) {
		*fout << func << flush; }
	else if(!serveropt()) {
		/* CHECKED */ cout << func << flush; }
	return *this; }


Cstring add_pound_sign(const Cstring& S) {
	Cstring	ret;
	int	occs = S.OccurrencesOf("\n");
	int	L = S.length();
	int	indentation_level = Action_request::script_execution_level > 0 ? Action_request::script_execution_level - 1 : 0;
	char	*newstr = (char *) malloc(2 * indentation_level + 2 + L + 2 * occs + 1 + 1);
	char	*char_pointer = newstr;
	int	i;

	for(i = 0; i < indentation_level; i++) {
		*char_pointer++ = ' ';
		*char_pointer++ = ' '; }
	*char_pointer++ = '#';
	*char_pointer++ = ' ';
	for(i = 0; i < L; i++) {
		*char_pointer++ = S[i];
		if(S[i] == '\n' && S[i + 1]) {
			*char_pointer++ = '#';
			*char_pointer++ = ' '; } }
	*char_pointer++ = '\0';
	ret = newstr;
	free(newstr);
	return ret; }
