ActInstance::ActInstance(
	int		main_index,
	parsedExp&	P_act_instance_header,
	parsedExp&	P_tok_begin,
	parsedExp&	P_decomposition_info,
	parsedExp&	P_attributes_section_inst,
	parsedExp&	P_param_section_inst,
	parsedExp&	P_tok_end,
	parsedExp&	P_tok_activity,
	parsedExp&	P_tok_instance,
	parsedExp&	P_activity_id,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		act_instance_header(P_act_instance_header),
		tok_begin(P_tok_begin),
		decomposition_info(P_decomposition_info),
		attributes_section_inst(P_attributes_section_inst),
		param_section_inst(P_param_section_inst),
		tok_end(P_tok_end),
		tok_activity(P_tok_activity),
		tok_instance(P_tok_instance),
		activity_id(P_activity_id)
	{
	mainIndex = main_index;
	if(act_instance_header) {
		initExpression(act_instance_header->getData());
	} else {
		initExpression("(null)");
	}
}
ActInstance::ActInstance(const ActInstance& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.act_instance_header) {
		act_instance_header.reference(p.act_instance_header->copy());
	}
	if(p.activity_id) {
		activity_id.reference(p.activity_id->copy());
	}
	if(p.attributes_section_inst) {
		attributes_section_inst.reference(p.attributes_section_inst->copy());
	}
	if(p.decomposition_info) {
		decomposition_info.reference(p.decomposition_info->copy());
	}
	if(p.param_section_inst) {
		param_section_inst.reference(p.param_section_inst->copy());
	}
	if(p.tok_activity) {
		tok_activity.reference(p.tok_activity->copy());
	}
	if(p.tok_begin) {
		tok_begin.reference(p.tok_begin->copy());
	}
	if(p.tok_end) {
		tok_end.reference(p.tok_end->copy());
	}
	if(p.tok_instance) {
		tok_instance.reference(p.tok_instance->copy());
	}
	initExpression(p.getData());
}
ActInstance::ActInstance(bool, const ActInstance& p)
	: pE(origin_info(p.line, p.file)),
		act_instance_header(p.act_instance_header),
		activity_id(p.activity_id),
		attributes_section_inst(p.attributes_section_inst),
		decomposition_info(p.decomposition_info),
		param_section_inst(p.param_section_inst),
		tok_activity(p.tok_activity),
		tok_begin(p.tok_begin),
		tok_end(p.tok_end),
		tok_instance(p.tok_instance) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ActInstance::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(act_instance_header) {
		act_instance_header->recursively_apply(EA);
	}
	if(activity_id) {
		activity_id->recursively_apply(EA);
	}
	if(attributes_section_inst) {
		attributes_section_inst->recursively_apply(EA);
	}
	if(decomposition_info) {
		decomposition_info->recursively_apply(EA);
	}
	if(param_section_inst) {
		param_section_inst->recursively_apply(EA);
	}
	if(tok_activity) {
		tok_activity->recursively_apply(EA);
	}
	if(tok_begin) {
		tok_begin->recursively_apply(EA);
	}
	if(tok_end) {
		tok_end->recursively_apply(EA);
	}
	if(tok_instance) {
		tok_instance->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ActInstance::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ActInstance::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ActType::ActType(
	int		main_index,
	parsedExp&	P_initial_act_type_section,
	parsedExp&	P_body_of_activity_type_def,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		initial_act_type_section(P_initial_act_type_section),
		body_of_activity_type_def(P_body_of_activity_type_def)
	{
	mainIndex = main_index;
	if(initial_act_type_section) {
		initExpression(initial_act_type_section->getData());
	} else {
		initExpression("(null)");
	}
}
 ActType::ActType(const ActType& p)
	: pE(origin_info(p.line, p.file)),
		defined_attributes(p.defined_attributes) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.body_of_activity_type_def) {
		body_of_activity_type_def.reference(p.body_of_activity_type_def->copy());
	}
	if(p.initial_act_type_section) {
		initial_act_type_section.reference(p.initial_act_type_section->copy());
	}
	initExpression(p.getData());
 }
