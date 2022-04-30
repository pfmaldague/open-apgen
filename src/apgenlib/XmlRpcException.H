#ifndef _XMLRPCEXCEPTION_H_
#define _XMLRPCEXCEPTION_H_
//
// XmlRpc++ Copyright (c) 2002-2003 by Chris Morley
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)    // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
# include <string>
#endif

#include <stdexcept>


namespace XmlRpc {

  //! A class representing an error.
  //! If server methods throw this exception, a fault response is returned
  //! to the client.
  class XmlRpcException {
  public:
    //! Constructor
    //!   @param message  A descriptive error message
    //!   @param code     An integer error code
    XmlRpcException(const std::string& message, int code=-1) :
        _message(message), _code(code) {}

    //! Return the error message.
    const std::string& getMessage() const { return _message; }

    //! Return the error code.
    long getCode() const { return _code; }

	// 'standard' error codes for APcore server
	enum ErrorCode {
		TESTING = 0,
		BADMETHOD = 2,
		EMPTYPLAN = 4,
		NOSUCHID = 6,
		IDEXISTS = 8,
		NOSUCHPAR = 10,
		NOSUCHATT = 12,
		ATTUNDEF = 14,
		PARUNDEF = 16,
		BADPARNUM = 18,
		BADPARTYPE = 20,
		BADPARVAL = 22,
		BADSYNTAX = 24,
		OVERINIT = 26,
		UNDERINIT = 28,
		MODELERR = 30,
		NOSUBSYSTEM = 32};

  private:
    std::string _message;
    long _code;
  };

}

#endif	// _XMLRPCEXCEPTION_H_
