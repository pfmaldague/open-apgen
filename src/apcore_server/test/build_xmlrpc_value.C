#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <sstream>

#include <XmlRpc.H>
#include <UTL_time_base.H>

extern "C" {
#include <gdome_wrapper.h>
} // extern "C"

extern bool ap_verbose;
extern void build_children_of(XmlRpc::XmlRpcValue& val, GdomeNode* using_this_node, int indentation);

void build_xmlrpc_object(GdomeDocument* theDoc, GdomeElement* theEl, XmlRpc::XmlRpcValue& theExpression) {
	GdomeException		exc;
	GdomeNodeList*		theList0 = gdome_n_childNodes((GdomeNode*) theEl, &exc);
	nl_iterator		exprIterator = NULL_NL_ITERATOR;
	GdomeNode*		high_level_node;

	it_initialize(&exprIterator, theList0);
	while(high_level_node = it_next(&exprIterator)) {
	    if(gdome_n_nodeType(high_level_node, &exc) == GDOME_ELEMENT_NODE) {
		char*	high_level_name = get_tag(high_level_node);

		if(ap_verbose) {
			printf("high level node tag = %s\n", high_level_name); }

		if(!strcmp(high_level_name, "Expression")) {
		    // what we were hoping for...
		    GdomeNodeList*	theList1 = gdome_n_childNodes(high_level_node, &exc);
		    nl_iterator		level1Iterator = NULL_NL_ITERATOR;
		    GdomeNode*		node;

		    it_initialize(&level1Iterator, theList1);
		    while(node = it_next(&level1Iterator)) {
			if(gdome_n_nodeType(node, &exc) == GDOME_ELEMENT_NODE) {

			    if(ap_verbose) {
			    	printf("calling build_children_of at the top level for node %s...\n", get_tag(node)); }

			    build_children_of(theExpression, node, 0); } }
		    it_initialize(&level1Iterator, NULL); } } }
	if(ap_verbose) {
		std::ostringstream os;
		os << theExpression;
		std::string o(os.str());
		printf("Expression = %s\n", o.c_str()); }
	it_initialize(&exprIterator, NULL); }

void Indent(int k) {
	int i;
	for(i = 0; i < k; i++) {
		printf(" "); } }

void build_children_of(XmlRpc::XmlRpcValue& topVal, GdomeNode* node1, int I) {
	GdomeException	exc;
	char*	level1_name = get_tag(node1);
	int i;

	/* We expect that this expression will either
	 *	- be a simple expression like string, in which case we set topVal equal to it
	 *	- be an array or a struct, in which case we iterate over children and set topVal to the array or struct
	 */

	if(ap_verbose) {
		Indent(I);
		printf("node tag = %s\n", level1_name); }

	if(!strcmp(level1_name, "struct")) {
	    GdomeNodeList*	theList2 = gdome_n_childNodes(node1, &exc);
	    GdomeNode*		node2;
	    nl_iterator		level2Iter = NULL_NL_ITERATOR;

	    if(ap_verbose) {
		    Indent(I);
		    printf("got a struct\n"); }

	    it_initialize(&level2Iter, theList2);
	    while(node2 = it_next(&level2Iter)) {
		if(gdome_n_nodeType(node2, &exc) == GDOME_ELEMENT_NODE) {
		    char*	level2_name = get_tag(node2);

		    if(strcmp(level2_name, "element")) {
			printf("Error parsing struct node: didn't find tag \"element\".\n");
			return; }
		    else {
			// good
			GdomeNodeList*	theList3 = gdome_n_childNodes(node2, &exc);
			GdomeNode*	node3;
			nl_iterator	level3Iter = NULL_NL_ITERATOR;
			bool		gotTag = false;
			std::string	tag = "unknown";

			it_initialize(&level3Iter, theList3);
			while(node3 = it_next(&level3Iter)) {
			    char*	level3_name = get_tag(node3);

			    if(!gotTag) {
				if(!strcmp(level3_name, "ALPHA")) {
					// good
					tag = get_text(node3);
					if(ap_verbose) {
						Indent(I);
						printf("setting element tag to %s\n", tag.c_str()); }
					gotTag = true; }
				else {
					printf("Error parsing struct node: tag \"%s\" should be ALPHA.\n",
						level3_name);
					return; } }
			    else {
				XmlRpc::XmlRpcValue lowerVal;
				build_children_of(lowerVal, node3, I + 4);
				topVal[tag] = lowerVal; } }
			it_initialize(&level3Iter, NULL); } } }
	    it_initialize(&level2Iter, NULL); }
	else if(!strcmp(level1_name, "array")) {
	    GdomeNodeList*	theList2 = gdome_n_childNodes(node1, &exc);
	    GdomeNode*		node2;
	    nl_iterator		level2Iter = NULL_NL_ITERATOR;
	    int			array_index = 0;

	    if(ap_verbose) {
	    	Indent(I);
	    	printf("got an array\n"); }

	    it_initialize(&level2Iter, theList2);
	    while(node2 = it_next(&level2Iter)) {
		if(gdome_n_nodeType(node2, &exc) == GDOME_ELEMENT_NODE) {
		    char*	level2_name = get_tag(node2);
		    XmlRpc::XmlRpcValue	lowerVal;
		    build_children_of(lowerVal, node2, I + 4);

		    if(ap_verbose) {
			    Indent(I);
			    printf("setting array element %d\n", array_index); }
		    topVal[array_index++] = lowerVal; } }
	    it_initialize(&level2Iter, NULL); }
	else if(!strcmp(level1_name, "string")) {
		if(ap_verbose) {
			Indent(I);
			printf("got a string\n"); }
		topVal = get_text(node1); }
	else if(!strcmp(level1_name, "NUMBER")) {
		char*	theNumber = get_text(node1);

		if(index(theNumber, (int)'.')
		   || index(theNumber, 'E')
		   || index(theNumber, 'e')
		   || index(theNumber, 'D')
		   || index(theNumber, 'd')) {
			if(ap_verbose) {
				Indent(I);
				printf("got a float\n"); }
			topVal = atof(theNumber); }
		else {
			if(ap_verbose) {
				Indent(I);
				printf("got an int\n"); }
			topVal = atol(theNumber); } }
	else if(!strcmp(level1_name, "TIME")) {
		CTime_base	T(get_text(node1));
		if(ap_verbose) {
			Indent(I);
			printf("got a time\n"); }
		topVal[0] = T.get_seconds();
		topVal[1] = T.get_milliseconds(); }
	else if(!strcmp(level1_name, "DURATION")) {
		CTime_base	T(get_text(node1));
		if(ap_verbose) {
			Indent(I);
			printf("got a duration\n"); }
		topVal[0] = T.get_seconds();
		topVal[1] = T.get_milliseconds(); }
	else if(!strcmp(level1_name, "boolean")) {
		if(ap_verbose) {
			Indent(I);
			printf("got a boolean\n"); }
		if(!strcmp(get_text(node1), "true")) {
			topVal = true; }
		else {
			topVal = false; } }
	if(ap_verbose) {
		std::ostringstream os;
		os << topVal;
		std::string o(os.str());
		Indent(I);
		printf("build_children_of: returning topVal = %s\n", o.c_str()); } }
