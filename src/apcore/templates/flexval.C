#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef have_xmltol
#include <libxml++/libxml++.h>
#endif

#include "flexval.H"
extern "C" {
#include "App_concat_util.h"
}

#include "json.h"

void flexval::invalidate() {
    switch(_type) {
	case TypeArray: delete _value.asArray; break;
	case TypeStruct: delete _value.asStruct; break;
	case TypeString: delete _value.asString; break;
	case TypeTime: delete _value.asTime; break;
	case TypeDuration: delete _value.asDuration; break;
	default: break;
    }
    _type = TypeInvalid;
    _value.asArray = NULL;
}

void flexval::assertArray(int size) const {
    if (_type != TypeArray)
      throw eval_error("type error: expected an array");
    else if (int(_value.asArray->size()) < size)
      throw eval_error("range error: array index too large");
}


void flexval::assertArray(int size) {
    if (_type == TypeInvalid) {
      _type = TypeArray;
      _value.asArray = new ValueArray(size);
    } else if (_type == TypeArray) {
      if (int(_value.asArray->size()) < size)
        _value.asArray->resize(size);
    } else
      throw eval_error("type error: expected an array");
}

void flexval::assertStruct() {
    if (_type == TypeInvalid) {
      _type = TypeStruct;
      _value.asStruct = new ValueStruct();
    } else if (_type != TypeStruct)
      throw eval_error("type error: expected a struct");
}

void flexval::assertStruct() const {
    if (_type != TypeStruct) {
      throw eval_error("type error: expected a struct");
    }
}

void flexval::assertTypeOrInvalid(flexval::Type t) {
    if (_type == TypeInvalid) {
	_type = t;
	switch(_type) {
		case TypeBool: _value.asBool = false; break;
		case TypeInt: _value.asInt = 0; break;
		case TypeDouble: _value.asDouble = 0.0; break;
		case TypeString: _value.asString = new string; break;
		case TypeTime: _value.asTime = new cppTime; break;
		case TypeDuration: _value.asDuration = new cppDuration; break;
		default: _value.asInt = 0; break;
	}
    } else if (_type != t) {
        throw eval_error(string("type error: got ") + type2string(_type) + ", expected "
		+ type2string(t));
    }
}

void flexval::assertTypeOrInvalid(flexval::Type t) const {
    if (_type != t) {
        throw eval_error(string("type error: got ") + type2string(_type) + ", expected "
		+ type2string(t));
    }
}

const char* flexval::type2string(flexval::Type t) {
      switch(t) {
	case TypeBool: return "boolean";
	case TypeInt: return "integer";
	case TypeDouble: return "double";
	case TypeString: return "string";
	case TypeTime: return "time";
	case TypeDuration: return "duration";
	case TypeArray: return "array";
	case TypeStruct: return "struct";
	default: return "undefined";
      }
    return "undefined";
}

flexval& flexval::operator=(const flexval& rhs) {
    if(this != &rhs) {
	invalidate();
	_type = rhs._type;
	switch(_type) {
	    case TypeArray:	_value.asArray = new ValueArray(*rhs._value.asArray); break;
	    case TypeStruct:	_value.asStruct = new ValueStruct(*rhs._value.asStruct); break;
	    case TypeBool:	_value.asBool = rhs._value.asBool; break;
	    case TypeInt:	_value.asInt = rhs._value.asInt; break;
	    case TypeDouble:	_value.asDouble = rhs._value.asDouble; break;
	    case TypeString:	_value.asString = new string(*rhs._value.asString); break;
	    case TypeTime:	_value.asTime = new cppTime(*rhs._value.asTime); break;
	    case TypeDuration:	_value.asDuration = new cppDuration(*rhs._value.asDuration); break;
	    default:		_value.asArray = NULL; break;
	}
    }
    return *this;
}

