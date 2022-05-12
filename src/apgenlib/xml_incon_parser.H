#ifndef _XML_INCON_PARSER_H_
#define _XML_INCON_PARSER_H_

#ifdef have_xmltol

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <libxml++/libxml++.h>

class xml_incon_parser {
private:
	xmlpp::DomParser		dom_parser;
	xmlpp::Document*		document;
	const xmlpp::Node*		top_level_obj;
	const xmlpp::Element*		resource;
	Cstring				resname;
	TypedValue			resvalue;
	TypedValue			set_to_value;
	TypedValue			all_globals;
	bool				use_resvalue_for_set_to_value;
	long				usage_depth;
	CTime_base			record_time;
	CTime_base			final_time_stamp;
	bool				final_time_stamp_was_defined;
	int				rec_indx;
	const xmlpp::Element*		topElement;
	xmlpp::Node::const_NodeList	list;
	xmlpp::Node::const_NodeList::iterator mainIterator;
public:
	xml_incon_parser(const char* s)
		: start_parsing_here(s),
			document(NULL),
			topElement(NULL),
			resource(NULL),
			rec_indx(0),
			usage_depth(0),
			final_time_stamp_was_defined(false),
			top_level_obj(NULL) {}

	~xml_incon_parser() {}

	const char*	start_parsing_here;
	Cstring		parser_name() { return "XML activity parser"; }

	// debug
	void		print_node(const xmlpp::Node* node, unsigned int indentation = 0);
	void		print_indentation(unsigned int indentation);
	void		parse();
			// normal return = true
	bool		verify_that_string_is_an_array(Cstring& errors);
	bool		iterate(bool& is_a_final_res_record, bool& is_a_global_record);

	CTime_base	final_time() { return final_time_stamp; }

	CTime_base*	time_tag() { return &record_time; }

	Cstring		res_get_name();
	TypedValue	res_get_value();
	TypedValue	res_get_set_to_value();
	long		res_get_usage_depth();
	bool		res_has_indices(pairslist& indices);

	TypedValue	get_all_globals();
	

	bool		can_extract_values() { return true; }
			// not in the API, for internal use only:
	void		get_typed_value(const xmlpp::Element*	stuff,
					const Glib::ustring&	nodename,
					TypedValue&		val);
	Cstring		get_node_text(const xmlpp::Element* node);
			// return NULL if none:
	const xmlpp::Element*	get_child_named(const xmlpp::Element*	parent,
					const char*	childname);
			// key is value as ID in this file, payload is ptr to value of instance variable
	tlist<alpha_string, Cntnr<alpha_string, TypedValue*> > instance_values;
	};

Cstring	xml_incon_parser::get_node_text(const xmlpp::Element* node) {
	const xmlpp::TextNode*		the_text = node->get_first_child_text();
	if(the_text) {
		string			content = the_text->get_content();
		return Cstring(content); }
	return ""; }

const xmlpp::Element*	xml_incon_parser::get_child_named(const xmlpp::Element*	parent,
						const char*	childname) {
	xmlpp::Node::const_NodeList		list = parent->get_children(childname);
	xmlpp::Node::const_NodeList::iterator iter = list.begin();
	const xmlpp::Node*			node = *iter;

	if(node) {
		const xmlpp::Element*	element = dynamic_cast<const xmlpp::Element*>(node);
		return element; }
	return NULL; }

void xml_incon_parser::print_indentation(unsigned int indentation)
{
  for(unsigned int i = 0; i < indentation; ++i)
    std::cout << " ";
}

