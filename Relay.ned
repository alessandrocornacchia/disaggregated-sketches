//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 1992-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

package sketches;

//
// Sends received packets to the output specified in the packet header.
//
simple Relay
{
    parameters:
        @display("i=block/switch");
	    int indexOffset = default(0);
	    
    gates:
        input in[];
        output out[];
}


