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
typedef vector <pii> vpii;

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

struct CompareDistanceInfo {
    bool operator()(pair<DistanceInfo, int> const &d1, pair<DistanceInfo, int> const &d2) {
        if (d1.first.cost > d2.first.cost) {
            return true;
        } else if (d1.first.cost == d2.first.cost) {
            if (d1.second > d2.second) {
                return true;
            }
        }
        return false;
    }
};

map <pii, DistanceInfo> dijkstra(vector <vpii> &graph, int src, int V, set<int> nodesInGraph) {
    map <pii, DistanceInfo> shortestPaths;

    map<int, DistanceInfo> D;
    vector<bool> N(V, false);

    vector<int> pathVector;
    pathVector.push_back(src);
    for (int i: nodesInGraph) {
        DistanceInfo distInfo = {INT_MAX, pathVector};
        D[i] = distInfo;
    }
    D[src].cost = 0;

    priority_queue < pair < DistanceInfo, int >, vector < pair < DistanceInfo, int > >, CompareDistanceInfo > pq;
    DistanceInfo distInfo = {INT_MAX, pathVector};
    pq.push(make_pair(D[src], src));

    while (!pq.empty()) {
        int w = pq.top().second;
        DistanceInfo currDistInfo = pq.top().first;
        pq.pop();

        if (N[w]) {
            continue;
        }
        N[w] = true;

        for (pii &v: graph[w]) {
            int vNode = v.first;
            int originalCost = D[vNode].cost;
            int possibleNewCost = D[w].cost + v.second;
            if (originalCost > possibleNewCost) {
                D[vNode].cost = possibleNewCost;
                vector<int> pathVector = D[w].path;
                pathVector.push_back(vNode);
                D[vNode].path = pathVector;
                pq.push(make_pair(D[vNode], vNode));
            } else if (originalCost == possibleNewCost) {
                vector<int> originalPath = D[vNode].path;
                int nodeBeforeCurrent = originalPath[originalPath.size() - 2];
                vector<int> possibleNewPath = D[w].path;
                if (nodeBeforeCurrent > w) {
                    // Use new path when its id is smaller
                    possibleNewPath.push_back(vNode);
                    D[vNode].path = possibleNewPath;
                    pq.push(make_pair(D[vNode], vNode));
                }
            }
        }
    }

    FILE *fpOut;
    fpOut = fopen("output.txt", "a");
    for (int i: nodesInGraph) {
        int nodeCost = D[i].cost;
        if (nodeCost != INT_MAX) {
            vector<int> nodePath = D[i].path;
            int nextHop = nodePath[0];
            if (nodePath.size() > 1) {
                nextHop = nodePath[1];
            }
            fprintf(fpOut, "%d %d %d\n", i, nextHop, nodeCost);
            pii p = make_pair(src, i);
            shortestPaths[p] = D[i];
        }
    }
    fclose(fpOut);
    return shortestPaths;
}

void runAndWriteToFile(vector <vpii> &graph, int maxNodeNum, set<int> nodesInGraph,
                       map <pair<int, int>, DistanceInfo> shortestPathMap, vector <Message> messageList) {
    shortestPathMap.clear();
    for (int i: nodesInGraph) {
        map <pii, DistanceInfo> oneNodeMap = dijkstra(graph, i, maxNodeNum, nodesInGraph);
        shortestPathMap.insert(oneNodeMap.begin(), oneNodeMap.end());
    }

    FILE *fpOut;
    fpOut = fopen("output.txt", "a");
    for (int i = 0; i < messageList.size(); i++) {
        Message msg = messageList[i];
        int src = msg.src;
        int dest = msg.dest;
        string m = msg.m;
        auto it = shortestPathMap.find(make_pair(src, dest));
        if (it == shortestPathMap.end()) {
            fprintf(fpOut, "from %d to %d cost infinite hops unreachable message %s\n", src, dest, m.c_str());
        } else {
            DistanceInfo distanceInfo = it->second;
            fprintf(fpOut, "from %d to %d cost %d hops ", src, dest, distanceInfo.cost);
            vector<int> path = distanceInfo.path;
            for (int j = 0; j < path.size() - 1; j++) {
                fprintf(fpOut, "%d ", path[j]);
            }
            fprintf(fpOut, "message %s\n", m.c_str());
        }
    }
    fclose(fpOut);
}

int main(int argc, char **argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
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
        }
        maxNodeNum += 1;
        fclose(topofile);
    }

    vector <vpii> graph(maxNodeNum);
    vector <Message> messageList;
    vector <Change> changesList;

    topofile = fopen(argv[1], "r");
    while (fgets(line, sizeof(line), topofile)) {
        int firstNode = atoi(strtok(line, " "));
        int secondNode = atoi(strtok(NULL, " "));
        int cost = atoi(strtok(NULL, " "));
        graph[firstNode].erase(
                remove_if(graph[firstNode].begin(), graph[firstNode].end(), EraseIfFirstEqual(secondNode)),
                graph[firstNode].end());
        graph[secondNode].erase(
                remove_if(graph[secondNode].begin(), graph[secondNode].end(), EraseIfFirstEqual(firstNode)),
                graph[secondNode].end());
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
    FILE *fpOut;
    fpOut = fopen("output.txt", "w");
    fclose(fpOut);

    runAndWriteToFile(graph, maxNodeNum, nodesInGraph, shortestPathMap, messageList);
    for (int i = 0; i < changesList.size(); i++) {
        Change c = changesList[i];
        int src = c.src;
        int dest = c.dest;
        int newCost = c.newCost;
        vector <pii> srcGraph = graph[src];
        srcGraph.erase(remove_if(srcGraph.begin(), srcGraph.end(), EraseIfFirstEqual(dest)), srcGraph.end());
        vector <pii> destGraph = graph[dest];
        destGraph.erase(remove_if(destGraph.begin(), destGraph.end(), EraseIfFirstEqual(src)), destGraph.end());
        if (newCost != -999) {
            nodesInGraph.insert(src);
            nodesInGraph.insert(dest);
            srcGraph.push_back(make_pair(dest, newCost));
            destGraph.push_back(make_pair(src, newCost));
            maxNodeNum = max(max(maxNodeNum, src), dest);
            maxNodeNum += 1;
        }
        graph[src] = srcGraph;
        graph[dest] = destGraph;
        runAndWriteToFile(graph, maxNodeNum, nodesInGraph, shortestPathMap, messageList);
    }
    return 0;
}