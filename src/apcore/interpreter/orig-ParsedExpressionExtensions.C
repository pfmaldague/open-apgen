#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <RES_eval.H>
#include <ParsedExpressionExtensions.H>

using namespace pEsys;

void ParsedProgram::initExpression(
		expNodeType nt,
		const Cstring& nodeData) {
	nodeType = nt;
	theData = nodeData;
	switch(nt) {
		case ENT_PROGRAM:
			break;
		default:
			assert(false);
	}
}

void ParsedProgram::addExp(
		const parsedExp& pe) {
	switch(pe->getNodeType()) {
		case ENT_PROGRAM:
			for(int i = 0; i < pe->expressions.size(); i++) {
				expressions.push_back(pe->expressions[i]);
			}
			break;
		default:
			expressions.push_back(pe);
			// program_start++;
			break;
	}
}

void ParsedProgram::to_program(program& p) {
	for(int i = 0; i < expressions.size(); i++) {
		ParsedInstruction* pi = dynamic_cast<ParsedInstruction*>(expressions[i].object());
		assert(pi);
		assert(pi->my_instruction);
		p << pi->my_instruction;
		pi->my_instruction = NULL;
	}
}

void ParsedInstruction::initExpression(
		expNodeType nt,
		const Cstring& nodeData) {
	// func = NULL;
	nodeType = nt;
	theData = nodeData;
	// theResourceAccelerator = NULL;
	switch(nt) {
		case ENT_INSTRUCTION:
			break;
		default:
			assert(false);
	}
}

ParsedInstruction::~ParsedInstruction() {
	if(my_instruction) {
		delete my_instruction;
	}
	my_instruction = NULL;
}

void ParsedInstruction::to_stream(
		aoString*	outStream,
		int		indentation) const {
	assert(my_instruction);
	my_instruction->Print(*outStream, "");
}
