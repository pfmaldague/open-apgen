#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <APsax.H>
#include <string.h>

using namespace std;

#define MAKE_BASE_VIRTUAL_METHODS_VERBOSE

// for testing as a standalone program (compile with g++):
// #define INCLUDE_MAIN

int is_whitespace(int c) {
	if(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
		return 1; }
	return 0; }

void sax_parser::on_start_element(
		const string& el,
		const vector<pair<string, string> >& atts_in_order,
		const map<string, string>& atts) {
#	ifdef MAKE_BASE_VIRTUAL_METHODS_VERBOSE
	cout << "on_start_element(" << el << "): atts =\n";
	for(int i = 0; i < atts_in_order.size(); i++) {
		cout << "\t" << atts_in_order[i].first << " = "
			<< atts_in_order[i].second << "\n";
	}
#	endif /* MAKE_BASE_VIRTUAL_METHODS_VERBOSE */
}

void sax_parser::on_end_element(
		const string& el) {
#	ifdef MAKE_BASE_VIRTUAL_METHODS_VERBOSE
	cout << "on_end_element(" << el << ")\n";
#	endif /* MAKE_BASE_VIRTUAL_METHODS_VERBOSE */
}

void sax_parser::on_characters(
		const string& text) {
#	ifdef MAKE_BASE_VIRTUAL_METHODS_VERBOSE
	cout << "on_characters: text = " << text << "\n";
#	endif /* MAKE_BASE_VIRTUAL_METHODS_VERBOSE */
}

string	sax_parser::process_text() {
	string inbuf = Text().str();
	const char* s = inbuf.c_str();

	static char	Amp[] = "&amp;";
	static char	Gt[] = "&gt;";
	static char	Lt[] = "&lt;";
	static char	Quot[] = "&quot;";
	static char	Apo[] = "&apo;";

	const char* t = s;
	stringstream out;
	while(*t) {
		if(*t == '&') {
			if(!strncmp(t, Amp, strlen(Amp))) {
				t += strlen(Amp);
				out << '&';
			} else if(!strncmp(t, Gt, strlen(Gt))) {
				t += strlen(Gt);
				out << '>';
			} else if(!strncmp(t, Lt, strlen(Lt))) {
				t += strlen(Lt);
				out << '<';
			} else if(!strncmp(t, Quot, strlen(Quot))) {
				t += strlen(Quot);
				out << '"';
			} else if(!strncmp(t, Apo, strlen(Apo))) {
				t += strlen(Apo);
				out << '\'';
			} else {
				throw(exception("malformed entity", 0, 0));
			}
		} else {
			out << *t;
			t++;
		}
	}
	return out.str();
}

void sax_parser::on_start_document() {
#	ifdef MAKE_BASE_VIRTUAL_METHODS_VERBOSE
	cout << "on_start_document()\n";
#	endif /* MAKE_BASE_VIRTUAL_METHODS_VERBOSE */
}

void sax_parser::on_end_document() {
#	ifdef MAKE_BASE_VIRTUAL_METHODS_VERBOSE
	cout << "on_end_document()\n";
#	endif /* MAKE_BASE_VIRTUAL_METHODS_VERBOSE */
}

static stringstream& buffer() {
	static stringstream B;
	return B;
}

// void grab(char c) {
// 	buffer() << c;
// }

vector<string>&			sax_parser::startElements() {
	static vector<string>	V;
	return V;
}

vector<string>&			sax_parser::endElements() {
	static vector<string>	V;
	return V;
}

vector<pair<string, string> >&	sax_parser::AttributesInOrder() {
	static vector<pair<string, string> > V;
	return V;
}

map<string, string>&		sax_parser::Attributes() {
	static map<string, string>	M;
	return M;
}

stringstream&			sax_parser::currentElement() {
	static stringstream	s;
	return s;
}

stringstream&			sax_parser::currentAttributeName() {
	static stringstream	s;
	return s;
}

stringstream&			sax_parser::currentAttributeValue() {
	static stringstream	s;
	return s;
}

vector<string>&			sax_parser::AttributeNames() {
	static vector<string>	V;
	return V;
}

vector<string>&			sax_parser::AttributeValues() {
	static vector<string>	V;
	return V;
}

stringstream&			sax_parser::Text() {
	static stringstream	s;
	return s;
}