bool flexval::operator==(const flexval& other) const {
    if(_type != other._type) {
	return false;
    }
    switch(_type) {
	case TypeArray:	return *_value.asArray == *other._value.asArray;
	case TypeStruct: {
		if(_value.asStruct->size() != other._value.asStruct->size()) {
			return false;
		}
		ValueStruct::const_iterator it1 = _value.asStruct->begin();
		ValueStruct::const_iterator it2 = other._value.asStruct->begin();
		while(it1 != _value.asStruct->end()) {
			const flexval& v1 = it1->second;
			const flexval& v2 = it2->second;
			if(!(v1 == v2)) {
				return false;
			}
			it1++;
			it2++;
		}
		return true;
	}
	case TypeBool:
		return _value.asBool == other._value.asBool;
	case TypeInt:
		return _value.asInt == other._value.asInt;
	case TypeDouble:
		return _value.asDouble == other._value.asDouble;
	case TypeString:
		return *_value.asString == *other._value.asString;
	case TypeTime:
		return *_value.asTime == *other._value.asTime;
	case TypeDuration:
		return *_value.asDuration == *other._value.asDuration;
	case TypeInvalid:
	default:
		break;
    }
    return false;
}

bool flexval::operator!=(const flexval& other) const {
	return !(*this == other);
}

int flexval::size() const {
    switch(_type) {
	case TypeArray: return int(_value.asArray->size());
	case TypeStruct: return int(_value.asStruct->size());
	default: break;
    }
    return 0;
}

bool flexval::hasMember(const string& name) const {
    return _type == TypeStruct && _value.asStruct->find(name) != _value.asStruct->end();
}

string flexval::to_string() const {
    stringstream s;
    write(s);
    return s.str();
}

json_object* flexval::to_json() const {
    if(getType() == TypeInvalid) {
	return json_object_new_string("invalid");
    } else if(getType() == TypeArray) {
	json_object*	array_container = json_object_new_array();
	const vector<flexval>&	vec = get_array();
	for(int z = 0; z < vec.size(); z++) {
	    json_object_array_add(array_container, vec[z].to_json());
	}
	return array_container;
    } else if(getType() == TypeStruct) {
	json_object*	struct_container = json_object_new_object();
	const map<string, flexval>&	Map = get_struct();
	map<string, flexval>::const_iterator iter;
	for(iter = Map.begin(); iter != Map.end(); iter++) {
	    json_object_object_add(
			struct_container,
			iter->first.c_str(),
			iter->second.to_json());
	}
	return struct_container;
    } else if(getType() == TypeBool) {
	return json_object_new_boolean((const bool)*this);
    } else if(getType() == TypeInt) {
	return json_object_new_int((const long int)*this);
    } else if(getType() == TypeDouble) {
	return json_object_new_double((const double)*this);
    } else if(getType() == TypeString) {
	return json_object_new_string(((const string)*this).c_str());
    } else if(getType() == TypeTime) {
	return json_object_new_string(to_string().c_str());
    } else if(getType() == TypeDuration) {
	return json_object_new_string(to_string().c_str());
    }
    assert(false);
    return NULL;
}

