#ifndef _TOL__LEX_INTFC_H_
#define _TOL__LEX_INTFC_H_

namespace tol {
	int	process_alphanumeric_token(const char*);
}

void		tol_initialize_tokens();

#ifdef OBSOLETE

#include <sstream>
#include <RES_exceptions.H>

using namespace std;

		// in grammar.y:
extern int	tol_special_expression_token();
extern int	tol_special_statement_token();
extern int	tol_special_expression_list_token();
extern int	tol_error( const char * );
extern int	tolparse();
extern void	tol_add_to_error_string(const char *s);

		// in tokens.l:
extern int	tollex();
extern int	tollinenumber();

extern char	*toltext;

		// in lexer_support.C:
void		CCOUT(char * S);
void		Ccout(char * S);

#endif /* OBSOLETE */

#endif /* _TOL__LEX_INTFC_H_ */
