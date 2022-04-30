
#include "XmlRpcValue.H"
#include "XmlRpcException.H"
#include "XmlRpcUtil.H"
#include "base64.H"

#ifndef MAKEDEPEND
# include <iostream>
# include <ostream>
# include <stdlib.h>
# include <stdio.h>
#endif

namespace XmlRpc {


  static const char VALUE_TAG[]     = "<value>";
  static const char VALUE_ETAG[]    = "</value>";

  static const char BOOLEAN_TAG[]   = "<boolean>";
  static const char BOOLEAN_ETAG[]  = "</boolean>";
  static const char DOUBLE_TAG[]    = "<double>";
  static const char DOUBLE_ETAG[]   = "</double>";
  static const char INT_TAG[]       = "<int>";
  static const char INT_ETAG[]       = "</int>";
  static const char I4_TAG[]        = "<i4>";
  static const char I4_ETAG[]       = "</i4>";
  static const char STRING_TAG[]    = "<string>";
  static const char STRING_ETAG[]    = "</string>";
  static const char DATETIME_TAG[]  = "<dateTime.iso8601>";
  static const char DATETIME_ETAG[] = "</dateTime.iso8601>";
  static const char BASE64_TAG[]    = "<base64>";
  static const char BASE64_ETAG[]   = "</base64>";

  static const char ARRAY_TAG[]     = "<array>";
  static const char DATA_TAG[]      = "<data>";
  static const char DATA_ETAG[]     = "</data>";
  static const char ARRAY_ETAG[]    = "</array>";

  static const char STRUCT_TAG[]    = "<struct>";
  static const char MEMBER_TAG[]    = "<member>";
  static const char NAME_TAG[]      = "<name>";
  static const char NAME_ETAG[]     = "</name>";
  static const char MEMBER_ETAG[]   = "</member>";
  static const char STRUCT_ETAG[]   = "</struct>";


      
  // Format strings
  std::string XmlRpcValue::_doubleFormat("%f");



  // Clean up
  void XmlRpcValue::invalidate()
  {
    switch (_type) {
      case TypeString:    delete _value.asString; break;
      case TypeDateTime:  delete _value.asTime;   break;
      case TypeBase64:    delete _value.asBinary; break;
      case TypeArray:     delete _value.asArray;  break;
      case TypeStruct:    delete _value.asStruct; break;
      default: break;
    }
    _type = TypeInvalid;
    _value.asBinary = 0;
  }

  
  // Type checking
  void XmlRpcValue::assertTypeOrInvalid(Type t)
  {
    if (_type == TypeInvalid)
    {
      _type = t;
      switch (_type) {    // Ensure there is a valid value for the type
        case TypeString:   _value.asString = new std::string(); break;
        case TypeDateTime: _value.asTime = new struct tm();     break;
        case TypeBase64:   _value.asBinary = new BinaryData();  break;
        case TypeArray:    _value.asArray = new ValueArray();   break;
        case TypeStruct:   _value.asStruct = new ValueStruct(); break;
        default:           _value.asBinary = 0; break;
      }
    }
    else if (_type != t)
      throw XmlRpcException("type error");
  }

  void XmlRpcValue::assertArray(int size) const
  {
    if (_type != TypeArray)
      throw XmlRpcException("type error: expected an array");
    else if (int(_value.asArray->size()) < size)
      throw XmlRpcException("range error: array index too large");
  }


  void XmlRpcValue::assertArray(int size)
  {
    if (_type == TypeInvalid) {
      _type = TypeArray;
      _value.asArray = new ValueArray(size);
    } else if (_type == TypeArray) {
      if (int(_value.asArray->size()) < size)
        _value.asArray->resize(size);
    } else
      throw XmlRpcException("type error: expected an array");
  }

  void XmlRpcValue::assertStruct()
  {
    if (_type == TypeInvalid) {
      _type = TypeStruct;
      _value.asStruct = new ValueStruct();
    } else if (_type != TypeStruct)
      throw XmlRpcException("type error: expected a struct");
  }


