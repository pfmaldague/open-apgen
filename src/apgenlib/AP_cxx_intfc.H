#ifndef _AP_CXX_INTFC_H_
#define _AP_CXX_INTFC_H_

class AP_event_generator_res {
public:
	AP_event_generator_res() {}
	virtual ~AP_event_generator_res() {}

	// returns the name of the abstract resource
	virtual Cstring	get_name() = 0;
	virtual bool	has_a_modeling_section() = 0;
	virtual bool	has_a_resource_usage_section() = 0; };

class AP_event_generator_act {
public:
	AP_event_generator_act() {}
	virtual ~AP_event_generator_act() {}

	// returns the type
	virtual Cstring	get_name() = 0;
	virtual void	generate_events() throw(eval_error) = 0;
	virtual bool	has_a_decomposition_section() = 0;
	virtual bool	has_an_expansion_section() = 0;
	virtual bool	has_a_modeling_section() = 0;
	virtual bool	has_a_resource_usage_section() = 0; };

#endif /* _AP_CXX_INTFC_H_ */
