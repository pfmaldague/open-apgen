#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// #define apDEBUG

#include "aafReader.H"
#include "c_parser_bridge.h"
#include "AAFlibrary.H"
#include "lex_intfc.H"
#include "apDEBUG.H"
#include "fileReader.H"
#include "apcoreWaiter.H"
#include "ActivityInstance.H"
#include "ACT_exec.H"
#include "action_request.H"
#include "CMD_exec.H"
// #include <ParsedExpressionSystem.H>
#include "RES_eval.H"
#include "RES_exec.H"

#include 	"json.h"

extern void	zz_cleanup_lex_buffer();
extern void	yy_cleanup_tokens_buffer();

// #include <grammar_intfc.H>

extern void	yzparse1(), yzparse2();

stringstream&	fileReader::theErrorStream() {
	static stringstream S;
	return S;
}

Cstring fileReader::handle_environment(const Cstring &S) {
	Cstring		T(S);
	Cstring		prefix, postfix, varname;
	char		*env_value;

	while(T & "$") {
		prefix = T / "$";
		postfix = "$" / T;
		if( postfix & "/" ) {
			varname = postfix / "/";
		} else {
			varname = postfix;
		}
		postfix = "/" / postfix;
		if((env_value = getenv(*varname))) {
			T = prefix;
			T << env_value;
			if( postfix.length() ) {
				T << "/" << postfix;
			}
		} else {
			return T;
		}
	}
	return T;
}

slist<alpha_int, Cntnr<alpha_int, parsedExp> >&	fileReader::expressionList() {
	static slist<alpha_int, Cntnr<alpha_int, parsedExp> > s;
	return s;
}

fileParser&	fileParser::currentParser() {
	static fileParser fp;
	return fp;
}

tlist<alpha_string, btokennode>&	fileReader::Tokens() {
	static tlist<alpha_string, btokennode> s;
	return s;
}

tlist<alpha_string, btokennode>&	fileReader::CMD_Tokens() {
	static tlist<alpha_string, btokennode> c;
	return c;
}

stringtlist&	fileReader::resources_used_in_expression() {
	static stringtlist c;
	return c;
}

stringtlist&	fileReader::listOfFilesReadThroughSeqReview() {
	static stringtlist c;
	return c;
}

fileReader::fileReader()
	{
}

fileReader& fileReader::theFileReader() {
	static fileReader f;
	return f;
}

fileParser::fileParser()
	: which_parser(fileParser::ADAPTATION_PARSER)
	{}

bool fileReader::the_file_had_commands_in_it() {
	return (fileParser::currentParser().which_parser == fileParser::COMMAND_PARSER
		|| fileParser::currentParser().which_parser == fileParser::JSON_PARSER_IMCE);
}


/* Althrough this method can throw an exception, many errors are simply
 * accumulated in compiler_intfc::CompilationErrors().  Clients should
 * try {} this method and then check CompilationErrors for unthrown
 * parsing errors. */
