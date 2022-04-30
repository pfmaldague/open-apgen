#ifndef EVENT_REGISTRY_H
#define EVENT_REGISTRY_H

#include "UTL_stl.H"
#include "RES_eval.H"

#include <map>

typedef std::vector<TypedValue*> TypedValuePtrVect;

//client for Event Registry.
class EventRegistryClient {
public:

  //method called when event occurs.
  virtual void HandleEvent(
		const string& type,
		const TypedValuePtrVect& arguments) = 0; };


//to facilitate the communication between various components of the system
typedef std::map<string, EventRegistryClient*> EventClientMap;

typedef std::map<string, EventClientMap> EventTypeMap;

/*The Event Registry.
 *A class to facilitate communication between various components of the system.
 *this registry holds pointers to clients, it does not delete those pointers.
 */
class EventRegistry {

  //private constructor, this is a singleton
  EventRegistry() {};

  //private map
  EventTypeMap EventTypes;

public:

  //To get the handle
  static EventRegistry& Register();

  //Subscribe your client with a name and a type of event to subscribe to
  void Subscribe(
		const string& type,
		const string& nameString,
		EventRegistryClient* client);
  //this will remove your subscription
  void UnSubscribe(const string& type, const string& nameString);

  //This gets the names of the clients for an event type
  void GetRegisteredNamesForType(const string& type, StringVect* names) const;

  //Will get you a specific client's handle, returns NULL if not there
  EventRegistryClient* GetClient(const string& type, const string& nameString) const; 

  ///When an event occurs.  Broadcasts to all subscribers
  void PropagateEvent(
		const string& type,
		const TypedValuePtrVect& arguments);
  ///When an event occurs.  Sends to the named subscriber
  void PropagateEvent(
		const string& type,
		const string& name,
		const TypedValuePtrVect& arguments);
};

class UserFunctionEventClient;

typedef std::map<string, UserFunctionEventClient*> UserFunctionPtrMap;

///a class to manage UserFunctionEventClients.Subscribes and UnSubscribes when appropriate.
class UserFunctionEventClientManager : public EventRegistryClient {
	//the clients
	UserFunctionPtrMap Clients;

	//the private constructor, a singleton.
	UserFunctionEventClientManager();

public:
	//destructor
	~UserFunctionEventClientManager();
	//the accessor.
	static UserFunctionEventClientManager& Manager();

	//Creates a new client.
	UserFunctionEventClient* Create(
			const string& type,
			const string& name,
			const string& functionName);
	//Deletes the client by name.
	void Delete(const string& type, const string& name);
	//Because the Manager is subscribed to the PURGE event.
	void HandleEvent(const string &type, const TypedValuePtrVect& arguments);

	//For iterating through the clients.
	UserFunctionPtrMap::const_iterator Begin();
	//For iterating through the clients.
	UserFunctionPtrMap::const_iterator End(); };

///An Event Registry client that calls an AAF function with the arguments
class UserFunctionEventClient : public EventRegistryClient {
	//Type of client
	const string Type;
	//Name of client
	const string Name;
	//Name of function
	const string FunctionName;

	//private destructor -- use the UserFunctionEventClientManager to create these.
	~UserFunctionEventClient() {};
	//private constructor -- use the UserFunctionEventClientManager to create these.
	UserFunctionEventClient(
		const string& type,
		const string& name,
		const string& functionName);

	friend class UserFunctionEventClientManager;
public:

	//Type accessor
	string GetType() const {return Type;};
	//Name accessor
	string GetName() const {return Name;};
	//FunctionName accessor
	string GetFunctionName() const {return FunctionName;};

	//When an event occurs.
	void HandleEvent(const string& type, const TypedValuePtrVect& arguments);
};


#endif
