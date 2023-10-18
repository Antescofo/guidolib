/*
  GUIDO Library
  Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz
  Copyright (C) 2004 Grame

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

#include <iostream>
#include <algorithm>
#include <string.h>

#include "GRFingering.h"
#include "ARFingering.h"
#include "GRStaff.h"
#include "GRSingleNote.h"
#include "GUIDOInternal.h"	// for gGlobalSettings.gDevice
#include "TagParameterFloat.h"
#include "VGDevice.h"
#include "VGFont.h"

using namespace std;

extern GRStaff * gCurStaff;

GRFingering::GRFingering(GRStaff * staff, const ARText * ar) : GRText(staff, ar)
{
	mMustFollowPitch = true;
}

// -----------------------------------------------------------------------------
void GRFingering::accept (GRVisitor& visitor)
{
	visitor.visitStart (this);
	visitor.visitEnd (this);
}

// -----------------------------------------------------------------------------
const ARFingering * GRFingering::getARFingering() const
{
	return static_cast<const ARFingering*>(getAbstractRepresentation());
}

// -----------------------------------------------------------------------------
void GRFingering::OnDraw( VGDevice & hdc ) const
{
	if(!mDraw || !mShow) return;
	const ARFingering * fing = getARFingering();
	if( fing == 0 ) return;

	unsigned int fontalign;
	const VGColor textColor = startDraw(hdc, fontalign);
	const VGFont* font = hdc.GetTextFont();
	float offset = LSPACE * (font->GetSize() / 52.f);

	size_t flen = fing->countFingerings();
	const vector<string> & list = fing->fingerings();
	float ypos = mPosition.y + getOffset().y;
    float xpos = mPosition.x + getOffset().x;
    
	if ((fing->getFingeringPosition() != ARFingering::kAbove) && (getOffset().y >= 0.0))
		ypos += offset * (fing->countFingerings() - 1);
    
	for (size_t i = 0; i<flen; i++) {
	    hdc.DrawString( xpos, ypos, list[i].c_str(), (int)list[i].size());
	    ypos -= offset;
	}

	endDraw (hdc, textColor, fontalign);
}

// -----------------------------------------------------------------------------
void GRFingering::tellPosition(GObject * caller, const NVPoint & inPosition)
{
}

// -----------------------------------------------------------------------------
void GRFingering::tellPositionEnd(GRSingleNote * note, const NVPoint & inPosition)
{
	if( note == 0 ) {
		mDraw = false;
		return;
	}

	const GRStaff * staff = note->getGRStaff();
	if( staff == 0 ) return;

	const ARFingering * fing = getARFingering();
	if( fing == 0 ) return;

	mStaffBottom = staff->getStaffBottom();
	GRSystemStartEndStruct * sse = getSystemStartEndStruct(staff->getGRSystem());
	assert(sse);

	GRTextSaveStruct * st = (GRTextSaveStruct *)sse->p;
	GRNotationElement * startElement = sse->startElement;
	NVPoint newPos( inPosition );

	if (note == startElement)
	{
		float  hspace = staff->getStaffLSPACE() / 2;
		newPos.y = note->getPosition().y;
		st->position = newPos;

		const char* text = fing->getText();
		if (fing->countFingerings() > 1) st->text = text = fing->fingerings()[0].c_str();
		else if (text) st->text = text;
		int placement = fing->getFingeringPosition ();

		FloatRect r = getTextMetrics (*gGlobalSettings.gDevice, staff);
        r.ShiftY(-1.0*getOffset().y); // remove dy offset. it'll be handled during Draw
		NVRect ebb = note->getEnclosingBox(false, false, true);
		float xpos = note->getPosition().x - r.Width()/2;
        
        // AC: take into account multiple fingerings
        float height = r.Height() * (fing->countFingerings());
        
        NVRect bb (0, 0, r.Width(), height);
        mBoundingBox = bb;

		switch (placement) {
			// here positionning takes account of the implicit dy=1 inherited from ARText (see TagParameterStrings.cpp)
			case ARFingering::kAbove:
				setPosition (NVPoint(xpos, min(ebb.top, 0.f) - hspace - r.Height()));
				break;
			case ARFingering::kBelow:
				setPosition (NVPoint(xpos, max(ebb.bottom, staff->getDredgeSize())));
				break;
			default:
                float ypos = r.top;
                ypos -= (r.Height()/2.0 + hspace) ;
                if (getOffset().y > 0) { // downwards
                    ypos += hspace;
                }
                
                setPosition (NVPoint(xpos, ypos));
		}
	}
}