void fileParser::exerciseTheReader(
		compiler_intfc::source_type	InputType,
		compiler_intfc::content_type	inputSemantics,
		commandslist&			list_of_commands,
		Cstring&			chars_to_read) {
	if(currentParser().which_parser == COMMAND_PARSER) {
		commandslist::iterator theCommands(list_of_commands);

		if(InputType == compiler_intfc::FILE_NAME) {

			//
			// Read the file again. Remember, the first time
			// around (in determine_file_content) we only read
			// up to a Megabyte.
			//
			FILE*		f = fopen(*chars_to_read, "r");
			struct stat	FileStats;
			stat((char*)*chars_to_read, &FileStats);
			long contentLength = FileStats.st_size;
			char*	theString = new char[contentLength + 1];

			theString[contentLength] = '\0';
			fread(theString, 1, contentLength, f);
			fclose(f);

			fileReader::theErrorStream().str("");
			fileReader::theErrorStream().clear();
			zz_lex_buffers_initialize_to(theString);
			delete [] theString;
			CMD_exec::cmd_subsystem().set_list_of_commands_to(list_of_commands);
			try {
				zzparse();
			} catch(eval_error Err) {
				zz_cleanup_lex_buffer();
				throw(Err);
			}
			zz_cleanup_lex_buffer();
			string errs = fileReader::theErrorStream().str();
			if(errs.size()) {
				throw(eval_error(errs.c_str()));
			}
		} else {
			Cstring		temp(chars_to_read);

			removeQuotes(temp);
			list_of_commands << new OPEN_FILErequest(compiler_intfc::CONTENT_TEXT, temp);
		}
	} else if(fileParser::currentParser().which_parser == ADAPTATION_PARSER) {
		if(inputSemantics == compiler_intfc::ADAPTATION) {
			assert(chars_to_read.is_defined());
			try {
				aafReader::readAdaptationFile(InputType, *chars_to_read);
			} catch(eval_error Err) {
				throw(Err);
			}
		} else if(inputSemantics == compiler_intfc::EXPRESSION) {
			assert(chars_to_read.is_defined());
			try {
				aafReader::readAdaptationFile(InputType, *chars_to_read);
			} catch(eval_error Err) {
				throw(Err);
			}
		}
	} else if(fileParser::currentParser().which_parser == XML_PARSER_TOL_DATA) {
		assert(InputType == compiler_intfc::FILE_NAME);
		try {
			fileReader::parse_xml_stream(chars_to_read);
		} catch(eval_error Err) {
			throw(Err);
		}
	} else if(fileParser::currentParser().which_parser == XML_PARSER_UDEF) {
		TypedValue			contents;
		TypedValue			option;
		TypedValue			result;
		slst<TypedValue*>		args;

		if(InputType == compiler_intfc::FILE_NAME) {
			FILE* f = fopen(*chars_to_read, "r");
			struct stat	FileStats;
			stat((char*)*chars_to_read, &FileStats);
			long contentLength = FileStats.st_size;
			char*	theString = new char[contentLength + 1];
			theString[contentLength] = '\0';
			fread(theString, 1, contentLength, f);
			fclose(f);

			contents = theString;
			delete [] theString;
		} else {
			contents = *chars_to_read;
		}
		// OK, we can use this method to parse the file
		option = "parse";
		TypedValue* option_arg = &option;
		TypedValue* contents_arg = &contents;
		args.push_front(option_arg);
		args.push_front(contents_arg);
		// can throw:
		Execute_udef_function("file_converter", &result, args);
	} else if(fileParser::currentParser().which_parser == JSON_PARSER_IMCE) {
		Cstring contents;
		if(InputType == compiler_intfc::FILE_NAME) {
			FILE* f = fopen(*chars_to_read, "r");
			struct stat	FileStats;
			stat((char*)*chars_to_read, &FileStats);
			long contentLength = FileStats.st_size;
			char*	theString = new char[contentLength + 1];
			theString[contentLength] = '\0';
			fread(theString, 1, contentLength, f);
			fclose(f);

			contents = theString;
			delete [] theString;
		} else {
			contents = chars_to_read;
		}
		try {
			OPEN_FILErequest* of = fileReader::parse_json_imce(contents);
			list_of_commands << of;
		} catch(eval_error Err) {
			throw(Err);
		}
	}
}

void fileReader::CompileExpression(
		const Cstring&	Text,
		parsedExp&	S) {
	static Cstring			err;
	compiler_intfc::content_type	ct = compiler_intfc::EXPRESSION;

	if(!Text.length()) {
		throw(eval_error("CompileExpression: attempting to compile an empty string\""));
	}
	Cstring chars_to_read;
	int	N = aafReader::input_files().size();
	try {
		aafReader::current_file() = "Standalone Expression";
		compiler_intfc::source_type inputMedium = compiler_intfc::CONTENT_TEXT;
		// can throw:
		fileParser::initializeTheReader(
			Text,
			inputMedium,
			ct,
			chars_to_read);
		commandslist dummyList(NULL);
		// can throw:
		fileParser::exerciseTheReader(
			compiler_intfc::CONTENT_TEXT,
			ct,
			dummyList,
			chars_to_read);
		assert(aafReader::input_files().size() == N+1);
		S = aafReader::input_files()[N];
		aafReader::input_files().pop_back();
		aafReader::consolidate_expression(S, 0);
	}
	catch(eval_error Err) {
		Cstring errs;
		errs << "fileReader::CompileExpression() error:\n" << Err.msg;
		S.dereference();
		throw(eval_error(errs));
	}
}

void fileReader::CompileStatement(
		const Cstring& Text,
		pEsys::Program* S) {
	compiler_intfc::content_type	ct = compiler_intfc::STATEMENT;

	if(!Text.length()) {
		throw(eval_error("CompileStatement: attempting to compile an empty string\""));
	}
	Cstring chars_to_read;
	compiler_intfc::source_type inputMedium = compiler_intfc::CONTENT_TEXT;
	fileParser::initializeTheReader(
			Text,
			inputMedium,
			ct,
			chars_to_read);
#ifdef PREMATURE
	try {
		commandslist dummyList(NULL);
		P.exerciseTheReader(ct, dummyList, chars_to_read);
		pEsys::ParsedInstruction* pi = dynamic_cast<pEsys::ParsedInstruction*>(
						fileParser::currentParser().capturedExpression.object());
		assert(pi);
		(*S) << pi->my_instruction;
		pi->my_instruction = NULL;
	}
	catch(eval_error Err) {
		// pEsys::stack().reset();
		Cstring tmp("fileReader::CompileStatement() error:\n");
		tmp << Err.msg;
		throw(eval_error(tmp));
	}
#endif /* PREMATURE */
}

