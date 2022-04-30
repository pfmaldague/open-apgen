#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "apDEBUG.H"
#include "RD_sys.H"
#include <ActivityInstance.H>
#include "UI_mw_draw.H"
#include <mw_intfc.H>

#define check_and_delete( x ) if( x ) { delete [] x; x = NULL; }

using namespace std;
using namespace apgen;

extern resource_data ResourceData;

static blist &all_mw_objects() {
	static blist foo(compare_function(compare_bpointernodes, false));
	return foo;
}

// GLOBALS:
static int ac = 0;
#ifdef GUI
static Arg args[20];
#endif


const int MW_object::RES_NAME_MAXPLOT = 24;	//longest plottable object name
						//  (or \n-delimited substring)

	/* NOTE: y-values were definite numbers in the original implementation. In Dec. 2000 I
	 * decided to extend this to ranges so we could start handling "fuzzy timing".
	 *
	 * Resource scrolling: for compatibility with LegendObject we need to know (1) which
	 * file this object was defined in, and (2) what the preferred height is (in pixels).
	 *
	 * We also need to handle errors. Let's emulate ACT_req and use a creation function.
	 */
MW_object*	create_an_mw_object(	const Cstring&	theResource,
					const Cstring&	filename,
					Cstring&	any_errors,
					RD_sys*		res_sis) {
	Rsource*	res_handle = NULL;
			// consumable/nonconsumable/state/abstract etc.
	MW_object*	theObject;
	Cstring		resourcename = theResource;
	Cstring		minmax;

	// debug
	// cerr << "create_mw_object: adding \"" << theResource << "\"...\n";

	res_handle = eval_intfc::FindResource(theResource);

	if(!res_handle) {	//NULL returned if no match to name (UI should prevent)
		any_errors = "create_an_mw_object: ERROR: resource_subsystem could not locate \"";
		any_errors << theResource << "\" ...\n";
		// debug
		// cerr << any_errors;
		return NULL;
	}

	/* We have to figure out the preferred height... Aha, it used to be determined by the resource_display
	 * constructor. The default height there was set to 100, so we'll use that.
	 *
	 * We should not create MW_objects, but MW_discrete or MW_continuous instances, depending on
	 * what kind of resource we are dealing with.
	 * */
	apgen::RES_CLASS	res_type = res_handle->get_class();
	if(res_type == apgen::RES_CLASS::STATE) {
		// discrete constructor needs list of states... OK
		// Also need time resolution... not computed till later?
		theObject = new MW_discrete(	resourcename,
						res_handle,
						filename,
						100,
						res_sis,
						res_handle->get_range());
	} else if(	res_type == apgen::RES_CLASS::CONSUMABLE
			|| res_type == apgen::RES_CLASS::NONCONSUMABLE
			|| res_type == apgen::RES_CLASS::SETTABLE
			|| res_type == apgen::RES_CLASS::ASSOCIATIVE
			|| res_type == apgen::RES_CLASS::INTEGRAL) {
		theObject = new MW_continuous(	resourcename,
						res_handle,
						filename,
						100,
						res_sis);
	}
	if(!res_handle->derivedResource) {
		res_handle->derivedResource = new rsource_display_ext();
	}
	if(theObject->refresh_all_internal_data(any_errors) != apgen::RETURN_STATUS::SUCCESS) {
		// debug
		// cerr << "create_an_mw_object: DONE (ERROR -- " << any_errors << " )\n";
		return NULL;
	}

	return theObject;
}

apgen::RETURN_STATUS MW_object::refresh_all_internal_data(Cstring &any_errors) {
	Rsource*	res_handle = get_resource_handle();
	tlist<alpha_time, unattached_value_node, Rsource*>	list_of_values;
	Formatting	ret;
	CTime		time_resolution;
	bool		there_are_new_values = false;

	// debug
	// cerr << get_key() << "->refresh_all_internal_data START\n";

	setStaleFlag(false);

	// make the lists are available to further processing...
	if(synchronize_data_with_resource(
				list_of_values,
				time_resolution,
				there_are_new_values,
				any_errors) != apgen::RETURN_STATUS::SUCCESS) {

		// debug
		// cerr << "refresh_all_internal_data: DONE (ERROR) -- " << any_errors << endl;

		return apgen::RETURN_STATUS::FAIL;
	}

	// from RD_sys again:
	// Fifth, update the data values and labels used in the mw_object (object
	//  instantiation just creates empty Objects).

	// debug
	// cerr << get_key() << "->refresh_all_internal_data: there_are_new_values = " << there_are_new_values << endl;
	if(there_are_new_values) {
		ret = update_data(
			list_of_values,
			theResSysParent->get_siblings()->dwtimestart(),
			0.0,
			theResSysParent->get_siblings()->dwtimespan(),
			100.0,
			any_errors);	//uses full 0-100% of display
		if(ret == UNFORMATTED) {
			any_errors << "\nMW_object::refresh_all_internal_data() ERROR: see above.";
			return apgen::RETURN_STATUS::FAIL;
		}
	}
	theResSysParent->record_time_and_duration_parameters();
	return apgen::RETURN_STATUS::SUCCESS;
}

	/* name_to_display can be different from the resource name for
	 * fuzzy resources. */
MW_object::MW_object(	const Cstring	&name_to_display,
			Rsource		*res_handle,
			const Cstring	&filename,
			int		preferred_height,
			RD_sys		*res_sis)
		// Question: what is the right key to use in the VSI for fuzzy resources?
		: vertically_scrolled_item(res_handle->name, filename, preferred_height),
		the_name_to_display(name_to_display),
		theOriginalName(name_to_display),
		norm_count(0),
		norm_x(NULL),	// normalized abscissa
		norm_y(NULL),	// normalized minimum y-value (as a percentage of range)
		have_error_high(false),
		have_warn_high(false),
		have_warn_low(false),
		have_error_low(false),
		the_form(NULL),
		the_draw_area(NULL),
		theObject(NULL),
		theResSysParent(res_sis),
		yscrollmode(YSCROLL_AUTO_WINDOW),
		my_res_version(-1) /* ,
		my_other_version(-1),
		displayedResources(compare_function(compare_bpointernodes, false)) */ {
	TypedValue	attrib_val;

	// limit length of the_name_to_display if necessary
	if(the_name_to_display.length() > RES_NAME_MAXPLOT)
		the_name_to_display = the_name_to_display(0,RES_NAME_MAXPLOT-3) + "...";

	// displayedResources << new Pointer_node(res_handle, /* res_handle */ NULL);
	displayedResources << new RESptr(res_handle, 0);
	theActualHeights << new pointer_to_siblings(res_sis->legend_display, preferred_height);

	if(get_resource_handle()->Object->defines_property("units")) {
		attrib_val = (*get_resource_handle()->Object)["units"];
		if(attrib_val.is_string()) {
			Units = "(" + attrib_val.get_string() + ")";
		}
	}
}

const Cstring &MW_object::get_theoretical_label() {
	if(!actual_text.is_defined()) {
#ifdef		SKIP_FOR_NOW

		//First, check attributes (if any) for "Units" (if any), to be appended
		//  onto the Resource Name for use by the created MW_object.
		else	//no "Units" attribute, or couldn't evaluate it, or could but not a string
#endif		/* SKIP_FOR_NOW */
			actual_text = the_name_to_display; }
	return actual_text; }

Rsource*	MW_object::get_resource_handle() {
	RESptr*	p1 = displayedResources.first_node();

	if(p1) {
		return p1->payload; }
	return NULL; }

apgen::RETURN_STATUS MW_object::synchronize_data_with_resource(
		tlist<alpha_time, unattached_value_node, Rsource*>&	list_of_values,
		CTime&								time_resolution,
		bool&								new_values_obtained,
		Cstring&							errors) {
	/* Transferred from RD_sys.C */
	Rsource*		res_handle = get_resource_handle();
	Rsource*		R;
	Formatting		ret = FORMAT_OK;
	CTime			theStart = theResSysParent->get_siblings()->dwtimestart();
	CTime			theSpan = theResSysParent->get_siblings()->dwtimespan();

	R = res_handle;


	// debug
	// cerr << "MW_object::synchronize_data_with_resources: START\n";

	/* First, get the resource values: */
	if(yscrollmode == YSCROLL_AUTO_PLAN) {
		/* KLUDGE to get Plan min,max:
		 * CTime	zero_time = CTime(0,0), kludged_duration = CTime(50 * SECS_PER_YEAR, 0);
		 * MUCH better way: ask for min and max, then add 2 nodes outside of view window. This
		 * is used when the -C option is set. Otherwise, brute force is used: the "all nodes"
		 * option automatically gets the entire history of the resource. This is inefficient
		 * but probably unavoidable if the -C option is not specified.
		 */

		try {
			if(res_handle->get_res_version() != my_res_version) {
				new_values_obtained = true;

				// debug
				// cerr << "-> synch: calling " << R->name << "->get_values w/ flag = 1\n";

				R->get_values(theStart, theSpan, list_of_values, true, my_res_version);

				// cerr << "\tgot " << list_of_values.get_length() << " value(s)\n";
			} else {
				// debug
				// cerr << "->synch: " << R->name << " has the same version ("
				// 	<< my_res_version << ", not getting new data\n";
			}
		}
		catch(eval_error Err) {
			errors = "MW_object: error in getting values;\n";
			errors << Err.msg;
			return apgen::RETURN_STATUS::FAIL;
		}
	} else {
		try {
			if(res_handle->get_res_version() != my_res_version) {
				new_values_obtained = true;

				// debug
				// cerr << "-> synch: calling " << R->name << "->get_values w/ flag = 0\n";

				R->get_values(theStart, theSpan, list_of_values, 0, my_res_version);

				// cerr << "\tgot " << list_of_values.get_length() << " value(s)\n";
			} else {
				// debug
				// cerr << "->synch: " << R->name << " has the same version ("
				// 	<< my_res_version << ", not getting new data\n";
			}
		}
		catch(eval_error Err) {
			errors = "MW_object::synchronize_data_with_resource: error in getting values;\n";
			errors << Err.msg;
			return apgen::RETURN_STATUS::FAIL;
		}
	}

	// debug
	// cout << "synchronize_data_with_resource(" << res_handle->name << "): list of nodes =\n";
	// tlist<alpha_time, unattached_value_node, Rsource*>::iterator	debug_iter(list_of_values);
	// unattached_value_node* unatt;
	// while((unatt = debug_iter())) {
	// 	cout << "\t" << unatt->Key.getetime().to_string() << " - ";
	// 	Cstring tmp;
	// 	unatt->val.print(tmp);
	// 	cout << tmp << "\n";
	// 	}

	//  Second, check whether there a (time) resolution was specified for this thing:
	time_resolution = res_handle->derivedResource->get_resolution();
	if(time_resolution > CTime(0, 0, true)) {
		// no sense in defining sub-pixel resolution:
		if(theSpan / time_resolution > MAX_BACKGROUND_PTS) {
			time_resolution = theSpan / (double) MAX_BACKGROUND_PTS;
		}

		TypedValue              V;
		CTime                   endT, startT, T;
		unattached_value_node*  vn;
		bool                    found_limits = false;

		startT = theStart - time_resolution;
		endT = theStart + theSpan + time_resolution;
		if((vn = list_of_values.first_node())) {
			if(	vn->get_error_high()
				|| vn->get_warn_high()
				|| vn->get_warn_low()
				|| vn->get_error_low()) {
				found_limits = true; }
			if(vn->Key.getetime() > startT) {
				startT = vn->Key.getetime();
			}
		}
		if((vn = list_of_values.last_node()) && vn->Key.getetime() < endT)
			endT = vn->Key.getetime();
		for(	T = startT;
			T <= endT;
			T += time_resolution) {
			if(have_error_high || have_warn_high || have_warn_low || have_error_low) {
				LimitContainer	lc;
				try {
					res_handle->evaluate_present_value(
						V,
						T,
						&lc);
				}
				catch(eval_error Err) {
					errors = "Res_sys::add_resource: evaluation error\n";
					errors << Err.msg;
				
					return apgen::RETURN_STATUS::FAIL;
				}
				list_of_values << new unattached_node_w_limits(V, res_handle->get_datatype(), T,
					lc.error_high_val,
					lc.warn_high_val,
					lc.warn_low_val,
					lc.error_low_val);
			} else {
				try {
					res_handle->evaluate_present_value(
						V,
						T);
				}
				catch(eval_error Err) {
					errors = "Res_sys::add_resource: evaluation error\n";
					errors << Err.msg;
				
					return apgen::RETURN_STATUS::FAIL;
				}
				list_of_values << new unattached_value_node(V, res_handle->get_datatype(), T);
			}
		}
	}

	// ALWAYS do this now; min/max add extra nodes:
	list_of_values.order();

	if(new_events_are_present()) {
		new_values_obtained = true;
	}

	//Fifth, update the data values and labels used in the mw_object (object
	//  instantiation just creates empty Objects).
	if(new_values_obtained) {
		ret = update_data(	list_of_values,
					theResSysParent->get_siblings()->dwtimestart(),
					0.0,
					theResSysParent->get_siblings()->dwtimespan(),
					100.0,
					errors);	//uses full 0-100% of display
		if(ret == UNFORMATTED) {
			errors << "\nMW_object::synchronize_data_with_resource() ERROR: see above.";
			return apgen::RETURN_STATUS::FAIL;
		}
	}

	// cerr << "MW_object::synchronize_data_with_resource: We are computing time_resolution and doing nothing with it...\n";

	//Sixth, if the first Resource in this Display, activate the Y-scrollbar
	//  according to the current Y-scrollbar mode, and set its parameters.
	//  Also, adjust the Y-axis according to the current Y-scrollbar mode.
	adjust_yaxis();

	/* End of stuff transferred from RD_sys.C */
	return apgen::RETURN_STATUS::SUCCESS;
}

MW_object::~MW_object() {
	bpointernode	*bp;

	if(theObject) {
		delete theObject;
		theObject = NULL;
	}

	check_and_delete(norm_x);
	check_and_delete(norm_y);
	// Looks like a memory leak, but interferes with organized widget destruction:
	// already deleted by destroy_widgets at top level.
	// if(the_form) {
	//	the_form->destroy_widgets();
	//	delete the_form; }

	// just in case the_form is defined in a base class with a non-trivial destructor...
	the_form = NULL;
	if((bp = (bpointernode *) all_mw_objects().find((void *) this))) {
		delete bp;
	}
}

	/*
	 * The purpose of this virtual method is to update data (Model) using reference to function values, plus domain
	 * start and span (and % of plotted domain those map to--usually 0%,100%)
	 */
MW_object::Formatting MW_object::update_data(
		tlist<alpha_time, unattached_value_node, Rsource*> &,
		CTime,
		double,
		CTime,
		double,
		Cstring &errs) {
	return TRUNCATED;
}

	/*
	 * The purpose of this virtual method is to lookup range (Y) for a given domain (X) value (0% to 100%), using the
	 * ACTUAL DATA within the MW_object, which produces the visible plot;
	 * lookup fails if outside data domain (i.e. Y undefined and unplotted)
	 */
