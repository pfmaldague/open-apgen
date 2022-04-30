#ifndef _fileReader_H
#define _fileReader_H

#include "AP_exp_eval.H"
#include "c_parser_bridge.h"
#include <action_request.H>

namespace pEsys {
class Program;
class pE;
}

// for parsing activity instances:
class instance_tag_PLD {
public:
	instance_tag_PLD() : act(NULL), invisibility(apgen::act_visibility_state::VISIBILITY_REGULAR) {}
			// for regular instances and chameleons:
	instance_tag_PLD(ActivityInstance*			p,
			apgen::act_visibility_state	v)
		: act(p), invisibility(v) {}
			// for requests:
	instance_tag_PLD(const Cstring&			n,
			const Cstring&			templ,
			const List&			atts,	// List of instructions
			apgen::act_visibility_state	v)
		: act(NULL), req_name(n), req_template(templ), invisibility(v), AttributeInstructions(atts) {}
	instance_tag_PLD(const Cstring&			n,
			const Cstring&			templ,
			const tvslist&			atts,
			apgen::act_visibility_state	v)
		: act(NULL), req_name(n), req_template(templ), invisibility(v), AttributeValues(atts) {}
	instance_tag_PLD(const instance_tag_PLD& dt)
		: act(dt.act), parents(dt.parents), toddlers(dt.toddlers),
			req_name(dt.req_name), invisibility(dt.invisibility),
			AttributeInstructions(dt.AttributeInstructions),
			AttributeValues(dt.AttributeValues) {}
	~instance_tag_PLD() {}

	ActivityInstance*		act;
	stringtlist			parents;
	stringtlist			toddlers;
	List				AttributeInstructions;	// List of instructions
	tvslist				AttributeValues;

					// Capture visibility level in instances' hierarchy
	apgen::act_visibility_state	invisibility;
	Cstring				req_template;
	Cstring				req_name;
};

typedef Cnode0<alpha_string, instance_tag_PLD> instance_tag;

class instance_pointer_PLD {
public:
	apgen::act_visibility_state	invisibility;
	instance_tag*			act;

	// NOTE: old Node was smart - may need to worry about that; hopefully not
	instance_pointer_PLD(instance_tag* p, apgen::act_visibility_state vis)
		: act(p), invisibility(vis)
				{}
	instance_pointer_PLD(const instance_pointer_PLD& pn)
		: act(pn.act), invisibility(pn.invisibility) {}
	~instance_pointer_PLD() {} };

typedef Cnode0<alpha_void, instance_pointer_PLD> instance_pointer;

class fileParser;
typedef Cnode0<alpha_int, fileParser>	parserNode;

class fileParser {
public:

/*	Nuts and bolts of parsing:
 *
 * 	parser			reads from	how invoked
 *	------			----------	-----------
 *	ADAPTATION_PARSER	string		ab_parse
 *	COMMAND_PARSER		string		zz_parse
 *	JSON_PARSER_IMCE	string		fileReader:parse_json_imce
 *	XML_PARSER_TOL_DATA	file		fileReader::parse_xml_stream
 *	XML_PARSER_UDEF		string		Execute_udef_function("file_converter"...)
 */

	// enum that's useful for picking the "right" parser (aaf/apf or command)
	typedef enum WHICH_PARSER {
		ADAPTATION_PARSER,	// AAF/APF parser
		COMMAND_PARSER,		// script file parser
		// JSON_PARSER_ACT_DATA,
		// JSON_PARSER_RES_SPEC,
		// JSON_PARSER_RES_DATA,
		JSON_PARSER_IMCE,
		XML_PARSER_TOL_DATA,
		XML_PARSER_UDEF
		} WHICH_PARSER;

	fileParser();
	~fileParser() {}

	static void				initializeTheReader(
							const Cstring&			Text,
							compiler_intfc::source_type&	InputSource,
							compiler_intfc::content_type&	ContentType,
							Cstring&			charsToParse);

	static void				exerciseTheReader(
							compiler_intfc::source_type	InputSource,
							compiler_intfc::content_type	ContentType,
							commandslist&			list_of_commands,
							Cstring&			charsToParse);

	static fileParser&			currentParser();

	static void				determine_file_content(
							const Cstring&			Text,
							compiler_intfc::source_type& 	inputType,
							Cstring&			charsToParse);

	WHICH_PARSER				which_parser;

	tlist<alpha_string, instance_tag>	Instances;	// indexed by actual act ID
	tlist<alpha_string, instance_tag>	ListOfActivityPointersIndexedByIDsInThisFile;
	stringtlist				theAncestors;
	stringtlist				theChildren;

	void					FixAllPointersToParentsAndChildren(
							bool use_pE);
};

class fileReader {
private:
	fileReader();
public:

	typedef enum act_viz {
		TBD,
		visible,
		abstracted,
		decomposed } act_viz;

	typedef enum act_sort {
		regular_act,
		request_act,
		chameleon_act } act_sort;

	~fileReader() {}

	bool			the_file_had_commands_in_it();
	static tlist<alpha_string, btokennode>&	Tokens();
	static tlist<alpha_string, btokennode>&	CMD_Tokens();
	static stringtlist&	resources_used_in_expression();
	static stringtlist&	listOfFilesReadThroughSeqReview();
	static stringstream&	theErrorStream();

	static void		parse_json_string(
					const Cstring& charsToParse);
	static OPEN_FILErequest* parse_json_imce(
					const Cstring& charsToParse);
	static void		parse_xml_stream(
					const Cstring& filename);
	static void		process_activity_instance(
					const CTime_base&	time_tag,
					act_viz			Visibility,
					act_sort		request_or_chameleon,
					tvslist&		the_attributes,
					tvslist&		the_parameters,
					const Cstring&		act_name,
					const Cstring&		act_type,
					const stringslist&	abstractableParent,
					const stringslist&	abstractedParent,
					const stringslist&	decomposableChildren,
					const stringslist&	decomposedChildren,
					Cstring&		ID_for_this_file);
	static void		CompileExpression(
					const Cstring&	Text,
					parsedExp& pc);
	static void		CompileStatement(
					const Cstring&	Text,
					pEsys::Program*	prog);
	static void		CompileExpressionList(
					const Cstring&	Text,
					slist<alpha_int, Cnode0<alpha_int, parsedExp> >& exps);
	static void		Evaluate(
					parsedExp&		S,
					behaving_element&	MA,
					TypedValue&		result);
	static void		EvaluateSelf(
					parsedExp&		S,
					TypedValue&		result);
	static void		CompileAndEvaluate(
					const Cstring&		exp_text,
					behaving_element&	MA,
		                        TypedValue&		result);
	static void		CompileAndEvaluateSelf(
					const Cstring&		exp_text,
		                        TypedValue&		result);
	static void		initialize_error_string();

	static apgen::RETURN_STATUS read_file_from_seq_review(
					const Cstring&		filename,
					Cstring&		translated_content,
					Cstring&		error_msg,
					const Cstring&		optional_format,
					const stringslist&	strips,
					const Cstring&		last_instruction) ;
	static apgen::RETURN_STATUS read_file_from_seq_review_pipe(
					const Cstring&		filename,
					Cstring&		translated_content,
					Cstring&		error_msg,
					const Cstring&		optional_format,
					const stringslist&	strips,
					const Cstring&		last_instruction);

	static Cstring		handle_environment(const Cstring&	s);
				// singleton pattern:
	static fileReader&	theFileReader();
	static slist<alpha_int, Cnode0<alpha_int, parsedExp> >& expressionList();
};

#endif /* _fileReader_H */
