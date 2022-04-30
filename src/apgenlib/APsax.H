#ifndef _AP_SAX_H_
#define _AP_SAX_H_

#include <stdio.h>
#include <assert.h>

#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "flexval.H"

using namespace std;

class sax_parser {
  public:
    typedef enum {
	IDLE,		// ^<foo>Hello world</foo>
	LESS_THAN,	// <^foo>Hello world</foo> or <foo>Hello world<^/foo> 
	START_TAG,	// <f^oo>Hello world</foo>
	TAG_CONTENT,	// <foo>^Hello world</foo>
	NEW_TAG,	// <foo>Hello world<^/foo> 
	END_TAG,	// <foo>Hello world</^foo>
	SELF_TAG,	// <foo/^>
	EXCLAM,		// <!^-- this is a comment -->
	EXCLAM_MINUS,	// <!-^- this is a comment -->
	COMMENT,	// <!--^ this is a comment -->
	MINUS,		// <!-- this is a comment -^->
	MINUS2,		// <!-- this is a comment --^>
	QUESTION,	// <?^xml version="1.0"?>
	QUESTION2,	// <?xml version="1.0"?^>
	ATT,		// <foo ^complexity="trivial">Hello world</foo>
	ATT_NAME,	// <foo c^omplexity="trivial">Hello world</foo>
	ATT_EQUAL,	// <foo complexity=^"trivial">Hello world</foo>
	ATT_VALUE,	// <foo complexity="^trivial">Hello world</foo>
	ATT_END,	// <foo complexity="trivial"^>Hello world</foo>
	CDATA_START,	// <![^CDATA[ characters with markup ]]>
	CDATA,		// <![CDATA[^ characters with markup ]]>
	CDATA_END,	// <![CDATA[ characters with markup ]^]>
	CDATA_END2	// <![CDATA[ characters with markup ]]^>
	} State;

    class exception {
      public:
	string		message;
	int		line;
	int		col;
	exception(const string& msg, int l, int c)
		: message(msg), line(l), col(c) {}
	~exception() {} };

    static const char* print_state(State x) {
	switch(x) {
		case IDLE:
			return "IDLE";
		case LESS_THAN:
			return "LESS_THAN";
		case START_TAG:
			return "START_TAG";
		case TAG_CONTENT:
			return "TAG_CONTENT";
		case NEW_TAG:
			return "NEW_TAG";
		case END_TAG:
			return "END_TAG";
		case SELF_TAG:
			return "SELF_TAG";
		case EXCLAM:
			return "EXCLAM";
		case EXCLAM_MINUS:
			return "EXCLAM_MINUS";
		case COMMENT:
			return "COMMENT";
		case MINUS:
			return "MINUS";
		case MINUS2:
			return "MINUS2";
		case QUESTION:
			return "QUESTION";
		case QUESTION2:
			return "QUESTION2";
		case ATT:
			return "ATT";
		case ATT_NAME:
			return "ATT_NAME";
		case ATT_EQUAL:
			return "ATT_EQUAL";
		case ATT_VALUE:
			return "ATT_VALUE";
		case ATT_END:
			return "ATT_END";
		case CDATA_START:
			return "CDATA_START";
		case CDATA:
			return "CDATA";
		case CDATA_END:
			return "CDATA_END";
		case CDATA_END2:
			return "CDATA_END2";
		defaut:
			return "UNKNOWN";
		}
	}

    		/* S(i) should return the i-th character - but note
		 * that S is called for strictly increasing values
		 * of i, starting with 0, so i is really redundant.
		 * It's there as a convenience if you have a const
		 * char* Foo containing a string to parse, in which
		 * case you can implement S as
		 *
		 * int	S(int i) {
		 * 	return Foo[i];
		 * 	}
		 *
		 * The most important case is that in which S reads
		 * chars from a huge XML file.  In that case you may
		 * want to implement S as
		 * int	S(int) {
		 *	char x;
		 *	size_t l = fread(&c, 1, 1, F);
		 *	return l ? c : 0;
		 *	}
		 * where F* is a FILE* pointing to the huge XML file.
		 * This way, your program won't need to store the
		 * huge XML file internally. */
    void	parse(int (*S)(long));

    void adjust_elements_to(int);
    void adjust_attributes_to(int);

    sax_parser() : line(0), col(0) {}
    virtual ~sax_parser() {}

    virtual void			on_start_document();
    virtual void			on_end_document();
    virtual void			on_start_element(
						const string& el,
						const vector<pair<string, string> >& atts_in_order,
						const map<string, string>& atts);
    virtual void			on_end_element(const string& el);
    virtual void			on_characters(const string&);

    vector<string>&			startElements();
    vector<string>&			endElements();
    vector<pair<string, string> >&	AttributesInOrder();
    map<string, string>&		Attributes();
    stringstream&			currentElement();
    stringstream&			currentAttributeName();
    stringstream&			currentAttributeValue();
    vector<string>&			AttributeNames();
    vector<string>&			AttributeValues();
    stringstream&			Text();

    string				process_text();
    void				grabAttributes();

    int					line;
    int					col;
};

class XmlTolSaxParser : public sax_parser {
public:
	XmlTolSaxParser();
	virtual ~XmlTolSaxParser();

	//overrides:
	virtual void on_start_document();
	virtual void on_end_document();
	virtual void on_start_element(
			const string& name,
		        const vector<pair<string, string> >&	attributes_in_order,
			const map<string, string>&		attributes);
	virtual void on_end_element(const string& name);
	virtual void on_characters(const string& characters);
	// virtual void on_comment(const Glib::ustring& text);
	// virtual void on_warning(const Glib::ustring& text);
	// virtual void on_error(const Glib::ustring& text);
	// virtual void on_fatal_error(const Glib::ustring& text);

	void at_end_of_tol_record();

	vector<flexval>			curval;
	vector<flexval>			curindex;
	vector<string>			compoundType;
	int				cur_depth;
	map<string, bool>		inElement;

	list<pair<string, flexval> >	instanceAttributes;
	list<pair<string, flexval> >	parameters;

	bool		context_includes(const string&);

	vector<string>	resIndices;
	stringstream	fullResName;
	string		resInterpolation;
	string		resDataType;
	string		resName;
	string		actName;
	string		actID;
	string		actType;
	string		actParent;
	string		actVisibility;
	string		attributeName;
	string		parameterName;
	int  		index_level;

	string		recordType;

	bool		reached_the_end;
	CTime_base	record_time;

	bool		error_found;
	string		error_message;

	void		resize_all(int maxindex);
	void		plan_building_thread_waits_for_main_parser_to_get_to_end_of_record(bool last_time = false);
	void		main_process_waits_for_thread_to_iterate(bool first_time = false);

	string		indent_by_depth() {

		stringstream s;
		for(int i = 0; i < cur_depth; i++) {
			s << " ";
		}
		return s.str();
	}
};

#endif /* _AP_SAX_H_ */
