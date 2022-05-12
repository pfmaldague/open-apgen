#ifndef _COMPILER_INTFC_H_
#define _COMPILER_INTFC_H_
#include <ParsedExpressionSystem.H>

class compiler_intfc {
public:
	compiler_intfc() {}
	~compiler_intfc() {}

	typedef enum {
		UNKNOWN,
		ADAPTATION,
		SCRIPT,
		EXPRESSION,
		STATEMENT,
       		EXPRESSION_LIST	} content_type;
	typedef enum {
		FILE_NAME,
		CONTENT_TEXT } source_type;

		/* NOTE: The methods in here are function pointers so we can
		 *	 install them at a highly derived level of the class
		 *	 hierarchy */

	static void		(*CompileExpression)(
					const Cstring&	Text,
					parsedExp&	opaqueParsedObject);
	static void		(*CompileStatement)(
					const Cstring&	Text,
					pEsys::Program*	opaqueParsedObject);
	static void		(*CompileExpressionList)(
					const Cstring&	Text,
					slist<alpha_int, Cntnr<alpha_int, parsedExp> >&	L);
				/* we certainly need some context here
				 * so local symbols can be obtained. Assume
				 * a behaving element is available. The
				 * opaqueParsedObject should store its own
				 * value. */
	static void		(*Evaluate)(
					parsedExp&	opaqueParsedObject,
					behaving_element& MA,
					TypedValue&	result);
				// no local context provided
	static void		(*EvaluateSelf)(
					parsedExp&	opaqueParsedObject,
					TypedValue&	result);

				/* NOTE: typedef'd functions cannot throw... */
	static void		(*CompileAndEvaluate)(
					const Cstring&	exp_text,
					behaving_element& MA,
					TypedValue&	result);
				// no local context provided
	static void		(*CompileAndEvaluateSelf)(
					const Cstring&	exp_text,
					TypedValue&	result);
	static stringslist&	CompilationWarnings();
};

#endif /* _COMPILER_INTFC_H_ */

