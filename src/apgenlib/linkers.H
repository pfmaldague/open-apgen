#ifndef _LINKERS_H_
#define _LINKERS_H_

// nC: nodeClass
template <class nC>
class baseLinker {
public:
  baseLinker(nC* M) : me(M) {}
  baseLinker(const baseLinker& bl) : me(bl.me) {}
  virtual ~baseLinker() {}

  nC*			me;
  virtual void		add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal) = 0;
  virtual void		add_cons_to_ancestors() = 0;
  virtual void		attach(		TreeDir::Dir right_or_left,
					nC* node_to_attach,
					TreeDir::cons_adjust CA) = 0;
  virtual void		attach_left(	nC* node_to_attach,
					TreeDir::cons_adjust CA) = 0;
  virtual void		attach_right(	nC* node_to_attach,
					TreeDir::cons_adjust CA) = 0;
  virtual double	get_consumption() const = 0;
  virtual valueHolder	get_i_cons() const = 0;
  virtual double	get_integral() const = 0;
  virtual double	get_integral_left() const = 0;
  virtual double	get_integral_right() const = 0;
  virtual true_long	get_int_consumption() const = 0;
  virtual valueHolder	get_l_cons() const = 0;
  virtual valueHolder	get_r_cons() const = 0;
  virtual nC*		get_parent() const = 0;
  virtual void		init_cons() = 0;
  virtual bool		is_float() const { return false; }
  virtual void		set_i_cons(const valueHolder* vh) = 0;
  virtual void		set_parent(nC*) = 0;
  virtual void		subtract_cons_from_ancestors(nC* stop_here = NULL) = 0;
};

template <class nC>
class simpleLinker: public baseLinker<nC> {
public:

  simpleLinker(nC* bn) : baseLinker<nC>(bn) {}
  simpleLinker(const simpleLinker& sl) : baseLinker<nC>(sl) {}
  ~simpleLinker() {}

