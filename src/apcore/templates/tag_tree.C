#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <templates.H>
#include <alphastring.H>

// #define apDEBUG

extern "C" {
#include "tag_tree.h"
}

class tagO;

class tagPLD {
public:
	tagPLD() : is_a_string(false), is_a_list(false), is_nothing(true) {}
	tagPLD(const Cstring& s) : equiv(s), is_a_string(true), is_a_list(false), is_nothing(false) {}
	tagPLD(int) : is_a_string(false), is_a_list(true), is_nothing(false) {}
	~tagPLD() {}

	// somewhat lame
	bool is_a_string;
	bool is_a_list;
	bool is_nothing;
	Cstring				equiv;
	tlist<alpha_string, tagO>	l;
	};

class tagO: public Dnode0<alpha_string, tagO> {
public:
	tagO() {}
	tagO(const Cstring& s) : Dnode0<alpha_string, tagO>(s) {}
	tagO(const Dnode0<alpha_string, tagO>& d) : Dnode0<alpha_string, tagO>(d) {}
	tagO(const Cstring& s, const Cstring& t) : Dnode0<alpha_string, tagO>(s), payload(t) {}
	tagO(const Cstring& s, int i) : Dnode0<alpha_string, tagO>(s), payload(i) {}

	tagO*	copy() { return new tagO(*this); }
	tagO*			get_this() { return this; }
	const tagO*		get_this() const { return this; }

	tagPLD	payload; };

// Hidden globals that are specific to resource definition parsing

using namespace std;

static tlist<alpha_string, tagO>&
theTrees() {
	static tlist<alpha_string, tagO> B;
	return B; }

extern "C" {

// resets or initializes a table of symbols (e. g. "globals", "instances")
void*
getAnEmptyListNamed(
		const char *Name) {
	tagO*	Tag;
	if((Tag = theTrees().find(Name))) {
		Tag->payload.l.clear(); }
	else {
		Cstring n(Name);
		theTrees() << (Tag = new tagO(n, 1)); }
	return &Tag->payload.l; }

void
deleteListNamed(const char* theName) {
	tagO*	Tag;
	if((Tag = theTrees().find(theName))) {
		delete Tag; } }

void*
getListNamed(
		const char* Name) {
	tagO*	Tag;
	if((Tag = theTrees().find(Name))) {
		return &Tag->payload.l; }
	return NULL; }

// defines or reports the existence of a symbol in a given table
void*
findInList(
		void* theList,
		const char* symbolName) {
	tagO*	theTag;

	theTag = ((tlist<alpha_string, tagO>*)theList)->find(symbolName);
	return (void *) theTag; }

int
getLengthOfList(
		void* theList) {
	return ((tlist<alpha_string, tagO>*)theList)->get_length(); }

void*
getIteratorForList(
		void* theList) {
	slist<alpha_string, tagO>::iterator* q;
	if(!theList) return NULL;
	q = new slist<alpha_string, tagO>::iterator(*((slist<alpha_string, tagO>*)theList));
	return (void *)q; }

void*
getNextNode(
		void* theIterator) {
	slist<alpha_string, tagO>::iterator*	lit = (slist<alpha_string, tagO>::iterator*) theIterator;
	if(lit) {
		tagO*	N = (*lit)();
		if(!N) {
			delete lit;
			return NULL; }
		return (void *) N; }
	return NULL; }

void*
getListOf(
		void* theNode) {
	tagO*	N = (tagO*) theNode;
	if(N && N->payload.is_a_list) {
		return &N->payload.l; }
	return NULL; }

const char*
getNodeKey(
		void* theNode) {
	if(theNode) {
		return *((tagO*) theNode)->get_key(); }
	return ""; }

const char*
getNodeValue(
		void* theNode) {
	tagO* n = (tagO*) theNode;
	if(n && n->payload.is_a_string) {
		return *n->payload.equiv; }
	return ""; }

void*
insertListNodeNamed(
		const char* theKey,
		void* theList) {
	if(!theList) return NULL;
	tlist<alpha_string, tagO>* l = (tlist<alpha_string, tagO>*) theList;
	tagO* Tag = new tagO(theKey, 1);
	(*l) << Tag;
	return (void*) Tag; }

void*
insertSymbolNodeNamed(
		const char* theKey,
		const char* theValue,
		void* theList) {
	if(!theList) return NULL;
	tlist<alpha_string, tagO>* l = (tlist<alpha_string, tagO>*) theList;
	tagO* bsn = new tagO(theKey, Cstring(theValue));
	(*l) << bsn;
	return (void *) bsn; }

} // extern "C"
