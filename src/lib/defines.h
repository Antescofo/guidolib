#ifndef GuidoDefines_H
#define GuidoDefines_H

/*
	GUIDO Library
	Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz
	Copyright (C) 2003  Grame

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <iosfwd>

#include "nvstring.h"
#include "Fraction.h"

// - Guido types (that should be moved elsewhere ?)
typedef Fraction TYPE_TIMEPOSITION;
typedef Fraction TYPE_DURATION;

// Implementation is in GUIDOEngine.cpp
void GuidoWarn( const char * inMessage, const char * inInfo = 0 );
void GuidoTrace( const char * inMessage );

#endif
