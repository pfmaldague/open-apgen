#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "gtk_bridge.H"

int gS::de_facto_string::transfer_to_stream(buf_struct *s , compact_int &MaxLength ) {
	int		l = contents.length();

	if( MaxLength < l ) {
		char	*c = ( char * ) malloc( 1 + (int)MaxLength );

		strncpy( c , contents.c_str() , (int)MaxLength );
		c[(int)MaxLength] = '\0';
		concatenate(s, c);
		free(c);
		MaxLength = 0;
		return 1; }
	concatenate(s, contents.c_str());
	MaxLength -= l;
	return 0; }

gS::indexed_string_or_list	*gS::de_facto_string::copy() const {
	return new de_facto_string(	get_key(),
					get_descr(),
					Range,
					get_string() ); }

gS::de_facto_array::~de_facto_array() {
	gtk_editor_array::iterator i;
	contents.erase(contents.begin(), contents.end()); }

int gS::de_facto_array::length() {
	int				L = 2; // "[]"
	gtk_editor_array::iterator	k;

	for(k = contents.begin(); k != contents.end(); k++) {
		L += (*k)->get_key().length();
		L += 3; // " = "
		L += (*k)->length() ; }
	return L ; }

	// returns 1 if truncated
int gS::de_facto_array::transfer_to_stream(buf_struct *B, compact_int &MaxLength) {
	int				L = length() ;
	gtk_editor_array::iterator	k ;

	if(MaxLength <= 0) return 1;
	concatenate(B, "[");
	MaxLength--;
	for(k = contents.begin(); k != contents.end() && MaxLength > 0;) {
		if(has_labels()) {
			string		aLabel(add_quotes_to((*k)->get_key().c_str()));
			int		l = aLabel.length();

			if(MaxLength < l) {
				char	*c = (char *) malloc(1 + (int)MaxLength);

				strncpy(c, aLabel.c_str(), (int)MaxLength);
				c[(int)MaxLength] = '\0';
				concatenate(B, c);
				free(c);
				return 1; }
			concatenate(B, aLabel.c_str());
			MaxLength -= l ;
			if(MaxLength < 3) {
				char	*c = ( char * ) malloc( 1 + (int)MaxLength ) ;

				strncpy( c , " = " , (int)MaxLength ) ;
				c[(int)MaxLength] = '\0' ;
				concatenate(B, c);
				free(c);
				return 1 ; }
			concatenate(B, " = ");
			MaxLength -= 3 ; }
		if((*k)->transfer_to_stream(B, MaxLength)) {
			return 1; }
		k++;
		if(k != contents.end()) {
			if( MaxLength < 2 ) {
				char	*c = ( char * ) malloc( 1 + (int)MaxLength ) ;

				strncpy(c, ", ", (int)MaxLength);
				c[(int)MaxLength] = '\0' ;
				concatenate(B, c);
				free(c);
				return 1 ; }
			concatenate(B, ", ");
			MaxLength -= 2 ; } }
	if( MaxLength <= 0 ) {
		return 1 ; }
	concatenate(B, "]");
	MaxLength-- ;
	return 0 ; }

gS::indexed_string_or_list *gS::de_facto_array::copy() const {
	de_facto_array			*d = new de_facto_array(
						get_key(),
						get_descr(),
						Range,
						has_labels() );
	gtk_editor_array::const_iterator	k = contents.begin();

	for(	k = contents.begin();
		k != contents.end();
		k++ ) {
		indexed_string_or_list* element_copy = (*k)->copy();
		d->contents.push_back(gS::ISL(element_copy));
	}
	return d;
}

editor_intfc &editor_intfc::get_interface() {
	static editor_intfc theInterface;
	return theInterface; }

string	gS::add_quotes_to(const string &s) {
	char		* t;
	int		i;
	int		j = -1;
	int		quote_count = 0;
	int		l = s.length();
	string		S;


	for(i = 0; i < l; i++ ) {
		if( s[i] == '\"' ) quote_count++;
		else if( s[i] == '\\' ) quote_count++;
		else if( s[i] == '\n' ) quote_count += 3; }

	t = (char *) malloc(s.length() + quote_count + 3);
	t[++j] = '\"';
	i = 0;
	while (i < l) {
		if(s[i] == '\"') {
			t[++j] = '\\';
			t[++j] = '\"'; }
		else if(s[i] == '\\') {
			t[++j] = '\\';
			t[++j] = '\\'; }
		else if(s[i] == '\n') {
			t[++j] = '\\';
			t[++j] = 'n';
			// add an escaped newline for readability:
			t[++j] = '\\';
			t[++j] = '\n'; }
		else {
			t[++j] = s[i]; }
		i++; }
	t[++j] = '\"';
	t[++j] = '\0';
	S = t;
	free((char *) t);
	return S; }

void gS::remove_quotes_from(string &s) {
	int	l = s.length();
	char	*t = (char *) malloc(l + 1);
	int	i = -1;
	int	j = 0;
	int	state = 0;

	while(1) {
		if( ++i == l ) break;
		if( state == 0 ) {
			if( s[i] == '"' )
				;
			else if( s[i] == '\\' )
				state = 1;	// escape state
			else
				t[j++] = s[i]; }
		else if( state == 1 )		// escape state
			{
			if( s[i] == '\"' ) t[j++] = '\"';
			else if( s[i] == 'n' ) t[j++] = '\n';
			else if( s[i] == 't' ) t[j++] = '\t';
			else if( s[i] == '\n' );
			else t[j++] = s[i];
			state = 0; } }
	t[j] = '\0';
	s = t;
	free( t );
	}