void sax_parser::adjust_elements_to(int level) {
	if(startElements().size() <= level) {
		startElements().resize(level + 1);
	}
	if(endElements().size() <= level) {
		endElements().resize(level + 1);
	}
}

void sax_parser::adjust_attributes_to(int level) {
	if(AttributeNames().size() <= level) {
		AttributeNames().resize(level + 1);
	}
	if(AttributeValues().size() <= level) {
		AttributeValues().resize(level + 1);
	}
}

void sax_parser::grabAttributes() {
	Attributes().clear();
	AttributesInOrder().clear();
	vector<string>::iterator	Name = AttributeNames().begin();
	vector<string>::iterator	Value = AttributeValues().begin();
	for(	;
		Name != AttributeNames().end() && Value != AttributeValues().end();
		Name++, Value++) {
		AttributesInOrder().push_back(pair<string, string>(*Name, *Value));
		Attributes()[*Name] = *Value;
	}
}

void sax_parser::parse(
		int (*S)(long)) {
	char		c;
	long		where = 0;
	State		state = IDLE;
	int		level = -1;
	int		attribute_count = 0;

#	ifdef SAX_DEBUG
	stringstream	sofar;
#	endif /* SAX_DEBUG */

	line = 1;
	col = 1;
	on_start_document();
	while((c = S(where++)) != '\0') {
		col++;
		if(c == '\n') {
			line++;
			col = 1;
		}

#		ifdef SAX_DEBUG
		sofar << c;
#		endif /* SAX_DEBUG */

		if(state == IDLE) {
			/* parsing the start of a tag */
			if(c == '<') {
				state = LESS_THAN;
			} else if(is_whitespace(c)) {
#				ifdef SAX_DEBUG
				sofar.str("");
#				endif /* SAX_DEBUG */
			} else {
				stringstream s;
				s << "Bizarre character " << c << ", was expecting <.\n";
				throw(exception(s.str(), line, col));
			}
		} else if(state == LESS_THAN) {
			/* parsing the 2nd char of a tag */
			if(c == '/') {
				state = END_TAG;
				currentElement().str("");
			} else if(c == '!') {
				state = EXCLAM;
			} else if(c == '?') {
				state = QUESTION;
			} else {
				state = START_TAG;
				adjust_elements_to(++level);
				currentElement().str("");
				currentElement() << c;
			}
		} else if(state == QUESTION) {
			if(c == '?') {
				state = QUESTION2;
			}
		} else if(state == QUESTION2) {
			if(c == '>') {
				state = IDLE;
			} else {
				throw(exception("malformed prolog", line, col));
			}
		} else if(state == START_TAG) {
			/* parsing the element name of a start tag */
			if(c == '/') {
				state = SELF_TAG;
				startElements()[level] = currentElement().str();
				if(!startElements()[level].size()) {
					throw(exception("missing element name", line, col));
				}
				endElements()[level] = currentElement().str();
				grabAttributes();
				on_start_element(
					startElements()[level],
					AttributesInOrder(),
					Attributes());
				on_end_element(endElements()[level]);
				level--;
			} else if(c == '>') {
				startElements()[level] = currentElement().str();
				if(!startElements()[level].size()) {
					throw(exception("missing element name", line, col)); }
				state = TAG_CONTENT;
				Text().str("");
				on_start_element(
					startElements()[level],
					AttributesInOrder(),
					Attributes());
			} else if(is_whitespace(c)) {
				state = ATT;
				startElements()[level] = currentElement().str();
				currentAttributeName().str("");
				attribute_count = 0;
			} else {
				currentElement() << c;
			}
		} else if(state == ATT) {
			if(is_whitespace(c)) {
			} else if(c == '>') {
				state = TAG_CONTENT;
				grabAttributes();
				on_start_element(
					startElements()[level],
					AttributesInOrder(),
					Attributes());
				Text().str("");
			} else if(c == '/') {
				state = SELF_TAG;
				grabAttributes();
				on_start_element(
					startElements()[level],
					AttributesInOrder(),
					Attributes());
				endElements()[level] = startElements()[level];
				on_end_element(endElements()[level]);
				level--;
			} else {
				state = ATT_NAME;
				currentAttributeName().str("");
				currentAttributeName() << c;
			}
		} else if(state == ATT_NAME) {
			if(c == '=') {
				string att(currentAttributeName().str());
				adjust_attributes_to(attribute_count);
				AttributeNames()[attribute_count] = att;
				currentAttributeName().str("");
				state = ATT_EQUAL;
			} else {
				currentAttributeName() << c;
			}
		} else if(state == ATT_EQUAL) {
			if(c == '"') {
				currentAttributeValue().str("");
				state = ATT_VALUE;
			} else {
				throw(exception("malformed attribute", line, col));
			}
		} else if(state == ATT_VALUE) {
			if(c == '"') {
				adjust_attributes_to(attribute_count);
				AttributeValues()[attribute_count] = currentAttributeValue().str();
				attribute_count++;
				state = ATT;
			} else {
				currentAttributeValue() << c;
			}
		} else if(state == SELF_TAG) {
			/* parsing the last char of a self-contained tag */
			if(c == '>') {
				state = IDLE;
			} else {
				stringstream s;
				s << "Bizarre char " << (char)c << ", was expecting end of tag.\n";
				throw(exception(s.str(), line, col));
			}
		} else if(state == TAG_CONTENT) {
			/* parsing text or the start/end of an element tag */
			if(c == '<') {
				try { on_characters(process_text()); }
				catch(exception Err) {
					throw(exception(Err.message, line, col)); }
				state = NEW_TAG;
			} else {
				Text() << c;
			}
		} else if(state == END_TAG) {
			/* reading the element name of an end tag */
			if(c == '>')  {
				endElements()[level] = currentElement().str();
				if(endElements()[level] != startElements()[level]) {
					stringstream s;
					s << "tag mismatch at level " << level << ": start = "
						<< startElements()[level] << ", end = "
						<< endElements()[level];
					throw(exception(s.str(), line, col)); }
				on_end_element(endElements()[level]);
				level--;
				state = IDLE;
			} else {
				currentElement() << c;
			}
		} else if(state == NEW_TAG) {
			/* reading the 2nd char of a start or end tag */
			if(c == '/') {
				currentElement().str("");
				state = END_TAG;
			} else if(c == '!') {
				state = EXCLAM;
			} else {
				adjust_elements_to(++level);
				currentElement().str("");
				currentElement() << c;
				state = START_TAG;
			}
		} else if(state == EXCLAM) {
			if(c == '[') {
				state = CDATA_START;
			} else if(c == '-') {
				state = EXCLAM_MINUS;
			}
		} else if(state == EXCLAM_MINUS) {
			if(c == '-') {
				state = COMMENT;
			} else {
				throw(exception("malformed comment", line, col));
			}
		} else if(state == COMMENT) {
			if(c == '-') {
				state = MINUS;
			}
		} else if(state == MINUS) {
			if(c == '-') {
				state = MINUS2;
			} else {
				throw(exception("malformed comment", line, col));
			}
		} else if(state == MINUS2) {
			if(c == '>') {
				state = IDLE;
			} else {
				throw(exception("malformed comment", line, col));
			}
		} else if(state == CDATA_START) {
			if(c == '[') {
				state = CDATA;
			}
			// else we are presumably scanning the CDATA keyword
		} else if(state == CDATA) {
			if(c == ']') {
				state = CDATA_END;
			}
			// else we are presumably scanning the CDATA content
		} else if(state == CDATA_END) {
			if(c == ']') {
				state = CDATA_END2;
			} else {
				throw(exception("malformed CDATA section", line, col));
			}
		} else if(state == CDATA_END2) {
			if(c == '>') {
				state = IDLE;
			} else {
				throw(exception("malformed CDATA section", line, col));
			}
		}

#		ifdef SAX_DEBUG
		string Sofar(sofar.str());
		cout << Sofar << " - state = " << print_state(state) << "\n";
		sofar.str("");
		sofar << Sofar;
#		endif /* SAX_DEBUG */

	}
	on_end_document();
}

#ifdef INCLUDE_MAIN

int getchar_wrapper(int) {
	return getchar();
}

int main(int argc, char* argv[]) {
	sax_parser	Parser;

	try {
		Parser.parse(getchar_wrapper);
	}
	catch(sax_parser::exception Err) {
		cerr << "Parsing error: " << Err.message << " on line "
			<< Err.line << ", col " << Err.col << "\n";
	}
	return 0;
}
#endif /* INCLUDE_MAIN */
