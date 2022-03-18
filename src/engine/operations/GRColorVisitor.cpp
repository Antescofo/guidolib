//
//  GRColorVisitor.cpp
//  guidolib
//
//  Created by Arshia Cont on 21/07/2021.
//

#include <iostream>

#include "GRColorVisitor.h"

#include "GRSingleNote.h"
#include "GRSingleRest.h"
#include "GRBeam.h"
#include "GRTie.h"
#include "GRFingering.h"
#include "GRTuplet.h"
#include "GRSlur.h"
#include "GRArpeggio.h"
#include "GROctava.h"
#include "GRGlobalStem.h"

using namespace std;


void GRColorVisitor::visitStart(GRNotationElement *o) {
    if (o->getStaffNumber() == staffNumber) {
        if (!o->isPedal()) {
            o->setColor(params.c_str());
        }
    }
}

void GRColorVisitor::visitStart(GRSingleNote *o) {
    if (o->getStaffNumber() == staffNumber) {
        o->setColor(params.c_str());
        NEPointerList* assoc = o->getAssociations();
        if (assoc) {
            GuidoPos pos = assoc->GetHeadPosition();
            while(pos) {
                auto next = assoc->GetNext(pos);
                GRFingering * el = next->isGRFingering();
                if (el)
                    el->setColor(params.c_str());
                
                GRTuplet *tp = dynamic_cast<GRTuplet *>(next);
                if (tp) {
                    tp->GRTag::setColor(params.c_str());
                }
                
                GRGlobalStem *st = dynamic_cast<GRGlobalStem *>(next);
                if (st) {
                    st->setColor(params.c_str());
                }

                next->setColor(params.c_str());
            }
        }
    }
}

void GRColorVisitor::visitStart(GRArpeggio *o) {
    const NEPointerList * nelist = o->getAssociations();
    if (nelist) {
        GuidoPos pos = nelist->GetHeadPosition();
        while(pos) {
            auto next = nelist->GetNext(pos);
            if (next->getStaffNumber() == staffNumber) {
                o->GRTag::setColor(params.c_str());
                break;
            }
        }
    }
}

void GRColorVisitor::visitStart(GROctava *o) {
    if (o->getStaffNumber()==staffNumber) {
        o->GRTag::setColor(params.c_str());
    }
}

void GRColorVisitor::visitStart(GRSingleRest *o) {
    if (o->getStaffNumber() == staffNumber) {
        o->setColor(params.c_str());
    }
}

void GRColorVisitor::visitStart(GRBeam *o) {
    if (o->getStaffNumber() == staffNumber) {
        o->GRTag::setColor(params.c_str());
    }
}

void GRColorVisitor::visitStart(GRSlur *o) {
    if (o->getStaffNumber() == staffNumber) {
        o->setColor(params.c_str());
    }
}

void GRColorVisitor::visitStart(GRTie *o) {
    if (o->getStaffNumber() == staffNumber) {
        o->setColor(params.c_str());
    }
}