void fileReader::CompileExpressionList(
		const Cstring& Text,
		slist<alpha_int, Cntnr<alpha_int, parsedExp> >& L) {
	compiler_intfc::content_type	ct = compiler_intfc::EXPRESSION_LIST;

	if(!Text.length()) {
		throw(eval_error("CompileExpressionList: attempting to compile an empty string\""));
	}
	fileReader::expressionList().clear();
	Cstring chars_to_read;
	compiler_intfc::source_type inputMedium = compiler_intfc::CONTENT_TEXT;
	fileParser::initializeTheReader(
			Text,
			inputMedium,
			ct,
			chars_to_read);
	try {
		commandslist dummyList(NULL);
		fileParser::exerciseTheReader(
			compiler_intfc::CONTENT_TEXT,
			ct,
			dummyList,
			chars_to_read);
	}
	catch(eval_error Err) {
		// pEsys::stack().reset();
		Cstring tmp;
		tmp << "fileReader::CompileExpressionList() error:\n" << Err.msg;
		throw(eval_error(tmp));
	}
	L = fileReader::expressionList();
}

void fileReader::Evaluate(
		parsedExp&		S,
		behaving_element&	ST,
		TypedValue&		result) {
        try { 
                S->eval_expression(ST.object(), result);
	}
        catch(eval_error Err) {
		Cstring	errors;
                errors = "Error in expression ";
                S->print(errors);
		errors << ":\n";
                errors << Err.msg;
                throw(eval_error(errors));
	}
}

void fileReader::EvaluateSelf(
		parsedExp&		S,
		TypedValue&		result) {
        try { 
		S->eval_expression(behaving_object::GlobalObject(), result);
	}
        catch(eval_error Err) {
		Cstring	errors;
                errors = "Error in expression ";
                S->print(errors);
		errors << ":\n";
                errors << Err.msg;
                throw(eval_error(errors));
	}
}

void fileReader::CompileAndEvaluateSelf(
		const Cstring&		exp_text,
		TypedValue&		result) {
        parsedExp       Expression;

	if(!exp_text.length()) {
		throw(eval_error("CompileAndEvaluateSelf: attempting to compile an empty string\""));
	}
        try {
		CompileExpression(exp_text, Expression);
                Expression->eval_expression(behaving_object::GlobalObject(), result);
	}
	catch(eval_error Err) {
        	Cstring         errors;

		errors << "Expression " << exp_text << " has errors:\n" << Err.msg;
                throw(eval_error(errors));
	}
}

void fileReader::CompileAndEvaluate(
			const Cstring&		exp_text,
                        behaving_element&	ST,
                        TypedValue&		result) {
        // let's use apgen64's string_to_valid_time() function in ACT_exec.C as a template
        parsedExp       Expression;

	if(!exp_text.length()) {
		throw(eval_error("CompileAndEvaluate: attempting to compile an empty string\""));
	}

        try {
		CompileExpression(exp_text, Expression);
                Expression->eval_expression(ST.object(), result);
	}
        catch(eval_error Err) {
		Cstring	errors;
                errors << "Error in expression " << exp_text << ":\n" << Err.msg;
                throw(eval_error(errors));
	}
}

apgen::RETURN_STATUS fileReader::read_file_from_seq_review(
		const Cstring&	filename,
		Cstring&	new_file,
		Cstring&	error_msg,
		const Cstring&	optional_format,
		const stringslist& strips,
		const Cstring&	last_instruction) {
	if(read_file_from_seq_review_pipe(	filename,
		    				new_file,
						error_msg,
						optional_format,
						strips,
						last_instruction) != apgen::RETURN_STATUS::SUCCESS) {
		return apgen::RETURN_STATUS::FAIL;
	} // bad format
	return apgen::RETURN_STATUS::SUCCESS;
}

	/* The following function is used both for reading adaptation files
	 * and for creating filtered TOL output, whence the extra parameters
	 * containing optional formats/strips. */
