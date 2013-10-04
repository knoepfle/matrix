/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef MATRIXPATHROUTING_H_
#define MATRIXPATHROUTING_H_

#include "../Server/DataStructures/QueryObjectsStorage.h"
#define TWO_HEAP 1

// the first error encountered will be returned
// from location not found
#define ERR_FROM -1	
// to location not found
#define ERR_TO   -2
// no route from from to to made INT_MAX to ShortestPathRouting
#define ERR_LINK INT_MAX

template<class QueryDataT>
class MatrixRouting : public BasicRoutingInterface<QueryDataT>{
    typedef BasicRoutingInterface<QueryDataT> super;
    typedef typename QueryDataT::QueryHeap QueryHeap;
public:
    MatrixRouting( QueryDataT & qd) : super(qd) {}

    ~MatrixRouting() {}

    void operator()(std::vector<PhantomNode> fromNodesVector, std::vector<PhantomNode> toNodesVector, RawRouteData *rr) const {
		int rows = fromNodesVector.size();
		int cols = toNodesVector.size();
		int bound=INT_MAX;
		NodeID *middle = new NodeID[rows*cols];

		QueryHeap *fh = new QueryHeap(super::_queryData.nodeHelpDesk->getNumberOfNodes());
		QueryHeap **rh = new QueryHeap*[cols];

		QueryHeap *emptyheap = new QueryHeap(super::_queryData.nodeHelpDesk->getNumberOfNodes());
		QueryHeap & eheap = *emptyheap;

		RawRouteData *md1 = new RawRouteData[rows*cols];
		memset(md1,0,rows*cols*sizeof(RawRouteData));
		int i;

		for (i=0; i< rows*cols; i++) { 
			middle[i] = INT_MAX;
			md1[i].lengthOfShortestPath=ERR_FROM;
		} 

		int x=0;
		int y=0;

		// set all to nodes on all to (reverse) heaps
       	BOOST_FOREACH(const PhantomNode & targetPhantom, toNodesVector) {
			if (targetPhantom.edgeBasedNode == UINT_MAX) {
				rh[x++] = NULL;
				continue;
			} else
        		rh[x]= new QueryHeap(super::_queryData.nodeHelpDesk->getNumberOfNodes());
			QueryHeap & reverse_heap = *rh[x];
            reverse_heap.Clear();	

            reverse_heap.Insert(targetPhantom.edgeBasedNode, targetPhantom.weight1, targetPhantom.edgeBasedNode);
			x++;
		} 

		// loop over from and to heaps, keep to heaps but delete
		// from after each outer loop
		y=0;
        BOOST_FOREACH(const PhantomNode & startPhantom, fromNodesVector) {
			x = 0;
			bound = INT_MAX;
			NodeID mid = INT_MAX;

			if (startPhantom.edgeBasedNode == UINT_MAX) {
				for (x=0; x< cols; x++) { 
					md1[cols*y+x].lengthOfShortestPath = ERR_FROM;
				} 
				continue;
			} 

			QueryHeap & forward_heap = *fh;
            forward_heap.Clear();
            forward_heap.Insert(startPhantom.edgeBasedNode, -startPhantom.weight1, startPhantom.edgeBasedNode);
            if(startPhantom.isBidirected()) {
               	forward_heap.Insert(startPhantom.edgeBasedNode+1, -startPhantom.weight2, startPhantom.edgeBasedNode+1);
            }
        	int forward_offset = startPhantom.weight1 + (startPhantom.isBidirected() ? startPhantom.weight2 : 0);

           	while (0 < forward_heap.Size()) { 
            	super::RoutingStep(forward_heap, eheap, &mid, &bound, forward_offset, true,false);
   	        }
			forward_heap.Transform();
       		BOOST_FOREACH(const PhantomNode & targetPhantom, toNodesVector) {
				QueryHeap & reverse_heap = *rh[x];
				bound=INT_MAX;
				mid=INT_MAX;

				if (targetPhantom.edgeBasedNode == UINT_MAX) {
					md1[y*cols+x].lengthOfShortestPath = ERR_TO;
					x++;
					continue;
				} 
            	int reverse_offset = targetPhantom.weight1 + (targetPhantom.isBidirected() ? targetPhantom.weight2 : 0);
	
           		while (0 < reverse_heap.Size()) { 
              		super::RoutingStep(reverse_heap, eheap, &mid, &bound, reverse_offset, false, false);
           		}
				reverse_heap.Transform();
				md1[y*cols+x].lengthOfShortestPath = super::Match(forward_heap,reverse_heap,&middle[cols*y+x],&bound);

				std::vector<NodeID> temporaryPackedPath1;
				std::vector<NodeID> packedPath1;
				if(INT_MAX == md1[y*cols+x].lengthOfShortestPath) {
					md1[y*cols+x].lengthOfShortestPath = ERR_LINK; 	// no route !!
				} 
				x++;
			} 
			y++;
		} 

		// second stage ------------------ 
        RawRouteData *md2 = new RawRouteData[rows*cols];
		memset(md2,0,rows*cols*sizeof(RawRouteData));

		for (i=0; i< rows*cols; i++) { 
			middle[i] = INT_MAX;
			md2[i].lengthOfShortestPath=ERR_FROM;
		} 
		x=0;
       	BOOST_FOREACH(const PhantomNode & targetPhantom, toNodesVector) {
			if (targetPhantom.edgeBasedNode == UINT_MAX) {
				if (rh[x]) delete rh[x];
				rh[x++] = NULL;
				continue;
			} else { 
				if (rh[x]) rh[x]->Clear();
				else rh[x]= new QueryHeap(super::_queryData.nodeHelpDesk->getNumberOfNodes());
			} 
			QueryHeap & reverse_heap = *rh[x];
			reverse_heap.Clear();
            if(targetPhantom.isBidirected() ) {
               	reverse_heap.Insert(targetPhantom.edgeBasedNode+1, targetPhantom.weight2, targetPhantom.edgeBasedNode+1);
            }
			x++;
		} 

        // set start nodes for all from heaps
		y=0;
        BOOST_FOREACH(const PhantomNode & startPhantom, fromNodesVector) {
			x = 0;
			bound=INT_MAX;
			NodeID mid=INT_MAX;
			if (startPhantom.edgeBasedNode == UINT_MAX) {
				for (x=0; x< cols; x++) { 
					md2[cols*y+x].lengthOfShortestPath = ERR_FROM;
				} 
				continue;
			} 

			QueryHeap & forward_heap = *fh;
            forward_heap.Clear();
			forward_heap.Insert(startPhantom.edgeBasedNode, -startPhantom.weight1, startPhantom.edgeBasedNode);
			if(startPhantom.isBidirected()) {
                forward_heap.Insert(startPhantom.edgeBasedNode+1, -startPhantom.weight2, startPhantom.edgeBasedNode+1);
            }

        	int forward_offset = startPhantom.weight1 + (startPhantom.isBidirected() ? startPhantom.weight2 : 0);
           	while (0 < forward_heap.Size()) { 
            	super::RoutingStep(forward_heap, eheap, &mid, &bound, forward_offset, true, false);
   	        }
			forward_heap.Transform();
       		BOOST_FOREACH(const PhantomNode & targetPhantom, toNodesVector) {
				QueryHeap & reverse_heap = *rh[x];
				bound=INT_MAX;
				mid=INT_MAX;
	
				if (targetPhantom.edgeBasedNode == UINT_MAX) { 
					md2[y*cols+x].lengthOfShortestPath = ERR_TO;
					x++;
					continue;
				} 

            	int reverse_offset = targetPhantom.weight1 + (targetPhantom.isBidirected() ? targetPhantom.weight2 : 0);
	
           		while (0 < reverse_heap.Size()) { 
              		super::RoutingStep(reverse_heap, eheap, &mid, &bound, reverse_offset, false, false);
           		}
				reverse_heap.Transform();
				md2[y*cols+x].lengthOfShortestPath = super::Match(forward_heap,reverse_heap,&middle[cols*y+x],&bound);
				std::vector<NodeID> temporaryPackedPath;
				std::vector<NodeID> packedPath;
				if(INT_MAX == md2[y*cols+x].lengthOfShortestPath) {
					md2[y*cols+x].lengthOfShortestPath = ERR_LINK;
				} 
				x++;
			} 
			y++;
		} 

		// wrapup : choose the lowest cost of the 2 dijkstra's
		for (y=0; y< rows; y++) { 
			for (x=0; x< cols; x++) { 
				RawRouteData d=md2[y*cols+x];
				if (md1[y*cols+x].lengthOfShortestPath < md2[y*cols+x].lengthOfShortestPath) 
					d = md1[y*cols+x];
				// -1 means no result, at least try the other one
				if (md1[y*cols+x].lengthOfShortestPath < 0) 
					d = md2[y*cols+x];
				if (md2[y*cols+x].lengthOfShortestPath < 0) 
					d = md1[y*cols+x];

        		rr[y*cols+x].lengthOfShortestPath = d.lengthOfShortestPath;
			} 
		} 

		for (i=0; i< rows; i++) {
			if (rh[i]) delete rh[i];
		} 
			
		delete[] rh;
		delete emptyheap;
		delete fh;

		delete[] middle;
		delete[] md1;
		delete[] md2;
       	return;
    }
};

#endif  // MATRIXPATHROUTING_H_