  // Operators
  XmlRpcValue& XmlRpcValue::operator=(XmlRpcValue const& rhs)
  {
    if (this != &rhs)
    {
      invalidate();
      _type = rhs._type;
      switch (_type) {
        case TypeBoolean:  _value.asBool = rhs._value.asBool; break;
        case TypeInt:      _value.asInt = rhs._value.asInt; break;
        case TypeDouble:   _value.asDouble = rhs._value.asDouble; break;
        case TypeDateTime: _value.asTime = new struct tm(*rhs._value.asTime); break;
        case TypeString:   _value.asString = new std::string(*rhs._value.asString); break;
        case TypeBase64:   _value.asBinary = new BinaryData(*rhs._value.asBinary); break;
        case TypeArray:    _value.asArray = new ValueArray(*rhs._value.asArray); break;
        case TypeStruct:   _value.asStruct = new ValueStruct(*rhs._value.asStruct); break;
        default:           _value.asBinary = 0; break;
      }
    }
    return *this;
  }


  // Predicate for tm equality
  static bool tmEq(struct tm const& t1, struct tm const& t2) {
    return t1.tm_sec == t2.tm_sec && t1.tm_min == t2.tm_min &&
            t1.tm_hour == t2.tm_hour && t1.tm_mday == t2.tm_mday &&
            t1.tm_mon == t2.tm_mon && t1.tm_year == t2.tm_year;
  }

  bool XmlRpcValue::operator==(XmlRpcValue const& other) const
  {
    if (_type != other._type)
      return false;

    switch (_type) {
      case TypeBoolean:  return ( !_value.asBool && !other._value.asBool) ||
                                ( _value.asBool && other._value.asBool);
      case TypeInt:      return _value.asInt == other._value.asInt;
      case TypeDouble:   return _value.asDouble == other._value.asDouble;
      case TypeDateTime: return tmEq(*_value.asTime, *other._value.asTime);
      case TypeString:   return *_value.asString == *other._value.asString;
      case TypeBase64:   return *_value.asBinary == *other._value.asBinary;
      case TypeArray:    return *_value.asArray == *other._value.asArray;

      // The map<>::operator== requires the definition of value< for kcc
      case TypeStruct:   //return *_value.asStruct == *other._value.asStruct;
        {
          if (_value.asStruct->size() != other._value.asStruct->size())
            return false;
          
          ValueStruct::const_iterator it1=_value.asStruct->begin();
          ValueStruct::const_iterator it2=other._value.asStruct->begin();
          while (it1 != _value.asStruct->end()) {
            const XmlRpcValue& v1 = it1->second;
            const XmlRpcValue& v2 = it2->second;
            if ( ! (v1 == v2))
              return false;
            it1++;
            it2++;
          }
          return true;
        }
      default: break;
    }
    return true;    // Both invalid values ...
  }

  bool XmlRpcValue::operator!=(XmlRpcValue const& other) const
  {
    return !(*this == other);
  }


  // Works for strings, binary data, arrays, and structs.
  int XmlRpcValue::size() const
  {
    switch (_type) {
      case TypeString: return int(_value.asString->size());
      case TypeBase64: return int(_value.asBinary->size());
      case TypeArray:  return int(_value.asArray->size());
      case TypeStruct: return int(_value.asStruct->size());
      default: break;
    }

    // throw XmlRpcException("type error");
    return 0;
  }

  // Checks for existence of struct member
  bool XmlRpcValue::hasMember(const std::string& name) const
  {
    return _type == TypeStruct && _value.asStruct->find(name) != _value.asStruct->end();
  }

static int debugIndent = 0;
static void debug_print(const char *s) {
	int i;
	for(i = 0; i < debugIndent; i++) {
		std::cout << " "; }
	std::cout << s << "\n"; }
static void debug_print_name(const char *s) {
	int i;
	for(i = 0; i < debugIndent; i++) {
		std::cout << " "; }
	std::cout << s << ": "; }
static void debug_print(long k) {
	int i;
	char s[100];
	sprintf(s, "%ld\n", k);
	for(i = 0; i < debugIndent; i++) {
		std::cout << " "; }
	std::cout << s; }

