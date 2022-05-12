#if HAVE_CONFIG_H
#include <config.h>
#endif

#define debug_templates

#include <APbasic.H>

extern int	test_multi(		int argc,	char* argv[]);
extern int	test_time(		int argc,	char* argv[]);
extern int	test_typed_value(	int argc,	char* argv[]);
extern int	test_json(		int argc,	char* argv[]);
extern int	test_json_parser(	int argc,	char* argv[]);
extern int	test_xslt(		int argc,	char* argv[]);
// extern int	test_double_list(	int argc,	char* argv[]);
#ifdef have_xmltol
extern int	test_xml2(		int argc,	char* argv[]);
extern int	test_xml_parsing(	int argc,	char* argv[]);
extern int	test_json_resource(	int argc,	char* argv[]);
#endif
extern int	test_regex(		int argc,	char* argv[]);
extern int	test_intervals(	int argc,	char* argv[]);
extern int 	test_gtest(		int argc, 	char* argv[]);

		/* we make this one a handle because it is
		 * implemented in a higher-level library: */
int		(*interp_windows_handle)(int argc, char* argv[]) = NULL;

static int usage(char*s) {
	cout << "usage: " << s << " <-nocrash>\n";
	return -1; }

static int test1(bool crash);
static int test2(bool crash);
static int test3();
static int test4(bool crash);
static int test0(int argc, char* argv[]);

static const char* test_names[] = {
	"test",
	"test_time",
	"test_multi",
	"test_typed_value",
	"test_json",
	"test_json_parser",
#ifdef have_xmltol
	"test_xml2",
	"test_xml_parsing",
	"test_json_resource",
#endif
	"test_xslt",
//	"test_double_list",
	"test_regex",
	"test_intervals",
	"test_interp_windows",

#ifdef have_gTest
	"test_gtest",
#endif

	NULL };

const char** help_test() {
	return test_names; }

int test_harness(int argc, char* argv[]) {
	if(!strcmp(argv[0], "test")) {
		return test0(argc, argv);
	} else if(!strcmp(argv[0], "test_time")) {
		return test_time(argc, argv);
	} else if(!strcmp(argv[0], "test_multi")) {
		return test_multi(argc, argv);
	} else if(!strcmp(argv[0], "test_typed_value")) {
		return test_typed_value(argc, argv);
	} else if(!strcmp(argv[0], "test_json")) {
		return test_json(argc, argv);
	} else if(!strcmp(argv[0], "test_json_parser")) {
		return test_json_parser(argc, argv);
	}
#ifdef have_xmltol
	else if(!strcmp(argv[0], "test_xml2")) {
		return test_xml2(argc, argv);
	} else if(!strcmp(argv[0], "test_xml_parsing")) {
		return test_xml_parsing(argc, argv);
	} else if(!strcmp(argv[0], "test_json_resource")) {
		return test_json_resource(argc, argv);
	}
#endif
	else if(!strcmp(argv[0], "test_xslt")) {
		return test_xslt(argc, argv);
//	} else if(!strcmp(argv[0], "test_double_list")) {
//		return test_double_list(argc, argv);
	} else if(!strcmp(argv[0], "test_regex")) {
		return test_regex(argc, argv);
	} else if(!strcmp(argv[0], "test_intervals")) {
		return test_intervals(argc, argv);
	} else if(!strcmp(argv[0], "test_interp_windows")) {
		if(!interp_windows_handle) {
			Cstring any_errors = "interp_windows handle was not set; cannot run test.\n";
			throw(eval_error(any_errors));
		} else {
			return interp_windows_handle(argc, argv);
		}
	}
	//NEW

#ifdef have_gTest
	else if (!strcmp(argv[0], "test_gtest")){
		return test_gtest(argc, argv);
	}
#endif

	//END NEW
	Cstring any_errors = "option -test should be followed by a unique test name and optional args.\n";
	any_errors << "Valid test names:\n";
	const char**	theNames = help_test();
	const char*	which_name = theNames[0];
	while(which_name) {
		any_errors << "\t" << which_name << "\n";
		which_name = *(++theNames);
		// the last one is NULL, so don't print!
	}
	throw(eval_error(any_errors));
}


int test0(int argc, char* argv[]) {
	bool crash = true;

	if(argc > 1) {
		if(argc != 2) {
			return usage(argv[0]); }
		if(!strncmp(argv[1], "-nocrash", strlen("-nocrash"))) {
			crash = false; }
		else return usage(argv[0]); }

	return test1(crash) || test2(crash) || test3() || test4(crash); }

