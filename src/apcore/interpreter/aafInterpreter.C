#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <ParsedExpressionSystem.H>
#include <aafReader.H>

using namespace pEsys;

namespace aafInterpreter {

void interpret_aaf(
		parsedExp&	input_file) {
	pEsys::InputFile*	iF
		= dynamic_cast<pEsys::InputFile*>(input_file.object());
	pEsys::File*		F
		= dynamic_cast<pEsys::File*>(iF->file_body.object());
	parsedExp	item;
	if(F->adaptation_item) {
		item = F->adaptation_item;
	} else if(F->plan_item) {
		item = F->plan_item;
	}

	// debug
	cerr << "interpreting item " << item->getData() << "\n";

	// list<pE*>	theExpressionStack;
	// item->resolve_symbols(theExpressionStack);

	// for(int i = 0; i < F->expressions.size(); i++) {
	// 	item = F->expressions[i];

		// debug
	// 	cerr << "interpreting item " << item->getData() << "\n";

	// 	item->resolve_symbols(theExpressionStack);
	// }
}

} // namespace aafInterpreter
