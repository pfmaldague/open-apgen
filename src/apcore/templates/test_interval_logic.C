#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <multitemplate.H>
#include <APdata.H>

static int usage(char* s) {
	cout << "usage: " << s << " (and|or|minus|xor) 1: <timeval> <timeval> ... 2: <timeval> <timeval> ...\n";
	cout << "       NOTE: an even number of strictly increasing times is expected within each list.\n";
	return -1;
}

static bool starts_with_number(char*& c) {
	bool	ok = false;
	while(*c && isdigit(*c)) {
		ok = true;
		c++;
	}
	return ok;
}

static bool (*membership_evaluator)(bool, bool) = NULL;

static bool interval_union(bool a, bool b) {
	return a || b;
}

static bool interval_intersection(bool a, bool b) {
	return a && b;
}

static bool interval_minus(bool a, bool b) {
	return a && !b;
}

static bool interval_xor(bool a, bool b) {
	return a != b;
}

int test_intervals(int argc, char* argv[]) {
	bool		valid = true;
	char*		t;
	CTime_base	T;
	long int	num_lists = 0;
	slist<alpha_int, Cnode0<alpha_int, slist<alpha_time, Cnode0<alpha_time, int> > > >	M;
	slist<alpha_time, Cnode0<alpha_time, int> >						L;

	if(argc <= 8) {
		return usage(argv[0]);
	}

	if(!strcmp(argv[1], "and")) {
		membership_evaluator = interval_intersection;
	} else if(!strcmp(argv[1], "or")) {
		membership_evaluator = interval_union;
	} else if(!strcmp(argv[1], "minus")) {
		membership_evaluator = interval_minus;
	} else if(!strcmp(argv[1], "xor")) {
		membership_evaluator = interval_xor;
	} else {
		cout << "first arg must be 'and', 'or', 'minus' or 'xor', not '" << argv[1] << "'\n";
		return -1;
	}	

	for(int i = 2; i < argc; i++) {
		char*		c = argv[i];
		int		state = 0;
		bool		is_number_followed_by_semicolon = false;
		bool		is_time_value = false;

		// starts with number updates c so it points to the first character following the number
		while(starts_with_number(c)) {
			if(state == 0 && *c == ':') {
				if(*++c == '\0') {
					if(L.get_length() > 0) {
						if(L.get_length() % 2) {
							cout << "List # " << num_lists << " has " << L.get_length() << " time(s) in it. "
								"The number of times must be even within each list.\n";
							break;
						}
						// save time values into an list
						M << new Cnode0<alpha_int, slist<alpha_time, Cnode0<alpha_time, int> > >(num_lists, L);
						L.clear();
						num_lists++;
					}
					is_number_followed_by_semicolon = true;
					break;
				}
			} else if(state == 0 && *c == '-') {
				// got year
				state = 1;
				c++;
			} else if(state == 1 && *c == 'T') {
				// got day
				state = 2;
				c++;
			} else if(state == 2 && *c == ':') {
				// got hours
				state = 3;
				c++;
			} else if(state == 3 && *c == ':') {
				// got minutes
				state = 4;
				c++;
			} else if(state == 4) {
			       if(*c == '.') {
					// got seconds, expect milliseconds
					state = 5;
					c++;
			       } else if(*c == '\0') {
					// got seconds, done
					T = CTime_base(argv[i]);
					if(L.get_length() && L.last_node()->getKey().getetime() >= T) {
						cout << "Given times must increase within each list\n";
						break;
					}
					is_time_value = true;
					L << new Cnode0<alpha_time, int>(T, 0);
					break;
				}
			} else if(state == 5 && *c == '\0') {
					// got milliseconds, done
					is_time_value = true;
					T = CTime_base(argv[i]);
					if(L.get_length() && L.last_node()->getKey().getetime() >= T) {
						cout << "Given times must increase within each list\n";
						break;
					}
					L << new Cnode0<alpha_time, int>(T, 0);
					break;
			} else {
				return usage(argv[0]);
			}
		}
		if(is_number_followed_by_semicolon) {
			cout << "got 'number:'\n";
		} else if(is_time_value) {
			cout << "got time value\n";
		} else {
			cout << "error found\n";
			break;
		}
	}

	// save the last list
	if(L.get_length() > 0) {
		if(L.get_length() % 2) {
			cout << "List # " << num_lists << " has " << L.get_length() << " time(s) in it. "
				"The number of times must be even within each list.\n";
		} else {
		// save time values into an list
		M << new Cnode0<alpha_int, slist<alpha_time, Cnode0<alpha_time, int> > >(num_lists, L);
		}
	}

	if(M.get_length() != 2) {
		cout << "Two lists of times must be supplied.\n";
		return -1;
	}

	// now we can conduct the test
	slist<alpha_time, Cnode0<alpha_time, int> >&					A(M.first_node()->payload);
	slist<alpha_time, Cnode0<alpha_time, int> >&					B(M.last_node()->payload);
	Miterator<slist<alpha_time, Cnode0<alpha_time, int> >, Cnode0<alpha_time, int> > miter("miterator");

	assert(A.get_length() > 0);
	assert(B.get_length() > 0);

	ListOVal	lov;
	ArrayElement*	ae;
	long int	Index = 0;

	miter.add_thread(A, "A", 0);
	miter.add_thread(B, "B", 1);
	miter.first();

	assert(miter.peek() != NULL);

	Cnode0<alpha_time, int>*		curnode = NULL;
	bool				wasInA = false;
	bool				wasInB = false;
	bool				wasInC = false;
	bool				isInA = false;
	bool				isInB = false;
	bool				isInC = false;
	while((curnode = miter.next())) {
		wasInA = isInA;
		wasInB = isInB;
		wasInC = isInC;
		if(curnode->list == &A) {
			isInA = !wasInA;
		} else if(curnode->list == &B) {
			isInB = !wasInB;
		}
		if(miter.peek() && miter.peek()->getKey().getetime() == curnode->getKey().getetime()) {
			curnode = miter.next();
			// note: intervals must have length > 0
			if(curnode->list == &A) {
				isInA = !wasInA;
			} else if(curnode->list == &B) {
				isInB = !wasInB;
			}
		}
		CTime_base	itime = curnode->getKey().getetime();
		isInC = membership_evaluator(isInA, isInB);

		cout << itime.to_string() << " - " << argv[1] << "(" << isInA << ", " << isInB << ") = " << isInC << "\n";

		if(isInC != wasInC) {
			lov.add(ae = new ArrayElement(Index++));
			ae->Val() = itime;
			cout << "\t" << itime.to_string() << "\n";
		}
	}

	// sanity check
	assert(!isInA);
	assert(!isInB);
	assert(!isInC);

	return 0;
}