/* This class follows the paradigm for defining a back-pointer.
 * The only requirement on the class is that it must implement
 * the pure virtual function get_id(). */
class Important {
public:
  backptr<Important>	ptrs;

  Important(const Cstring& s) : ID(s) {}
  ~Important() { ptrs.delete_pointers_to_this(); }

  Cstring ID;
  Cstring get_id() const { return ID; }
  backptr<Important>* Pointers() { return &ptrs; }
};

// This is the canonical way of defining a ptr to a backptr.
class imp: public baseC<alpha_void, imp>, public p2b<Important> {
public:
	imp(void* v, const Cstring& s, Important* i)
		: baseC<alpha_void, imp>(v), payload(s),
		p2b<Important>(i) {}
	imp(const imp& I) : baseC<alpha_void, imp>(I), p2b<Important>(I), payload(I.payload) {}
	~imp() {
		if(list) {
			list->remove_node(this); }
		cout << payload << " disappearing\n"; }
  void	remove(bool S) { if(S) baseC<alpha_void, imp>::temporarily_remove_from_list();
			else if(list) list->remove_node(this); }
  bool	reacts_to_time_events() const { return false; }
  void	re_insert() { baseC<alpha_void, imp>::re_insert_in_list(); }
  imp*	copy() { return new imp(*this); }
  imp*	get_this() { return this; }
  const imp*	get_this() const { return this; }
  // virtual const char* className() const { return "imp"; }
  virtual void*	voidThis() { return this; }
  Cstring	payload;
};

// This is the canonical way of defining a ptr to a backptr.
class imp2: public baseC<alpha_void, imp2>, public p2b<Important> {
public:
	imp2(void* v, const Cstring& s, Important* i)
		: baseC<alpha_void, imp2>(v),
		payload(s),
		p2b<Important>(i) {}
	imp2(const imp2& I) : baseC<alpha_void, imp2>(I), payload(I.payload), p2b<Important>(I) {}
	~imp2() {
		if(list) {
			list->remove_node(this); }
		cout << payload << " disappearing\n"; }
  void	remove(bool S) { if(S) baseC<alpha_void, imp2>::temporarily_remove_from_list();
			else if(list) list->remove_node(this); }
  bool	reacts_to_time_events() const { return false; }
  void	re_insert() { baseC<alpha_void, imp2>::re_insert_in_list(); }
  imp2*	copy() { return new imp2(*this); }
  imp2*	get_this() { return this; }
  const imp2*	get_this() const { return this; }

  // virtual const char* className() const { return "imp2"; }
  virtual void*	voidThis() { return this; }
  Cstring	payload;
};

// This is the canonical way of defining a ptr to a backptr.
class imp3: public baseC<alpha_void, imp3>, public p2b<Important> {
public:
	imp3(void* v, const Cstring& s, Important* i)
		: baseC<alpha_void, imp3>(v),
		payload(s),
		p2b<Important>(i) {}
	imp3(const imp3& I) : baseC<alpha_void, imp3>(I), p2b<Important>(I), payload(I.payload) {}
	~imp3() {
		if(list) {
			list->remove_node(this); }
		cout << payload << " disappearing\n"; }
  void	remove(bool S) { if(S) baseC<alpha_void, imp3>::temporarily_remove_from_list();
			else if(list) list->remove_node(this); }
  bool	reacts_to_time_events() const { return false; }
  void	re_insert() { baseC<alpha_void, imp3>::re_insert_in_list(); }
  imp3*	copy() { return new imp3(*this); }
  imp3*	get_this() { return this; }
  const imp3*	get_this() const { return this; }

  Cstring special_imp3_method() { return "Hello, I am imp3 and I have a special method!"; }

  // virtual const char* className() const { return "imp3"; }
  virtual void*	voidThis() { return this; }
  Cstring	payload;
};

class ai : public baseC<alpha_string, ai> {
    public:
	ai() {}
	ai(const Cstring& s, int i) : baseC<alpha_string, ai>(s), payload(i) {}
	ai(const Cstring& s) : baseC<alpha_string, ai>(s), payload(0) {}
	~ai() {}

	ai*		copy() { return new ai(*this); }
  ai*	get_this() { return this; }
  const ai*	get_this() const { return this; }

