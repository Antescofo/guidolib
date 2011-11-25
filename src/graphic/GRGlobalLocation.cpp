/*
	GUIDO Library
	Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz

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

#include "ARShareLocation.h"
#include "GRGlobalLocation.h"
#include "GRStaff.h"
#include "GREmpty.h"
#include "GRSpring.h"
#include "GRStdNoteHead.h"
#include "GRGlobalStem.h"
#include "GRVoice.h"
// #include "NEPointerList.h"


GRGlobalLocation::GRGlobalLocation( GRStaff * grstaff, ARShareLocation * pshare )
: GRPTagARNotationElement(pshare)
{


	GRSystemStartEndStruct * sse = new GRSystemStartEndStruct;

	sse->grsystem = grstaff->getGRSystem();
	sse->startElement = NULL;
	sse->startflag = GRSystemStartEndStruct::LEFTMOST;
	sse->endElement = NULL;
	sse->p = NULL;

	mStartEndList.AddTail(sse);

	mFirstEl = NULL;

}

GRGlobalLocation::~GRGlobalLocation()
{
	// we just remove any association manually
	if (mAssociated)
	{
		GuidoPos pos = mAssociated->GetHeadPosition();
		while(pos)
		{
			GRNotationElement * el = mAssociated->GetNext(pos);
			if( el ) el->removeAssociation(this);
		}
	}
	if (mFirstEl)
	{
		mFirstEl->removeAssociation(this);
	}
}

void GRGlobalLocation::removeAssociation(GRNotationElement * grnot)
{
    if (grnot == mFirstEl)
	{
		GRNotationElement::removeAssociation(grnot);
		mFirstEl = NULL;
	}
	else
		GRPTagARNotationElement::removeAssociation(grnot);
}

void GRGlobalLocation::addAssociation(GRNotationElement * grnot)
{
	// grnot->getNeedsSpring() is zero, if an other
	// location-grabber has been already active.
	// (that is a \shareStem within a \shareLocation range)
	// then, the \shareStem-Tag is responsible for
	// setting the position. -> the first event 
	// of a shareStem is then associated with the
	// global shareLocation tag.
	if (grnot->getNeedsSpring() == 0) return;

	GREvent * ev = NULL;
	if ( (ev = GREvent::cast(grnot)) != NULL)
	{
		// take the length ...
		ev->setGlobalLocation(this);
	}

	// This sets the first element in the range ...
	if (!mFirstEl)
	{
		// this associates the first element with
		// this tag .... 
		mFirstEl = grnot;
		mFirstEl->addAssociation(this);

		return;

		// the firstElement is not added to the associated
		// ones -> it does not have to told anything!?
	}

	
	GRNotationElement::addAssociation(grnot);
	GRPositionTag::addAssociation(grnot);

	
	// this is needed, because the share location can
	// be used in different staves. Elements that need
	// be told of the location can be deleted before this
	// tag, therefor we must know, if these elements are
	// deleted. 
	// The recursive cycle with tellPosition is broken,
	// because only the first element really sets the
	// position. And the first is not included in the
	// own associated list.
	grnot->addAssociation(this);
}

void GRGlobalLocation::RangeEnd(GRStaff *grstaff)
{
	GRPTagARNotationElement::RangeEnd(grstaff);
}

void GRGlobalLocation::setHPosition( GCoord nx)
{
	// the first tells the element itself of its new position
	// and also the associated elements ... 
	GRPTagARNotationElement::setHPosition(nx + getOffset().x);
}

void GRGlobalLocation::OnDraw( VGDevice & hdc) const
{
}

void GRGlobalLocation::tellPosition( GObject * obj, const NVPoint &pt)
{
	if (dynamic_cast<GRNotationElement *>(obj) == mFirstEl) // useless cast ?
	{
		setHPosition(pt.x);
	}
}

int GRGlobalLocation::getHighestAndLowestNoteHead(GRStdNoteHead ** highest,
												  GRStdNoteHead ** lowest) const
{
	*highest = *lowest = 0;
	if (mFirstEl)
	{
		// find the GRGlobalStem
		const NEPointerList * plist = mFirstEl->getAssociations();
		if (plist)
		{
			GuidoPos pos = plist->GetHeadPosition();
			while (pos)
			{
				GRGlobalStem * gstem = dynamic_cast<GRGlobalStem *>(plist->GetNext(pos));
				if (gstem)
				{
					gstem->getHighestAndLowestNoteHead(highest,lowest);
					return gstem->getStemDir();
				}
			}
				
		}
	}

	// then we have not found it yet ....
	// check my own associations ....
	const NEPointerList * plist = mAssociated;
	if (plist)
	{
		GuidoPos pos = plist->GetHeadPosition();
		while (pos)
		{
			GRGlobalStem * gstem = dynamic_cast<GRGlobalStem *>(plist->GetNext(pos));
			if (gstem)
			{
				gstem->getHighestAndLowestNoteHead(highest,lowest);
				return gstem->getStemDir();
			}
		}
		
	}

	return 0;

}
