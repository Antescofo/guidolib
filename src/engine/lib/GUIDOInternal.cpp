/*
  GUIDO Library
  Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz
  Copyright (C) 2003, 2004  Grame


  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

/** \file GUIDOInternal.cpp

	Internal Guido functions.

*/

#include <iostream>
#include <sstream>

#include "defines.h"		// for GuidoWarn and GuidoTrace
#include "gddefs.h"			
#include "ARPageFormat.h"
#include "GUIDOInternal.h"

#include "GRStaffManager.h"
#include "FontManager.h"

using namespace std;

extern bool gInited;			// GuidoInit() Flag
extern ARPageFormat gARPageFormat;

// --------------------------------------------------------------------------
// Prototypes are in "defines.h"
void GuidoWarn( const char * inMessage, const char * info )	
{
//	cerr << "Guido Warning: " << inMessage << " " << (info ? info : "") << endl;
}

// --------------------------------------------------------------------------
void GuidoTrace( const char * inMessage )	
{
//	cerr << "Guido Trace: " << inMessage <<endl;
}

// --------------------------------------------------------------------------
void guido_deinit()
{
	FontManager::ReleaseAllFonts();
	gInited = false;
}

// --------------------------------------------------------------------------
void guido_cleanup()
{
}

// --------------------------------------------------------------------------
/** \brief Adds an ARMusic (abstract representation) object to the global list,
	then returns the corresponding guido AR handler.

	The reference counter of the AR is still sets to zero at this point, to 
	maintain the behaviour of previous version of the library.
		
	result > 0: guido AR handle
	result < 0: error code
			
*/
ARHandler guido_RegisterARMusic( ARMusic * inMusic )
{
	if( inMusic == 0 ) return 0;
	
	// - Create and setup a new NodeARMusic node
	NodeAR * newNode = new NodeAR;
	newNode->refCount = 1;
	newNode->armusic = inMusic;
	
	return newNode; // returns the brand new guido AR handle.
}

// --------------------------------------------------------------------------
/** \brief Adds an GRMusic (graphical representation) object to the global list,
	then returns the corresponding guido handle.

	The reference counter of the AR object is left untouched.

	\return a Guido opaque handle to the newly resistered GR structure.
			
*/
GRHandler guido_RegisterGRMusic( GRMusic * inMusic, ARHandler inHandleAR )
{
	if( inMusic == 0 ) return 0;
	if( inHandleAR == 0 ) return 0;
	
	// - Create and setup a new NodeGr
	NodeGR * newNode = new NodeGR;
	if (newNode == 0) 
		return 0;

	newNode->page = 1;	// page is obsolete. Just here for compatibility.
	newNode->grmusic = inMusic;
	newNode->arHandle = inHandleAR;
	return newNode;  // returns the brand new guido handle.
}

//} // extern "C" 
