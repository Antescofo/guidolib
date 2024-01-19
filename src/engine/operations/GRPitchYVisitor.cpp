/*
  GUIDO Library
  Copyright (C) 2023 D. Fober

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <iostream>

#include "GRPitchYVisitor.h"

#include "GuidoDefs.h"
#include "GRBar.h"
#include "GRClef.h"
#include "GREmpty.h"
#include "GRKey.h"
#include "GRMeter.h"
#include "GRMusic.h"
#include "GROctava.h"
#include "GREvent.h"
#include "GRRepeatBegin.h"
#include "GRSingleNote.h"
#include "GRSingleRest.h"
#include "GRStaff.h"

using namespace std;

//-------------------------------------------------------------------------------
NVPoint GRPitchYVisitor::getPitchPos (GRMusic* music, int staffNum, int midipitch, TYPE_TIMEPOSITION date)
{
	fTargetStaff = staffNum;
	fCurrentStaff = 0;
	fBasePitch 	= NOTE_G;
	fBaseLine 	= 3;
	fBaseOct 	= 1;
	fOctava 	= 0;
	fTargetDate = date;
	fNextX = 0.f;
	fDone = false;
	fStaff = nullptr;
	fTargetElt = nullptr;
    fNumKeys = 0;
	music->accept (*this);
	NVPoint p;
	if (fDone && fStaff && fTargetElt) {
		midipitch -= (12 * fOctava);
		// convert midi pitch in pitch class and octava
		int oct = (midipitch / 12) - 4;
        int pitch = midiToGuidoPitch(midipitch, fNumKeys);
        // calculate position
		NVPoint spos = fStaff->getPosition();
		float y = fStaff->getNotePosition ( pitch, oct, fBasePitch, fBaseLine, fBaseOct);
		p.x = interpolateXPos(fTargetElt, fTargetDate, fNextX, fNextDate);
		p.y = (spos.y + y);
	}
	return p;
}

int GRPitchYVisitor::midiToGuidoPitch(int midipitch, int numKeys) {
    int pitch = midipitch % 12;
    int guidoPitch;
    switch (pitch) {
        case 0: guidoPitch = NOTE_C; break;
        case 1: guidoPitch = NOTE_CIS; break;
        case 2: guidoPitch = NOTE_D; break;
        case 3: guidoPitch = NOTE_DIS; break;
        case 4: guidoPitch = NOTE_E; break;
        case 5: guidoPitch = NOTE_F; break;
        case 6: guidoPitch = NOTE_FIS; break;
        case 7: guidoPitch = NOTE_G; break;
        case 8: guidoPitch = NOTE_GIS; break;
        case 9: guidoPitch = NOTE_A; break;
        case 10: guidoPitch = NOTE_AIS; break;
        case 11: guidoPitch = NOTE_H; break;
        default: return  EMPTY;
    }
    // Key Consideration: if fNumKey is 0 or positive (sharps) nothing to do! If negative, we need to adjust
    if (numKeys >= 0) {
        return guidoPitch;
    } else {
        // if numkeys is negative we have flats beginning at quint[6-j]=B
        // (B,Es,As,Des,Ges)
        // The allFlats array below orders flats (in their sharp names)! Just truncate using numKeys!
        int allFlats[] = { NOTE_AIS, NOTE_DIS, NOTE_GIS, NOTE_CIS, NOTE_FIS };
        int flatNums = abs(numKeys);
        int myFlats[flatNums];
        memcpy(myFlats, &allFlats[0], flatNums*sizeof(*allFlats));
        // if guidoPitch is contained in flat array, then the "next note" should be sent to getNotePosition!
        int found = -1;
        for (int i = 0; i < flatNums; i++) {
            if (myFlats[i] == guidoPitch) {
                found = i;
            }
        }
        if (found != -1) {
            switch (guidoPitch) {
                case NOTE_AIS: return NOTE_H;
                case NOTE_DIS: return NOTE_E;
                case NOTE_GIS: return NOTE_A;
                case NOTE_CIS: return NOTE_D;
                case NOTE_FIS: return NOTE_G;
                default: return guidoPitch;
            }
        } else {
            return guidoPitch;
        }
    }
}

//-------------------------------------------------------------------------------
float GRPitchYVisitor::interpolateXPos (const GRNotationElement* elt, TYPE_TIMEPOSITION target, float nextx, TYPE_TIMEPOSITION nextDate) const
{
    TYPE_TIMEPOSITION currentEltTime = elt->getRelativeTimePosition();
	TYPE_TIMEPOSITION offset = target - currentEltTime;
    float segmentDuration;
    // To calculate segment: if nextDate is on a note we are > than currentEltTime. If it's less or equal then we might be on a Bar or similar and we use duration (case of notes at the end of measure)
    if (nextDate <= currentEltTime) {
        segmentDuration = float(elt->getDuration());
    } else {
        segmentDuration = float(nextDate - currentEltTime);
    }
	float ratio = float(offset) / segmentDuration;
	float x = elt->getPosition().x;
//    cerr<<"\t<<< ui guidog x="<<x<<" nextx="<<nextx;
//    cerr<<" dates: "<<double(currentEltTime)<<"-"<<double(target)<<"-"<<double(nextDate);
//    cerr<<" segmentDuration="<<segmentDuration;
//    cerr<<endl;
	return x + (nextx - x) * ratio;
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRStaff* o)
{
	if (fDone) return;
	fCurrentStaff = o->getStaffNumber();
	fStaff = o;
}

//-------------------------------------------------------------------------------
bool GRPitchYVisitor::checkTimePos (const GRNotationElement* elt)
{
	TYPE_TIMEPOSITION date = elt->getRelativeTimePosition();
	TYPE_TIMEPOSITION endDate = date + elt->getDuration();
	if ((fTargetDate >= date) && (fTargetDate < endDate)) {
		fTargetElt = elt;
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::check (const GRNotationElement* o)
{
	if (fCurrentStaff != fTargetStaff) return;
	if (fDone) {
        checkNextElement(o);
	}
	else fDone = checkTimePos(o);
}

void GRPitchYVisitor::checkNextElement(const GRNotationElement* elt)
{
    if (fCurrentStaff != fTargetStaff) return;
    if (fDone && !fNextX) {
        // fNextX MUST BE greater than fTargetElt's x-pos!
        float currentX = fTargetElt->getPosition().x;
        TYPE_TIMEPOSITION runningDate = elt->getRelativeTimePosition();
        float runningX = elt->getPosition().x;
        if ((runningX > currentX)) { //&& (runningDate > fTargetDate)
            fNextX = elt->getPosition().x;
            fNextDate = elt->getRelativeTimePosition();
        }
    }
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRBar* o)
{
	if (fCurrentStaff != fTargetStaff) return;
	if (fDone && !fNextX) fNextX = o->getPosition().x;
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRMeter* o)
{
	if (fCurrentStaff != fTargetStaff) return;
	if (fDone && !fNextX) fNextX = o->getPosition().x;
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRKey* o)
{
	if (fCurrentStaff != fTargetStaff) return;
    if (fDone) {
        if (!fNextX) fNextX = o->getPosition().x;
        return;
    }
    int mynumkeys = NUMNOTES;
    float mymkarray [ NUMNOTES ];
    fNumKeys = o->getKeyArray(mymkarray);
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRRepeatBegin* o)
{
	if (fCurrentStaff != fTargetStaff) return;
	if (fDone && !fNextX) fNextX = o->getPosition().x;
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRSingleNote* o) 	{ check(o); }
void GRPitchYVisitor::visitStart (GREmpty* o) 		{ check(o); }
void GRPitchYVisitor::visitStart (GRSingleRest* o) 	{ check(o); }

void GRPitchYVisitor::visitStart (GRClef* o)
{
	if (fCurrentStaff != fTargetStaff) return;
	if (fDone) {
		if (!fNextX) fNextX = o->getPosition().x;
		return;
	}
	fBasePitch = o->getBasePitch();
	fBaseOct = o->getBaseOct();
	fBaseLine = o->getBaseLine();
}

void GRPitchYVisitor::visitStart (GROctava* o)
{
	if (fDone || (fCurrentStaff != fTargetStaff)) return;
	fOctava = o->getOctava();
}
