#ifndef ACTION_REQUEST_CLIENT_H
#define ACTION_REQUEST_CLIENT_H

class Action_request;
class Cstring;
///
class Action_request_client
{
public:
  
  ///
  virtual ~Action_request_client(){};

  ///
  virtual void IDReceived(Action_request* ar, const Cstring& id) {return;};

};





#endif
