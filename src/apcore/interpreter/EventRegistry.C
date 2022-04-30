#include "EventRegistry.H"
#include "apDEBUG.H"
#include "Multiterator.H"
#include <ActivityInstance.H>
#include "RES_eval.H"
#include "EventImpl.H"
#include <assert.h>

using namespace std;

EventRegistry&
EventRegistry::Register() {
	static EventRegistry eventRegis;
	return eventRegis; }

void
EventRegistry::Subscribe(	const string& type,
				const string& nameString,
				EventRegistryClient* client) {
	//create type if necessary
	EventTypeMap::iterator there = EventTypes.find(type);

	if(there != EventTypes.end()) {
		there->second.insert(
			EventClientMap::value_type(nameString, client)); }
	else {
		EventClientMap newType;
		newType.insert(EventClientMap::value_type(nameString, client));
		EventTypes.insert(EventTypeMap::value_type(type, newType)); } }

void
EventRegistry::UnSubscribe(const string& type, const string& nameString) {
	EventTypeMap::iterator there = EventTypes.find(type);

	//trying to unsubscribe from a type that does not exist
	if(there != EventTypes.end()) {
		//will remove the client
		there->second.erase(nameString);

		//remove the type if empty
		if(there->second.empty())
			EventTypes.erase(type); } }

void
EventRegistry::GetRegisteredNamesForType(
		const string& type,
		StringVect* names) const {
	EventTypeMap::const_iterator there = EventTypes.find(type);

	if(there != EventTypes.end()) {
		EventClientMap::const_iterator iter = there->second.begin();
    
		while(iter != there->second.end()) {
			names->push_back(iter->first);
			iter++; } } }

EventRegistryClient*
EventRegistry::GetClient(const string& type, const string& name) const {
	EventTypeMap::const_iterator there = EventTypes.find(type);

	if(there != EventTypes.end()) {
		EventClientMap::const_iterator iter = there->second.find(name);
    
		if(iter != there->second.end()) {
			return iter->second; } }

	return NULL; }

void
EventRegistry::PropagateEvent(
		const string& type,
		const TypedValuePtrVect& arguments) {
	EventTypeMap::iterator there = EventTypes.find(type);

	if(there != EventTypes.end()) {
		EventClientMap::iterator iter = there->second.begin();

		while(iter != there->second.end()) {
			iter->second->HandleEvent(type, arguments);
			iter++; } } }

void
EventRegistry::PropagateEvent(
		const string& type,
		const string& name,
		const TypedValuePtrVect& arguments) {
	EventRegistryClient* client = GetClient(type, name);

	if(client) {
		client->HandleEvent(type, arguments); } }

UserFunctionEventClientManager::UserFunctionEventClientManager() {
	//subscribe to Purge Events
	EventRegistry::Register().Subscribe(
		"PURGE",
		"UserFunctionEventClientManager",
		this); }

UserFunctionEventClientManager::~UserFunctionEventClientManager() {
	//clean up clients
	UserFunctionPtrMap::iterator iter = Clients.begin();

	while(iter != Clients.end()) {
		delete iter->second;
		iter++; } }

UserFunctionEventClient*
UserFunctionEventClientManager::Create(const string& type, const string& name, const string& functionName)
{
	//key is type + name
	string key = type + name;

	UserFunctionPtrMap::iterator there = Clients.find(key);

	if(there != Clients.end())
		return there->second;

	UserFunctionEventClient *newClient = new UserFunctionEventClient(type, name, functionName);

	Clients.insert(UserFunctionPtrMap::value_type(key, newClient));

	//register here
	EventRegistry::Register().Subscribe(type, name, newClient);

	return newClient;
}

void
UserFunctionEventClientManager::Delete(const string& type, const string& name)
{
	string key = type + name;

	UserFunctionPtrMap::iterator there = Clients.find(key);

	if(there != Clients.end())
	{
		delete there->second;
		EventRegistry::Register().UnSubscribe(type, name);
		Clients.erase(key);
	}
}

void
UserFunctionEventClientManager::HandleEvent(
	const string& type, const TypedValuePtrVect& arguments) {
	Cstring arg = arguments[0]->get_string();

	if((type == "PURGE") && (arg == "ALL")) {
		UserFunctionPtrMap::iterator iter = Clients.begin();

		while(iter != Clients.end()) {
			//unsubscribe and delete
			EventRegistry::Register().UnSubscribe(
				iter->second->GetType(),
			iter->second->GetName());
			delete iter->second;
			iter++; }

		//clear the elements
		Clients.clear(); } }

UserFunctionPtrMap::const_iterator
UserFunctionEventClientManager::Begin() {
	return Clients.begin(); }

UserFunctionPtrMap::const_iterator
UserFunctionEventClientManager::End() {
	return Clients.end(); }

UserFunctionEventClientManager&
UserFunctionEventClientManager::Manager() {
	static UserFunctionEventClientManager manage;
	return manage; }

UserFunctionEventClient::UserFunctionEventClient(
	const string &type,
	const string &name,
	const string &functionName)
  : Name(name),
    Type(type),
    FunctionName(functionName)
	{}

void
UserFunctionEventClient::HandleEvent(
	const string& type,
	const TypedValuePtrVect& arguments) {
#ifdef GUTTED
	//need to ensure that the arguments are not modified
  
	//copy them
	TypedValuePtrVect tempVect;

	TypedValuePtrVect::const_iterator citer = arguments.begin();

	while(citer != arguments.end()) {
		TypedValue *newValue = new TypedValue(*(*citer));
		tempVect.push_back(newValue);
		citer++; }

	TypedValue result;

	try {
		Execute_AAF_function(
			FunctionName,
			&result,
			tempVect);
	} catch(eval_error& err) {
		Cstring errorString;
		errorString << "problem in UserFunctionEventClient::Event: ";
		errorString << err.message << "\n";
		throw(eval_error(errorString));
	}
  
	//clean up -- no matter what
	TypedValuePtrVect::iterator iter = tempVect.begin();

	while(iter != tempVect.end()) {
		delete *iter;
		iter++;
	}
#endif /* GUTTED */
}