  // Set the value from xml. The chars at *offset into valueXml 
  // should be the start of a <value> tag. Destroys any existing value.
  bool XmlRpcValue::fromXml(std::string const& valueXml, int* offset)
  {
    int savedOffset = *offset;

    invalidate();
    if ( ! XmlRpcUtil::nextTagIs(VALUE_TAG, valueXml, offset)) {
	XmlRpcUtil::log(5, "fromXml: next tag is not a value, returning\n");
      return false; }       // Not a value, offset not updated

	int afterValueOffset = *offset;
    std::string typeTag = XmlRpcUtil::getNextTag(valueXml, offset);
    bool result = false;
    if (typeTag == BOOLEAN_TAG)
      result = boolFromXml(valueXml, offset);
    else if (typeTag == I4_TAG || typeTag == INT_TAG)
      result = intFromXml(valueXml, offset);
    else if (typeTag == DOUBLE_TAG)
      result = doubleFromXml(valueXml, offset);
    else if (typeTag.empty() || typeTag == STRING_TAG)
      result = stringFromXml(valueXml, offset);
    else if (typeTag == DATETIME_TAG)
      result = timeFromXml(valueXml, offset);
    else if (typeTag == BASE64_TAG)
      result = binaryFromXml(valueXml, offset);
    else if (typeTag == ARRAY_TAG) {
	// debugIndent += 4;
	// debug_print("{");
	result = arrayFromXml(valueXml, offset);
	// debug_print("}");
	// debugIndent -= 4;
	}
    else if (typeTag == STRUCT_TAG) {
	// debugIndent += 4;
	// debug_print("[");
	result = structFromXml(valueXml, offset);
	// debug_print("]");
	// debugIndent -= 4;
	}
    // Watch for empty/blank strings with no <string>tag
    else if (typeTag == VALUE_ETAG)
    {
      *offset = afterValueOffset;   // back up & try again
      result = stringFromXml(valueXml, offset);
    }

    if (result)  // Skip over the </value> tag
      XmlRpcUtil::findTag(VALUE_ETAG, valueXml, offset);
    else        // Unrecognized tag after <value>
      *offset = savedOffset;

    return result;
  }

  // Encode the Value in xml
  std::string XmlRpcValue::toXml() const
  {
    switch (_type) {
      case TypeBoolean:  return boolToXml();
      case TypeInt:      return intToXml();
      case TypeDouble:   return doubleToXml();
      case TypeString:   return stringToXml();
      case TypeDateTime: return timeToXml();
      case TypeBase64:   return binaryToXml();
      case TypeArray:    return arrayToXml();
      case TypeStruct:   return structToXml();
      default: break;
    }
    return std::string();   // Invalid value
  }


  // Boolean
  bool XmlRpcValue::boolFromXml(std::string const& valueXml, int* offset)
  {
    const char* valueStart = valueXml.c_str() + *offset;
    char* valueEnd;
    long ivalue = strtol(valueStart, &valueEnd, 10);
    if (valueEnd == valueStart || (ivalue != 0 && ivalue != 1))
      return false;

    _type = TypeBoolean;
    _value.asBool = (ivalue == 1);
    *offset += int(valueEnd - valueStart);
    return true;
  }

  std::string XmlRpcValue::boolToXml() const
  {
    std::string xml = VALUE_TAG;
    xml += BOOLEAN_TAG;
    xml += (_value.asBool ? "1" : "0");
    xml += BOOLEAN_ETAG;
    xml += VALUE_ETAG;
    return xml;
  }

