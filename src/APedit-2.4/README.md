# APGenX Editor(s)
Documentation for the GTK-based GUI panels of APGenX:

- the Activity Editor

- the Globals Editor

- the XMLTOL export panel

## Generalities
The GTK panels are implemented in the object-oriented graphics toolkit gtkmm, which provides a set of object-oriented, C++-based wrappers for the GTK library used by the GNU Gnome project. The main APGenX window, however, is implemented in Motif, which is a completely separate toolkit. At the time when the activity editor was mandated by the MER project, the only reasonable way to link the two toolkits was to keep them in separate applications, and to fork one from the other when needed. More recently, it has become possible to have both toolkits coexist as separate pthreads within the same application (atm). The resulting design is somewhat complex, but it works.

## Code Layout
The interfaces between GTK-based GUI panels and the core engine are based on the original design of the APGenX activity editor, which was completed around 2003 for use by the MER project. At that time, I was trying to put into practice "separation of concerns," since it sounded like a good design idea. As a result, each GTK panel is implemented as summarized in the following table.

Panel | Header file | GTK source file | Interface source file(s)
----- | ----------- | --------------- | ------------------------
(all) | gtk\_bridge.H | (N/A) | atm\_client/gtk\_editor\_bridge.C
Activity Editor | apgen\_editor.H | APedit-2.4/gtk\_editwin.C | atm\_client/gtk\_editor\_bridge.C
Global Editor   | apgen\_globwin.H | APedit-2.4/gtk\_globwin.C | atm\_client/gtk\_globals\_bridge.C
XMLTOL Generator | apgen\_xmlwin.H | APedit-2.4/gtk\_xmlwin\_bridge.C | atm\_client/gtk\_xmlwin\_bridge.C

Header file gtk\_editor\_bridge.H plays a dual role; it contains the generic bridge data definitions as well as activity editor-specific code. This is due to the (long) history of this file; early on, the activity editor was the only GTK panel. In the future, this header may be split into two to reflect its two separate purposes. 

## Interface Design
The basic idea of the interface design is that each GTK panel deals with data coming from the APGenX engine. In all cases, the most important part of the data can be stored in a tree structure. We discuss this tree structure first.

### Tree Data Interface
In all three panels, there are three distinct data trees to keep in mind. The following description deals with all three.

The data stored in each one of those three trees consists of (label, value) pairs. The label is a string that uniquely identifies the node in the tree at that level in the tree; the value is either a TypedValue or a string instance, as discussed below. The three distinct data trees in each panel are the following:

1. Otree, the original tree structure containing the APGenX engine data. Because the engine does not store the data in a formal tree structure, Otree is a _virtual tree_, which must be built on the fly when exercising the interface.
2. Itree, a tree which parallels the structure of Otree but whose nodes are strings instead of the TypedValue objects present in Otree. In fact, Itree is a copy of Otree with each TypedValue object replaced by its serialization.
3. Dtree, the tree of strings as they appear in the GTK-based GUI panel.

The last one, Dtree, is the easiest one to describe because it parallels what is shown in the GUI exactly. Consider the Activity Editor, and focus on the Parameters section of the display. Consider a simple activity with two parameters A and B, whose values are an integer and a floating-point value, respectively. This is what the GUI display looks like:

Label | Value
----- | -----
Parameters | ""
A | "12"
B | "43.881"

In the table, we have listed the labels A and B under the Parameters label. In the GUI, however, the Parameters label appears in leftmost position, while A and B are indented to the right by a fixed amount. In fact, the user can choose whether to display the information under the Parameters label thanks to a little triangle that can be toggled; this triangle appears immediately to the left of the Parameters label. When initially displayed, the table shows Parameters with the triangle pointing to the right. When toggled, the triangle points to the bottom and the items below Parameters are displayed with a fixed indentation.

To capture the notion that labels such as A and B belong to a level that is lower than Parameters, GTK introduces the notion of path. In the above example, the path to Parameters is just Parameters, while the paths to A and B are Parameters:A and Parameters:B. More generally, if A and B themselves were trees containing hierarchies of values, the values under them would appear in paths such as Parameters:A:Foo, Parameters:A:Bar and Parameters:B:Foo. Note that the label "Foo" is not ambiguous; a label should only be unique within the smallest subtree to which it belongs.