apgen::RETURN_STATUS fileReader::read_file_from_seq_review_pipe(
		const Cstring&	filename,
		Cstring&	new_file,
		Cstring&	error_msg,
		const Cstring&	optional_format,
		const stringslist& strips,
		const Cstring&	last_instruction) {
	aoString	converted_file;
	Cstring		scriptline;
	Cstring		command;
	Cstring		true_filename(filename);
	char*		val;
	int		method = 0;
	FILE*		F;
	char		buffer[2000];
	int		chars_read;
	emptySymbol*	sn;

	// first make sure a config file is there
	if((val = getenv("seq_review_cfg"))) {
		method = 1;
		F = fopen(val, "r");
		if(filename == "seq_review.cfg") true_filename = val;
	} else {
		method = 2;
		F = fopen("seq_review.cfg", "r");
	}

	if(!F) {		// no config file
		error_msg = Cstring("Trying to use seq_review to read file ") << true_filename <<
			", since it is not a plan/adaptation/constraint file; however, ";
		if(method == 1)
			error_msg << "environment variable \"seq_review_cfg\", "
				<< "which is set to \"" << val << "\", is not a valid file name. #";
		else
			error_msg << "environment variable \"seq_review_cfg\" is not defined and "
				<< "there is no file named \"seq_review.cfg\" in the current directory. #";
		return apgen::RETURN_STATUS::FAIL;
	}			// report that file is in wrong format
	fclose(F);
	// next try to write a script for seq_review to execute:
	F = fopen("apgen.scr", "w");
	if(!F) {		// cannot write script file
		error_msg = Cstring("Trying to use seq_review to read file ") << true_filename <<
			", since it is not a plan/adaptation/constraint file; however, "
			<< "seq_review script cannot be generated because "
			<< "current directory is not writable. #";
		return apgen::RETURN_STATUS::FAIL;
	}			// report that file is in wrong format
	scriptline = "file open " + true_filename + "\n";
	fwrite((char *) *scriptline, scriptline.length(), 1, F);
	for(	sn = strips.first_node();
		sn;
		sn = sn->next_node()) {
		scriptline = sn->get_key() + "\n";
		fwrite((char *) *scriptline, scriptline.length(), 1, F);
	}
	if(optional_format.length()) {
		scriptline = Cstring("CATALOG FORMAT ") + optional_format + "\n";
		fwrite((char *) *scriptline, scriptline.length(), 1, F);
	}
	// OLD:
	// scriptline = "catalog script convert_to_apgen_format\n";
	// NEW:
	if(last_instruction.length()) {
		scriptline = last_instruction;
		fwrite((char *) *scriptline, scriptline.length(), 1, F);
	}
	scriptline = "file exit nosave\n";
	fwrite((char *) *scriptline, scriptline.length(), 1, F);
	fclose(F);

	// next look for an executable and pipe command to it
	// But first, make sure the executable is there:
	F = NULL;
	if((val = getenv("APGEN_SEQ_REVIEW")))
		F = fopen(val, "rb");
	if(!val) {		// cannot run seq_review
		error_msg = Cstring("Trying to use seq_review to read file ") << true_filename <<
			", since it is not a plan/adaptation/constraint file; however, ";
		error_msg << "environment var. APGEN_SEQ_REVIEW is not defined. #";
		// UI_subsystem->changeCursor(-1);
		return apgen::RETURN_STATUS::FAIL;
	} else if(!F) {
		// report that file is in wrong format
		error_msg = Cstring("Trying to use seq_review to read file ") << true_filename <<
			", since it is not a plan/adaptation/constraint file; however, ";
		error_msg << "environment var. APGEN_SEQ_REVIEW points to a non-existent executable. #";
		// UI_subsystem->changeCursor(-1);
		return apgen::RETURN_STATUS::FAIL;
	}			// report that file is in wrong format
	fclose(F);

	// Now look for an executable and pipe command to it

	command = Cstring(val) + " -batch -noexit -script apgen.scr";
	F = popen(*command, "r");
	while((chars_read = fread(buffer, 1, 1999, F))) {
		buffer[chars_read] = '\0';
		converted_file << buffer;
	}
	new_file = converted_file.str();
	pclose(F);
	if(! (new_file & "APGEN START READING HERE>>>>>>>>>>>>")) {
		error_msg = Cstring("Error found while using seq_review to read file ") << true_filename <<
			", which is not a plan/adaptation/constraint file. " << "None of the format descriptor(s) "
			<< "in the directory pointed to by environment variable \"apgen_convert_dir\" seems to apply. "
			<< "Please verify that this variable is set to a valid directory and/or try running seq_review "
			<< "interactively to identify format problem. #";
		// UI_subsystem->changeCursor(-1);
		return apgen::RETURN_STATUS::FAIL;
	}
	system("rm -f apgen.scr std.scr seq_review.logfile seq_review.logfile.bak");
	new_file = "APGEN START READING HERE>>>>>>>>>>>>" / new_file;
	new_file = new_file / "<<<<<<<<<<<<APGEN STOP READING HERE";
	return apgen::RETURN_STATUS::SUCCESS;
}


