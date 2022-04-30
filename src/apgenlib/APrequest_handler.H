#ifndef AP_REQ_HANDLER_H
#define AP_REQ_HANDLER_H

#include <vector>
using namespace std;

class Action_request;
typedef void (*req_handler)(Action_request *, int);

typedef apgen::RETURN_STATUS (*directive_handler)(const char *, const Cstring &, List &, Cstring &);
typedef void		(*res_layout_handler)(
					const Cstring&		layout_id,
					const vector<Cstring>&	res_names,
					const vector<int>&	res_display_heights,
					const Cstring&		source_file,
					int			line_number);
typedef void		(*act_display_horizon_handler)(
					const CTime_base&	start,
					const CTime_base&	duration);
typedef void		(*display_size_handler)(
					const int&		x,
					const int&		y);


class req_intfc {
public:
	req_intfc() {}
	~req_intfc() {}
	static req_handler	ABSTRACT_ALLhandler;
	static req_handler	ADD_RESOURCEhandler;
	static req_handler	COMMONhandler;
	static req_handler	CLOSE_ACT_DISPLAYhandler;
	static req_handler	CLOSE_RES_DISPLAYhandler;
	static req_handler	DELETE_ALL_DESCENDANTShandler;
	static req_handler	DETAIL_ALLhandler;
	static req_handler	FIND_RESOURCEhandler;
	static req_handler	NEW_ACT_DISPLAYhandler;
	static req_handler	NEW_HORIZONhandler;
	static req_handler	NEW_RES_DISPLAYhandler;
	static req_handler	OPEN_FILEhandler;
	static req_handler	PAUSEhandler;
	static req_handler	PURGEhandler;
	static req_handler	REMODELhandler;
	static req_handler	SCHEDULEhandler;
	static req_handler	UNSCHEDULEhandler;
	static req_handler	ENABLE_SCHEDULINGhandler;
	static req_handler	REMOVE_RESOURCEhandler;
	static req_handler	RESOURCE_SCROLLhandler;
	static req_handler	SAVE_FILEhandler;
	static req_handler	SELECT_RES_LEGENDhandler;

	static res_layout_handler		RES_LAYOUThandler;
	static act_display_horizon_handler	ACT_DISPLAY_HORIZONhandler;
	static display_size_handler		DISPLAY_SIZEhandler;
};
#endif /* AP_REQ_HANDLER_H */
