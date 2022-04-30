# TOL output (including XMLTOL)

## Introduction

TOL and XMLTOL output is one of the most time-consuming tasks in APGenX modeling, due to the sheer size of the data to be output. TOL files routinely exceed 1 Gb in size; XMLTOL files can be about 3 times as big. As it turns out, the amount of processing performed for each data item is fairly small - mostly querying a resource for its value at a given time. Not only that, but by their very definition - time-ordered listings - these files are created by scanning activity and resource histories in time order. As a result, TOL and XMLTOL output are ideal examples of processes that can run concurrently with the remodeling process, as long as they "stay behind" the modeling process in terms of modeling time (a.k.a. 'now').

Since constraint checking is already done concurrently with modeling, the same process was adapted for use by TOL processing also. 

## Basic algorithm

TOLs contain three types of records:
- activity start and end
- constraint violations and release
- resource values

Here we focus on resource values. The main and only source of information for resource values is the set of resource histories, one for each resource. A resource history contains nodes that are instances of the _value_node_ class. The nodes are contained in two simultaneous containers, a linked list and a balanced tree. The balanced tree is used for fast (logarthmic) access to nodes with a specified time tag. The modeling thread continuously inserts new nodes into resource histories, and as it does so the topology of the balanced tree changes. Therefore, value_node data members that pertain to the balanced tree structure cannot safely be accessed by threads other than the modeling thread.

The linked list structure of value_nodes, however, does not change globally when new nodes are inserted into the list; only pointers of nodes immediately preceding and following the new node are affected. Since APGenX modeling proceeds strictly forward in time, one can rely on the following "invariant":

```
the payload and previous_node pointer of a history node with a time tag
less than or equal to modeling::current_time() can be accessed safely
by all threads
```

Here, modeling::current\_time() refers to the time tag of the event currently being processed in the modeling loop. This invariant property is exploited by a "miterator" (multi-iterator) called _potential_triggers_ (in the EventLoop class); it is managed by the modeling thread as part of the event loops ProcessEvents() algorithm. The algorithm makes sure that the following properties are invariant:

```
The iterNode pointer of each iterator in potential_triggers (a leading
iterator, see templates.H) points to the first history node with a time
tag strictly greater than modeling::current_time()

The safeNode pointer of each such iterator is suitable for evaluation
of the current value of the resource (a.k.a. currentval())
```

The benefit of having this miterator is that the currentval method of any resource can be evaluated very efficiently: no search is needed.

A second benefit is that trailing threads can safely access history nodes using the linked list structure that contains them, provided they stay behind the safeNode pointer. Note incidentally that this is made possible by the links that exist between resources, resource histories, and any miterator that has registered itself with a resource history. Thanks to this registration mechanism, the resource has access to the iterator that represents it inside the miterator, hence to the node in its history that should be used to compute _currentval_.

## Potential Triggers: more details

The EventLoop class, which is responsible for carrying out the modeling process via its _ProcessEvents()_ method, has a member called _potential_triggers_ which is the only instance of the _model_res_miter_ class. This class, defined in header file res_multiterator.H, contains a good fraction of the overall complexity of the modeling process.

Like all Miterators, _potential_triggers_ has an _add_thread()_ method which is used to add a resource history to the list of "threads" tracked by _potential_triggers_. Here, we are using the word "thread" in the sense of Miterators, not in the C++ programming sense. We will use the expression "pthread" or "std::thread" to denote those. The terminology used to define the members of _potential_triggers_ consists of _define_ statements in the header file res_multiterator.H. They are listed in the table below.

