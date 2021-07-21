//
//  GRColorVisitor.h
//  guidolib
//
//  Created by Arshia Cont on 21/07/2021.
//

#ifndef GRColorVisitor_h
#define GRColorVisitor_h

#include <iostream>
#include "GUIDOEngine.h"
#include "GRVisitor.h"

class GRColorVisitor : public GRVisitor
{
    int      staffNumber;
    std::string params;

    public:
    GRColorVisitor(std::string params, int staffNum) : staffNumber(staffNum), params(params) {}
    virtual ~GRColorVisitor() {}
    
    virtual bool voiceMode () { return false; }
    
    virtual void visitStart (GRNotationElement* o);
    virtual void visitStart (GRSingleNote* o);
    virtual void visitStart (GRSingleRest* o);
    virtual void visitStart (GRBeam* o);
    virtual void visitStart (GRSlur* o);
    virtual void visitStart (GRTie* o);
};

#endif /* GRColorVisitor_h */