  // Int
  bool XmlRpcValue::intFromXml(std::string const& valueXml, int* offset)
  {
    const char* valueStart = valueXml.c_str() + *offset;
    char* valueEnd;
    long ivalue = strtol(valueStart, &valueEnd, 10);
    if (valueEnd == valueStart)
      return false;

    _type = TypeInt;
    _value.asInt = long(ivalue);
	// debug
	// std::cerr << "stringFromXml: " << *_value.asString << std::endl;
	// debug_print(_value.asInt);
    *offset += int(valueEnd - valueStart);
    return true;
  }

  std::string XmlRpcValue::intToXml() const
  {
    char buf[256];
    snprintf(buf, sizeof(buf)-1, "%ld", _value.asInt);
    buf[sizeof(buf)-1] = 0;
    std::string xml = VALUE_TAG;
    // PFM 11/21/2005
    // xml += I4_TAG;
    xml += INT_TAG;
    xml += buf;
    // PFM 11/21/2005
    // xml += I4_ETAG;
    xml += INT_ETAG;
    xml += VALUE_ETAG;
    return xml;
  }

  // Double
  bool XmlRpcValue::doubleFromXml(std::string const& valueXml, int* offset)
  {
    const char* valueStart = valueXml.c_str() + *offset;
    char* valueEnd;
    double dvalue = strtod(valueStart, &valueEnd);
    if (valueEnd == valueStart)
      return false;

    _type = TypeDouble;
    _value.asDouble = dvalue;
    *offset += int(valueEnd - valueStart);
    return true;
  }

  std::string XmlRpcValue::doubleToXml() const
  {
    char buf[256];
    snprintf(buf, sizeof(buf)-1, getDoubleFormat().c_str(), _value.asDouble);
    buf[sizeof(buf)-1] = 0;

    std::string xml = VALUE_TAG;
    xml += DOUBLE_TAG;
    xml += buf;
    xml += DOUBLE_ETAG;
    xml += VALUE_ETAG;
    return xml;
  }

  // String
  bool XmlRpcValue::stringFromXml(std::string const& valueXml, int* offset)
  {
    size_t valueEnd = valueXml.find('<', *offset);
    if (valueEnd == std::string::npos)
      return false;     // No end tag;

    _type = TypeString;
    _value.asString = new std::string(XmlRpcUtil::xmlDecode(valueXml.substr(*offset, valueEnd-*offset)));
	// debug
	// std::cerr << "stringFromXml: " << *_value.asString << std::endl;
	// debug_print(_value.asString->c_str());

    *offset += int(_value.asString->length());
    return true;
  }

  std::string XmlRpcValue::stringToXml() const
  {
    std::string xml = VALUE_TAG;
	// PFM 11/21/2005
    //xml += STRING_TAG; optional
    xml += STRING_TAG;
    xml += XmlRpcUtil::xmlEncode(*_value.asString);
	// PFM 11/21/2005
    //xml += STRING_ETAG;
    xml += STRING_ETAG;
    xml += VALUE_ETAG;
    return xml;
  }

  // DateTime (stored as a struct tm)
  bool XmlRpcValue::timeFromXml(std::string const& valueXml, int* offset)
  {
    size_t valueEnd = valueXml.find('<', *offset);
    if (valueEnd == std::string::npos)
      return false;     // No end tag;

    std::string stime = valueXml.substr(*offset, valueEnd-*offset);

    struct tm t;
    // original:
    // if (sscanf(stime.c_str(),"%4d%2d%2dT%2d:%2d:%2d",&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec) != 6)
    // modified PFM Sep. 13 2005:
    // if (sscanf(stime.c_str(),"  %2d%2d%2dT%2d:%2d:%2d",&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec) != 6)
    // changed back PFM Sep. 14 2005:

	/* See http://www.w3.org/TR/NOTE-datetime for the ISO 8601 specification.
	 * I am following this religiously now. PFM 9/19/2005
	 */

    if (sscanf(stime.c_str(),"%4d%2d%2dT%2d:%2d:%2d",&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec) != 6) {
	// debug
	// std::cout << "XmlRpcValue::timeFromXml: sscanf error\n";
      return false; }
	// debug
	// std::cout << "XmlRpcValue::timeFromXml: sscanf result " << t.tm_year << " "
	// 	<< t.tm_mon << " (month) " 
	// 	<< t.tm_mday << " (day) " 
	// 	<< t.tm_hour << " (hour) " 
	// 	<< t.tm_min << " (min) " 
	// 	<< t.tm_sec << " (sec)\n";
	/* PFM ISO 8601 adjustments ('man ctime' for struct tm info):
	 *
	 * struct tm counts Y since the epoch */
	t.tm_year -= 1900;
	/* struct tm counts month starting at Jan = 0, ISO 8601 at 1: */
	t.tm_mon -= 1;
	// PFM: the following sounds fishy... well, OK; let's not ask for any corrections.
	t.tm_isdst = -1;
	_type = TypeDateTime;
	_value.asTime = new struct tm(t);
	// debug
	// std::cout << "XmlRpcValue::timeFromXml: success\n";
	*offset += int(stime.length());
	return true; }

