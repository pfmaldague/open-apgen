/*
Classes and methods to layout activities without overlapping (if possible) in a generic space
Author: Allan H. Wax, Oct/2002
*/

#ifndef IO_ACTIVITYLAYOUT_H
#define IO_ACTIVITYLAYOUT_H
// IO_activitylayout start

#include <vector>

using namespace std;
class ActivityInstance;

class ActivityLayoutInfo {
private:
    ActivityInstance* activity;
    double x, y, width, height;
public:
    ActivityLayoutInfo() {
	    activity = NULL;
    }

    ActivityLayoutInfo(double xpos, double ypos, double w, double h, ActivityInstance* act) {
	x = xpos;
	y = ypos;
	width = w;
	height = h;
	activity = act;
    }

    ActivityLayoutInfo (const ActivityLayoutInfo& rhs) {	// copy constructor
	x = rhs.getX();
	y = rhs.getY();
	width = rhs.getWidth();
	height = rhs.getHeight();
	activity = rhs.getActivity();
    }

    ~ActivityLayoutInfo() {}

    double getX() const {return x;}
    void setX(double xpos) {x = xpos;}

    double getY() const {return y;}
    void setY(double ypos) {y = ypos;}

    double getWidth() const {return width;}
    void setWidth(double w) {width = w;}

    double getHeight() const {return height;}
    void setHeight(double h) {height = h;}

    ActivityInstance* getActivity() const {return activity;}
    void setActivity(ActivityInstance* act) {activity = act;}

    // operators

    ActivityLayoutInfo& operator= (const ActivityLayoutInfo& rhs) {
	x = rhs.getX();
	y = rhs.getY();
	width = rhs.getWidth();
	height = rhs.getHeight();
	activity = rhs.getActivity();

	return *this;
    }

};

class ActivityEnumerator {
public:
    virtual void action(ActivityLayoutInfo& activityLayoutInfo) = 0;	// do something
};

class ActivityData {
    // Information about each rectangle to be placed
private:
    double start;	// start (time) of this object
    double end;		// end (time) of this object
    double height;	// height in units of this rectangle

public:
    ActivityData(double begin, double finish, double vertical) {
	start = begin;
	end = finish;
	height = vertical;
    }

    double getStart() const {return start;}
    // double getEnd const () {return end;}
    double getEnd() const {return end;}
    double getHeight() const {return height;}
};

typedef vector<ActivityData*> ActivityDataList;	// a list of associated data

class ActivityInfo {
    // This class is passed in by the caller to provide a list of regions composed of subregions
private:
    ActivityDataList* activityDataList;
    bool autoDelete;
public:
    ActivityInstance* activity;	// the user object associated with the ActivityDataList
public:
    ActivityInfo() : activityDataList(new ActivityDataList), autoDelete(true) {}
    ~ActivityInfo() {
	if (autoDelete) {
	    for (int i = activityDataList->size() - 1; i >= 0; i--)
		delete (*activityDataList)[i];
	}
	delete activityDataList;
    }

    ActivityDataList& getActivityDataList() const { return *activityDataList;}

    // setAutoDelete to true if the destructor should also delete stuff added to the list when the list goes away.
    void setAutoDelete(bool autoDel) {autoDelete = autoDel;}
};


typedef vector<ActivityInfo*> ActivityInfoList;		// list of 

typedef vector<ActivityLayoutInfo*> ActivityOverlayList;// for use by ActivityLayout
typedef vector<ActivityOverlayList*> ActivityOverlays;	// for use by ActivityLayout

class ActivityLayout {
private:
    ActivityOverlays overlays;
    double height;
public:
    ActivityLayout() : overlays() {};
    ActivityLayout(ActivityInfoList& activities,// a list of adjacent composite rectangles to place on the page
		   double regionHeight		// height of writable region in units
		   );
    
    ~ActivityLayout();

    double getRegionHeight() const { return height; }

    void enumerate(ActivityEnumerator& enumerator) {
	// for each activity in the list, call the action part of the enumerator
	for (int i = 0; i < overlays.size(); i++) {
	    ActivityOverlayList& regions = *overlays[i];
	    for (int j = 0; j < regions.size(); j++) {
		enumerator.action(*regions[j]);
	    }
	}
    }

    // returns true if any region overlaps(overlays) another region
    bool containsOverlap() {return overlays.size() > 1;}
};

#endif // end IO_activitylayout