void fileParser::initializeTheReader(
  const Cstring&		theInput,	// the input string
  compiler_intfc::source_type&	inputMedium,	// FILE_NAME or CONTENT_TEXT
  compiler_intfc::content_type&	inputSemantics,	// UNKNOWN or ADAPTATION
						// or EXPRESSION or EXPRESSION_LIST
						// or STATEMENT or SCRIPT
  Cstring&			stringToHandOverToParser
  ) {
	if(inputMedium == compiler_intfc::CONTENT_TEXT) {
		;
	} else if(inputMedium == compiler_intfc::FILE_NAME) {
		if(inputSemantics != compiler_intfc::UNKNOWN) {
			Cstring		error_msg;
			error_msg = "initializeTheReader: reading from file, but content type "
				"is known; do not know how to handle that\n";
			throw(eval_error(error_msg));
		}
	}

	switch(inputSemantics) {
	    case compiler_intfc::UNKNOWN:

		//
		// The call to determine_file_content() will set which_parser
		// to the correct parser, based on the contents of the file.
		// It will also return either a string containing the text
		// to be fed to the chosen parser, or a file name containing
		// the name of a valid file from which the parser should
		// extract the data to be parsed.  In most cases, the first
		// option is used.  When the input file is an XMLTOL, it is
		// often the case (Europa project) that the file is huge
		// (several Gigabytes) in which case it is important that
		// the parser should only read a little bit of information
		// at a time.
		//
		// If inputMedium is CONTENT_TEXT, that's all we need to know.
		// Note that in that case the parser will be either the
		// command or the adaptation parser; XML or Json input
		// cannot be provided as a string - only as a file.
		//
		// If inputMedium is FILE_NAME, the call will ascertain that
		// the file is valid (it will throw if not).
		//
		// If the file is in a foreign format understood by
		// seq_review, the call will also return the string which
		// was obtained from the seq_review translation process;
		// the selected parser should then use that string as the
		// text to parse.
		//
		// Otherwise, the selected parser is responsible for opening
		// the file (the call only reads at most 1 Meg from the file)
		// and read its contents.
		//

		try {
		    determine_file_content(
				theInput,
				inputMedium,
				stringToHandOverToParser); // not undefined when returning
		} catch(eval_error Err) {
		    throw(Err);
		}
		if(currentParser().which_parser == COMMAND_PARSER) {
		    inputSemantics = compiler_intfc::SCRIPT;
		} else if(currentParser().which_parser == ADAPTATION_PARSER) {
		    inputSemantics = compiler_intfc::ADAPTATION;
		}
		break;
	    case compiler_intfc::ADAPTATION:
		stringToHandOverToParser = theInput;
		currentParser().which_parser = ADAPTATION_PARSER;
		break;
	    case compiler_intfc::EXPRESSION:
		stringToHandOverToParser = theInput;
		currentParser().which_parser = ADAPTATION_PARSER;
		break;
	    case compiler_intfc::EXPRESSION_LIST:
		stringToHandOverToParser = theInput;
		currentParser().which_parser = ADAPTATION_PARSER;
		break;
	    case compiler_intfc::STATEMENT:
		stringToHandOverToParser = theInput;
		currentParser().which_parser = ADAPTATION_PARSER;
		break;
	    case compiler_intfc::SCRIPT:
		stringToHandOverToParser = theInput;
		currentParser().which_parser = COMMAND_PARSER;
		break;
	    default:
		assert(false);
	}

	if(currentParser().which_parser == COMMAND_PARSER) {
		// in CMD_intfc.C:
		reset_error_flag();
		CMD_exec::cmd_subsystem().clear();
		FIRST_call = 0;
		if(!fileReader::CMD_Tokens().get_length()) {
		    cmd_initialize_tokens();
		}
	} else if(currentParser().which_parser == ADAPTATION_PARSER) {
		FIRST_call = 0;
		if(inputSemantics == compiler_intfc::EXPRESSION) {
		    FIRST_call = 1;
		} else if(inputSemantics == compiler_intfc::STATEMENT) {
		    FIRST_call = 2;
		} else if(inputSemantics == compiler_intfc::EXPRESSION_LIST) {
		    FIRST_call = 3;
		}
		currentParser().ListOfActivityPointersIndexedByIDsInThisFile.clear();
		currentParser().Instances.clear();
	} else {

		//
		// we need to pass stringToHandOverToParser
		// or fileToParse to the appropriate parser
		//
	}
}

static bool look_for_top_element(
		const char* text_to_parse,
		Cstring& topElement) {

	// skip over white space

	const char*	s = text_to_parse;

	while(	*s == '\n' ||
		*s == ' ' ||
		*s == '\t' ||
		*s == '\r' ) {
		s++; }

	// get to the first element; this is the one that says <?xml ... >

	while(*s && *s != '<') s++;
	s++;

	// get the next element

	while(*s && *s != '<') s++;
	s++;
	if(!*s) {
		return false;
	}
	const char* t = s;
	while(*s && *s != '>' && *s != ' ' && *s != '\t') s++;
	if(!*s) {
		return false;
	}
	char* top_level_tag = (char *) malloc(s - t + 1);
	strncpy(top_level_tag, t, s - t);
	top_level_tag[s - t] = '\0';
	topElement = top_level_tag;
	free(top_level_tag);
	return true;
}

/* inputMedium = compiler_intfc::FILE_NAME: string is name of a file
 * inputMedium = compiler_intfc::CONTENT_TEXT: string is text to be parsed
 *
 * This method is only called when processing an OPEN_FILErequest. In that
 * context, it is not known ahead of time what the file contains; it could be
 * adaptation code, an apgen script, an XMLTOL to be parser as activities and
 * resource values, or a JSON array to be parsed as a set of activities. The
 * method figures out which parser should be used.
 */

/* NOTE: if inputMedium == CONTENT_TEXT, it is assumed that the content
 *	 is either an APGen script or a snippet of adaptation. It cannot be
 *	 XML nor Json.
 */