  std::string XmlRpcValue::timeToXml() const
  {
    struct tm* t = _value.asTime;
    char buf[20];
	// PFM 9/14/2005
    int mod_year = 1900 + t->tm_year;
    snprintf(buf, sizeof(buf)-1, "%4d%02d%02dT%02d:%02d:%02d", 
		mod_year,
		t->tm_mon + 1,	// see ISO 8601 note above
		t->tm_mday,
		t->tm_hour,
		t->tm_min,
		t->tm_sec);
    buf[sizeof(buf)-1] = 0;

	// debug
	// std::cerr << "XmlRpcValue::timeToXml: time = " << buf << std::endl;
    std::string xml = VALUE_TAG;
    xml += DATETIME_TAG;
    xml += buf;
    xml += DATETIME_ETAG;
    xml += VALUE_ETAG;
    return xml;
  }


  // Base64
  bool XmlRpcValue::binaryFromXml(std::string const& valueXml, int* offset)
  {
    size_t valueEnd = valueXml.find('<', *offset);
    if (valueEnd == std::string::npos)
      return false;     // No end tag;

    _type = TypeBase64;
    std::string asString = valueXml.substr(*offset, valueEnd-*offset);
    _value.asBinary = new BinaryData();
    // check whether base64 encodings can contain chars xml encodes...

    // convert from base64 to binary
    int iostatus = 0;
	  base64<char> decoder;
    std::back_insert_iterator<BinaryData> ins = std::back_inserter(*(_value.asBinary));
		decoder.get(asString.begin(), asString.end(), ins, iostatus);

    *offset += int(asString.length());
    return true;
  }


  std::string XmlRpcValue::binaryToXml() const
  {
    // convert to base64
    std::vector<char> base64data;
    int iostatus = 0;
	  base64<char> encoder;
    std::back_insert_iterator<std::vector<char> > ins = std::back_inserter(base64data);
		encoder.put(_value.asBinary->begin(), _value.asBinary->end(), ins, iostatus, base64<>::crlf());

    // Wrap with xml
    std::string xml = VALUE_TAG;
    xml += BASE64_TAG;
    xml.append(base64data.begin(), base64data.end());
    xml += BASE64_ETAG;
    xml += VALUE_ETAG;
    return xml;
  }


  // Array
  bool XmlRpcValue::arrayFromXml(std::string const& valueXml, int* offset)
  {
    if ( ! XmlRpcUtil::nextTagIs(DATA_TAG, valueXml, offset))
      return false;

    _type = TypeArray;
    _value.asArray = new ValueArray;
    XmlRpcValue v;
    while (v.fromXml(valueXml, offset))
      _value.asArray->push_back(v);       // copy...

    // Skip the trailing </data>
    (void) XmlRpcUtil::nextTagIs(DATA_ETAG, valueXml, offset);
    return true;
  }


