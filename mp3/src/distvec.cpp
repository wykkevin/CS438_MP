#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<iostream>
#include<vector>
#include<set>
#include <sstream>
#include <queue>
#include <fstream>
#include <map>
#include <climits>
#include <algorithm>

using namespace std;

typedef pair<int, int> pii;

struct Message {
    int src;
    int dest;
    string m;
};

struct Change {
    int src, dest, newCost;
};

struct DistanceInfo {
    int cost;
    vector<int> path;
};

// Used when changes remove the connection
struct EraseIfFirstEqual {
    int value;

    EraseIfFirstEqual(int val) : value(val) {}

    bool operator()(const pii &p) const {
        return p.first == value;
    }
};

map <pii, DistanceInfo> bellmanFord(vector <vector<pii>> &graph, int src, int V, ofstream &outputFile) {
    // node, row of the table in node, col of the table in node
    map <pii, DistanceInfo> distances;

    // initialize
//    for(int i = 0; i < V; i++){
//        vector<int> p;
//        DistanceInfo d = {numeric_limits<int>::max(), p};
//        distances[make_pair(src, i)] = d;
//    }
//
//    // set the cost of the src to 0
//    vector<int> p;
//    p.push_back(src);
//    DistanceInfo d = {0, p};
//    distances[make_pair(src, src)] = d;
//
//    for(auto j : graph[src]){
//        int from = src;
//        int to = j.first;
//        int w = j.second;
//        distances[make_pair(from, to)].cost = w;
//        distances[make_pair(from, to)].path.push_back(from);
//    }

    for (int i = 0; i < V; i++) {
        if (i == src) {
            distances[make_pair(src, src)] = {0, {src}};
        } else {
            distances[make_pair(src, i)] = {numeric_limits<int>::max(), {}};
        }
    }

//    for(auto k : graph[src]){
//        distances[make_pair(src, k.first)] = {k.second, {src, k.first}};
//    }

    for (int i = 1; i < V; i++) {
        for (int j = 0; j < V; j++) {
            for (auto k: graph[j]) {
                int from = j;
                int to = k.first;
                int w = k.second;
//                if (src == 3) {
//                    cout << "from " << from << " to " << to << " w " << w << endl;
//                }
                int originalCost = distances[make_pair(src, to)].cost;
                int possibleNewCost = distances[make_pair(src, from)].cost + w;
//                if (src == 3) {
//                    cout << "originalCost " << originalCost << " possibleNewCost " << possibleNewCost << endl;
//                }
//                if (src == 3) {
//                    if (from == 5 && to == 4) {
//                        vector<int> p = distances[make_pair(src, to)].path;
//                        for (int aa: p) {
//                            cout << "current path " << aa << endl;
//                        }
//                        vector<int> tofromP = distances[make_pair(src, from)].path;
//                        for (int bb: tofromP) {
//                            cout << "to from " << from << " path " << bb << endl;
//                        }
//                    }
//                }

                if (distances[make_pair(src, from)].cost != numeric_limits<int>::max() &&
                    originalCost > possibleNewCost) {
//                    if (src == 3) {
//                        cout << "!!!" << endl;
//                    }
                    distances[make_pair(src, to)].cost = possibleNewCost;
                    distances[make_pair(src, to)].path = distances[make_pair(src, from)].path;
                    distances[make_pair(src, to)].path.push_back(to);
                } else if (distances[make_pair(src, from)].cost != numeric_limits<int>::max() &&
                           distances[make_pair(src, to)].cost == distances[make_pair(src, from)].cost + w) {
//                    if (src == 3) {
//                        cout << "@@@" << endl;
//                        if (from == 5 && to == 4) {
//                            cout << "distances[make_pair(src, to)].path[1]  " << distances[make_pair(src, to)].path[1]
//                                 << endl;
//                            cout << "distances[make_pair(src, from)].path[1]  "
//                                 << distances[make_pair(src, from)].path[1] << endl;
//                        }
//                    }
                    if ((distances[make_pair(src, to)].path.size() > 1 &&
                         distances[make_pair(src, from)].path.size() > 1 &&
                         distances[make_pair(src, to)].path[1] > distances[make_pair(src, from)].path[1])
                        || (distances[make_pair(src, to)].path.size() > 1 &&
                            distances[make_pair(src, from)].path.size() <= 1 &&
                            distances[make_pair(src, to)].path[1] > from)
                        || (distances[make_pair(src, to)].path.size() <= 1 &&
                            distances[make_pair(src, from)].path.size() > 1 &&
                            to > distances[make_pair(src, from)].path[1])
                        || (distances[make_pair(src, to)].path.size() <= 1 &&
                            distances[make_pair(src, from)].path.size() <= 1 &&
                            to > from)) {
//                        if (src == 3) {
//                            cout << "###" << endl;
//                        }

                        distances[make_pair(src, to)].path = distances[make_pair(src, from)].path;
                        distances[make_pair(src, to)].path.push_back(to);
                    } else if ((distances[make_pair(src, to)].path.size() > 1 &&
                                distances[make_pair(src, from)].path.size() > 1 &&
                                distances[make_pair(src, to)].path[1] == distances[make_pair(src, from)].path[1])
                               || (distances[make_pair(src, to)].path.size() > 1 &&
                                   distances[make_pair(src, from)].path.size() <= 1 &&
                                   distances[make_pair(src, to)].path[1] == from)
                               || (distances[make_pair(src, to)].path.size() <= 1 &&
                                   distances[make_pair(src, from)].path.size() > 1 &&
                                   to == distances[make_pair(src, from)].path[1])
                               || (distances[make_pair(src, to)].path.size() <= 1 &&
                                   distances[make_pair(src, from)].path.size() <= 1 &&
                                   to == from)) {
                        if (distances[make_pair(src, to)].path[distances[make_pair(src, to)].path.size() - 2]
                            > from) {
                            if (src == 3) {
                                cout << "$$$" << endl;
                            }

                            distances[make_pair(src, to)].path = distances[make_pair(src, from)].path;
                            distances[make_pair(src, to)].path.push_back(to);
                        }
                    }
                }
            }
        }
    }

//    for(auto it = distances.begin(); it != distances.end(); it++){
//        it->second.path.push_back(it->first.second);
//    }

    for (int i = 1; i < V; i++) {
        if (distances[make_pair(src, i)].cost != numeric_limits<int>::max()) {
            int nextHop = distances[make_pair(src, i)].path[1];
            if (distances[make_pair(src, i)].path.size() == 1) {
                nextHop = i;
            }
            int w = distances[make_pair(src, i)].cost;
            outputFile << i << " " << nextHop << " " << w << endl;
        }
    }
    outputFile << "\n" << endl;


    return distances;
}

