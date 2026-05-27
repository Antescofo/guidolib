/*
  GUIDO Library
  Copyright (C) 2016 Grame

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

#ifndef __TCollisions__
#define __TCollisions__

#include <vector>
#include <map>
#include <iostream>

#include "NVRect.h"
#include "NVPoint.h"
#include "ARSpace.h"
#include "GUIDOTypes.h"
#include "defines.h"

class ARMusicalObject;
class GRNotationElement;
class GRStaff;
class GRSystemSlice;

// ----------------------------------------------------------------------------
// TCollisionInfo provides the information required to solve a collision ar AR level
class  TCollisionInfo {
	public:
		TCollisionInfo (const ARMusicalObject* ar, int voice, ARSpace* space)
			: fSpace(space), fARObject(ar), fVoice(voice),
			  fAllowSameDatePositionInsert(false), fUseTimeBearingPositionInsert(false) {}
		TCollisionInfo (const ARMusicalObject* ar, int voice, ARSpace* space, bool allowSameDatePositionInsert,
						bool useTimeBearingPositionInsert)
			: fSpace(space), fARObject(ar), fVoice(voice),
			  fAllowSameDatePositionInsert(allowSameDatePositionInsert),
			  fUseTimeBearingPositionInsert(useTimeBearingPositionInsert) {}
	
		void print(std::ostream& os) const;

		TYPE_TIMEPOSITION	date() const	{ return fARObject->getRelativeTimePosition(); }
		float				space() const	{ return fSpace->getValue(); }

		ARSpace*		 fSpace;		// a space element intended to solve the collision
		const ARMusicalObject* fARObject;	// the ar object after which the space should be inserted
		int				 fVoice;			// the corresponding ARVoice number

		// Position tags are resolved by date because their AR object may not be
		// in the voice event list. These flags keep the generic collision path
		// compatible with lyrics while allowing harmony-specific spacing anchors.
		bool			 fAllowSameDatePositionInsert;	// true when the target can be the event at the tag date
		bool			 fUseTimeBearingPositionInsert;	// true when rests may be the spacing anchor, as for harmonies
};

std::ostream& operator<< (std::ostream& os, const TCollisionInfo* ci);
std::ostream& operator<< (std::ostream& os, const TCollisionInfo& ci);

// ----------------------------------------------------------------------------
class  TCollisions {
	public:
				 TCollisions ();
				 TCollisions (GuidoPos lastpos);
		virtual ~TCollisions () {}

		void	setStaff (int num);
		int		getStaff() const							{ return fStaff; }
		void	setSystem (int num);
		int		getSystem() const							{ return fSystem; }
		bool	collides () const							{ return !fCollisions.empty(); }
		size_t	count () const								{ return fCollisions.size(); }
		const std::vector<TCollisionInfo>& list () const	{ return fCollisions; }

		const GRNotationElement * lastElement()				{ return fLastElements[fStaff]; }
		const NVRect&			  lastBB()					{ return fLastBB[fStaff]; }
		const NVPoint			  yOffset() const;

		bool	check (const NVRect& r, bool slice=false);
		void	update (const GRNotationElement * e, const NVRect& r);
		void	update (const GRSystemSlice * slice, const NVRect& r);
		void	reset (bool resetSystem);
		void	clearElements ();
		void	clear ();
		void	print (std::ostream& out) const;

		void	resolve (const ARMusicalObject* ar, float gap, bool allowSameDatePositionInsert=false,
						 bool useTimeBearingPositionInsert=false);

	private:
		bool	checkElement (const NVRect& r);
		bool	checkSlice (const NVRect& r);

		std::map<int, const GRNotationElement*> fLastElements;	// the previous element associated to a staff
		std::map<int, NVRect> 	fLastBB;		// the previous elements bounding box
		const GRSystemSlice*	fLastSlice;		// the last system slice
		NVRect					fLastSliceBB;	// the previous system slice bounding box
		int						fStaff;			// the current staff number
		int						fSystem;		// the current system number
		std::vector<TCollisionInfo>		fCollisions;	// a list of space elements intended to resolve the collisions
};

std::ostream& operator<< (std::ostream& os, const TCollisions& c);


#endif