  // In general, its preferable to generate the xml of each element of the
  // array as it is needed rather than glomming up one big string.
  std::string XmlRpcValue::arrayToXml() const
  {
    std::string xml = VALUE_TAG;
    xml += ARRAY_TAG;
    xml += DATA_TAG;

    int s = int(_value.asArray->size());
    for (int i=0; i<s; ++i)
       xml += _value.asArray->at(i).toXml();

    xml += DATA_ETAG;
    xml += ARRAY_ETAG;
    xml += VALUE_ETAG;
    return xml;
  }


  // Struct
  bool XmlRpcValue::structFromXml(std::string const& valueXml, int* offset)
  {
    _type = TypeStruct;
    _value.asStruct = new ValueStruct;

    while (XmlRpcUtil::nextTagIs(MEMBER_TAG, valueXml, offset)) {
      // name
      const std::string name = XmlRpcUtil::parseTag(NAME_TAG, valueXml, offset);
	// debug
	// debug_print_name(name.c_str());
      // value
      XmlRpcValue val(valueXml, offset);
      if ( ! val.valid()) {
        invalidate();
        return false;
      }
      const std::pair<const std::string, XmlRpcValue> p(name, val);
      _value.asStruct->insert(p);

      (void) XmlRpcUtil::nextTagIs(MEMBER_ETAG, valueXml, offset);
    }
    return true;
  }


  // In general, its preferable to generate the xml of each element
  // as it is needed rather than glomming up one big string.
  std::string XmlRpcValue::structToXml() const
  {
    std::string xml = VALUE_TAG;
    xml += STRUCT_TAG;

    ValueStruct::const_iterator it;
    for (it=_value.asStruct->begin(); it!=_value.asStruct->end(); ++it) {
      xml += MEMBER_TAG;
      xml += NAME_TAG;
      xml += XmlRpcUtil::xmlEncode(it->first);
      xml += NAME_ETAG;
      xml += it->second.toXml();
      xml += MEMBER_ETAG;
    }

    xml += STRUCT_ETAG;
    xml += VALUE_ETAG;
    return xml;
  }