void runAndWriteToFile(vector <vector<pii>> &graph, int maxNodeNum, set<int> nodesInGraph,
                       map <pair<int, int>, DistanceInfo> shortestPathMap, vector <Message> messageList,
                       ofstream &outputFile) {
    // TODO: run the whole table together?
//    for(auto it = graph.begin(); it != graph.end(); it++){
//        for(auto it2 = it->begin(); it2 != it->end(); it2++){
//            cout << it2->first << " " << it2->second << " ";
//        }
//        cout << endl;
//    }
    shortestPathMap.clear();
    for (int i = 0; i < maxNodeNum; i++) {
        if (nodesInGraph.find(i) != nodesInGraph.end()) {
            map <pii, DistanceInfo> oneNodeMap = bellmanFord(graph, i, maxNodeNum, outputFile);
            shortestPathMap.insert(oneNodeMap.begin(), oneNodeMap.end());
        }
    }

    for (int i = 0; i < messageList.size(); i++) {
        Message msg = messageList[i];
        int src = msg.src;
        int dest = msg.dest;
        string m = msg.m;
        auto it = shortestPathMap.find(make_pair(src, dest));

        if (it->second.cost == numeric_limits<int>::max()) {
            outputFile << "from " << src << " to " << dest << " cost infinite hops unreachable message " << m << endl;
        } else {
            DistanceInfo distanceInfo = it->second;
            outputFile << "from " << src << " to " << dest << " cost " << distanceInfo.cost << " hops ";
            vector<int> path = distanceInfo.path;
            for (int j = 0; j < path.size() - 1; j++) {
                outputFile << path[j] << " ";
            }
            outputFile << "message " << m << endl;
        }
    }

}