apgen::RETURN_STATUS MW_object::lookup_yvalue(const double xval, double& yval) {
	int i;

	//Initialize output to reasonable default (return code indicates if found).
	yval = 0.0;

	/*
	 * Now iterate linearly through points, to get 2 points which bracket
	 * the domain value.  If get only 1 point, then outside domain, so lookup
	 * fails.  If land on exact value (!), keep cycling to get 2 points which
	 * bracket it, albeit with the left one being the exact value (this is so
	 * that the "latest" value for a given time is the one used).  Linear
	 * search logic ***should probably be replaced in the FUTURE***.
	 */

	if(norm_count < 1)		//no data in window
		return apgen::RETURN_STATUS::FAIL;

	//X values are limited to domain [0.0, 100.0]; however, need not check xval
	//  since its value is irrelevant to logic of search for bracketing values.
	//  Use an epsilon for comparing double array-values, double search-value.
	if((xval < (norm_x[0] - POS_EPSILON))		//before data starts
	 || (xval > (norm_x[norm_count-1] + POS_EPSILON))) {	//after data ends
		// debug
		// cerr << "MW_object::lookup_yvalue apgen::RETURN_STATUS::FAIL; xval = " << xval << " is outside of ["
		// 	<< norm_x[0] << " - " << POS_EPSILON << ", " << norm_x[norm_count-1]
		// 	<< " + " << POS_EPSILON << "]\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	//With norm_count values, have (norm_count-1) brackets (of which some or
	//  all may be degenerate, i.e. zero-length); find out which bracket the
	//  X falls into.
	for(i = 0; i < norm_count; i++)
		if(xval < (norm_x[i] - POS_EPSILON))
				break;
	//i==norm_count if finish loop without break; i==0 is also a boundary case;
	//  in both cases, xval does not land in ANY bracket.

	if((i > 0) && (i < norm_count)) {
		/*
		 * general case:  falls within a bracket
		 * interpolate linearly between the values (i.e. what Chart Object did)
		 */
		yval = norm_y[i-1] + (
					(norm_y[i] - norm_y[i-1]) * ((xval - norm_x[i-1]) / (norm_x[i] - norm_x[i-1]))
				   );
	} else if(	(xval >= (norm_x[norm_count-1] - POS_EPSILON))
			/* norm_count of 1+ is OK */ //special case: lands "on" final value
			&& (xval <= (norm_x[norm_count-1] + POS_EPSILON))
	       ) {
		yval = norm_y[norm_count-1];
	} else {		//does not fall within bracket or land "on" final value
		cerr << "MW_object::lookup_yvalue FAIL (surprising); i = " << i << ", count = " << norm_count << endl;
		return apgen::RETURN_STATUS::FAIL;
	}

	return apgen::RETURN_STATUS::SUCCESS;
}

//update_data() and others call this to change Y-axis limits
void MW_object::adjust_yaxis_limits(bool, double, double) {
	;
}

void MW_object::refresh_legend(Device &) {
	;
}

void MW_object::refresh_canvas(Device & , const int, const int) {}

//Absolute pan/zoom for any/all Resources with continuous numeric values
//  (derived classes' methods will "know" how/whether to do this action)
MW_object::Formatting MW_object::yabspanzoom (double, double) {
	return TRUNCATED;
}

int	MW_object::get_official_height() const {
#ifdef GUI
	if(GUI_Flag) {
		return get_vertical_size_in_pixels_in(theResSysParent->legend_display);
	}
#endif
	return preferredHeight;
}

motif_widget *MW_object::get_toggle_button_for(motif_widget *w) {
	return (motif_widget *) w->children[0];
}

void MW_objectExposeCallback(Widget, callback_stuff *callback_data, void *) {
	MW_object	*theOb = (MW_object *) callback_data->data;

	theOb->expose_callback();
}

void MW_object::expose_callback() {
	VSI_owner	*v_o = theResSysParent->legend_display;
	int		top = v_o->vertical_distance_above_item(this);
	int		H = get_vertical_size_in_pixels_in(v_o);

	plot_screen	device(	get_drawarea_widget(),
				v_o->get_this()->getwidth(),
				0, // top + RESVERUNIT,
				H - RESVERUNIT,
				USE_LEGEND_GC);
	// ::refresh_legend() better expect the global form in this case...
	refresh_legend(device); }

	/* We are given the parent widget indirectly, through the VSI_owner (V). That will give us
	 * a form on which we are to position our top-level widget, which is also a form. On it,
	 * we will install a toggle button with the resource name on it, then a DrawArea on which
	 * we will draw the ticmarks, the units, and the labels that go with the ticmarks.
	 *
	 * Assumptions regarding the client-defined arguments (a, a_count):
	 * 	- background, foreground colors have been defined
	 * 	- 'no recompute size'
	 * 	- XmNheight, XmNy both specified
	 *
	 * Addendum 1/9/2003: trying to enforce attachments on the legend forms does not work,
	 * because the internal widget logic conflicts with our attempts to control widget
	 * height. One could get rid of the form, but that messes up the object hierarchy.
	 * Solution: do not use attachments; specify XmNx, XmNy, XmNheight, XmNwidth
	 * explicitly. Also, these attributes are different for all widgets, so we do not
	 * want to pass Xt arguments to the method; we'll pass the requested height instead
	 * (the requested width is fixed, = ACTLEGNDWIDTH.)
	 */
motif_widget	*MW_object::create_scrolled_item(
			int		the_index,
			VSI_owner	*V,
			int		desired_y,
			int		desired_height) {
	DS_scroll_graph	*parent_to_use = V->get_this();
	if(GUI_Flag) {
	Dimension 	I, J;

	// Note: after experimentation, it looks like setting the height of a form is not very useful,
	// because the form adjusts as its children are inserted. Therefore, use desired_height as
	// a basis in all height calculations and hope that the form will somehow do the right thing.
	//
	// Also: recomputeSize is probably irrelevant in view of the form's behavior (it resizes
	// itself anyway), so we'll turn that off. Well, maybe not... Let's see.
	ac = 0;
	XtSetArg(args[ac ], XmNbackground,		ResourceData.grey_color)	; ac++;
	XtSetArg(args[ac ], XmNforeground,		ResourceData.black_color)	; ac++;
	XtSetArg(args[ac ], XmNresizable,		False)				; ac++;
	XtSetArg(args[ac ], XmNtopAttachment,	XmATTACH_FORM)			; ac++;
	XtSetArg(args[ac ], XmNtopOffset,		desired_y)			; ac++;
	XtSetArg(args[ac ], XmNleftAttachment,	XmATTACH_FORM)			; ac++;
	XtSetArg(args[ac ], XmNleftOffset,		0)			; ac++;
	// XtSetArg(args[ac ], XmNx,			0)				; ac++;
	// XtSetArg(args[ac ], XmNy,			desired_y)			; ac++;
	XtSetArg(args[ac ], XmNwidth,		ACTLEGNDWIDTH)			; ac++;
	XtSetArg(args[ac ], XmNheight,		desired_height)		; ac++;
	the_form = new motif_widget(Cstring("RD_legendform_") + the_index,
				xmFormWidgetClass,		parent_to_use,
				args,			ac,
				TRUE);
	// XtVaGetValues(the_form->widget, XmNheight, &J, NULL);

	ac = 0;
	XtSetArg(args[ac ], XmNbackground,	ResourceData.grey_color)	; ac++;
	XtSetArg(args[ac ], XmNforeground,	ResourceData.black_color)	; ac++;
	XtSetArg(args[ac ], XmNhighlightThickness, 0)				; ac++;
	XtSetArg(args[ac ], XmNrecomputeSize,	False)				; ac++;
	XtSetArg(args[ac ], XmNfont,		ResourceData.legend_font)	; ac++;
	XtSetArg(args[ac ], XmNwidth,		ACTLEGNDWIDTH)			; ac++;
	XtSetArg(args[ac ], XmNalignment,	XmALIGNMENT_BEGINNING)		; ac++;
	XtSetArg(args[ac ], XmNx,		0)				; ac++;
	// XtSetArg(args[ac ], XmNy,		0)				; ac++;
	XtSetArg(args[ac ], XmNtopAttachment,	XmATTACH_FORM)			; ac++;
	XtSetArg(args[ac ], XmNtopOffset,	0)				; ac++;
	XtSetArg(args[ac ], XmNleftAttachment,	XmATTACH_FORM)			; ac++;
	XtSetArg(args[ac ], XmNleftOffset,	0)				; ac++;
	XtSetArg(args[ac ], XmNrightAttachment,	XmATTACH_FORM)			; ac++;
	XtSetArg(args[ac ], XmNrightOffset,	0)				; ac++;
	XtSetArg(args[ac ], XmNbottomAttachment, XmATTACH_FORM)			; ac++;
	XtSetArg(args[ac ], XmNbottomOffset,	(desired_height - RESVERUNIT))	; ac++;
	// Name simplified at Dennis' request
	toggle_button = new motif_widget(theOriginalName + "_select",
				xmToggleButtonWidgetClass,	the_form,
				args,				ac,
				TRUE);
	toggle_button->set_label(the_name_to_display);
	XtVaGetValues(toggle_button->widget, XmNheight, &I, NULL);
	XtVaGetValues(the_form->widget, XmNheight, &J, NULL);

	ac = 0;
	XtSetArg(args[ac ], XmNbackground,	ResourceData.grey_color)	; ac++;
	XtSetArg(args[ac ], XmNforeground,	ResourceData.black_color)	; ac++;
	XtSetArg(args[ac ], XmNfont,		ResourceData.legend_font)	; ac++;
	XtSetArg(args[ac ], XmNrecomputeSize,	False)				; ac++;
	XtSetArg(args[ac ], XmNx,		0)				; ac++;
	XtSetArg(args[ac ], XmNwidth,		ACTLEGNDWIDTH)			; ac++;
	// XtSetArg(args[ac ], XmNy,		I)				; ac++;
	// XtSetArg(args[ac ], XmNheight,	desired_height - I)		; ac++;
	XtSetArg(args[ac ], XmNtopAttachment,	XmATTACH_FORM)			; ac++;
	XtSetArg(args[ac ], XmNtopOffset,	RESVERUNIT)			; ac++;
	XtSetArg(args[ac ], XmNbottomAttachment,XmATTACH_FORM)			; ac++;
	XtSetArg(args[ac ], XmNbottomOffset,	0)				; ac++;
	// Name changed at Dennis' request
	the_draw_area = new motif_widget(theOriginalName + "_drawarea",
				xmDrawingAreaWidgetClass,	the_form,
				args,				ac,
				TRUE);
	the_draw_area->add_event_handler(MW_objectExposeCallback, ExposureMask, this);
	// the_draw_area->add_event_handler(MW_objectExposeCallback, StructureNotifyMask, this);

	toggle_button->add_callback(MW_legend::resourceselected, XmNvalueChangedCallback, this);
	if(get_selection()) {
		*toggle_button = "1"; }
	}
	return the_form;
}

int MW_legend::get_minimum_scrolled_item_height() {
	return 50; }

const int MW_discrete::DISCRETE_LABEL_MINSLOT = 4;  //shortest space for label
const int MW_discrete::DISCRETE_LABEL_MAXPLOT = 26; //longest plottable label
const int MW_discrete::DISCRETE_LABEL_MAXLEN = 255; //longest possible label

MW_discrete::MW_discrete(	const Cstring&	objectName,
				Rsource*	r_handle,
				const Cstring&	fileName,
				int		prefHeight,
				RD_sys*		res_sis,
				vector<TypedValue>& range_values) // list of TypedNodes (PFM)
			: MW_object(	objectName,
					r_handle,
					fileName,
					prefHeight,
					res_sis
					) {
	int			i;
	DATA_TYPE		range_type;	// (PFM) = range_values->get_type();
	TaggedValue*		tdn;
	static char		str[DISCRETE_LABEL_MAXLEN+1];	//Lippman (2nd ed.) p. 563

	//First, initialize flags that indicate whether various types of
	//  data-range info exist.  Also squirrel away the name.
	yset = FALSE;		//NO current actual plotted minimum and span
	yupdate_set = FALSE;	//NO current minimum and span of window update
	ymodeled_set = FALSE;	//NO current minimum and span of modeled values
	//No "yrange_set" since full range is implicit in discrete_values (above).
	yincr_set = FALSE;		//NO computed major & minor tick increments

	//Next, copy values from READ-ONLY Lstarray* into the discrete_values List.
	//  Note that since 0 is at bottom, labels are assigned from bottom to top.
	//  Logic assumes values are of type DATA_TYPE::STRING or INTEGER or FLOAT (PFM 10/12) or TIME or DURATION.

	discrete_values.clear();

	for(int si = 0; si < range_values.size(); si++) {
		aoString		ostr;

		range_type = range_values[si].get_type();
		if(range_type == DATA_TYPE::STRING) {		//likeliest case
			strncpy(str, *range_values[si].get_string(), DISCRETE_LABEL_MAXLEN); //trunc. if too long
			str[DISCRETE_LABEL_MAXLEN] = '\0';
		} else {
			if(range_type == DATA_TYPE::INTEGER) {	// possible, so format as string
				ostr << Cstring(range_values[si].get_int());
			} else if(range_type == DATA_TYPE::BOOL_TYPE) {
				if(range_values[si].get_int()) {
					ostr << "TRUE";
				} else {
					ostr << "FALSE";
				}
			} else if(range_type == DATA_TYPE::FLOATING) {	// unlikely,but format as string
				ostr << Cstring(range_values[si].get_double());
			} else if(range_type == DATA_TYPE::DURATION) {
				CTime temp_time(range_values[si].get_time_or_duration());
				ostr << temp_time.to_string();
			} else if(range_type == DATA_TYPE::TIME) {
				CTime temp_time(range_values[si].get_time_or_duration());
				ostr << temp_time.to_string();
			}
			strncpy(str, ostr.str(), DISCRETE_LABEL_MAXLEN);
			str[DISCRETE_LABEL_MAXLEN] = '\0';
		}

		//full label (subject to very large string length) goes into List
		discrete_values.insert_node((Node*) (new String_node(str)));
		//any truncation (with/without ellipsis to indicate) done when use
	}
}

MW_discrete::~MW_discrete() {
	discrete_values.clear();
}


//GET various minimum+span pairs (no public methods to SET the values).
//  bool return indicates whether both values are set (and retrievable).
//  By convention, the caller allocates all reference args.
bool MW_discrete::getyminspan(double &min,  double &span) {    //actual plotted
	min  = yset ? ymin  : 0.0;
	span = yset ? yspan : 0.0;
	return yset;
}

bool MW_discrete::getyupdate_minspan(double& min, double &span) {	//last-updated
	min  = yupdate_set ? yupdate_min  : 0.0;
	span = yupdate_set ? yupdate_span : 0.0;
	return yupdate_set; }

bool MW_discrete::getymodeled_minspan(double& min,double& span) { //all modeled
	min  = ymodeled_set ? ymodeled_min  : 0.0;
	span = ymodeled_set ? ymodeled_span : 0.0;
	return ymodeled_set; }

bool MW_discrete::getyrange_minspan(double& min,double& span) { //entire range
	min  = 0.0;		//ALWAYS start at 0 for Discrete values
	span = discrete_values.get_length() - 1.0;  //(span+1) enumerated values
	return TRUE; }		//ALWAYS know the full discrete_values range

//Get(i.e. retrieve from member vars) major & minor Y-axis tick increments.
bool MW_discrete::getyincrements(double& major, double& minor) {
	//major was previously set to 0.0 if increments were not set
	major = yincr_major;
	minor = 0.0;	//ALWAYS (since unused), therefore no data member

	return yincr_set; }

//Compute from ymin & yspan, the major & minor Y-axis tick increments,
//  and save them in member variables.
bool MW_discrete::compute_yincrements(double& major, double& minor) {
	//For discrete variables, values are ALWAYS set 1.0 apart, so use that as
	//  the major increment (no matter HOW MANY discrete values); NO minor
	//  increment desired; this algorithm renders ymin & yspan irrelevant

	major = 1.0;
	minor = 0.0;		//zero means "no increment value"

	//save into member variables (NO yincr_minor)
	yincr_major = major;
	yincr_set = TRUE;

	udef_intfc::something_happened()++;
	return yincr_set;
}


/* Updates resource (Model) data using values in supplied blist (first arg.).
 * Domain start and span (and % of plotted domain those map to--usually 0%,100%)
 * are specified both as times and as percentages. Presumably, Dave Glicksberg
 * (who is the original implementor of this method) had in mind partial refresh
 * of the resource plot.
 *
 * The main output from this routine consists of the internal arrays norm_x, norm_y
 * which express (time, value) pairs as double-precision percentages of domain and range.
 *
 * Misc. notes left over from Dave's thinking:
 *
 * ***96-10-10 DSG:  prefer to convert args to window_start and window_span, and
 *   update_start and update_span (i.e. divorce percent and rescaling from this
 *   method).  Note that more rigorous arg-checking is thereby required.
 * ***96-10-15 DSG:  while that is useful/general, it is easier/faster to just
 *   have a "start" and "span", thereby forcing the update interval to coincide
 *   with the plot window.  Since update_data saves values into a data member,
 *   the logic to replace only a portion of the data is much more complicated.
 * ***96-11-15 DSG:  until engine handles arbitrary profiles intelligently, must
 *   be able to handle a case of (horrors!) diagonal between adjacent points!!!
 *   (see also MW_discrete::refresh_canvas())
 */
MW_object::Formatting MW_discrete::update_data(
					tlist<alpha_time, unattached_value_node, Rsource*>& values,
					CTime		startval,
					double		startpct,
					CTime		spanval,
					double		spanpct,
					Cstring&	any_errors) {
	int			count, first, i, nval, last_before, first_after, n;
	static char		str[DISCRETE_LABEL_MAXLEN+1];			//Lippman (2nd ed.) p. 563
	List_iterator		lidiscrete(discrete_values);
	double*			xArray = NULL;
	double*			yArray = NULL;
	CTime*			tArray = NULL;
	double			ymn, ymx;

	//Cannot reset to reasonable default for these, so just exit:
	if(spanval <= CTime(0, 0, true) || spanpct <= 0.0) {
		any_errors = "MW_discrete::update_data: DONE (ERROR - bad time interval)\n";
		return UNFORMATTED;
	}
	if(!values.get_length()) {
		if(norm_x) {
			delete [] norm_x;
		}
		if(norm_y) {
			delete [] norm_y;
		}
		norm_x = NULL;
		norm_y = NULL;
		norm_count = 0;
		return FORMAT_OK;
	}

	xArray = new double[ values.get_length() ];
	tArray = new CTime[ values.get_length() ];
	yArray = new double[ values.get_length() ];
	//For other cases, can reset reasonably (and may need to due to roundoff
	//  error in calculations of actual parameter values):
	if(startpct < 0.0)
		startpct = 0.0;
	if(startpct > 100.0)
		startpct = 100.0;
	if((startpct + spanpct) > 100.0)
		spanpct = 100.0 - startpct;

	//Logic assumes:  domain of type time; context(range) string, int,or float;
	//  NO SLOPE (deleted when converted from context_variable to unattached_value_node).
	//  This loop fetches ALL X,Y values (even if X remains constant) (12/95).

	nval = 0;	//Count ALL context values, even with same domain values(12/95)
				//DSG 97-11-20:  PFM says values is GUARANTEED in time order, so OK to
				//  use more-efficient first_node() instead of earliest_node():
	unattached_value_node*	OneValue = values.first_node();
	DATA_TYPE		context_type = OneValue->val.get_type();

	//Among blist of unattached_value_node's, could use "Node* find_latest_before(Node*)
	//  const" to find first and last nodes within the domain being updated.
	//???can we get COUNT of nodes between these??? <- new method?

	for(i = 1;; i++) {
		if(!OneValue) break;

		//Make sure have value as a string (discrete values are kept that way)
		if(context_type == DATA_TYPE::STRING) {		//likeliest case
			strncpy(str, *OneValue->val.get_string(), DISCRETE_LABEL_MAXLEN); //trunc. if too long
			str[DISCRETE_LABEL_MAXLEN] = '\0';
		} else {
			aoString		ostr;

			if(context_type == DATA_TYPE::INTEGER) {
				ostr << Cstring(OneValue->val.get_int());
			}	//possible, so format as string
			else if(context_type == DATA_TYPE::BOOL_TYPE) {
				if(OneValue->val.get_int()) {
					ostr << "TRUE";
				} else {
					ostr << "FALSE";
				}
			} else if(context_type == DATA_TYPE::FLOATING) {
				ostr << Cstring(OneValue->val.get_double());
			}	//unlikely, but format as string
			else if(context_type == DATA_TYPE::TIME) {
				ostr << OneValue->val.get_time_or_duration().to_string();
			}
			else if(context_type == DATA_TYPE::DURATION) {
				ostr << OneValue->val.get_time_or_duration().to_string();
			}
			strncpy(str, ostr.str(), DISCRETE_LABEL_MAXLEN);
			str[DISCRETE_LABEL_MAXLEN] = '\0';
		}

		//Loop thru all discrete value nodes, to find a match.  Get position of
		//  node (0-based), to use as Y-value (since labels match 0, 1, 2,...).
		List_iterator		lidiscrete(discrete_values); //Declare here so iterator
		String_node		*discretenode;		     //  is re-Constructed!!!

		// Search for which of the official discrete value represents the current value:
		n = 0;
		while((discretenode = (String_node *) lidiscrete())) {
			if(discretenode->get_key() == str)
				break;
			n++; }

		if(TRUE) {				//ALL context values (12/95)
			tArray[nval] = OneValue->Key.getetime();
			yArray[nval] = n;
			nval++; }

		//DSG 97-11-20:  PFM says values is GUARANTEED in time order, so OK to
		//  use more-efficient next_node() instead of following_node():
		OneValue = OneValue->next_node(); }

	//Then, adjust the X values to the plot window (0-100%).  Values outside
	//  the plot window (<0.0 or >100.0) are possible at this point; we mark
	//  the last point BEFORE & first point AFTER the window for the next step.
	//  ALSO, compute min and max of entire set of values (later do in-window).
	last_before = -1;	//out-of-range index
	first_after = nval;	//ditto
	for(i = 0; i < nval; i++) {
		if(tArray[i] < startval)	//overwrites until no longer TRUE
				last_before = i;
		if((first_after == nval) && (tArray[i] > (startval + spanval)))
				first_after = i;
		xArray[i] = startpct + spanpct * ((tArray[i] - startval) / spanval);
		if(i == 0)
			ymx = ymn = yArray[0];
		else {
			ymn = (yArray[i] < ymn) ? yArray[i] : ymn;
			ymx = (yArray[i] > ymx) ? yArray[i] : ymx; } }
	//Set minimum & span of Y-values, within the entire modeled set of values.
	//*** works fine for nominal case of integral ymn,ymx -- NOT for DIAGONALS!
	ymodeled_min = (int) (ymn + 0.001);
	ymodeled_span = (int) (ymx + 0.001 - ymn);
	ymodeled_set = TRUE;

	//We now have six cases (arbitrarily labeled A-F), diagramatically:
	//
	//			|<-----plot window----->|
	//		    startval		(startval+spanval)	
	//			|			|
	// A|============|      | B|=================|  | C|============|
	//	       D|=======|=======|      E|=======|=======|
	//		  F|====|=======================|====|
	//
	//    last_before	  first_after	  NOTES
	// -|---------------|---------------|--------------------------------------
	// A|  2*nval-2     |     2*nval-1  | nval>=1; NO PLOTTING NEEDED
	// B| -1	    |     2*nval-1  | nval>=1; plot all values
	// C| -1	    |  0	    | nval>=1; NO PLOTTING NEEDED
	// D| [0, 2*nval-4] |     2*nval-1  | nval>=2; use prev val at window start
	// E| -1	    | [1, 2*nval-3] | nval>=2; use prev val at window end
	// F| [0, 2*nval-4] | [1, 2*nval-3] | nval>=2; use prev val at both ends
	//
	// Window-start previous-value use overwrites element indexed last_before;
	//   window-end previous-value use overwrites element indexed first_after.
	// Due to interleaving of extra points, points indexed 2*n and 2*n-1
	//   (0 < n < nval) have the SAME X for any given n.
	//

	//Marked values are used (based upon 6 cases outlined above) to determine
	//  which portion of array (none, some, or all) is used, and whether new
	//  previous-value points must be created at the window edge(s).
	first = 0;				//nominal, i.e. start 1st point
	count = nval;				//nominal, i.e. use ALL points
	//First, mark no-plot cases:
	if((last_before == nval-1) && (first_after == nval))	//Case "A"
		count = 0;
	else if((last_before == -1) && (first_after == 0))		//Case "C"
		count = 0;
	//Second, set array start and count for plot-all case:
	else if((last_before == -1) && (first_after == nval))	//Case "B"
		;
	//Third, create prev-value window-edge point(s), and set start and count:
	else {						//Cases "D", "E", "F"
		if(last_before != -1) {			//Cases "D","F"
			//***allow for interpolation of diagonals(works fine if horizontal)
			// Create prev-value left-edge point:
			//  yArray[last_before] = yArray[last_before];
			// Create interpolated left-edge point (???could use value method):
			yArray[last_before] = yArray[last_before]
			+ ((yArray[last_before+1] - yArray[last_before]) * (xArray[last_before]
				/ (xArray[last_before] - xArray[last_before+1])));
			xArray[last_before] = 0.0;
		}
		if(first_after != nval) {		//Cases "E","F"
			//*** allow for interpolation of diagonals(works fine if horizontal)
			// Create prev-value right-edge point:
			//  yArray[first_after] = yArray[first_after-1];
			// Create interpolated right-edge point (???could use value method):
			yArray[first_after] = yArray[first_after-1]
				+ ((yArray[first_after] - yArray[first_after-1])
				  * ((100.0 - xArray[first_after-1])
				/ (xArray[first_after] - xArray[first_after-1])));
			xArray[first_after] = 100.0;
		}
		//Set array start and count for these cases:
		if(first_after == nval) {		//Case "D"
			first = last_before;
			count = first_after - first;
		}
		else if(last_before == -1)		//Case "E"
			count = first_after + 1;	//(first == 0)
		else {					//Case "F"
			first = last_before;
			count = first_after - first + 1;
		}
	}

	/* Finally, we copy the relevant portion of the data into the float arrays
	 *  (if count==0, none copied); also compute min and max Y-value here (if
	 *  count==0, min and max remain from previous, entire-array computation):
	 *
	 * Get rid of previously allocated values, and replace with new ones.  NOTE
	 *  this is slightly irregular, since normally parent class MW_object
	 *  handles NULL-initialization and deletion (but subclass allocates HERE). */
	check_and_delete(norm_x);
	check_and_delete(norm_y);
	norm_x = new double[count];
	norm_y = new double[count];
	norm_count = count;

	for(i = 0; i < count; i++) {
		norm_x[i] = xArray[first+i];
		norm_y[i] = yArray[first+i];
		if(i == 0)
			ymx = ymn = yArray[first+0];
		else {
			ymn = (yArray[first+i] < ymn) ? yArray[first+i] : ymn;
			ymx = (yArray[first+i] > ymx) ? yArray[first+i] : ymx;
		}
	}
	//Set minimum & span of Y-values, within the last-updated time window.
	if(count > 0) {
		//*** DSG 96-11-19:  HORRIBLE KLUDGE to cover for DIAGONAL lines in
		//***   current scheme (non-integral values possible at window edge!!!)
		yupdate_min = (int) (ymn + 0.001);
		// yupdate_span = (int) (ymx + 0.001 - ymn);
		yupdate_span = (int) (ymx + 0.999 - yupdate_min);
		yupdate_set = TRUE;
	} else {
		yupdate_set = FALSE;
	}

	/* Instead of C.O. stuff (delete old widgets, create new ones with new values),
	 *  time-normalized values (stored above) are displayed by refresh_canvas(). */

	//cleanup
	check_and_delete(xArray);
	check_and_delete(yArray);
	check_and_delete(tArray);

	//Note that Y-axis limits are NOT updated here -- do at RD_sys level!

	return FORMAT_OK;
}

//format a looked-up Y value, based on requirements of this class
MW_object::Formatting MW_discrete::format_yvalue(
		const double	yval,
		const int	maxchar,
		Cstring&	ystring) {
	String_node*	ynode;
	int		yint = (int) (yval + 0.5);

	ystring.undefine();

	//Must have Y-value that (rounds to) an integer between 0 and Ndiscrete-1. 
	if((yint < 0) || (yint >= discrete_values.get_length())) {
		return UNFORMATTED;
	}

	//Must have at least 4 characters (1 char + ellipsis to denote truncation).
	if(maxchar < DISCRETE_LABEL_MINSLOT) {
		return UNFORMATTED;
	}

	ynode = (String_node *) discrete_values[yint];
	ystring = ynode->get_key();

	//fit string into no more than maxchar chars (\0 not counted), by cutting
	//  off extra chars, and replacing last 3 chars with ellipsis
	if(ystring.length() > maxchar) {
		ystring = ystring(0,(maxchar-3)) + "...";
		return TRUNCATED;
	}	//slight misuse of meaning; use to indicate ellipsis

	return FORMAT_OK;
}

MW_object::Formatting MW_discrete::format_ydelta(
		const double yval,
		const int maxchar,
		Cstring &ystring) {
	// Don't know how to deal with a delta in this case...
	return UNFORMATTED;
}

//change Y-axis limits (+tick increments), as specified, with/without padding
void MW_discrete::adjust_yaxis_limits(bool do_pad, double min, double span) {
	double major, minor;
	bool have_inc;

	//Set ACTUAL plotted Y-axis minimum and span (need double not int).  This
	//  may be exact, or padded with top & bottom margins of half a discrete
	//  value, about the specified range (which may have span==0).  Span < 0
	//  should NOT occur, just set to 0 (e.g. via roundoff error in caller).
	if(do_pad) {	//use pad algorithm
		ymin  = min - 0.5;
		yspan = ((span < 0.0) ? 0.0 : span) + 1.0; }
	else {	//exact values used
		ymin = min;
		yspan = ((span < 0.0) ? 0.0 : span); }
	yset = TRUE;

	//Compute appropriate major+minor tick increments(uses just-set ymin,yspan)
	have_inc = compute_yincrements(major, minor);

	// *** ???what if Y-limits change causes axis LABELS to change width???
	}


void MW_discrete::refresh_legend(Device &device) {
	int		width, height, nmajor, i, yfirst, xnow, ynow, yprev;
	int		name_y, units_y, nu_x;
	Cstring		ystring, ustring;
	Formatting	ret;
	bool		ps;
	double		x_mul, y_mul; //multiplier(scale) factors, used to adjust unit size

	/* The logic here REQUIRES that the device be given a DrawArea, not a Form.
	 * Need to change the MW_widgetdraw constructor. */

	// (DSG) First, check device type for choice of HARDCODE "constants"!  This
	//  affects much of the remainder of this method!
	if(device.get_device_type() == POSTSCRIPT_DEVICE)
		ps = TRUE;
	else if(device.get_device_type() == MOTIF_DEVICE)
		ps = FALSE;
	else
		ps = FALSE;

	static sr_Rectangle zerect = { 0, 0, 0, 0 };
	zerect.x = 0;
	zerect.y = 0; // device.top();
	zerect.width = device.width();
	zerect.height = device.height();
	device.clear_drawing(zerect, FALSE);	//i.e. clear ENTIRE draw area

	//Use X and Y scale factors to change the fineness of (integer) units.
	//  Note that scale factors are always 1.0 for non-scalable devices.
	//  FOR NOW, ONLY PostScript devices are scalable.
	width  = height = 0;
	if(ps) {
		x_mul  = 1.0 / device.x_sc();
		width  = (int) (POS_EPSILON + (device.width()  * x_mul));
		y_mul  = 1.0 / device.y_sc();
		height = (int) (POS_EPSILON + (device.height() * y_mul)); }
	else {
		x_mul  = y_mul = 1.0;
		width  = device.width();
		height = device.height(); }

	//compute the tick marks (major ONLY for discrete) and the Y-axis line
	//*** algorithm ASSUMES yincr_major == 1.0 (ALWAYS true)
	//*** also assumes yspan extends 0.5 beyond each end value (ALWAYS true)
	if(yset)
		nmajor = (int) (yspan + 0.001);
	else
		nmajor = 0;

	sr_Segment*   segvals  = new sr_Segment[nmajor + 1]; //Y-axis + ticks

	//build Y-axis
	if(ps)
		segvals[0].x1 = segvals[0].x2 = width;   //i.e. all the way to right
	else
		segvals[0].x1 = segvals[0].x2 = width-1; //i.e. rightmost legend pixel
	segvals[0].y1 = 0;
	segvals[0].y2 = height;   // (height-1) leaves 1 unpainted pixel at bottom

	//build major ticks (if any)
	for(i = 1; i <= nmajor; i++) {	//1-based loop, since 0 is for Y-axis
		if(ps)
			segvals[i].x1 = width - (int) (x_mul * RESMAJORTICKLEN);
		else
			segvals[i].x1 = width - 1 - RESMAJORTICKLEN;
		segvals[i].x2 = width - (ps ? 0 : 1);
		segvals[i].y1 = segvals[i].y2 =
			/* device.top() + */ (int) floor(POS_EPSILON /*fix roundoff*/ +
			height * (((ymin + yspan) - floor(ymin+i)) / yspan)); }

	//good logic:  separate computing loop (above) from rendering loop (below)

	//draw axis and ticks
	device.draw_line_segments(
			segvals,	// segment array
			nmajor + 1,	// n in this array (0 seems OK)
			FALSE,		// bold?
			FALSE);	// dash?

	if(!ps) {
		sr_Segment	the_grey_line;

		the_grey_line.x1 = 0;
		the_grey_line.x2 = width - 1;
		the_grey_line.y1 = height - 1;
		the_grey_line.y2 = height - 1;

		//draw the line facing the grey line on the canvas side
		device.draw_line_segments(
			&the_grey_line,	// segment array
			1,		// 1 segment
			FALSE,		// bold?
			FALSE); }	// dash?

	//Draw major-tick labels (minor ticks NEVER labeled) (empirical,HARDCODED).
	//*** HARDCODE 12 pixels high by 6 wide for (non-prop.) font, and positions
	//Loop thru all discrete value nodes, only drawing those which are visible,
	//  and only when have enough room for the label (skip some when crowded).
	//  Draw bottom-to-top (i.e., high values of Y to low values of Y).

	//*** algorithm ASSUMES yincr_major == 1.0 (ALWAYS true)
	yfirst = (int) (ymin + 1);	//first plotted major tick
	//set previous tick height (to allow adequate spacing), starting at (1.5 *
	//  minimum separation) above the top, so max. bottom half of text clipped
	yprev = height + (ps ? (int) (y_mul * 14) : 18);
	for(	i = yfirst;
		(i < (yfirst + nmajor)) && (i < discrete_values.get_length());
		i++) {
		ynow = (ps ? (int) (y_mul * 2) : 5) /*Y-centers text w/r/t tick*/
				   + (int)floor(POS_EPSILON /*fix roundoff*/ +
				       height * ((yspan + ymin - i) / yspan));

		ret = format_yvalue((double)i, (ps ? 23 : DISCRETE_LABEL_MAXPLOT), ystring);
		if((ret != UNFORMATTED)	//FORMAT_OK, or TRUNCATED (string truncated)
		 && (ynow <= (yprev - (ps ? (int) (y_mul * 8) : 12)))
		) {	//subtracted
						// number is pixels min.vert.separation
			if(ps)
				xnow = (int) (width - (x_mul * 7)
					   - (x_mul * 4.8 * ystring.length()));
			else
				xnow = width-9 /*next to tick(was (ACTLEGNDWIDTH==200)-9==191)*/
					- (sr_get_font_width() /*font width*/ * ystring.length());
			device.draw_text(xnow, ynow, ystring());
			yprev = ynow;
		}
	}

	//Print resource name (and units, if any); truncates with ellipsis if too
	//  long; code is after tick labels so may OVERWRITE some of them!!!; crams
	//  text upwards if window is too short (empirical and HARDCODED).
	nu_x = (ps ? (int) (x_mul * 6) : 10);	//Name and Units X
	if(height > (ps ? (int) (y_mul * 32) : 36)) {	//standard
		name_y  = (ps ? (int) (y_mul * 16) : 20);
		units_y = (ps ? (int) (y_mul * 28) : 20);
	} else {		//Y-challenged
		name_y  = (ps ? (int) (y_mul * 6)  : 10);
		units_y = (ps ? (int) (y_mul * 14) : 20);
	}

	// name (PS only; screen already has toggle button label)
	if(ps) {
		ustring = get_key();
		if(ustring.is_defined()) {
			if(ustring.length() > 22) {
					// Cstring::operator(i1, i2) returns a Csubstring
					ustring = ustring(0, (22  - 3)) + "...";
			}
			device.draw_text(nu_x, name_y, ustring());
		}
	}

	// units (units are always in parentheses)
	ustring = get_units();
	if(ustring.is_defined()) {
		if(ustring.length() > (ps ? (22 - 1) : (RES_NAME_MAXPLOT - 1)))
			ustring = ustring(0, ((ps ? (22 - 1) : (RES_NAME_MAXPLOT - 1))-3)) + "...)";
		device.draw_text(nu_x, units_y, ustring());
	}
	
	delete [] segvals;
}

//***96-11-15 DSG:  until engine handles arbitrary profiles intelligently, must
//  be able to handle a case of (horrors!) diagonal between adjacent points!!!
//  (see also MW_discrete::update_data())
void MW_discrete::refresh_canvas(	Device		&device,
					const int	pages_per_display_width,
					int		page	//1-based index
					) {
	int		width, height, i, nseg;
  

	//CHECK INPUT VALUES, and return if problem:
	if((pages_per_display_width < 1) || (page < 1) || (page > pages_per_display_width)) {
		return; }

	// Change size of rectangle to reflect reality
	static sr_Rectangle zerect = { 0, 0, 0, 0 };
	zerect.x = 0;
	zerect.y = device.top();
	zerect.width = device.width();
	zerect.height = device.height();
	// Change device data so it knows the right height, width
	device.clear_drawing(zerect, FALSE);	//i.e. clear ENTIRE draw area
  
	/* Since canvas can be split across pages, increase width accordingly.  Also
	 * use X and Y scale factors to change the fineness of (integer) units.
	 * Note that scale factors are always 1.0 for non-scalable devices. */

	// POS_EPSILON -> fix roundoff

	width  = height = 0;
		// Note that device width, height are queried here
	width  = (int) (POS_EPSILON + (device.width() / device.x_sc() * pages_per_display_width));
	height = (int) (POS_EPSILON + (device.height() / device.y_sc()));
  
	//RETURN AFTER CLEARING, IF NO DATA TO DRAW:
  
	// norm_count is computed by update_data().
	if(norm_count <= 1) {	//0 means nothing to draw; shouldn't get just 1
		return; }
	//else, have 2+ points to draw, so convert to pixels and draw:
  
	sr_IntPoint*  intvals  = new sr_IntPoint[norm_count];
	sr_Segment*   segvals  = new sr_Segment[norm_count]; //BOLD HORIZONTALS
	nseg = 0;
  
	//algorithm formerly handle stepped points only (zero or infinite slopes
	//  ONLY), since this is what "discrete" implies! -- however, certain
	//  situations in profile CURRENTLY yield diagonal line, so handle it too:
	for(i = 0; i < norm_count; i++) {
		//normalized X is 0.0 to 100.0; convert to pixel values 0 to (width-1):
		intvals[i].x = (int) ((width / 100.0) * norm_x[i]);	//truncates

		/* allow for interpolation of diagonals; should not exceed ultimate
		 *  X short limits, since ranges are VERY limited for discrete case
		 *
		 * convert to pixel values 0 (at MAX of Y-range) to height-1 (at MIN):
		 * 	if(norm_y[i] < ymin)
		 * 	    intvals[i].y = height;   //want to go PAST window, so no edge line
		 *	else if(norm_y[i] > (ymin + yspan))
		 *	    intvals[i].y = -1;       //want to go PAST window, so no edge line
		 *	else */
		intvals[i].y = device.top() + (int) floor(POS_EPSILON
					+ height * (((ymin + yspan) - norm_y[i]) / yspan));
		if(	i > 0			//horz segments are between TWO points
			&& (intvals[i].x  > intvals[i-1].x)	//AND have non-zero width
			&& (intvals[i].y == intvals[i-1].y)) {	//AND are flat, not diagonal
			segvals[nseg].x1 = intvals[i-1].x;
  
			//use MAXIMUM Y of the 2 points
			segvals[nseg].y1 = ((intvals[i-1].y > intvals[i].y) ?
				intvals[i-1].y : intvals[i].y);	//if value is < 0 or >= height, gets clipped
			segvals[nseg].x2  = intvals[i].x;
			segvals[nseg].y2  = segvals[nseg].y1;
  
			nseg++; } }	//successfully drew a segment, so increment count
  
	//Adjust intvals,segvals for plotting when (pages_per_display_width > 1):
	//  only X's are affected; Y's are unchanged.
	if(pages_per_display_width > 1) {
		for(i = 0; i < norm_count; i++) {
			intvals[i].x -= ((page - 1) * (width / pages_per_display_width)); }

		for(i = 0; i < nseg; i++) {
			segvals[i].x1 -= ((page - 1) * (width / pages_per_display_width));
			segvals[i].x2 -= ((page - 1) * (width / pages_per_display_width)); } }
  
	//good logic:  separate computing loop (above) from rendering loop (below)
  
	//draw as steps (y == -1, y == height are CLIPPED)
	device.draw_lines(
		intvals,	// line array
		norm_count,	// n in this array (0 seems OK)
		FALSE,		// bold?
		FALSE);	// dash?
  
	//draw BOLD HORIZONTAL segments
	device.draw_line_segments(
		segvals,	// segment array
		nseg,		// n in this array (0 seems OK)
		TRUE,		// bold?
		FALSE);	// dash?
  
	delete [] intvals;
	delete [] segvals;
}

//Absolute pan/zoom for any/all Resources with continuous numeric values.
//  Since this is INCOMPATIBLE with DISCRETE Resources, IGNORE the request.
MW_object::Formatting MW_discrete::yabspanzoom (double, double) {
	return FORMAT_OK;
}	//Successfully ignore the request to abspanzoom

const int MW_continuous::CONTINUOUS_VALUE_MINFMT = 8;  //shortest legal format
								//  (e.g. "-1.2e-07")
const int MW_continuous::CONTINUOUS_VALUE_MAXPLOT= 24; //longest desired plot
								//  format
const int MW_continuous::CONTINUOUS_VALUE_MAXFMT = 255; //longest legal format

MW_continuous::MW_continuous(
		const Cstring	&objectName,
		Rsource		*res_handle,
		const Cstring	&filename,
		int		default_height,
		RD_sys		*res_sis) :
				MW_object(	objectName,
						res_handle,
						filename,
						default_height,
						res_sis
					 ),
				event_x(NULL),
				event_y(NULL),
				event_length(0),
				ymin(0.0),
				yspan(0.0),
				ListOfEvents(compare_function(compare_Time_nodes, true)),
				norm_error_high(NULL),
				norm_warn_high(NULL),
				norm_warn_low(NULL),
				norm_error_low(NULL),
				things_are_fuzzy(false) {
	bool		have_maximum = false, have_minimum = false;
	double		value_maximum = 0., value_minimum = 0.;

	//First, initialize flags that indicate whether various types of
	//  data-range info exist.  Also, squirrel away the name.
	yset = false;		//NO current actual plotted minimum and span
	yupdate_set = false;	//NO current minimum and span of window update
	ymodeled_set = false;	//NO current minimum and span of modeled values
	yrange_set = false;	//NO running minimum and span of modeled values
	yincr_set = false;	//NO computed major & minor tick increments

	const parsedProg& attrProgPtr(get_resource_handle()->get_attribute_program());
	if(attrProgPtr) {
		have_error_high = attrProgPtr->symbols.find("errorhigh") != attrProgPtr->symbols.end();
		have_error_low = attrProgPtr->symbols.find("errorlow") != attrProgPtr->symbols.end();
		have_warn_high = attrProgPtr->symbols.find("warninghigh") != attrProgPtr->symbols.end();
		have_warn_low = attrProgPtr->symbols.find("warninglow") != attrProgPtr->symbols.end();
	}

	//Second-and-three-quarters, check whether a Maximum and/or Minimum
	//  value(s) (as error thresholds) were specified for the Resource:

	//97-04-20 DSG:  STATE resources ignore Maximum FOR NOW
	if(res_handle->get_class() != apgen::RES_CLASS::STATE) {
		if(res_handle->Object->defines_property("maximum")) {
			have_maximum  = true;
			value_maximum = (*res_handle->Object)["maximum"].get_double();
		} else {
			have_maximum  = false;
			value_maximum = 0;
		}
		if(res_handle->Object->defines_property("minimum")) {
			have_minimum  = true;
			value_minimum = (*res_handle->Object)["minimum"].get_double();
		} else {
			have_minimum  = false;
			value_minimum = 0;
		}
	}

	ythreshold_max_set = have_maximum;	//have maximum-value threshold?
	if(ythreshold_max_set)
		ythreshold_max = value_maximum + POS_EPSILON;
	else
		ythreshold_max = 0;

	ythreshold_min_set = have_minimum;	//have minimum-value threshold?
	if(ythreshold_min_set)
		ythreshold_min = value_minimum - POS_EPSILON;
	else
		ythreshold_min = 0;

	//Gee, is this all?  ... YUP!
	}

MW_continuous::~MW_continuous() {}

class event_node: public Time_node {
public:
	CTime_base	event_time;
	event_node(const CTime_base& T, double& V) :
		event_time(T), value(V) {}
	event_node(const event_node &E) :
		Time_node(E), event_time(E.event_time), value(E.value) {}
	~event_node() {}

	Node* copy() {
		return new event_node(*this);
	}
	const Cstring& get_key() const {
		static Cstring temp;
		temp = event_time.to_string();
		return temp;
	}
	virtual void	setetime(
				const CTime_base& new_time) {
		event_time = new_time;
	}
	virtual const CTime_base& getetime() const {
		return event_time;
	}
	Node_type		node_type() const {
		return CONCRETE_TIME_NODE;
	}
	double value;
};

//GET various minimum+span pairs (no public methods to SET the values).
//  bool return indicates whether both values are set (and retrievable).
//  By convention, the caller allocates all reference args.
//4 pair variants are:  actual plotted; last-updated; all modeled; entire range
bool MW_continuous::getyminspan(double& min, double& span) {
	min  = yset ? ymin  : 0.0;
	span = yset ? yspan : 0.0;
	return yset;
}

bool MW_continuous::getyupdate_minspan(double& min,double& span) {
	min  = yupdate_set ? yupdate_min  : 0.0;
	span = yupdate_set ? yupdate_span : 0.0;
	return yupdate_set;
}

bool MW_continuous::getymodeled_minspan(double& min,double& span) {
	min  = ymodeled_set ? ymodeled_min  : 0.0;
	span = ymodeled_set ? ymodeled_span : 0.0;
	return ymodeled_set;
}

bool MW_continuous::getyrange_minspan(double& min,double& span) {
	min  = yrange_set ? yrange_min  : 0.0;
	span = yrange_set ? yrange_span : 0.0;
	return yrange_set;
}

//Get(i.e. retrieve from member vars) major & minor Y-axis tick increments.
bool MW_continuous::getyincrements(double& major, double& minor) {
	//major and minor were previously set to 0.0 if increments are not set
	major = yincr_major;
	minor = yincr_minor;

	return yincr_set;
}

//Compute from ymin & yspan, the major & minor Y-axis tick increments,
//  and save them in member variables.
bool MW_continuous::compute_yincrements(double &major, double &minor) {
	double log10span, span_exponent, span_mantissa;

				/*
				 * For continuous variables, we want to set increments according to the
				 *   following algorithm, which is based solely on the span (ymin ignored):
				 * Choose "nice" major increments:  ...,  1, 2, 5, 10, 20, 50, 100, ...
				 * Corresponding minor increments:  ..., .5, 1, 1,  5, 10, 10,  50, ...
				 * Main goal of above is to use factor-of-10 repetition (as Chart Object
				 *   did), but with added in-between values, at multiples of 2 or 2.5
				 */
	static const double one  = /*log10(1.0)*/ 0.0;//smallest possible mantissa
	static const double two  = log10(2.0);	   //mantissa cutoff #1
	static const double five = log10(5.0);	   //mantissa cutoff #2
	static const double ten  = /*log10(10.)*/ 1.0;//mantissa top limit (co #3)

				/*
				 * Use a fixed multiple of the major increment, as the factor used to
				 *   determine the cutoff between use of each major increment value.  For
				 *   example, if multiple is 4.0, then (40==4.0*10) <= yspan < (80==4.0*20)
				 *   uses major increment of 10; 80 <= yspan < (200==4.0*50) uses 20; etc.
				 *   This number (when truncated to an integer, and want >=2) is the MINIMUM
				 *   number of major (i.e. potentially-labeled) ticks visible on the Y-axis.
				 */
	static const double multiple = log10(3.0);  //actually,log10(above number)

	if(yspan > 0.0) {
		log10span = log10(yspan) - multiple;  //i.e. divide yspan by multiple
		span_exponent = floor(log10span);  //integer, corresp. to power of 10
		span_mantissa = log10span - span_exponent;	//fractional mantissa
		if(span_mantissa < two) {
			major = 1.0;
			minor = 0.5;
		}
		else if(span_mantissa < five) {
			major = 2.0;
			minor = 1.0;
		}
		else { // (span_mantissa < ten)
			major = 5.0;
			minor = 1.0;
		}
		major *= pow(10.0, span_exponent);
		minor *= pow(10.0, span_exponent);

		//save into member variables
		yincr_major = major;
		yincr_minor = minor;
		yincr_set = TRUE;
	}
	else { //zero or negative yspan shouldn't happen!!! so use zero==unspecified
		major = minor = 0.0;
		yincr_set = FALSE;
	}
	udef_intfc::something_happened()++;
	return yincr_set;
}


	/*
	 * The following method updates data (Model) using reference to function values, plus domain
	 *   start and span (and % of plotted domain those map to--usually 0%,100%)
	 * ***96-10-10 DSG:  prefer to convert args to window_start and window_span, and
	 *   update_start and update_span (i.e. divorce percent and rescaling from this
	 *   method).  Note that more rigorous arg-checking is thereby required.
	 * ***96-10-15 DSG:  while that is useful/general, it is easier/faster to just
	 *   have a "start" and "span", thereby forcing the update interval to coincide
	 *   with the plot window.  Since update_data saves values into a data member,
	 *   the logic to replace only a portion of the data is much more complicated.
	 *
	 * When everything is said and done, we must end up with norm_x and norm_y (normalized x-
	 * and y-value arrays, respectively) that will be used by RD_sys to do the actual
	 * drawing to the screen.
	 *
	 * The actual drawing is done by MW_continuous::refresh_canvas() below.
	 */
MW_object::Formatting MW_continuous::update_data(
		tlist<alpha_time, unattached_value_node, Rsource*>& values,
		CTime			startval,
		double			startpct,
		CTime			spanval,
		double			spanpct,
		Cstring&		any_errors) {
	int			count, first, i, nval, last_before, first_after, BigN;
	double			yrange_max, ymn, ymx;
	double*			xArray = NULL;
	CTime*			tArray = NULL;
	double*			doubleArray = NULL;
	double*			errorHigh = NULL;
	double*			warnHigh = NULL;
	double*			warnLow = NULL;
	double*			errorLow = NULL;
	// CTime*			timeArray = NULL;
	unattached_value_node*	curval;
	DATA_TYPE		context_type;
	CTime_base		current_time;

	// debug
	// cerr << "MW_continuous: updating data\n";

	//Cannot reset to reasonable default for these, so just exit:
	if(	(spanval <= CTime(0, 0, true)) || (spanpct <= 0.0)) {
		any_errors = "MW_continuous::update_data() error: either vanishing time span, or no values provided.";

		// debug
		// cerr << any_errors;

		return UNFORMATTED;
	}
	if(!values.get_length()) {
		if(norm_x) delete [] norm_x;
		if(norm_y) delete [] norm_y;
		if(norm_error_high) delete [] norm_error_high;
		if(norm_warn_high) delete [] norm_warn_high;
		if(norm_warn_low) delete [] norm_warn_low;
		if(norm_error_low) delete [] norm_error_low;
		norm_x = NULL;
		norm_y = NULL;
		norm_error_high = NULL;
		norm_warn_high = NULL;
		norm_warn_low = NULL;
		norm_error_low = NULL;
		norm_count = 0;

		// debug
		// cerr << "... no values; returning apgen::SUCCESS.\n";

		return FORMAT_OK;
	}

	//DSG 97-11-20:  PFM says values is GUARANTEED in time order, so OK to
	//  use more-efficient first_node() instead of earliest_node():
	curval = values.first_node();
	context_type = curval->val.get_type();

	if(		context_type	!= DATA_TYPE::INTEGER
			&& context_type	!= DATA_TYPE::BOOL_TYPE
			&& context_type	!= DATA_TYPE::FLOATING
			&& context_type	!= DATA_TYPE::TIME
			&& context_type	!= DATA_TYPE::DURATION
			) {
		any_errors = "MW_continuous::update_data() error: context is neither integer nor float.";
		return UNFORMATTED;
	}
	BigN = values.get_length();

	/* We will plot abscissas using both percentage (xArray) and time (tArray) values: */
	xArray = new double[BigN];
	tArray = new CTime[BigN];

	//For other cases, can reset reasonably (and may need to due to roundoff
	//  error in calculations of actual parameter values):
	if(startpct < 0.0) {
		startpct = 0.0;
	}
	if(startpct > 100.0) {
		startpct = 100.0;
	}
	if((startpct + spanpct) > 100.0) {
		spanpct = 100.0 - startpct;
	}

	// debug
	// cerr << "MW_continuous::update_data: list of values has " << BigN << " elements\n";

	/* Logic assumes:  domain of type time; context(range) of type float OR int;
	 * NO SLOPE (deleted when converted from context_variable to unattached_value_node).
	 *
	 * The following loop fetches ALL X and Y values (even if X stays same) from
	 * the values array and stuffs them into the appropriate array (double or time).
	 */

	nval = 0;	//Count ALL context values, even with same domain values(12/95)

	if(have_error_high) {
		errorHigh = new double[BigN];
	}
	if(have_warn_high) {
		warnHigh = new double[BigN];
	}
	if(have_warn_low) {
		warnLow = new double[BigN];
	}
	if(have_error_low) {
		errorLow = new double[BigN];
	}
	if(		context_type	== DATA_TYPE::INTEGER
			|| context_type	== DATA_TYPE::BOOL_TYPE
			|| context_type	== DATA_TYPE::FLOATING) {
		doubleArray = new double[BigN];
	} else if(	context_type	== DATA_TYPE::TIME
			|| context_type	== DATA_TYPE::DURATION) {
		doubleArray = new double[BigN];
		// timeArray = new CTime[BigN];
	} else  {
		// there may have been an error in get_values, resulting in UNDEFINED values.
		any_errors = "MW_continuous::update_data() error: the values provided have an unclear context.";
		return UNFORMATTED;
	}

	// debug
	// cerr << "\n\n";

	for(i = 1;; i++) {
		if(!curval) break;

		tArray[nval] = curval->Key.getetime();

		// debug
		// cerr << curval->Key.getetime().to_string() << " - ";

		if(context_type == DATA_TYPE::INTEGER) {
			doubleArray[nval] = curval->val.get_int();
		} else if(context_type == DATA_TYPE::FLOATING) {
			// debug
			// cerr << "  setting doubleArray[" << nval << "] to " << curval->val.get_double() << endl;

			// cout << "val: " << curval->val.get_double() << " ";
	
			doubleArray[nval] = curval->val.get_double();
		} else if(context_type == DATA_TYPE::DURATION
			|| context_type == DATA_TYPE::TIME) {
			doubleArray[nval] = curval->val.get_double();
			// timeArray[nval] = curval->val.get_time_or_duration();
		}

		if(have_error_high) {
			errorHigh[nval] = curval->get_error_high();

			// debug
			// cout << "eh: " << errorHigh[nval] << " ";
			}
		if(have_warn_high) {
			warnHigh[nval] = curval->get_warn_high();

			// debug
			// cout << "wh: " << warnHigh[nval] << " ";
			}
		if(have_warn_low) {
			warnLow[nval] = curval->get_warn_low();

			// debug
			// cout << "wl: " << warnLow[nval] << " ";
			}
		if(have_error_low) {
			errorLow[nval] = curval->get_error_low();

			// debug
			// cout << "el: " << errorLow[nval] << " ";
			}
		// debug
		// cout << "\n";
		nval++;
		/*
		 * DSG 97-11-20:  PFM says values is GUARANTEED in time order, so OK to
		 * use more-efficient next_node() instead of following_node():
		 */
		curval = curval->next_node();
	}

	/* We need to take into account any external events to be plotted alongside the
	 * continuous curves. From the code below, we see that the canonical way to convert
	 * a time value T to a double-precision percentage x is
	 *
	 * 	x = (T - startval) / spanval;
	 *
	 * As for y values, the key is to use ymin, yspan, width and height. ymin and yspan
	 * are class members, but you won't find them set anywhere within this file... the
	 * reason is that the policy for setting them is defined in RD_sys.C, which takes
	 * into account user preferences such as global vs. local windowing etc. There is
	 * a call to adjust_yaxis() in ::synchronize_data..., which is implemented in
	 * RD_sys.C. That solves the little secret of the vertical scale... */

	event_node	*en;
	if(event_x) delete event_x;
	if(event_y) delete event_y;
	event_x = NULL;
	event_y = NULL;
	event_length = ListOfEvents.get_length();

	// debug
	// cerr << "MW_continuous: event length = " << event_length << endl;
	if(event_length) {
		List_iterator	it(ListOfEvents);
		int		i = 0;

		event_x = new double[event_length];
		event_y = new double[event_length];
		while((en = (event_node *) it())) {
			/* only x is normalized to [0, 1]; y will have to be
			 * compared against ymin, ymin + yspan in the drawing routine. */
			event_x[i] = (en->getetime() - startval) / spanval;
			event_y[i] = en->value;
			// debug
			// cerr << "   i = " << i << ", event_x = " << event_x[i] << ", event_y = " << event_y[i] << endl;
			i++;
		}
	}

	/* Next, we adjust the X values to the plot window (0-100%).  Values outside
	 * the plot window (<0.0 or >100.0) are possible at this point; we mark
	 * the last point BEFORE & first point AFTER the window for the next step.
	 * ALSO, compute min and max of entire set of values (later do in-window).
	 * 96-10-16 DSG:  normalization to 0-100 range was required for Chart Object,
	 * but is still a useful way of removing the time scaling factor, so keep! */
	last_before = -1;	//out-of-range index
	first_after = nval;	//ditto
	for(i = 0; i < nval; i++) {
		if(tArray[i] < startval) {	//overwrites until no longer TRUE
				last_before = i;
		}
		if((first_after == nval) && (tArray[i] > (startval + spanval))) {
				first_after = i;
		}
		xArray[i] = startpct + spanpct * ((tArray[i] - startval) / spanval);
		if(doubleArray) {
			if(i == 0) {
				ymn = doubleArray[0];
				ymx = ymn;
			} else {
				ymn = (doubleArray[i] < ymn) ? doubleArray[i] : ymn;
				ymx = (doubleArray[i] > ymx) ? doubleArray[i] : ymx;
			}
		}
		// else if(timeArray) {
		// 	if(i == 0) {
		// 		ymx = ymn = timeArray[0].convert_to_double_use_with_caution();
		// 	} else {
		// 		ymn = (timeArray[i].convert_to_double_use_with_caution() < ymn) ?
		// 			timeArray[i].convert_to_double_use_with_caution() : ymn;
		// 		ymx = (timeArray[i].convert_to_double_use_with_caution() > ymx) ?
		// 			timeArray[i].convert_to_double_use_with_caution() : ymx;
		// 	}
		// }
	}
	//Set minimum & span of Y-values, within the entire modeled set of values.
	ymodeled_min = ymn;
	ymodeled_span = ymx - ymn;
	ymodeled_set = TRUE;
	//Also, initialize or update minimum & span of all possible Y-values.
	if(!yrange_set) {	//initialize with current modeled min & span
		yrange_min = ymodeled_min;
		yrange_span = ymodeled_span;
	} else {		//update existing values
		yrange_max = yrange_min + yrange_span;	//easier to compare mins, maxs
		yrange_min = ((ymn < yrange_min) ? ymn : yrange_min);
		yrange_max = ((ymx > yrange_max) ? ymx : yrange_max);
		yrange_span = yrange_max - yrange_min;
	}
	yrange_set = TRUE;


	/*
	 * We now have six cases (arbitrarily labeled A-F), diagramatically:
	 *
				|<-----plot window----->|
			    startval		(startval+spanval)
				|			|
	   A|============|  	| B|=================|  | C|============|
		       D|=======|=======|      E|=======|=======|
			  F|====|=======================|====|

	    last_before	  first_after	  NOTES
	 --|---------------|---------------|--------------------------------------
	 A |    2*nval-2   |     2*nval-1  | nval>=1; NO PLOTTING NEEDED
	 B |	-1	   |     2*nval-1  | nval>=1; plot all values
	 C |	-1	   |  	 0	   | nval>=1; NO PLOTTING NEEDED
	 D | [0, 2*nval-4] |     2*nval-1  | nval>=2; interpolate at window start
	 E | -1		   | [1, 2*nval-3] | nval>=2; interpolate at window end
	 F | [0, 2*nval-4] | [1, 2*nval-3] | nval>=2; interpolate at both ends

	 * Window-start interpolation overwrites element indexed last_before;
	 *   window-end interpolation overwrites element indexed first_after.
	 * Due to interleaving of extra points, points indexed 2*n and 2*n-1
	 *   (0 < n < nval) have the SAME X for any given n.
	 */

	//Marked values are used (based upon 6 cases outlined above) to determine
	//  which portion of array (none, some, or all) is used, and whether new
	//  interpolated points must be created at the window edge(s).
	first = 0;			// nominal, i.e. start 1st point
	count = nval;			// nominal, i.e. use ALL points

	//First, mark no-plot cases:
	if((last_before == count-1) && (first_after == count)) {	//Case "A"
		count = 0;
	} else if((last_before == -1) && (first_after == 0)) {		//Case "C"
		count = 0;
	//Second, set array start and count for plot-all case:
	} else if((last_before == -1) && (first_after == count)) {	//Case "B"
		;

	//Third, create interpolated window-edge point(s), and set start and count:
	} else {								//Cases "D", "E", "F"

		if(last_before != -1) {					//Cases "D","F"
			if(doubleArray) {
				double factor = xArray[last_before] / (xArray[last_before] - xArray[last_before+1]);
				//Create interpolated left-edge point (???could use value method):
				doubleArray[last_before] = doubleArray[last_before]
					+ (doubleArray[last_before+1] - doubleArray[last_before]) * factor;
				if(have_error_high) {
					errorHigh[last_before] = errorHigh[last_before]
						+ (errorHigh[last_before+1] - errorHigh[last_before]) * factor;
				}
				if(have_warn_high) {
					warnHigh[last_before] = warnHigh[last_before]
						+ (warnHigh[last_before+1] - warnHigh[last_before]) * factor;
				}
				if(have_warn_low) {
					warnLow[last_before] = warnLow[last_before]
						+ (warnLow[last_before+1] - warnLow[last_before]) * factor;
				}
				if(have_error_low) {
					errorLow[last_before] = errorLow[last_before]
						+ (errorLow[last_before+1] - errorLow[last_before]) * factor;
				}
			}
			// else {
				//Create interpolated left-edge point (???could use value method):
			// 	timeArray[last_before] = timeArray[last_before]
			// 		+ ((timeArray[last_before+1] - timeArray[last_before])
			// 		  * (xArray[last_before]
			// 		/ (xArray[last_before] - xArray[last_before+1])));
			// }
			xArray[last_before] = 0.0;
		}

		if(first_after != nval) {				//Cases "E","F"
			if(doubleArray) {
				double factor = (100.0 - xArray[first_after-1]) / (xArray[first_after] - xArray[first_after-1]);
				//Create interpolated right-edge point (???could use value method):
				doubleArray[first_after] = doubleArray[first_after-1]
					+ (doubleArray[first_after] - doubleArray[first_after-1]) * factor;
				if(have_error_high) {
					errorHigh[first_after] = errorHigh[first_after-1]
						+ (errorHigh[first_after] - errorHigh[first_after-1]) * factor;
				}
				if(have_warn_high) {
					warnHigh[first_after] = warnHigh[first_after-1]
						+ (warnHigh[first_after] - warnHigh[first_after-1]) * factor;
				}
				if(have_warn_low) {
					warnLow[first_after] = warnLow[first_after-1]
						+ (warnLow[first_after] - warnLow[first_after-1]) * factor;
				}
				if(have_error_low) {
					errorLow[first_after] = errorLow[first_after-1]
						+ (errorLow[first_after] - errorLow[first_after-1]) * factor;
				}
			}
			// else {
				//Create interpolated right-edge point (???could use value method):
			// 	timeArray[first_after] = timeArray[first_after-1]
			// 	+ ((timeArray[first_after] - timeArray[first_after-1])
			// 	  * ((100.0 - xArray[first_after-1])
			// 	/ (xArray[first_after] - xArray[first_after-1])));
			// }
			xArray[first_after] = 100.0;
		}
		//Set array start and count for these cases:
		if(first_after == nval) {				//Case "D"
			first = last_before;
			count = first_after - first;
		} else if(last_before == -1)				//Case "E"
			count = first_after + 1;	//(first == 0)
		else {							//Case "F"
			first = last_before;
			count = first_after - first + 1;
		}
	}

	//Finally, we copy the relevant portion of the data into the float arrays
	//  (if count==0, none copied); also compute min and max Y-value here (if
	//  count==0, min and max remain from previous, entire-array computation):

	//Get rid of previously allocated values, and replace with new ones.  NOTE
	//  this is slightly irregular, since normally parent class MW_object
	//  handles NULL-initialization and deletion (but subclass allocates HERE).
	check_and_delete(norm_x);
	check_and_delete(norm_y);
	check_and_delete(norm_error_high);
	check_and_delete(norm_warn_high);
	check_and_delete(norm_warn_low);
	check_and_delete(norm_error_low);

	norm_x = new double[count];
	norm_y = new double[count];
	if(have_error_high) {
		norm_error_high = new double[count];
	}
	if(have_warn_high) {
		norm_warn_high = new double[count];
	}
	if(have_warn_low) {
		norm_warn_low = new double[count];
	}
	if(have_error_low) {
		norm_error_low = new double[count];
	}
	norm_count = count;

	for(i = 0; i < count; i++) {
		norm_x[i] = xArray[first+i];
		if(doubleArray) {
			norm_y[i] = doubleArray[first+i];
			if(have_error_high) {
				norm_error_high[i] = errorHigh[first+i] - POS_EPSILON;
			}
			if(have_warn_high) {
				norm_warn_high[i] = warnHigh[first+i] - POS_EPSILON;
			}
			if(have_warn_low) {
				norm_warn_low[i] = warnLow[first+i] + POS_EPSILON;
			}
			if(have_error_low) {
				norm_error_low[i] = errorLow[first+i] + POS_EPSILON;
			}
			if(i == 0) {
				ymn = doubleArray[first+0];
				ymx = ymn;
			} else {
				ymn = (doubleArray[first+i] < ymn) ? doubleArray[first+i] : ymn;
				ymx = (doubleArray[first+i] > ymx) ? doubleArray[first+i] : ymx;
			}
		}
		// else {
		// 	norm_y[i] = timeArray[first+i].convert_to_double_use_with_caution();
		// 	if(i == 0) {
		// 		ymx = ymn = timeArray[first+0].convert_to_double_use_with_caution();
		// 	} else {
		// 		ymn = (timeArray[first+i].convert_to_double_use_with_caution() < ymn) ?
		// 			timeArray[first+i].convert_to_double_use_with_caution() : ymn;
		// 		ymx = (timeArray[first+i].convert_to_double_use_with_caution() > ymx) ?
		// 			timeArray[first+i].convert_to_double_use_with_caution() : ymx;
		// 	}
		// }
	}

	//Set minimum & span of Y-values, within the last-updated time window.
	if(count > 0) {
		yupdate_min = ymn;
		yupdate_span = ymx - ymn;
		yupdate_set = TRUE;
	} else {
		yupdate_set = FALSE;
	}

	// Pave the way for drawing auxiliary resources at the appropriate scale

	// first, iterate over any resources listed in the "Reference" attribute
	bool res_has_auxiliary = get_resource_handle()->Object->defines_property("auxiliary");
	if(res_has_auxiliary) {
		TypedValue	attrib_val = (*get_resource_handle()->Object)["auxiliary"];
		ArrayElement*	ae;
		int		numAuxRes = attrib_val.get_array().get_length();

		/* Allocate an array of structures that can each hold the values of one auxiliary resource.
		 * Each structure should have
		 *
		 * 	resource name (should stick somewhere in the legend)
		 * 	norm_x
		 * 	norm_y
		 *	norm_count
		 */

		auxResources.clear();
		for(int z = 0; z < attrib_val.get_array().get_length(); z++) {
			ae = attrib_val.get_array()[z];
			// we need to fetch a pointer to the resource
			Cstring			resName = ae->payload.get_string();
			Rsource*		res = eval_intfc::FindResource(resName);
			CTime			theStart = theResSysParent->get_siblings()->dwtimestart();
			CTime			theSpan = theResSysParent->get_siblings()->dwtimespan();
			tlist<alpha_time, unattached_value_node, Rsource*> auxvals;
			long			dummy_version = 0;
			resCurveNode*		rcn;

			res->get_values(theStart, theSpan, auxvals, true, dummy_version);
			auxResources << (rcn = new resCurveNode(resName));

			handle_one_res_plot(
				auxvals,
				startval,
				startpct,
				spanval,
				spanpct,
				rcn,
				any_errors);
		}
	}

	//Instead of C.O. stuff(delete old widgets,create new ones with new values),
	//  time-normalized values(stored above) are displayed by refresh_canvas().

	//cleanup
	check_and_delete(xArray);
	if(doubleArray) {
		delete [] doubleArray;
		doubleArray = NULL;
	}
	if(errorHigh) {
		delete [] errorHigh;
		errorHigh = NULL; }
	if(warnHigh) {
		delete [] warnHigh;
		warnHigh = NULL;
	}
	if(warnLow) {
		delete [] warnLow;
		warnLow = NULL;
	}
	if(errorLow) {
		delete [] errorLow;
		errorLow = NULL;
	}
	// check_and_delete(timeArray);
	check_and_delete(tArray);

	//Note that Y-axis limits are NOT updated here -- do at RD_sys level!

	return FORMAT_OK;
}

apgen::RETURN_STATUS MW_continuous::handle_one_res_plot(
		tlist<alpha_time, unattached_value_node, Rsource*>& values,
		CTime			startval,
		double			startpct,
		CTime			spanval,
		double			spanpct,
		resCurveNode*		rcn,
		Cstring&		any_errors) {
	int			count, first, i, nval, last_before, first_after, BigN;
	double			yrange_max, ymn, ymx;
	double*			xArray = NULL;
	CTime*			tArray = NULL;
	double*			doubleArray = NULL;
	// CTime*			timeArray = NULL;
	unattached_value_node*	curval;
	DATA_TYPE		context_type;
	CTime_base		current_time;
	double**		res_norm_x = &rcn->payload.norm_x;
	double**		res_norm_y = &rcn->payload.norm_y;
	int*			norm_count = &rcn->payload.norm_count;

	// debug
	// cerr << "MW_continuous: handle_one_res_plot\n";


	//Cannot reset to reasonable default for these, so just exit:
	if(	(spanval <= CTime(0, 0, true)) || (spanpct <= 0.0)) {
		any_errors = "MW_continuous::handle_one_res_plot() error: either vanishing time span, or no values provided.";
		return apgen::RETURN_STATUS::FAIL; }
	if(!values.get_length()) {
		*res_norm_x = NULL;
		*res_norm_y = NULL;
		*norm_count = 0;
		return apgen::RETURN_STATUS::SUCCESS; }

	//DSG 97-11-20:  PFM says values is GUARANTEED in time order, so OK to
	//  use more-efficient first_node() instead of earliest_node():
	curval = values.first_node();
	context_type = curval->val.get_type();

	if(		context_type	!= DATA_TYPE::INTEGER
			&& context_type	!= DATA_TYPE::FLOATING
			&& context_type	!= DATA_TYPE::TIME
			&& context_type	!= DATA_TYPE::DURATION
			) {
		any_errors = "MW_continuous::handle_one_res_plot() error: context is neither integer nor float.";
		return apgen::RETURN_STATUS::FAIL; }
	BigN = values.get_length();

	/* We will plot abscissas using both percentage (xArray) and time (tArray) values: */
	xArray = new double[BigN];
	tArray = new CTime[BigN];

	//For other cases, can reset reasonably (and may need to due to roundoff
	//  error in calculations of actual parameter values):
	if(startpct < 0.0) {
		startpct = 0.0; }
	if(startpct > 100.0) {
		startpct = 100.0; }
	if((startpct + spanpct) > 100.0) {
		spanpct = 100.0 - startpct; }

	// debug
	// cerr << "MW_continuous::handle_one_res_plot: list of values has " << BigN << " elements\n";

	/* Logic assumes:  domain of type time; context(range) of type float OR int;
	 * NO SLOPE (deleted when converted from context_variable to unattached_value_node).
	 *
	 * The following loop fetches ALL X and Y values (even if X stays same) from
	 * the values array and stuffs them into the appropriate array (double or time).
	 */

	nval = 0;	//Count ALL context values, even with same domain values(12/95)

	if(		context_type	== DATA_TYPE::INTEGER
			|| context_type	== DATA_TYPE::FLOATING) {
		doubleArray = new double[BigN];
	} else if(	context_type	== DATA_TYPE::TIME
			|| context_type	== DATA_TYPE::DURATION) {
		doubleArray = new double[BigN];
		// timeArray = new CTime[BigN];
	} else  {
		// there may have been an error in get_values, resulting in UNDEFINED values.
		any_errors = "MW_continuous::handle_one_res_plot() error: the values provided have an unclear context.";
		return apgen::RETURN_STATUS::FAIL; }

	for(i = 1;; i++) {
		if(!curval) break;

		tArray[nval] = curval->Key.getetime();

		if(context_type == DATA_TYPE::INTEGER) {
			doubleArray[nval] = curval->val.get_int(); }
		else if(context_type == DATA_TYPE::FLOATING) {
			// debug
			// cout << "  setting doubleArray[" << nval << "] to " << curval->val.get_double() << endl;

			// cout << "val: " << curval->val.get_double() << " ";
	
			doubleArray[nval] = curval->val.get_double();
		} else if(context_type == DATA_TYPE::DURATION
			|| context_type == DATA_TYPE::TIME) {
			doubleArray[nval] = curval->val.get_double();
		}

		nval++;
		/*
		 * DSG 97-11-20:  PFM says values is GUARANTEED in time order, so OK to
		 * use more-efficient next_node() instead of following_node():
		 */
		curval = curval->next_node(); }

	/* Next, we adjust the X values to the plot window (0-100%).  Values outside
	 * the plot window (<0.0 or >100.0) are possible at this point; we mark
	 * the last point BEFORE & first point AFTER the window for the next step.
	 * ALSO, compute min and max of entire set of values (later do in-window).
	 * 96-10-16 DSG:  normalization to 0-100 range was required for Chart Object,
	 * but is still a useful way of removing the time scaling factor, so keep! */
	last_before = -1;	//out-of-range index
	first_after = nval;	//ditto
	for(i = 0; i < nval; i++) {
		if(tArray[i] < startval) {	//overwrites until no longer TRUE
			last_before = i; }
		if((first_after == nval) && (tArray[i] > (startval + spanval))) {
			first_after = i; }
		xArray[i] = startpct + spanpct * ((tArray[i] - startval) / spanval);
		if(doubleArray) {
			if(i == 0) {
				ymn = doubleArray[0];
				ymx = ymn; }
			else {
				ymn = (doubleArray[i] < ymn) ? doubleArray[i] : ymn;
				ymx = (doubleArray[i] > ymx) ? doubleArray[i] : ymx;
			}
		}
		// else if(timeArray) {
		// 	if(i == 0) {
		// 		ymx = ymn = timeArray[0].convert_to_double_use_with_caution(); }
		// 	else {
		// 		ymn = (timeArray[i].convert_to_double_use_with_caution() < ymn) ?
		// 			timeArray[i].convert_to_double_use_with_caution() : ymn;
		// 		ymx = (timeArray[i].convert_to_double_use_with_caution() > ymx) ?
		//			timeArray[i].convert_to_double_use_with_caution() : ymx;
		//	}
		// }
	}
	/*
	 * We now have six cases (arbitrarily labeled A-F), diagramatically:
	 *
				|<-----plot window----->|
			    startval		(startval+spanval)
				|			|
	   A|============|  	| B|=================|  | C|============|
		       D|=======|=======|      E|=======|=======|
			  F|====|=======================|====|

	    last_before	  first_after	  NOTES
	 --|---------------|---------------|--------------------------------------
	 A |    2*nval-2   |     2*nval-1  | nval>=1; NO PLOTTING NEEDED
	 B |	-1	   |     2*nval-1  | nval>=1; plot all values
	 C |	-1	   |  	 0	   | nval>=1; NO PLOTTING NEEDED
	 D | [0, 2*nval-4] |     2*nval-1  | nval>=2; interpolate at window start
	 E | -1		   | [1, 2*nval-3] | nval>=2; interpolate at window end
	 F | [0, 2*nval-4] | [1, 2*nval-3] | nval>=2; interpolate at both ends

	 * Window-start interpolation overwrites element indexed last_before;
	 *   window-end interpolation overwrites element indexed first_after.
	 * Due to interleaving of extra points, points indexed 2*n and 2*n-1
	 *   (0 < n < nval) have the SAME X for any given n.
	 */

	//Marked values are used (based upon 6 cases outlined above) to determine
	//  which portion of array (none, some, or all) is used, and whether new
	//  interpolated points must be created at the window edge(s).
	first = 0;			// nominal, i.e. start 1st point
	count = nval;			// nominal, i.e. use ALL points

	//First, mark no-plot cases:
	if((last_before == count-1) && (first_after == count)) {	//Case "A"
		count = 0; }
	else if((last_before == -1) && (first_after == 0)) {		//Case "C"
		count = 0; }
	//Second, set array start and count for plot-all case:
	else if((last_before == -1) && (first_after == count)) {	//Case "B"
		; }

	//Third, create interpolated window-edge point(s), and set start and count:
	//
	else {								//Cases "D", "E", "F"

		if(last_before != -1) {					//Cases "D","F"
			if(doubleArray) {
				double factor = xArray[last_before] / (xArray[last_before] - xArray[last_before+1]);
				//Create interpolated left-edge point (???could use value method):
				doubleArray[last_before] = doubleArray[last_before]
					+ (doubleArray[last_before+1] - doubleArray[last_before]) * factor;
			}
			// else {
			// 	//Create interpolated left-edge point (???could use value method):
			// 	timeArray[last_before] = timeArray[last_before]
			// 		+ ((timeArray[last_before+1] - timeArray[last_before])
			// 		  * (xArray[last_before]
			//		/ (xArray[last_before] - xArray[last_before+1])));
			//
			// }
			xArray[last_before] = 0.0;
		}

		if(first_after != nval) {				//Cases "E","F"
			if(doubleArray) {
				double factor = (100.0 - xArray[first_after-1]) / (xArray[first_after] - xArray[first_after-1]);
				//Create interpolated right-edge point (???could use value method):
				doubleArray[first_after] = doubleArray[first_after-1]
					+ (doubleArray[first_after] - doubleArray[first_after-1]) * factor;
			}
			// else {
			// 	//Create interpolated right-edge point (???could use value method):
			// 	timeArray[first_after] = timeArray[first_after-1]
			// 	+ ((timeArray[first_after] - timeArray[first_after-1])
			// 	  * ((100.0 - xArray[first_after-1])
			//	/ (xArray[first_after] - xArray[first_after-1])));
			//
			// }
			xArray[first_after] = 100.0;
		}
		//Set array start and count for these cases:
		if(first_after == nval) {				//Case "D"
			first = last_before;
			count = first_after - first; }
		else if(last_before == -1)				//Case "E"
			count = first_after + 1;	//(first == 0)
		else {							//Case "F"
			first = last_before;
			count = first_after - first + 1; } }

	//Finally, we copy the relevant portion of the data into the float arrays
	//  (if count==0, none copied); also compute min and max Y-value here (if
	//  count==0, min and max remain from previous, entire-array computation):

	*res_norm_x = new double[count];
	*res_norm_y = new double[count];
	*norm_count = count;

	for(i = 0; i < count; i++) {
		(*res_norm_x)[i] = xArray[first+i];
		if(doubleArray) {
			(*res_norm_y)[i] = doubleArray[first+i];
		}
		// else {
		// 	(*res_norm_y)[i] = timeArray[first+i].convert_to_double_use_with_caution();
		// }
	}

	//Instead of C.O. stuff(delete old widgets,create new ones with new values),
	//  time-normalized values(stored above) are displayed by refresh_canvas().

	//cleanup
	check_and_delete(xArray);
	check_and_delete(doubleArray);
	// check_and_delete(timeArray);
	check_and_delete(tArray);

	return apgen::RETURN_STATUS::SUCCESS;
}

//format a looked-up Y value, based on requirements of this class
MW_object::Formatting MW_continuous::format_yvalue(const double yval, const int maxchar, Cstring& ystring) {
	static char		str[CONTINUOUS_VALUE_MAXFMT+1];
	int			i;
	DATA_TYPE		datatype = get_resource_handle()->get_datatype();

	ystring.undefine();

	if(maxchar < CONTINUOUS_VALUE_MINFMT) {
		return UNFORMATTED;
	}

	for(	i = DOUBLE_DIG;
		i >= 2;
		i--) {
		if(datatype == DATA_TYPE::FLOATING || datatype == DATA_TYPE::INTEGER) {
			ystring = Cstring(yval, i - 1);
		} else if(datatype == DATA_TYPE::DURATION) {
			ystring = CTime_base::convert_from_double_use_with_caution(yval, true).to_string();
		} else if(datatype == DATA_TYPE::TIME) {
			ystring = CTime_base::convert_from_double_use_with_caution(yval, false).to_string();
		}
		if(ystring.length() <= maxchar) {
			break;
		} else {
			ystring.undefine();
		}
	}
	if(i == 1) {	//can't fit in specified size, without 1+ significant digits
		return UNFORMATTED;
	}

	return FORMAT_OK;
}

//format a looked-up Y delta, based on requirements of this class
MW_object::Formatting MW_continuous::format_ydelta(const double ydel, const int maxchar, Cstring& ystring) {
	static char		str[CONTINUOUS_VALUE_MAXFMT+1];
	int			i;
	DATA_TYPE		datatype = get_resource_handle()->get_datatype();

	ystring.undefine();

	if(maxchar < CONTINUOUS_VALUE_MINFMT) {
		return UNFORMATTED;
	}

	for(	i = DOUBLE_DIG;	//significant digits in double (int arithmetic)
		i >= 2;
		i--) {
		if(datatype == DATA_TYPE::FLOATING || datatype == DATA_TYPE::INTEGER) {
			ystring = Cstring(ydel, i - 1);
		} else if(datatype == DATA_TYPE::DURATION) {
			ystring = CTime_base::convert_from_double_use_with_caution(ydel, true).to_string();
		} else if(datatype == DATA_TYPE::TIME) {
			ystring = CTime_base::convert_from_double_use_with_caution(ydel, true).to_string();
		}
		if(ystring.length() <= maxchar) {
			break;
		} else {
			ystring.undefine();
		}
	}
	if(i == 1) {	//can't fit in specified size, without 1+ significant digits
		return UNFORMATTED;
	}

	return FORMAT_OK;
}

//change Y-axis limits (+tick increments), as specified, with/without padding
void MW_continuous::adjust_yaxis_limits(bool do_pad, double min, double span) {
	double		major, minor;
	bool		have_inc;

	//Set ACTUAL plotted Y-axis minimum and span (need double not int).  This
	//  may be exact, or padded with top & bottom margins about the specified
	//  range (the logic handles virtually-zero spans differently).  Span < 0
	//  should NOT occur, just set to 0 (e.g. via roundoff error in caller).
	if(do_pad) {	//use pad algorithm
		//First, need to handle range that is (virtually) all the same value.
		if(span < POS_EPSILON) { //???refine algorithm for valid SMALL values
			if(min > 0.0) {	//range of 20% of abs value, centered on value
				ymin  = 0.9 * min;
				yspan = 0.2 * min; }
			else if(min < 0.0) {	//range of 20% of abs value, centered on value
				ymin  = 1.1 * min;
				// fixes FR 1.1-23
				// yspan = 0.2 * min;
				yspan = -0.2 * min; }
			else {		//exactly 0.0, so do arbitrary centered range
				ymin  = -1.0;
				yspan =  2.0; } }

		//Otherwise, want to provide some margins (say, 5% of max-min height)
		//  about the specified range, for visibility [but no way to adjust
		//  this based on actual height in pixels, of the graph on the screen].
		else {
			ymin  = min - (0.05 * span);
			yspan = 1.10 * span; } }
	else {	//exact values used
		ymin = min;
		yspan = ((span < 0.0) ? 0.0 : span); }
	yset = TRUE;

	//Compute appropriate major+minor tick increments(uses just-set ymin,yspan)
	have_inc = compute_yincrements(major, minor);

	// *** ???what if Y-limits change causes axis LABELS to change width???
	}


void MW_continuous::refresh_legend(Device &device) {
	int		width, height, i, nmajor, nminor, xnow, ynow, yprev;
	int		units_y, nu_x, name_y;
	double		lown, lowmajor, highn, highmajor, major, lowminor, highminor, minor;
	Cstring		ystring, ustring;
	Formatting	ret;
	bool		ps;
	double		x_mul, y_mul; //multiplier(scale) factors, used to adjust unit size

	//First, check device type for choice of HARDCODE "constants"!  This
	//  affects much of the remainder of this method!
	if(device.get_device_type() == POSTSCRIPT_DEVICE)
		ps = TRUE;
	else if(device.get_device_type() == MOTIF_DEVICE)
		ps = FALSE;
	else
		ps = FALSE;

	static sr_Rectangle zerect = { 0, 0, 0, 0 };
	device.clear_drawing(zerect, FALSE);	//i.e. clear ENTIRE draw area

	//Use X and Y scale factors to change the fineness of (integer) units.
	//  Note that scale factors are always 1.0 for non-scalable devices.
	//  FOR NOW, ONLY PostScript devices are scalable.
	width  = height = 0;
	if(ps) {
		x_mul  = 1.0 / device.x_sc();
		width  = (int) (POS_EPSILON /*fix roundoff*/
				+ (device.width()  * x_mul));
		y_mul  = 1.0 / device.y_sc();
		height = (int) (POS_EPSILON /*fix roundoff*/
				+ (device.height() * y_mul)); }
	else {
		x_mul  = y_mul = 1.0;
		width  = device.width();
		height = device.height(); }

	//All logic below should handle N==0 cases!!!

	//count major ticks, and find lowest (bottom) and highest (top) tick values
	nmajor = 0;
	if(yincr_set) {
		lown  = (ceil)(ymin / yincr_major);
		lowmajor  = yincr_major * lown;
		highn = (floor)((ymin + yspan) / yincr_major);
		highmajor = yincr_major * highn;
		nmajor = (int) (highn - lown + 1.5); }

	//count minor ticks, and find lowest (bottom) and highest (top) tick values
	//  (this logic is the same as for major ticks; will duplicate major ticks)
	nminor = 0;
	if(yincr_set) {
		lown  = (ceil)(ymin / yincr_minor);
		lowminor  = yincr_minor * lown;
		highn = (floor)((ymin + yspan) / yincr_minor);
		highminor = yincr_minor * highn;
		nminor = (int) (highn - lown + 1.5); }

	sr_Segment			*segvals = new sr_Segment[nmajor+nminor+1]; //Y-axis + ticks

	//build Y-axis
	if(ps)
		segvals[0].x1 = segvals[0].x2 = width;   //i.e. all the way to right
	else
		segvals[0].x1 = segvals[0].x2 = width-1; //i.e. rightmost legend pixel
	segvals[0].y1 = 0;
	segvals[0].y2 = height;   // (height-1) leaves 1 unpainted pixel at bottom

	//build minor ticks (if any) -- first so duplicate major ticks overwrite
	for(i = 1, minor = lowminor;
		 i <= nminor;
		 i++, minor = lowminor + ((i - 1) * yincr_minor)) {
		if(ps)
				segvals[i].x1 = width - (int) (x_mul * RESMINORTICKLEN);
		else
				segvals[i].x1 = width - 1 - RESMINORTICKLEN;
		segvals[i].x2 = width - (ps ? 0 : 1);
		segvals[i].y1 = segvals[i].y2 = (int) floor(POS_EPSILON +
				height * (((ymin + yspan) - minor) / yspan)); }

	//build major ticks (if any) -- second so overwrites duplicate minor ticks
	for(i = 1 + nminor, major = lowmajor;
		 i <= nminor + nmajor;
		 i++, major = lowmajor + ((i - (1 + nminor)) * yincr_major)) {
		if(ps)
			segvals[i].x1 = width - (int) (x_mul * RESMAJORTICKLEN);
		else
			segvals[i].x1 = width - 1 - RESMAJORTICKLEN;
		segvals[i].x2 = width - (ps ? 0 : 1);
		segvals[i].y1 = segvals[i].y2 =
			 (int) floor(POS_EPSILON + height * (((ymin + yspan) - major) / yspan)); }

	//good logic:  separate computing loop (above) from rendering loop (below)

	//draw axis and ticks
	device.draw_line_segments(
		segvals,	// segment array
		1 + nminor + nmajor,	// n in this array (0 seems OK)
		FALSE,		// bold?
		FALSE);	// dash?

	//Draw major-tick labels (minor ticks NEVER labeled) (empirical,HARDCODED).
	//*** HARDCODE 12 pixels high by 6 wide for (non-prop.) font, and positions
	//Loop to compute and format values, only drawing those which are visible,
	//  and only when have enough room for the label (skip some when crowded).
	//  Draw bottom-to-top (i.e., high values of Y to low values of Y).

	//set previous tick height (to allow adequate spacing), starting at (1.5 *
	//  minimum separation) above the top, so max. bottom half of text clipped
	yprev = height + (ps ? (int) (y_mul * 14) : 18);
	for(i = 0; i < nmajor; i++) {
		major = lowmajor + (i * yincr_major);
		ynow = (ps ? (int) (y_mul * 2) : 5) /*Y-centers text w/r/t tick*/
				   + (int)floor(POS_EPSILON /*fix roundoff*/ +
				   height * (((ymin + yspan) - major) / yspan));

		ret = format_yvalue(major, CONTINUOUS_VALUE_MAXPLOT, ystring);
		if(
			(ret != UNFORMATTED)	//FORMAT_OK, or TRUNCATED (string truncated)
			&& ynow <= (yprev - (ps ? (int) (y_mul * 8) : 12))
		) {    //subtracted
						// number is pixels min.vert.separation
			if(ps)
				xnow = (int) (width - (x_mul * 7) - (x_mul * 4.8 * ystring.length()));
			else
				xnow = width-9 /*next to tick(was (ACTLEGNDWIDTH==200)-9==191)*/
					- (sr_get_font_width() /*font width*/ * ystring.length());
			device.draw_text(xnow, ynow, ystring());
			yprev = ynow; } }

	if(!ps) {
		sr_Segment	the_grey_line;

		the_grey_line.x1 = 0;
		the_grey_line.x2 = width - 1;
		the_grey_line.y1 = height - 1;
		the_grey_line.y2 = height - 1;
		//draw the line facing the grey line on the canvas side
		device.draw_line_segments(
			&the_grey_line,	// segment array
			1,		// 1 segment
			FALSE,		// bold?
			FALSE); }	// dash?

	//print resource name (and units, if any); truncates with ellipsis if too
	//  long; code is after tick labels so may OVERWRITE some of them!!!; crams
	//  text upwards if window is too short (empirical and HARDCODED)
	nu_x = (ps ? (int) (x_mul * 6) : 10);	//Name and Units X

	if(height > (ps ? (int) (y_mul * 32) : 36)) {	//standard
		name_y  = (ps ? (int) (y_mul * 16) : 20);
		units_y = (ps ? (int) (y_mul * 28) : 20); }
	else {		//Y-challenged
		name_y  = (ps ? (int) (y_mul * 6)  : 10);
		units_y = (ps ? (int) (y_mul * 14) : 20); }

	// draw name (PS only; sceen already has the toggle button label)
	if(ps) {
		ustring = get_key();
		if(ustring.is_defined()) {
			if(ustring.length() > 22) {
					// Cstring::operator(i1, i2) returns a Csubstring
					ustring = ustring(0, (22 - 3)) + "..."; }
			device.draw_text(nu_x, name_y, ustring()); } }

	// units (units are always in parentheses)
	ustring = get_units();
	if(ustring.is_defined()) {
		if(ustring.length() > (ps ? (22 - 1) : (RES_NAME_MAXPLOT - 1)))
			ustring = ustring(0, ((ps ? (22 - 1) : (RES_NAME_MAXPLOT - 1))-3)) + "...)";
		device.draw_text(nu_x, units_y, ustring()); }

	delete [] segvals;
}

	// Careful: 1-based index
void	MW_continuous::refresh_canvas(
		Device		&device,
		const int	pages_per_display_width,
		const int	page) {
	int		width, height;
	double		lown, lowmajor, highn, highmajor, major;
	bool		draw_ythreshold_max, draw_ythreshold_min;
	int		nmajor;

	int		maxValue = 0;	// to determine cutoff values if min and max values are in force.
	int		minValue = 100000;	// note that the coordinate system is reversed in y, so min > max

	//CHECK INPUT VALUES, and return if problem:
	if((pages_per_display_width < 1)
	  || (page < 1)
	  || (page > pages_per_display_width)) {
		return; }

		// Change this to the 'real' coordinates
	static sr_Rectangle zerect = { 0, 0, 0, 0 };
	zerect.x = 0;
	zerect.y = device.top();
	zerect.width = device.width();
	zerect.height = device.height();

	// debug
	// cerr << "MW_continuous::refresh_canvas: calling clear drawing; norm_count = " << norm_count << "\n";
	device.clear_drawing(zerect, FALSE);	//i.e. clear ENTIRE draw area

	/* Since canvas can be split across pages, increase width accordingly.  Also
	 *  use X and Y scale factors to change the fineness of (integer) units.
	 *  Note that scale factors are always 1.0 for non-scalable devices. */

	// NOTE: device height, width is mentioned here.

	width  = (int) (POS_EPSILON /*fix roundoff*/
			+ (device.width() / device.x_sc() * pages_per_display_width));
	height = (int) (POS_EPSILON /*fix roundoff*/
			+ (device.height() / device.y_sc()));

	//COMPUTE AND RENDER HORIZONTAL DASHED LINES (present even if no data), and
	//COMPUTE AND RENDER optional HORIZONTAL SOLID LINE (for "Maximum"), and/or
	//COMPUTE AND RENDER optional HORIZONTAL SOLID LINE (for "Minimum")

	//count major ticks, and find lowest (bottom) and highest (top) tick values
	nmajor = 0;
	if(yincr_set) {
		lown  = (ceil)(ymin / yincr_major);
		lowmajor  = yincr_major * lown;
		highn = (floor)((ymin + yspan) / yincr_major);
		highmajor = yincr_major * highn;
		nmajor = (int) (highn - lown + 1.5); }

	//create dashed horizontal lines at major tick intervals; also, if have
	//  maximum-value and/or minimum-value threshold(s) visible, draw it/them
	//  as solid horizontal line(s)
	if(ythreshold_max_set && (ythreshold_max >= ymin) && (ythreshold_max <= ymin + yspan))
		draw_ythreshold_max = TRUE;
	else
		draw_ythreshold_max = FALSE;

	if(ythreshold_min_set && (ythreshold_min >= ymin) && (ythreshold_min <= ymin + yspan))
		draw_ythreshold_min = TRUE;
	else
		draw_ythreshold_min = FALSE;
	
	sr_Segment *segvals = new sr_Segment[nmajor + (draw_ythreshold_max ? 1 : 0)
		+ (draw_ythreshold_min ? 1 : 0)];
	int	i;

	for(i = 0, major = lowmajor; i < nmajor; i++, major = lowmajor + (i * yincr_major)) {
		segvals[i].x1 = 0;
		segvals[i].x2 = width;
		segvals[i].y1 = segvals[i].y2 =
		  	device.top() + (int)floor(POS_EPSILON /*fix roundoff*/
				+ height * (((ymin + yspan) - major) / yspan)); }

	if(draw_ythreshold_max) {
		segvals[i].x1 = 0;
		segvals[i].x2 = width;
		segvals[i].y1 = segvals[i].y2 = maxValue =
			device.top() + (int) floor(POS_EPSILON /*fix roundoff*/
				+ height * (((ymin + yspan) - ythreshold_max) / yspan));
		i++;
		}
	
	if(draw_ythreshold_min) {
		segvals[i].x1 = 0;
		segvals[i].x2 = width;
		segvals[i].y1 = segvals[i].y2 = minValue =
			device.top() + (int) floor(POS_EPSILON /*fix roundoff*/
				+ height * (((ymin + yspan) - ythreshold_min) / yspan));
	}
	
	//good logic:  separate computing loop (above) from rendering loop (below)

	//draw horizontal dashed line at each major tick
	device.draw_line_segments(
			segvals,	// segment array
			nmajor,		// n in this array (0 seems OK)
			FALSE,		// bold?
			TRUE);	// dash?

	//draw horizontal solid line(s), if called for
	i = nmajor;
	if(draw_ythreshold_max)
		device.draw_line_segments(segvals + i++, 1, FALSE, FALSE);
	if(draw_ythreshold_min)
		device.draw_line_segments(segvals + i, 1, FALSE, FALSE);

	//RETURN AFTER CLEARING AND DRAWING DASHED LINES, IF NO DATA TO DRAW:

	// norm_count is computed by update_data().
	if(norm_count <= 1) {	//0 means nothing to draw; shouldn't get just 1
		delete [] segvals;
		return; }

	//else, have 2+ points to draw, so convert to pixels and draw:
	
	draw_one_curve_and_fill(
		norm_count,
		width,
		height,
		pages_per_display_width,
		page,
		device,
		minValue,
		maxValue,
		SOLID);

	// now that we have done the basic work, we should overlay auxiliary resources

	slist<alpha_string, resCurveNode>::iterator	iter(auxResources);
	resCurveNode*					rcn;

	while((rcn = iter())) {
		// debug
		// cout << "\nDrawing " << rcn->get_key() << "...\n";
		draw_one_curve(
			rcn->payload.norm_count,
			rcn->payload.norm_x,
			rcn->payload.norm_y,
			width,
			height,
			pages_per_display_width,
			page,
			device,
			minValue,
			maxValue); }

	// debug
	// cerr << "  event_length = " << event_length << "; calling draw_external_points if > 0 ...\n";
	if(event_length) {
		draw_external_points(
			event_length,
			width,
			height,
			device,
			TELEMETRY); }

	delete [] segvals;

}

void MW_continuous::draw_external_points(
			int		Count,
			int		width,
			int		height,
			Device		&device,
			sr_DRAW_STYLE	fill_type) {

	sr_Rectangle			*eventvals = NULL;
	sr_DRAW_STYLE			*eventvals_style = NULL;
	int				i, nrect = 0;
	int				RECT_HALF_WIDTH = 2, RECT_HALF_HEIGHT = 2;

	if(Count == 0) return;
	eventvals 	= new sr_Rectangle[Count];
	eventvals_style = new sr_DRAW_STYLE[Count];

	// debug
	// cerr << "draw_external points: " << Count << " point(s)\n";
	for(i = 0; i < Count; i++) {
		int left_x = ((int) (width * event_x[i])) - RECT_HALF_WIDTH;
		int right_y = (int) floor(POS_EPSILON			/* fix roundoff */
				+ height * (((ymin + yspan) - event_y[i]) / yspan))
				- RECT_HALF_HEIGHT;

		// cerr << "    left_x = " << left_x << ", right_y = " << right_y << "; w,h = (" << width << ", " << height << ")\n";
		if(0 <= left_x && left_x <= width && 0 <= right_y && right_y <= height) {
			eventvals[nrect].x = left_x;
			eventvals[nrect].y = device.top() + right_y;
			//O'Reilly Vol II, p. 232, for XFillRectangle reason for "1 +":
			eventvals[nrect].width = 1 + 2 * RECT_HALF_WIDTH;
			eventvals[nrect].height = 1 + 2 * RECT_HALF_HEIGHT;
			nrect++; }
		// else {
		// 	cerr << "    out of bounds. damn.\n"; }
		}

	for(i = 0; i < nrect; i++) {
		device.fill_rectangle(
			eventvals[i],	// single rectangle
			fill_type);	// fill style (e.g. DIAGONAL)
		}
	delete [] eventvals;
	delete [] eventvals_style; }

void MW_continuous::draw_one_curve_and_fill(
			int		Count,
			int		width,
			int		height,
			int		pages_per_display_width,
			int		page,
			Device		&device,
			int		minVal,
			int		maxVal,
			sr_DRAW_STYLE	standard_fill
			) {
	int	i, nseg, nrect, nciti;

	/*
	 * NOTE:  tried leaving Y-values which are beyond the window bounds.  This
	 *   approach fails because large values wrap under ultimate conversion to X
	 *   (which uses short's), and artificial reduction of values' magnitude
	 *   affects the displayed slope.  No other apparent way to explicitly clip
	 *   (still have too-large Y's).  Therefore, forced to use complex logic.
	 *
	 * Create outline line and fill rectangles and/or truncated-rectangles
	 *   ("CitiBank" shape) during the same loop.  Rectangles are assumed to be
	 *   more efficient, and are used whenever possible.  Each interval (there
	 *   are (Count-1) of them) may yield a rectangle (if the interval
	 *   endpoints yield the same pixel-Y) OR a truncated-rectangle (if the
	 *   endpoints yield different pixel-Y's) OR both (latter situation with
	 *   truncation of the triangular portion by the upper window bound).
	 *   Truncation by the lower window bound doesn't add possibilities; note,
	 *   however, that a triangle is a "degenerate" truncated-rectangle, i.e. it
	 *   has no extent below the truncation, and 2 of 4 points are coincident.
	 *   Outline is broken into segments (max 1 per interval) because a polyline
	 *   would be broken by excursions beyond the upper and lower Y-bounds.
	 * 
	 * saved line-segments, rectangles, and truncated-rectangles
	 * also, the drawing-style contexts for each (separately from geometry)
	 */
	sr_Segment*	linevals	= new sr_Segment[Count];
	sr_Rectangle*	rectvals	= new sr_Rectangle[Count];
	sr_DRAW_STYLE*	rectvals_style	= new sr_DRAW_STYLE[Count];
	sr_CitiBank*	citivals 	= new sr_CitiBank[Count];
	sr_DRAW_STYLE*	citivals_style	= new sr_DRAW_STYLE[Count];

	// debug
	// cerr << "draw_one_curve_and_fill: Count = " << Count << endl;
	// cerr << "    ythreshold_max_set = " << ((int) ythreshold_max_set) << ", ythreshold_max = " << ythreshold_max << endl;
	// cerr << "    ythreshold_min_set = " << ((int) ythreshold_min_set) << ", ythreshold_min = " << ythreshold_min << endl;

	nseg  = 0;
	nrect = 0;
	nciti = 0;

			//Do we have (1 of each of) these for the current loop iteration?
	bool		have_seg, have_rect, have_citi;

	//working variables
	//X,Y at left,right ends of interval

	int		left_x, left_y, right_x, right_y;

	//X,Y at interpolated locations:  left,middle,right.  Use these as such, if
	//  above X's differ AND above Y's differ.  Otherwise, use as work vars.
	//int lint_x, lint_y, mint_x, mint_y, rint_x, rint_y;

	int		lint_y, rint_y;

	//Does left point have the lower Y value (i.e. greater pixel value)?
	bool		left_low;

	//Lower point and higher point (in Y value; opposite of "pixel value")
	int		lo_y, hi_y;
	//X-values at lower- and upper-bounds intersections; if no interpolation
	//  WITHIN INTERVAL, corresponding bool is set to FALSE
	int		lo_int_x, hi_int_x;
	bool		is_lo_int, is_hi_int;

	right_x = (int) ((width / 100.0) * norm_x[0]);     //truncates
	right_y = (int) floor(POS_EPSILON /*fix roundoff*/ +
				height * (((ymin + yspan) - norm_y[0]) / yspan));

	for(i = 1 /*i==0 doesn't define an interval*/; i < Count; i++) {
		//X has visible pixel range 0 <= X < width, 0 <= Y < height

		// debug
		// cout << " drawing curve, i = " << i << ", value = " << norm_y[i] << endl;

		//right-hand points of previous interval are left-hand points of this 1
		left_x = right_x;
		left_y = right_y;

		//normalized X is 0.0 to 100.0; convert to pixel values 0 to (width-1):
		right_x = (int) ((width / 100.0) * norm_x[i]);	//truncates

		//compute pixel value of Y, whether or not in displayed range(shouldn't
		//  overflow, as long as int's are 4 bytes NOT 2!!!) -- OK assumption
		//  for SUN, HP (note that low-level X routines ultimately convert to
		//  short (2 bytes on SUN,HP), so can't stop here and let X clip it!!!)
		right_y = (int)floor(POS_EPSILON /*fix roundoff*/ +
				height * (((ymin + yspan) - norm_y[i]) / yspan));

		//Case A:  have zero width, so only need to draw edge line (no fill):
		if(right_x == left_x) {
			have_rect = FALSE;
			have_citi = FALSE;

				/*
				 * line may have zero height (point, need not draw), or it may have
				 *   height, and be completely inside or completely outside the
				 *   Y-range, or it may span both inside and outside (hi and/or low)
				 */
			lint_y = left_y;
				/*
				 * collapse values outside Y-range, to nearest value just outside
				 *   (can do this, because slope inside the range, is not an issue)
				 */
			if(lint_y < 0) {
				lint_y = -1; }
			else if(lint_y >= height) {
				lint_y = height; }

			rint_y = right_y;
			if(rint_y < 0) {
				rint_y = -1; }
			else if(rint_y >= height) {
				rint_y = height; }

			if(rint_y == lint_y) {    //either a point, or fully outside Y-range
				have_seg  = FALSE; }
			else {
				/*
				 * if segment goes outside Y-range,consider only the inside part
				 *   (do -1-->0 (for unsigned); need height(-1 ends line short))
				 */
				if(lint_y == -1)
					lint_y = 0;
				if(rint_y == -1)
					rint_y = 0;

				have_seg  = TRUE;
				linevals[nseg].x1 = linevals[nseg].x2 = left_x;
				linevals[nseg].y1 = device.top() + lint_y;
				linevals[nseg].y2 = device.top() + rint_y; } }

		//Case B:  have rectangle with positive width (fill and edge line):
		else if(right_y == left_y) {
			/* && (right_x > left_x) */
			have_citi = FALSE;

			//Case B1:  rectangle extends above Y-range (so no top edge line):
			if(left_y < 0) {
				have_seg  = FALSE;

				have_rect = TRUE;
				rectvals[nrect].x = left_x;
				rectvals[nrect].y = device.top();
				//O'Reilly Vol II, p. 232, for XFillRectangle reason for "1 +":
				rectvals[nrect].width = 1 + right_x - left_x;
				rectvals[nrect].height = height;

				// PFM: changed >= to > (for integer violations the max # is ok)
				// if(ythreshold_max_set && (norm_y[i] >= ythreshold_max))
				if(	ythreshold_max_set
					&& (
						(norm_y[i - 1] > ythreshold_max)
						|| (norm_y[i] > ythreshold_max)
					   )
				  ) {
					// debug
					// cout << "  (over max)\n";
					rectvals_style[nrect] = DIAG_OVER_MAX; }
				else if(ythreshold_min_set
					&& (
						(norm_y[i - 1] < ythreshold_min)
						|| (norm_y[i] < ythreshold_min)
					   )
				       ) {
					// debug
					// cout << "  (under min)\n";
					rectvals_style[nrect] = DIAG_UNDER_MIN; }
				else if(have_error_high
					&& (
						(norm_y[i - 1] > norm_error_high[i - 1])
						&& (norm_y[i] > norm_error_high[i])
					   )
				       ) {
					rectvals_style[nrect] = DIAG_OVER_MAX; }
				else if(have_warn_high
					&& (
						(norm_y[i - 1] > norm_warn_high[i - 1])
						&& (norm_y[i] > norm_warn_high[i])
					   )
				       ) {
					rectvals_style[nrect] = FUZZY; }
				else if(have_error_low
					&& (
						(norm_y[i - 1] < norm_error_low[i - 1])
						&& (norm_y[i] < norm_error_low[i])
					   )
				       ) {
					rectvals_style[nrect] = DIAG_OVER_MAX; }
				else if(have_warn_low
					&& (
						(norm_y[i - 1] < norm_warn_low[i - 1])
						&& (norm_y[i] < norm_warn_low[i])
					   )
				       ) {
					rectvals_style[nrect] = FUZZY; }
				else {
					// debug
					// cout << "  (normal)\n";
					rectvals_style[nrect] = standard_fill; } }
			//Case B2:  rectangle within Y-range (fill plus top edge line):
			else if(left_y < height) { /* && (left_y >= 0) */
				have_seg  = TRUE;
				linevals[nseg].x1 = left_x;
				linevals[nseg].x2 = right_x;
				linevals[nseg].y1 = linevals[nseg].y2 = device.top() + left_y;

				have_rect  = TRUE;
				//segment overlaps 1 row of rect pixels; OK since draw rect 1st
				rectvals[nrect].x = left_x;
				rectvals[nrect].y = device.top() + left_y;
				//O'Reilly Vol II, p. 232, for XFillRectangle reason for "1 +":
				rectvals[nrect].width = 1 + right_x - left_x;
				rectvals[nrect].height = height - left_y;

				// PFM: changed >= to > -- for integer violations the max # is ok
				// if(ythreshold_max_set && (norm_y[i] >= ythreshold_max))
				if(	ythreshold_max_set
					&& (
						(norm_y[i - 1] > ythreshold_max)
						|| (norm_y[i] > ythreshold_max)
					   )
				  ) {
					// debug
					// cout << "  (over max)\n";
					rectvals_style[nrect] = DIAG_OVER_MAX; }
				else if(ythreshold_min_set
					&& (
						(norm_y[i - 1] < ythreshold_min)
						|| (norm_y[i] < ythreshold_min)
					   )
				       ) {
					// debug
					// cout << "  (under min)\n";
					rectvals_style[nrect] = DIAG_UNDER_MIN; }
				else if(have_error_high
					&& (
						(norm_y[i - 1] > norm_error_high[i - 1])
						&& (norm_y[i] > norm_error_high[i])
					   )
				       ) {
					rectvals_style[nrect] = DIAG_OVER_MAX; }
				else if(have_warn_high
					&& (
						(norm_y[i - 1] > norm_warn_high[i - 1])
						&& (norm_y[i] > norm_warn_high[i])
					   )
				       ) {
					rectvals_style[nrect] = FUZZY; }
				else if(have_error_low
					&& (
						(norm_y[i - 1] < norm_error_low[i - 1])
						&& (norm_y[i] < norm_error_low[i])
					   )
				       ) {
					rectvals_style[nrect] = DIAG_OVER_MAX; }
				else if(have_warn_low
					&& (
						(norm_y[i - 1] < norm_warn_low[i - 1])
						&& (norm_y[i] < norm_warn_low[i])
					   )
				       ) {
					rectvals_style[nrect] = FUZZY; }
				else {
					// debug
					// cout << "  (normal)\n";
					rectvals_style[nrect] = standard_fill; } }
			//Case B3:  rectangle is below Y-range, so see nothing:
			else { /* (left_y >= height) */
				have_seg  = FALSE;
				have_rect = FALSE; } }
		//Case C:  have "CitiBank" truncated-rectangle (fill and edge line):
		else {
			/* (right_y != left_y) && (right_x > left_x) */
				//EITHER:  slopes DOWN (lower Y; bigger pixel-Y) to right
			if(left_y < right_y) {
				left_low = FALSE;
				lo_y = right_y;
				hi_y = left_y; }
				//OR:  slopes UP (higher Y; lesser pixel-Y) to right
			else { /* (left_y > right_y) */
				left_low = TRUE;
				lo_y = left_y;
				hi_y = right_y; }

				/*
				 * compute X-pixel locations for (0, 1, or 2) intersections of line
				 *   between interval endpoints, with Y==0 and Y==height window
				 *   bounds (or mark no intersection using "-1", since range is
				 *   0 <= value < width); for accuracy, compute using double values!
				 */
			if(left_low) {	//slopes up
				if((norm_y[i-1] > (ymin + POS_EPSILON))
					|| (norm_y[i]   < (ymin - POS_EPSILON))) {
					is_lo_int = FALSE; }
				else {
					is_lo_int = TRUE; }
				if((norm_y[i-1] > (ymin + yspan + POS_EPSILON))
					|| (norm_y[i]   < (ymin + yspan - POS_EPSILON))) {
					is_hi_int = FALSE; }
				else {
					is_hi_int = TRUE; } }
			else {			//slopes down
				if((norm_y[i-1] < (ymin - POS_EPSILON))
				 || (norm_y[i]   > (ymin + POS_EPSILON))) {
					is_lo_int = FALSE; }
				else {
					is_lo_int = TRUE; }
				if((norm_y[i-1] < (ymin + yspan - POS_EPSILON))
				 || (norm_y[i]   > (ymin + yspan + POS_EPSILON))) {
					is_hi_int = FALSE; }
				else
					is_hi_int = TRUE; }
			if(is_lo_int) {
				lo_int_x =	//pixel-Y == height
					(int) ((width / 100.0) * (norm_x[i-1]
				  + ((norm_x[i] - norm_x[i-1])
					* ((ymin - norm_y[i-1])
					  / (norm_y[i] - norm_y[i-1])))));
				if(lo_int_x == width) {	//fix possible roundoff glitch
					lo_int_x = width - 1; } }
			else {
				lo_int_x = -1; }		//set to illegal value, for safety
			if(is_hi_int) {
				hi_int_x =	//pixel-Y == 0
				(int) ((width / 100.0) * (norm_x[i-1]
					  + ((norm_x[i] - norm_x[i-1])
						* (((ymin + yspan) - norm_y[i-1])
						  / (norm_y[i] - norm_y[i-1])))));
				if(hi_int_x == width) {	//fix possible roundoff glitch
					hi_int_x = width - 1; } }
			else {
				hi_int_x = -1; }	//set to illegal value, for safety

			// Case C1:  apex of truncated-rectangle is below Y-range, see zip:
			if(hi_y >= height) {
				have_seg  = FALSE;
				have_rect = FALSE;
				have_citi = FALSE; }
			//Case C2:  lower of 2 truncated-rectangle top points, is above
			//		Y-range, so see a rectangle which fills the Y-range
			//		and no top edge line (this is identical to Case B1):
			else if(lo_y <= 0) {
				have_seg  = FALSE;
				have_citi = FALSE;

				have_rect = TRUE;
				rectvals[nrect].x = left_x;
				rectvals[nrect].y = device.top() + 0;
				//O'Reilly Vol II, p. 232, for XFillRectangle reason for "1 +":
				rectvals[nrect].width = 1 + right_x - left_x;
				rectvals[nrect].height = height;

				// PFM: changed >= to > -- for integer violations the max # is ok
				// if(ythreshold_max_set && (norm_y[i] >= ythreshold_max))
				if(	ythreshold_max_set
					&& (
						(norm_y[i - 1] > ythreshold_max)
						|| (norm_y[i] > ythreshold_max)
					   )
				  ) {
					// debug
					// cout << "  (over max)\n";
					rectvals_style[nrect] = DIAG_OVER_MAX; }
				else if(ythreshold_min_set
					&& (
						(norm_y[i - 1] < ythreshold_min)
						|| (norm_y[i] < ythreshold_min)
					   )
				       ) {
					// debug
					// cout << "  (under min)\n";
					rectvals_style[nrect] = DIAG_UNDER_MIN; }
				else if(have_error_high
					&& (
						(norm_y[i - 1] > norm_error_high[i - 1])
						&& (norm_y[i] > norm_error_high[i])
					   )
				       ) {
					rectvals_style[nrect] = DIAG_OVER_MAX; }
				else if(have_warn_high
					&& (
						(norm_y[i - 1] > norm_warn_high[i - 1])
						&& (norm_y[i] > norm_warn_high[i])
					   )
				       ) {
					rectvals_style[nrect] = FUZZY; }
				else if(have_error_low
					&& (
						(norm_y[i - 1] < norm_error_low[i - 1])
						&& (norm_y[i] < norm_error_low[i])
					   )
				       ) {
					rectvals_style[nrect] = DIAG_OVER_MAX; }
				else if(have_warn_low
					&& (
						(norm_y[i - 1] < norm_warn_low[i - 1])
						&& (norm_y[i] < norm_warn_low[i])
					   )
				       ) {
					rectvals_style[nrect] = FUZZY; }
				else {
					// debug
					// cout << "  (normal)\n";
					rectvals_style[nrect] = standard_fill; } }
			//Case C3:  higher of 2 truncated-rectangle top points, is within
			//		the Y-range, so see a truncated-rectangle (possibly
			//		degenerate to the apex triangle):
			else if((hi_y >= 0) && (hi_y < height)) {
				have_rect = FALSE;

				//Case C3a:  lower of top points is also within Y-range, so see
				//	   (possibly-degenerate) FULL-WIDTH truncated-rectangle
				if(lo_y <= height) {
					have_seg  = TRUE;
					have_citi = TRUE;

					linevals[nseg].x1 = citivals[nciti].x1 = left_x;
					linevals[nseg].y1 = citivals[nciti].y1 = device.top() + left_y;
					linevals[nseg].x2 = citivals[nciti].x2 = right_x;
					linevals[nseg].y2 = citivals[nciti].y2 = device.top() + right_y;
					citivals[nciti].rect_low = FALSE; //fill to high pixel-Y
					citivals[nciti].rect_height = height - lo_y; }
				//Case C3b: 	lower of top points is below Y-range, so see
				//		degenerate PARTIAL-WIDTH truncated-rectangle
				else { /* (lo_y > height) */
					have_seg  = TRUE;
					have_citi = TRUE;
					citivals[nciti].rect_low = FALSE; //fill to high pixel-Y
					citivals[nciti].rect_height = 0;//degenerate,i.e. triangle

					//*** (height-1) instead of height???
					if(left_low) {	//slopes up
						linevals[nseg].x1 = citivals[nciti].x1 = lo_int_x;
						linevals[nseg].y1 = citivals[nciti].y1 = device.top() + height;
						linevals[nseg].x2 = citivals[nciti].x2 = right_x;
						linevals[nseg].y2 = citivals[nciti].y2 = device.top() + right_y; }
					else {		//slopes down
						linevals[nseg].x1 = citivals[nciti].x1 = left_x;
						linevals[nseg].y1 = citivals[nciti].y1 = device.top() + left_y;
						linevals[nseg].x2 = citivals[nciti].x2 = lo_int_x;
						linevals[nseg].y2 = citivals[nciti].y2 = device.top() + height; } }

				// PFM: changed >= to > -- for integer violations the max # is ok
				// if(ythreshold_max_set && (norm_y[i] >= ythreshold_max))
				if(	ythreshold_max_set
					&& (
						(norm_y[i - 1] > ythreshold_max)
						|| (norm_y[i] > ythreshold_max)
					   )
				  ) {
					// debug
					// cout << "  (over max)\n";
					citivals_style[nciti] = DIAG_OVER_MAX; }
				else if(ythreshold_min_set
					&& (
						(norm_y[i - 1] < ythreshold_min)
						|| (norm_y[i] < ythreshold_min)
					   )
				       ) {
					// debug
					// cout << "  (under min)\n";
					citivals_style[nciti] = DIAG_UNDER_MIN; }
				else if(have_error_high
					&& (
						(norm_y[i - 1] > norm_error_high[i - 1])
						&& (norm_y[i] > norm_error_high[i])
					   )
				       ) {
					citivals_style[nciti] = DIAG_OVER_MAX; }
				else if(have_warn_high
					&& (
						(norm_y[i - 1] > norm_warn_high[i - 1])
						&& (norm_y[i] > norm_warn_high[i])
					   )
				       ) {
					citivals_style[nciti] = FUZZY; }
				else if(have_error_low
					&& (
						(norm_y[i - 1] < norm_error_low[i - 1])
						&& (norm_y[i] < norm_error_low[i])
					   )
				       ) {
					citivals_style[nciti] = DIAG_OVER_MAX; }
				else if(have_warn_low
					&& (
						(norm_y[i - 1] < norm_warn_low[i - 1])
						&& (norm_y[i] < norm_warn_low[i])
					   )
				       ) {
					citivals_style[nciti] = FUZZY; }
				else {
					// debug
					// cout << "  (normal)\n";
					citivals_style[nciti] = standard_fill; } }
			//Case C4:  triangular portion of truncated-rectangle is clipped at
			//		top of Y-range, so split into truncated-rectangle and
			//		regular rectangle
			else { /* ((lo_y > 0) && (hi_y < 0)) */
				have_seg  = TRUE;
				have_rect = TRUE;
				have_citi = TRUE;
				citivals[nciti].rect_low = FALSE;	//fill to high pixel-Y

				//Case C4a:  lower of top points is also within Y-range, so see
				//	(possibly-degenerate) truncated-rectangle, adjacent to
				//	a regular rectangle (for the clipped portion), which
				//	together fill the FULL WIDTH of the segment
				//*** (height-1) instead of height???  OTHER CASES ABOVE TOO???
				if(lo_y <= height) {
					//*** ???bother checking for 0-width citi or rect?
					if(left_low) {	//slopes up
						linevals[nseg].x1 = citivals[nciti].x1 = left_x;
						linevals[nseg].y1 = citivals[nciti].y1 = device.top() + left_y;
						linevals[nseg].x2 = citivals[nciti].x2 = hi_int_x;
						linevals[nseg].y2 = citivals[nciti].y2 = device.top() + 0;
						citivals[nciti].rect_height = height - lo_y;

						rectvals[nrect].x = hi_int_x;
						rectvals[nrect].y = device.top() + 0;
						//O'Reilly V II,p. 232, for XFillRectangle "1+" reason:
						rectvals[nrect].width = 1 + right_x - hi_int_x;
						rectvals[nrect].height = height; }
					else {		//slopes down
						rectvals[nrect].x = left_x;
						rectvals[nrect].y = 0;
						//O'Reilly V II,p. 232, for XFillRectangle "1+" reason:
						rectvals[nrect].width = 1 + hi_int_x - left_x;
						rectvals[nrect].height = height;

						linevals[nseg].x1 = citivals[nciti].x1 = hi_int_x;
						linevals[nseg].y1 = citivals[nciti].y1 = device.top() + 0;
						linevals[nseg].x2 = citivals[nciti].x2 = right_x;
						linevals[nseg].y2 = citivals[nciti].y2 = device.top() + right_y;
						citivals[nciti].rect_height = height - lo_y; } }
				//Case C4b:  lower of top points is BELOW Y-range, so see
				//	degenerate truncated-rectangle, adjacent to a regular
				//	rectangle (for the clipped portion), where the the
				//	truncated-rectangle is only PARTIAL-WIDTH
				//NOTE:  rectangle logic SAME as C4a!
				//*** (height-1) instead of height???
				else { /* (lo_y > height) */
					//*** ???bother checking for 0-width citi or rect?
					if(left_low) {	//slopes up
						linevals[nseg].x1 = citivals[nciti].x1 = lo_int_x;
						linevals[nseg].y1 = citivals[nciti].y1 = device.top() + height;
						linevals[nseg].x2 = citivals[nciti].x2 = hi_int_x;
						linevals[nseg].y2 = citivals[nciti].y2 = device.top() + 0;
						citivals[nciti].rect_height = 0;

						rectvals[nrect].x = hi_int_x;
						rectvals[nrect].y = device.top() + 0;
						//O'Reilly V II,p. 232, for XFillRectangle "1+" reason:
						rectvals[nrect].width = 1 + right_x - hi_int_x;
						rectvals[nrect].height = height; }
					else {		//slopes down
						rectvals[nrect].x = left_x;
						rectvals[nrect].y = device.top() + 0;
						//O'Reilly V II,p. 232, for XFillRectangle "1+" reason:
						rectvals[nrect].width = 1 + hi_int_x - left_x;
						rectvals[nrect].height = height;

						linevals[nseg].x1 = citivals[nciti].x1 = hi_int_x;
						linevals[nseg].y1 = citivals[nciti].y1 = device.top() + 0;
						linevals[nseg].x2 = citivals[nciti].x2 = lo_int_x;
						linevals[nseg].y2 = citivals[nciti].y2 = device.top() + height;
						citivals[nciti].rect_height = 0; } }

				// PFM: changed >= to > -- for integer violations the max # is ok
				// if(ythreshold_max_set && (norm_y[i] >= ythreshold_max))
				if(	ythreshold_max_set
					&& (
						(norm_y[i - 1] > ythreshold_max)
						|| (norm_y[i] > ythreshold_max)
					   )
				  ) {
					// debug
					// cout << "  (over max)\n";
					rectvals_style[nrect] = DIAG_OVER_MAX;
					citivals_style[nciti] = DIAG_OVER_MAX; }
				else if(ythreshold_min_set
					&& (
						(norm_y[i - 1] < ythreshold_min)
						|| (norm_y[i] < ythreshold_min)
					   )
				       ) {
					// debug
					// cout << "  (under min)\n";
					rectvals_style[nrect] = DIAG_UNDER_MIN;
					citivals_style[nciti] = DIAG_UNDER_MIN; }
				else if(have_error_high
					&& (
						(norm_y[i - 1] > norm_error_high[i - 1])
						&& (norm_y[i] > norm_error_high[i])
					   )
				       ) {
					rectvals_style[nrect] = DIAG_OVER_MAX;
					citivals_style[nciti] = DIAG_OVER_MAX; }
				else if(have_warn_high
					&& (
						(norm_y[i - 1] > norm_warn_high[i - 1])
						&& (norm_y[i] > norm_warn_high[i])
					   )
				       ) {
					rectvals_style[nrect] = FUZZY;
					citivals_style[nciti] = FUZZY; }
				else if(have_error_low
					&& (
						(norm_y[i - 1] < norm_error_low[i - 1])
						&& (norm_y[i] < norm_error_low[i])
					   )
				       ) {
					rectvals_style[nrect] = DIAG_OVER_MAX;
					citivals_style[nciti] = DIAG_OVER_MAX; }
				else if(have_warn_low
					&& (
						(norm_y[i - 1] < norm_warn_low[i - 1])
						&& (norm_y[i] < norm_warn_low[i])
					   )
				       ) {
					rectvals_style[nrect] = FUZZY;
					citivals_style[nciti] = FUZZY; }
				else {
					// debug
					// cout << "  (normal)\n";
					rectvals_style[nrect] = standard_fill;
					citivals_style[nciti] = standard_fill; } } }

		if(have_seg)
			nseg++;
		if(have_rect)
			nrect++;
		if(have_citi)
			nciti++; }

	//Adjust linevals for plotting when (pages_per_display_width > 1):
	//  only X's are affected; Y's are unchanged.
	if(pages_per_display_width > 1) {
		for(i = 0; i < nseg; i++) {
			linevals[i].x1 -= ((page - 1) * (width / pages_per_display_width));
			linevals[i].x2 -= ((page - 1) * (width / pages_per_display_width)); } }

	//good logic:  separate computing loop (above) from rendering loop (below)

	//rectangles then truncated-rectangles (which may interleave) are drawn
	//draw fill AFTER dashed horizontal lines at major ticks, so overwrites
	for(i = 0; i < nrect; i++)
		device.fill_rectangle(
			rectvals[i],	// single rectangle
			rectvals_style[i]);	//fill style (e.g. DIAGONAL)

	if(device.get_device_type() == MOTIF_DEVICE) {
		//draw fill AFTER dashed horizontal lines at major ticks, so overwrites
		for(i = 0; i < nciti; i++)
			device.fill_citibank(
				citivals[i],	//single "citibank" truncated rectangle
				citivals_style[i]); }	//fill style (e.g. DIAGONAL)
	else {	// POSTSCRIPT_DEVICE
		// convert segments to a series of point to allow for drawing under the curve
		sr_IntPoint* pointValues = new sr_IntPoint[2*nseg];
		int index = 0;
		static DeviceColor normal = {(char*) "BrightGreen", 0, 255, 0};
		static DeviceColor violation = {(char*) "BrightRed", 255, 0, 0};
		static DeviceColor white = {(char*) "White", 255, 255, 255};

		const double xScale = device.x_sc();	// scaling factor for limit points
		const double yScale = device.y_sc();

		// compute all points on curve (violations + normal)
		for(int i = 0; i < nseg; i++) {
			if(i == 0) {
				pointValues[index].x = linevals[i].x1;
				pointValues[index].y = linevals[i].y1;
				pointValues[index+1].x = linevals[i].x2;
				pointValues[index+1].y = linevals[i].y2;
				index += 2; }
			else {
				pointValues[index].x = linevals[i].x2;
				pointValues[index].y = linevals[i].y2;
				index++; } }

		// draw normal+violations curve as violations
		// device.fillToBaseline(pointValues, index, &violation);
		device.fillToBaseline(pointValues, index, &white);	// zap area under curve (remove grid lines)
		device.fillRegionWithPattern(pointValues, index, &violation);

		// compute points for normal region
		// (regarding min/maxValue; coordinate system is reversed)
		index = 0;
		for(int i = 0; i < nseg; i++) {
			if(i == 0) {
				pointValues[index].x = linevals[i].x1;
				pointValues[index+1].x = linevals[i].x2;
				if(linevals[i].y2 > device.top() + minVal || linevals[i].y2 < device.top() + maxVal)
					pointValues[index].y = pointValues[index+1].y = device.top() + (int) (1000/yScale);
					// create points to clip out normal values
				else {
					pointValues[index].y = linevals[i].y1;
					pointValues[index+1].y = linevals[i].y2; } }
			else {
				pointValues[index].x = pointValues[index-1].x;
				pointValues[index+1].x = linevals[i].x2;
				if(linevals[i].y2 > device.top() + minVal || linevals[i].y2 < device.top() + maxVal) {
					// create points to clip out normal values
					pointValues[index].y = device.top() + (int) (1000/yScale);
					pointValues[index+1].y = device.top() + (int) (1000/yScale); }
				else {
					pointValues[index].y = linevals[i-1].y2;
					pointValues[index+1].y = linevals[i].y2; } }
			index += 2; }

		// draw normal curve
		device.fillToBaseline(pointValues, index, &normal);

		delete [] pointValues; }

	/*
	 * draw outline AFTER fill, so overwrites (y <= -1, y >= height are CLIPPED)
	 */
	device.draw_line_segments(
			linevals,	// segment array
			nseg,		// n in this array (0 seems OK)
			FALSE,		// bold?
			FALSE);		// dash?


	// PFM used purify to find these:
	delete [] rectvals_style;
	delete [] citivals_style;

	delete [] linevals;
	delete [] rectvals;
	delete [] citivals; }

void MW_continuous::draw_one_curve(
			int		Count,
			double*		res_norm_x,
			double*		res_norm_y,
			int		width,
			int		height,
			int		pages_per_display_width,
			int		page,
			Device		&device,
			int		minVal,
			int		maxVal
			) {
	int	i, nseg, nrect, nciti;

	sr_Segment*	linevals	= new sr_Segment[Count];

	nseg  = 0;

			//Do we have (1 of each of) these for the current loop iteration?
	bool		have_seg;

	//working variables
	//X,Y at left,right ends of interval

	int		left_x, left_y, right_x, right_y;

	//X,Y at interpolated locations:  left,middle,right.  Use these as such, if
	//  above X's differ AND above Y's differ.  Otherwise, use as work vars.
	//int lint_x, lint_y, mint_x, mint_y, rint_x, rint_y;

	int		lint_y, rint_y;

	//Does left point have the lower Y value (i.e. greater pixel value)?
	bool		left_low;

	//Lower point and higher point (in Y value; opposite of "pixel value")
	int		lo_y, hi_y;
	//X-values at lower- and upper-bounds intersections; if no interpolation
	//  WITHIN INTERVAL, corresponding bool is set to FALSE
	int		lo_int_x, hi_int_x;
	bool		is_lo_int, is_hi_int;

	right_x = (int) ((width / 100.0) * res_norm_x[0]);     //truncates
	right_y = (int) floor(POS_EPSILON /*fix roundoff*/ +
				height * (((ymin + yspan) - res_norm_y[0]) / yspan));

	// debug
	// cerr << "\n\n";
	for(i = 1 /*i==0 doesn't define an interval*/; i < Count; i++) {
		//X has visible pixel range 0 <= X < width, 0 <= Y < height

		// debug
		// cerr << " drawing curve, i = " << i << ", value = " << res_norm_y[i] << endl;

		//right-hand points of previous interval are left-hand points of this 1
		left_x = right_x;
		left_y = right_y;

		//normalized X is 0.0 to 100.0; convert to pixel values 0 to (width-1):
		right_x = (int) ((width / 100.0) * res_norm_x[i]);	//truncates

		//compute pixel value of Y, whether or not in displayed range(shouldn't
		//  overflow, as long as int's are 4 bytes NOT 2!!!) -- OK assumption
		//  for SUN, HP (note that low-level X routines ultimately convert to
		//  short (2 bytes on SUN,HP), so can't stop here and let X clip it!!!)
		right_y = (int)floor(POS_EPSILON /*fix roundoff*/ +
				height * (((ymin + yspan) - res_norm_y[i]) / yspan));

		//Case A:  have zero width, so only need to draw edge line (no fill):
		if(right_x == left_x) {
				/*
				 * line may have zero height (point, need not draw), or it may have
				 *   height, and be completely inside or completely outside the
				 *   Y-range, or it may span both inside and outside (hi and/or low)
				 */
			lint_y = left_y;
				/*
				 * collapse values outside Y-range, to nearest value just outside
				 *   (can do this, because slope inside the range, is not an issue)
				 */
			if(lint_y < 0) {
				lint_y = -1; }
			else if(lint_y >= height) {
				lint_y = height; }

			rint_y = right_y;
			if(rint_y < 0) {
				rint_y = -1; }
			else if(rint_y >= height) {
				rint_y = height; }

			if(rint_y == lint_y) {    //either a point, or fully outside Y-range
				have_seg  = false; }
			else {
				/*
				 * if segment goes outside Y-range,consider only the inside part
				 *   (do -1-->0 (for unsigned); need height(-1 ends line short))
				 */
				if(lint_y == -1)
					lint_y = 0;
				if(rint_y == -1)
					rint_y = 0;

				have_seg  = true;
				linevals[nseg].x1 = linevals[nseg].x2 = left_x;
				linevals[nseg].y1 = device.top() + lint_y;
				linevals[nseg].y2 = device.top() + rint_y; } }

		//Case B:  have rectangle with positive width (fill and edge line):
		else if(right_y == left_y) {

			//Case B1:  rectangle extends above Y-range (so no top edge line):
			if(left_y < 0) {
				have_seg  = false; }
			//Case B2:  rectangle within Y-range (fill plus top edge line):
			else if(left_y < height) { /* && (left_y >= 0) */
				have_seg  = true;
				linevals[nseg].x1 = left_x;
				linevals[nseg].x2 = right_x;
				linevals[nseg].y1 = linevals[nseg].y2 = device.top() + left_y; }
			//Case B3:  rectangle is below Y-range, so see nothing:
			else { /* (left_y >= height) */
				have_seg  = false; } }
		//Case C:  have "CitiBank" truncated-rectangle (fill and edge line):
		else {
			/* (right_y != left_y) && (right_x > left_x) */
				//EITHER:  slopes DOWN (lower Y; bigger pixel-Y) to right
			if(left_y < right_y) {
				left_low = false;
				lo_y = right_y;
				hi_y = left_y; }
				//OR:  slopes UP (higher Y; lesser pixel-Y) to right
			else { /* (left_y > right_y) */
				left_low = true;
				lo_y = left_y;
				hi_y = right_y; }

				/*
				 * compute X-pixel locations for (0, 1, or 2) intersections of line
				 *   between interval endpoints, with Y==0 and Y==height window
				 *   bounds (or mark no intersection using "-1", since range is
				 *   0 <= value < width); for accuracy, compute using double values!
				 */
			if(left_low) {	//slopes up
				if((res_norm_y[i-1] > (ymin + POS_EPSILON))
					|| (res_norm_y[i]   < (ymin - POS_EPSILON))) {
					is_lo_int = false; }
				else {
					is_lo_int = true; }
				if((res_norm_y[i-1] > (ymin + yspan + POS_EPSILON))
					|| (res_norm_y[i]   < (ymin + yspan - POS_EPSILON))) {
					is_hi_int = false; }
				else {
					is_hi_int = true; } }
			else {			//slopes down
				if((res_norm_y[i-1] < (ymin - POS_EPSILON))
				 || (res_norm_y[i]   > (ymin + POS_EPSILON))) {
					is_lo_int = false; }
				else {
					is_lo_int = true; }
				if((res_norm_y[i-1] < (ymin + yspan - POS_EPSILON))
				 || (res_norm_y[i]   > (ymin + yspan + POS_EPSILON))) {
					is_hi_int = false; }
				else
					is_hi_int = true; }
			if(is_lo_int) {
				lo_int_x =	//pixel-Y == height
					(int) ((width / 100.0) * (res_norm_x[i-1]
				  + ((res_norm_x[i] - res_norm_x[i-1])
					* ((ymin - res_norm_y[i-1])
					  / (res_norm_y[i] - res_norm_y[i-1])))));
				if(lo_int_x == width) {	//fix possible roundoff glitch
					lo_int_x = width - 1; } }
			else {
				lo_int_x = -1; }	//set to illegal value, for safety
			if(is_hi_int) {
				hi_int_x =	//pixel-Y == 0
				(int) ((width / 100.0) * (res_norm_x[i-1]
					  + ((res_norm_x[i] - res_norm_x[i-1])
						* (((ymin + yspan) - res_norm_y[i-1])
						  / (res_norm_y[i] - res_norm_y[i-1])))));
				if(hi_int_x == width) {	//fix possible roundoff glitch
					hi_int_x = width - 1; } }
			else {
				hi_int_x = -1; }	//set to illegal value, for safety

			// Case C1:  apex of truncated-rectangle is below Y-range, see zip:
			if(hi_y >= height) {
				have_seg  = false; }
			//Case C2:  lower of 2 truncated-rectangle top points, is above
			//		Y-range, so see a rectangle which fills the Y-range
			//		and no top edge line (this is identical to Case B1):
			else if(lo_y <= 0) {
				have_seg  = false; }
			else if((hi_y >= 0) && (hi_y < height)) {
				//Case C3a:  lower of top points is also within Y-range, so see
				//	   (possibly-degenerate) FULL-WIDTH truncated-rectangle
				if(lo_y <= height) {
					have_seg  = true;
					linevals[nseg].x1 = left_x;
					linevals[nseg].y1 = device.top() + left_y;
					linevals[nseg].x2 = right_x;
					linevals[nseg].y2 = device.top() + right_y; }
				//Case C3b: 	lower of top points is below Y-range, so see
				//		degenerate PARTIAL-WIDTH truncated-rectangle
				else { /* (lo_y > height) */
					have_seg  = true;

					//*** (height-1) instead of height???
					if(left_low) {	//slopes up
						linevals[nseg].x1 = lo_int_x;
						linevals[nseg].y1 = device.top() + height;
						linevals[nseg].x2 = right_x;
						linevals[nseg].y2 = device.top() + right_y; }
					else {		//slopes down
						linevals[nseg].x1 = left_x;
						linevals[nseg].y1 = device.top() + left_y;
						linevals[nseg].x2 = lo_int_x;
						linevals[nseg].y2 = device.top() + height; } } }

			//Case C4:  triangular portion of truncated-rectangle is clipped at
			//		top of Y-range, so split into truncated-rectangle and
			//		regular rectangle
			else { /* ((lo_y > 0) && (hi_y < 0)) */

				//Case C4a:  lower of top points is also within Y-range, so see
				//	(possibly-degenerate) truncated-rectangle, adjacent to
				//	a regular rectangle (for the clipped portion), which
				//	together fill the FULL WIDTH of the segment
				//*** (height-1) instead of height???  OTHER CASES ABOVE TOO???
				if(lo_y <= height) {
					have_seg  = true;
					//*** ???bother checking for 0-width citi or rect?
					if(left_low) {	//slopes up
						linevals[nseg].x1 = left_x;
						linevals[nseg].y1 = device.top() + left_y;
						linevals[nseg].x2 = hi_int_x;
						linevals[nseg].y2 = device.top() + 0; }
					else {		//slopes down
						linevals[nseg].x1 = hi_int_x;
						linevals[nseg].y1 = device.top() + 0;
						linevals[nseg].x2 = right_x;
						linevals[nseg].y2 = device.top() + right_y; } }
				//Case C4b:  lower of top points is BELOW Y-range, so see
				//	degenerate truncated-rectangle, adjacent to a regular
				//	rectangle (for the clipped portion), where the the
				//	truncated-rectangle is only PARTIAL-WIDTH
				//NOTE:  rectangle logic SAME as C4a!
				//*** (height-1) instead of height???
				else { /* (lo_y > height) */
					have_seg  = true;
					//*** ???bother checking for 0-width citi or rect?
					if(left_low) {	//slopes up
						linevals[nseg].x1 = lo_int_x;
						linevals[nseg].y1 = device.top() + height;
						linevals[nseg].x2 = hi_int_x;
						linevals[nseg].y2 = device.top() + 0; }
					else {		//slopes down
						linevals[nseg].x1 = hi_int_x;
						linevals[nseg].y1 = device.top() + 0;
						linevals[nseg].x2 = lo_int_x;
						linevals[nseg].y2 = device.top() + height; } } } }

		if(have_seg) {
			// debug
			// cerr << "linevals[" << nseg << "] = (" << linevals[nseg].x1 << ", "
			// 	<< linevals[nseg].y1 << ") -> (" << linevals[nseg].x2 << ", " << linevals[nseg].y2 << ")\n";
			nseg++; } }

	//Adjust linevals for plotting when (pages_per_display_width > 1):
	//  only X's are affected; Y's are unchanged.
	if(pages_per_display_width > 1) {
		for(i = 0; i < nseg; i++) {
			linevals[i].x1 -= ((page - 1) * (width / pages_per_display_width));
			linevals[i].x2 -= ((page - 1) * (width / pages_per_display_width)); } }

	//good logic:  separate computing loop (above) from rendering loop (below)

	device.draw_line_segments(
			linevals,	// segment array
			nseg,		// n in this array (0 seems OK)
			FALSE,		// bold?
			FALSE);		// dash?

	delete [] linevals; }

//Absolute pan/zoom for any/all Resources with continuous numeric values
//  (this request is ALWAYS appropriate for this class, so do it!)
MW_object::Formatting MW_continuous::yabspanzoom (double min, double span) {
	adjust_yaxis_limits(FALSE, min, span);	//exact values, NO pad!
	return FORMAT_OK;
}

/* All that happens in the following function is that an event_node is added to
 * ListOfEvents. This will be detected later on in IO_seqtalk.C, where
 * theDelayedWorkProc() makes a call to new_events_are_present(). As of
 * 3/30/2009, this call is followed by a call to
 * MW_continuous::get_res_sys()->configure_graphics(). We want to avoid this call,
 * because it causes a lot of useless flicker in the resource displays.
 *
 * We need to know
 *
 * 	- how the caller to update_data() gets its startval, spanval info
 *
 * 		OK, not that difficult: refresh_all_internal_data() makes the
 * 	  	following call:
 *
 *		    update_data(
 *			list_of_values,
 *				list_of_other_values,
 *				theResSysParent->get_siblings()->dwtimestart(),
 *				0.0,
 *				theResSysParent->get_siblings()->dwtimespan(),
 *				100.0,
 *				any_errors);	//uses full 0-100% of display
 *
 * 	- how refresh_canvas() gets its device information
 * 
 * 		OK, that's easy: place the call from RD_sys and emulate
 * 		MW_widgetdraw::update_graphics(). I'll call the new function
 * 		configure_events.
 */
void MW_continuous::add_external_event(const CTime_base& etime, double &val) {
	ListOfEvents << new event_node(etime, val);
	// debug
	// cerr << "MW_continuous: adding ext event at " << etime.to_string() << ", val = " << val << endl;
	// cerr << " list of events has length " << ListOfEvents.get_length() << ", event_length = " << event_length << endl;
	}


void	MW_continuous::refresh_events(Device &device) {
	sr_Rectangle		*eventvals = NULL;
	sr_DRAW_STYLE		*eventvals_style = NULL;
	int			nrect = 0;
	int			RECT_HALF_WIDTH = 2, RECT_HALF_HEIGHT = 2;
	sr_DRAW_STYLE		fill_type = TELEMETRY;

	CTime			startval = theResSysParent->get_siblings()->dwtimestart();
	double			startpct = 0.0;
	CTime			spanval = theResSysParent->get_siblings()->dwtimespan();
	double			spanpct = 100.0;
	int			count, first, i, nval, last_before, first_after, BigN;
				// Note: MW_continuous already has members yrange_min,
				// yrange_span and yrange_set:
	double			yrange_max, ymn, ymx;
	DATA_TYPE		context_type;
	CTime_base		current_time;
	List_iterator		it(ListOfEvents);
	event_node		*en;
	int width  = (int) (POS_EPSILON /*fix roundoff*/
		 + (device.width() / device.x_sc()));
	int height = (int) (POS_EPSILON /*fix roundoff*/
		+ (device.height() / device.y_sc()));

	if(event_x) delete event_x;
	if(event_y) delete event_y;
	event_x = NULL;
	event_y = NULL;
	event_length = ListOfEvents.get_length();

	if(event_length == 0) return;

	// debug
	// cerr << "MW_continuous::refresh_events(): event length = " << event_length << endl;

	i = 0;
	event_x = new double[event_length];
	event_y = new double[event_length];
	while((en = (event_node *) it())) {
		/* only x is normalized to [0, 1]; y will have to be
		 * compared against ymin, ymin + yspan in the drawing routine. */
		event_x[i] = (en->getetime() - startval) / spanval;
		event_y[i] = en->value;
		// debug
		// cerr << "   i = " << i << ", event_x = " << event_x[i] << ", event_y = " << event_y[i] << endl;
		i++; }

	eventvals 	= new sr_Rectangle[event_length];
	eventvals_style = new sr_DRAW_STYLE[event_length];


	// debug
	// cerr << "refresh_events: " << event_length << " point(s)\n";
	for(i = 0; i < event_length; i++) {
		int left_x = ((int) (width * event_x[i])) - RECT_HALF_WIDTH;
		int right_y = (int) floor(POS_EPSILON			/* fix roundoff */
				+ height * (((ymin + yspan) - event_y[i]) / yspan))
				- RECT_HALF_HEIGHT;

		// cerr << "    left_x = " << left_x << ", right_y = " << right_y << "; w,h = (" << width << ", " << height << ")\n";
		if(0 <= left_x && left_x <= width && 0 <= right_y && right_y <= height) {
			eventvals[nrect].x = left_x;
			eventvals[nrect].y = device.top() + right_y;
			//O'Reilly Vol II, p. 232, for XFillRectangle reason for "1 +":
			eventvals[nrect].width = 1 + 2 * RECT_HALF_WIDTH;
			eventvals[nrect].height = 1 + 2 * RECT_HALF_HEIGHT;
			nrect++; }
		// else {
		// 	cerr << "    out of bounds. damn.\n"; }
		}

	for(i = 0; i < nrect; i++) {
		device.fill_rectangle(
			eventvals[i],	// single rectangle
			fill_type);	// fill style (e.g. DIAGONAL)
		}
	delete [] eventvals;
	delete [] eventvals_style; }
