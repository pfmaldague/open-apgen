#ifndef _EVAL_TIMING_H_
#define _EVAL_TIMING_H_

class Rsource;

namespace EvalTiming {

  typedef enum _strategy {
	NORMAL,
	BEFORE,
	INTERPOLATED
  } Strategy;


  class strategy_setter {
  public:
	strategy_setter() = delete;
	strategy_setter(const strategy_setter&) = delete;
	strategy_setter(Rsource* res, Strategy strat);
	~strategy_setter();

	Rsource* my_resource;
	Strategy new_strategy;
	Strategy old_strategy;
  };

  const char* spell_strategy(Strategy s);

} /* namespace EvalTiming */

#endif /* _EVAL_TIMING_H_ */