  // Write the value without xml encoding it
  std::ostream& XmlRpcValue::write(std::ostream& os, int level, int &est_length) const {
    int k;
    switch (_type) {
      default:           break;
      case TypeBoolean:
	os << _value.asBool;
	if(est_length >= 0) {
		est_length += 1; }
	break;
      case TypeInt:
	os << _value.asInt;
	if(est_length >= 0) {
		est_length += 6; }
	break;
      case TypeDouble:
	os << _value.asDouble;
	if(est_length >= 0) {
		est_length += 16; }
	break;
      case TypeString:
	os << *_value.asString;
	if(est_length >= 0) {
		est_length += _value.asString->size(); }
	break;
      case TypeDateTime:
        {
        struct tm* t = _value.asTime;
        char buf[23];

	// Changed 05/03/2006 PFM added 1 to month so it starts at zero; added spaces
        // snprintf(buf, sizeof(buf)-1, "%4d%02d%02dT%02d:%02d:%02d", 
        //   t->tm_year + 1900,t->tm_mon,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
        snprintf(buf, sizeof(buf)-1, "%4d %02d %02d T%02d:%02d:%02d", 
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
        buf[sizeof(buf)-1] = 0;
        os << buf;
	if(est_length >= 0) {
		est_length += 22; }
        break;
        }
      case TypeBase64:
	{
	int iostatus = 0;
	std::ostreambuf_iterator<char> out(os);
	base64<char> encoder;
	encoder.put(_value.asBinary->begin(), _value.asBinary->end(), out, iostatus, base64<>::crlf());
	if(est_length >= 0) {
		// may not be right
		est_length += _value.asBinary->size(); }
	break;
        }
      case TypeArray:
	{
	// modified by PFM to reflect common usage
	int s = int(_value.asArray->size());
	bool containsSequentialData = true;

	// 1. parent has printed prefix corresponding to level, if appropriate
        os << '[';
	if(est_length >= 0) {
		est_length++; }

	// 2. check whether the array contains sequential data
	if(est_length >= 0) {
        	for (int i=0; i<s; ++i) {
			Type	theT = _value.asArray->at(i).getType();
			if(theT == TypeArray || theT == TypeStruct) {
				containsSequentialData = false;
				break; } } }
        for (int i=0; i<s; ++i) {
		if(!containsSequentialData) {
			if(i > 0) {
				os << ",\n"; }
			else {
				os << "\n"; }
			for(k = 0; k < level; k++) {
				os << " "; }
			est_length = level;
			level += 2; }
		// modified by PFM to add a space for legibility
		_value.asArray->at(i).write(os, level + 2, est_length);
		if(containsSequentialData) {
			if(i < (s - 1)) {
				if(est_length >= 80) {
					os << ",\n";
					for(k = 0; k < level; k++) {
						os << " "; }
					est_length = level; }
				else {
					os << ", ";
					if(est_length >= 0) {
						est_length += 2; } } } }
		else {
			level -= 2;
			// do nothing else; we'll add a newline in a moment
			} }
	if(!containsSequentialData) {
		os << "\n";
		for(k = 0; k < level; k++) {
			os << " "; }
		est_length = level; }
	os << "]";
	if(est_length >= 0) {
		est_length++; }
	break; }
      case TypeStruct:
        {
	// modified by PFM to reflect common usage
	bool containsSequentialData = true;

	os << '{';
	if(est_length >= 0) {
		est_length++; }
        ValueStruct::const_iterator it;
	if(est_length >= 0) {
        	for (it=_value.asStruct->begin(); it!=_value.asStruct->end(); ++it) {
			Type	theT = it->second.getType();
			if(theT == TypeArray || theT == TypeStruct) {
				containsSequentialData = false;
				break; } } }
        for (it=_value.asStruct->begin(); it!=_value.asStruct->end(); ++it) {
	    // modified by PFM to add a space for legibility
	    if(containsSequentialData) {
		if (it!=_value.asStruct->begin()) {
		    if(est_length < 80) {
			os << ", ";
			if(est_length >= 0) {
				est_length += 2; } }
		    else {
			os << ",\n";
			for(k = 0; k < level; k++) {
				os << " "; }
			est_length = level; } } }
	    else {
		// non-sequential data
		if (it == _value.asStruct->begin()) {
			os << "\n"; }
		else {
			os << ",\n"; }
		for(k = 0; k < level; k++) {
			os << " "; }
		est_length = level;
		level += 2; }
            os << it->first << ": ";
	    if(est_length >= 0) {
		est_length += it->first.size() + 2; }
            it->second.write(os, level + 2, est_length);
	    if(!containsSequentialData) {
		level -= 2; } }
	if(!containsSequentialData) {
		os << "\n";
		est_length = level;
		for(k = 0; k < level; k++) {
			os << " "; } }
        os << '}';
	if(est_length >= 0) {
		est_length++; }
        break; }
      
    }
    
    return os;
  }

// PFM utility
time_t timegm (struct tm *tm) {
	time_t ret, checking;
	struct tm localtimestr;

	/* returns the number of seconds corresponding to what's in tm,
	 * but unfortunately the time zone is taken into account... and
	 * there is no analog for UTC on the Solaris platform.
	 */
	ret = mktime(tm);
	// if tz is 7 hours off, then ret is too big by 7 hours
	gmtime_r(&ret, &localtimestr);
	// now localtimestr reflects the erroneous 7 extra hours. But
	// first we correct the @#$#@$%^^* daylight savings time bit that will
	// mess everything up:
	localtimestr.tm_isdst = -1;
	checking = mktime(&localtimestr);
	// finally we correct ret by subtracting the difference between checking and ret
	ret = ret - (checking - ret);
	return ret; }
} // namespace XmlRpc


// ostream
std::ostream& operator<<(std::ostream& os, XmlRpc::XmlRpcValue& v) 
{ 
  int est_length = 0;
  // If you want to output in xml format:
  //return os << v.toXml(); 
  return v.write(os, 0, est_length);
}
