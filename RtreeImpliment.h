//
// Created by Carter Swarm on 11/29/24.
//
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <limits>
#include <fstream>
#include <sstream>

#ifndef RTREEGROUPPROJECT_RTREEIMPLIMENT_H
#define RTREEGROUPPROJECT_RTREEIMPLIMENT_H

#endif //RTREEGROUPPROJECT_RTREEIMPLIMENT_H


using namespace std;

// Structure class to represent a flight interval
class FlightInterval {
public:
    string flightNumb; // Flight number from cvs file
    double start;        // Unix timestamp start time also from the file stored in the tree
    double timeInAir;     // Actual elapsed time in seconds from the file stored in the tree
    FlightInterval(const string& num, double s, double d) : flightNumb(num), start(s), timeInAir(d) {}
    double getTotalTime() const;  //calculate the total time
};

// Node class for the R-tree implimtation
//stores the min bonding rectangle, its entries, and provides methods to update its bounds based on
// new data or children with max num of entries per node.
class Node {
public:
    bool isLeaf;
    double minbonding_start;
    double minbonding_end;
    vector<shared_ptr<Node> > children;
    vector<FlightInterval> entries;
    static const int MAX_ENTRIES = 2;  // Maximum entries per node, are pros and cons to each one depending what needs done
    Node(bool leaf = true) : isLeaf(leaf), minbonding_start(numeric_limits<double>::max()), minbonding_end(numeric_limits<double>::lowest()) {}
    void updateMBR(const FlightInterval& interval) ;   // Update MBR based on a new flight interval
    void updateMBR(const shared_ptr<Node>& child); // Update MBR based on a child node
};


//some basic implimatations in the tree like insertion, searches, traversals, etc
class FlightRTree {
private:
    shared_ptr<Node> root;
    int chooseBestChild(const shared_ptr<Node>& node, const FlightInterval& interval);
    void recalculateMBR(const shared_ptr<Node>& node);
    shared_ptr<Node> splitNode(shared_ptr<Node>& node);  // Split a node when it exceeds maximum entries when building tree
    shared_ptr<Node> insertHelper(shared_ptr<Node>& node, const FlightInterval& interval);
    void searchHelper(const shared_ptr<Node>& node, double queryTime, vector<FlightInterval>& results) const;
    void traverseHelper(const shared_ptr<Node>& node, int depth = 0) const;
public:
    FlightRTree() : root(make_shared<Node>()) {}
    void insert(const string& flightNumber, double start, double duration); // Insert flight interval into  tree (used const to fix and prevent some unattended modifications)
    void loadFromCSV(const string& filename);  // Read from  file, insert in R-tree
    vector<FlightInterval> search(double queryTime) const;// Search for flights that contain a specific time point (from a vector of diffrent times)
    void traverse() const; // Traverse and print the tree structure
};
