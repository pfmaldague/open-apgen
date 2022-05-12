#ifndef _TOL_HISTORY_H_
#define  _TOL_HISTORY_H_

#include "UTL_time_base.H"
#include "templates.H"
#include "alphastring.H"

namespace tol {

class resource;

template<class V>
class history {
public:
	typedef Cntnr<alpha_time, V, resource*> flexnode;

	history()
	    : values(true) {
	}
	history(resource* R)
	    : values(true) {
	    values.Owner = R;
	}
	history(const history&) = default;

	//
	// We use synchronized tlists as a substitute for vectors,
	// so we can use find_at_or_before(n) to get the n-th element
	//
	history(resource* R,
		CTime_base T,
		const V& x)
		    : values(true) {
		values << new flexnode(T, x);
		values.Owner = R;
	}
	virtual ~history() = default;

	virtual const V&		currentval() const {
		assert_not_empty();
		return values.last_node()->payload;
	}
	virtual const CTime_base	currenttime() const {
		assert_not_empty();
		return values.last_node()->getKey().getetime();
	}
	virtual flexnode*		currentnode() const {
		assert_not_empty();
		return values.last_node();
	}
	virtual const CTime_base	earliest_time() const {
		assert_not_empty();
		return values.first_node()->getKey().getetime();
	}
	virtual flexnode*		earliest_node() const {
		assert_not_empty();
		return values.first_node();
	}
	bool empty() const {
		return !values.get_length();
	}

	virtual void assert_not_empty() const {
		if(!values.get_length()) {
		    Cstring errs;
		    errs << "empty history error";
		    throw(eval_error(errs));
		}
	}

	virtual void		append(flexnode* F) {
	    if(empty()) {
		values << F;
		return;
	    }

	    //
	    // Make sure we are appending
	    //
	    if(F->Key.getetime() < currenttime()) {
		Cstring err;
		err << "append: given time is earlier than current";
		throw(eval_error(err));
	    }
	    values << F;
	}

	tlist<alpha_time, flexnode, resource*>	values;
};

} // namespace tol

#endif /* _TOL_HISTORY_H_ */