### Multithreading Interface
Here we need to deal with the fact that the main APGenX window runs in Motif, while the global editor and other GTK panels run in the "GTK loop", which is completely separate and therefore runs in a separate thread. The solution is that whichever GTK panel is created first plays the role of a main window. Launching a panel takes two steps:

* creating a GTK object
* invoking the object's run() method

Creating multiple GTK objects is not a problem. The problem is that only one object's run method can be invoked. When a second panel is needed, the existing panel can invoke the new panel's show() method, which will bring it up.

There is a single display\_gtk\_window() method; it takes one argument, a gS::WinObj panel identifier. The very first time this method is invoked, it creates a new thread - the GTK thread. The GTK thread is provided with the panel identifier. It creates the required panel and invokes its run() method.

At that point, there are two possibilities:

1. the user closes the GTK panel just created, e. g. by clicking OK or Cancel. In this case, the GTK thread call to the first panel's run() method returns. But we do not let the GTK thread terminate, because of the possibility that another GTK panel might be requested by the user. Instead, the thread tries to unlock a lock that is held by the main APGenX process and will only be released when the main APGenX process either
  * terminates, and wants to join the GTK thread. In this case, the GTK thread returns.
  * requests a new (or the same) GTK panel. In this case, the GTK thread creates the requested panel if necessary or reuses the panel already created, then invokes its run() method.
2. the user wants to invoke a second GTK panel. In this case, the APGenX engine needs to get the attention of the GTK thread. This is done by attaching a timer to whichever GTK panel happens to be the "main" GTK panel, i. e., the oldest one among currently active panel(s). The timeout is short (0.1 second), and the timer is reactivated if no instructions are available; otherwise, the appropriate action is taken (e. g. a new GTK panel is displayed) and the timer resumes its job.

### Detailed Protocol
The following sequence of events describes the interaction between the main (engine) thread and the GTK thread.

1. Initially only the engine thread is running. The pointer to the GTK thread is NULL.
2. The very first time a GTK panel is requested, the engine
  * locks the wait-for-instructions mutex;
  * sets instructions-available to false;
  * creates the GTK thread and passes it the display\_gtk\_window() function along with the WinObj identifier of the desired window;
  * continues execution, which is managed by the Motif loop. This loop uses a 0.1 second timer to check periodically on various things, including GTK requests implemented via global booleans protected by the GTK mutex.
3. The newly created GTK thread creates the requested GTK panel and invokes its run() method. Three types of events are generated by the GTK thread:
  * requests to the engine originating from a GTK panel via a callback method, such as the editor "OK" button being pushed. The GTK callback locks the GTK mutex, sets one of the global Booleans monitored by the engine, and unlocks the mutex.
  * timeouts of the built-in timer owned by the GTK panel, which occur every 0.1 second. The GTK thread locks the GTK mutex, and checks the instruction-available global. If instructions are available, the GTK thread takes appropriate action (typically, launch a new GTK panel and display it via the new panel's show() method). Then, the GTK thread unlocks the mutex and rearms the timer.
  * the currently active GTK panel becomes hidden, as a result of the user clicking OK or Cancel. In that case, the following happens:
    * the run() method of the GTK panel, which was invoked by display\_gtk\_window(), returns.
    * the GTK thread locks the GTK mutex and checks the instructions-available global.
    * If instructions are available, they are executed exactly as in the case of a timeout, except that the new GTK panel's run() method needs to be invoked, instead of the show() method used when activated from a timeout.
    * If no instructions are available, the GTK thread sets gtk-process-idle global, unlocks the GTK mutex, and attempts to lock the wait-for-instructions mutex.
4. Whenever the engine needs the attention of the GTK thread, the following happens:
  * the engine locks the GTK mutex
  * the engine checks gtk-process-idle:
    * if the GTK thread is idle, the engine writes its instruction data, sets instructions-available to true and unlocks the GTK mutex. However, it needs to unlock the wait-for-instructions mutex, which is currently blocking the GTK thread.