	int		payload; };

/* test1 tests
 * 	- simple list
 * 	- simple back-pointer */
int test1(bool crash) {
	slist<alpha_string, ai>		L;
	slist<alpha_void, imp>		M;
	slist<alpha_void, imp2>		N;
	slist<alpha_void, imp3>		P;
	imp*				m1;
	imp2*				m2;
	imp3*				m3;
	ai*				T;
	Important*			important_ptr = NULL;

	try {
	L << new ai("atom", 11);
	L << new ai("molecule", 12);
	L << new ai("atom", 91);
	L << new ai("atom", 71);
	L << new ai("atom2", 1); }
	catch(eval_error Err) {
		cout << "Error:\n" << Err.msg << "\n";
		exit(-1); }
	slist<alpha_string, ai>::iterator iter(L);
	int i = 0;
	while((T = iter())) {
		cout << i++ << ": (" << T->get_key() << ", " << T->payload << ")\n"; }
	L.clear();

	important_ptr = new Important("Big guy");

	M << (m1 = new imp(NULL, "I am m1", important_ptr));
	N << (m2 = new imp2(NULL, "I am m2", important_ptr));
	P << (m3 = new imp3(NULL, "I am m3", important_ptr));
	delete m2;
	delete m1;

	backptr<Important>::ptr2p2b* aPtrNode;
	tlist<alpha_void, backptr<Important>::ptr2p2b>::iterator
		aPtrNodeIterator(important_ptr->Pointers()->pointers);
	while((aPtrNode = aPtrNodeIterator())) {
		imp3*	imp3_ptr = dynamic_cast<imp3*>(aPtrNode->payload);
		if(imp3_ptr) {
			cout << "Aha, this is an imp3; calling special method: return is "
				<< imp3_ptr->special_imp3_method() << "\n"; } }
	delete important_ptr;
	cout << "All done, returning.\n";
	return 0; }

class stringintnode: public baseC<alpha_string, stringintnode> {
    public:
	stringintnode() : payload(0) {}
	stringintnode(const Cstring& s, int i) : baseC<alpha_string, stringintnode>(s), payload(i) {}
	stringintnode(const stringintnode& d) : baseC<alpha_string, stringintnode>(d), payload(d.payload) {}
	stringintnode(const baseC<alpha_string, stringintnode>& d) : baseC<alpha_string, stringintnode>(d), payload(0) {}
	~stringintnode() {
		if(list) {
			list->remove_node(this); } }

	stringintnode*	copy() { return new stringintnode(*this); }
  stringintnode*	get_this() { return this; }
  const stringintnode*	get_this() const { return this; }
	
	long	payload; };

class intintnode: public baseC<ualpha_int, intintnode> {
    public:
	intintnode() : payload(0), baseC<ualpha_int, intintnode>(0L) {}
	intintnode(int s, int i) : baseC<ualpha_int, intintnode>(s), payload(i) {}
	intintnode(const intintnode& d) : baseC<ualpha_int, intintnode>(d), payload(d.payload) {}
	intintnode(const baseC<ualpha_int, intintnode>& d) : baseC<ualpha_int, intintnode>(d), payload(0) {}
	~intintnode() {
		if(list) {
			list->remove_node(this); } }

	intintnode*	copy() { return new intintnode(*this); }
  intintnode*	get_this() { return this; }
  const intintnode*	get_this() const { return this; }
	long	payload; };

class voidintnode: public baseC<alpha_void, voidintnode> {
    public:
	voidintnode() : payload(0) {}
	voidintnode(void* s, int i) : baseC<alpha_void, voidintnode>(s), payload(i) {}
	voidintnode(const voidintnode& d) : baseC<alpha_void, voidintnode>(d), payload(d.payload) {}
	voidintnode(const baseC<alpha_void, voidintnode>& d) : baseC<alpha_void, voidintnode>(d), payload(0) {}
	~voidintnode() {
		if(list) {
			list->remove_node(this); } }

	voidintnode*	copy() { return new voidintnode(*this); }
  voidintnode*	get_this() { return this; }
  const voidintnode*	get_this() const { return this; }
	
	long	payload; };


/* test2 tests
 * 	- simple tlist
 */