//
// In the following method, we assume that aux is always
// consistent with the 'shape' of *this if *this is an
// array or a map. For TOL files generated by APGenX, this
// is always the case because auxiliary information was
// gathered by collecting parameters of all activities of
// a given type in the TOL(s) that was/were output.
//
json_object* flexval::to_json(const flexval& aux) const {
    if(getType() == TypeInvalid) {
	if(aux.getType() == TypeArray) {
	    return json_object_new_array();
	} else if(aux.getType() == TypeStruct) {
	    return json_object_new_object();
	} else {
	    return json_object_new_string("invalid");
	}
    } else if(getType() == TypeArray) {
	json_object*	array_container = json_object_new_array();
	const vector<flexval>&	vec = get_array();
	const vector<flexval>&	aux_vec = aux.get_array();
	for(int z = 0; z < vec.size(); z++) {
	    if(vec[z].getType() == TypeInvalid) {
		if(aux_vec[z].getType() == TypeArray) {
		    json_object_array_add(array_container, json_object_new_array());
		} else if(aux_vec[z].getType() == TypeStruct) {
		    json_object_array_add(array_container, json_object_new_object());
		} else {
		    json_object_array_add(array_container, json_object_new_string("invalid"));
		}
	    } else {
		json_object_array_add(array_container, vec[z].to_json());
	    }
	}
	return array_container;
    } else if(getType() == TypeStruct) {

	json_object*	struct_container = json_object_new_object();

	const map<string, flexval>&	Map = get_struct();
	const map<string, flexval>&	AuxMap = aux.get_struct();

	map<string, flexval>::const_iterator iter;
	map<string, flexval>::const_iterator aux_iter;

	for(iter = Map.begin(); iter != Map.end(); iter++) {
	    if(iter->second.getType() == TypeInvalid) {
		aux_iter = AuxMap.find(iter->first);
		if(aux_iter->second.getType() == TypeArray) {
		    json_object_object_add(
				struct_container,
				iter->first.c_str(),
				json_object_new_array());
		} else if(aux_iter->second.getType() == TypeStruct) {
		    json_object_object_add(
				struct_container,
				iter->first.c_str(),
				json_object_new_object());
		} else {
		    json_object_object_add(
				struct_container,
				iter->first.c_str(),
				json_object_new_string("invalid"));
		}
	    } else {
		json_object_object_add(
			struct_container,
			iter->first.c_str(),
			iter->second.to_json());
	    }
	}
	return struct_container;
    } else if(getType() == TypeBool) {
	return json_object_new_boolean((const bool)*this);
    } else if(getType() == TypeInt) {
	return json_object_new_int((const long int)*this);
    } else if(getType() == TypeDouble) {
	return json_object_new_double((const double)*this);
    } else if(getType() == TypeString) {
	return json_object_new_string(((const string)*this).c_str());
    } else if(getType() == TypeTime) {
	return json_object_new_string(to_string().c_str());
    } else if(getType() == TypeDuration) {
	return json_object_new_string(to_string().c_str());
    }
    assert(false);
    return NULL;
}

void flexval::from_json(json_object* obj) {
    enum json_type		theType = json_object_get_type(obj);
    struct json_object_iterator	obj_iter;
    struct json_object_iterator	obj_end;

    switch(theType) {
	case json_type_null:
	    break;
	case json_type_boolean:
	    *this = (bool)json_object_get_boolean(obj);
	    break;
	case json_type_double:
	    *this = json_object_get_double(obj);
	    break;
	case json_type_int:
	    *this = (long int)json_object_get_int64(obj);
	    break;
	case json_type_object:
	    obj_end = json_object_iter_end(obj);
	    obj_iter = json_object_iter_begin(obj);
	    while(!json_object_iter_equal(&obj_iter, &obj_end)) {
		flexval F;
		F.from_json(json_object_iter_peek_value(&obj_iter));
		(*this)[string(json_object_iter_peek_name(&obj_iter))] = F;
		json_object_iter_next(&obj_iter);
	    }
	    break;
	case json_type_array:
	    for(int i = 0; i < json_object_array_length(obj); i++) {
		flexval F;
		F.from_json(json_object_array_get_idx(obj, i));
		(*this)[i] = F;
	    }
	    if(getType() == TypeInvalid) {
		(*this) = string("invalid");
	    }
	    break;
	case json_type_string:
	    {
	    const char* s = json_object_get_string(obj);
	    if(strcmp(s, "invalid")) {
		(*this) = string(json_object_get_string(obj));
	    }
	    }
	    break;
	default:
	    break;
    }
}

