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


#ifndef QUERYOBJECTSSTORAGE_H_
#define QUERYOBJECTSSTORAGE_H_

#include "../../Util/GraphLoader.h"
#include "../../Util/OSRMException.h"
#include "../../Util/SimpleLogger.h"
#include "../../DataStructures/NodeInformationHelpDesk.h"
#include "../../DataStructures/QueryEdge.h"
#include "../../DataStructures/StaticGraph.h"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <vector>
#include <string>


struct QueryObjectsStorage {
    typedef StaticGraph<QueryEdge::EdgeData>    QueryGraph;
    typedef QueryGraph::InputEdge               InputEdge;

    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<std::string> names;
    QueryGraph * graph;
    std::string timestamp;
    unsigned checkSum;

    QueryObjectsStorage(
        const std::string & hsgrPath,
        const std::string & ramIndexPath,
        const std::string & fileIndexPath,
        const std::string & nodesPath,
        const std::string & edgesPath,
        const std::string & namesPath,
        const std::string & timestampPath
    );

    ~QueryObjectsStorage();
};

#endif /* QUERYOBJECTSSTORAGE_H_ */