int test2(bool crash) {
	tlist<alpha_string, stringintnode> L;
	tlist<ualpha_int, intintnode> M;
	tlist<alpha_void, voidintnode> Q;

	try {
	L << new stringintnode("atom", 11);
	L << new stringintnode("advance", 10);
	L << new stringintnode("zealous", 27);
	L << new stringintnode("magistrate", 94);
	L << new stringintnode("advance", 19);
	L << new stringintnode("atom", 11);
	L << new stringintnode("atom", 112);
	L << new stringintnode("atom", 110);
	L << new stringintnode("atom", 111);
	L << new stringintnode("atom", 113);
	L << new stringintnode("atom", 114);
	L << new stringintnode("atom", 115);
	L << new stringintnode("atom", 116);
	L << new stringintnode("atom", 117);
	L << new stringintnode("atom", 118);
	L << new stringintnode("atom", 119);
	L << new stringintnode("atom", 120);
	L << new stringintnode("advance", 319);
	L << new stringintnode("Merlot", 623); }
	catch(eval_error Err) {
		cout << "Error:\n" << Err.msg << "\n";
		exit(-1); }


	cout << "In insertion order (using next_node()):\n";

	stringintnode* N = L.first_node();
	while(N) {
		cout << "  " << N->get_key() << ", " << N->payload << "\n";
		N = N->next_node(); }

	cout << "In insertion order (using iterator):\n";

	tlist<alpha_string, stringintnode>::iterator iter(L);
	while((N = iter())) {
		cout << "  " << N->get_key() << ", " << N->payload << "\n"; }

	cout << "In tlist order:\n";
	N = L.earliest_node();
	while(N) {
		cout << "  " << N->get_key() << ", " << N->payload << "\n";
		N = N->following_node(); }
	if((N = L.find("Merlot"))) {
		cout << "deleting Merlot...\n";
		delete N; }
	cout << "In tlist order after deletion:\n";
	N = L.earliest_node();
	while(N) {
		cout << "  " << N->get_key() << ", " << N->payload << "\n";
		N = N->following_node(); }

	intintnode* O = NULL;
	try {
		M << (O = new intintnode(12, 13));
		M << (O = new intintnode(1024, 13));
		M << (O = new intintnode(-120, 31415));
		M << (O = new intintnode(94, 20));
		if(crash) {
			M << (O = new intintnode(94, 21)); } }
	catch(eval_error Err) {
		if(O) delete O;
		cout << "Error:\n" << Err.msg << "\n";
		return(-1); }

	cout << "In insertion order:\n";
	O = M.first_node();
	while(O) {
		cout << "  " << O->get_key() << ", " << O->payload << "\n";
		O = O->next_node(); }
	cout << "In tlist order:\n";
	O = M.earliest_node();
	while(O) {
		cout << "  " << O->get_key() << ", " << O->payload << "\n";
		O = O->following_node(); }
	if((O = M.find("-120"))) {
		cout << "deleting -120...\n";
		delete O; }
	cout << "In tlist order after deletion:\n";
	O = M.earliest_node();
	while(O) {
		cout << "  " << O->get_key() << ", " << O->payload << "\n";
		O = O->following_node(); }

	int a = 1, b = 2, c = 3, d = 4;
	voidintnode* P = NULL;
	try {
		Q << (P = new voidintnode(&c, 0));
		Q << (P = new voidintnode(&a, 0));
		Q << (P = new voidintnode(&b, 2)); }
	catch(eval_error Err) {
		if(P) delete P;
		cout << "Error:\n" << Err.msg << "\n"; }
	cout << "In insertion order:\n";
	tlist<alpha_void, voidintnode>::iterator viter(Q);
	while((P = viter())) {
		cout <<  "  " << P->get_key() << ", " << P->payload << "\n"; }
	cout << "In tlist order:\n";
	P = Q.earliest_node();
	while(P) {
		cout <<  "  " << P->get_key() << ", " << P->payload << "\n";
		P = P->following_node(); }
	return 0; }

/* test4 tests
 * 	- simple tlist with synchronize_orders = true
 */
