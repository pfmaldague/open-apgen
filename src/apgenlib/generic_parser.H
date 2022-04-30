#ifndef _GENERIC_PARSER_AP_
#define _GENERIC_PARSER_AP_

//
// New pure virtual class which should be a base class for
// the actual parser invoked by parse_act_plan.
//
class generic_parser {
public:
    generic_parser() = default;
    virtual ~generic_parser() = default;

    // pure virtual methods

    //
    // For many formats, plan objects are stored in an
    // array or array-like structure. The purpose of
    // this method is to make sure that the correct
    // structure is present in the string to be parsed.
    //
    virtual bool		verify_that_string_is_an_array(Cstring& errors) = 0;

    //
    // This function is the parser's workhorse.  It is
    // invoked repeatedly by parse_act_plan(), and should
    // return once per object found in the string to be
    // parsed.
    //
    virtual bool		iterate(	bool&	isAnActivity,
					bool&	isAResource,
					bool&	isAConstraint,
					int*	lineno) = 0;

    //
    // Object-specific utilities
    //
    virtual Cstring		act_get_id() = 0;
    virtual Cstring		act_get_type() = 0;
    virtual bool		can_extract_values() = 0;
    virtual void		act_get_attribute_values(tvslist&) = 0;
    virtual void		act_get_attribute_strings(pairslist&) = 0;
    virtual Cstring		act_get_name(pairtlist&) = 0;
    virtual bool		act_has_parent() = 0;
    virtual Cstring		act_get_parent(bool& abstractable, bool& abstracted) = 0;
    virtual void		act_get_parameter_values(tvslist&) = 0;
    virtual void		act_get_parameter_strings(pairtlist&) = 0;
    virtual Cstring		res_get_name() = 0;
    virtual TypedValue		res_get_value() = 0;
    virtual bool		res_has_indices(pairslist& indices) = 0;
    virtual CTime_base*		time_tag() = 0;
    virtual Cstring		parser_name() = 0;
};

#endif /* _GENERIC_PARSER_AP_ */