void xml_incon_parser::print_node(const xmlpp::Node* node, unsigned int indentation) {
  std::cout << std::endl; //Separate nodes by an empty line.
  
  const xmlpp::ContentNode* nodeContent = dynamic_cast<const xmlpp::ContentNode*>(node);
  const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*>(node);
  const xmlpp::CommentNode* nodeComment = dynamic_cast<const xmlpp::CommentNode*>(node);

  if(nodeText && nodeText->is_white_space()) //Let's ignore the indenting - you don't always want to do this.
    return;
    
  const Glib::ustring nodename = node->get_name();

  if(!nodeText && !nodeComment && !nodename.empty()) //Let's not say "name: text".
  {
    print_indentation(indentation);

    const Glib::ustring namespace_prefix = node->get_namespace_prefix();
    if(namespace_prefix.empty())
      std::cout << "Node name = " << nodename << std::endl;
    else
      std::cout << "Node name = " << namespace_prefix << ":" << nodename << std::endl;
  }
  else if(nodeText) //Let's say when it's text. - e.g. let's say what that white space is.
  {
    print_indentation(indentation);
    std::cout << "Text Node" << std::endl;
  }

  const xmlpp::Element* nodeElement;

  //Treat the various node types differently: 
  if(nodeText)
  {
    print_indentation(indentation);
    std::cout << "text = \"" << nodeText->get_content() << "\"" << std::endl;
  }
  else if(nodeComment)
  {
    print_indentation(indentation);
    std::cout << "comment = " << nodeComment->get_content() << std::endl;
  }
  else if(nodeContent)
  {
    print_indentation(indentation);
    std::cout << "content = " << nodeContent->get_content() << std::endl;
  }
  else if((nodeElement = dynamic_cast<const xmlpp::Element*>(node)))
  {
    //A normal Element node:

    //line() works only for ElementNodes.
    print_indentation(indentation);
    std::cout << "     line = " << node->get_line() << std::endl;

    //Print attributes:
    const xmlpp::Element::const_AttributeList& attributes = nodeElement->get_attributes();
    for(xmlpp::Element::const_AttributeList::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter)
    {
      const xmlpp::Attribute* attribute = *iter;
      print_indentation(indentation);

      const Glib::ustring namespace_prefix = attribute->get_namespace_prefix();
      if(namespace_prefix.empty())
        std::cout << "  Attribute " << attribute->get_name() << " = " << attribute->get_value() << std::endl; 
      else
        std::cout << "  Attribute " << namespace_prefix  << ":" << attribute->get_name() << " = " << attribute->get_value() << std::endl;
    }

    const xmlpp::Attribute* attribute = nodeElement->get_attribute("title");
    if(attribute)
    {
      std::cout << "title found: =" << attribute->get_value() << std::endl;
    }
  }
  
  if(!nodeContent)
  {
    //Recurse through child nodes:
    xmlpp::Node::const_NodeList list = node->get_children();
    for(xmlpp::Node::const_NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
    {
      print_node(*iter, indentation + 2); //recursive
    }
  }
}



void xml_incon_parser::parse() {
	try {
		Glib::ustring	contents_to_parse(start_parsing_here);
		dom_parser.parse_memory(contents_to_parse); }
	catch(const std::exception& ex) {
		throw(eval_error(ex.what())); }
	top_level_obj = dom_parser.get_document()->get_root_node();
	if(!top_level_obj) {
		throw(eval_error("No top-level node not found")); }
	}

bool	xml_incon_parser::verify_that_string_is_an_array(Cstring& errors) {
	topElement = dynamic_cast<const xmlpp::Element*>(top_level_obj);
	if(topElement) {
		const Glib::ustring	nodename = top_level_obj->get_name();
		const Glib::ustring	namespace_prefix = top_level_obj->get_namespace_prefix();

		list = top_level_obj->get_children();
		if(namespace_prefix.empty()) {
			; }
		assert(nodename == "XML_TOL");
		mainIterator = list.begin();
		// debug
		// cout << "verify_that_string_is_an_array: success\n";
		return true; }
	errors = "top-level object is not an element";
	return false; }

bool	xml_incon_parser::iterate(bool&	finalRec, bool& globalRec) {
	const xmlpp::Node*	recordnode;
	const xmlpp::Node*	subnode;
	const xmlpp::Node*	indexnode;
	const xmlpp::Element*	recordelement;
	const xmlpp::Element*	indexelement;

	// debug
	// cout << "iterate(): rec_indx = " << rec_indx << "\n";

	while((mainIterator != list.end())) {
		recordnode = *mainIterator;
		recordelement = dynamic_cast<const xmlpp::Element*>(recordnode);
		if(recordelement) {
		    const Glib::ustring	nodename = recordnode->get_name();
		    // this is a TOLrecord or a ResourceMetadata element
		    if(nodename == "TOLrecord") {
			const Glib::ustring	namespace_prefix = recordnode->get_namespace_prefix();

			// find out if this is an activity
			
			const xmlpp::Attribute*	attribute = recordelement->get_attribute("type");

			if(!attribute) {
				throw(eval_error("Attribute 'type' not found in TOLrecord")); }

			// debug
			// cout << "iterate(): found attribute " << attribute->get_name() << ", value " << attribute->get_value() << "\n";

			const xmlpp::Element* timestamp = get_child_named(recordelement, "TimeStamp");
			assert(timestamp);

			Cstring	content = get_node_text(timestamp);
			record_time = CTime_base(content);
				

			// debug
			// cout << "Time stamp: " << content << "\n";

			if(attribute->get_value() == "RES_FINAL_VAL") {

				resource = get_child_named(recordelement, "Resource");
				assert(resource);
				resvalue.undefine();

				if(!final_time_stamp_was_defined) {
					final_time_stamp_was_defined = true;
					final_time_stamp = record_time; }

				xmlpp::Node::const_NodeList		res_children = resource->get_children();
				xmlpp::Node::const_NodeList::iterator	res_iter;

				use_resvalue_for_set_to_value = true;

				for(		res_iter = res_children.begin();
						res_iter != res_children.end();
						++res_iter) {
					const xmlpp::Element*	res_child = dynamic_cast<const xmlpp::Element*>(*res_iter);
					if(res_child) {
						const Glib::ustring&	tag = res_child->get_name();

						if(tag == "Name") {
							resname = get_node_text(res_child); }
						else if(tag == "Index") {
							; }
						else if(tag == "State") {
							const xmlpp::Attribute* state_state = res_child->get_attribute("res_state");

							if(!state_state) {
								throw(eval_error("Attribute 'res_state' not found in State subelement of RES_FINAL_VAL")); }
							if(state_state->get_value() == "USE_SET_PENDING") {
								const xmlpp::Attribute* usage_depth_att = res_child->get_attribute("usage_depth");
								const xmlpp::Element*	set_to_el = get_child_named(res_child, "ValueSetTo");
								xmlpp::Node::const_NodeList	set_to_list = set_to_el->get_children();
								xmlpp::Node::const_NodeList::iterator res_iter2;

								if(!usage_depth_att) {
									throw(eval_error("Attribute 'usage_depth' not found in State subelement of RES_FINAL_VAL")); }
								if(!set_to_el) {
									throw(eval_error("Attribute 'ValueSetTo' not found in State subelement of RES_FINAL_VAL")); }
								sscanf((char*) usage_depth_att->get_value().c_str(), "%ld", &usage_depth);
								for(	res_iter2 = set_to_list.begin();
									res_iter2 != set_to_list.end();
									res_iter2++) {
									const xmlpp::Element*	set_val_el = dynamic_cast<const xmlpp::Element*>(
													*res_iter2);
									if(set_val_el) {
										use_resvalue_for_set_to_value = false;
										get_typed_value(	set_val_el,
													set_val_el->get_name(),
													set_to_value); } } }
							else {
								set_to_value = resvalue; } }
						else {
							get_typed_value(res_child,
									tag,
									resvalue); } } }
				++mainIterator;
				rec_indx++;
				finalRec = true;
				globalRec = false;
				return true; } }
		    else if(nodename == "PropagatedGlobals") {
			xmlpp::Node::const_NodeList		glob_children = recordelement->get_children();
			xmlpp::Node::const_NodeList::iterator	glob_child_iter;
			ListOVal*			lov = new ListOVal;
			ArrayElement*			ae;

			for(	glob_child_iter = glob_children.begin();
				glob_child_iter != glob_children.end();
				++glob_child_iter) {
				const xmlpp::Element*	glob_child = dynamic_cast<const xmlpp::Element*>(*glob_child_iter);
				if(glob_child) {
					const xmlpp::Attribute*		glob_name_att = glob_child->get_attribute("Name");
					Cstring				globname = glob_name_att->get_value().c_str();
					xmlpp::Node::const_NodeList		glob_list = glob_child->get_children();
					xmlpp::Node::const_NodeList::iterator	glob_list_iter;

					for(	glob_list_iter = glob_list.begin();
						glob_list_iter != glob_list.end();
						glob_list_iter++) {
						const xmlpp::Element*	glob_value_el = dynamic_cast<const xmlpp::Element*>(*glob_list_iter);
						if(glob_value_el) {
							TypedValue	global_value;
							get_typed_value(	glob_value_el,
										glob_value_el->get_name(),
										global_value);
							// stick all globals into an array
							ae = new ArrayElement(globname, global_value.get_type());
							ae->SetVal(global_value);

							// debug
							// cout << "capturing global " << globname << "\n";

							lov->add(ae);
						}
					}
				}
			}
			all_globals = *lov;
			++mainIterator;
			rec_indx++;
			finalRec = false;
			globalRec = true;
			return true;
		    }
		}
		++mainIterator;
		rec_indx++;
	}
	finalRec = false;
	globalRec = false;
	return false;
}

Cstring	xml_incon_parser::res_get_name() {
	return resname;
}

TypedValue xml_incon_parser::res_get_value() {

	// debug
	// cout << "xml_incon_parser::res_get_value(" << resname << "): returning ";
	// Cstring temp;
	// resvalue.print(temp);
	// cout << temp << "\n";

	return resvalue;
}

long xml_incon_parser::res_get_usage_depth() {
	return usage_depth; }

TypedValue xml_incon_parser::get_all_globals() {
	return all_globals; }

TypedValue xml_incon_parser::res_get_set_to_value() {
	if(use_resvalue_for_set_to_value) {

		// debug
		// cout << "xml_incon_parser::res_get_set_to_value(" << resname << "): returning ";
		// Cstring temp;
		// resvalue.print(temp);
		// cout << temp << "\n";

		return resvalue; }
	else {

		// debug
		// cout << "xml_incon_parser::res_get_set_to_value(" << resname << "): returning ";
		// Cstring temp;
		// set_to_value.print(temp);
		// cout << temp << "\n";

		return set_to_value; } }

bool	xml_incon_parser::res_has_indices(pairslist& indices) {
	/* find out whether there are any Index or
	 * Indices */
	xmlpp::Node::const_NodeList		nodelist = resource->get_children("Index");
	xmlpp::Node::const_NodeList::iterator	nodeiter;
	int				levels = 0;

	// should get all levels
	for(nodeiter = nodelist.begin(); nodeiter != nodelist.end(); nodeiter++) {
		const xmlpp::Node* indexnode = *nodeiter;
		const xmlpp::Element* indexelement = dynamic_cast<const xmlpp::Element*>(indexnode);
		if(indexelement) {
			const xmlpp::Attribute*	indexattribute = indexelement->get_attribute("level");
			assert(indexattribute);
			indices << new symNode(indexattribute->get_value().c_str(), get_node_text(indexelement));
			levels++; } }
	// if(!levels) {
		// debug
		// cout << "scalar resource - ";
	// 	}
	return levels > 0; }

	/* things to get:
	 *
	 *	ID
	 *	Name
	 *	Type
	 *	Parent
	 *	Visibility
	 *	Attributes
	 *	Parameters
	 */

void	xml_incon_parser::get_typed_value(
		const xmlpp::Element*	stuff,
		const Glib::ustring&	nodename,
		TypedValue&		val) {
	if(nodename == "BooleanValue") {
		val.declared_type = apgen::DATA_TYPE::BOOL_TYPE;
		if(get_node_text(stuff) == "true" || get_node_text(stuff) == "TRUE") {
			val = (long) 1; }
		else {
			val = (long) 0; } }
	else if(nodename == "DoubleValue") {
		double	D;
		Cstring	d = get_node_text(stuff);
		sscanf(*d, "%lf", &D);
		val.declared_type = apgen::DATA_TYPE::FLOATING;
		val = D; }
	else if(nodename == "InstanceValue") {
		/* The challenge here is that we need a pointer to a live
		 * instance, but such pointers won't be available till later,
		 * when activities are actually constructed. And, by the way,
		 * these activities may have IDs that differ from the ID
		 * stated in the file being ingested.
		 *
		 * Proposed solution: keep pointers to the typed values that
		 * reference instances, and finalize their values in a loop
		 * that is run after the actual instances have been created.
		 * This, by the way, will require a "dictionary" that explains
		 * the link between the old IDs and the new.
		 */
		Cstring	s = get_node_text(stuff);
		val.declared_type = apgen::DATA_TYPE::INSTANCE;
		val = s;
		instance_values << new Cntnr<alpha_string, TypedValue*>(s, &val); }
	else if(nodename == "IntegerValue") {
		long	L;
		Cstring	l = get_node_text(stuff);
		sscanf(*l, "%ld", &L);
		val.declared_type = apgen::DATA_TYPE::INTEGER;
		val = L; }
	else if(nodename == "StringValue") {
		Cstring	s = get_node_text(stuff);
		val.declared_type = apgen::DATA_TYPE::STRING;
		val = s; }
	else if(nodename == "DurationValue") {
		Cstring		d = get_node_text(stuff);
		CTime_base	T(*d);
		val.declared_type = apgen::DATA_TYPE::DURATION;
		val = T; }
	else if(nodename == "TimeValue") {
		Cstring		d = get_node_text(stuff);
		CTime_base	T(*d);
		val.declared_type = apgen::DATA_TYPE::TIME;
		val = T; }
	else if(nodename == "ListValue") {
		xmlpp::Node::const_NodeList			list_kids = stuff->get_children("Element");
		TypedValue				child_val;
		ListOVal*				lov = new ListOVal;
		long					i = 0;
		ArrayElement*				ae;
		Cstring					Err;
		val.declared_type = apgen::DATA_TYPE::ARRAY;
		lov->set_array_type(TypedValue::arrayType::LIST_STYLE);
		val = *lov;
		for(	xmlpp::Node::const_NodeList::iterator list_iter = list_kids.begin();
			list_iter != list_kids.end();
			list_iter++) {
			const xmlpp::Element*	one_el = dynamic_cast<const xmlpp::Element*>(*list_iter);
			xmlpp::Node::const_NodeList	list_grandkids = one_el->get_children();

			for(	xmlpp::Node::const_NodeList::iterator	list_granditer = list_grandkids.begin();
				list_granditer != list_grandkids.end();
				list_granditer++) {
				const xmlpp::Element* subel = dynamic_cast<const xmlpp::Element*>(*list_granditer);
				// Note: only one child expected
				if(subel) {
					get_typed_value(subel, subel->get_name(), child_val);
					lov->add(ae = new ArrayElement(i));
					ae->set_typed_val(child_val);
					i++;
				}
			}
		}
	} else if(nodename == "StructValue") {
		xmlpp::Node::const_NodeList			list_kids = stuff->get_children("Element");
		TypedValue				child_val;
		ListOVal*				lov = new ListOVal;
		ArrayElement*				ae;
		Cstring					Err;


		val.declared_type = apgen::DATA_TYPE::ARRAY;
		lov->set_array_type(TypedValue::arrayType::STRUCT_STYLE);
		val = *lov;
		for(	xmlpp::Node::const_NodeList::iterator list_iter = list_kids.begin();
			list_iter != list_kids.end();
			list_iter++) {
			const xmlpp::Element* one_el = dynamic_cast<const xmlpp::Element*>(*list_iter);
    			const xmlpp::Attribute*	attribute = one_el->get_attribute("index");
			Cstring				attval(attribute->get_value());
			xmlpp::Node::const_NodeList		list_grandkids = one_el->get_children();

			for(	xmlpp::Node::const_NodeList::iterator	list_granditer = list_grandkids.begin();
				list_granditer != list_grandkids.end();
				list_granditer++) {
				const xmlpp::Element* subel = dynamic_cast<const xmlpp::Element*>(*list_granditer);
				// Note: only one child expected
				if(subel) {
					get_typed_value(subel, subel->get_name(), child_val);
					lov->add(ae = new ArrayElement(attval, child_val.get_type()));
					ae->set_typed_val(child_val);
				}
			}
		}
	} else if(nodename == "EmptyListValue") {
		ListOVal*				lov = new ListOVal;
		val.declared_type = apgen::DATA_TYPE::ARRAY;
		val = *lov;
	} else {
		Cstring temp("Don't know what to do with array child element \"");
		temp << nodename.c_str() << "\"";
		throw(eval_error(temp));
	}
}

#endif /* have_xmltol */

#endif /* _XML_INCON_PARSER_H_ */