void fileParser::determine_file_content(
		const Cstring&			theInput,	 // File name or data to parse

		compiler_intfc::source_type&	inputMedium,	 // FILE_NAME or CONTENT_TEXT

		Cstring&			chars_for_parser // Name of the file containing the data,
								 // or the data itself
		) {
	if(inputMedium == compiler_intfc::FILE_NAME) {
		Cstring		actual_filename, filename(theInput);
		Cstring		firstNonBlankLineFromFile;
		Cstring		firstMegabyteFromFile;
		struct stat	FileStats;
		const char	*start_here;
		FILE*		PARSER_inputFile;
		long int	PARSER_stringLength;

		chars_for_parser.undefine();

		// 1. handle any environment variables embedded in the file name

		actual_filename = fileReader::handle_environment(filename);

		// 2. open the file

		PARSER_inputFile = fopen((char *) *actual_filename, "r");
		if(!PARSER_inputFile) {
			Cstring any_errors;
			any_errors = Cstring("File ") << actual_filename << " not found #";
			throw(eval_error(any_errors));
		}

		// 3. get file stats, limit what we read from the file to 1 Meg

		if(stat((char*)*actual_filename, &FileStats)) {
			Cstring any_errors;
			any_errors = Cstring("Cannot get size of file ") << actual_filename;
			delete PARSER_inputFile;
			throw(eval_error(any_errors));
		}
		PARSER_stringLength = (long int) FileStats.st_size;
		if(PARSER_stringLength > 1024*1024) {
			PARSER_stringLength = 1024*1024;
		}

		// 4. read some reasonable amount of data from the file

		char* Str = (char*) malloc(PARSER_stringLength + 1);
		if(fread(Str, 1, PARSER_stringLength, PARSER_inputFile) != PARSER_stringLength) {
			Cstring any_errors;
			any_errors = Cstring("Cannot read ");
			any_errors << PARSER_stringLength << " chars from file "
				<< actual_filename;
			fclose(PARSER_inputFile);
			free(Str);
			throw(eval_error(any_errors));
		}

		Str[PARSER_stringLength] = '\0';
		fclose(PARSER_inputFile);

		// 5. skip over white space

		Cstring temp(Str, /* permanent = */ true);
		start_here = *temp;

		// skip over white space:
		while(	*start_here == '\n' ||
			*start_here == ' ' ||
			*start_here == '\t' ||
			*start_here == '\r' )
			start_here++;

		//
		// restrict search to the first line -
		// (these strange manipulations are intended to avoid copying the input string)
		//
		temp = Cstring(start_here, true);
		firstNonBlankLineFromFile = temp / "\n";
		firstMegabyteFromFile = Cstring(Str, /* permanent = */ true);


		// 6. look for "apgen" keyword, in case it's an adaptation or script file

		//
		// NOTE: at this point:
		//	firstNonBlankLineFromFile contains the first non-blank line of the file
		//	firstMegabyteFromFile contains the first Meg (at most) of file data
		//

		bool found_apgen_header = false;
		if(	(firstNonBlankLineFromFile & "apgen")
			|| (firstNonBlankLineFromFile & "APGEN")) {
			// we got it
			if(	(firstNonBlankLineFromFile & "script")
				|| (firstNonBlankLineFromFile & "SCRIPT")) {
				currentParser().which_parser = COMMAND_PARSER;
				chars_for_parser = actual_filename;
				found_apgen_header = true;
			} else if((firstNonBlankLineFromFile & "version")
				|| (firstNonBlankLineFromFile & "VERSION")) {
				currentParser().which_parser = ADAPTATION_PARSER;
				chars_for_parser = actual_filename;
				found_apgen_header = true;
			}
		}

		// 7. look for XML

		if(found_apgen_header) {

			//
			// we are happy
			//
			free(Str);
		} else if(firstNonBlankLineFromFile & "<?xml") { // it's XML
			Cstring	topElement;

			if(look_for_top_element(*firstMegabyteFromFile, topElement)) {
				if(topElement == "XML_TOL") {
					currentParser().which_parser = XML_PARSER_TOL_DATA;
					chars_for_parser = actual_filename;
				} else if(aaf_intfc::internalAndUdefFunctions().find("file_converter")) {
					// try using other user-defined XML parsers
					slst<TypedValue*>		args;
					TypedValue			result;
					TypedValue			option;
					TypedValue			top_element_name;

					option = "check";
					top_element_name = topElement;
					TypedValue* option_arg = &option;
					TypedValue* top_element_name_arg = &top_element_name;
					args.push_front(option_arg);
					args.push_front(top_element_name_arg);
					Execute_udef_function(
							"file_converter",
							&result,
							args);
					if(result.is_numeric() && result.get_int()) {
						currentParser().which_parser = XML_PARSER_UDEF;
					} else {
						Cstring any_errors;
						free(Str);
						any_errors = "file appears to be XML "
							"but top-level element has unknown type ";
						any_errors << topElement << ".\n";
						throw(eval_error(any_errors));
					}
					chars_for_parser = actual_filename;
				}
			}
			free(Str);
		} else if(firstNonBlankLineFromFile == "{") {
			free(Str);
			chars_for_parser = actual_filename;
			currentParser().which_parser = JSON_PARSER_IMCE;
		} else { // try seq_review
			stringslist	empty;
			Cstring	after_translation;
			Cstring	any_errors;

			free(Str);
			if(fileReader::read_file_from_seq_review(
						actual_filename,
						after_translation,
						any_errors,
						"",
						empty,
						"catalog script convert_to_apgen_format\n" )
					!= apgen::RETURN_STATUS::SUCCESS ) {
				throw(eval_error(any_errors));
			} // bad format

			// IT WORKED!! IT WORKED!!
			inputMedium = compiler_intfc::CONTENT_TEXT;
			if(!fileReader::listOfFilesReadThroughSeqReview().find(actual_filename))
				fileReader::listOfFilesReadThroughSeqReview()
					<< new emptySymbol(actual_filename);
			chars_for_parser = after_translation;
			start_here = *chars_for_parser;
			// skip over white space:
			while(	*start_here == '\n' ||
					*start_here == ' ' ||
					*start_here == '\t' ||
					*start_here == '\r' )
				start_here++ ;
			if((!strncmp(start_here, "apgen", 5))
			   || (!strncmp(start_here, "APGEN",5))) {	// we got it
				start_here += 6;
				while(*start_here == ' ' || *start_here == '\t') start_here++;
				if((!strncmp(start_here, "script", 6))
				   || (!strncmp(start_here, "SCRIPT", 6))) {
					currentParser().which_parser = COMMAND_PARSER;
				} else {
					currentParser().which_parser = ADAPTATION_PARSER;
				}
			}
		}
	} else if(inputMedium == compiler_intfc::CONTENT_TEXT) {
		chars_for_parser = theInput;
		// 'case' was missing prior to 4/12/2011. Amazing that this worked;
		// I guess apgen.log uses caps...
		if(!strncasecmp(*theInput, "APGEN script", strlen("APGEN script"))) {
			// We are compiling a string with the command parser
			currentParser().which_parser = COMMAND_PARSER;
		} else {
			// We are compiling a string with the adaptation parser
			const char *s, *start_here = *theInput;

			// skip over whitespace
			s = start_here;
			while(*s && (*s == ' ' || *s == '\n' || *s == '\t')) s++;
			// check for XML
			if(!strncmp(s, "<?xml", strlen("<?xml"))) {
				Cstring converted_string;
				Cstring top_element;

				if(	look_for_top_element(s, top_element)
					&& top_element == "XML_TOL") {
					currentParser().which_parser = XML_PARSER_TOL_DATA;
				}
			} else {
				currentParser().which_parser = ADAPTATION_PARSER;
			}
		}
	}
}