ActType::ActType(bool, const ActType& p)
	: pE(origin_info(p.line, p.file)),
		body_of_activity_type_def(p.body_of_activity_type_def),
		initial_act_type_section(p.initial_act_type_section) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ActType::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(body_of_activity_type_def) {
		body_of_activity_type_def->recursively_apply(EA);
	}
	if(initial_act_type_section) {
		initial_act_type_section->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	ActType::initExpression(
		const Cstring& nodeData) {
	theData = "ActType";
	if(initial_act_type_section) {
		ActTypeInitial* ati = dynamic_cast<ActTypeInitial*>(initial_act_type_section.object());
		if(ati) {
			if(ati->opt_type_class_variables) {
				ActTypeClassVariables* atc = dynamic_cast<ActTypeClassVariables*>(ati->opt_type_class_variables.object());
				if(atc) {
					Program* pg = dynamic_cast<Program*>(atc->type_class_variables.object());
					assert(pg);
					original_class_vars.reference(pg->copy());
				}
			}
			if(ati->opt_act_attributes_section) {
				ActTypeAttributes* ata = dynamic_cast<ActTypeAttributes*>(ati->opt_act_attributes_section.object());
				if(ata) {
					Program* pg = dynamic_cast<Program*>(ata->declarative_program.object());
					assert(pg);
					original_attributes.reference(pg->copy());
				}
			}
			if(ati->opt_act_type_param_section) {
				Parameters* P = dynamic_cast<Parameters*>(ati->opt_act_type_param_section.object());
				if(P) {
					original_parameters.reference(P->copy());
				}
			}
		}
	}
	if(body_of_activity_type_def) {
		ActTypeBody* atb = dynamic_cast<ActTypeBody*>(body_of_activity_type_def.object());
		if(atb) {
			if(atb->opt_creation_section) {
				TypeCreationSection* tc = dynamic_cast<TypeCreationSection*>(atb->opt_creation_section.object());
				if(tc) {
					Program* P = dynamic_cast<Program*>(tc->program.object());
					assert(P);
					original_creation.reference(P->copy());
				}
			}
			if(atb->opt_res_usage_section) {
				ModelingSection* mc = dynamic_cast<ModelingSection*>(atb->opt_res_usage_section.object());
				if(mc) {
					Program* P = dynamic_cast<Program*>(mc->program.object());
					assert(P);
					original_modeling.reference(P->copy());
				}
			}
			if(atb->opt_decomposition_section) {
				ActTypeDecomp* atd = dynamic_cast<ActTypeDecomp*>(atb->opt_decomposition_section.object());
				if(atd) {
					Program* P = dynamic_cast<Program*>(atd->program.object());
					assert(P);
					original_decomposition.reference(P->copy());
					if(atd->opt_res_usage_section) {
						ModelingSection* mc = dynamic_cast<ModelingSection*>(atd->opt_res_usage_section.object());
						if(mc) {
							Program* P = dynamic_cast<Program*>(mc->program.object());
							assert(P);
							original_modeling.reference(P->copy());
						}
					}
				}
			}
			if(atb->opt_destruction_section) {
				TypeCreationSection* dc = dynamic_cast<TypeCreationSection*>(atb->opt_destruction_section.object());
				if(dc) {
					Program* P = dynamic_cast<Program*>(dc->program.object());
					assert(P);
					original_destruction.reference(P->copy());
				}
			}
		}
	}
 }
void	ActType::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ActTypeAttributes::ActTypeAttributes(
	int		main_index,
	parsedExp&	P_declarative_program,
	parsedExp&	P_tok_attributes,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		declarative_program(P_declarative_program),
		tok_attributes(P_tok_attributes)
	{
	mainIndex = main_index;
	if(declarative_program) {
		initExpression(declarative_program->getData());
	} else {
		initExpression("(null)");
	}
}
ActTypeAttributes::ActTypeAttributes(const ActTypeAttributes& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.declarative_program) {
		declarative_program.reference(p.declarative_program->copy());
	}
	if(p.tok_attributes) {
		tok_attributes.reference(p.tok_attributes->copy());
	}
	initExpression(p.getData());
}
ActTypeAttributes::ActTypeAttributes(bool, const ActTypeAttributes& p)
	: pE(origin_info(p.line, p.file)),
		declarative_program(p.declarative_program),
		tok_attributes(p.tok_attributes) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ActTypeAttributes::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(declarative_program) {
		declarative_program->recursively_apply(EA);
	}
	if(tok_attributes) {
		tok_attributes->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	ActTypeAttributes::initExpression(
		const Cstring& nodeData) {
	// nodeType = nt;
	theData = "ActTypeAttributes";
 }
void	ActTypeAttributes::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ActTypeBody::ActTypeBody(
	int		main_index,
	parsedExp&	P_opt_creation_section,
	parsedExp&	P_opt_res_usage_section,
	parsedExp&	P_opt_decomposition_section,
	parsedExp&	P_opt_destruction_section,
	parsedExp&	P_end_of_activity_type_def,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		opt_creation_section(P_opt_creation_section),
		opt_res_usage_section(P_opt_res_usage_section),
		opt_decomposition_section(P_opt_decomposition_section),
		opt_destruction_section(P_opt_destruction_section),
		end_of_activity_type_def(P_end_of_activity_type_def)
	{
	mainIndex = main_index;
	if(opt_creation_section) {
		initExpression(opt_creation_section->getData());
	} else {
		initExpression("(null)");
	}
}
ActTypeBody::ActTypeBody(const ActTypeBody& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.end_of_activity_type_def) {
		end_of_activity_type_def.reference(p.end_of_activity_type_def->copy());
	}
	if(p.opt_creation_section) {
		opt_creation_section.reference(p.opt_creation_section->copy());
	}
	if(p.opt_decomposition_section) {
		opt_decomposition_section.reference(p.opt_decomposition_section->copy());
	}
	if(p.opt_destruction_section) {
		opt_destruction_section.reference(p.opt_destruction_section->copy());
	}
	if(p.opt_res_usage_section) {
		opt_res_usage_section.reference(p.opt_res_usage_section->copy());
	}
	initExpression(p.getData());
}
ActTypeBody::ActTypeBody(bool, const ActTypeBody& p)
	: pE(origin_info(p.line, p.file)),
		end_of_activity_type_def(p.end_of_activity_type_def),
		opt_creation_section(p.opt_creation_section),
		opt_decomposition_section(p.opt_decomposition_section),
		opt_destruction_section(p.opt_destruction_section),
		opt_res_usage_section(p.opt_res_usage_section) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ActTypeBody::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(end_of_activity_type_def) {
		end_of_activity_type_def->recursively_apply(EA);
	}
	if(opt_creation_section) {
		opt_creation_section->recursively_apply(EA);
	}
	if(opt_decomposition_section) {
		opt_decomposition_section->recursively_apply(EA);
	}
	if(opt_destruction_section) {
		opt_destruction_section->recursively_apply(EA);
	}
	if(opt_res_usage_section) {
		opt_res_usage_section->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ActTypeBody::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ActTypeBody::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ActTypeClassVariables::ActTypeClassVariables(
	int		main_index,
	parsedExp&	P_type_class_variables,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		type_class_variables(P_type_class_variables)

	{
	mainIndex = main_index;
	if(type_class_variables) {
		initExpression(type_class_variables->getData());
	} else {
		initExpression("(null)");
	}
}
ActTypeClassVariables::ActTypeClassVariables(const ActTypeClassVariables& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.type_class_variables) {
		type_class_variables.reference(p.type_class_variables->copy());
	}
	initExpression(p.getData());
}
ActTypeClassVariables::ActTypeClassVariables(bool, const ActTypeClassVariables& p)
	: pE(origin_info(p.line, p.file)),
		type_class_variables(p.type_class_variables) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ActTypeClassVariables::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(type_class_variables) {
		type_class_variables->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ActTypeClassVariables::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ActTypeClassVariables::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ActTypeDecl::ActTypeDecl(
	int		main_index,
	parsedExp&	P_activity_name,
	parsedExp&	P_activity_type_preface,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		activity_name(P_activity_name),
		activity_type_preface(P_activity_type_preface),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(activity_name) {
		initExpression(activity_name->getData());
	} else {
		initExpression("(null)");
	}
}
ActTypeDecl::ActTypeDecl(const ActTypeDecl& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.activity_name) {
		activity_name.reference(p.activity_name->copy());
	}
	if(p.activity_type_preface) {
		activity_type_preface.reference(p.activity_type_preface->copy());
	}
	initExpression(p.getData());
}
ActTypeDecl::ActTypeDecl(bool, const ActTypeDecl& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		activity_name(p.activity_name),
		activity_type_preface(p.activity_type_preface) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ActTypeDecl::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(activity_name) {
		activity_name->recursively_apply(EA);
	}
	if(activity_type_preface) {
		activity_type_preface->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	ActTypeDecl::initExpression(
		const Cstring& nodeData) {
	// nodeType = nt;
	theData = nodeData;
	// make sure we grab the function name
	if(!aafReader::activity_types().find(nodeData)) {
		aafReader::activity_types() << new emptySymbol(nodeData); 
	}
 }
void	ActTypeDecl::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ActTypeDecomp::ActTypeDecomp(
	int		main_index,
	parsedExp&	P_decomposition_header,
	parsedExp&	P_program,
	parsedExp&	P_opt_res_usage_section,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		decomposition_header(P_decomposition_header),
		program(P_program),
		opt_res_usage_section(P_opt_res_usage_section)
	{
	mainIndex = main_index;
	if(decomposition_header) {
		initExpression(decomposition_header->getData());
	} else {
		initExpression("(null)");
	}
}
ActTypeDecomp::ActTypeDecomp(const ActTypeDecomp& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.decomposition_header) {
		decomposition_header.reference(p.decomposition_header->copy());
	}
	if(p.opt_res_usage_section) {
		opt_res_usage_section.reference(p.opt_res_usage_section->copy());
	}
	if(p.program) {
		program.reference(p.program->copy());
	}
	initExpression(p.getData());
}
ActTypeDecomp::ActTypeDecomp(bool, const ActTypeDecomp& p)
	: pE(origin_info(p.line, p.file)),
		decomposition_header(p.decomposition_header),
		opt_res_usage_section(p.opt_res_usage_section),
		program(p.program) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ActTypeDecomp::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(decomposition_header) {
		decomposition_header->recursively_apply(EA);
	}
	if(opt_res_usage_section) {
		opt_res_usage_section->recursively_apply(EA);
	}
	if(program) {
		program->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ActTypeDecomp::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ActTypeDecomp::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ActTypeInitial::ActTypeInitial(
	int		main_index,
	parsedExp&	P_activity_type_header,
	parsedExp&	P_opt_type_class_variables,
	parsedExp&	P_opt_act_attributes_section,
	parsedExp&	P_opt_act_type_param_section,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		activity_type_header(P_activity_type_header),
		opt_type_class_variables(P_opt_type_class_variables),
		opt_act_attributes_section(P_opt_act_attributes_section),
		opt_act_type_param_section(P_opt_act_type_param_section)
	{
	mainIndex = main_index;
	if(activity_type_header) {
		initExpression(activity_type_header->getData());
	} else {
		initExpression("(null)");
	}
}
ActTypeInitial::ActTypeInitial(const ActTypeInitial& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.activity_type_header) {
		activity_type_header.reference(p.activity_type_header->copy());
	}
	if(p.opt_act_attributes_section) {
		opt_act_attributes_section.reference(p.opt_act_attributes_section->copy());
	}
	if(p.opt_act_type_param_section) {
		opt_act_type_param_section.reference(p.opt_act_type_param_section->copy());
	}
	if(p.opt_type_class_variables) {
		opt_type_class_variables.reference(p.opt_type_class_variables->copy());
	}
	initExpression(p.getData());
}
ActTypeInitial::ActTypeInitial(bool, const ActTypeInitial& p)
	: pE(origin_info(p.line, p.file)),
		activity_type_header(p.activity_type_header),
		opt_act_attributes_section(p.opt_act_attributes_section),
		opt_act_type_param_section(p.opt_act_type_param_section),
		opt_type_class_variables(p.opt_type_class_variables) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ActTypeInitial::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(activity_type_header) {
		activity_type_header->recursively_apply(EA);
	}
	if(opt_act_attributes_section) {
		opt_act_attributes_section->recursively_apply(EA);
	}
	if(opt_act_type_param_section) {
		opt_act_type_param_section->recursively_apply(EA);
	}
	if(opt_type_class_variables) {
		opt_type_class_variables->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
  void	ActTypeInitial::initExpression(
		const Cstring& nodeData) {
	// nodeType = nt;
	theData = "ActTypeInitial";
	// need to stick the act type string into aafReader::activity_types()
	aafReader::activity_types() << new emptySymbol(activity_type_header->getData()); 
 }
void	ActTypeInitial::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
AdditiveExp::AdditiveExp(
	int		main_index,
	parsedExp&	P_tok_plus,
	parsedExp&	P_maybe_a_sum,
	parsedExp&	P_maybe_added_or_subtracted,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_plus(P_tok_plus),
		maybe_a_sum(P_maybe_a_sum),
		maybe_added_or_subtracted(P_maybe_added_or_subtracted)
	{
	mainIndex = main_index;
	if(tok_plus) {
		initExpression(tok_plus->getData());
	} else {
		initExpression("(null)");
	}
}
AdditiveExp::AdditiveExp(
	int		main_index,
	parsedExp&	P_tok_minus,
	parsedExp&	P_maybe_a_sum,
	parsedExp&	P_maybe_added_or_subtracted,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_minus(P_tok_minus),
		maybe_a_sum(P_maybe_a_sum),
		maybe_added_or_subtracted(P_maybe_added_or_subtracted)
	{
	mainIndex = main_index;
	if(tok_minus) {
		initExpression(tok_minus->getData());
	} else {
		initExpression("(null)");
	}
}
 AdditiveExp::AdditiveExp(const AdditiveExp& p)
	: pE(origin_info(p.line, p.file)) {
	binaryFunc = p.binaryFunc;
	tlist<alpha_void, Cnode0<alpha_void, pE*> >	newobjs(true);
	pE*						obj;

	for(int i = 0; i < p.expressions.size(); i++) {
		if((obj = p.expressions[i].object())) {
			expressions.push_back(parsedExp(
				obj->smart_copy(newobjs)));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.maybe_a_sum) {
		maybe_a_sum.reference(p.maybe_a_sum->smart_copy(newobjs));
	}
	if(p.maybe_added_or_subtracted) {
		maybe_added_or_subtracted.reference(p.maybe_added_or_subtracted->smart_copy(newobjs));
	}
	if(p.tok_minus) {
		tok_minus.reference(p.tok_minus->smart_copy(newobjs));
	}
	if(p.tok_plus) {
		tok_plus.reference(p.tok_plus->smart_copy(newobjs));
	}
	if(p.Lhs) {
		Lhs.reference(p.Lhs->smart_copy(newobjs));
	}
	if(p.Rhs) {
		Rhs.reference(p.Rhs->smart_copy(newobjs));
	}
	if(p.Operator) {
		Operator.reference(p.Operator->smart_copy(newobjs));
	}
 }
 AdditiveExp::AdditiveExp(bool, const AdditiveExp& p)
	: pE(origin_info(p.line, p.file)),
		maybe_a_sum(p.maybe_a_sum),
		maybe_added_or_subtracted(p.maybe_added_or_subtracted),
		tok_minus(p.tok_minus),
		tok_plus(p.tok_plus),
		Lhs(p.Lhs),
		Rhs(p.Rhs),
		Operator(p.Operator) {
	binaryFunc = p.binaryFunc;
	expressions = p.expressions;
 }
 void	AdditiveExp::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	if(Lhs) {
		Lhs->recursively_apply(EA);
	}
	if(Rhs) {
		Rhs->recursively_apply(EA);
	}
	EA.post_analyze(this);
 }
 void	AdditiveExp::initExpression(
		const Cstring& nodeData) {
	// nodeType = nt;
	theData = nodeData;
	if(tok_minus) {
		Operator = tok_minus;
		Lhs = maybe_a_sum;
		Rhs = maybe_added_or_subtracted;
		binaryFunc = func_MINUS;
	} else if(tok_plus) {
		Operator = tok_plus;
		Lhs = maybe_a_sum;
		Rhs = maybe_added_or_subtracted;
		binaryFunc = func_PLUS;
	}
 }
 void	AdditiveExp::eval_expression(
		behaving_base*	local_object,
		TypedValue&	result
		)  {
	try {
	    TypedValue val1, val2;
	    Lhs->eval_expression(local_object, val1),
	    Rhs->eval_expression(local_object, val2),
	    binaryFunc(
		val1,
		val2,
		result);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ":\n" << Err.msg;
		throw(eval_error(err));
	}

	if(APcloptions::theCmdLineOptions().debug_execute) {
		TypedValue	val, val2;
		Lhs->eval_expression(local_object, val);
		Rhs->eval_expression(local_object, val2);
		Cstring tmp = val.to_string();
		tmp << " " << theData << " "
			<< val2.to_string()
			<< " = " << result.to_string();
		cerr << "AdditiveExp::eval - " << tmp << "\n";
	}
 }
Array::Array(
	int		main_index,
	parsedExp&	P_rest_of_list,
	parsedExp&	P_ASCII_91,	// [
	parsedExp&	P_ASCII_93,	// ]
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		rest_of_list(P_rest_of_list),
		ASCII_91(P_ASCII_91),
		ASCII_93(P_ASCII_93)
	{
	mainIndex = main_index;
	if(rest_of_list) {
		initExpression(rest_of_list->getData());
	} else {
		initExpression("(null)");
	}
}
 Array::Array(const Array& A)
		: pE(origin_info(A.line, A.file)),
		is_list(A.is_list),
		is_struct(A.is_struct) {
	theData = A.getData();
	if(A.rest_of_list) {
		rest_of_list.reference(A.rest_of_list.object());
	}
	tlist<alpha_void, Cnode0<alpha_void, pE*> >	newobjs(true);
	for(int i = 0; i < A.actual_values.size(); i++) {
		actual_values.push_back(parsedExp(A.actual_values[i]->smart_copy(newobjs)));
	}
	for(int i = 0; i < A.actual_keys.size(); i++) {
		actual_keys.push_back(parsedExp(A.actual_keys[i]->smart_copy(newobjs)));
	}
 }
 Array::Array(bool, const Array& A)
		: pE(origin_info(A.line, A.file)),
		is_list(A.is_list),
		is_struct(A.is_struct),
		rest_of_list(A.rest_of_list),
		actual_values(A.actual_values),
		actual_keys(A.actual_keys) {
	theData = A.getData();
 }
void Array::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_91) {
		ASCII_91->recursively_apply(EA);
	}
	if(ASCII_93) {
		ASCII_93->recursively_apply(EA);
	}
	if(rest_of_list) {
		rest_of_list->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	Array::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
 void	Array::eval_expression(
		behaving_base*	loc,
		TypedValue&	res)
			 {
	ListOVal* lov = new ListOVal;
	ArrayElement* ae;
	if(is_list) {
		for(int i = 0; i < actual_values.size(); i++) {
			pEsys::pE* pe = actual_values[i].object();
			ae = new ArrayElement(i);
			try {
				TypedValue val;
				pe->eval_expression(loc, val);
				ae->SetVal(val);
			} catch(eval_error Err) {
				Cstring err;
				err << "File " << file << ", line " << line << ": "
					<< "error in evaluating "
					<< i << "-th item in list:\n"
					<< Err.msg;
				throw(eval_error(err));
			}
			lov->add(ae);
		}
	} else if(is_struct) {
		// cerr << "Array::eval_expression: is_list = false; " << actual_values.size()
		// 	<< " value(s) found\n";
		for(int i = 0; i < actual_values.size(); i++) {
			pEsys::pE* the_item = actual_values[i].object();
			pEsys::pE* the_key = actual_keys[i].object();
			try {
				TypedValue key_value;
				the_key->eval_expression(loc, key_value);
				ae = new ArrayElement(key_value.get_string());
				TypedValue item_value;
				the_item->eval_expression(loc, item_value);
				ae->SetVal(item_value);
				lov->add(ae);
			} catch(eval_error Err) {
				Cstring err;
				err << "File " << file << ", line " << line << ": "
					<< "error in evaluating "
					<< i << "-th item in array:\n"
					<< Err.msg;
				throw(eval_error(err));
			}
		}
	}
	res = *lov;
 }
ArrayList::ArrayList(
	int		main_index,
	parsedExp&	P_one_array,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		one_array(P_one_array)

	{
	mainIndex = main_index;
	if(one_array) {
		initExpression(one_array->getData());
	} else {
		initExpression("(null)");
	}
}
 ArrayList::ArrayList(const ArrayList& A)
		: pE(origin_info(A.line, A.file)),
		evaluated_elements(A.evaluated_elements) {
	theData = A.getData();
	if(A.one_array) {
		one_array.reference(A.one_array.object());
	}
 }
ArrayList::ArrayList(bool, const ArrayList& p)
	: pE(origin_info(p.line, p.file)),
		one_array(p.one_array) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ArrayList::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(one_array) {
		one_array->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	ArrayList::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
 }
void	ArrayList::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ArrayPrecomp::ArrayPrecomp(
	int		main_index,
	parsedExp&	P_keyword_value_pairs_precomp,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		keyword_value_pairs_precomp(P_keyword_value_pairs_precomp)

	{
	mainIndex = main_index;
	if(keyword_value_pairs_precomp) {
		initExpression(keyword_value_pairs_precomp->getData());
	} else {
		initExpression("(null)");
	}
}
ArrayPrecomp::ArrayPrecomp(const ArrayPrecomp& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.keyword_value_pairs_precomp) {
		keyword_value_pairs_precomp.reference(p.keyword_value_pairs_precomp->copy());
	}
	initExpression(p.getData());
}
ArrayPrecomp::ArrayPrecomp(bool, const ArrayPrecomp& p)
	: pE(origin_info(p.line, p.file)),
		keyword_value_pairs_precomp(p.keyword_value_pairs_precomp) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ArrayPrecomp::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(keyword_value_pairs_precomp) {
		keyword_value_pairs_precomp->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ArrayPrecomp::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ArrayPrecomp::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Assignment::Assignment(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_equal_sign,
	parsedExp&	P_Expression_2,
	parsedExp&	P_opt_range,
	parsedExp&	P_opt_descr,
	parsedExp&	P_opt_units,
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_equal_sign(P_tok_equal_sign),
		Expression_2(P_Expression_2),
		opt_range(P_opt_range),
		opt_descr(P_opt_descr),
		opt_units(P_opt_units)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
Assignment::Assignment(
	int		main_index,
	parsedExp&	P_lhs_of_assign,
	parsedExp&	P_assignment_op,
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_1)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		lhs_of_assign(P_lhs_of_assign),
		assignment_op(P_assignment_op),
		Expression(P_Expression),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(lhs_of_assign) {
		initExpression(lhs_of_assign->getData());
	} else {
		initExpression("(null)");
	}
}
Assignment::Assignment(const Assignment& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.Expression_2) {
		Expression_2.reference(p.Expression_2->copy());
	}
	if(p.assignment_op) {
		assignment_op.reference(p.assignment_op->copy());
	}
	if(p.lhs_of_assign) {
		lhs_of_assign.reference(p.lhs_of_assign->copy());
	}
	if(p.opt_descr) {
		opt_descr.reference(p.opt_descr->copy());
	}
	if(p.opt_range) {
		opt_range.reference(p.opt_range->copy());
	}
	if(p.opt_units) {
		opt_units.reference(p.opt_units->copy());
	}
	if(p.tok_equal_sign) {
		tok_equal_sign.reference(p.tok_equal_sign->copy());
	}
	initExpression(p.getData());
}
Assignment::Assignment(bool, const Assignment& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		Expression_2(p.Expression_2),
		assignment_op(p.assignment_op),
		lhs_of_assign(p.lhs_of_assign),
		opt_descr(p.opt_descr),
		opt_range(p.opt_range),
		opt_units(p.opt_units),
		tok_equal_sign(p.tok_equal_sign) {
	expressions = p.expressions;
	initExpression(p.getData());
}
 void Assignment::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(Expression_2) {
		Expression_2->recursively_apply(EA);
	}
	if(assignment_op) {
		assignment_op->recursively_apply(EA);
	}

	decomp_finder* DF = dynamic_cast<decomp_finder*>(&EA);
	if(!DF) {

	    //
	    // We don't want DF to parse the symbol in the lhs
	    // as a 'read' item; it's already been flagged as
	    // a 'set' item.
	    //
	    if(lhs_of_assign) {
		lhs_of_assign->recursively_apply(EA);
	    }
	}
	if(opt_descr) {
		opt_descr->recursively_apply(EA);
	}
	if(opt_range) {
		opt_range->recursively_apply(EA);
	}
	if(opt_units) {
		opt_units->recursively_apply(EA);
	}
	if(tok_equal_sign) {
		tok_equal_sign->recursively_apply(EA);
	}
	EA.post_analyze(this);
 }
 void	Assignment::initExpression(
		const Cstring& nodeData) {
	theData = "Assignment";
 }
 void	Assignment::eval_expression(
		behaving_base*	loc,
		TypedValue&	result)
			 {
	assert(false);
 }
AssignmentPrecomp::AssignmentPrecomp(
	int		main_index,
	parsedExp&	P_tok_stringval,
	parsedExp&	P_tok_equal_sign,
	parsedExp&	P_ASCII_91,	// [
	parsedExp&	P_one_number,
	parsedExp&	P_ASCII_44,	// ,
	parsedExp&	P_one_number_5,
	parsedExp&	P_ASCII_93,	// ]
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_stringval(P_tok_stringval),
		tok_equal_sign(P_tok_equal_sign),
		ASCII_91(P_ASCII_91),
		one_number(P_one_number),
		ASCII_44(P_ASCII_44),
		one_number_5(P_one_number_5),
		ASCII_93(P_ASCII_93)
	{
	mainIndex = main_index;
	if(tok_stringval) {
		initExpression(tok_stringval->getData());
	} else {
		initExpression("(null)");
	}
}
AssignmentPrecomp::AssignmentPrecomp(const AssignmentPrecomp& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_44) {
		ASCII_44.reference(p.ASCII_44->copy());
	}
	if(p.ASCII_91) {
		ASCII_91.reference(p.ASCII_91->copy());
	}
	if(p.ASCII_93) {
		ASCII_93.reference(p.ASCII_93->copy());
	}
	if(p.one_number) {
		one_number.reference(p.one_number->copy());
	}
	if(p.one_number_5) {
		one_number_5.reference(p.one_number_5->copy());
	}
	if(p.tok_equal_sign) {
		tok_equal_sign.reference(p.tok_equal_sign->copy());
	}
	if(p.tok_stringval) {
		tok_stringval.reference(p.tok_stringval->copy());
	}
	initExpression(p.getData());
}
AssignmentPrecomp::AssignmentPrecomp(bool, const AssignmentPrecomp& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_44(p.ASCII_44),
		ASCII_91(p.ASCII_91),
		ASCII_93(p.ASCII_93),
		one_number(p.one_number),
		one_number_5(p.one_number_5),
		tok_equal_sign(p.tok_equal_sign),
		tok_stringval(p.tok_stringval) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void AssignmentPrecomp::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_44) {
		ASCII_44->recursively_apply(EA);
	}
	if(ASCII_91) {
		ASCII_91->recursively_apply(EA);
	}
	if(ASCII_93) {
		ASCII_93->recursively_apply(EA);
	}
	if(one_number) {
		one_number->recursively_apply(EA);
	}
	if(one_number_5) {
		one_number_5->recursively_apply(EA);
	}
	if(tok_equal_sign) {
		tok_equal_sign->recursively_apply(EA);
	}
	if(tok_stringval) {
		tok_stringval->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	AssignmentPrecomp::initExpression(
		const Cstring& nodeData) {
	theData = tok_stringval->getData();

	//
	// Get the latest temporary coefficient node
	//
	aafReader::state_series& coeff
	    = aafReader::single_precomp_res::UnderConsolidation->payload;

	Cstring theDataCopy(theData);
	removeQuotes(theDataCopy);
	CTime_base time_stamp(theDataCopy);
	Cnode0<alpha_time, aafReader::state2>* state_node
	    = new Cnode0<alpha_time, aafReader::state2>(time_stamp);

	//
	// Extract the position and velocity data
	// from the right-hand side
	//
	OneNumber* on = dynamic_cast<OneNumber*>(one_number.object());
	state_node->payload.s[0] = on->get_double();
	on = dynamic_cast<OneNumber*>(one_number_5.object());
	state_node->payload.s[1] = on->get_double();

	//
	// Add the state node to the series
	//
	coeff << state_node;

	//
	// We are done. This object may now be deleted.
	//
 }
void	AssignmentPrecomp::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ByValueSymbol::ByValueSymbol(
	int		main_index,
	parsedExp&	P_tok_mult,
	parsedExp&	P_tok_id,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_mult(P_tok_mult),
		tok_id(P_tok_id)
	{
	mainIndex = main_index;
	if(tok_mult) {
		initExpression(tok_mult->getData());
	} else {
		initExpression("(null)");
	}
}
 ByValueSymbol::ByValueSymbol(const ByValueSymbol& p)
			: pE(origin_info(p.line, p.file)) {
		initExpression(p.getData());
		my_task = p.my_task;
		my_level = p.my_level;
		my_index = p.my_index;
		my_use_task_index = p.my_use_task_index;
		my_index_in_use_task = p.my_index_in_use_task;
		if(p.tok_id) {
			tok_id.reference(p.tok_id->copy());
		}
		if(p.tok_mult) {
			tok_mult.reference(p.tok_mult->copy());
		}
		for(int i = 0; i < p.expressions.size(); i++) {
			if(p.expressions[i]) {
				expressions.push_back(parsedExp(p.expressions[i]->copy()));
			} else {
				expressions.push_back(parsedExp());
			}
		}

		//
		// Check, because the symbol may not have been consolidated:
		//

		if(my_task) {
			task* use_task = my_task->Type.tasks[my_use_task_index];
			assert(	my_index_in_use_task >= 0
				&& my_index_in_use_task < use_task->get_varinfo().size());
		}
 }
 ByValueSymbol::ByValueSymbol(bool, const ByValueSymbol& p)
			: pE(origin_info(p.line, p.file)) {
		initExpression(p.getData());
		my_task = p.my_task;
		my_level = p.my_level;
		my_index = p.my_index;
		my_use_task_index = p.my_use_task_index;
		my_index_in_use_task = p.my_index_in_use_task;
		tok_id = p.tok_id;
		tok_mult = p.tok_mult;
		expressions = p.expressions;

		//
		// Check, because the symbol may not have been consolidated:
		//

		if(my_task) {
			task* use_task = my_task->Type.tasks[my_use_task_index];
			assert(	my_index_in_use_task >= 0
				&& my_index_in_use_task < use_task->get_varinfo().size());
		}
 }
void ByValueSymbol::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	if(tok_mult) {
		tok_mult->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	ByValueSymbol::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	my_task = NULL;
	my_level = 0;
	my_index = 0;
	my_use_task_index = 0;
	my_index_in_use_task = 0;
 }
 void	ByValueSymbol::eval_expression(
		behaving_base*	loc,
		TypedValue&	result
		)  {
	result = get_val_ref(loc);
 }
ClassMember::ClassMember(
	int		main_index,
	parsedExp&	P_symbol,
	parsedExp&	P_ASCII_46,	// .
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		symbol(P_symbol),
		ASCII_46(P_ASCII_46)
	{
	mainIndex = main_index;
	if(symbol) {
		initExpression(symbol->getData());
	} else {
		initExpression("(null)");
	}
}
 ClassMember::ClassMember(const ClassMember& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_46) {
		ASCII_46.reference(p.ASCII_46->copy());
	}
	if(p.symbol) {
		symbol.reference(p.symbol->copy());
	}
	if(p.OptionalIndex) {
		OptionalIndex.reference(p.OptionalIndex->copy());
	}
	initExpression(p.getData());
 }
 ClassMember::ClassMember(bool, const ClassMember& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_46(p.ASCII_46),
		symbol(p.symbol) {
	expressions = p.expressions;
	OptionalIndex = p.OptionalIndex;
	initExpression(p.getData());
 }
void ClassMember::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_46) {
		ASCII_46->recursively_apply(EA);
	}
	if(symbol) {
		symbol->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ClassMember::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ClassMember::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Comparison::Comparison(
	int		main_index,
	parsedExp&	P_tok_lessthan,
	parsedExp&	P_maybe_compared,
	parsedExp&	P_maybe_compared_2,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_lessthan(P_tok_lessthan),
		maybe_compared(P_maybe_compared),
		maybe_compared_2(P_maybe_compared_2)
	{
	mainIndex = main_index;
	if(tok_lessthan) {
		initExpression(tok_lessthan->getData());
	} else {
		initExpression("(null)");
	}
}
Comparison::Comparison(
	int		main_index,
	parsedExp&	P_tok_lessthanoreq,
	parsedExp&	P_maybe_compared,
	parsedExp&	P_maybe_compared_2,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_lessthanoreq(P_tok_lessthanoreq),
		maybe_compared(P_maybe_compared),
		maybe_compared_2(P_maybe_compared_2)
	{
	mainIndex = main_index;
	if(tok_lessthanoreq) {
		initExpression(tok_lessthanoreq->getData());
	} else {
		initExpression("(null)");
	}
}
Comparison::Comparison(
	int		main_index,
	parsedExp&	P_tok_gtrthanoreq,
	parsedExp&	P_maybe_compared,
	parsedExp&	P_maybe_compared_2,
	Differentiator_2)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_gtrthanoreq(P_tok_gtrthanoreq),
		maybe_compared(P_maybe_compared),
		maybe_compared_2(P_maybe_compared_2)
	{
	mainIndex = main_index;
	if(tok_gtrthanoreq) {
		initExpression(tok_gtrthanoreq->getData());
	} else {
		initExpression("(null)");
	}
}
Comparison::Comparison(
	int		main_index,
	parsedExp&	P_tok_gtrthan,
	parsedExp&	P_maybe_compared,
	parsedExp&	P_maybe_compared_2,
	Differentiator_3)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_gtrthan(P_tok_gtrthan),
		maybe_compared(P_maybe_compared),
		maybe_compared_2(P_maybe_compared_2)
	{
	mainIndex = main_index;
	if(tok_gtrthan) {
		initExpression(tok_gtrthan->getData());
	} else {
		initExpression("(null)");
	}
}
Comparison::Comparison(const Comparison& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.maybe_compared) {
		maybe_compared.reference(p.maybe_compared->copy());
	}
	if(p.maybe_compared_0) {
		maybe_compared_0.reference(p.maybe_compared_0->copy());
	}
	if(p.maybe_compared_2) {
		maybe_compared_2.reference(p.maybe_compared_2->copy());
	}
	if(p.tok_gtrthan) {
		tok_gtrthan.reference(p.tok_gtrthan->copy());
	}
	if(p.tok_gtrthanoreq) {
		tok_gtrthanoreq.reference(p.tok_gtrthanoreq->copy());
	}
	if(p.tok_lessthan) {
		tok_lessthan.reference(p.tok_lessthan->copy());
	}
	if(p.tok_lessthanoreq) {
		tok_lessthanoreq.reference(p.tok_lessthanoreq->copy());
	}
	initExpression(p.getData());
}
Comparison::Comparison(bool, const Comparison& p)
	: pE(origin_info(p.line, p.file)),
		maybe_compared(p.maybe_compared),
		maybe_compared_0(p.maybe_compared_0),
		maybe_compared_2(p.maybe_compared_2),
		tok_gtrthan(p.tok_gtrthan),
		tok_gtrthanoreq(p.tok_gtrthanoreq),
		tok_lessthan(p.tok_lessthan),
		tok_lessthanoreq(p.tok_lessthanoreq) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Comparison::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(maybe_compared) {
		maybe_compared->recursively_apply(EA);
	}
	if(maybe_compared_0) {
		maybe_compared_0->recursively_apply(EA);
	}
	if(maybe_compared_2) {
		maybe_compared_2->recursively_apply(EA);
	}
	if(tok_gtrthan) {
		tok_gtrthan->recursively_apply(EA);
	}
	if(tok_gtrthanoreq) {
		tok_gtrthanoreq->recursively_apply(EA);
	}
	if(tok_lessthan) {
		tok_lessthan->recursively_apply(EA);
	}
	if(tok_lessthanoreq) {
		tok_lessthanoreq->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Comparison::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	if(tok_gtrthan) {
		Operator = tok_gtrthan;
		binaryFunc = func_GTRTHAN;
	} else if(tok_gtrthanoreq) {
		Operator = tok_gtrthanoreq;
		binaryFunc = func_GTRTHANOREQ;
	} else if(tok_lessthan) {
		Operator = tok_lessthan;
		binaryFunc = func_LESSTHAN;
	} else if(tok_lessthanoreq) {
		Operator = tok_lessthanoreq;
		binaryFunc = func_LESSTHANOREQ;
	}
 }
 void	Comparison::eval_expression(
		behaving_base*	local_object,
		TypedValue&	result
		) {
	TypedValue val1, val2;
	try {
	    maybe_compared->eval_expression(local_object, val1),
	    maybe_compared_2->eval_expression(local_object, val2),
	    binaryFunc(
		val1,
		val2,
		result);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ":\n" << Err.msg;
		throw(eval_error(err));
	}

	if(APcloptions::theCmdLineOptions().debug_execute) {
		Cstring tmp = val1.to_string();
		tmp << " " << theData << " " << val2.to_string()
			<< " = " << result.to_string();
		cerr << "Comparison::eval - " << tmp << "\n";
	}
 }
ConcreteResourceTypeEtc::ConcreteResourceTypeEtc(
	int		main_index,
	parsedExp&	P_concrete_resource_type_etc,
	parsedExp&	P_resource_data_type,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		concrete_resource_type_etc(P_concrete_resource_type_etc),
		resource_data_type(P_resource_data_type)
	{
	mainIndex = main_index;
	if(concrete_resource_type_etc) {
		initExpression(concrete_resource_type_etc->getData());
	} else {
		initExpression("(null)");
	}
}
ConcreteResourceTypeEtc::ConcreteResourceTypeEtc(const ConcreteResourceTypeEtc& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.concrete_resource_type_etc) {
		concrete_resource_type_etc.reference(p.concrete_resource_type_etc->copy());
	}
	if(p.resource_data_type) {
		resource_data_type.reference(p.resource_data_type->copy());
	}
	initExpression(p.getData());
}
ConcreteResourceTypeEtc::ConcreteResourceTypeEtc(bool, const ConcreteResourceTypeEtc& p)
	: pE(origin_info(p.line, p.file)),
		concrete_resource_type_etc(p.concrete_resource_type_etc),
		resource_data_type(p.resource_data_type) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ConcreteResourceTypeEtc::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(concrete_resource_type_etc) {
		concrete_resource_type_etc->recursively_apply(EA);
	}
	if(resource_data_type) {
		resource_data_type->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ConcreteResourceTypeEtc::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ConcreteResourceTypeEtc::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_number,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_number(P_tok_number)

	{
	mainIndex = main_index;
	if(tok_number) {
		initExpression(tok_number->getData());
	} else {
		initExpression("(null)");
	}
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_false,
	Differentiator_1)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_false(P_tok_false)

	{
	mainIndex = main_index;
	if(tok_false) {
		initExpression(tok_false->getData());
	} else {
		initExpression("(null)");
	}
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_true,
	Differentiator_2)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_true(P_tok_true)

	{
	mainIndex = main_index;
	if(tok_true) {
		initExpression(tok_true->getData());
	} else {
		initExpression("(null)");
	}
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_stringval,
	Differentiator_3)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_stringval(P_tok_stringval)

	{
	mainIndex = main_index;
	if(tok_stringval) {
		initExpression(tok_stringval->getData());
	} else {
		initExpression("(null)");
	}
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_duration,
	Differentiator_4)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_duration(P_tok_duration)

	{
	mainIndex = main_index;
	if(tok_duration) {
		initExpression(tok_duration->getData());
	} else {
		initExpression("(null)");
	}
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_pi,
	Differentiator_5)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_pi(P_tok_pi)

	{
	mainIndex = main_index;
	if(tok_pi) {
		initExpression(tok_pi->getData());
	} else {
		initExpression("(null)");
	}
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_rad,
	Differentiator_6)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_rad(P_tok_rad)

	{
	mainIndex = main_index;
	if(tok_rad) {
		initExpression(tok_rad->getData());
	} else {
		initExpression("(null)");
	}
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_stringval,
	Differentiator_7)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_stringval(P_tok_stringval)

	{
	mainIndex = main_index;
	if(tok_stringval) {
		initExpression(tok_stringval->getData());
	} else {
		initExpression("(null)");
	}
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_time,
	Differentiator_8)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_time(P_tok_time)

	{
	mainIndex = main_index;
	if(tok_time) {
		initExpression(tok_time->getData());
	} else {
		initExpression("(null)");
	}
}
Constant::Constant(
	int		main_index,
	parsedExp&	P_tok_stringval,
	parsedExp&	P_ASCII_58,	// :
	parsedExp&	P_tok_duration,
	Differentiator_9)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_stringval(P_tok_stringval),
		ASCII_58(P_ASCII_58),
		tok_duration(P_tok_duration)
	{
	mainIndex = main_index;
	if(tok_stringval) {
		initExpression(tok_stringval->getData());
	} else {
		initExpression("(null)");
	}
}
 Constant::Constant(const Constant& p)
	: pE(origin_info(p.line, p.file)) {
	val.declared_type = p.val.declared_type;
	val = p.val;
	theData = p.getData();
	is_timesystem_based = p.is_timesystem_based;
	timesystem_duration = p.timesystem_duration;
	timesystem_name = p.timesystem_name;
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_58) {
		ASCII_58.reference(p.ASCII_58->copy());
	}
	if(p.tok_duration) {
		tok_duration.reference(p.tok_duration->copy());
	}
	if(p.tok_duration_2) {
		tok_duration_2.reference(p.tok_duration_2->copy());
	}
	if(p.tok_false) {
		tok_false.reference(p.tok_false->copy());
	}
	if(p.tok_number) {
		tok_number.reference(p.tok_number->copy());
	}
	if(p.tok_pi) {
		tok_pi.reference(p.tok_pi->copy());
	}
	if(p.tok_rad) {
		tok_rad.reference(p.tok_rad->copy());
	}
	if(p.tok_stringval) {
		tok_stringval.reference(p.tok_stringval->copy());
	}
	if(p.tok_time) {
		tok_time.reference(p.tok_time->copy());
	}
	if(p.tok_true) {
		tok_true.reference(p.tok_true->copy());
	}
 }
 Constant::Constant(bool, const Constant& p)
	: pE(origin_info(p.line, p.file)) {
	val.declared_type = p.val.declared_type;
	val = p.val;
	theData = p.getData();
	is_timesystem_based = p.is_timesystem_based;
	timesystem_duration = p.timesystem_duration;
	timesystem_name = p.timesystem_name;
	expressions = p.expressions;
	ASCII_58 = p.ASCII_58;
	tok_duration = p.tok_duration;
	tok_duration_2 = p.tok_duration_2;
	tok_false = p.tok_false;
	tok_number = p.tok_number;
	tok_pi = p.tok_pi;
	tok_rad = p.tok_rad;
	tok_stringval = p.tok_stringval;
	tok_time = p.tok_time;
	tok_true = p.tok_true;
 }
void Constant::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_58) {
		ASCII_58->recursively_apply(EA);
	}
	if(tok_duration) {
		tok_duration->recursively_apply(EA);
	}
	if(tok_duration_2) {
		tok_duration_2->recursively_apply(EA);
	}
	if(tok_false) {
		tok_false->recursively_apply(EA);
	}
	if(tok_number) {
		tok_number->recursively_apply(EA);
	}
	if(tok_pi) {
		tok_pi->recursively_apply(EA);
	}
	if(tok_rad) {
		tok_rad->recursively_apply(EA);
	}
	if(tok_stringval) {
		tok_stringval->recursively_apply(EA);
	}
	if(tok_time) {
		tok_time->recursively_apply(EA);
	}
	if(tok_true) {
		tok_true->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Constant::initExpression(
		const Cstring& nodeData) {
	apgen::DATA_TYPE dt = apgen::DATA_TYPE::UNINITIALIZED;
	is_timesystem_based = false;
	timesystem_name.undefine();
	timesystem_duration = CTime_base(0, 0, true);
	if(tok_duration) {
		theData = tok_duration->getData();
		if(tok_stringval) {
			is_timesystem_based = true;
			timesystem_name = tok_stringval->getData();
			removeQuotes(timesystem_name);
			timesystem_duration = CTime_base(tok_duration->getData());
			val.declared_type = apgen::DATA_TYPE::TIME;
			// NOTE: value assignment will take place later in
			// consolidation
		} else {
			val = CTime_base(theData);
			dt = apgen::DATA_TYPE::DURATION;
			val.declared_type = apgen::DATA_TYPE::DURATION;
			assert(get_result_type() == apgen::DATA_TYPE::DURATION);
		}
		return;
	} else if(tok_number) {
		theData = tok_number->getData();
		if(theData & ".") {
			// fix format so the modeller can understand it...
			if(!("." / theData).length())
				theData = theData + "0";
			if(!(theData / ".").length())
				theData = Cstring("0") + theData;
			val = atof(*theData);
			dt = apgen::DATA_TYPE::FLOATING;
			assert(get_result_type() == apgen::DATA_TYPE::FLOATING);
		} else if(theData & "e") {

			// add the following 2 lines because 'tokens' now
			// accepts engineering-notation numbers w/o decimal point:
			val = atof(*theData);
			dt = apgen::DATA_TYPE::FLOATING;
			assert(get_result_type() == apgen::DATA_TYPE::FLOATING);
		} else if((theData & "E") || (theData & "d") || (theData & "D")) {
			/* ... not to mention 'd' and 'D' ... PFM 2/19/99
			 * ... NOTE: 'e' is standard apgen notation for engineering exponent, as
			 *	enforced by process_number() in grammar_intfc.C. However, just in case,
			 *	since 'atof' is not supposed to understand 'd' or 'D': */
			char	*temp = theData();
			char	*c = temp;

			while(*c) {
				if(*c == 'E' || *c == 'd' || *c == 'D')
					*c = 'e';
				c++;
			}
			val = atof(temp);
			dt = apgen::DATA_TYPE::FLOATING;
			assert(get_result_type() == apgen::DATA_TYPE::FLOATING);
		} else {	// must be integer:
			val = atol(*theData);
			dt = apgen::DATA_TYPE::INTEGER;
			assert(get_result_type() == apgen::DATA_TYPE::INTEGER);
		}
	} else if(tok_stringval) {
		theData = tok_stringval->getData();
		if(tok_stringval->lval == TOK_STRINGVAL) {
			Cstring tmp(theData);
			removeQuotes(tmp);
			val = tmp;
			dt = apgen::DATA_TYPE::STRING;
			assert(get_result_type() == apgen::DATA_TYPE::STRING);
		} else if(tok_stringval->lval == TOK_INSTANCE) {
			val.declared_type = apgen::DATA_TYPE::INSTANCE;
			dt = apgen::DATA_TYPE::INSTANCE;
			assert(get_result_type() == apgen::DATA_TYPE::INSTANCE);
		}
	} else if(tok_time) {
		theData = tok_time->getData();
		val = CTime_base(theData);
		dt = apgen::DATA_TYPE::TIME;
		assert(get_result_type() == apgen::DATA_TYPE::TIME);
	} else if(tok_true) {
		theData = "true";
		val = true;
		dt = apgen::DATA_TYPE::BOOL_TYPE;
		assert(get_result_type() == apgen::DATA_TYPE::BOOL_TYPE);
	} else if(tok_false) {
		theData = "false";
		val = false;
		dt = apgen::DATA_TYPE::BOOL_TYPE;
		assert(get_result_type() == apgen::DATA_TYPE::BOOL_TYPE);
	} else if(tok_pi) {
		theData = tok_pi->getData();
		val = M_PI;
		dt = apgen::DATA_TYPE::FLOATING;
		assert(get_result_type() == apgen::DATA_TYPE::FLOATING);
	} else if(tok_rad) {
		theData = tok_rad->getData();
		val = 180.0 / M_PI;
		dt = apgen::DATA_TYPE::FLOATING;
		assert(get_result_type() == apgen::DATA_TYPE::FLOATING);
	}
	val.declared_type = dt;
 }
 void	Constant::eval_expression(
		behaving_base*	loc,
		TypedValue&	result) {
	if(val.get_type() == apgen::DATA_TYPE::UNINITIALIZED) {
		Cstring err;
		err << "File " << file << ", line " << line << ": constant value "
			<< theData << " was never initialized";
		throw(eval_error(err));
	}
	try {
		result = val;
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ":\n" << Err.msg;
		throw(eval_error(err));
	}
 }
Continue::Continue(
	int		main_index,
	parsedExp&	P_tok_continue,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_continue(P_tok_continue),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(tok_continue) {
		initExpression(tok_continue->getData());
	} else {
		initExpression("(null)");
	}
}
Continue::Continue(const Continue& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.tok_continue) {
		tok_continue.reference(p.tok_continue->copy());
	}
	initExpression(p.getData());
}
Continue::Continue(bool, const Continue& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		tok_continue(p.tok_continue) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Continue::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(tok_continue) {
		tok_continue->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	Continue::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	Continue::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
CustomAttributes::CustomAttributes(
	int		main_index,
	parsedExp&	P_tok_attributes,
	parsedExp&	P_custom_decls,
	parsedExp&	P_tok_end,
	parsedExp&	P_tok_attributes_3,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_attributes(P_tok_attributes),
		custom_decls(P_custom_decls),
		tok_end(P_tok_end),
		tok_attributes_3(P_tok_attributes_3)
	{
	mainIndex = main_index;
	if(tok_attributes) {
		initExpression(tok_attributes->getData());
	} else {
		initExpression("(null)");
	}
}
CustomAttributes::CustomAttributes(const CustomAttributes& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.custom_decls) {
		custom_decls.reference(p.custom_decls->copy());
	}
	if(p.tok_attributes) {
		tok_attributes.reference(p.tok_attributes->copy());
	}
	if(p.tok_attributes_3) {
		tok_attributes_3.reference(p.tok_attributes_3->copy());
	}
	if(p.tok_end) {
		tok_end.reference(p.tok_end->copy());
	}
	initExpression(p.getData());
}
CustomAttributes::CustomAttributes(bool, const CustomAttributes& p)
	: pE(origin_info(p.line, p.file)),
		custom_decls(p.custom_decls),
		tok_attributes(p.tok_attributes),
		tok_attributes_3(p.tok_attributes_3),
		tok_end(p.tok_end) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void CustomAttributes::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(custom_decls) {
		custom_decls->recursively_apply(EA);
	}
	if(tok_attributes) {
		tok_attributes->recursively_apply(EA);
	}
	if(tok_attributes_3) {
		tok_attributes_3->recursively_apply(EA);
	}
	if(tok_end) {
		tok_end->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	CustomAttributes::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	CustomAttributes::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
DataType::DataType(
	int		main_index,
	parsedExp&	P_builtin_type,
	parsedExp&	P_tok_local,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		builtin_type(P_builtin_type),
		tok_local(P_tok_local)
	{
	mainIndex = main_index;
	if(builtin_type) {
		initExpression(builtin_type->getData());
	} else {
		initExpression("(null)");
	}
}
DataType::DataType(
	int		main_index,
	parsedExp&	P_builtin_type,
	Differentiator_1)

	: pE(origin_info(yylineno, aafReader::current_file())),
		builtin_type(P_builtin_type)

	{
	mainIndex = main_index;
	if(builtin_type) {
		initExpression(builtin_type->getData());
	} else {
		initExpression("(null)");
	}
}
DataType::DataType(
	int		main_index,
	parsedExp&	P_tok_dyn_type,
	Differentiator_2)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_dyn_type(P_tok_dyn_type)

	{
	mainIndex = main_index;
	if(tok_dyn_type) {
		initExpression(tok_dyn_type->getData());
	} else {
		initExpression("(null)");
	}
}
DataType::DataType(
	int		main_index,
	parsedExp&	P_builtin_type,
	Differentiator_3)

	: pE(origin_info(yylineno, aafReader::current_file())),
		builtin_type(P_builtin_type)

	{
	mainIndex = main_index;
	if(builtin_type) {
		initExpression(builtin_type->getData());
	} else {
		initExpression("(null)");
	}
}
DataType::DataType(
	int		main_index,
	parsedExp&	P_builtin_type,
	Differentiator_4)

	: pE(origin_info(yylineno, aafReader::current_file())),
		builtin_type(P_builtin_type)

	{
	mainIndex = main_index;
	if(builtin_type) {
		initExpression(builtin_type->getData());
	} else {
		initExpression("(null)");
	}
}
DataType::DataType(const DataType& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.builtin_type) {
		builtin_type.reference(p.builtin_type->copy());
	}
	if(p.tok_dyn_type) {
		tok_dyn_type.reference(p.tok_dyn_type->copy());
	}
	if(p.tok_local) {
		tok_local.reference(p.tok_local->copy());
	}
	initExpression(p.getData());
}
DataType::DataType(bool, const DataType& p)
	: pE(origin_info(p.line, p.file)),
		builtin_type(p.builtin_type),
		tok_dyn_type(p.tok_dyn_type),
		tok_local(p.tok_local) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void DataType::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(builtin_type) {
		builtin_type->recursively_apply(EA);
	}
	if(tok_dyn_type) {
		tok_dyn_type->recursively_apply(EA);
	}
	if(tok_local) {
		tok_local->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	DataType::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	DataType::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
DataTypeDefaultValue::DataTypeDefaultValue(
	int		main_index,
	parsedExp&	P_builtin_type,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_ASCII_41,	// )
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		builtin_type(P_builtin_type),
		ASCII_40(P_ASCII_40),
		ASCII_41(P_ASCII_41)
	{
	mainIndex = main_index;
	if(builtin_type) {
		initExpression(builtin_type->getData());
	} else {
		initExpression("(null)");
	}
}
DataTypeDefaultValue::DataTypeDefaultValue(
	int		main_index,
	parsedExp&	P_tok_dyn_type,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_ASCII_41,	// )
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_dyn_type(P_tok_dyn_type),
		ASCII_40(P_ASCII_40),
		ASCII_41(P_ASCII_41)
	{
	mainIndex = main_index;
	if(tok_dyn_type) {
		initExpression(tok_dyn_type->getData());
	} else {
		initExpression("(null)");
	}
}
DataTypeDefaultValue::DataTypeDefaultValue(const DataTypeDefaultValue& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_40) {
		ASCII_40.reference(p.ASCII_40->copy());
	}
	if(p.ASCII_40_0) {
		ASCII_40_0.reference(p.ASCII_40_0->copy());
	}
	if(p.ASCII_41) {
		ASCII_41.reference(p.ASCII_41->copy());
	}
	if(p.ASCII_41_2) {
		ASCII_41_2.reference(p.ASCII_41_2->copy());
	}
	if(p.builtin_type) {
		builtin_type.reference(p.builtin_type->copy());
	}
	if(p.tok_dyn_type) {
		tok_dyn_type.reference(p.tok_dyn_type->copy());
	}
	initExpression(p.getData());
}
DataTypeDefaultValue::DataTypeDefaultValue(bool, const DataTypeDefaultValue& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_40(p.ASCII_40),
		ASCII_40_0(p.ASCII_40_0),
		ASCII_41(p.ASCII_41),
		ASCII_41_2(p.ASCII_41_2),
		builtin_type(p.builtin_type),
		tok_dyn_type(p.tok_dyn_type) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void DataTypeDefaultValue::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_40) {
		ASCII_40->recursively_apply(EA);
	}
	if(ASCII_40_0) {
		ASCII_40_0->recursively_apply(EA);
	}
	if(ASCII_41) {
		ASCII_41->recursively_apply(EA);
	}
	if(ASCII_41_2) {
		ASCII_41_2->recursively_apply(EA);
	}
	if(builtin_type) {
		builtin_type->recursively_apply(EA);
	}
	if(tok_dyn_type) {
		tok_dyn_type->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	DataTypeDefaultValue::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
 void	DataTypeDefaultValue::eval_expression(
			behaving_base*	loc,
			TypedValue&	result) {
	result = val;
 }
Declaration::Declaration(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_ASCII_58,	// :
	parsedExp&	P_param_scope_and_type,
	parsedExp&	P_tok_default,
	parsedExp&	P_tok_to,
	parsedExp&	P_Expression,
	parsedExp&	P_opt_range,
	parsedExp&	P_opt_descr,
	parsedExp&	P_opt_units,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		ASCII_58(P_ASCII_58),
		param_scope_and_type(P_param_scope_and_type),
		tok_default(P_tok_default),
		tok_to(P_tok_to),
		Expression(P_Expression),
		opt_range(P_opt_range),
		opt_descr(P_opt_descr),
		opt_units(P_opt_units),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
Declaration::Declaration(const Declaration& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_58) {
		ASCII_58.reference(p.ASCII_58->copy());
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.opt_descr) {
		opt_descr.reference(p.opt_descr->copy());
	}
	if(p.opt_range) {
		opt_range.reference(p.opt_range->copy());
	}
	if(p.opt_units) {
		opt_units.reference(p.opt_units->copy());
	}
	if(p.param_scope_and_type) {
		param_scope_and_type.reference(p.param_scope_and_type->copy());
	}
	if(p.tok_default) {
		tok_default.reference(p.tok_default->copy());
	}
	if(p.tok_id) {
		tok_id.reference(p.tok_id->copy());
	}
	if(p.tok_to) {
		tok_to.reference(p.tok_to->copy());
	}
	initExpression(p.getData());
}
Declaration::Declaration(bool, const Declaration& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_58(p.ASCII_58),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		opt_descr(p.opt_descr),
		opt_range(p.opt_range),
		opt_units(p.opt_units),
		param_scope_and_type(p.param_scope_and_type),
		tok_default(p.tok_default),
		tok_id(p.tok_id),
		tok_to(p.tok_to) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Declaration::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_58) {
		ASCII_58->recursively_apply(EA);
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(opt_descr) {
		opt_descr->recursively_apply(EA);
	}
	if(opt_range) {
		opt_range->recursively_apply(EA);
	}
	if(opt_units) {
		opt_units->recursively_apply(EA);
	}
	if(param_scope_and_type) {
		param_scope_and_type->recursively_apply(EA);
	}
	if(tok_default) {
		tok_default->recursively_apply(EA);
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	if(tok_to) {
		tok_to->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Declaration::initExpression(
		const Cstring& nodeData) {
	theData = "Declaration";
 }
void	Declaration::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Declarations::Declarations(
	int		main_index,
	parsedExp&	P_declaration,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		declaration(P_declaration)

	{
	mainIndex = main_index;
	if(declaration) {
		initExpression(declaration->getData());
	} else {
		initExpression("(null)");
	}
}
Declarations::Declarations(const Declarations& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.declaration) {
		declaration.reference(p.declaration->copy());
	}
	initExpression(p.getData());
}
Declarations::Declarations(bool, const Declarations& p)
	: pE(origin_info(p.line, p.file)),
		declaration(p.declaration) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Declarations::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(declaration) {
		declaration->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Declarations::initExpression(
		const Cstring& nodeData) {
	theData = "Declarations";
 }
void	Declarations::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Decomp::Decomp(
	int		main_index,
	parsedExp&	P_tok_act_type,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_optional_expression_list,
	parsedExp&	P_ASCII_41,	// )
	parsedExp&	P_temporalSpecification,
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_act_type(P_tok_act_type),
		ASCII_40(P_ASCII_40),
		optional_expression_list(P_optional_expression_list),
		ASCII_41(P_ASCII_41),
		temporalSpecification(P_temporalSpecification)
	{
	mainIndex = main_index;
	if(tok_act_type) {
		initExpression(tok_act_type->getData());
	} else {
		initExpression("(null)");
	}
}
Decomp::Decomp(
	int		main_index,
	parsedExp&	P_tok_call,
	parsedExp&	P_call_or_spawn_arguments,
	parsedExp&	P_temporalSpecification,
	Differentiator_1)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_call(P_tok_call),
		call_or_spawn_arguments(P_call_or_spawn_arguments),
		temporalSpecification(P_temporalSpecification)
	{
	mainIndex = main_index;
	if(tok_call) {
		initExpression(tok_call->getData());
	} else {
		initExpression("(null)");
	}
}
Decomp::Decomp(
	int		main_index,
	parsedExp&	P_tok_spawn,
	parsedExp&	P_call_or_spawn_arguments,
	parsedExp&	P_temporalSpecification,
	Differentiator_2)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_spawn(P_tok_spawn),
		call_or_spawn_arguments(P_call_or_spawn_arguments),
		temporalSpecification(P_temporalSpecification)
	{
	mainIndex = main_index;
	if(tok_spawn) {
		initExpression(tok_spawn->getData());
	} else {
		initExpression("(null)");
	}
}
 Decomp::Decomp(const Decomp& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_40) {
		ASCII_40.reference(p.ASCII_40->copy());
	}
	if(p.ASCII_41) {
		ASCII_41.reference(p.ASCII_41->copy());
	}
	if(p.call_or_spawn_arguments) {
		call_or_spawn_arguments.reference(p.call_or_spawn_arguments->copy());
	}
	if(p.call_or_spawn_arguments_1) {
		call_or_spawn_arguments_1.reference(p.call_or_spawn_arguments_1->copy());
	}
	// if(p.cond_spec) {
	// 	cond_spec.reference(p.cond_spec->copy());
	// }
	// if(p.cond_spec_3) {
	// 	cond_spec_3.reference(p.cond_spec_3->copy());
	// }
	if(p.optional_expression_list) {
		optional_expression_list.reference(p.optional_expression_list->copy());
	}
	if(p.temporalSpecification) {
		temporalSpecification.reference(p.temporalSpecification->copy());
	}
	if(p.temporalSpecification_2) {
		temporalSpecification_2.reference(p.temporalSpecification_2->copy());
	}
	if(p.tok_act_type) {
		tok_act_type.reference(p.tok_act_type->copy());
	}
	if(p.tok_call) {
		tok_call.reference(p.tok_call->copy());
	}
	if(p.tok_spawn) {
		tok_spawn.reference(p.tok_spawn->copy());
	}
	initExpression( p.getData());
	for(int k = 0; k < p.ActualArguments.size(); k++) {
		ActualArguments.push_back(parsedExp(p.ActualArguments[k]->copy()));
	}
 }
Decomp::Decomp(bool, const Decomp& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_40(p.ASCII_40),
		ASCII_41(p.ASCII_41),
		call_or_spawn_arguments(p.call_or_spawn_arguments),
		call_or_spawn_arguments_1(p.call_or_spawn_arguments_1),
		optional_expression_list(p.optional_expression_list),
		temporalSpecification(p.temporalSpecification),
		temporalSpecification_2(p.temporalSpecification_2),
		tok_act_type(p.tok_act_type),
		tok_call(p.tok_call),
		tok_spawn(p.tok_spawn) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Decomp::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_40) {
		ASCII_40->recursively_apply(EA);
	}
	if(ASCII_41) {
		ASCII_41->recursively_apply(EA);
	}
	if(call_or_spawn_arguments) {
		call_or_spawn_arguments->recursively_apply(EA);
	}
	if(call_or_spawn_arguments_1) {
		call_or_spawn_arguments_1->recursively_apply(EA);
	}
	if(optional_expression_list) {
		optional_expression_list->recursively_apply(EA);
	}
	if(temporalSpecification) {
		temporalSpecification->recursively_apply(EA);
	}
	if(temporalSpecification_2) {
		temporalSpecification_2->recursively_apply(EA);
	}
	if(tok_act_type) {
		tok_act_type->recursively_apply(EA);
	}
	if(tok_call) {
		tok_call->recursively_apply(EA);
	}
	if(tok_spawn) {
		tok_spawn->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Decomp::initExpression(
		const Cstring& nodeData) {
	Type = NULL;
	statementType = apgen::DECOMP_STATEMENT_TYPE::ACTIVITY_CONSTRUCTOR;
 }
void	Decomp::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
DefaultExpression::DefaultExpression(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
DefaultExpression::DefaultExpression(const DefaultExpression& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	initExpression(p.getData());
}
DefaultExpression::DefaultExpression(bool, const DefaultExpression& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void DefaultExpression::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	DefaultExpression::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	DefaultExpression::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Directive::Directive(
	int		main_index,
	parsedExp&	P_one_declarative_assignment,
	parsedExp&	P_tok_directive,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		one_declarative_assignment(P_one_declarative_assignment),
		tok_directive(P_tok_directive)
	{
	mainIndex = main_index;
	if(one_declarative_assignment) {
		initExpression(one_declarative_assignment->getData());
	} else {
		initExpression("(null)");
	}
}
Directive::Directive(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_directive,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_directive(P_tok_directive),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
Directive::Directive(const Directive& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.one_declarative_assignment) {
		one_declarative_assignment.reference(p.one_declarative_assignment->copy());
	}
	if(p.tok_directive) {
		tok_directive.reference(p.tok_directive->copy());
	}
	if(p.tok_directive_0) {
		tok_directive_0.reference(p.tok_directive_0->copy());
	}
	initExpression(p.getData());
}
Directive::Directive(bool, const Directive& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		one_declarative_assignment(p.one_declarative_assignment),
		tok_directive(p.tok_directive),
		tok_directive_0(p.tok_directive_0) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Directive::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(one_declarative_assignment) {
		one_declarative_assignment->recursively_apply(EA);
	}
	if(tok_directive) {
		tok_directive->recursively_apply(EA);
	}
	if(tok_directive_0) {
		tok_directive_0->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Directive::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	if(one_declarative_assignment) {
		executableExp* ee = dynamic_cast<executableExp*>(
			one_declarative_assignment.object());
		assert(ee);
		assignment.reference(ee);
	}
 }
void	Directive::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
EqualityTest::EqualityTest(
	int		main_index,
	parsedExp&	P_tok_equal,
	parsedExp&	P_maybe_equal,
	parsedExp&	P_maybe_equal_2,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_equal(P_tok_equal),
		maybe_equal(P_maybe_equal),
		maybe_equal_2(P_maybe_equal_2)
	{
	mainIndex = main_index;
	if(tok_equal) {
		initExpression(tok_equal->getData());
	} else {
		initExpression("(null)");
	}
}
EqualityTest::EqualityTest(
	int		main_index,
	parsedExp&	P_tok_notequal,
	parsedExp&	P_maybe_equal,
	parsedExp&	P_maybe_equal_2,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_notequal(P_tok_notequal),
		maybe_equal(P_maybe_equal),
		maybe_equal_2(P_maybe_equal_2)
	{
	mainIndex = main_index;
	if(tok_notequal) {
		initExpression(tok_notequal->getData());
	} else {
		initExpression("(null)");
	}
}
EqualityTest::EqualityTest(const EqualityTest& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.maybe_equal) {
		maybe_equal.reference(p.maybe_equal->copy());
	}
	if(p.maybe_equal_0) {
		maybe_equal_0.reference(p.maybe_equal_0->copy());
	}
	if(p.maybe_equal_2) {
		maybe_equal_2.reference(p.maybe_equal_2->copy());
	}
	if(p.tok_equal) {
		tok_equal.reference(p.tok_equal->copy());
	}
	if(p.tok_notequal) {
		tok_notequal.reference(p.tok_notequal->copy());
	}
	initExpression(p.getData());
}
EqualityTest::EqualityTest(bool, const EqualityTest& p)
	: pE(origin_info(p.line, p.file)),
		maybe_equal(p.maybe_equal),
		maybe_equal_0(p.maybe_equal_0),
		maybe_equal_2(p.maybe_equal_2),
		tok_equal(p.tok_equal),
		tok_notequal(p.tok_notequal) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void EqualityTest::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(maybe_equal) {
		maybe_equal->recursively_apply(EA);
	}
	if(maybe_equal_0) {
		maybe_equal_0->recursively_apply(EA);
	}
	if(maybe_equal_2) {
		maybe_equal_2->recursively_apply(EA);
	}
	if(tok_equal) {
		tok_equal->recursively_apply(EA);
	}
	if(tok_notequal) {
		tok_notequal->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	EqualityTest::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	if(tok_equal) {
		Operator = tok_equal;
		binaryFunc = func_equal;
	} else if(tok_notequal) {
		Operator = tok_notequal;
		binaryFunc = func_NOTEQUAL;
	}
 }
 void	EqualityTest::eval_expression(
		behaving_base*	local_object,
		TypedValue&	result
		) {
	TypedValue val1, val2;
	try {
	    maybe_equal->eval_expression(local_object, val1);
	    maybe_equal_2->eval_expression(local_object, val2);
	    binaryFunc(
		val1,
		val2,
		result);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ":\n" << Err.msg;
		throw(eval_error(err));
	}

	if(APcloptions::theCmdLineOptions().debug_execute) {
		Cstring tmp = val1.to_string();
		tmp << " " << theData << " " << val2.to_string()
			<< " = " << result.to_string();
		cerr << "EqualityTest::eval - " << tmp << "\n";
	}
 }
ExpressionList::ExpressionList(
	int		main_index,
	parsedExp&	P_Expression,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression)

	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
ExpressionList::ExpressionList(const ExpressionList& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	initExpression(p.getData());
}
ExpressionList::ExpressionList(bool, const ExpressionList& p)
	: pE(origin_info(p.line, p.file)),
		Expression(p.Expression) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ExpressionList::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ExpressionList::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ExpressionList::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ExpressionListPrecomp::ExpressionListPrecomp(
	int		main_index,
	parsedExp&	P_one_series,
	parsedExp&	P_time_series_keyword,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		one_series(P_one_series),
		time_series_keyword(P_time_series_keyword)
	{
	mainIndex = main_index;
	if(one_series) {
		initExpression(one_series->getData());
	} else {
		initExpression("(null)");
	}
}
ExpressionListPrecomp::ExpressionListPrecomp(const ExpressionListPrecomp& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.one_series) {
		one_series.reference(p.one_series->copy());
	}
	if(p.time_series_keyword) {
		time_series_keyword.reference(p.time_series_keyword->copy());
	}
	initExpression(p.getData());
}
ExpressionListPrecomp::ExpressionListPrecomp(bool, const ExpressionListPrecomp& p)
	: pE(origin_info(p.line, p.file)),
		one_series(p.one_series),
		time_series_keyword(p.time_series_keyword) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ExpressionListPrecomp::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(one_series) {
		one_series->recursively_apply(EA);
	}
	if(time_series_keyword) {
		time_series_keyword->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	ExpressionListPrecomp::initExpression(
		const Cstring& nodeData) {

	//
	// This list contains the two high-level arguments
	// to time_series(). The first is a regular list,
	// the second is a custom, high-efficiency array
	// of keyword-value pairs.
	//
	theData = nodeData;
 }
void	ExpressionListPrecomp::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
File::File(
	int		main_index,
	parsedExp&	P_adaptation_item,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		adaptation_item(P_adaptation_item)

	{
	mainIndex = main_index;
	if(adaptation_item) {
		initExpression(adaptation_item->getData());
	} else {
		initExpression("(null)");
	}
}
File::File(
	int		main_index,
	parsedExp&	P_plan_item,
	Differentiator_1)

	: pE(origin_info(yylineno, aafReader::current_file())),
		plan_item(P_plan_item)

	{
	mainIndex = main_index;
	if(plan_item) {
		initExpression(plan_item->getData());
	} else {
		initExpression("(null)");
	}
}
File::File(const File& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.adaptation_item) {
		adaptation_item.reference(p.adaptation_item->copy());
	}
	if(p.plan_item) {
		plan_item.reference(p.plan_item->copy());
	}
	initExpression(p.getData());
}
File::File(bool, const File& p)
	: pE(origin_info(p.line, p.file)),
		adaptation_item(p.adaptation_item),
		plan_item(p.plan_item) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void File::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(adaptation_item) {
		adaptation_item->recursively_apply(EA);
	}
	if(plan_item) {
		plan_item->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	File::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	File::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_tok_internal_func,
	parsedExp&	P_zero_or_more_args,
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_internal_func(P_tok_internal_func),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(tok_internal_func) {
		initExpression(tok_internal_func->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_tok_local_function,
	parsedExp&	P_zero_or_more_args,
	Differentiator_1)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_local_function(P_tok_local_function),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(tok_local_function) {
		initExpression(tok_local_function->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_symbol,
	parsedExp&	P_zero_or_more_args,
	Differentiator_2)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		symbol(P_symbol),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(symbol) {
		initExpression(symbol->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_tok_internal_func,
	parsedExp&	P_zero_or_more_args,
	Differentiator_3)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_internal_func(P_tok_internal_func),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(tok_internal_func) {
		initExpression(tok_internal_func->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_tok_local_function,
	parsedExp&	P_zero_or_more_args,
	Differentiator_4)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_local_function(P_tok_local_function),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(tok_local_function) {
		initExpression(tok_local_function->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_tok_method,
	parsedExp&	P_zero_or_more_args,
	Differentiator_5)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_method(P_tok_method),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(tok_method) {
		initExpression(tok_method->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_tok_method,
	parsedExp&	P_tok_spawn,
	parsedExp&	P_zero_or_more_args,
	Differentiator_6)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_method(P_tok_method),
		tok_spawn(P_tok_spawn),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(tok_method) {
		initExpression(tok_method->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_qualified_symbol,
	parsedExp&	P_zero_or_more_args,
	Differentiator_7)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		qualified_symbol(P_qualified_symbol),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(qualified_symbol) {
		initExpression(qualified_symbol->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_tok_internal_func,
	parsedExp&	P_zero_or_more_args,
	Differentiator_8)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_internal_func(P_tok_internal_func),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(tok_internal_func) {
		initExpression(tok_internal_func->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionCall::FunctionCall(
	int		main_index,
	parsedExp&	P_tok_local_function,
	parsedExp&	P_zero_or_more_args,
	Differentiator_9)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_local_function(P_tok_local_function),
		zero_or_more_args(P_zero_or_more_args)
	{
	mainIndex = main_index;
	if(tok_local_function) {
		initExpression(tok_local_function->getData());
	} else {
		initExpression("(null)");
	}
}
 FunctionCall::FunctionCall(const FunctionCall& fc)
		: executableExp(origin_info(fc.line, fc.file)) {
	for(int i = 0; i < fc.actual_arguments.size(); i++) {
		if(fc.actual_arguments[i]) {
			parsedExp pe(fc.actual_arguments[i]->copy());
			actual_arguments.push_back(pe);
		}
	}
	if(fc.tok_internal_func) {
		tok_internal_func.reference(fc.tok_internal_func->copy());
	}
	if(fc.tok_local_function) {
		tok_local_function.reference(fc.tok_local_function->copy());
	}
	if(fc.zero_or_more_args) {
		zero_or_more_args.reference(fc.zero_or_more_args->copy());
	}
	func = fc.func;
	TaskInvoked = fc.TaskInvoked;
	theData = fc.theData;
	for(int i = 0; i < fc.expressions.size(); i++) {
		if(fc.expressions[i]) {
			expressions.push_back(parsedExp(fc.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(fc.indices) {
		MultidimIndices* mi = dynamic_cast<MultidimIndices*>(fc.indices->copy());
		indices.reference(mi);
	}
	if(fc.a_member) {
		ClassMember* cm = dynamic_cast<ClassMember*>(fc.a_member->copy());
		a_member.reference(cm);
	}
	if(fc.qualified_symbol) {
		qualified_symbol.reference(fc.qualified_symbol->copy());
	}
 }
 FunctionCall::FunctionCall(bool, const FunctionCall& fc)
		: executableExp(origin_info(fc.line, fc.file)) {
	actual_arguments = fc.actual_arguments;
	tok_internal_func = fc.tok_internal_func;
	tok_local_function = fc.tok_local_function;
	zero_or_more_args = fc.zero_or_more_args;
	func = fc.func;
	TaskInvoked = fc.TaskInvoked;
	theData = fc.theData;
	expressions = fc.expressions;
	indices = fc.indices;
	a_member = fc.a_member;
	qualified_symbol = fc.qualified_symbol;
 }
 void	FunctionCall::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);

	//
	// This is appropriate for FunctionCalls that have
	// been consolidated:
	//

	for(int i = 0; i < actual_arguments.size(); i++) {
		actual_arguments[i]->recursively_apply(EA);
	}
	if(TaskInvoked && TaskInvoked->prog) {
	    // if(!EA.inhibit(TaskInvoked->prog.object())) {
		TaskInvoked->prog->recursively_apply(EA);
	    // }
	}

	//
	// ... but in case they haven't been yet, we use
	// this:
	//

	if(qualified_symbol) {
		qualified_symbol->recursively_apply(EA);
	}

	decomp_finder* DF = dynamic_cast<decomp_finder*>(&EA);
	if(!DF) {

		//
		// The name of the function is sometimes parsed as
		// a symbol; this works for functions that are
		// invoked before they are declared or defined.
		// In that case, we do not want the symbol to
		// be interpreted as a global by decomp_finder.
		//
		if(symbol) {
			symbol->recursively_apply(EA);
		}
	}
	if(zero_or_more_args) {
		zero_or_more_args->recursively_apply(EA);
	}

	EA.post_analyze(this);
 }
 void	FunctionCall::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	func = NULL;
	TaskInvoked = NULL;
 }
 void	FunctionCall::eval_expression(
		behaving_base*	local_object,
		TypedValue&	result
		) {
    TypedValue*	the_result = &result;
    bool	post_processing_needed = indices || a_member;
    TypedValue	val;

    if(post_processing_needed) {
	the_result = &val;
    }
    if(func) {
	generic_function	gf = func->payload.Func;

	// debug
	func->payload.count++;

	//
	// Handle a special case first
	//
	if(gf == exp_make_constant) {
	    pE* obj = actual_arguments[0].object();
	    Symbol* sym = dynamic_cast<Symbol*>(obj);
	    if(!sym) {
		Cstring err;
		err << "File " << file << ", line " << line << ": "
		    << "argument of function make_constant() is "
		    << sym->getData()
		    << "; expected a global array";
		throw(eval_error(err));
	    }
	    TypedValue& symval = sym->get_val_ref(local_object);
	    if(!symval.is_array()) {
		Cstring err;
		err << "File " << file << ", line " << line << ": "
		    << "argument of function make_constant is "
		    << apgen::spell(symval.get_type())
		    << "; expected a global array";
		throw(eval_error(err));
	    }
	    ListOVal& symarray = symval.get_array();
	    // symval.recursively_reset_modifiers();
	    return;
	}

	//
	// Now, return to the general case
	//
	slst<TypedValue>	args;
	slst<TypedValue*>	arg_refs;

	for(int k = 0; k < actual_arguments.size(); k++) {
	    pE* obj = actual_arguments[k].object();
	    try {

		//
		// we push the evaluated result onto the
		// argument stack for the function's benefit
		//
		args.push_front(TypedValue());
		TypedValue* val = &args.head->obj;

		//
		// debug
		//
		// TypedValue::interesting = true;

		obj->eval_expression(local_object, *val);

		//
		// debug
		//
		// TypedValue::interesting = false;

		arg_refs.push_front(val);
	    } catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ": "
		    << "error in evaluating argument " << (k+1)
		    << " of function "
		    << getData() << ":\n" << Err.msg;
		throw(eval_error(err));
	    }
	}
	Cstring error;

	//
	// Check for 1st letter of "internal"
	//
	if((*func->payload.Origin)[0] == 'i') {

	    //
	    // Internal function
	    //
	    if(gf(	error,
			the_result,
			arg_refs) != apgen::RETURN_STATUS::SUCCESS) {
		Cstring err;
		err << "File " << file << ", line " << line << ": "
		    << "error executing function "
		    << getData() << ": " << error;
		throw(eval_error(err));
	    }
	} else {

	    //
	    // Origin is "user-defined".
	    //
	    // User-defined function; may not be thread-safe
	    //
	    static mutex 		mutx3;
	    lock_guard<mutex>	lock(mutx3);
	    if(gf(	error,
			the_result,
			arg_refs) != apgen::RETURN_STATUS::SUCCESS) {
		Cstring err;
		err << "File " << file << ", line " << line << ": "
		    << "error executing function "
		    << getData() << ": " << error;
		throw(eval_error(err));
	    }
	}
    } else if(TaskInvoked && TaskInvoked->prog) {
	// must execute a task
	TypedValue returnedval;

	// update the call count
	TaskInvoked->called_once();

	//
	// We need to define an object that is suitable
	// for executing the function, and install the evaluated
	// results in the appropriate value locations
	//
	function_object* fo = NULL;
	behaving_element fbe;
	apgen::METHOD_TYPE method_type = TaskInvoked->prog->orig_section;
	if(method_type != apgen::METHOD_TYPE::FUNCTION) {

	    //
	    // We want to prevent this. A method should
	    // only be invoked via execute().
	    //
	    Cstring err;
	    err << "File " << file << ", line " << line << ": "
		<< "method " << getData()
		<< " should only be executed, not evaluated.\n";
	    throw(eval_error(err));
	}

	//
	// We are dealing with an ordinary function call.
	//
	fbe.reference(fo = new function_object(
				returnedval,
				*TaskInvoked));

	for(int k = 0; k < actual_arguments.size(); k++) {
	    TypedValue val;
	    pE* obj = actual_arguments[k].object();
	    int pk = TaskInvoked->paramindex[k];
	    obj->eval_expression(local_object, val);
	    (*fo)[pk] = val;
	}

	execution_context* context;
	smart_ptr<execution_context> EC;
	execution_context::return_code Code = execution_context::REGULAR;
	try {
	    //
	    // invoke Execute - NOTE: at some point, replace the constant
	    // strategy by invoking a global that should be set
	    // appropriately when evaluating resource profiles...
	    //
	    context = new execution_context(
		    fbe,
		    TaskInvoked->prog.object());
	    EC.reference(context);
	    context->ExCon(Code);
	} catch(eval_error& Err) {

	    Cstring err;
	    err << "File " << file << ", line " << line << ", "
	        << "in local function \"" << getData() << "\":\n"
	        << Err.msg;
	    throw (eval_error(err));
	}
	if(Code != execution_context::RETURNING) {
 #	    define DONT_WORRY_ABOUT_IT_HERE_LET_THE_CALLER_DEAL_WITH_THIS
 #	    ifndef DONT_WORRY_ABOUT_IT_HERE_LET_THE_CALLER_DEAL_WITH_THIS
	    Cstring err;
	    err << "File " << file << ", line " << line << ": ";
	    err << " local function \"" << getData() << "\" is not "
	    "returning anything; you should not use it in an assignment";
	    throw (eval_error(err));
 #	    endif /* DONT_WORRY_ABOUT_IT_HERE_LET_THE_CALLER_DEAL_WITH_THIS */
	} else {
	    *the_result = fo->theReturnedValue;
	}
    }

    else {

	//
	// Must be a PrecomputedResource; evaluation is done via
	// hard-coded cubic interpolation rather than a DSL-coded
	// program.
	//
	eval_precomputed_expression(local_object, result);
    }

    if(post_processing_needed) {
	pEsys::ClassMember* cm;
	if(indices) {
	    the_result = indices->apply_indices_to(the_result, local_object);
	    if((cm = a_member.object())) {
		behaving_base* instance_obj;

		//
		// dereference the instance pointer
		//
		if(	!the_result->is_instance()
			|| !(instance_obj = the_result->get_object().object())) {
		    Cstring err;
		    err << "File " << file << ", line " << line << ": ";
		    err << " local function \"" << getData() << "\" is not "
			<< "returning a valid instance; cannot get member "
			<< cm->symbol->getData();
		    throw (eval_error(err));
		}
		the_result = cm->eval_expression_special(
					instance_obj,
					cm->symbol->getData());
	    }
	} else if((cm = a_member.object())) {
	    behaving_base* instance_obj;

	    //
	    // dereference the instance pointer
	    //
	    if(	!the_result->is_instance()
	    	|| !(instance_obj = the_result->get_object().object())) {
		Cstring err;
		err << "File " << file << ", line " << line << ": ";
		err << " local function \"" << getData() << "\" is not "
		    << "returning a valid instance; cannot get member "
		    << cm->symbol->getData();
		throw (eval_error(err));
	    }
	    the_result = cm->eval_expression_special(
					instance_obj,
					cm->symbol->getData());
	}
	result = *the_result;
    }
 }
FunctionDeclaration::FunctionDeclaration(
	int		main_index,
	parsedExp&	P_start_function_decl,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_signature,
	parsedExp&	P_ASCII_41,	// )
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		start_function_decl(P_start_function_decl),
		ASCII_40(P_ASCII_40),
		signature(P_signature),
		ASCII_41(P_ASCII_41),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(start_function_decl) {
		initExpression(start_function_decl->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionDeclaration::FunctionDeclaration(const FunctionDeclaration& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_40) {
		ASCII_40.reference(p.ASCII_40->copy());
	}
	if(p.ASCII_41) {
		ASCII_41.reference(p.ASCII_41->copy());
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.signature) {
		signature.reference(p.signature->copy());
	}
	if(p.start_function_decl) {
		start_function_decl.reference(p.start_function_decl->copy());
	}
	initExpression(p.getData());
}
FunctionDeclaration::FunctionDeclaration(bool, const FunctionDeclaration& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_40(p.ASCII_40),
		ASCII_41(p.ASCII_41),
		ASCII_59(p.ASCII_59),
		signature(p.signature),
		start_function_decl(p.start_function_decl) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void FunctionDeclaration::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_40) {
		ASCII_40->recursively_apply(EA);
	}
	if(ASCII_41) {
		ASCII_41->recursively_apply(EA);
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(signature) {
		signature->recursively_apply(EA);
	}
	if(start_function_decl) {
		start_function_decl->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	FunctionDeclaration::initExpression(
		const Cstring& nodeData) {
	//
	// make sure that
	//    1. we grab the function name, but
	//    2. we allow multiple declarations
	//    3. a duplicate definition is an error
	//
	if(!aafReader::functions().find(nodeData)) {
		aafReader::functions() << new emptySymbol(nodeData);
	}
	theData = nodeData;
 }
void	FunctionDeclaration::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
FunctionDefinition::FunctionDefinition(
	int		main_index,
	parsedExp&	P_start_function_def,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_optional_func_params,
	parsedExp&	P_ASCII_123,	// {
	parsedExp&	P_program,
	parsedExp&	P_ASCII_125,	// }
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		start_function_def(P_start_function_def),
		ASCII_40(P_ASCII_40),
		optional_func_params(P_optional_func_params),
		ASCII_123(P_ASCII_123),
		program(P_program),
		ASCII_125(P_ASCII_125)
	{
	mainIndex = main_index;
	if(start_function_def) {
		initExpression(start_function_def->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionDefinition::FunctionDefinition(const FunctionDefinition& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_123) {
		ASCII_123.reference(p.ASCII_123->copy());
	}
	if(p.ASCII_125) {
		ASCII_125.reference(p.ASCII_125->copy());
	}
	if(p.ASCII_40) {
		ASCII_40.reference(p.ASCII_40->copy());
	}
	if(p.optional_func_params) {
		optional_func_params.reference(p.optional_func_params->copy());
	}
	if(p.program) {
		program.reference(p.program->copy());
	}
	if(p.start_function_def) {
		start_function_def.reference(p.start_function_def->copy());
	}
	initExpression(p.getData());
}
FunctionDefinition::FunctionDefinition(bool, const FunctionDefinition& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_123(p.ASCII_123),
		ASCII_125(p.ASCII_125),
		ASCII_40(p.ASCII_40),
		optional_func_params(p.optional_func_params),
		program(p.program),
		start_function_def(p.start_function_def) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void FunctionDefinition::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_123) {
		ASCII_123->recursively_apply(EA);
	}
	if(ASCII_125) {
		ASCII_125->recursively_apply(EA);
	}
	if(ASCII_40) {
		ASCII_40->recursively_apply(EA);
	}
	if(optional_func_params) {
		optional_func_params->recursively_apply(EA);
	}
	if(program) {
		program->recursively_apply(EA);
	}
	if(start_function_def) {
		start_function_def->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	FunctionDefinition::initExpression(
		const Cstring& nodeData) {
    FunctionIdentity* identity = dynamic_cast<FunctionIdentity*>(
					start_function_def.object());
    assert(identity);
    bool is_model = identity->tok_model;
    bool is_res = identity->tok_resource;
    bool is_decomp = identity->tok_decomposition;

    if(is_model || is_res || is_decomp) {

	//
	// we are defining a method
	//
	if(aafReader::methods().find(nodeData)) {
	    Cstring err;
	    err << "File " << file << ", line " << line << ": "
		<< "duplicate definition of method '"
		<< nodeData << "'.";
	    throw(eval_error(err));
	}
	if(is_model) {
	    aafReader::methods()
		<< new Cnode0<alpha_string, apgen::METHOD_TYPE>(nodeData, apgen::METHOD_TYPE::MODELING);
	} else if(is_res) {
	    aafReader::methods()
		<< new Cnode0<alpha_string, apgen::METHOD_TYPE>(nodeData, apgen::METHOD_TYPE::RESUSAGE);
	} else if(is_decomp) {
	    if(identity->tok_decomposition->getData() == "nonexclusive_decomposition") {
		aafReader::methods()
		    << new Cnode0<alpha_string,
				apgen::METHOD_TYPE>(nodeData, apgen::METHOD_TYPE::NONEXCLDECOMP);
	    } else if(identity->tok_decomposition->getData() == "decomposition") {
		aafReader::methods()
		    << new Cnode0<alpha_string,
				apgen::METHOD_TYPE>(nodeData, apgen::METHOD_TYPE::DECOMPOSITION);
	    } else if(identity->tok_decomposition->getData() == "concurrent_expansion"
		      || identity->tok_decomposition->getData() == "expansion") {
		aafReader::methods()
		    << new Cnode0<alpha_string,
				apgen::METHOD_TYPE>(nodeData, apgen::METHOD_TYPE::CONCUREXP);
	    }
	}
    } else {

	//
	// we are defining a function
	//

	//
	// make sure that
	//    1. we grab the function name, but
	//    2. we allow multiple declarations
	//    3. a duplicate definition is an error
	//
	if(aafReader::defined_functions().find(nodeData)) {
	    Cstring err;
	    err << "File " << file << ", line " << line << ": "
		<< "duplicate definition of function '"
		<< nodeData << "'.";
	    throw(eval_error(err));
	}
	aafReader::defined_functions() << new emptySymbol(nodeData);
	if(!aafReader::functions().find(nodeData)) {
	    aafReader::functions() << new emptySymbol(nodeData);
	}
    }
    theData = nodeData;
 }
void	FunctionDefinition::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
FunctionIdentity::FunctionIdentity(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_tok_func,
	parsedExp&	P_function_return_type,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		tok_func(P_tok_func),
		function_return_type(P_function_return_type)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionIdentity::FunctionIdentity(
	int		main_index,
	parsedExp&	P_tok_local_function,
	parsedExp&	P_tok_func,
	parsedExp&	P_function_return_type,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_local_function(P_tok_local_function),
		tok_func(P_tok_func),
		function_return_type(P_function_return_type)
	{
	mainIndex = main_index;
	if(tok_local_function) {
		initExpression(tok_local_function->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionIdentity::FunctionIdentity(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_tok_func,
	Differentiator_2)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		tok_func(P_tok_func)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionIdentity::FunctionIdentity(
	int		main_index,
	parsedExp&	P_tok_local_function,
	parsedExp&	P_tok_func,
	Differentiator_3)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_local_function(P_tok_local_function),
		tok_func(P_tok_func)
	{
	mainIndex = main_index;
	if(tok_local_function) {
		initExpression(tok_local_function->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionIdentity::FunctionIdentity(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_tok_model,
	parsedExp&	P_tok_func,
	Differentiator_4)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		tok_model(P_tok_model),
		tok_func(P_tok_func)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionIdentity::FunctionIdentity(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_tok_resource,
	parsedExp&	P_tok_usage,
	parsedExp&	P_tok_func,
	Differentiator_5)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		tok_resource(P_tok_resource),
		tok_usage(P_tok_usage),
		tok_func(P_tok_func)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionIdentity::FunctionIdentity(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_tok_decomposition,
	parsedExp&	P_tok_func,
	Differentiator_6)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		tok_decomposition(P_tok_decomposition),
		tok_func(P_tok_func)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionIdentity::FunctionIdentity(const FunctionIdentity& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.function_return_type) {
		function_return_type.reference(p.function_return_type->copy());
	}
	if(p.function_return_type_1) {
		function_return_type_1.reference(p.function_return_type_1->copy());
	}
	if(p.tok_decomposition) {
		tok_decomposition.reference(p.tok_decomposition->copy());
	}
	if(p.tok_func) {
		tok_func.reference(p.tok_func->copy());
	}
	if(p.tok_func_0) {
		tok_func_0.reference(p.tok_func_0->copy());
	}
	if(p.tok_func_1) {
		tok_func_1.reference(p.tok_func_1->copy());
	}
	if(p.tok_func_2) {
		tok_func_2.reference(p.tok_func_2->copy());
	}
	if(p.tok_id) {
		tok_id.reference(p.tok_id->copy());
	}
	if(p.tok_local_function) {
		tok_local_function.reference(p.tok_local_function->copy());
	}
	if(p.tok_model) {
		tok_model.reference(p.tok_model->copy());
	}
	if(p.tok_resource) {
		tok_resource.reference(p.tok_resource->copy());
	}
	if(p.tok_usage) {
		tok_usage.reference(p.tok_usage->copy());
	}
	initExpression(p.getData());
}
FunctionIdentity::FunctionIdentity(bool, const FunctionIdentity& p)
	: pE(origin_info(p.line, p.file)),
		function_return_type(p.function_return_type),
		function_return_type_1(p.function_return_type_1),
		tok_decomposition(p.tok_decomposition),
		tok_func(p.tok_func),
		tok_func_0(p.tok_func_0),
		tok_func_1(p.tok_func_1),
		tok_func_2(p.tok_func_2),
		tok_id(p.tok_id),
		tok_local_function(p.tok_local_function),
		tok_model(p.tok_model),
		tok_resource(p.tok_resource),
		tok_usage(p.tok_usage) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void FunctionIdentity::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(function_return_type) {
		function_return_type->recursively_apply(EA);
	}
	if(function_return_type_1) {
		function_return_type_1->recursively_apply(EA);
	}
	if(tok_decomposition) {
		tok_decomposition->recursively_apply(EA);
	}
	if(tok_func) {
		tok_func->recursively_apply(EA);
	}
	if(tok_func_0) {
		tok_func_0->recursively_apply(EA);
	}
	if(tok_func_1) {
		tok_func_1->recursively_apply(EA);
	}
	if(tok_func_2) {
		tok_func_2->recursively_apply(EA);
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	if(tok_local_function) {
		tok_local_function->recursively_apply(EA);
	}
	if(tok_model) {
		tok_model->recursively_apply(EA);
	}
	if(tok_resource) {
		tok_resource->recursively_apply(EA);
	}
	if(tok_usage) {
		tok_usage->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	FunctionIdentity::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	FunctionIdentity::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
FunctionReturnType::FunctionReturnType(
	int		main_index,
	parsedExp&	P_builtin_type,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		builtin_type(P_builtin_type)

	{
	mainIndex = main_index;
	if(builtin_type) {
		initExpression(builtin_type->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionReturnType::FunctionReturnType(
	int		main_index,
	parsedExp&	P_tok_void,
	Differentiator_1)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_void(P_tok_void)

	{
	mainIndex = main_index;
	if(tok_void) {
		initExpression(tok_void->getData());
	} else {
		initExpression("(null)");
	}
}
FunctionReturnType::FunctionReturnType(const FunctionReturnType& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.builtin_type) {
		builtin_type.reference(p.builtin_type->copy());
	}
	if(p.tok_void) {
		tok_void.reference(p.tok_void->copy());
	}
	initExpression(p.getData());
}
FunctionReturnType::FunctionReturnType(bool, const FunctionReturnType& p)
	: pE(origin_info(p.line, p.file)),
		builtin_type(p.builtin_type),
		tok_void(p.tok_void) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void FunctionReturnType::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(builtin_type) {
		builtin_type->recursively_apply(EA);
	}
	if(tok_void) {
		tok_void->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	FunctionReturnType::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	FunctionReturnType::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
GetInterpwins::GetInterpwins(
	int		main_index,
	parsedExp&	P_expression_list,
	parsedExp&	P_tok_interpwins,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_ASCII_41,	// )
	parsedExp&	P_tok_for,
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		expression_list(P_expression_list),
		tok_interpwins(P_tok_interpwins),
		ASCII_40(P_ASCII_40),
		ASCII_41(P_ASCII_41),
		tok_for(P_tok_for),
		Expression(P_Expression),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(expression_list) {
		initExpression(expression_list->getData());
	} else {
		initExpression("(null)");
	}
}
GetInterpwins::GetInterpwins(const GetInterpwins& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_40) {
		ASCII_40.reference(p.ASCII_40->copy());
	}
	if(p.ASCII_41) {
		ASCII_41.reference(p.ASCII_41->copy());
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.expression_list) {
		expression_list.reference(p.expression_list->copy());
	}
	if(p.tok_for) {
		tok_for.reference(p.tok_for->copy());
	}
	if(p.tok_interpwins) {
		tok_interpwins.reference(p.tok_interpwins->copy());
	}
	initExpression(p.getData());
}
GetInterpwins::GetInterpwins(bool, const GetInterpwins& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_40(p.ASCII_40),
		ASCII_41(p.ASCII_41),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		expression_list(p.expression_list),
		tok_for(p.tok_for),
		tok_interpwins(p.tok_interpwins) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void GetInterpwins::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_40) {
		ASCII_40->recursively_apply(EA);
	}
	if(ASCII_41) {
		ASCII_41->recursively_apply(EA);
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(expression_list) {
		expression_list->recursively_apply(EA);
	}
	if(tok_for) {
		tok_for->recursively_apply(EA);
	}
	if(tok_interpwins) {
		tok_interpwins->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	GetInterpwins::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	GetInterpwins::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
GetWindows::GetWindows(
	int		main_index,
	parsedExp&	P_expression_list,
	parsedExp&	P_tok_getwindows,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_ASCII_41,	// )
	parsedExp&	P_tok_for,
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		expression_list(P_expression_list),
		tok_getwindows(P_tok_getwindows),
		ASCII_40(P_ASCII_40),
		ASCII_41(P_ASCII_41),
		tok_for(P_tok_for),
		Expression(P_Expression),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(expression_list) {
		initExpression(expression_list->getData());
	} else {
		initExpression("(null)");
	}
}
 GetWindows::GetWindows(const GetWindows& p)
	: executableExp(origin_info(p.line, p.file)) {
	theData = p.getData();
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.actual_arguments.size() && p.actual_arguments[0]) {
		parsedExp pe(p.actual_arguments[0]->copy());
		actual_arguments.push_back(pe);
	}
	if(p.Options) {
		Array* a = dynamic_cast<Array*>(p.Options->copy());
		assert(a);
		Options.reference(a);
	}
	if(p.SchedulingCondition) {
		SchedulingCondition.reference(p.SchedulingCondition->copy());
	}
	if(p.ActualSymbol) {
		Symbol* s = dynamic_cast<Symbol*>(p.ActualSymbol->copy());
		assert(s);
		ActualSymbol.reference(s);
	}
 }
 GetWindows::GetWindows(bool, const GetWindows& p)
	: executableExp(origin_info(p.line, p.file)) {
	theData = p.getData();
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->shallow_copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.Expression) {
		Expression.reference(p.Expression->shallow_copy());
	}
	if(p.actual_arguments.size() && p.actual_arguments[0]) {
		parsedExp pe(p.actual_arguments[0]->shallow_copy());
		actual_arguments.push_back(pe);
	}
	if(p.Options) {
		Array* a = dynamic_cast<Array*>(p.Options->shallow_copy());
		assert(a);
		Options.reference(a);
	}
	if(p.SchedulingCondition) {
		SchedulingCondition.reference(p.SchedulingCondition->shallow_copy());
	}
	if(p.ActualSymbol) {
		Symbol* s = dynamic_cast<Symbol*>(p.ActualSymbol->shallow_copy());
		assert(s);
		ActualSymbol.reference(s);
	}
 }
void GetWindows::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_40) {
		ASCII_40->recursively_apply(EA);
	}
	if(ASCII_41) {
		ASCII_41->recursively_apply(EA);
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(expression_list) {
		expression_list->recursively_apply(EA);
	}
	if(tok_for) {
		tok_for->recursively_apply(EA);
	}
	if(tok_getwindows) {
		tok_getwindows->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	GetWindows::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	GetWindows::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Global::Global(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_tok_epoch,
	parsedExp&	P_tok_equal_sign,
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		tok_epoch(P_tok_epoch),
		tok_equal_sign(P_tok_equal_sign),
		Expression(P_Expression),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
Global::Global(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_tok_timesystem,
	parsedExp&	P_tok_equal_sign,
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		tok_timesystem(P_tok_timesystem),
		tok_equal_sign(P_tok_equal_sign),
		Expression(P_Expression),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
Global::Global(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_local_or_global,
	parsedExp&	P_any_data_type,
	parsedExp&	P_tok_equal_sign,
	parsedExp&	P_Expression,
	parsedExp&	P_opt_range,
	parsedExp&	P_opt_descr,
	parsedExp&	P_opt_units,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_2)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		local_or_global(P_local_or_global),
		any_data_type(P_any_data_type),
		tok_equal_sign(P_tok_equal_sign),
		Expression(P_Expression),
		opt_range(P_opt_range),
		opt_descr(P_opt_descr),
		opt_units(P_opt_units),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	data_type = any_data_type->getData();
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
Global::Global(const Global& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.ASCII_59_4) {
		ASCII_59_4.reference(p.ASCII_59_4->copy());
	}
	if(p.ASCII_59_8) {
		ASCII_59_8.reference(p.ASCII_59_8->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.Expression_3) {
		Expression_3.reference(p.Expression_3->copy());
	}
	if(p.Expression_4) {
		Expression_4.reference(p.Expression_4->copy());
	}
	if(p.any_data_type) {
		any_data_type.reference(p.any_data_type->copy());
	}
	if(p.local_or_global) {
		local_or_global.reference(p.local_or_global->copy());
	}
	if(p.opt_descr) {
		opt_descr.reference(p.opt_descr->copy());
	}
	if(p.opt_range) {
		opt_range.reference(p.opt_range->copy());
	}
	if(p.opt_units) {
		opt_units.reference(p.opt_units->copy());
	}
	if(p.tok_epoch) {
		tok_epoch.reference(p.tok_epoch->copy());
	}
	if(p.tok_equal_sign) {
		tok_equal_sign.reference(p.tok_equal_sign->copy());
	}
	if(p.tok_equal_sign_2) {
		tok_equal_sign_2.reference(p.tok_equal_sign_2->copy());
	}
	if(p.tok_equal_sign_3) {
		tok_equal_sign_3.reference(p.tok_equal_sign_3->copy());
	}
	if(p.tok_id) {
		tok_id.reference(p.tok_id->copy());
	}
	if(p.tok_timesystem) {
		tok_timesystem.reference(p.tok_timesystem->copy());
	}
	initExpression(p.getData());
}
Global::Global(bool, const Global& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		ASCII_59_4(p.ASCII_59_4),
		ASCII_59_8(p.ASCII_59_8),
		Expression(p.Expression),
		Expression_3(p.Expression_3),
		Expression_4(p.Expression_4),
		any_data_type(p.any_data_type),
		local_or_global(p.local_or_global),
		opt_descr(p.opt_descr),
		opt_range(p.opt_range),
		opt_units(p.opt_units),
		tok_epoch(p.tok_epoch),
		tok_equal_sign(p.tok_equal_sign),
		tok_equal_sign_2(p.tok_equal_sign_2),
		tok_equal_sign_3(p.tok_equal_sign_3),
		tok_id(p.tok_id),
		tok_timesystem(p.tok_timesystem) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Global::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(ASCII_59_4) {
		ASCII_59_4->recursively_apply(EA);
	}
	if(ASCII_59_8) {
		ASCII_59_8->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(Expression_3) {
		Expression_3->recursively_apply(EA);
	}
	if(Expression_4) {
		Expression_4->recursively_apply(EA);
	}
	if(any_data_type) {
		any_data_type->recursively_apply(EA);
	}
	if(local_or_global) {
		local_or_global->recursively_apply(EA);
	}
	if(opt_descr) {
		opt_descr->recursively_apply(EA);
	}
	if(opt_range) {
		opt_range->recursively_apply(EA);
	}
	if(opt_units) {
		opt_units->recursively_apply(EA);
	}
	if(tok_epoch) {
		tok_epoch->recursively_apply(EA);
	}
	if(tok_equal_sign) {
		tok_equal_sign->recursively_apply(EA);
	}
	if(tok_equal_sign_2) {
		tok_equal_sign_2->recursively_apply(EA);
	}
	if(tok_equal_sign_3) {
		tok_equal_sign_3->recursively_apply(EA);
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	if(tok_timesystem) {
		tok_timesystem->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Global::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	if(!aafReader::global_items().find(nodeData)) {
		aafReader::global_items() << new symNode(nodeData, data_type);
	}
	theData = "Global";
 }
void	Global::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Hierarchy::Hierarchy(
	int		main_index,
	parsedExp&	P_tok_hierarchy_keyword,
	parsedExp&	P_tok_into,
	parsedExp&	P_id_list,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_hierarchy_keyword(P_tok_hierarchy_keyword),
		tok_into(P_tok_into),
		id_list(P_id_list),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(tok_hierarchy_keyword) {
		initExpression(tok_hierarchy_keyword->getData());
	} else {
		initExpression("(null)");
	}
}
Hierarchy::Hierarchy(const Hierarchy& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.id_list) {
		id_list.reference(p.id_list->copy());
	}
	if(p.tok_hierarchy_keyword) {
		tok_hierarchy_keyword.reference(p.tok_hierarchy_keyword->copy());
	}
	if(p.tok_into) {
		tok_into.reference(p.tok_into->copy());
	}
	initExpression(p.getData());
}
Hierarchy::Hierarchy(bool, const Hierarchy& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		id_list(p.id_list),
		tok_hierarchy_keyword(p.tok_hierarchy_keyword),
		tok_into(p.tok_into) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Hierarchy::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(id_list) {
		id_list->recursively_apply(EA);
	}
	if(tok_hierarchy_keyword) {
		tok_hierarchy_keyword->recursively_apply(EA);
	}
	if(tok_into) {
		tok_into->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	Hierarchy::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	Hierarchy::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
IdList::IdList(
	int		main_index,
	parsedExp&	P_activity_id,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		activity_id(P_activity_id)

	{
	mainIndex = main_index;
	if(activity_id) {
		initExpression(activity_id->getData());
	} else {
		initExpression("(null)");
	}
}
IdList::IdList(const IdList& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.activity_id) {
		activity_id.reference(p.activity_id->copy());
	}
	initExpression(p.getData());
}
IdList::IdList(bool, const IdList& p)
	: pE(origin_info(p.line, p.file)),
		activity_id(p.activity_id) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void IdList::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(activity_id) {
		activity_id->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	IdList::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	IdList::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
If::If(
	int		main_index,
	parsedExp&	P_tok_if,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_41,	// )
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_if(P_tok_if),
		ASCII_40(P_ASCII_40),
		Expression(P_Expression),
		ASCII_41(P_ASCII_41)
	{
	mainIndex = main_index;
	if(tok_if) {
		initExpression(tok_if->getData());
	} else {
		initExpression("(null)");
	}
}
If::If(
	int		main_index,
	parsedExp&	P_tok_else,
	parsedExp&	P_tok_if,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_41,	// )
	Differentiator_1)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_else(P_tok_else),
		tok_if(P_tok_if),
		ASCII_40(P_ASCII_40),
		Expression(P_Expression),
		ASCII_41(P_ASCII_41)
	{
	mainIndex = main_index;
	if(tok_else) {
		initExpression(tok_else->getData());
	} else {
		initExpression("(null)");
	}
}
If::If(
	int		main_index,
	parsedExp&	P_tok_else,
	Differentiator_2)

	: executableExp(origin_info(yylineno, aafReader::current_file())),
		tok_else(P_tok_else)

	{
	mainIndex = main_index;
	if(tok_else) {
		initExpression(tok_else->getData());
	} else {
		initExpression("(null)");
	}
}
If::If(const If& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_40) {
		ASCII_40.reference(p.ASCII_40->copy());
	}
	if(p.ASCII_40_2) {
		ASCII_40_2.reference(p.ASCII_40_2->copy());
	}
	if(p.ASCII_41) {
		ASCII_41.reference(p.ASCII_41->copy());
	}
	if(p.ASCII_41_4) {
		ASCII_41_4.reference(p.ASCII_41_4->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.Expression_3) {
		Expression_3.reference(p.Expression_3->copy());
	}
	if(p.tok_else) {
		tok_else.reference(p.tok_else->copy());
	}
	if(p.tok_if) {
		tok_if.reference(p.tok_if->copy());
	}
	if(p.tok_if_1) {
		tok_if_1.reference(p.tok_if_1->copy());
	}
	initExpression(p.getData());
}
If::If(bool, const If& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_40(p.ASCII_40),
		ASCII_40_2(p.ASCII_40_2),
		ASCII_41(p.ASCII_41),
		ASCII_41_4(p.ASCII_41_4),
		Expression(p.Expression),
		Expression_3(p.Expression_3),
		tok_else(p.tok_else),
		tok_if(p.tok_if),
		tok_if_1(p.tok_if_1) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void If::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_40) {
		ASCII_40->recursively_apply(EA);
	}
	if(ASCII_40_2) {
		ASCII_40_2->recursively_apply(EA);
	}
	if(ASCII_41) {
		ASCII_41->recursively_apply(EA);
	}
	if(ASCII_41_4) {
		ASCII_41_4->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(Expression_3) {
		Expression_3->recursively_apply(EA);
	}
	if(tok_else) {
		tok_else->recursively_apply(EA);
	}
	if(tok_if) {
		tok_if->recursively_apply(EA);
	}
	if(tok_if_1) {
		tok_if_1->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	If::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	if(tok_if) {
		if(tok_else) {
			execute_method = &pEsys::If::execute_elseif;
		} else {
			execute_method = &pEsys::If::execute_if;
		}
	} else if(tok_else) {
		execute_method = &pEsys::If::execute_else;
	} else {
		assert(false);
	}
 }
void	If::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
InputFile::InputFile(
	int		main_index,
	parsedExp&	P_file_body,
	parsedExp&	P_tok_apgen,
	parsedExp&	P_tok_version,
	parsedExp&	P_Expression,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		file_body(P_file_body),
		tok_apgen(P_tok_apgen),
		tok_version(P_tok_version),
		Expression(P_Expression)
	{
	mainIndex = main_index;
	if(file_body) {
		initExpression(file_body->getData());
	} else {
		initExpression("(null)");
	}
}
InputFile::InputFile(const InputFile& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.file_body) {
		file_body.reference(p.file_body->copy());
	}
	if(p.tok_apgen) {
		tok_apgen.reference(p.tok_apgen->copy());
	}
	if(p.tok_version) {
		tok_version.reference(p.tok_version->copy());
	}
	initExpression(p.getData());
}
InputFile::InputFile(bool, const InputFile& p)
	: pE(origin_info(p.line, p.file)),
		Expression(p.Expression),
		file_body(p.file_body),
		tok_apgen(p.tok_apgen),
		tok_version(p.tok_version) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void InputFile::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(file_body) {
		file_body->recursively_apply(EA);
	}
	if(tok_apgen) {
		tok_apgen->recursively_apply(EA);
	}
	if(tok_version) {
		tok_version->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	InputFile::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	InputFileName = aafReader::current_file();
 }
void	InputFile::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
LabeledDefault::LabeledDefault(
	int		main_index,
	parsedExp&	P_when_case_label,
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		when_case_label(P_when_case_label),
		Expression(P_Expression),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(when_case_label) {
		initExpression(when_case_label->getData());
	} else {
		initExpression("(null)");
	}
}
LabeledDefault::LabeledDefault(const LabeledDefault& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.when_case_label) {
		when_case_label.reference(p.when_case_label->copy());
	}
	initExpression(p.getData());
}
LabeledDefault::LabeledDefault(bool, const LabeledDefault& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		when_case_label(p.when_case_label) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void LabeledDefault::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(when_case_label) {
		when_case_label->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	LabeledDefault::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	LabeledDefault::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
LabeledDefaultList::LabeledDefaultList(
	int		main_index,
	parsedExp&	P_labeled_default_list,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		labeled_default_list(P_labeled_default_list)

	{
	mainIndex = main_index;
	if(labeled_default_list) {
		initExpression(labeled_default_list->getData());
	} else {
		initExpression("(null)");
	}
}
LabeledDefaultList::LabeledDefaultList(const LabeledDefaultList& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.labeled_default_list) {
		labeled_default_list.reference(p.labeled_default_list->copy());
	}
	initExpression(p.getData());
}
LabeledDefaultList::LabeledDefaultList(bool, const LabeledDefaultList& p)
	: pE(origin_info(p.line, p.file)),
		labeled_default_list(p.labeled_default_list) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void LabeledDefaultList::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(labeled_default_list) {
		labeled_default_list->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	LabeledDefaultList::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	LabeledDefaultList::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
LabeledProfiles::LabeledProfiles(
	int		main_index,
	parsedExp&	P_when_case_label,
	parsedExp&	P_list_of_profile_expressions,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		when_case_label(P_when_case_label),
		list_of_profile_expressions(P_list_of_profile_expressions),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(when_case_label) {
		initExpression(when_case_label->getData());
	} else {
		initExpression("(null)");
	}
}
LabeledProfiles::LabeledProfiles(const LabeledProfiles& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.list_of_profile_expressions) {
		list_of_profile_expressions.reference(p.list_of_profile_expressions->copy());
	}
	if(p.when_case_label) {
		when_case_label.reference(p.when_case_label->copy());
	}
	initExpression(p.getData());
}
LabeledProfiles::LabeledProfiles(bool, const LabeledProfiles& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		list_of_profile_expressions(p.list_of_profile_expressions),
		when_case_label(p.when_case_label) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void LabeledProfiles::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(list_of_profile_expressions) {
		list_of_profile_expressions->recursively_apply(EA);
	}
	if(when_case_label) {
		when_case_label->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	LabeledProfiles::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	LabeledProfiles::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
LabeledProgram::LabeledProgram(
	int		main_index,
	parsedExp&	P_when_case_label,
	parsedExp&	P_declarative_program,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		when_case_label(P_when_case_label),
		declarative_program(P_declarative_program)
	{
	mainIndex = main_index;
	if(when_case_label) {
		initExpression(when_case_label->getData());
	} else {
		initExpression("(null)");
	}
}
LabeledProgram::LabeledProgram(const LabeledProgram& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.declarative_program) {
		declarative_program.reference(p.declarative_program->copy());
	}
	if(p.when_case_label) {
		when_case_label.reference(p.when_case_label->copy());
	}
	initExpression(p.getData());
}
LabeledProgram::LabeledProgram(bool, const LabeledProgram& p)
	: pE(origin_info(p.line, p.file)),
		declarative_program(p.declarative_program),
		when_case_label(p.when_case_label) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void LabeledProgram::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(declarative_program) {
		declarative_program->recursively_apply(EA);
	}
	if(when_case_label) {
		when_case_label->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	LabeledProgram::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	LabeledProgram::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
LabeledStates::LabeledStates(
	int		main_index,
	parsedExp&	P_when_case_label,
	parsedExp&	P_expression_list,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		when_case_label(P_when_case_label),
		expression_list(P_expression_list),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(when_case_label) {
		initExpression(when_case_label->getData());
	} else {
		initExpression("(null)");
	}
}
LabeledStates::LabeledStates(const LabeledStates& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.expression_list) {
		expression_list.reference(p.expression_list->copy());
	}
	if(p.when_case_label) {
		when_case_label.reference(p.when_case_label->copy());
	}
	initExpression(p.getData());
}
LabeledStates::LabeledStates(bool, const LabeledStates& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		expression_list(p.expression_list),
		when_case_label(p.when_case_label) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void LabeledStates::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(expression_list) {
		expression_list->recursively_apply(EA);
	}
	if(when_case_label) {
		when_case_label->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	LabeledStates::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	LabeledStates::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Logical::Logical(
	int		main_index,
	parsedExp&	P_tok_orop,
	parsedExp&	P_Expression,
	parsedExp&	P_maybe_ORed,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_orop(P_tok_orop),
		Expression(P_Expression),
		maybe_ORed(P_maybe_ORed)
	{
	mainIndex = main_index;
	if(tok_orop) {
		initExpression(tok_orop->getData());
	} else {
		initExpression("(null)");
	}
}
Logical::Logical(
	int		main_index,
	parsedExp&	P_tok_andop,
	parsedExp&	P_maybe_ORed,
	parsedExp&	P_maybe_ANDed,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_andop(P_tok_andop),
		maybe_ORed(P_maybe_ORed),
		maybe_ANDed(P_maybe_ANDed)
	{
	mainIndex = main_index;
	if(tok_andop) {
		initExpression(tok_andop->getData());
	} else {
		initExpression("(null)");
	}
}
 Logical::Logical(const Logical& p)
	: pE(origin_info(p.line, p.file)) {
	theData = p.getData();
	binaryFunc = p.binaryFunc;
	tlist<alpha_void, Cnode0<alpha_void, pE*> > newobjs(true);
	pE* obj;

	for(int i = 0; i < p.expressions.size(); i++) {
		if((obj = p.expressions[i].object())) {
			expressions.push_back(parsedExp(obj->smart_copy(newobjs)));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.Expression) {
		Expression.reference(p.Expression->smart_copy(newobjs));
	}
	if(p.maybe_ANDed) {
		maybe_ANDed.reference(p.maybe_ANDed->smart_copy(newobjs));
	}
	if(p.maybe_ORed) {
		maybe_ORed.reference(p.maybe_ORed->smart_copy(newobjs));
	}
	if(p.maybe_ORed_0) {
		maybe_ORed_0.reference(p.maybe_ORed_0->smart_copy(newobjs));
	}
	if(p.tok_andop) {
		tok_andop.reference(p.tok_andop->smart_copy(newobjs));
	}
	if(p.tok_orop) {
		tok_orop.reference(p.tok_orop->smart_copy(newobjs));
	}
	if(p.Lhs) {
		Lhs.reference(p.Lhs->smart_copy(newobjs));
	}
	if(p.Rhs) {
		Rhs.reference(p.Rhs->smart_copy(newobjs));
	}
	if(p.Operator) {
		Operator.reference(p.Operator->smart_copy(newobjs));
	}
 }
 Logical::Logical(bool, const Logical& p)
	: pE(origin_info(p.line, p.file)) {
	theData = p.getData();
	binaryFunc = p.binaryFunc;
	expressions = p.expressions;
	Expression = p.Expression;
	maybe_ANDed = p.maybe_ANDed;
	maybe_ORed = p.maybe_ORed;
	maybe_ORed_0 = p.maybe_ORed_0;
	tok_andop = p.tok_andop;
	tok_orop = p.tok_orop;
	Lhs = p.Lhs;
	Rhs = p.Rhs;
	Operator = p.Operator;
 }
 void	Logical::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	if(Lhs) {
		Lhs->recursively_apply(EA);
	}
	if(Rhs) {
		Rhs->recursively_apply(EA);
	}
	EA.post_analyze(this);
 }
 void	Logical::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
	if(tok_orop) {
		Operator = tok_orop;
		Lhs = Expression;
		Rhs = maybe_ORed;
		binaryFunc = func_LOR;
	} else if(tok_andop) {
		Operator = tok_andop;
		Lhs = maybe_ORed;
		Rhs = maybe_ANDed;
		binaryFunc = func_LAND;
	}
 }
 void	Logical::eval_expression(
		behaving_base*	local_object,
		TypedValue&	result
		) {
	try {
	    binaryFunc(
		Lhs,
		Rhs,
		local_object,
		result);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ":\n" << Err.msg;
		throw(eval_error(err));
	}

	if(APcloptions::theCmdLineOptions().debug_execute) {
		TypedValue val1, val2;
		Lhs->eval_expression(local_object, val1);
		Rhs->eval_expression(local_object, val2);
		Cstring tmp = val1.to_string();
		tmp << " " << theData << " " << val2.to_string()
			<< " = " << result.to_string();
		cerr << "FuncBinary::eval - " << tmp << "\n";
	}
 }
ModelingSection::ModelingSection(
	int		main_index,
	parsedExp&	P_resource_usage_header,
	parsedExp&	P_program,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		resource_usage_header(P_resource_usage_header),
		program(P_program)
	{
	mainIndex = main_index;
	if(resource_usage_header) {
		initExpression(resource_usage_header->getData());
	} else {
		initExpression("(null)");
	}
}
ModelingSection::ModelingSection(const ModelingSection& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.program) {
		program.reference(p.program->copy());
	}
	if(p.resource_usage_header) {
		resource_usage_header.reference(p.resource_usage_header->copy());
	}
	initExpression(p.getData());
}
ModelingSection::ModelingSection(bool, const ModelingSection& p)
	: pE(origin_info(p.line, p.file)),
		program(p.program),
		resource_usage_header(p.resource_usage_header) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ModelingSection::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(program) {
		program->recursively_apply(EA);
	}
	if(resource_usage_header) {
		resource_usage_header->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ModelingSection::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ModelingSection::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
MultidimIndices::MultidimIndices(
	int		main_index,
	parsedExp&	P_single_index,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		single_index(P_single_index)

	{
	mainIndex = main_index;
	if(single_index) {
		initExpression(single_index->getData());
	} else {
		initExpression("(null)");
	}
}
 MultidimIndices::MultidimIndices(const MultidimIndices& md)
		: pE(origin_info(md.line, md.file)) {
	// nodeType = md.nodeType;
	theData = md.theData;
	for(int i = 0; i < md.expressions.size(); i++) {
		expressions.push_back(
			parsedExp(md.expressions[i]->copy()));
	}
	for(int i = 0; i < md.actual_indices.size(); i++) {
		actual_indices.push_back(
			parsedExp(md.actual_indices[i]->copy()));
	}
 }
 MultidimIndices::MultidimIndices(bool, const MultidimIndices& md)
		: pE(origin_info(md.line, md.file)) {
	// nodeType = md.nodeType;
	theData = md.theData;
	expressions = md.expressions;
	actual_indices = md.actual_indices;
 }
 // special recursive method
 void MultidimIndices::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		expressions[i]->recursively_apply(EA);
	}
	for(int i = 0; i < actual_indices.size(); i++) {
		actual_indices[i]->recursively_apply(EA);
	}
	EA.post_analyze(this);
 }
 void	MultidimIndices::initExpression(
			const Cstring& nodeData) {
	theData = "MultidimIndices";
	expressions.push_back(single_index);
	for(int i = 0; i < single_index->expressions.size(); i++) {
		expressions.push_back(single_index->expressions[i]);
	}
	single_index->expressions.clear();
	single_index.dereference();
 }
void	MultidimIndices::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
MultiplicativeExp::MultiplicativeExp(
	int		main_index,
	parsedExp&	P_tok_mult,
	parsedExp&	P_maybe_a_product,
	parsedExp&	P_maybe_a_factor,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_mult(P_tok_mult),
		maybe_a_product(P_maybe_a_product),
		maybe_a_factor(P_maybe_a_factor)
	{
	mainIndex = main_index;
	if(tok_mult) {
		initExpression(tok_mult->getData());
	} else {
		initExpression("(null)");
	}
}
MultiplicativeExp::MultiplicativeExp(
	int		main_index,
	parsedExp&	P_tok_div,
	parsedExp&	P_maybe_a_product,
	parsedExp&	P_maybe_a_factor,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_div(P_tok_div),
		maybe_a_product(P_maybe_a_product),
		maybe_a_factor(P_maybe_a_factor)
	{
	mainIndex = main_index;
	if(tok_div) {
		initExpression(tok_div->getData());
	} else {
		initExpression("(null)");
	}
}
MultiplicativeExp::MultiplicativeExp(
	int		main_index,
	parsedExp&	P_tok_mod,
	parsedExp&	P_maybe_a_product,
	parsedExp&	P_maybe_a_factor,
	Differentiator_2)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_mod(P_tok_mod),
		maybe_a_product(P_maybe_a_product),
		maybe_a_factor(P_maybe_a_factor)
	{
	mainIndex = main_index;
	if(tok_mod) {
		initExpression(tok_mod->getData());
	} else {
		initExpression("(null)");
	}
}
MultiplicativeExp::MultiplicativeExp(const MultiplicativeExp& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.maybe_a_factor) {
		maybe_a_factor.reference(p.maybe_a_factor->copy());
	}
	if(p.maybe_a_factor_2) {
		maybe_a_factor_2.reference(p.maybe_a_factor_2->copy());
	}
	if(p.maybe_a_product) {
		maybe_a_product.reference(p.maybe_a_product->copy());
	}
	if(p.maybe_a_product_0) {
		maybe_a_product_0.reference(p.maybe_a_product_0->copy());
	}
	if(p.tok_div) {
		tok_div.reference(p.tok_div->copy());
	}
	if(p.tok_mod) {
		tok_mod.reference(p.tok_mod->copy());
	}
	if(p.tok_mult) {
		tok_mult.reference(p.tok_mult->copy());
	}
	initExpression(p.getData());
}
MultiplicativeExp::MultiplicativeExp(bool, const MultiplicativeExp& p)
	: pE(origin_info(p.line, p.file)),
		maybe_a_factor(p.maybe_a_factor),
		maybe_a_factor_2(p.maybe_a_factor_2),
		maybe_a_product(p.maybe_a_product),
		maybe_a_product_0(p.maybe_a_product_0),
		tok_div(p.tok_div),
		tok_mod(p.tok_mod),
		tok_mult(p.tok_mult) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void MultiplicativeExp::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(maybe_a_factor) {
		maybe_a_factor->recursively_apply(EA);
	}
	if(maybe_a_factor_2) {
		maybe_a_factor_2->recursively_apply(EA);
	}
	if(maybe_a_product) {
		maybe_a_product->recursively_apply(EA);
	}
	if(maybe_a_product_0) {
		maybe_a_product_0->recursively_apply(EA);
	}
	if(tok_div) {
		tok_div->recursively_apply(EA);
	}
	if(tok_mod) {
		tok_mod->recursively_apply(EA);
	}
	if(tok_mult) {
		tok_mult->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	MultiplicativeExp::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	if(tok_div) {
		Operator = tok_div;
		binaryFunc = func_divide;
	} else if(tok_mult) {
		Operator = tok_mult;
		binaryFunc = func_MULT;
	} else if(tok_mod) {
		Operator = tok_mod;
		binaryFunc = func_MOD;
	}
 }
 void	MultiplicativeExp::eval_expression(
		behaving_base*	local_object,
		TypedValue&	result
		) {
	TypedValue val1, val2;
	try {
	    maybe_a_product->eval_expression(local_object, val1);
	    maybe_a_factor->eval_expression(local_object, val2);
	    binaryFunc(
		val1,
		val2,
		result);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ":\n" << Err.msg;
		throw(eval_error(err));
	}

	if(APcloptions::theCmdLineOptions().debug_execute) {
	    Cstring tmp = val1.to_string();
	    tmp << " " << theData << " " << val2.to_string()
			<< " = " << result.to_string();
	    cerr << "MultiplicativeExp::eval - " << tmp << "\n";
	}
 }
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_1)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_2)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_3)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_4)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_5)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_6)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_7)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_8)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_9)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_10)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_11)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_12)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_13)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_14)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_15)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_16)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_17)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_18)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_19)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_20)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_21)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_22)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_23)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_24)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_25)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(
	int		main_index,
	parsedExp&	P_null,
	Differentiator_26)

	: pE(origin_info(yylineno, aafReader::current_file())),
		null(P_null)

	{
	mainIndex = main_index;
	if(null) {
		initExpression(null->getData());
	} else {
		initExpression("(null)");
	}
}
Null::Null(const Null& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.null) {
		null.reference(p.null->copy());
	}
	initExpression(p.getData());
}
Null::Null(bool, const Null& p)
	: pE(origin_info(p.line, p.file)),
		null(p.null) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Null::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(null) {
		null->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	Null::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	Null::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
OneNumber::OneNumber(
	int		main_index,
	parsedExp&	P_tok_number,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_number(P_tok_number)

	{
	mainIndex = main_index;
	if(tok_number) {
		initExpression(tok_number->getData());
	} else {
		initExpression("(null)");
	}
}
OneNumber::OneNumber(
	int		main_index,
	parsedExp&	P_tok_number,
	parsedExp&	P_tok_minus,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_number(P_tok_number),
		tok_minus(P_tok_minus)
	{
	mainIndex = main_index;
	if(tok_number) {
		initExpression(tok_number->getData());
	} else {
		initExpression("(null)");
	}
}
OneNumber::OneNumber(const OneNumber& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.tok_minus) {
		tok_minus.reference(p.tok_minus->copy());
	}
	if(p.tok_number) {
		tok_number.reference(p.tok_number->copy());
	}
	initExpression(p.getData());
}
OneNumber::OneNumber(bool, const OneNumber& p)
	: pE(origin_info(p.line, p.file)),
		tok_minus(p.tok_minus),
		tok_number(p.tok_number) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void OneNumber::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(tok_minus) {
		tok_minus->recursively_apply(EA);
	}
	if(tok_number) {
		tok_number->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	OneNumber::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	OneNumber::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Parameters::Parameters(
	int		main_index,
	parsedExp&	P_param_declarations,
	parsedExp&	P_tok_parameters,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		param_declarations(P_param_declarations),
		tok_parameters(P_tok_parameters)
	{
	mainIndex = main_index;
	if(param_declarations) {
		initExpression(param_declarations->getData());
	} else {
		initExpression("(null)");
	}
}
Parameters::Parameters(
	int		main_index,
	parsedExp&	P_param_declarations,
	parsedExp&	P_tok_parameters,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		param_declarations(P_param_declarations),
		tok_parameters(P_tok_parameters)
	{
	mainIndex = main_index;
	if(param_declarations) {
		initExpression(param_declarations->getData());
	} else {
		initExpression("(null)");
	}
}
Parameters::Parameters(const Parameters& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.param_declarations) {
		param_declarations.reference(p.param_declarations->copy());
	}
	if(p.tok_parameters) {
		tok_parameters.reference(p.tok_parameters->copy());
	}
	if(p.tok_parameters_0) {
		tok_parameters_0.reference(p.tok_parameters_0->copy());
	}
	initExpression(p.getData());
}
Parameters::Parameters(bool, const Parameters& p)
	: pE(origin_info(p.line, p.file)),
		param_declarations(p.param_declarations),
		tok_parameters(p.tok_parameters),
		tok_parameters_0(p.tok_parameters_0) {
	expressions = p.expressions;
	initExpression(p.getData());
}
 void Parameters::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	decomp_finder* DF = dynamic_cast<decomp_finder*>(&EA);
	if(!DF) {
	    if(param_declarations) {
		param_declarations->recursively_apply(EA);
	    }
	    if(tok_parameters) {
		tok_parameters->recursively_apply(EA);
	    }
	    if(tok_parameters_0) {
		tok_parameters_0->recursively_apply(EA);
	    }
	}
	EA.post_analyze(this);
 }
 void	Parameters::initExpression(
		const Cstring& nodeData) {
	theData = "Parameters";
 }
void	Parameters::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Parentheses::Parentheses(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_ASCII_41,	// )
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		ASCII_40(P_ASCII_40),
		ASCII_41(P_ASCII_41)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
Parentheses::Parentheses(const Parentheses& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_40) {
		ASCII_40.reference(p.ASCII_40->copy());
	}
	if(p.ASCII_41) {
		ASCII_41.reference(p.ASCII_41->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	initExpression(p.getData());
}
Parentheses::Parentheses(bool, const Parentheses& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_40(p.ASCII_40),
		ASCII_41(p.ASCII_41),
		Expression(p.Expression) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Parentheses::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_40) {
		ASCII_40->recursively_apply(EA);
	}
	if(ASCII_41) {
		ASCII_41->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	initExpression(
		const Cstring& nodeData);
 void	Parentheses::eval_expression(
			behaving_base*	loc,
			TypedValue&	result)
				{
	Expression->eval_expression(loc, result);
 }
PassiveCons::PassiveCons(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_tok_constraint,
	parsedExp&	P_tok_id_1,
	parsedExp&	P_ASCII_58,	// :
	parsedExp&	P_tok_begin,
	parsedExp&	P_opt_constraint_class_variables,
	parsedExp&	P_constraint_body,
	parsedExp&	P_constraint_message,
	parsedExp&	P_constraint_severity,
	parsedExp&	P_tok_end,
	parsedExp&	P_tok_constraint_10,
	parsedExp&	P_tok_id_11,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		tok_constraint(P_tok_constraint),
		tok_id_1(P_tok_id_1),
		ASCII_58(P_ASCII_58),
		tok_begin(P_tok_begin),
		opt_constraint_class_variables(P_opt_constraint_class_variables),
		constraint_body(P_constraint_body),
		constraint_message(P_constraint_message),
		constraint_severity(P_constraint_severity),
		tok_end(P_tok_end),
		tok_constraint_10(P_tok_constraint_10),
		tok_id_11(P_tok_id_11)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
PassiveCons::PassiveCons(const PassiveCons& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_58) {
		ASCII_58.reference(p.ASCII_58->copy());
	}
	if(p.constraint_body) {
		constraint_body.reference(p.constraint_body->copy());
	}
	if(p.constraint_message) {
		constraint_message.reference(p.constraint_message->copy());
	}
	if(p.constraint_severity) {
		constraint_severity.reference(p.constraint_severity->copy());
	}
	if(p.opt_constraint_class_variables) {
		opt_constraint_class_variables.reference(p.opt_constraint_class_variables->copy());
	}
	if(p.tok_begin) {
		tok_begin.reference(p.tok_begin->copy());
	}
	if(p.tok_constraint) {
		tok_constraint.reference(p.tok_constraint->copy());
	}
	if(p.tok_constraint_10) {
		tok_constraint_10.reference(p.tok_constraint_10->copy());
	}
	if(p.tok_end) {
		tok_end.reference(p.tok_end->copy());
	}
	if(p.tok_id) {
		tok_id.reference(p.tok_id->copy());
	}
	if(p.tok_id_1) {
		tok_id_1.reference(p.tok_id_1->copy());
	}
	if(p.tok_id_11) {
		tok_id_11.reference(p.tok_id_11->copy());
	}
	initExpression(p.getData());
}
PassiveCons::PassiveCons(bool, const PassiveCons& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_58(p.ASCII_58),
		constraint_body(p.constraint_body),
		constraint_message(p.constraint_message),
		constraint_severity(p.constraint_severity),
		opt_constraint_class_variables(p.opt_constraint_class_variables),
		tok_begin(p.tok_begin),
		tok_constraint(p.tok_constraint),
		tok_constraint_10(p.tok_constraint_10),
		tok_end(p.tok_end),
		tok_id(p.tok_id),
		tok_id_1(p.tok_id_1),
		tok_id_11(p.tok_id_11) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void PassiveCons::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_58) {
		ASCII_58->recursively_apply(EA);
	}
	if(constraint_body) {
		constraint_body->recursively_apply(EA);
	}
	if(constraint_message) {
		constraint_message->recursively_apply(EA);
	}
	if(constraint_severity) {
		constraint_severity->recursively_apply(EA);
	}
	if(opt_constraint_class_variables) {
		opt_constraint_class_variables->recursively_apply(EA);
	}
	if(tok_begin) {
		tok_begin->recursively_apply(EA);
	}
	if(tok_constraint) {
		tok_constraint->recursively_apply(EA);
	}
	if(tok_constraint_10) {
		tok_constraint_10->recursively_apply(EA);
	}
	if(tok_end) {
		tok_end->recursively_apply(EA);
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	if(tok_id_1) {
		tok_id_1->recursively_apply(EA);
	}
	if(tok_id_11) {
		tok_id_11->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	PassiveCons::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	PassiveCons::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
PassiveConsItem::PassiveConsItem(
	int		main_index,
	parsedExp&	P_constraint_item,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		constraint_item(P_constraint_item)

	{
	mainIndex = main_index;
	if(constraint_item) {
		initExpression(constraint_item->getData());
	} else {
		initExpression("(null)");
	}
}
PassiveConsItem::PassiveConsItem(const PassiveConsItem& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.constraint_item) {
		constraint_item.reference(p.constraint_item->copy());
	}
	initExpression(p.getData());
}
PassiveConsItem::PassiveConsItem(bool, const PassiveConsItem& p)
	: pE(origin_info(p.line, p.file)),
		constraint_item(p.constraint_item) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void PassiveConsItem::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(constraint_item) {
		constraint_item->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	PassiveConsItem::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	PassiveConsItem::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
PassiveConsMessage::PassiveConsMessage(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_message,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_message(P_tok_message),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
PassiveConsMessage::PassiveConsMessage(const PassiveConsMessage& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.tok_message) {
		tok_message.reference(p.tok_message->copy());
	}
	initExpression(p.getData());
}
PassiveConsMessage::PassiveConsMessage(bool, const PassiveConsMessage& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		tok_message(p.tok_message) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void PassiveConsMessage::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(tok_message) {
		tok_message->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	PassiveConsMessage::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	PassiveConsMessage::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
PassiveConsSeverity::PassiveConsSeverity(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_severity,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_severity(P_tok_severity),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
PassiveConsSeverity::PassiveConsSeverity(const PassiveConsSeverity& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.tok_severity) {
		tok_severity.reference(p.tok_severity->copy());
	}
	initExpression(p.getData());
}
PassiveConsSeverity::PassiveConsSeverity(bool, const PassiveConsSeverity& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		tok_severity(p.tok_severity) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void PassiveConsSeverity::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(tok_severity) {
		tok_severity->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	PassiveConsSeverity::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	PassiveConsSeverity::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ProfileList::ProfileList(
	int		main_index,
	parsedExp&	P_Expression,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression)

	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
ProfileList::ProfileList(const ProfileList& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	initExpression(p.getData());
}
ProfileList::ProfileList(bool, const ProfileList& p)
	: pE(origin_info(p.line, p.file)),
		Expression(p.Expression) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ProfileList::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ProfileList::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ProfileList::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Program::Program(
	int		main_index,
	parsedExp&	P_one_declarative_assignment,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		one_declarative_assignment(P_one_declarative_assignment)

	{
	mainIndex = main_index;
	if(one_declarative_assignment) {
		initExpression(one_declarative_assignment->getData());
	} else {
		initExpression("(null)");
	}
}
Program::Program(
	int		main_index,
	parsedExp&	P_statement,
	Differentiator_1)

	: pE(origin_info(yylineno, aafReader::current_file())),
		statement(P_statement)

	{
	mainIndex = main_index;
	if(statement) {
		initExpression(statement->getData());
	} else {
		initExpression("(null)");
	}
}
Program::Program(
	int		main_index,
	parsedExp&	P_class_var_declaration,
	Differentiator_2)

	: pE(origin_info(yylineno, aafReader::current_file())),
		class_var_declaration(P_class_var_declaration)

	{
	mainIndex = main_index;
	if(class_var_declaration) {
		initExpression(class_var_declaration->getData());
	} else {
		initExpression("(null)");
	}
}
 // special copy constructor
 Program::Program(const Program& P)
		: pE(origin_info(P.line, P.file)) {
	theData = "Program";
	ReturnType = P.ReturnType;
	orig_section = P.orig_section;
	for(int i = 0; i < P.statements.size(); i++) {
		executableExp* ee = dynamic_cast<executableExp*>(P.statements[i]->copy());
		statements.push_back(smart_ptr<executableExp>(ee));
	}
 }
Program::Program(bool, const Program& p)
	: pE(origin_info(p.line, p.file)),
		class_var_declaration(p.class_var_declaration),
		one_declarative_assignment(p.one_declarative_assignment),
		statement(p.statement) {
	expressions = p.expressions;
	initExpression(p.getData());
}
 // special recursive method
 void Program::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < statements.size(); i++) {
		statements[i]->recursively_apply(EA);
	}
	EA.post_analyze(this);
 }
 void	Program::initExpression(
		const Cstring& nodeData) {
	theData = "Program";
	executableExp* ee = NULL;
	if(class_var_declaration) {
		ee = dynamic_cast<executableExp*>(class_var_declaration.object());
	} else if(one_declarative_assignment) {
		ee = dynamic_cast<executableExp*>(one_declarative_assignment.object());
	} else if(statement) {
		ee = dynamic_cast<executableExp*>(statement.object());
	}
	statements.push_back(smart_ptr<executableExp>(ee));
	orig_section = apgen::METHOD_TYPE::NONE;
	ReturnType = apgen::DATA_TYPE::UNINITIALIZED;
 }
void	Program::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Qualifications::Qualifications(
	int		main_index,
	parsedExp&	P_qualification,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		qualification(P_qualification)

	{
	mainIndex = main_index;
	if(qualification) {
		initExpression(qualification->getData());
	} else {
		initExpression("(null)");
	}
}
Qualifications::Qualifications(
	int		main_index,
	parsedExp&	P_member,
	Differentiator_1)

	: pE(origin_info(yylineno, aafReader::current_file())),
		member(P_member)

	{
	mainIndex = main_index;
	if(member) {
		initExpression(member->getData());
	} else {
		initExpression("(null)");
	}
}
 Qualifications::Qualifications(const Qualifications& Q)
		: pE(origin_info(Q.line, Q.file)) {
	theData = Q.getData();
	for(int i = 0; i < Q.IndicesOrMembers.size(); i++) {
		IndicesOrMembers.push_back(parsedExp(Q.IndicesOrMembers[i]->copy()));
	}
 }
 Qualifications::Qualifications(bool, const Qualifications& Q)
		: pE(origin_info(Q.line, Q.file)) {
	theData = Q.getData();
	IndicesOrMembers = Q.IndicesOrMembers;
 }
void Qualifications::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(member) {
		member->recursively_apply(EA);
	}
	if(qualification) {
		qualification->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Qualifications::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	if(qualification) {
		IndicesOrMembers.push_back(parsedExp(qualification.object()));
		qualification.dereference();
	} else if(member) {
		IndicesOrMembers.push_back(parsedExp(member.object()));
		member.dereference();
	}
 }
void	Qualifications::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
QualifiedSymbol::QualifiedSymbol(
	int		main_index,
	parsedExp&	P_symbol,
	parsedExp&	P_qualifications,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		symbol(P_symbol),
		qualifications(P_qualifications)
	{
	mainIndex = main_index;
	if(symbol) {
		initExpression(symbol->getData());
	} else {
		initExpression("(null)");
	}
}
 QualifiedSymbol::QualifiedSymbol(const QualifiedSymbol &Q)
		: pE(origin_info(Q.line, Q.file)) {
	if(Q.symbol) {
		symbol = Q.symbol->copy();
		Symbol* s = dynamic_cast<Symbol*>(symbol.object());
		my_level = s->my_level;
		my_index = s->my_index;
	} else {
		my_level = 0;
		my_index = 0;
	}
	if(Q.qualifications) {
		qualifications = Q.qualifications->copy();
	}
	if(Q.expression_list) {
		expression_list = Q.expression_list->copy();
	}
	theData = Q.getData();
	tolerant_array_evaluation = true;
	if(Q.indices) {
		MultidimIndices* mi = dynamic_cast<MultidimIndices*>(Q.indices->copy());
		indices.reference(mi);
	}
	if(Q.a_member) {
		ClassMember* cm = dynamic_cast<ClassMember*>(Q.a_member->copy());
		a_member.reference(cm);
	}
 }
 QualifiedSymbol::QualifiedSymbol(bool, const QualifiedSymbol &Q)
		: pE(origin_info(Q.line, Q.file)) {
	symbol = Q.symbol;
	if(symbol) {
		my_level = Q.my_level;
		my_index = Q.my_index;
	} else {
		my_level = 0;
		my_index = 0;
	}
	qualifications = Q.qualifications;
	expression_list = Q.expression_list;
	theData = Q.getData();
	tolerant_array_evaluation = true;
	indices = Q.indices;
	a_member = Q.a_member;
 }
  void	QualifiedSymbol::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	if(indices) {
		indices->recursively_apply(EA);
	}
 	decomp_finder* DF = dynamic_cast<decomp_finder*>(&EA);
	if(!DF) {

		//
		// a_member would be processed as a Symbol by DF
		//
		if(a_member) {
			a_member->recursively_apply(EA);
		}
	}
	EA.post_analyze(this);
 }
 void	QualifiedSymbol::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	tolerant_array_evaluation = true;
 }
 void	QualifiedSymbol::eval_expression(
		behaving_base*	loc,
		TypedValue&	result) {

	//
	// JIRA AP-1407, more info in error messages
	//
	tolerant_array_evaluation = false;
	result = get_val_ref(loc);
	tolerant_array_evaluation = true;

	//
	// JIRA AP-1150, Nonexistent array element error not reported
	//
	if(result.get_type() == apgen::DATA_TYPE::UNINITIALIZED) {
		Cstring errs;
		errs << "File " << file << ", line " << line
		    << ": " << getData() << " does not have the requested element.";
		throw(eval_error(errs));
	}
 }
RaiseToPower::RaiseToPower(
	int		main_index,
	parsedExp&	P_tok_exponent,
	parsedExp&	P_maybe_a_factor,
	parsedExp&	P_atom,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_exponent(P_tok_exponent),
		maybe_a_factor(P_maybe_a_factor),
		atom(P_atom)
	{
	mainIndex = main_index;
	if(tok_exponent) {
		initExpression(tok_exponent->getData());
	} else {
		initExpression("(null)");
	}
}
RaiseToPower::RaiseToPower(const RaiseToPower& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.atom) {
		atom.reference(p.atom->copy());
	}
	if(p.maybe_a_factor) {
		maybe_a_factor.reference(p.maybe_a_factor->copy());
	}
	if(p.tok_exponent) {
		tok_exponent.reference(p.tok_exponent->copy());
	}
	initExpression(p.getData());
}
RaiseToPower::RaiseToPower(bool, const RaiseToPower& p)
	: pE(origin_info(p.line, p.file)),
		atom(p.atom),
		maybe_a_factor(p.maybe_a_factor),
		tok_exponent(p.tok_exponent) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void RaiseToPower::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(atom) {
		atom->recursively_apply(EA);
	}
	if(maybe_a_factor) {
		maybe_a_factor->recursively_apply(EA);
	}
	if(tok_exponent) {
		tok_exponent->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	initExpression(
		const Cstring& nodeData);
 void	RaiseToPower::eval_expression(
		behaving_base*	local_object,
		TypedValue&	result
		) {
	TypedValue val1, val2;
	try {
	    maybe_a_factor->eval_expression(local_object, val1);
	    atom->eval_expression(local_object, val2);
	    binaryFunc(
		val1,
		val2,
		result);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ":\n" << Err.msg;
		throw(eval_error(err));
	}

	if(APcloptions::theCmdLineOptions().debug_execute) {
		Cstring tmp = val1.to_string();
		tmp << " " << theData << " " << val2.to_string()
			<< " = " << result.to_string();
		cerr << "RaiseToPower::eval - " << tmp << "\n";
	}
 }
RangeExpression::RangeExpression(
	int		main_index,
	parsedExp&	P_Expression,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression)

	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
RangeExpression::RangeExpression(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_range_sym,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_range_sym(P_tok_range_sym)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
RangeExpression::RangeExpression(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_range_sym,
	parsedExp&	P_Expression_2,
	Differentiator_2)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_range_sym(P_tok_range_sym),
		Expression_2(P_Expression_2)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
RangeExpression::RangeExpression(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_range_sym,
	parsedExp&	P_tok_multiple_of,
	parsedExp&	P_Expression_3,
	parsedExp&	P_opt_rest_of_range,
	Differentiator_3)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_range_sym(P_tok_range_sym),
		tok_multiple_of(P_tok_multiple_of),
		Expression_3(P_Expression_3),
		opt_rest_of_range(P_opt_rest_of_range)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
RangeExpression::RangeExpression(
	int		main_index,
	parsedExp&	P_tok_multiple_of,
	parsedExp&	P_Expression,
	parsedExp&	P_opt_rest_of_range,
	Differentiator_4)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_multiple_of(P_tok_multiple_of),
		Expression(P_Expression),
		opt_rest_of_range(P_opt_rest_of_range)
	{
	mainIndex = main_index;
	if(tok_multiple_of) {
		initExpression(tok_multiple_of->getData());
	} else {
		initExpression("(null)");
	}
}
RangeExpression::RangeExpression(const RangeExpression& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.Expression_1) {
		Expression_1.reference(p.Expression_1->copy());
	}
	if(p.Expression_2) {
		Expression_2.reference(p.Expression_2->copy());
	}
	if(p.Expression_3) {
		Expression_3.reference(p.Expression_3->copy());
	}
	if(p.opt_rest_of_range) {
		opt_rest_of_range.reference(p.opt_rest_of_range->copy());
	}
	if(p.opt_rest_of_range_2) {
		opt_rest_of_range_2.reference(p.opt_rest_of_range_2->copy());
	}
	if(p.tok_multiple_of) {
		tok_multiple_of.reference(p.tok_multiple_of->copy());
	}
	if(p.tok_range_sym) {
		tok_range_sym.reference(p.tok_range_sym->copy());
	}
	if(p.tok_range_sym_1) {
		tok_range_sym_1.reference(p.tok_range_sym_1->copy());
	}
	initExpression(p.getData());
}
RangeExpression::RangeExpression(bool, const RangeExpression& p)
	: pE(origin_info(p.line, p.file)),
		Expression(p.Expression),
		Expression_1(p.Expression_1),
		Expression_2(p.Expression_2),
		Expression_3(p.Expression_3),
		opt_rest_of_range(p.opt_rest_of_range),
		opt_rest_of_range_2(p.opt_rest_of_range_2),
		tok_multiple_of(p.tok_multiple_of),
		tok_range_sym(p.tok_range_sym),
		tok_range_sym_1(p.tok_range_sym_1) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void RangeExpression::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(Expression_1) {
		Expression_1->recursively_apply(EA);
	}
	if(Expression_2) {
		Expression_2->recursively_apply(EA);
	}
	if(Expression_3) {
		Expression_3->recursively_apply(EA);
	}
	if(opt_rest_of_range) {
		opt_rest_of_range->recursively_apply(EA);
	}
	if(opt_rest_of_range_2) {
		opt_rest_of_range_2->recursively_apply(EA);
	}
	if(tok_multiple_of) {
		tok_multiple_of->recursively_apply(EA);
	}
	if(tok_range_sym) {
		tok_range_sym->recursively_apply(EA);
	}
	if(tok_range_sym_1) {
		tok_range_sym_1->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	RangeExpression::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	RangeExpression::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ResUsageWithArgs::ResUsageWithArgs(
	int		main_index,
	parsedExp&	P_resource_usage_name,
	parsedExp&	P_tok_action,
	parsedExp&	P_ASCII_40,	// (
	parsedExp&	P_optional_expression_list,
	parsedExp&	P_ASCII_41,	// )
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		resource_usage_name(P_resource_usage_name),
		tok_action(P_tok_action),
		ASCII_40(P_ASCII_40),
		optional_expression_list(P_optional_expression_list),
		ASCII_41(P_ASCII_41)
	{
	mainIndex = main_index;
	if(resource_usage_name) {
		initExpression(resource_usage_name->getData());
	} else {
		initExpression("(null)");
	}
}
ResUsageWithArgs::ResUsageWithArgs(const ResUsageWithArgs& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_40) {
		ASCII_40.reference(p.ASCII_40->copy());
	}
	if(p.ASCII_41) {
		ASCII_41.reference(p.ASCII_41->copy());
	}
	if(p.optional_expression_list) {
		optional_expression_list.reference(p.optional_expression_list->copy());
	}
	if(p.resource_usage_name) {
		resource_usage_name.reference(p.resource_usage_name->copy());
	}
	if(p.tok_action) {
		tok_action.reference(p.tok_action->copy());
	}
	initExpression(p.getData());
}
ResUsageWithArgs::ResUsageWithArgs(bool, const ResUsageWithArgs& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_40(p.ASCII_40),
		ASCII_41(p.ASCII_41),
		optional_expression_list(p.optional_expression_list),
		resource_usage_name(p.resource_usage_name),
		tok_action(p.tok_action) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ResUsageWithArgs::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_40) {
		ASCII_40->recursively_apply(EA);
	}
	if(ASCII_41) {
		ASCII_41->recursively_apply(EA);
	}
	if(optional_expression_list) {
		optional_expression_list->recursively_apply(EA);
	}
	if(resource_usage_name) {
		resource_usage_name->recursively_apply(EA);
	}
	if(tok_action) {
		tok_action->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ResUsageWithArgs::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ResUsageWithArgs::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Resource::Resource(
	int		main_index,
	parsedExp&	P_resource_prefix,
	parsedExp&	P_resource_def,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		resource_prefix(P_resource_prefix),
		resource_def(P_resource_def)
	{
	mainIndex = main_index;
	if(resource_prefix) {
		initExpression(resource_prefix->getData());
	} else {
		initExpression("(null)");
	}
}
Resource::Resource(const Resource& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.resource_def) {
		resource_def.reference(p.resource_def->copy());
	}
	if(p.resource_prefix) {
		resource_prefix.reference(p.resource_prefix->copy());
	}
	initExpression(p.getData());
}
Resource::Resource(bool, const Resource& p)
	: pE(origin_info(p.line, p.file)),
		resource_def(p.resource_def),
		resource_prefix(p.resource_prefix) {
	expressions = p.expressions;
	initExpression(p.getData());
}
 void Resource::recursively_apply(exp_analyzer& EA) {
    EA.pre_analyze(this);

    decomp_finder* DF = dynamic_cast<decomp_finder*>(&EA);
    if(DF) {
	bool abstract;
	Cstring name;
	DF->handle_a_resource(this, abstract, name);
	if(abstract) {
		Behavior* ptr = Behavior::find_type("abstract resource",
						    name);
		assert(ptr);
		const vector<task*>& tasks
			= ptr->tasks;
		for(int i = 0; i < tasks.size(); i++) {
			if(tasks[i]->prog) {
				tasks[i]->prog->recursively_apply(EA);
			}
		}
	} else {
		RCsource* container
			= RCsource::resource_containers().find(name);
		assert(container);
		Rsource* first_resource =
		    container->payload->Object->array_elements[0];
		const vector<task*>& tasks = first_resource->Type.tasks;
		for(int i = 0; i < tasks.size(); i++) {
			if(tasks[i]->prog) {
				tasks[i]->prog->recursively_apply(EA);
			}
		}
	}
    } else {
        ResourceDef* rd = dynamic_cast<ResourceDef*>(resource_def.object());
        ResourceInfo* ri = dynamic_cast<ResourceInfo*>(resource_prefix.object());
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	rd->recursively_apply(EA);
	ri->recursively_apply(EA);
    }
    EA.post_analyze(this);
 }
void	Resource::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	Resource::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ResourceDef::ResourceDef(
	int		main_index,
	parsedExp&	P_concrete_resource_def_top_line,
	parsedExp&	P_ASCII_123,	// {
	parsedExp&	P_res_parameter_section_or_null,
	parsedExp&	P_ASCII_125,	// }
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		concrete_resource_def_top_line(P_concrete_resource_def_top_line),
		ASCII_123(P_ASCII_123),
		res_parameter_section_or_null(P_res_parameter_section_or_null),
		ASCII_125(P_ASCII_125)
	{
	mainIndex = main_index;
	if(concrete_resource_def_top_line) {
		initExpression(concrete_resource_def_top_line->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceDef::ResourceDef(
	int		main_index,
	parsedExp&	P_abstract_resource_def_top_line,
	parsedExp&	P_ASCII_123,	// {
	parsedExp&	P_res_parameter_section_or_null,
	parsedExp&	P_ASCII_125,	// }
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		abstract_resource_def_top_line(P_abstract_resource_def_top_line),
		ASCII_123(P_ASCII_123),
		res_parameter_section_or_null(P_res_parameter_section_or_null),
		ASCII_125(P_ASCII_125)
	{
	mainIndex = main_index;
	if(abstract_resource_def_top_line) {
		initExpression(abstract_resource_def_top_line->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceDef::ResourceDef(
	int		main_index,
	parsedExp&	P_abstract_resource_def_top_line,
	parsedExp&	P_tok_begin,
	parsedExp&	P_opt_attributes_section_res,
	parsedExp&	P_res_parameter_section_or_null,
	parsedExp&	P_resource_usage_section,
	parsedExp&	P_tok_end,
	parsedExp&	P_tok_resource,
	parsedExp&	P_tok_id,
	Differentiator_2)
	: pE(origin_info(yylineno, aafReader::current_file())),
		abstract_resource_def_top_line(P_abstract_resource_def_top_line),
		tok_begin(P_tok_begin),
		opt_attributes_section_res(P_opt_attributes_section_res),
		res_parameter_section_or_null(P_res_parameter_section_or_null),
		resource_usage_section(P_resource_usage_section),
		tok_end(P_tok_end),
		tok_resource(P_tok_resource),
		tok_id(P_tok_id)
	{
	mainIndex = main_index;
	if(abstract_resource_def_top_line) {
		initExpression(abstract_resource_def_top_line->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceDef::ResourceDef(
	int		main_index,
	parsedExp&	P_concrete_resource_def_top_line,
	parsedExp&	P_tok_begin,
	parsedExp&	P_opt_attributes_section_res,
	parsedExp&	P_res_parameter_section_or_null,
	parsedExp&	P_opt_states_section,
	parsedExp&	P_opt_profile_section,
	parsedExp&	P_opt_time_series,
	parsedExp&	P_opt_default_section,
	parsedExp&	P_usage_section_or_null,
	Differentiator_3)
	: pE(origin_info(yylineno, aafReader::current_file())),
		concrete_resource_def_top_line(P_concrete_resource_def_top_line),
		tok_begin(P_tok_begin),
		opt_attributes_section_res(P_opt_attributes_section_res),
		res_parameter_section_or_null(P_res_parameter_section_or_null),
		opt_states_section(P_opt_states_section),
		opt_profile_section(P_opt_profile_section),
		opt_time_series(P_opt_time_series),
		opt_default_section(P_opt_default_section),
		usage_section_or_null(P_usage_section_or_null)
	{
	mainIndex = main_index;
	if(concrete_resource_def_top_line) {
		initExpression(concrete_resource_def_top_line->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceDef::ResourceDef(const ResourceDef& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_123) {
		ASCII_123.reference(p.ASCII_123->copy());
	}
	if(p.ASCII_123_1) {
		ASCII_123_1.reference(p.ASCII_123_1->copy());
	}
	if(p.ASCII_125) {
		ASCII_125.reference(p.ASCII_125->copy());
	}
	if(p.ASCII_125_3) {
		ASCII_125_3.reference(p.ASCII_125_3->copy());
	}
	if(p.abstract_resource_def_top_line) {
		abstract_resource_def_top_line.reference(p.abstract_resource_def_top_line->copy());
	}
	if(p.concrete_resource_def_top_line) {
		concrete_resource_def_top_line.reference(p.concrete_resource_def_top_line->copy());
	}
	if(p.opt_attributes_section_res) {
		opt_attributes_section_res.reference(p.opt_attributes_section_res->copy());
	}
	if(p.opt_attributes_section_res_2) {
		opt_attributes_section_res_2.reference(p.opt_attributes_section_res_2->copy());
	}
	if(p.opt_default_section) {
		opt_default_section.reference(p.opt_default_section->copy());
	}
	if(p.opt_profile_section) {
		opt_profile_section.reference(p.opt_profile_section->copy());
	}
	if(p.opt_states_section) {
		opt_states_section.reference(p.opt_states_section->copy());
	}
	if(p.opt_time_series) {
		opt_time_series.reference(p.opt_time_series->copy());
	}
	if(p.res_parameter_section_or_null) {
		res_parameter_section_or_null.reference(p.res_parameter_section_or_null->copy());
	}
	if(p.res_parameter_section_or_null_2) {
		res_parameter_section_or_null_2.reference(p.res_parameter_section_or_null_2->copy());
	}
	if(p.res_parameter_section_or_null_3) {
		res_parameter_section_or_null_3.reference(p.res_parameter_section_or_null_3->copy());
	}
	if(p.resource_usage_section) {
		resource_usage_section.reference(p.resource_usage_section->copy());
	}
	if(p.tok_begin) {
		tok_begin.reference(p.tok_begin->copy());
	}
	if(p.tok_begin_1) {
		tok_begin_1.reference(p.tok_begin_1->copy());
	}
	if(p.tok_end) {
		tok_end.reference(p.tok_end->copy());
	}
	if(p.tok_id) {
		tok_id.reference(p.tok_id->copy());
	}
	if(p.tok_resource) {
		tok_resource.reference(p.tok_resource->copy());
	}
	if(p.usage_section_or_null) {
		usage_section_or_null.reference(p.usage_section_or_null->copy());
	}
	initExpression(p.getData());
}
ResourceDef::ResourceDef(bool, const ResourceDef& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_123(p.ASCII_123),
		ASCII_123_1(p.ASCII_123_1),
		ASCII_125(p.ASCII_125),
		ASCII_125_3(p.ASCII_125_3),
		abstract_resource_def_top_line(p.abstract_resource_def_top_line),
		concrete_resource_def_top_line(p.concrete_resource_def_top_line),
		opt_attributes_section_res(p.opt_attributes_section_res),
		opt_attributes_section_res_2(p.opt_attributes_section_res_2),
		opt_default_section(p.opt_default_section),
		opt_profile_section(p.opt_profile_section),
		opt_states_section(p.opt_states_section),
		opt_time_series(p.opt_time_series),
		res_parameter_section_or_null(p.res_parameter_section_or_null),
		res_parameter_section_or_null_2(p.res_parameter_section_or_null_2),
		res_parameter_section_or_null_3(p.res_parameter_section_or_null_3),
		resource_usage_section(p.resource_usage_section),
		tok_begin(p.tok_begin),
		tok_begin_1(p.tok_begin_1),
		tok_end(p.tok_end),
		tok_id(p.tok_id),
		tok_resource(p.tok_resource),
		usage_section_or_null(p.usage_section_or_null) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ResourceDef::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_123) {
		ASCII_123->recursively_apply(EA);
	}
	if(ASCII_123_1) {
		ASCII_123_1->recursively_apply(EA);
	}
	if(ASCII_125) {
		ASCII_125->recursively_apply(EA);
	}
	if(ASCII_125_3) {
		ASCII_125_3->recursively_apply(EA);
	}
	if(abstract_resource_def_top_line) {
		abstract_resource_def_top_line->recursively_apply(EA);
	}
	if(concrete_resource_def_top_line) {
		concrete_resource_def_top_line->recursively_apply(EA);
	}
	if(opt_attributes_section_res) {
		opt_attributes_section_res->recursively_apply(EA);
	}
	if(opt_attributes_section_res_2) {
		opt_attributes_section_res_2->recursively_apply(EA);
	}
	if(opt_default_section) {
		opt_default_section->recursively_apply(EA);
	}
	if(opt_profile_section) {
		opt_profile_section->recursively_apply(EA);
	}
	if(opt_states_section) {
		opt_states_section->recursively_apply(EA);
	}
	if(opt_time_series) {
		opt_time_series->recursively_apply(EA);
	}
	if(res_parameter_section_or_null) {
		res_parameter_section_or_null->recursively_apply(EA);
	}
	if(res_parameter_section_or_null_2) {
		res_parameter_section_or_null_2->recursively_apply(EA);
	}
	if(res_parameter_section_or_null_3) {
		res_parameter_section_or_null_3->recursively_apply(EA);
	}
	if(resource_usage_section) {
		resource_usage_section->recursively_apply(EA);
	}
	if(tok_begin) {
		tok_begin->recursively_apply(EA);
	}
	if(tok_begin_1) {
		tok_begin_1->recursively_apply(EA);
	}
	if(tok_end) {
		tok_end->recursively_apply(EA);
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	if(tok_resource) {
		tok_resource->recursively_apply(EA);
	}
	if(usage_section_or_null) {
		usage_section_or_null->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ResourceDef::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ResourceDef::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ResourceInfo::ResourceInfo(
	int		main_index,
	parsedExp&	P_tok_id,
	parsedExp&	P_tok_resource,
	parsedExp&	P_resource_arrays,
	parsedExp&	P_ASCII_58,	// :
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id),
		tok_resource(P_tok_resource),
		resource_arrays(P_resource_arrays),
		ASCII_58(P_ASCII_58)
	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceInfo::ResourceInfo(const ResourceInfo& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_58) {
		ASCII_58.reference(p.ASCII_58->copy());
	}
	if(p.resource_arrays) {
		resource_arrays.reference(p.resource_arrays->copy());
	}
	if(p.tok_id) {
		tok_id.reference(p.tok_id->copy());
	}
	if(p.tok_resource) {
		tok_resource.reference(p.tok_resource->copy());
	}
	initExpression(p.getData());
}
ResourceInfo::ResourceInfo(bool, const ResourceInfo& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_58(p.ASCII_58),
		resource_arrays(p.resource_arrays),
		tok_id(p.tok_id),
		tok_resource(p.tok_resource) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ResourceInfo::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_58) {
		ASCII_58->recursively_apply(EA);
	}
	if(resource_arrays) {
		resource_arrays->recursively_apply(EA);
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	if(tok_resource) {
		tok_resource->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	ResourceInfo::initExpression(
		const Cstring& nodeData) {
	theData = "nodeData";
	aafReader::current_resource() = tok_id->getData();
	if(!aafReader::resources().find(tok_id->getData())) {
		aafReader::resources() << new emptySymbol(tok_id->getData());
	}
 }
void	ResourceInfo::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ResourceType::ResourceType(
	int		main_index,
	parsedExp&	P_tok_assoc,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_assoc(P_tok_assoc)

	{
	mainIndex = main_index;
	if(tok_assoc) {
		initExpression(tok_assoc->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceType::ResourceType(
	int		main_index,
	parsedExp&	P_tok_assoc,
	parsedExp&	P_tok_consumable,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_assoc(P_tok_assoc),
		tok_consumable(P_tok_consumable)
	{
	mainIndex = main_index;
	if(tok_assoc) {
		initExpression(tok_assoc->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceType::ResourceType(
	int		main_index,
	parsedExp&	P_tok_integral,
	parsedExp&	P_tok_id,
	Differentiator_2)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_integral(P_tok_integral),
		tok_id(P_tok_id)
	{
	mainIndex = main_index;
	if(tok_integral) {
		initExpression(tok_integral->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceType::ResourceType(
	int		main_index,
	parsedExp&	P_tok_consumable,
	Differentiator_3)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_consumable(P_tok_consumable)

	{
	mainIndex = main_index;
	if(tok_consumable) {
		initExpression(tok_consumable->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceType::ResourceType(
	int		main_index,
	parsedExp&	P_tok_nonconsumable,
	Differentiator_4)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_nonconsumable(P_tok_nonconsumable)

	{
	mainIndex = main_index;
	if(tok_nonconsumable) {
		initExpression(tok_nonconsumable->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceType::ResourceType(
	int		main_index,
	parsedExp&	P_tok_settable,
	Differentiator_5)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_settable(P_tok_settable)

	{
	mainIndex = main_index;
	if(tok_settable) {
		initExpression(tok_settable->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceType::ResourceType(
	int		main_index,
	parsedExp&	P_tok_state,
	Differentiator_6)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_state(P_tok_state)

	{
	mainIndex = main_index;
	if(tok_state) {
		initExpression(tok_state->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceType::ResourceType(
	int		main_index,
	parsedExp&	P_tok_extern,
	Differentiator_7)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_extern(P_tok_extern)

	{
	mainIndex = main_index;
	if(tok_extern) {
		initExpression(tok_extern->getData());
	} else {
		initExpression("(null)");
	}
}
ResourceType::ResourceType(const ResourceType& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.tok_assoc) {
		tok_assoc.reference(p.tok_assoc->copy());
	}
	if(p.tok_consumable) {
		tok_consumable.reference(p.tok_consumable->copy());
	}
	if(p.tok_extern) {
		tok_extern.reference(p.tok_extern->copy());
	}
	if(p.tok_id) {
		tok_id.reference(p.tok_id->copy());
	}
	if(p.tok_integral) {
		tok_integral.reference(p.tok_integral->copy());
	}
	if(p.tok_nonconsumable) {
		tok_nonconsumable.reference(p.tok_nonconsumable->copy());
	}
	if(p.tok_settable) {
		tok_settable.reference(p.tok_settable->copy());
	}
	if(p.tok_state) {
		tok_state.reference(p.tok_state->copy());
	}
	initExpression(p.getData());
}
ResourceType::ResourceType(bool, const ResourceType& p)
	: pE(origin_info(p.line, p.file)),
		tok_assoc(p.tok_assoc),
		tok_consumable(p.tok_consumable),
		tok_extern(p.tok_extern),
		tok_id(p.tok_id),
		tok_integral(p.tok_integral),
		tok_nonconsumable(p.tok_nonconsumable),
		tok_settable(p.tok_settable),
		tok_state(p.tok_state) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ResourceType::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(tok_assoc) {
		tok_assoc->recursively_apply(EA);
	}
	if(tok_consumable) {
		tok_consumable->recursively_apply(EA);
	}
	if(tok_extern) {
		tok_extern->recursively_apply(EA);
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	if(tok_integral) {
		tok_integral->recursively_apply(EA);
	}
	if(tok_nonconsumable) {
		tok_nonconsumable->recursively_apply(EA);
	}
	if(tok_settable) {
		tok_settable->recursively_apply(EA);
	}
	if(tok_state) {
		tok_state->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ResourceType::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ResourceType::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Return::Return(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_return,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_return(P_tok_return),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
Return::Return(const Return& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.tok_return) {
		tok_return.reference(p.tok_return->copy());
	}
	initExpression(p.getData());
}
Return::Return(bool, const Return& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		tok_return(p.tok_return) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Return::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(tok_return) {
		tok_return->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Return::initExpression(
		const Cstring& nodeData) {
	theData = "Return";
 }
void	Return::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
ReverseRangeExpression::ReverseRangeExpression(
	int		main_index,
	parsedExp&	P_tok_range_sym,
	parsedExp&	P_Expression,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_range_sym(P_tok_range_sym),
		Expression(P_Expression)
	{
	mainIndex = main_index;
	if(tok_range_sym) {
		initExpression(tok_range_sym->getData());
	} else {
		initExpression("(null)");
	}
}
ReverseRangeExpression::ReverseRangeExpression(const ReverseRangeExpression& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.tok_range_sym) {
		tok_range_sym.reference(p.tok_range_sym->copy());
	}
	initExpression(p.getData());
}
ReverseRangeExpression::ReverseRangeExpression(bool, const ReverseRangeExpression& p)
	: pE(origin_info(p.line, p.file)),
		Expression(p.Expression),
		tok_range_sym(p.tok_range_sym) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void ReverseRangeExpression::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(tok_range_sym) {
		tok_range_sym->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	ReverseRangeExpression::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	ReverseRangeExpression::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Signature::Signature(
	int		main_index,
	parsedExp&	P_signature_stuff,
	parsedExp&	P_tok_id,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		signature_stuff(P_signature_stuff),
		tok_id(P_tok_id)
	{
	mainIndex = main_index;
	if(signature_stuff) {
		initExpression(signature_stuff->getData());
	} else {
		initExpression("(null)");
	}
}
Signature::Signature(const Signature& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.signature_stuff) {
		signature_stuff.reference(p.signature_stuff->copy());
	}
	if(p.tok_id) {
		tok_id.reference(p.tok_id->copy());
	}
	initExpression(p.getData());
}
Signature::Signature(bool, const Signature& p)
	: pE(origin_info(p.line, p.file)),
		signature_stuff(p.signature_stuff),
		tok_id(p.tok_id) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Signature::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(signature_stuff) {
		signature_stuff->recursively_apply(EA);
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	Signature::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	Signature::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
SingleIndex::SingleIndex(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_ASCII_91,	// [
	parsedExp&	P_ASCII_93,	// ]
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		ASCII_91(P_ASCII_91),
		ASCII_93(P_ASCII_93)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
SingleIndex::SingleIndex(const SingleIndex& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_91) {
		ASCII_91.reference(p.ASCII_91->copy());
	}
	if(p.ASCII_93) {
		ASCII_93.reference(p.ASCII_93->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	initExpression(p.getData());
}
SingleIndex::SingleIndex(bool, const SingleIndex& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_91(p.ASCII_91),
		ASCII_93(p.ASCII_93),
		Expression(p.Expression) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void SingleIndex::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_91) {
		ASCII_91->recursively_apply(EA);
	}
	if(ASCII_93) {
		ASCII_93->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	SingleIndex::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	SingleIndex::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
SinglePrecompResName::SinglePrecompResName(
	int		main_index,
	parsedExp&	P_tok_stringval,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_stringval(P_tok_stringval)

	{
	mainIndex = main_index;
	if(tok_stringval) {
		initExpression(tok_stringval->getData());
	} else {
		initExpression("(null)");
	}
}
SinglePrecompResName::SinglePrecompResName(const SinglePrecompResName& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.tok_stringval) {
		tok_stringval.reference(p.tok_stringval->copy());
	}
	initExpression(p.getData());
}
SinglePrecompResName::SinglePrecompResName(bool, const SinglePrecompResName& p)
	: pE(origin_info(p.line, p.file)),
		tok_stringval(p.tok_stringval) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void SinglePrecompResName::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(tok_stringval) {
		tok_stringval->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	SinglePrecompResName::initExpression(
		const Cstring& nodeData) {
    Cstring theDataCopy(nodeData);
    removeQuotes(theDataCopy);
    theData = theDataCopy;
    aafReader::precomp_container*	container = aafReader::precomp_containers().last_node();
    aafReader::single_precomp_res*	single_res = new aafReader::single_precomp_res(theData);

    container->payload << single_res;
    aafReader::single_precomp_res::UnderConsolidation = single_res;
 }
void	SinglePrecompResName::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Symbol::Symbol(
	int		main_index,
	parsedExp&	P_tok_id,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id)

	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
Symbol::Symbol(
	int		main_index,
	parsedExp&	P_tok_id,
	Differentiator_1)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id)

	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
Symbol::Symbol(
	int		main_index,
	parsedExp&	P_tok_id,
	Differentiator_2)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_id(P_tok_id)

	{
	mainIndex = main_index;
	if(tok_id) {
		initExpression(tok_id->getData());
	} else {
		initExpression("(null)");
	}
}
Symbol::Symbol(
	int		main_index,
	parsedExp&	P_tok_start,
	Differentiator_3)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_start(P_tok_start)

	{
	mainIndex = main_index;
	if(tok_start) {
		initExpression(tok_start->getData());
	} else {
		initExpression("(null)");
	}
}
Symbol::Symbol(
	int		main_index,
	parsedExp&	P_tok_finish,
	Differentiator_4)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_finish(P_tok_finish)

	{
	mainIndex = main_index;
	if(tok_finish) {
		initExpression(tok_finish->getData());
	} else {
		initExpression("(null)");
	}
}
Symbol::Symbol(
	int		main_index,
	parsedExp&	P_tok_nodeid,
	Differentiator_5)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_nodeid(P_tok_nodeid)

	{
	mainIndex = main_index;
	if(tok_nodeid) {
		initExpression(tok_nodeid->getData());
	} else {
		initExpression("(null)");
	}
}
Symbol::Symbol(
	int		main_index,
	parsedExp&	P_tok_type,
	Differentiator_6)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_type(P_tok_type)

	{
	mainIndex = main_index;
	if(tok_type) {
		initExpression(tok_type->getData());
	} else {
		initExpression("(null)");
	}
}
 Symbol::Symbol(const Symbol&S)
		: pE(origin_info(S.line, S.file)) {
	if(S.tok_finish) {
		tok_finish = S.tok_finish->copy();
	} else if(S.tok_id) {
		tok_id = S.tok_id->copy();
	} else if(S.tok_nodeid) {
		tok_nodeid = S.tok_nodeid->copy();
	} else if(S.tok_start) {
		tok_start = S.tok_start->copy();
	} else if(S.tok_type) {
		tok_type = S.tok_type->copy();
	}
	// if(S.qualifications) {
	// 	qualifications = S.qualifications->copy();
	// }
	// if(S.expression_list) {
	// 	expression_list = S.expression_list->copy();
	// }
	// evaluator = S.evaluator;
	theData = S.getData();
 }
 Symbol::Symbol(bool, const Symbol&S)
		: pE(origin_info(S.line, S.file)) {
	tok_finish = S.tok_finish;
	tok_id = S.tok_id;
	tok_nodeid = S.tok_nodeid;
	tok_start = S.tok_start;
	tok_type = S.tok_type;
	theData = S.getData();
 }
void Symbol::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(tok_finish) {
		tok_finish->recursively_apply(EA);
	}
	if(tok_id) {
		tok_id->recursively_apply(EA);
	}
	if(tok_nodeid) {
		tok_nodeid->recursively_apply(EA);
	}
	if(tok_start) {
		tok_start->recursively_apply(EA);
	}
	if(tok_type) {
		tok_type->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Symbol::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
 }
 bool Symbol::debug_symbol = false;
 void	Symbol::eval_expression(
		behaving_base*	loc,
		TypedValue&	result)
			{
	result = get_val_ref(loc);

	if(eval_error::thread_unsafe_error()) {
		Cstring err;
		err << "File " << file << ", line " << line << ": "
			<< "referencing non-constant variable "
			<< getData()
			<< " from a thread that does not own it.\n";
		symNode* sn = aafReader::assignments_to_global_arrays().find(
			getData());
		if(sn) {
			err << "Assignments to this array were found at\n"
				<< sn->payload << "\n";
		}
		throw(eval_error(err));
	}
 }
TemporalSpec::TemporalSpec(
	int		main_index,
	parsedExp&	P_tok_from,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_to,
	parsedExp&	P_Expression_3,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_from(P_tok_from),
		Expression(P_Expression),
		tok_to(P_tok_to),
		Expression_3(P_Expression_3)
	{
	mainIndex = main_index;
	if(tok_from) {
		initExpression(tok_from->getData());
	} else {
		initExpression("(null)");
	}
}
TemporalSpec::TemporalSpec(
	int		main_index,
	parsedExp&	P_tok_at,
	parsedExp&	P_Expression,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_at(P_tok_at),
		Expression(P_Expression)
	{
	mainIndex = main_index;
	if(tok_at) {
		initExpression(tok_at->getData());
	} else {
		initExpression("(null)");
	}
}
TemporalSpec::TemporalSpec(
	int		main_index,
	parsedExp&	P_tok_immediately,
	Differentiator_2)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_immediately(P_tok_immediately)

	{
	mainIndex = main_index;
	if(tok_immediately) {
		initExpression(tok_immediately->getData());
	} else {
		initExpression("(null)");
	}
}
TemporalSpec::TemporalSpec(const TemporalSpec& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.Expression_1) {
		Expression_1.reference(p.Expression_1->copy());
	}
	if(p.Expression_3) {
		Expression_3.reference(p.Expression_3->copy());
	}
	if(p.tok_at) {
		tok_at.reference(p.tok_at->copy());
	}
	if(p.tok_from) {
		tok_from.reference(p.tok_from->copy());
	}
	if(p.tok_immediately) {
		tok_immediately.reference(p.tok_immediately->copy());
	}
	if(p.tok_to) {
		tok_to.reference(p.tok_to->copy());
	}
	initExpression(p.getData());
}
TemporalSpec::TemporalSpec(bool, const TemporalSpec& p)
	: pE(origin_info(p.line, p.file)),
		Expression(p.Expression),
		Expression_1(p.Expression_1),
		Expression_3(p.Expression_3),
		tok_at(p.tok_at),
		tok_from(p.tok_from),
		tok_immediately(p.tok_immediately),
		tok_to(p.tok_to) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void TemporalSpec::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(Expression_1) {
		Expression_1->recursively_apply(EA);
	}
	if(Expression_3) {
		Expression_3->recursively_apply(EA);
	}
	if(tok_at) {
		tok_at->recursively_apply(EA);
	}
	if(tok_from) {
		tok_from->recursively_apply(EA);
	}
	if(tok_immediately) {
		tok_immediately->recursively_apply(EA);
	}
	if(tok_to) {
		tok_to->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	TemporalSpec::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	TemporalSpec::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
TimeSeriesStart::TimeSeriesStart(
	int		main_index,
	parsedExp&	P_tok_time_series,
	Differentiator_0)

	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_time_series(P_tok_time_series)

	{
	mainIndex = main_index;
	if(tok_time_series) {
		initExpression(tok_time_series->getData());
	} else {
		initExpression("(null)");
	}
}
TimeSeriesStart::TimeSeriesStart(const TimeSeriesStart& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.tok_time_series) {
		tok_time_series.reference(p.tok_time_series->copy());
	}
	initExpression(p.getData());
}
TimeSeriesStart::TimeSeriesStart(bool, const TimeSeriesStart& p)
	: pE(origin_info(p.line, p.file)),
		tok_time_series(p.tok_time_series) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void TimeSeriesStart::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(tok_time_series) {
		tok_time_series->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	TimeSeriesStart::initExpression(
		const Cstring& nodeData) {
    theData = nodeData;

    //
    // This is our chance to create a new structure
    // to hold the time series data for the pre-computed
    // resource being parsed.
    //

    //
    // The last item in the list of resources is the name
    // of the resource currently being parsed, unless there
    // is an error in the adaptation and the resource has
    // already been defined
    //
    Cstring resname = aafReader::current_resource();

    emptySymbol* res_node = aafReader::resources().find(resname);
    if(res_node != aafReader::resources().last_node()
       || aafReader::precomp_containers().find(resname)) {
	Cstring err;
	err << "File " << file << ", line " << line
	    << ": resource " << resname << " has already been defined\n";
	throw(eval_error(err));
    }
    aafReader::precomp_container* container = new aafReader::precomp_container(resname);
    aafReader::precomp_containers() << container;
 }
void	TimeSeriesStart::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
TypeCreationSection::TypeCreationSection(
	int		main_index,
	parsedExp&	P_tok_creation,
	parsedExp&	P_program,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_creation(P_tok_creation),
		program(P_program)
	{
	mainIndex = main_index;
	if(tok_creation) {
		initExpression(tok_creation->getData());
	} else {
		initExpression("(null)");
	}
}
TypeCreationSection::TypeCreationSection(
	int		main_index,
	parsedExp&	P_tok_destruction,
	parsedExp&	P_program,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_destruction(P_tok_destruction),
		program(P_program)
	{
	mainIndex = main_index;
	if(tok_destruction) {
		initExpression(tok_destruction->getData());
	} else {
		initExpression("(null)");
	}
}
TypeCreationSection::TypeCreationSection(const TypeCreationSection& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.program) {
		program.reference(p.program->copy());
	}
	if(p.program_1) {
		program_1.reference(p.program_1->copy());
	}
	if(p.tok_creation) {
		tok_creation.reference(p.tok_creation->copy());
	}
	if(p.tok_destruction) {
		tok_destruction.reference(p.tok_destruction->copy());
	}
	initExpression(p.getData());
}
TypeCreationSection::TypeCreationSection(bool, const TypeCreationSection& p)
	: pE(origin_info(p.line, p.file)),
		program(p.program),
		program_1(p.program_1),
		tok_creation(p.tok_creation),
		tok_destruction(p.tok_destruction) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void TypeCreationSection::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(program) {
		program->recursively_apply(EA);
	}
	if(program_1) {
		program_1->recursively_apply(EA);
	}
	if(tok_creation) {
		tok_creation->recursively_apply(EA);
	}
	if(tok_destruction) {
		tok_destruction->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	TypeCreationSection::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	TypeCreationSection::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
Typedef::Typedef(
	int		main_index,
	parsedExp&	P_typedef_preface,
	parsedExp&	P_tok_equal_sign,
	parsedExp&	P_Expression,
	parsedExp&	P_opt_range,
	parsedExp&	P_opt_descr,
	parsedExp&	P_opt_units,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		typedef_preface(P_typedef_preface),
		tok_equal_sign(P_tok_equal_sign),
		Expression(P_Expression),
		opt_range(P_opt_range),
		opt_descr(P_opt_descr),
		opt_units(P_opt_units),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(typedef_preface) {
		initExpression(typedef_preface->getData());
	} else {
		initExpression("(null)");
	}
}
Typedef::Typedef(const Typedef& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.opt_descr) {
		opt_descr.reference(p.opt_descr->copy());
	}
	if(p.opt_range) {
		opt_range.reference(p.opt_range->copy());
	}
	if(p.opt_units) {
		opt_units.reference(p.opt_units->copy());
	}
	if(p.tok_equal_sign) {
		tok_equal_sign.reference(p.tok_equal_sign->copy());
	}
	if(p.typedef_preface) {
		typedef_preface.reference(p.typedef_preface->copy());
	}
	initExpression(p.getData());
}
Typedef::Typedef(bool, const Typedef& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		opt_descr(p.opt_descr),
		opt_range(p.opt_range),
		opt_units(p.opt_units),
		tok_equal_sign(p.tok_equal_sign),
		typedef_preface(p.typedef_preface) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Typedef::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(opt_descr) {
		opt_descr->recursively_apply(EA);
	}
	if(opt_range) {
		opt_range->recursively_apply(EA);
	}
	if(opt_units) {
		opt_units->recursively_apply(EA);
	}
	if(tok_equal_sign) {
		tok_equal_sign->recursively_apply(EA);
	}
	if(typedef_preface) {
		typedef_preface->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Typedef::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	if(!aafReader::typedefs().find(nodeData)) {
		aafReader::typedefs() << new emptySymbol(nodeData);
	}
 }
void	Typedef::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
UnaryMinus::UnaryMinus(
	int		main_index,
	parsedExp&	P_tok_minus,
	parsedExp&	P_exp_modifiable_by_unary_minus,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_minus(P_tok_minus),
		exp_modifiable_by_unary_minus(P_exp_modifiable_by_unary_minus)
	{
	mainIndex = main_index;
	if(tok_minus) {
		initExpression(tok_minus->getData());
	} else {
		initExpression("(null)");
	}
}
UnaryMinus::UnaryMinus(const UnaryMinus& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.exp_modifiable_by_unary_minus) {
		exp_modifiable_by_unary_minus.reference(p.exp_modifiable_by_unary_minus->copy());
	}
	if(p.tok_minus) {
		tok_minus.reference(p.tok_minus->copy());
	}
	initExpression(p.getData());
}
UnaryMinus::UnaryMinus(bool, const UnaryMinus& p)
	: pE(origin_info(p.line, p.file)),
		exp_modifiable_by_unary_minus(p.exp_modifiable_by_unary_minus),
		tok_minus(p.tok_minus) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void UnaryMinus::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(exp_modifiable_by_unary_minus) {
		exp_modifiable_by_unary_minus->recursively_apply(EA);
	}
	if(tok_minus) {
		tok_minus->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
	void	initExpression(
		const Cstring& nodeData);
 void	UnaryMinus::eval_expression(
		behaving_base*	local_object,
		TypedValue&	result
		) {
	TypedValue val1;
	if(unaryFunc) {
		try {
		    TypedValue val;
		    exp_modifiable_by_unary_minus->eval_expression(local_object, val);
		    unaryFunc(
			val,
			result);
		} catch(eval_error Err) {
			Cstring err;
			err << "File " << file << ", line " << line << ":\n" << Err.msg;
			throw(eval_error(err));
		}
	} else {
		exp_modifiable_by_unary_minus->eval_expression(local_object, result);
	}
 }
Usage::Usage(
	int		main_index,
	parsedExp&	P_resource_usage_with_arguments,
	parsedExp&	P_temporalSpecification,
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		resource_usage_with_arguments(P_resource_usage_with_arguments),
		temporalSpecification(P_temporalSpecification)
	{
	mainIndex = main_index;
	if(resource_usage_with_arguments) {
		initExpression(resource_usage_with_arguments->getData());
	} else {
		initExpression("(null)");
	}
}
 Usage::Usage(const Usage& p)
	: executableExp(origin_info(p.line, p.file)),
		theStorageTaskIndex(p.theStorageTaskIndex),
		consumptionIndex(p.consumptionIndex),
		theTaskIndex(p.theTaskIndex),
		theContainer(p.theContainer),
		abs_type(p.abs_type),
		usageType(p.usageType)
	{
	theData = p.getData();

	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	// if(p.temporalSpec) {
	// 	temporalSpec.reference(p.temporalSpec->copy());
	// }
	if(p.whenSpec) {
		whenSpec.reference(p.whenSpec->copy());
	}
	if(p.resource_usage_with_arguments) {
		resource_usage_with_arguments.reference(p.resource_usage_with_arguments->copy());
	}
	if(p.temporalSpecification) {
		temporalSpecification.reference(p.temporalSpecification->copy());
	}
	// if(p.cond_spec) {
	// 	cond_spec.reference(p.cond_spec->copy());
	// }
	if(p.indices) {
		MultidimIndices* mi = dynamic_cast<MultidimIndices*>(p.indices->copy());
		indices.reference(mi);
	}
	for(int k = 0; k < p.actual_arguments.size(); k++) {
		actual_arguments.push_back(parsedExp(p.actual_arguments[k]->copy()));
	}
 }
Usage::Usage(bool, const Usage& p)
	: executableExp(origin_info(p.line, p.file)),
		resource_usage_with_arguments(p.resource_usage_with_arguments),
		temporalSpecification(p.temporalSpecification) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void Usage::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(resource_usage_with_arguments) {
		resource_usage_with_arguments->recursively_apply(EA);
	}
	if(temporalSpecification) {
		temporalSpecification->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	Usage::initExpression(
		const Cstring& nodeData) {
	theData = nodeData;
	usageType = apgen::USAGE_TYPE::USE;
	theContainer = NULL;
	abs_type = NULL;
	theTaskIndex = -1;
	consumptionIndex = -1;
	theStorageTaskIndex = -1;
 }
void	Usage::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
UsageSection::UsageSection(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_usage,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_usage(P_tok_usage),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
UsageSection::UsageSection(const UsageSection& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.tok_usage) {
		tok_usage.reference(p.tok_usage->copy());
	}
	initExpression(p.getData());
}
UsageSection::UsageSection(bool, const UsageSection& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		tok_usage(p.tok_usage) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void UsageSection::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(tok_usage) {
		tok_usage->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	UsageSection::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	UsageSection::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
VarDescription::VarDescription(
	int		main_index,
	parsedExp&	P_tok_stringval,
	parsedExp&	P_ASCII_63,	// ?
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_stringval(P_tok_stringval),
		ASCII_63(P_ASCII_63)
	{
	mainIndex = main_index;
	if(tok_stringval) {
		initExpression(tok_stringval->getData());
	} else {
		initExpression("(null)");
	}
}
VarDescription::VarDescription(const VarDescription& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_63) {
		ASCII_63.reference(p.ASCII_63->copy());
	}
	if(p.tok_stringval) {
		tok_stringval.reference(p.tok_stringval->copy());
	}
	initExpression(p.getData());
}
VarDescription::VarDescription(bool, const VarDescription& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_63(p.ASCII_63),
		tok_stringval(p.tok_stringval) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void VarDescription::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_63) {
		ASCII_63->recursively_apply(EA);
	}
	if(tok_stringval) {
		tok_stringval->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	VarDescription::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	VarDescription::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
VarRange::VarRange(
	int		main_index,
	parsedExp&	P_range_list,
	parsedExp&	P_tok_range,
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		range_list(P_range_list),
		tok_range(P_tok_range)
	{
	mainIndex = main_index;
	if(range_list) {
		initExpression(range_list->getData());
	} else {
		initExpression("(null)");
	}
}
VarRange::VarRange(
	int		main_index,
	parsedExp&	P_simple_function_call,
	parsedExp&	P_tok_range,
	parsedExp&	P_tok_func,
	Differentiator_1)
	: pE(origin_info(yylineno, aafReader::current_file())),
		simple_function_call(P_simple_function_call),
		tok_range(P_tok_range),
		tok_func(P_tok_func)
	{
	mainIndex = main_index;
	if(simple_function_call) {
		initExpression(simple_function_call->getData());
	} else {
		initExpression("(null)");
	}
}
VarRange::VarRange(const VarRange& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.range_list) {
		range_list.reference(p.range_list->copy());
	}
	if(p.simple_function_call) {
		simple_function_call.reference(p.simple_function_call->copy());
	}
	if(p.tok_func) {
		tok_func.reference(p.tok_func->copy());
	}
	if(p.tok_range) {
		tok_range.reference(p.tok_range->copy());
	}
	if(p.tok_range_0) {
		tok_range_0.reference(p.tok_range_0->copy());
	}
	initExpression(p.getData());
}
VarRange::VarRange(bool, const VarRange& p)
	: pE(origin_info(p.line, p.file)),
		range_list(p.range_list),
		simple_function_call(p.simple_function_call),
		tok_func(p.tok_func),
		tok_range(p.tok_range),
		tok_range_0(p.tok_range_0) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void VarRange::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(range_list) {
		range_list->recursively_apply(EA);
	}
	if(simple_function_call) {
		simple_function_call->recursively_apply(EA);
	}
	if(tok_func) {
		tok_func->recursively_apply(EA);
	}
	if(tok_range) {
		tok_range->recursively_apply(EA);
	}
	if(tok_range_0) {
		tok_range_0->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	VarRange::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	VarRange::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
VarUnits::VarUnits(
	int		main_index,
	parsedExp&	P_tok_stringval,
	parsedExp&	P_ASCII_36,	// $
	Differentiator_0)
	: pE(origin_info(yylineno, aafReader::current_file())),
		tok_stringval(P_tok_stringval),
		ASCII_36(P_ASCII_36)
	{
	mainIndex = main_index;
	if(tok_stringval) {
		initExpression(tok_stringval->getData());
	} else {
		initExpression("(null)");
	}
}
VarUnits::VarUnits(const VarUnits& p)
	: pE(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_36) {
		ASCII_36.reference(p.ASCII_36->copy());
	}
	if(p.tok_stringval) {
		tok_stringval.reference(p.tok_stringval->copy());
	}
	initExpression(p.getData());
}
VarUnits::VarUnits(bool, const VarUnits& p)
	: pE(origin_info(p.line, p.file)),
		ASCII_36(p.ASCII_36),
		tok_stringval(p.tok_stringval) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void VarUnits::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_36) {
		ASCII_36->recursively_apply(EA);
	}
	if(tok_stringval) {
		tok_stringval->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	VarUnits::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	VarUnits::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
WaitFor::WaitFor(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_wait,
	parsedExp&	P_tok_for,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_wait(P_tok_wait),
		tok_for(P_tok_for),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
WaitFor::WaitFor(const WaitFor& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.tok_for) {
		tok_for.reference(p.tok_for->copy());
	}
	if(p.tok_wait) {
		tok_wait.reference(p.tok_wait->copy());
	}
	initExpression(p.getData());
}
WaitFor::WaitFor(bool, const WaitFor& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		tok_for(p.tok_for),
		tok_wait(p.tok_wait) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void WaitFor::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(tok_for) {
		tok_for->recursively_apply(EA);
	}
	if(tok_wait) {
		tok_wait->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	WaitFor::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	WaitFor::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
WaitUntil::WaitUntil(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_wait,
	parsedExp&	P_tok_until,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_wait(P_tok_wait),
		tok_until(P_tok_until),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
WaitUntil::WaitUntil(const WaitUntil& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.tok_until) {
		tok_until.reference(p.tok_until->copy());
	}
	if(p.tok_wait) {
		tok_wait.reference(p.tok_wait->copy());
	}
	initExpression(p.getData());
}
WaitUntil::WaitUntil(bool, const WaitUntil& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		tok_until(p.tok_until),
		tok_wait(p.tok_wait) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void WaitUntil::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(tok_until) {
		tok_until->recursively_apply(EA);
	}
	if(tok_wait) {
		tok_wait->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	WaitUntil::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	WaitUntil::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
WaitUntilRegexp::WaitUntilRegexp(
	int		main_index,
	parsedExp&	P_Expression,
	parsedExp&	P_tok_wait,
	parsedExp&	P_tok_until,
	parsedExp&	P_tok_regexp,
	parsedExp&	P_ASCII_59,	// ;
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		Expression(P_Expression),
		tok_wait(P_tok_wait),
		tok_until(P_tok_until),
		tok_regexp(P_tok_regexp),
		ASCII_59(P_ASCII_59)
	{
	mainIndex = main_index;
	if(Expression) {
		initExpression(Expression->getData());
	} else {
		initExpression("(null)");
	}
}
WaitUntilRegexp::WaitUntilRegexp(const WaitUntilRegexp& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_59) {
		ASCII_59.reference(p.ASCII_59->copy());
	}
	if(p.Expression) {
		Expression.reference(p.Expression->copy());
	}
	if(p.tok_regexp) {
		tok_regexp.reference(p.tok_regexp->copy());
	}
	if(p.tok_until) {
		tok_until.reference(p.tok_until->copy());
	}
	if(p.tok_wait) {
		tok_wait.reference(p.tok_wait->copy());
	}
	initExpression(p.getData());
}
WaitUntilRegexp::WaitUntilRegexp(bool, const WaitUntilRegexp& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_59(p.ASCII_59),
		Expression(p.Expression),
		tok_regexp(p.tok_regexp),
		tok_until(p.tok_until),
		tok_wait(p.tok_wait) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void WaitUntilRegexp::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_59) {
		ASCII_59->recursively_apply(EA);
	}
	if(Expression) {
		Expression->recursively_apply(EA);
	}
	if(tok_regexp) {
		tok_regexp->recursively_apply(EA);
	}
	if(tok_until) {
		tok_until->recursively_apply(EA);
	}
	if(tok_wait) {
		tok_wait->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
void	WaitUntilRegexp::initExpression(
			const Cstring& nodeData) {
	theData = nodeData;
}
void	WaitUntilRegexp::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
While::While(
	int		main_index,
	parsedExp&	P_while_statement_header,
	parsedExp&	P_ASCII_123,	// {
	parsedExp&	P_program,
	parsedExp&	P_ASCII_125,	// }
	Differentiator_0)
	: executableExp(origin_info(yylineno, aafReader::current_file())),
		while_statement_header(P_while_statement_header),
		ASCII_123(P_ASCII_123),
		program(P_program),
		ASCII_125(P_ASCII_125)
	{
	mainIndex = main_index;
	if(while_statement_header) {
		initExpression(while_statement_header->getData());
	} else {
		initExpression("(null)");
	}
}
While::While(const While& p)
	: executableExp(origin_info(p.line, p.file)) {
	for(int i = 0; i < p.expressions.size(); i++) {
		if(p.expressions[i]) {
			expressions.push_back(parsedExp(p.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
	if(p.ASCII_123) {
		ASCII_123.reference(p.ASCII_123->copy());
	}
	if(p.ASCII_125) {
		ASCII_125.reference(p.ASCII_125->copy());
	}
	if(p.program) {
		program.reference(p.program->copy());
	}
	if(p.while_statement_header) {
		while_statement_header.reference(p.while_statement_header->copy());
	}
	initExpression(p.getData());
}
While::While(bool, const While& p)
	: executableExp(origin_info(p.line, p.file)),
		ASCII_123(p.ASCII_123),
		ASCII_125(p.ASCII_125),
		program(p.program),
		while_statement_header(p.while_statement_header) {
	expressions = p.expressions;
	initExpression(p.getData());
}
void While::recursively_apply(exp_analyzer& EA) {
	EA.pre_analyze(this);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->recursively_apply(EA);
		}
	}
	if(ASCII_123) {
		ASCII_123->recursively_apply(EA);
	}
	if(ASCII_125) {
		ASCII_125->recursively_apply(EA);
	}
	if(program) {
		program->recursively_apply(EA);
	}
	if(while_statement_header) {
		while_statement_header->recursively_apply(EA);
	}
	EA.post_analyze(this);
}
 void	While::initExpression(
		const Cstring& nodeData) {
	theData = "While";
 }
void	While::eval_expression(
		behaving_base* loc,
		TypedValue& result) {
}