int main(int argc, char **argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    int maxNodeNum = 0;
    char line[128];
    set<int> nodesInGraph;

    FILE *topofile;
    topofile = fopen(argv[1], "r");
    if (topofile != NULL) {
        while (fgets(line, sizeof(line), topofile)) {
            int firstNode = atoi(strtok(line, " "));
            int secondNode = atoi(strtok(NULL, " "));
            nodesInGraph.insert(firstNode);
            nodesInGraph.insert(secondNode);
            maxNodeNum = max(max(maxNodeNum, firstNode), secondNode);
//            cout << "maxNodeNum " << maxNodeNum << endl;
        }
        maxNodeNum += 1;
        fclose(topofile);
    }

    vector <vector<pii>> graph(maxNodeNum);
    vector <Message> messageList;
    vector <Change> changesList;

    topofile = fopen(argv[1], "r");
    while (fgets(line, sizeof(line), topofile)) {
        int firstNode = atoi(strtok(line, " "));
        int secondNode = atoi(strtok(NULL, " "));
        int cost = atoi(strtok(NULL, " "));
//        cout << "first node " << firstNode << " second node " << secondNode << " cost " << cost << endl;
        graph[firstNode].push_back(make_pair(secondNode, cost));
        graph[secondNode].push_back(make_pair(firstNode, cost));
    }
    fclose(topofile);

    int src, dest;
    string m;
    ifstream messagefile(argv[2]);
    while (messagefile >> src >> dest) {
        getline(messagefile, m);
        Message msg = {src, dest, m.substr(1, m.length())};
        messageList.push_back(msg);
    }
    messagefile.close();

    FILE *changesfile;
    changesfile = fopen(argv[3], "r");
    while (fgets(line, sizeof(line), changesfile)) {
        int src = atoi(strtok(line, " "));
        int dest = atoi(strtok(NULL, " "));
        int newCost = atoi(strtok(NULL, " "));
        Change c = {src, dest, newCost};
        changesList.push_back(c);
    }
    fclose(changesfile);

    map <pair<int, int>, DistanceInfo> shortestPathMap;
    ofstream outputFile;
    outputFile.open("output.txt");
    runAndWriteToFile(graph, maxNodeNum, nodesInGraph, shortestPathMap, messageList, outputFile);
    for (int i = 0; i < changesList.size(); i++) {
        Change c = changesList[i];
        int src = c.src;
        int dest = c.dest;
        int newCost = c.newCost;
        if (newCost == -999) {
            vector <pii> srcGraph = graph[src];
            srcGraph.erase(remove_if(srcGraph.begin(), srcGraph.end(), EraseIfFirstEqual(dest)), srcGraph.end());
            vector <pii> destGraph = graph[dest];
            destGraph.erase(remove_if(destGraph.begin(), destGraph.end(), EraseIfFirstEqual(src)), destGraph.end());
            graph[src] = srcGraph;
            graph[dest] = destGraph;
        } else {
            graph[src].push_back(make_pair(dest, newCost));
            graph[dest].push_back(make_pair(src, newCost));
        }
        runAndWriteToFile(graph, maxNodeNum, nodesInGraph, shortestPathMap, messageList, outputFile);
    }
    outputFile.close();
    return 0;
}