  void			add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal) {}
  void			add_cons_to_ancestors() {}
  void			attach(		TreeDir::Dir right_or_left,
					nC* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_left(	nC* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_right(	nC* node_to_attach,
					TreeDir::cons_adjust CA);
  double		get_consumption() const { return 0.0; }
  true_long		get_int_consumption() const { return 0; }
  valueHolder		get_i_cons() const { return valueHolder(0L); }
  double		get_integral() const { return 0.0; }
  double		get_integral_left() const { return 0.0; }
  double		get_integral_right() const { return 0.0; }
  valueHolder		get_l_cons() const { return valueHolder(0L); }
  valueHolder		get_r_cons() const { return valueHolder(0L); }
  nC*			get_parent() const { return NULL; }
  void			init_cons() {}
  void			set_i_cons(const valueHolder* vh) {}
  void			set_parent(nC*) {}
  void			subtract_cons_from_ancestors(nC* stop_here = NULL) {} };

template <class bN>
void
simpleLinker<bN>::attach_left(bN* N, TreeDir::cons_adjust CA) {
	attach(TreeDir::LEFT_DIR, N, TreeDir::UPDATE_CONS); }

template <class bN>
void
simpleLinker<bN>::attach_right(bN* N, TreeDir::cons_adjust CA) {
	attach(TreeDir::RIGHT_DIR, N, TreeDir::UPDATE_CONS); }

template <class bN>
void
simpleLinker<bN>::attach(TreeDir::Dir right_or_left, bN* N, TreeDir::cons_adjust CA) {
	baseLinker<bN>::me->Links[right_or_left] = N; }


template <class bN>
class intLinker: public baseLinker<bN> {
public:
  bN*			avlParent;
  valueHolder		myCount;
  valueHolder		nCount_right;
  valueHolder		nCount_left;

  intLinker(bN* bn) :	baseLinker<bN>(bn),
			avlParent(NULL),
			myCount(1L),
			nCount_right(0L),
			nCount_left(0L) {}
  intLinker(const intLinker& il) : baseLinker<bN>(il) ,
			avlParent(NULL),
			myCount(1L),
			nCount_right(0L),
			nCount_left(0L) {}
  ~intLinker() {}

  void			add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal);
  void			add_cons_to_ancestors();
  void			attach(		TreeDir::Dir right_or_left,
					bN* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_left(	bN* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_right(	bN* node_to_attach,
					TreeDir::cons_adjust CA);
  double		get_consumption() const { return (double) get_int_consumption(); }
  true_long		get_int_consumption() const;
  valueHolder		get_i_cons() const { return myCount; }
  double		get_integral() const { return 0.0; }
  double		get_integral_left() const { return 0.0; }
  double		get_integral_right() const { return 0.0; }
  valueHolder		get_l_cons() const { return nCount_left; }
  valueHolder		get_r_cons() const { return nCount_right; }
  bN*			get_parent() const { return avlParent; }
  virtual void		init_cons() {
				avlParent = NULL;
				nCount_left = valueHolder(0L);
				nCount_right = valueHolder(0L); }
  void			set_i_cons(const valueHolder* vh) { myCount = vh ? *vh : valueHolder(0L); }
  void			set_parent(bN* b) { avlParent = b; }
  void			subtract_cons_from_ancestors(bN* stop_here = NULL); };

template <class bN>
void
intLinker<bN>::add_cons(
		TreeDir::Dir which_dir,
		const valueHolder& the_amount,
		const valueHolder& /* integralVal */) {
	if(which_dir == TreeDir::RIGHT_DIR) {
		nCount_right += the_amount; }
	else {
		nCount_left += the_amount; } }

template <class bN>
void
intLinker<bN>::add_cons_to_ancestors() {
	bN* p = baseLinker<bN>::me->get_parent();
	bN* t = baseLinker<bN>::me;

	while(p) {
		if(t == p->Links[TreeDir::LEFT_DIR]) {
			p->add_cons(TreeDir::LEFT_DIR, myCount, 0.0); }
		else {
			p->add_cons(TreeDir::RIGHT_DIR, myCount, 0.0); }
		t = p;
		p = p->get_parent(); } }

template <class bN>
void
intLinker<bN>::subtract_cons_from_ancestors(
		bN* until_this) {
	bN* p = baseLinker<bN>::me->get_parent();
	bN* t = baseLinker<bN>::me;

	while(p) {
		if(t == p->Links[TreeDir::LEFT_DIR]) {
			p->add_cons(TreeDir::LEFT_DIR, valueHolder(-myCount.get_content()), 0L); }
		else {
			p->add_cons(TreeDir::RIGHT_DIR, valueHolder(-myCount.get_content()), 0L); }
		t = p;
		p = p->get_parent();
		if(p == until_this) {
			break; } } }

template <class bN>
void
intLinker<bN>::attach_left(
		bN* N,
		TreeDir::cons_adjust CA) {
	attach(TreeDir::LEFT_DIR, N, CA); }

template <class bN>
void
intLinker<bN>::attach_right(
		bN* N,
		TreeDir::cons_adjust CA) {
	attach(TreeDir::RIGHT_DIR, N, CA); }

template <class bN>
void
intLinker<bN>::attach(
		TreeDir::Dir right_or_left,
		bN* N,
		TreeDir::cons_adjust CA) {
	bN* an = baseLinker<bN>::me;
	an->Links[right_or_left] = N;
	if(N) {
		N->set_parent(baseLinker<bN>::me);
		if(CA == TreeDir::UPDATE_CONS) {
			valueHolder	t(N->get_i_cons());
			t += N->get_l_cons();
			t += N->get_r_cons();
			if(right_or_left == TreeDir::RIGHT_DIR) {
				nCount_right = t; }
			else {
				nCount_left = t; } } }
	else if(CA == TreeDir::UPDATE_CONS) {
		if(right_or_left == TreeDir::RIGHT_DIR) {
			nCount_right = valueHolder(0L); }
		else {
			nCount_left = valueHolder(0L); } } }

template <class bN>
true_long
intLinker<bN>::get_int_consumption() const {
	true_long val = get_i_cons().get_content() + get_l_cons().get_content();
	const bN* q = baseLinker<bN>::me;
	const bN* p = avlParent;
	while(p) {
		if(q == p->get_link(TreeDir::RIGHT_DIR)) {
			val += p->get_i_cons().get_content() + p->get_l_cons().get_content(); }
		q = p;
		p = p->get_parent(); }
	return val; }

template <class bN>
class floatLinker: public baseLinker<bN> {
public:
  bN*			avlParent;
  valueHolder		myCount;
  valueHolder		nCount_right;
  valueHolder		nCount_left;
  valueHolder		integral_right, integral_left, my_integral;

  floatLinker(bN* bn) : baseLinker<bN>(bn),
			avlParent(NULL), myCount(0.0), nCount_right(0.0),
			nCount_left(0.0), my_integral(0.0),
			integral_right(0.0), integral_left(0.0) {}
  floatLinker(const floatLinker& fl) : baseLinker<bN>(fl),
			avlParent(NULL), myCount(0.0), nCount_right(0.0),
			nCount_left(0.0), my_integral(0.0),
			integral_right(0.0), integral_left(0.0) {}
  ~floatLinker() {}

  void			add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal);
  void			add_cons_to_ancestors();
  void			attach(		TreeDir::Dir right_or_left,
					bN* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_left(	bN* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_right(	bN* node_to_attach,
					TreeDir::cons_adjust CA);
  double		get_consumption() const;
  true_long		get_int_consumption() const { return 0; }
  double		get_integral() const;
  double		get_integral_left() const { return integral_left.get_double(); }
  double		get_integral_right() const { return integral_right.get_double(); }
  true_long		get_int_consumption() { return 0; }
  valueHolder		get_i_cons() const { return myCount; }
  valueHolder		get_l_cons() const { return nCount_left; }
  valueHolder		get_r_cons() const { return nCount_right; }
  bN*			get_parent() const { return avlParent; }
  virtual void		init_cons() {
				avlParent = NULL;
				nCount_left = valueHolder(0.0);
				nCount_right = valueHolder(0.0);
				// ??
				// myCount = valueHolder(0.0);
				my_integral = 0.0;
				integral_right = 0.0;
				integral_left = 0.0; }
  bool			is_float() const { return true; }
  void			set_i_cons(const valueHolder* vh) { myCount = vh ? *vh : valueHolder(0.0); }
  void			set_parent(bN* b) { avlParent = b; }
  void			subtract_cons_from_ancestors(bN* stop_here = NULL); };

template <class bN>
void
floatLinker<bN>::add_cons(
		TreeDir::Dir which_dir,
		const valueHolder &V,
		const valueHolder &I) {
	if(which_dir == TreeDir::RIGHT_DIR) {
		integral_right += I;
		nCount_right += V; }
	else {
		integral_left += I;
		nCount_left += V; } }

template <class bN>
void
floatLinker<bN>::add_cons_to_ancestors() {
	bN*		p = baseLinker<bN>::me->get_parent();
	bN*		t = baseLinker<bN>::me;
	CTime_base	t0(baseLinker<bN>::me->getKey().getetime());
	double		myTime = t0.convert_to_double_use_with_caution();
	double		deltaT;

	while(p) {
		CTime_base	t1(p->getKey().getetime());
		deltaT = t1.convert_to_double_use_with_caution() - myTime;
		if(t == p->Links[TreeDir::LEFT_DIR]) {
			p->add_cons(TreeDir::LEFT_DIR, myCount, valueHolder(myCount.get_double() * deltaT)); }
		else {
			p->add_cons(TreeDir::RIGHT_DIR, myCount, valueHolder(myCount.get_double() * deltaT)); }
		t = p;
		p = p->get_parent(); } }

template <class bN>
double
floatLinker<bN>::get_consumption() const {
	double val = get_i_cons().get_double() + get_l_cons().get_double();
	const bN* q = baseLinker<bN>::me;
	const bN* p = avlParent;
	while(p) {
		if(q == p->Links[TreeDir::RIGHT_DIR]) {
			val += p->get_i_cons().get_double() + p->get_l_cons().get_double(); }
		q = p;
		p = p->get_parent(); }
	return val; }

template <class bN>
double
floatLinker<bN>::get_integral() const {
	double val = integral_left.get_double();
	bN* q = baseLinker<bN>::me;
	bN* p = avlParent;
	CTime_base t1 = baseLinker<bN>::me->getKey().getetime(), t2;
	double deltaT;

	while(p) {
		if(q == p->Links[TreeDir::RIGHT_DIR]) {
			t2 = p->getKey().getetime();
			deltaT = t1.convert_to_double_use_with_caution() - t2.convert_to_double_use_with_caution();
			val += deltaT * (p->get_i_cons().get_double() + p->get_l_cons().get_double())
				+ p->get_integral_left(); }
		q = p;
		p = p->get_parent(); }
	return val; }

// keyClass'd better be alpha_time...
template <class bN>
void
floatLinker<bN>::subtract_cons_from_ancestors(
		bN* until_this) {
	bN*	p = get_parent();
	bN*	t = baseLinker<bN>::me;
	CTime_base t0 = baseLinker<bN>::me->getKey().getetime();
	double	deltaT;
	double	myTime = t0.convert_to_double_use_with_caution();

	while(p) {
		CTime_base t1(p->getKey().getetime());
		deltaT = t1.convert_to_double_use_with_caution() - myTime;
		if(t == p->Links[TreeDir::LEFT_DIR]) {
			p->add_cons(TreeDir::LEFT_DIR, valueHolder(-myCount.get_double()), valueHolder(-deltaT * myCount.get_double())); }
		else {
			p->add_cons(TreeDir::RIGHT_DIR, valueHolder(-myCount.get_double()), valueHolder(-deltaT * myCount.get_double())); }
		t = p;
		p = p->get_parent();
		if(p == until_this) {
			break; } } }

template <class bN>
void
floatLinker<bN>::attach(
		TreeDir::Dir right_or_left,
		bN* af,
		TreeDir::cons_adjust CA) {
	bN* an = baseLinker<bN>::me;
	an->Links[right_or_left] = af;
	if(af) {
		af->set_parent(baseLinker<bN>::me);
		if(CA == TreeDir::UPDATE_CONS) {
			valueHolder		t(af->get_i_cons());
			CTime_base		t1 = baseLinker<bN>::me->getKey().getetime(),
						t2 = af->getKey().getetime();
			double			deltaT;
			double			u = af->get_integral_left() + af->get_integral_right();

			deltaT = t1.convert_to_double_use_with_caution() - t2.convert_to_double_use_with_caution();
			t += af->get_l_cons();
			t += af->get_r_cons();
			u += t.get_double() * deltaT;
			if(right_or_left == TreeDir::RIGHT_DIR) {
				integral_right = u;
				nCount_right = t; }
			else {
				integral_left = u;
				nCount_left = t; } } }
	else if(CA == TreeDir::UPDATE_CONS) {
		if(right_or_left == TreeDir::RIGHT_DIR) {
			integral_right = valueHolder(0.0);
			nCount_right = valueHolder(0.0); }
		else {
			integral_left = valueHolder(0.0);
			nCount_left = valueHolder(0.0); } } }

template <class bN>
void
floatLinker<bN>::attach_left(
		bN* N,
		TreeDir::cons_adjust CA) {
	attach(TreeDir::LEFT_DIR, N, CA); }

template <class bN>
void
floatLinker<bN>::attach_right(
		bN* N,
		TreeDir::cons_adjust CA) {
	attach(TreeDir::RIGHT_DIR, N, CA); }

#endif /* _LINKERS_H_ */
