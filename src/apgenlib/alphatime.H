#ifndef _ALPHA_TIME_H_
#define _ALPHA_TIME_H_

#include <backpointer.H>
#include <UTL_time_base.H>

// a time-based class that owns its own event time and is suitable as a key for apnode
class alpha_time {
private:
// DATA
  unsigned long		secondaryKey;
  CTime_base		etime;
public:

// CONSTRUCTORS
  /* constructor */	alpha_time(const Cstring& s) : etime(s), secondaryKey(0) {}
  /* constructor */	alpha_time(const char* s) : etime(Cstring(s)), secondaryKey(0) {}
			alpha_time(const CTime_base& t) : etime(t), secondaryKey(0) {}
  /* constructor */	alpha_time(long d) : etime(d, 0, false), secondaryKey(0) {}
  /* constructor */	alpha_time(int s, int m) : etime(s, m, false), secondaryKey(0) {}
  /* constructor */	alpha_time(void* v) : etime((long)v, 0, false), secondaryKey(0) {}
  /* constructor */	alpha_time(const alpha_time& as) : etime(as.etime), /* this is the correct behavior: */ secondaryKey(0) {}
  virtual		~alpha_time() {}

// METHODS
  int			relTime(const CTime_base& t1, const CTime_base& t2) const {
				if(t1 > t2) return 1;
				else if(t1 == t2) return 0;
				return -1; }
  bool			operator > (const alpha_time& as) const {
				int f = relTime(etime, as.etime);
				if(!f) { return secondaryKey > as.secondaryKey; }
				return f > 0; }
  bool			operator < (const alpha_time& as) const {
				int f = relTime(etime, as.etime);
				if(!f) { return secondaryKey < as.secondaryKey; }
				return f < 0; }
  bool			operator == (const alpha_time& as) const {
				int f = relTime(etime, as.etime);
				if(!f) { return secondaryKey == as.secondaryKey; }
				return !f; }
  bool			operator != (const alpha_time& as) const {
				int f = relTime(etime, as.etime);
				if(!f) { return secondaryKey != as.secondaryKey; }
				return f != 0; }
  int			compare_to(const alpha_time& that, bool ignoreSecKey = false) const {
				int f = relTime(etime, that.etime);
				if(!f) {
					if(!ignoreSecKey) {
						if(secondaryKey > that.secondaryKey) {
							return 1; }
						else if(secondaryKey < that.secondaryKey) {
							return -1; }
						return 0; }
					else {
						return 0; } }
				return f; }
  static bool		disambiguates() { return true; }
  void			disambiguate(unsigned long& currMaxSecKey) { secondaryKey = ++currMaxSecKey; }
  const Cstring&	get_key() const { static Cstring s; s = etime.to_string(); return s; }
  unsigned long&	get_2nd_key() { return secondaryKey; }
  unsigned long		get_secondary_key() const { return secondaryKey; }
  Cstring		get_id() const { return Cstring(etime.to_string()); }
  virtual const CTime_base& getetime() const { return etime; }
  virtual void		setetime(const CTime_base& t) { etime = t; }
  static bool		reacts_to_time_events() { return true; }
  friend ostream& operator << (ostream& s, const alpha_string& x);
};


//
// a time-based class that owns its own event time and event ID;
// suitable as a key for resource value nodes
//
class prio_time {
// DATA
  unsigned long		secondaryKey;
public:
  CTime_base		etime;
  long			event_id;

// CONSTRUCTORS
			prio_time(
				const CTime_base&	t,
				long			p)
					: event_id(p), etime(t), secondaryKey(0) {}

			prio_time(const prio_time&	as)
					: etime(as.etime), event_id(as.event_id), secondaryKey(0) {}
  virtual		~prio_time() {}

private:
// METHODS
  int			relTime(const CTime_base& t1, const CTime_base& t2) const {
				return t1.pseudo_millisec > t2.pseudo_millisec ?
					1 : (t1.pseudo_millisec == t2.pseudo_millisec ?
						0 : -1);
				}
public:
  bool			operator > (const prio_time& as) const {
				return etime.pseudo_millisec > as.etime.pseudo_millisec ?
					true : (etime.pseudo_millisec == as.etime.pseudo_millisec ?
						(event_id > as.event_id ?
							true : (secondaryKey > as.secondaryKey ?
									true : false
							       )
						) : false);
				}
  bool			operator < (const prio_time& as) const {
				return etime.pseudo_millisec < as.etime.pseudo_millisec ?
					true : (etime.pseudo_millisec == as.etime.pseudo_millisec ?
						(event_id < as.event_id ?
							true : (secondaryKey < as.secondaryKey ?
									true : false
							       )
						) : false);
				}
  bool			operator == (const prio_time& as) const {
				return etime.pseudo_millisec != as.etime.pseudo_millisec ?
					false
					: (event_id != as.event_id ?
						false
						: (secondaryKey == as.secondaryKey ?
							true : false
						)
					);
				}
  bool			operator != (const prio_time& as) const {
				return etime.pseudo_millisec != as.etime.pseudo_millisec ?
					true
					: (event_id != as.event_id ?
						true : (secondaryKey != as.secondaryKey ?
							true : false
						)
					);
				}
  int			compare_to(const prio_time& that, bool ignoreSecKey = false) const {
				return etime.pseudo_millisec > that.etime.pseudo_millisec ?
				    	1
					: (etime.pseudo_millisec == that.etime.pseudo_millisec ?
					    (event_id > that.event_id ?
					        1
						: (event_id == that.event_id ?
						    (ignoreSecKey ?
						        0
						        : (secondaryKey > that.secondaryKey ?
						            1
						            : (secondaryKey == that.secondaryKey ?
							        0
							        : -1
							    )
						        )
					            )
						    : -1
						)
				    	    )
					    : -1
					);
			}
  static bool		disambiguates() { return true; }
  void			disambiguate(unsigned long& currMaxSecKey) { secondaryKey = ++currMaxSecKey; }
  const Cstring&	get_key() const { static Cstring s; s = etime.to_string(); return s; }
  unsigned long&	get_2nd_key() { return secondaryKey; }
  Cstring		get_id() const { return Cstring(etime.to_string()); }
  const CTime_base&	getetime() const { return etime; }
  long			get_event_id() const { return event_id; }
  void			set_event_id(long p) { event_id = p; }
  void			setetime(const CTime_base& t);
  static bool		reacts_to_time_events() { return true; }
  friend ostream& operator << (ostream& s, const alpha_string& x);
};

#endif /* _ALPHA_TIME_H_ */
