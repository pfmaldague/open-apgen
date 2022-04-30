#ifndef _S_C_READER_H_
#define _S_C_READER_H_

#include "SC_expressions.H"

// #define yylineno sclineno

extern int SClineno;

namespace SC {
class temporal_spec;
}

namespace SC_reader {

string make_space(int n);

			// These vectors are where all parsed objects go.
vector<SCExp>&	input_files();
string&		current_file();
stringtlist&	local_functions();

} // end namespace SC_reader

#endif /* _S_C_READER_H_ */