//
// Merges two structs containing information about
// activity parameters that are APGenX arrays.
//
// The given flexval F is guaranteed to be a container,
// either a vector or a map. The present flexval may
// be undefined, or it may be a container of the same
// type. In either case, we want to enhance the present
// flexval so that it contains at least the same amount
// of information as F.
//
void flexval::merge(const flexval& F) {
    Type	Ftype = F.getType();
    switch(_type) {
	case TypeInvalid:

		//
		// The easy case: 'this' knows nothing, so
		// we just adopt F wholesale.
		//
		(*this) = F;
		break;
	case TypeArray:
	    {
	    vector<flexval>& T1 = get_array();
	    const vector<flexval>& F1 = F.get_array();

	    //
	    // Make sure they have the same length
	    //
	    assert(T1.size() == F1.size());
	    for(int z = 0; z < F1.size(); z++) {
		if(F1[z].getType() == TypeString) {

		    //
		    // Make sure the two agree
		    //
		    assert(((string)F1[z]) == ((string)T1[z]));
		    if(((string)F1[z]) != ((string)T1[z])) {
			    Cstring err;
			    err << "Error merging F[" << z
				<< "] = " << F1[z].to_string() << " into (this)["
				<<  z << "] = " << T1[z].to_string() << "\n";
			    throw(eval_error(err));
		    }
		} else if(F1[z].getType() == TypeArray
			  || F1[z].getType() == TypeStruct) {
		    try {
		 	T1[z].merge(F1[z]);
		    } catch(eval_error Err) {
			Cstring err;
			err << "Error merging " << F1[z].to_string()
			    << " into (this)[" << z << "] = "
			    << T1[z].to_string() << "; details:\n"
			    << Err.msg;
			throw(eval_error(err));
		    }
		} else {
		    Cstring err;
		    err << "Error merging " << F1[z].to_string()
			    << " into (this)[" << z << "] = "
			    << T1[z].to_string() << "\n";
		    throw(eval_error(err));
		}
	    }
	    }
	    break;
	case TypeStruct:
	    {
	    map<string, flexval>& T1 = get_struct();
	    const map<string, flexval>& F1 = F.get_struct();
	    map<string, flexval>::iterator Titer;

	    //
	    // Make sure they have the same length? No, we
	    // cannot assume that that's the case.
	    //
	    // assert(T1.size() == F1.size());
	    map<string, flexval>::const_iterator iter;
	    for(  iter = F1.begin();
		  iter != F1.end();
		  iter++) {
		Titer = T1.find(iter->first);
		if(Titer != T1.end()) {

		    //
		    // This already has this element
		    //
		    if(iter->second.getType() == TypeString) {

			//
			// Make sure the two agree
			//
			if(((string)iter->second) != ((string)T1[iter->first])) {
			    Cstring err;
			    err << "Error merging F[" << iter->first
				<< "] = " << iter->second.to_string() << " into (this)["
				<<  iter->first << "] = " << T1[iter->first].to_string() << "\n";
			    throw(eval_error(err));
			}
		    } else if(iter->second.getType() == TypeArray
			      || iter->second.getType() == TypeStruct) {
			try {
			    T1[iter->first].merge(iter->second);
			} catch(eval_error Err) {
			    Cstring err;
			    err << "Error merging " << iter->second.to_string()
				<< " into (this)[" << iter->first << "] = "
				<< T1[iter->first].to_string() << "; details:\n"
				<< Err.msg;
			    throw(eval_error(err));
			}
		    } else {

			//
			// It may be that the F value is an empty array.
			//
			assert(iter->second.getType() == TypeInvalid);
		    }

		    //
		    // debug
		    //
		    // cerr << "\n(1) New value of T1[" << iter->first
		    // 	 << "]: " << T1[iter->first].to_string() << "\n";
		} else {

		    //
		    // This does not have the element in F.
		    // Adopt it wholesale.
		    //
		    T1[iter->first] = iter->second;

		    //
		    // debug
		    //
		    // cerr << "\n(2) New value of T1[" << iter->first
		    //	 << "]: " << T1[iter->first].to_string() << "\n";
		}
	    }
	    }
	    break;
	default:
	    {
	    Cstring err;
	    err << "Error merging this aux file array:\n" << F.to_string()
		<< "\ninto this system value:\n" << to_string() << "\n";
	    throw(eval_error(err));
	    }
    }

    //
    // debug
    //
    // cerr << "\n(3) New value of this: " << to_string() << "\n";
}

static void write_tab(ostream& os, int indent) {
    for(int i = 0; i < indent; i++) {
	os << ' ';
    }
}

