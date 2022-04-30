#ifndef _LEX_INTFC_H_
#define _LEX_INTFC_H_

#include <sstream>
#include <RES_exceptions.H>

using namespace std;

		// in grammar.y:
extern int	ab_special_expression_token();
extern int	ab_special_statement_token();
extern int	ab_special_expression_list_token();
extern int	aberror( const char * );
extern int	abparse();
extern void	add_to_error_string(const char *s);
		// NOTE: grammar.y needs ablex()

		// in tokens.l:
extern int	yylex();
extern int	y1linenumber();
extern void	yy_lex_buffers_initialize_to(const char *iString);
extern void	yy_cleanup_tokens_buffer();
extern char	*yytext;

		// in lexer_support.C:
void		CCOUT(char * S);
void		Ccout(char * S);
void		ab_initialize_tokens();

		// in grammar_intfc.H:
extern int	FIRST_call;

namespace aafReader {
	void		readAdaptationFile(const char*);
}

#endif /* _LEX_INTFC_H_ */