//
// After executing the function below, either there are errors in the
// parent/child pointers of the activity instances read from the input
// file, or all activity instances will have been created with the
// appropriate pointers to their children and to their parent.
//
// The logic in this function is based on the data in instance_tags
// stored in the list of activity pointers that populated while
// reading the input file. Even after processing, we must keep these
// pointers around, because they are the only place where visibiliy
// information (i. e., whether an instance is abstracted, active or
// decomposed) is stored. It is only after calling
// ACT_exec::instantiate_all() that it will be safe to delete the list.
//
void fileParser::FixAllPointersToParentsAndChildren(
		bool		use_attr_instructions) {
	instance_tag*					theInstanceTag;
	instance_tag*					theChildTag;
	instance_tag*					theParentTag;
	slist<alpha_string, instance_tag>::iterator	all_activity_nodes(ListOfActivityPointersIndexedByIDsInThisFile);
	emptySymbol*					bs;
	symNode*					theTag;
	// slist<alpha_void, instance_pointer>::iterator	the_requests(allRequests);
	instance_pointer*				act_ptr;
	ActivityInstance*				request = NULL;

	// DO THE REQUESTS FIRST!!

	// I. Check that all items in parent lists point to a valid instance pointer
	while((theInstanceTag = all_activity_nodes())) {
		request = theInstanceTag->payload.act;
		assert(request);
		smart_ptr<pEsys::ActInstance>& parsed_request = request->obj()->parsedThis;
		bool first = true;

		stringslist::iterator		p(theInstanceTag->payload.parents);

		while((bs = p())) {
			if(first) {
				first = false;
			} else {
				Cstring errs;
				errs << "Hierarchy conflict - File " << parsed_request->file << ", line "
					<< parsed_request->line << ": instance "
					<< theInstanceTag->get_key() << " has more than one parent:\n";
				stringslist::iterator p2(theInstanceTag->payload.parents);
				while((bs = p2())) {
					errs << "\t" << bs->get_key() << "\n";
				}
				throw(eval_error(errs));
			}
				
			theParentTag = ListOfActivityPointersIndexedByIDsInThisFile.find(bs->get_key());
			if(!theParentTag) {
				Cstring errs;
				errs << "Hierarchy conflict - File " << parsed_request->file << ", line "
					<< parsed_request->line << ": instance "
					<< theInstanceTag->get_key() << " has a non-existent parent.";
				throw(eval_error(errs));
			}
			emptySymbol* es2 = theParentTag->payload.toddlers.find(theInstanceTag->get_key());
			if(!es2) {
				Cstring errs;
				errs << "Hierarchy conflict - File " << parsed_request->file << ", line "
					<< parsed_request->line << ": instance "
					<< theInstanceTag->get_key() << " claims parent " << bs->get_key() << " but "
					<< bs->get_key() << " does not list " << theInstanceTag->get_key()
					<< " as a child.";
				throw(eval_error(errs));
			}
		}
	}

	// III. Attach children to non-request parents
	while((theInstanceTag = all_activity_nodes())) {
		stringslist::iterator		t(theInstanceTag->payload.toddlers);

		request = theInstanceTag->payload.act;
		smart_ptr<pEsys::ActInstance>& parsed_request = request->obj()->parsedThis;
		if(!theInstanceTag->payload.parents.get_length()) {
			if(theInstanceTag->payload.invisibility == apgen::act_visibility_state::VISIBILITY_ABSTRACTED) {
				Cstring errs;
				errs << "Visibility conflict - File " << parsed_request->file << ", line "
					<< parsed_request->line << ": instance "
					<< theInstanceTag->get_key() << " invisible.";
				throw(eval_error(errs));
			}
		}
		while((bs = t())) {
			bool	no_errors = true;

			theChildTag = ListOfActivityPointersIndexedByIDsInThisFile.find(bs->get_key());
			if(!theChildTag) {
				no_errors = false;
				Cstring errs;
				errs << "Hierarchy conflict - File " << parsed_request->file << ", line "
					<< parsed_request->line << ": instance "
					<< theInstanceTag->get_key()
					<< " has undefined child " << bs->get_key() << ".";
				throw(eval_error(errs));
			} else {
				// check that visibility is consistent between parent and child...
				if(theInstanceTag->payload.invisibility == apgen::act_visibility_state::VISIBILITY_REGULAR) {
					if(theChildTag->payload.invisibility != apgen::act_visibility_state::VISIBILITY_ABSTRACTED) {
						no_errors = false;
						Cstring errs;
						errs << "File " << parsed_request->file << ", line "
							<< parsed_request->line
							<< ": Visibility conflict between instance "
							<< theInstanceTag->get_key()
							<< " and child " << theChildTag->get_key() << ".";
						throw(eval_error(errs));
					}
				} else if(theInstanceTag->payload.invisibility == apgen::act_visibility_state::VISIBILITY_DECOMPOSED) {
					if(theChildTag->payload.invisibility == apgen::act_visibility_state::VISIBILITY_ABSTRACTED) {
						no_errors = false;
						Cstring errs;
						errs << "File " << parsed_request->file << ", line "
							<< parsed_request->line
							<< ": Visibility conflict between instance "
							<< theInstanceTag->get_key()
							<< " and child " << theChildTag->get_key() << ".";
						throw(eval_error(errs));
					}
				} else if(theInstanceTag->payload.invisibility == apgen::act_visibility_state::VISIBILITY_ABSTRACTED) {
					if(theChildTag->payload.invisibility != apgen::act_visibility_state::VISIBILITY_ABSTRACTED) {
						no_errors = false;
						Cstring errs;
						errs << "File " << parsed_request->file << ", line "
							<< parsed_request->line
							<< ": Visibility conflict between instance "
							<< theInstanceTag->get_key()
							<< " and child " << theChildTag->get_key() << ".";
						throw(eval_error(errs));
					}
				}
				assert(theChildTag->payload.act);
			}
			if(no_errors) {
				if(theChildTag->payload.act == request) {
					no_errors = false;
					Cstring errs;
					errs << "File " << parsed_request->file << ", line "
						<< parsed_request->line
						<< ": Instance " << theChildTag->get_key()
						<< " probably has errors; it points to itself as a parent.";
					throw(eval_error(errs));
				} else {

					//
					// This is overkill - all do_it does is call attach to parent:
					// instance_state_changer	ISC(theChildTag->payload.act);
					//
					// ISC.set_desired_parent_to(request);
					// ISC.do_it(NULL);

					theChildTag->payload.act->hierarchy().attach_to_parent(request->hierarchy());
				}
			}
		}
	}
	// IV. Check consistency of parent-child relation
	while((theInstanceTag = all_activity_nodes())) {
		ActivityInstance*	parent;

		request = theInstanceTag->payload.act;
		smart_ptr<pEsys::ActInstance>& parsed_request = request->obj()->parsedThis;


		if((parent = request->get_parent())) {
			smart_actptr*			child_ptr;
			slist<alpha_void, smart_actptr>::iterator sub(
					parent->get_down_iterator());

			while((child_ptr = sub())) {
				if(child_ptr->BP == request) {
					break;
				}
			}
			if(!child_ptr) {
				Cstring errs;
				errs << "File " << parsed_request->file << ", line "
					<< parsed_request->line
					<< ": activity \"" << request->identify()
					<< "\" has a parent that refuses to acknowledge its existence.";
				throw(eval_error(errs));
			}
		}
	}
}
