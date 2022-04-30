#ifndef _MW_INTFC_H_
#define _MW_INTFC_H_

#include <C_list.H>
#include <UTL_time_base.H>

class MW_object;

class mw_obj : public bstringnode {
public:
	mw_obj() {}
	mw_obj(const Cstring &name, MW_object *m) :
		bstringnode(name), obj(m) {}
	~mw_obj() {}

	MW_object	*obj;
	static mw_obj	*find_mw_object(const Cstring &);
	void		add_external_event(const CTime_base &t, double &D);
	};

#endif /* _MW_INTFC_H_ */