int test4(bool crash) {
	tlist<alpha_string, stringintnode> L(true);
	tlist<ualpha_int, intintnode> M;
	tlist<alpha_void, voidintnode> Q;

	try {
	L << new stringintnode("atom", 11);
	L << new stringintnode("advance", 10);
	L << new stringintnode("zealous", 27);
	L << new stringintnode("magistrate", 94);
	L << new stringintnode("advance", 19);
	L << new stringintnode("atom", 11);
	L << new stringintnode("atom", 112);
	L << new stringintnode("atom", 110);
	L << new stringintnode("atom", 111);
	L << new stringintnode("atom", 113);
	L << new stringintnode("atom", 114);
	L << new stringintnode("atom", 115);
	L << new stringintnode("atom", 116);
	L << new stringintnode("atom", 117);
	L << new stringintnode("atom", 118);
	L << new stringintnode("atom", 119);
	L << new stringintnode("atom", 120);
	L << new stringintnode("advance", 319);
	L << new stringintnode("Merlot", 623); }
	catch(eval_error Err) {
		cout << "Error:\n" << Err.msg << "\n";
		exit(-1); }


	cout << "In insertion order (using next_node()):\n";

	stringintnode* N = L.first_node();
	while(N) {
		cout << "  " << N->get_key() << ", " << N->payload << "\n";
		N = N->next_node(); }

	cout << "In insertion order (using iterator):\n";

	tlist<alpha_string, stringintnode>::iterator iter(L);
	while((N = iter())) {
		cout << "  " << N->get_key() << ", " << N->payload << "\n"; }

	cout << "In tlist order:\n";
	N = L.earliest_node();
	while(N) {
		cout << "  " << N->get_key() << ", " << N->payload << "\n";
		N = N->following_node(); }
	if((N = L.find("Merlot"))) {
		cout << "deleting Merlot...\n";
		delete N; }
	cout << "In tlist order after deletion:\n";
	N = L.earliest_node();
	while(N) {
		cout << "  " << N->get_key() << ", " << N->payload << "\n";
		N = N->following_node(); }

	intintnode* O = NULL;
	try {
		M << (O = new intintnode(12, 13));
		M << (O = new intintnode(1024, 13));
		M << (O = new intintnode(-120, 31415));
		M << (O = new intintnode(94, 20));
		if(crash) {
			M << (O = new intintnode(94, 21)); } }
	catch(eval_error Err) {
		if(O) delete O;
		cout << "Error:\n" << Err.msg << "\n";
		return(-1); }

	cout << "In insertion order:\n";
	O = M.first_node();
	while(O) {
		cout << "  " << O->get_key() << ", " << O->payload << "\n";
		O = O->next_node(); }
	cout << "In tlist order:\n";
	O = M.earliest_node();
	while(O) {
		cout << "  " << O->get_key() << ", " << O->payload << "\n";
		O = O->following_node(); }
	if((O = M.find("-120"))) {
		cout << "deleting -120...\n";
		delete O; }
	cout << "In tlist order after deletion:\n";
	O = M.earliest_node();
	while(O) {
		cout << "  " << O->get_key() << ", " << O->payload << "\n";
		O = O->following_node(); }

	int a = 1, b = 2, c = 3, d = 4;
	voidintnode* P = NULL;
	try {
		Q << (P = new voidintnode(&c, 0));
		Q << (P = new voidintnode(&a, 0));
		Q << (P = new voidintnode(&b, 2)); }
	catch(eval_error Err) {
		if(P) delete P;
		cout << "Error:\n" << Err.msg << "\n"; }
	cout << "In insertion order:\n";
	tlist<alpha_void, voidintnode>::iterator viter(Q);
	while((P = viter())) {
		cout <<  "  " << P->get_key() << ", " << P->payload << "\n"; }
	cout << "In tlist order:\n";
	P = Q.earliest_node();
	while(P) {
		cout <<  "  " << P->get_key() << ", " << P->payload << "\n";
		P = P->following_node(); }
	return 0; }

char aSmartPointer_string[] = "aSmartPointer";

class Global {
public:
	Global() : g("Hello") {}
	Global(const Global& G) : g(G.g) {}
	~Global() {}

	Cstring g;
};

// for creating true global symbols:
typedef BPnode<ualpha_string, Global> GlobalSymbolObj;
typedef BPptr<ualpha_string, GlobalSymbolObj> ptr2globalObj;

int test3() {
	tlist<ualpha_string, GlobalSymbolObj>		ListOfGlobalSymbols;
	tlist<ualpha_string, ptr2globalObj>		ListOfEpochs;
	tlist<ualpha_string, ptr2globalObj>		ListOfTimeSystems;

	Global j;
	GlobalSymbolObj* N = new GlobalSymbolObj("a new symbol", j);

	ListOfGlobalSymbols << N;
	ListOfEpochs << new ptr2globalObj("an epoch name", N);
	return 0; }