name | definition | semantics
---- | ---------- | ---------
listClass | multilist<prio\_time, value\_node, Rsource*> | The class of which resource histories are instances
baseListClass | slist<prio\_time, value\_node, Rsource*> | Base class for the above
leadingClass | baseListClass::leading_iter | Safe history iterator; features iterNode, safeNode
Tthread | Cnode0<relptr<leadingClass>, leadingClass*> | A smart, time-based pointer to a safe history iterator. In short: "smart history pointer". The key picks up the time tag of the leadingClass payload.
Tthreadtlist | tlist<relptr<leadingClass>, Tthread> | Time-ordered list/b-tree of pointers to smart history pointers. This is the central object of the class; it provides a complete "cut" through resource histories, pointing at a "safe event" just at or before the current modeling time, and a "next event" in the near future.
Tthreadslist | slist<relptr<leadingClass>, Tthread> | Same as before, but not searchable. Used to hold "dead" smart history pointers, i. e., pointers which have a NULL next node.
threadptr | Cnode0<alpha_void, Tthread*> | -
threadptrslist | slist<alpha_void, threadptr> | Container pointing to smart history pointers.
threadptrtlist | tlist<alpha_void, threadptr> | Same but searchable.

Active maintenance of potential_triggers is provided by the event loop. The high-level calls that trigger maintenance are EventLoop::can_a_thread_be_unlocked(), which detects opportunities to wake a waiting thread, and advance_now_to(), which advances the iterators in potential_triggers and, when in the second phase of scheduling, deletes obsolete history nodes from the previous modeling run.

## Trailing miterators

Next we discuss the _trailing_miter_ class, which is used by std::threads responsible for constraing checking and (XML)TOL output. Basically, a trailing miterator object is similar to potential_triggers in that it keeps track of both future and recent history nodes, where the future node can be used to advance the history scanning process while the recent history node allows efficient evaluation of currentval() relative to a specific thread. However, unlike the modeling miterator, a trailing miterator must be careful not to step to far into history lists; it has to stay safely behind the notes in potential triggers. Here, 'safely' means that only stable nodes in potential triggers should be accessed. A stable node is a node that (i) will not be deleted while modeling and (ii) has a next\_node pointer that is not going to be modified by the modeling loop, so it can be read by the trailing thread without conflict.

The iterators in a _trailing_miter_ belong to the baseListClass::safe\_iter class, where baseListClass is the same as in potential\_triggers and safe\_iter is similar to curval\_iter but contains, in addition to iterNode and prevNode, a safe_node pointer that points to a future node that is the safeNode in a snapshot of potential_triggers. These snapshorts are collected in a _safe_vector_info_ object prepared every so often by the modeling thread and passed every 1,000 events to the trailing threads. This is a somewhat arbitrary value; it is set by the TIME ADVANCES BETWEEN TRAILING THREAD UPDATES macro in EventLoopUtil.C. Early on, the code was updating the data fed to trailing threads for every event in the loop, but that ended up being very inefficient - presumably because of the complexity of atomic variables used in inter-thread communications.

Let's discuss the logic that the trailing miterator must implement. The object that is central to that logic is the safe_vector_info object (referred to as "the snapshot" for short) that the trailing miterator last received from the modeling thread. The snapshot contains a time T and vector of pointers to leading\_iter objects taken from potential\_triggers at one point in time. Each leading iter has a next node (iterNode) that either is NULL or has a time tag > T, and a safe node (safeNode) wich a time tag <= T. The item which is watched _very carefully_ by the trailing miterator is the safe node in a generic leading iterator in the snapshot. The trailing miterator can access the payload of that iterator but not its next\_node pointer, which may be NULL, point to a node that will be deleted or be otherwise modified by the modeling thread as history nodes are created as a result of the modeling process. Therefore, all the trailing iterator can do is test whether the history node it is currently pointing to is the safeNode of a leading iterator in the snapshot. If it is not such a safeNode, then the node held by the trailing miterator must be a predecessor of the safeNode in the snapshot.

Thus, a step in the iteration of the trailing iterator can be described as follows. It has an active tlist of smart history pointers; each such pointer contains a safe\_iter node that can be safely iterated (i. e. its iterNode and prevNode can both move forward) until the iterNode becomes equal to the safeNode of the safe\_node iterator in the snapshot. That safeNode is available within the safe\_iter as its safe_node member; it is updated when a new safe vector is obtained from the modeling thread.
