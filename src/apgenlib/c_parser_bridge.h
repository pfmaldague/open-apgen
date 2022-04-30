#ifndef _C_PARSER_BRIDGE_H
#define _C_PARSER_BRIDGE_H

		// in grammar.y:
extern int	ab_special_expression_token();
extern int	ab_special_statement_token();
extern int	ab_special_expression_list_token();
extern int	aberror( const char * );
extern int	abparse();
extern void	add_to_error_string(const char *s);
		// NOTE: grammar.y needs ablex()

		// in p1_grammar.y:
extern int	y1parse();
		// NOTE: p1_grammar.y needs y1lex()

		// in tokens.l:
extern int	yylex();
extern int	y1linenumber();
extern void	yy_lex_buffers_initialize_to(const char *iString);
extern char	*yytext;

		// in lexer_support.C:
void		CCOUT(char * S);
void		Ccout(char * S);
void		ab_initialize_tokens();

		// in cmd_grammar.y:
extern int	zzparse();
extern int	CMD_special_expression_token();
		// NOTE: cmd_grammar.y needs zzlex()

		// in cmd_tokens.l:
extern void	zz_lex_buffers_initialize_to(const char *s);
extern int	zzlinenumber();
extern void	cmd_initialize_tokens();

		// in grammar_intfc.H:
extern int	FIRST_call;

		// in grammar_intfc.C:
extern void	initialize_parser_globals();
extern void	reset_exp_stack();

		// in CMD_intfc.C:
extern void	reset_error_flag();
// extern void	turn_on_keyword_detection();

// extern int	read_MADL(
// 			char *theInputFileName,	// method 1
// 			char *theInputString,	// method 2
// 			const char *theAuxiliaryOutputFileName,
// 			char **theConvertedAaf,
// 			char **theErrors,
// 			int verbosity);
// extern int	read_APXML(
// 			char *theInputFileName,	// method 1
// 			char *theInputString,	// method 2
// 			char **theConvertedAaf,
// 			char **theErrors,
// 			int verbosity);


#endif /* _C_PARSER_BRIDGE_H */
