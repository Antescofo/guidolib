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
    fNextDate = TYPE_TIMEPOSITION(0, 1);
	fDone = false;
	fSearchingNext = false;
	fStaff = nullptr;
	fTargetElt = nullptr;
    fNumKeys = 0;
	music->accept (*this);
	NVPoint p;
	if (fDone && fStaff && fTargetElt) {
        const int targetBasePitch = fBasePitch;
        const int targetBaseLine = fBaseLine;
        const int targetBaseOct = fBaseOct;
        const int targetOctava = fOctava;
        const int targetNumKeys = fNumKeys;
        const GRStaff* targetStaff = fStaff;

        // Resolve the interpolation boundary in a second pass so the next
        // element is chosen by chronological order across the whole staff,
        // independently of the traversal order.
        //
        // Rationale: the visitor order is not guaranteed to match the score
        // chronology for a rendered staff. Using "first later element whose X
        // is greater than the target X" can therefore skip the true next
        // boundary on layouts where X is not monotonic with time and latch onto
        // a later note farther to the right.
        fSearchingNext = true;
        fCurrentStaff = 0;
        fNextX = 0.f;
        fNextDate = TYPE_TIMEPOSITION(0, 1);
        music->accept (*this);
        fSearchingNext = false;

		midipitch -= (12 * targetOctava);
		// convert midi pitch in pitch class and octava
		int oct = (midipitch / 12) - 4;
        int pitch = midiToGuidoPitch(midipitch, targetNumKeys);
        // calculate position
		NVPoint spos = targetStaff->getPosition();
		float y = targetStaff->getNotePosition ( pitch, oct, targetBasePitch, targetBaseLine, targetBaseOct);
		p.x = fNextX ? interpolateXPos(fTargetElt, fTargetDate, fNextX, fNextDate) : fTargetElt->getPosition().x;
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
	fCurrentStaff = o->getStaffNumber();
    if (!fSearchingNext && !fDone) fStaff = o;
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
    if (fSearchingNext) {
        considerNextCandidate(o);
    }
	else if (!fDone) {
        fDone = checkTimePos(o);
    }
}

void GRPitchYVisitor::considerNextCandidate(const GRNotationElement* elt)
{
    if ((fCurrentStaff != fTargetStaff) || !fTargetElt || (elt == fTargetElt)) return;

    TYPE_TIMEPOSITION runningDate = elt->getRelativeTimePosition();
    TYPE_TIMEPOSITION targetEndDate = fTargetElt->getRelativeEndTimePosition();
    if (runningDate < targetEndDate) return;

    float runningX = elt->getPosition().x;
    if (!fNextX || (runningDate < fNextDate)) {
        fNextX = runningX;
        fNextDate = runningDate;
        return;
    }
    if (runningDate > fNextDate) return;

    // For elements on the same date, prefer candidates that stay on the right
    // of the current element, then pick the closest one in X. This keeps the
    // interpolation tied to the nearest chronological boundary while still
    // behaving sensibly on line breaks and measure changes.
    float currentX = fTargetElt->getPosition().x;
    bool candidateForward = (runningX >= currentX);
    bool bestForward = (fNextX >= currentX);

    if (candidateForward && !bestForward) {
        fNextX = runningX;
        fNextDate = runningDate;
    }
    else if (candidateForward == bestForward) {
        bool betterX = candidateForward ? (runningX < fNextX) : (runningX > fNextX);
        if (betterX) {
            fNextX = runningX;
            fNextDate = runningDate;
        }
    }
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRBar* o)
{
	if (fCurrentStaff != fTargetStaff) return;
    if (fSearchingNext) considerNextCandidate(o);
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRMeter* o)
{
	if (fCurrentStaff != fTargetStaff) return;
    if (fSearchingNext) considerNextCandidate(o);
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRKey* o)
{
	if (fCurrentStaff != fTargetStaff) return;
    if (fSearchingNext) {
        considerNextCandidate(o);
        return;
    }
    if (fDone) return;
    int mynumkeys = NUMNOTES;
    float mymkarray [ NUMNOTES ];
    fNumKeys = o->getKeyArray(mymkarray);
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRRepeatBegin* o)
{
	if (fCurrentStaff != fTargetStaff) return;
    if (fSearchingNext) considerNextCandidate(o);
}

//-------------------------------------------------------------------------------
void GRPitchYVisitor::visitStart (GRSingleNote* o) 	{ check(o); }
void GRPitchYVisitor::visitStart (GREmpty* o) 		{ check(o); }
void GRPitchYVisitor::visitStart (GRSingleRest* o) 	{ check(o); }

void GRPitchYVisitor::visitStart (GRClef* o)
{
	if (fCurrentStaff != fTargetStaff) return;
    if (fSearchingNext) {
        considerNextCandidate(o);
		return;
	}
    if (fDone) return;
	fBasePitch = o->getBasePitch();
	fBaseOct = o->getBaseOct();
	fBaseLine = o->getBaseLine();
}

void GRPitchYVisitor::visitStart (GROctava* o)
{
	if (fSearchingNext || fDone || (fCurrentStaff != fTargetStaff)) return;
	fOctava = o->getOctava();
}