ostream& flexval::write(
		ostream& os,
		bool pretty	/* = false */,
		int indent	/* = 0 */ ) const {
    switch(_type) {
	case TypeArray:
	    {
	    int s = int(_value.asArray->size());

	    if(pretty) {
		write_tab(os, indent);
		os << "[\n";
		for(int i = 0; i < s; i++) {
		    _value.asArray->at(i).write(os, true, indent + 2);
		    if(i < s - 1) {
			os << ",\n";
		    } else {
			os << "\n";
		    }
		}
		write_tab(os, indent);
		os << ']';
	    } else {
		os << "[";
		for(int i = 0; i < s; i++) {
		    if(i > 0) {
			os << ", ";
		    }
		    os << _value.asArray->at(i);
		}
		os << ']';
	    }
	    }
	    break;
	case TypeStruct:
	    {
	    ValueStruct::const_iterator it;
	    if(pretty) {
		int N = _value.asStruct->size();
		int i = 0;
		write_tab(os, indent);
		os << "{\n";
		for(it = _value.asStruct->begin(); it != _value.asStruct->end(); ++it) {
		    write_tab(os, indent);
		    os << it->first << ":";
		    if(it->second.getType() != TypeInvalid) {
			os << "\n";
			it->second.write(os, true, indent + 2);
		    }
		    if(i < N - 1) {
			os << ",\n";
		    } else {
			os << "\n";
		    }
		    i++;
		}
		write_tab(os, indent);
		os << '}';
	    } else {
		os << '{';
		for(it = _value.asStruct->begin(); it != _value.asStruct->end(); ++it) {
		    if(it != _value.asStruct->begin()) {
			os << ", ";
		    }
		    os << it->first << ": ";
		    os << it->second;
		}
		os << '}';
	    }
	    }
	    break;
	case TypeBool:
	    {
	    if(pretty) {
		write_tab(os, indent);
	    }
	    if(_value.asBool)
		os << "true";
	    else
		os << "false";
	    }
	    break;
	case TypeInt:
	    {
	    if(pretty) {
		write_tab(os, indent);
	    }
		os << _value.asInt;
		break;
	    }
	case TypeDouble:
	    {
	    if(pretty) {
		write_tab(os, indent);
	    }
		os << _value.asDouble;
		break;
	    }
	case TypeString:
	    {
	    if(pretty) {
		write_tab(os, indent);
	    }
		os << addQuotes(*_value.asString);
		break;
	    }
	case TypeTime:
	    {
	    if(pretty) {
		write_tab(os, indent);
	    }
		os << (string)*_value.asTime;
		break;
	    }
	case TypeDuration:
	    {
	    if(pretty) {
		write_tab(os, indent);
	    }
		os << (string)*_value.asDuration;
		break;
	    }
	default:
	    if(pretty) {
		write_tab(os, indent);
	    }
	    break;
    }
    return os;
}

ostream& operator<<(ostream& os, const flexval& v) {
    return v.write(os);
}

