#ifndef ERROR_H
#define ERROR_H

#include "panel.H"

// Error messages are implemented as ref_nodes. The Substring member of the
// ref_node contains the error message; the ID field indicates status. The
// ID field is obtained by combining the following masks:

#define SERIOUS_ERROR			1
#define WARNING_ONLY			2
#define COMBINED_ERROR			4
#define HAS_BEEN_SENT_TO_LOG		8
#define HAS_BEEN_DISPLAYED_TO_USER	16

class client_errors: public panel
	{
public:
	client_errors();
	~client_errors();
					// Flag that indicates to the event manager that an
					// error message is being displayed:
	static int			Processing;
	static int			MoreProcessing;
	static char*			theTitle;

					// The following List should contain ref_nodes:
	static List			ErrorStack;

	static void			set_title_to(const Cstring&);

	static Cstring			get_error_message();
	static int			SeriousErrorsFound();

					// Virtual functions in panel class:
	void				apply() {} // unused:
	static void			error_cancel(Widget aaa, callback_stuff * client_data, void * ccc);
	static void			error_add(Widget aaa, callback_stuff * client_data, void * ccc);
	static void			DisplayMessage(const Cstring &theTitle, const Cstring &theMessage);
	static void			ExpectMore(); };

#endif
