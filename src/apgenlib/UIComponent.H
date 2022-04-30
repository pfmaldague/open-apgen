///////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//         This example code is from the book:
//
//           Object-Oriented Programming with C++ and OSF/Motif
//         by
//           Douglas Young
//           Prentice Hall, 1992
//           ISBN 0-13-630252-1	
//
//         Copyright 1991 by Prentice Hall
//         All Rights Reserved
//
//  Permission to use, copy, modify, and distribute this software for 
//  any purpose except publication and without fee is hereby granted, provided 
//  that the above copyright notice appear in all copies of the software.
///////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////
// UIComponent.h: Base class for all C++/Motif UI components
///////////////////////////////////////////////////////////////
#ifndef UICOMPONENT_H
#define UICOMPONENT_H

#ifdef GUI
#include <Xm/Xm.h>
#else
#include "UI_GeneralMotifInclude.H"
#include "UI_MotifPreview.H"
///
typedef char* String ;
///
typedef void* XtResourceList ;
#include <stdio.h>
#endif

//
// If this is not set, bx and cgx assume an old base class. App generation
// 	may not work at runtime.
//
#define cxx_bx_compat

/// This structure is for capturing app-defaults values for a Class

typedef struct _UIAppDefault
{
    const char*		wName;		// Widget name
    const char*		cInstName;	// Name of class instance (nested class)
    const char*		wRsc;		// Widget resource
    char*		value;		// value read from app-defaults
} ///
 UIAppDefault;

/// UIComponent Class
class UIComponent {
    
  private:
    
    /// Interface between XmNdestroyCallback and this class
    
    static void widgetDestroyedCallback ( Widget, 
					  XtPointer, 
					  XtPointer );
    
  protected:

    ///
    char    *_name;
    ///
    Widget   _w;    
    
    /// Protect constructor to prevent direct instantiation
    
    UIComponent ( const char * );
    
    ///Easy hook for derived classes
    void installDestroyHandler(); 
    
    /// Called by widgetDestroyedCallback() if base widget is destroyed
    
    virtual void widgetDestroyed(); 
    
    /// Loads component's default resources into database
    
    void setDefaultResources ( const Widget , const String *);
    
    /// Retrieve resources for this clsss from the resource manager
    
    void getResources ( const XtResourceList, const int );

    /// Initialize the app defaults structure for a class

    void initAppDefaults ( const Widget, const char *, UIAppDefault * );

    /// Set the app defaults for an instance of a class

    void setAppDefaults ( const Widget, UIAppDefault *, const char* inst_name = NULL);
   
  public:
    
    ///
    virtual ~UIComponent();  // Destructor
    
    // Manage the entire widget subtree represented
    /// by this component. Overrides BasicComponent method
    
    virtual void manage();   /// Manage and unmanage widget tree
    virtual void unmanage();

    ///
    const Widget baseWidget() { return _w; }
    
    /// Public access functions
    
    virtual const char *const className();
};

///
typedef struct _UICbStruct
{
    UIComponent *object;             // this pointer
    XtPointer   client_data;         // client data 
} ///
 UICallbackStruct;

#endif
