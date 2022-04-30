#ifndef _SC__LEX_INTFC_H_
#define _SC__LEX_INTFC_H_

#include <sstream>
#include <RES_exceptions.H>

using namespace std;

		// in grammar.y:
extern int	SC_special_expression_token();
extern int	SC_special_statement_token();
extern int	SC_special_expression_list_token();
extern int	SC_error( const char * );
extern int	SCparse();
extern void	SC_add_to_error_string(const char *s);

		// in tokens.l:
extern int	SClex();
extern int	SClinenumber();
extern void	SC_lex_buffers_initialize_to(const char *iString);
extern void	SC_cleanup_tokens_buffer();
extern char	*SCtext;

		// in lexer_support.C:
void		CCOUT(char * S);
void		Ccout(char * S);
void		SC_initialize_tokens(bool debug_grammar);

namespace SC_reader {
	void		readAdaptationFile(const char*);
	stringstream&	theErrorStream();
}

namespace SC {
	int	process_alphanumeric_token(const char*);
}

#endif /* _SC__LEX_INTFC_H_ */