flexval operator+(const flexval& a, const flexval&b) {
	switch(a._type) {
		case flexval::TypeInt:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(((long)a) + ((long)b));
				case flexval::TypeDouble:
					return flexval(((long)a) + ((double)b));
				default:
					throw(eval_error(string("cannot add types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeDouble:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(((double)a) + ((long)b));
				case flexval::TypeDouble:
					return flexval(((double)a) + ((double)b));
				default:
					throw(eval_error(string("cannot add types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeString:
			switch(b._type) {
				case flexval::TypeString:
					return flexval(((string)a) + ((string)b));
				default:
					throw(eval_error(string("cannot add types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeTime:
			switch(b._type) {
				case flexval::TypeDuration:
					return flexval(*a._value.asTime + *b._value.asDuration);
				default:
					throw(eval_error(string("cannot add types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeDuration:
			switch(b._type) {
				case flexval::TypeTime:
					return flexval(*b._value.asTime + *a._value.asDuration);
				case flexval::TypeDuration:
					return flexval(*a._value.asDuration + *b._value.asDuration);
				default:
					throw(eval_error(string("cannot add types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		default:
			throw(eval_error(string("cannot add types ")
				+ flexval::type2string(a._type)
				+ " and " + flexval::type2string(b._type)));
	}
}

flexval operator-(const flexval& a, const flexval&b) {
	switch(a._type) {
		case flexval::TypeInt:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(((long)a) - ((long)b));
				case flexval::TypeDouble:
					return flexval(((long)a) - ((double)b));
				default:
					throw(eval_error(string("cannot subtract types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeDouble:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(((double)a) - ((long)b));
				case flexval::TypeDouble:
					return flexval(((double)a) - ((double)b));
				default:
					throw(eval_error(string("cannot subtract types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeTime:
			switch(b._type) {
				case flexval::TypeTime:
					return flexval(*a._value.asTime - *b._value.asTime);
				case flexval::TypeDuration:
					return flexval(*a._value.asTime - *b._value.asDuration);
				default:
					throw(eval_error(string("cannot subtract types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
		case flexval::TypeDuration:
			switch(b._type) {
				case flexval::TypeDuration:
					return flexval(*a._value.asDuration - *b._value.asDuration);
				default:
					throw(eval_error(string("cannot subtract types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
		default:
			throw(eval_error(string("cannot subtract types ")
				+ flexval::type2string(a._type)
				+ " and " + flexval::type2string(b._type)));
	}
}

flexval operator-(const flexval& a) {
	switch(a._type) {
		case flexval::TypeInt:
			return flexval(-(long)a);
		case flexval::TypeDouble:
			return flexval(-(double)a);
		default:
			throw(eval_error(string("cannot negate type ")
				+ flexval::type2string(a._type)));
	}
}

flexval operator*(const flexval& a, const flexval&b) {
	switch(a._type) {
		case flexval::TypeInt:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(((long)a) * ((long)b));
				case flexval::TypeDouble:
					return flexval(((long)a) * ((double)b));
				case flexval::TypeDuration:
					return flexval(*b._value.asDuration * (double)((long)a));
				default:
					throw(eval_error(string("cannot multiply types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeDouble:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(((double)a) * ((long)b));
				case flexval::TypeDouble:
					return flexval(((double)a) * ((double)b));
				case flexval::TypeDuration:
					return flexval(*b._value.asDuration * ((double)a));
				default:
					throw(eval_error(string("cannot multiply types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeDuration:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(*a._value.asDuration * (double)((long)b));
				case flexval::TypeDouble:
					return flexval(*a._value.asDuration * ((double)b));
				default:
					throw(eval_error(string("cannot multiply types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
		default:
			throw(eval_error(string("cannot multiply types ")
				+ flexval::type2string(a._type)
				+ " and " + flexval::type2string(b._type)));
	}
}

flexval operator/(const flexval& a, const flexval&b) {
	switch(a._type) {
		case flexval::TypeInt:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(((long)a) / ((long)b));
				case flexval::TypeDouble:
					return flexval(((long)a) / ((double)b));
				default:
					throw(eval_error(string("cannot divide types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeDouble:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(((double)a) / ((long)b));
				case flexval::TypeDouble:
					return flexval(((double)a) / ((double)b));
				default:
					throw(eval_error(string("cannot divide types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
			break;
		case flexval::TypeDuration:
			switch(b._type) {
				case flexval::TypeInt:
					return flexval(*a._value.asDuration / (double)((long)b));
				case flexval::TypeDouble:
					return flexval(*a._value.asDuration / ((double)b));
				case flexval::TypeDuration:
					return flexval(*a._value.asDuration / *b._value.asDuration);
				default:
					throw(eval_error(string("cannot divide types ")
						+ flexval::type2string(a._type)
						+ " and " + flexval::type2string(b._type)));
			}
		default:
			throw(eval_error(string("cannot divide types ")
				+ flexval::type2string(a._type)
				+ " and " + flexval::type2string(b._type)));
	}
}

flexval operator+(const flexval& a, double b) {
	switch(a._type) {
		case flexval::TypeInt:
			return flexval(a._value.asInt + b);
		case flexval::TypeDouble:
			return flexval(a._value.asDouble + b);
		default:
			throw(eval_error(string("cannot add types ")
				+ flexval::type2string(a._type)
				+ " and double"));
	}
}

flexval operator+(double a, const flexval& b) {
	switch(b._type) {
		case flexval::TypeInt:
			return flexval(a + b._value.asInt);
		case flexval::TypeDouble:
			return flexval(a + b._value.asDouble);
		default:
			throw(eval_error(string("cannot add types double and ")
				+ flexval::type2string(b._type)));
	}
}

flexval operator-(double a, const flexval& b) {
	switch(b._type) {
		case flexval::TypeInt:
			return flexval(a - b._value.asInt);
		case flexval::TypeDouble:
			return flexval(a - b._value.asDouble);
		default:
			throw(eval_error(string("cannot subtract types double and ")
				+ flexval::type2string(b._type)));
	}
}

flexval operator-(const flexval& a, double b) {
	switch(a._type) {
		case flexval::TypeInt:
			return flexval(a._value.asInt - b);
		case flexval::TypeDouble:
			return flexval(a._value.asDouble - b);
		default:
			throw(eval_error(string("cannot subtract types ")
				+ flexval::type2string(a._type)
				+ " and double"));
	}
}

flexval operator*(double a, const flexval& b) {
	switch(b._type) {
		case flexval::TypeInt:
			return flexval(a * b._value.asInt);
		case flexval::TypeDouble:
			return flexval(a * b._value.asDouble);
		default:
			throw(eval_error(string("cannot multiply types double and ")
				+ flexval::type2string(b._type)));
	}
}

flexval operator*(const flexval& a, double b) {
	switch(a._type) {
		case flexval::TypeInt:
			return flexval(a._value.asInt * b);
		case flexval::TypeDouble:
			return flexval(a._value.asDouble * b);
		default:
			throw(eval_error(string("cannot multiply types ")
				+ flexval::type2string(a._type)
				+ " and double"));
	}
}

flexval operator/(const flexval& a, double b) {
	switch(a._type) {
		case flexval::TypeInt:
			return flexval(a._value.asInt / b);
		case flexval::TypeDouble:
			return flexval(a._value.asDouble / b);
		default:
			throw(eval_error(string("cannot divide types ")
				+ flexval::type2string(a._type)
				+ " and double"));
	}
}

flexval operator/(double a, const flexval& b) {
	switch(b._type) {
		case flexval::TypeInt:
			return flexval(a / b._value.asInt);
		case flexval::TypeDouble:
			return flexval(a / b._value.asDouble);
		default:
			throw(eval_error(string("cannot divide types double and ")
				+ flexval::type2string(b._type)));
	}
}

flexval operator+(const flexval& a, long int b) {
	switch(a._type) {
		case flexval::TypeInt:
			return flexval(a._value.asInt + b);
		case flexval::TypeDouble:
			return flexval(a._value.asDouble + b);
		default:
			throw(eval_error(string("cannot add types ")
				+ flexval::type2string(a._type)
				+ " and long int"));
	}
}

flexval operator+(long int a, const flexval& b) {
	switch(b._type) {
		case flexval::TypeInt:
			return flexval(a + b._value.asInt);
		case flexval::TypeDouble:
			return flexval(a + b._value.asDouble);
		default:
			throw(eval_error(string("cannot add types long int and ")
				+ flexval::type2string(b._type)));
	}
}

flexval operator-(long int a, const flexval& b) {
	switch(b._type) {
		case flexval::TypeInt:
			return flexval(a - b._value.asInt);
		case flexval::TypeDouble:
			return flexval(a - b._value.asDouble);
		default:
			throw(eval_error(string("cannot subtract types long int and ")
				+ flexval::type2string(b._type)));
	}
}

flexval operator-(const flexval& a, long int b) {
	switch(a._type) {
		case flexval::TypeInt:
			return flexval(a._value.asInt - b);
		case flexval::TypeDouble:
			return flexval(a._value.asDouble - b);
		default:
			throw(eval_error(string("cannot subtract types ")
				+ flexval::type2string(a._type)
				+ " and long int"));
	}
}

flexval operator*(long int a, const flexval& b) {
	switch(b._type) {
		case flexval::TypeInt:
			return flexval(a * b._value.asInt);
		case flexval::TypeDouble:
			return flexval(a * b._value.asDouble);
		default:
			throw(eval_error(string("cannot multiply types long int and ")
				+ flexval::type2string(b._type)));
	}
}

flexval operator*(const flexval& a, long int b) {
	switch(a._type) {
		case flexval::TypeInt:
			return flexval(a._value.asInt * b);
		case flexval::TypeDouble:
			return flexval(a._value.asDouble * b);
		default:
			throw(eval_error(string("cannot multiply types ")
				+ flexval::type2string(a._type)
				+ " and long int"));
	}
}

flexval operator/(const flexval& a, long int b) {
	switch(a._type) {
		case flexval::TypeInt:
			return flexval(a._value.asInt / b);
		case flexval::TypeDouble:
			return flexval(a._value.asDouble / b);
		default:
			throw(eval_error(string("cannot divide types ")
				+ flexval::type2string(a._type)
				+ " and long int"));
	}
}

flexval operator/(long int a, const flexval& b) {
	switch(b._type) {
		case flexval::TypeInt:
			return flexval(a / b._value.asInt);
		case flexval::TypeDouble:
			return flexval(a / b._value.asDouble);
		default:
			throw(eval_error(string("cannot divide types long int and ")
				+ flexval::type2string(b._type)));
	}
}

#ifdef have_xmltol
void	flexval::print_to_xmlpp(xmlpp::Element* parent_node) {
	xmlpp::Element*		element;

	if(_type == TypeInvalid) {
		element = parent_node->add_child_element("UninitializedValue");
		return; }
	stringstream	os;
	string		tempstring;
	switch(_type) {
		case TypeBool:
			element = parent_node->add_child_element("BooleanValue");
			write(os);
			element->add_child_text(os.str());
			break;
		case TypeDouble:
			element = parent_node->add_child_element("DoubleValue");
			write(os);
			element->add_child_text(os.str());
			break;
		case TypeInt:
			element = parent_node->add_child_element("IntegerValue");
			write(os);
			element->add_child_text(os.str());
			break;
		case TypeString:
			element = parent_node->add_child_element("StringValue");
			write(os);
			tempstring = os.str();
			element->add_child_text(tempstring.substr(1, tempstring.length() - 2));
			break;
		case TypeDuration:
			{
			char	buf[80];
			sprintf(buf, "%ld", (long int)*_value.asDuration);
			element = parent_node->add_child_element("DurationValue");
			write(os);
			element->add_child_text(os.str());
			element->set_attribute("milliseconds", buf); 
			}
			break;
		case TypeTime:
			{
			char	buf[80];
			sprintf(buf, "%ld", (long int)*_value.asDuration);
			element = parent_node->add_child_element("TimeValue");
			write(os);
			element->add_child_text(os.str());
			element->set_attribute("milliseconds", buf); 
			}
			break;
		case TypeArray:
			{
			long		c = 0;
			char		buf[80];

			element = parent_node->add_child_element("ListValue");
			flexval::ValueArray::iterator iter;
			for(	iter = _value.asArray->begin();
				iter != _value.asArray->end();
				iter++
			   ) {
				sprintf(buf, "%ld", c);
				xmlpp::Element*	subelement;
				subelement = element->add_child_element("Element");
				subelement->set_attribute("index", buf);
				c++;
				iter->print_to_xmlpp(subelement);
			  }
			}
			break;
		case TypeStruct:
			{
			long		c = 0;

			element = parent_node->add_child_element("StructValue");
			flexval::ValueStruct::iterator iter;
			for(	iter = _value.asStruct->begin();
				iter != _value.asStruct->end();
				iter++
			   ) {
				xmlpp::Element*	subelement;
				subelement = element->add_child_element("Element");
				subelement->set_attribute("index", iter->first);
				iter->second.print_to_xmlpp(subelement);
			  }
			}
			break;
		default:
			parent_node->add_child_element("UnknownValue");
			break;
	} 
}
#endif /* have_xmltol */

string	flexval::addQuotes(const string& input_string) {
	const char*	c = input_string.c_str();
	buf_struct	B = {NULL, 0, 0};
	string		out_string = app_add_quotes_no_nl(c, &B);
	destroy_buf_struct(&B);
	return out_string;
}

string	flexval::removeQuotes(const string& input_string) {
	const char*	c = input_string.c_str();
	buf_struct	B = {NULL, 0, 0};
	string		out_string = app_remove_quotes(c, &B);
	destroy_buf_struct(&B);
	return out_string;
}
