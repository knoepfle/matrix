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

#ifndef DISTANCEMATRIXPLUGIN_H_
#define DISTANCEMATRIXPLUGIN_H_



#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "BasePlugin.h"

#include "../Algorithms/ObjectToBase64.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/StaticGraph.h"
#include "../DataStructures/SearchEngine.h"
#include "../Descriptors/BaseDescriptor.h"
#include "../Descriptors/GPXDescriptor.h"
#include "../Descriptors/JSONDescriptor.h"
#include "../Server/DataStructures/QueryObjectsStorage.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"


class MatrixPlugin : public BasePlugin {
private:
    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<std::string> & names;
    StaticGraph<QueryEdge::EdgeData> * graph;
    HashTable<std::string, unsigned> descriptorTable;
    SearchEngine* searchEngine;
    std::string descriptor_string;
public:

    MatrixPlugin(QueryObjectsStorage * objects) : names(objects->names), descriptor_string("matrix") {
        nodeHelpDesk = objects->nodeHelpDesk;
        graph = objects->graph;

        searchEngine = new SearchEngine(graph, nodeHelpDesk, names);

        descriptorTable.insert(std::make_pair(""    , 0));
        descriptorTable.insert(std::make_pair("json", 0));
        descriptorTable.insert(std::make_pair("gpx", 1));
    }

    virtual ~MatrixPlugin() {
        delete searchEngine;
    }

    const std::string& GetDescriptor() const { return descriptor_string; }
    std::string GetVersionString() const { return std::string("0.3 (DL)"); }
    void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
        //check number of parameters
        if( 2 > routeParameters.coordinates.size() || INT_MAX < routeParameters.coordinates.size()) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }

        RawRouteData rawRoute;
        rawRoute.checkSum = nodeHelpDesk->GetCheckSum();
        bool checksumOK = (routeParameters.checkSum == rawRoute.checkSum);
        std::vector<std::string> textCoord;
        for(unsigned i = 0; i < routeParameters.coordinates.size(); ++i) {
            if(false == checkCoord(routeParameters.coordinates[i])) {
                reply = http::Reply::stockReply(http::Reply::badRequest);
                return;
            }
            rawRoute.rawViaNodeCoordinates.push_back(routeParameters.coordinates[i]);
        }
        std::vector<PhantomNode> phantomNodeVector(rawRoute.rawViaNodeCoordinates.size());
        for(unsigned i = 0; i < rawRoute.rawViaNodeCoordinates.size(); ++i) {
            if(checksumOK && i < routeParameters.hints.size() && "" != routeParameters.hints[i]) {
//                INFO("Decoding hint: " << routeParameters.hints[i] << " for location index " << i);
                DecodeObjectFromBase64(routeParameters.hints[i], phantomNodeVector[i]);
                if(phantomNodeVector[i].isValid(nodeHelpDesk->getNumberOfNodes())) {
//                    INFO("Decoded hint " << i << " successfully");
                    continue;
                }
            }
//            INFO("Brute force lookup of coordinate " << i);
            searchEngine->FindPhantomNodeForCoordinate( rawRoute.rawViaNodeCoordinates[i], phantomNodeVector[i], routeParameters.zoomLevel);
        }

        reply.status = http::Reply::ok;

        //TODO: Move to member as smart pointer
        if("" != routeParameters.jsonpParameter) {
            reply.content += routeParameters.jsonpParameter;
            reply.content += "(";
        }

        unsigned descriptorType = descriptorTable[routeParameters.outputFormat];
        std::stringstream times;

		times << "[";

		int nlocs= phantomNodeVector.size();

		RawRouteData *matrix = (RawRouteData *)calloc(sizeof(RawRouteData),nlocs*nlocs);
		searchEngine->matrixRouting(phantomNodeVector, phantomNodeVector, matrix);
        for(unsigned i = 0; i < phantomNodeVector.size(); ++i) {
			if (i>0) { times << ","; } 
			times << '[';
            for(unsigned j = 0; j < phantomNodeVector.size(); ++j) {
				if (j>0) times << ",";
				int seconds_or_error= matrix[i*nlocs+j].lengthOfShortestPath;
				if (seconds_or_error >0 ) seconds_or_error = seconds_or_error /10 + 1;
				times << seconds_or_error;
			} 
			times << ']';
		} 
		times << ']';

		reply.content += "[{\"version\": 0.3,\"status\":0,\"status_message\": \"Found matrix\"";
        reply.content += ",time_matrix:" + times.str();
		reply.content += "}]";

        if("" != routeParameters.jsonpParameter) {
            reply.content += ")\n";
        }

        reply.headers.resize(3);
        reply.headers[0].name = "Content-Length";
        std::string tmp;
        intToString(reply.content.size(), tmp);
        reply.headers[0].value = tmp;
        switch(descriptorType){
        case 0:
            if("" != routeParameters.jsonpParameter){
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "text/javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.js\"";
            } else {
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "application/x-javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.json\"";
            }

            break;
        case 1:
            reply.headers[1].name = "Content-Type";
            reply.headers[1].value = "application/gpx+xml; charset=UTF-8";
            reply.headers[2].name = "Content-Disposition";
            reply.headers[2].value = "attachment; filename=\"route.gpx\"";

            break;
        default:
            if("" != routeParameters.jsonpParameter){
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "text/javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.js\"";
            } else {
                reply.headers[1].name = "Content-Type";
                reply.headers[1].value = "application/x-javascript";
                reply.headers[2].name = "Content-Disposition";
                reply.headers[2].value = "attachment; filename=\"route.json\"";
            }

            break;
        }

        return;
    }
};

#endif
