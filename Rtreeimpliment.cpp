//
// Created by Carter Swarm on 11/29/24.
//
//sources that I used to gain some ideas on how Exactly a R tree works.
//textbook that inspired some of my simple insertion and splitting algortihms
//http://albert-jan.yzelman.net/PDFs/yzelman07b.pdf
//https://lib.ysu.am/disciplines_bk/b456f3d1b3ea0bd76d156319de2da855.pdf had some use as well
//
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <limits>
#include <fstream>
#include <sstream>
#include "RtreeImpliment.h"

using namespace std;

// stores the start time and time in air so we can calculate total time, time is in unix
double FlightInterval::getTotalTime() const {
    return start + timeInAir;
}

// Update MinBondingRectangle based on a new flight interval
void Node::updateMBR(const FlightInterval& interval) {
    minbonding_start = min(minbonding_start, interval.start);
    minbonding_end = max(minbonding_end, interval.getTotalTime());
}

// Update MinBondingRectangle for the child nodes
void Node::updateMBR(const shared_ptr<Node>& child) {
    minbonding_start = min(minbonding_start, child->minbonding_start); //sets == to either the largest or smallest value in a double
    minbonding_end = max(minbonding_end, child->minbonding_end);
}


// creates a FlightInterval object, calls the helper whitch inserts into the tree,
//checks if the new node was split (if it is not == to null then it split),
//if so, then a new root is created to hold the original root and the newly split node as its children
void FlightRTree::insert(const string& flightNumber, double start, double duration) {
    FlightInterval interval(flightNumber, start, duration);
    auto newNode = insertHelper(root, interval);
    if (newNode != nullptr) {
        auto newRoot = make_shared<Node>(false);
        newRoot->children.push_back(root);
        newRoot->children.push_back(newNode);
        newRoot->updateMBR(root);
        newRoot->updateMBR(newNode);
        root = newRoot;
    }
}

//through recursion
//if it is a leaf, the interval is directly added to the node's entries and if it is greather then the Max, then we split
//if not, we then chose the best child (least amount of expansion) and if it split, handle that accordnly
shared_ptr<Node> FlightRTree::insertHelper(shared_ptr<Node>& node, const FlightInterval& interval) {
    if (node->isLeaf) {
        node->entries.push_back(interval);
        node->updateMBR(interval);
        if (node->entries.size() > Node::MAX_ENTRIES) {
            return splitNode(node);
        }
        return nullptr;
    }
    int bestIdx = chooseBestChild(node, interval);
    auto newNode = insertHelper(node->children[bestIdx], interval);
    if (newNode != nullptr) {
        if (node->children.size() < Node::MAX_ENTRIES) {
            node->children.push_back(newNode);
            node->updateMBR(newNode);
            return nullptr;
        }
        return splitNode(node);
    }
    node->updateMBR(node->children[bestIdx]);
    return nullptr;
}


//we determin which child has the least/smallest minBondingRec expansion
int FlightRTree::chooseBestChild(const shared_ptr<Node>& node, const FlightInterval& interval) {
    double minExpansion = numeric_limits<double>::max();
    int best = 0;
    for (size_t i = 0; i < node->children.size(); ++i) {
        auto child = node->children[i];
        double calculatedexpansion = max(interval.getTotalTime(), child->minbonding_end) - min(interval.start, child->minbonding_start) - (child->minbonding_end - child->minbonding_start);
        if (calculatedexpansion < minExpansion) {
            minExpansion = calculatedexpansion;
            best = i;
        }
    }
    return best;
}

// Split a node when it exceeds maximum entries, depends on if a leaf or not. first have to create a new node.
//then have to Sort its entries by start time, determin the split point, (chose to do a simplified verson of a linear split (1/2 methoud) from the textbbook below it is the simplest to implement not worrying about overlap)
//source http://albert-jan.yzelman.net/PDFs/yzelman07b.pdf
//It is intresting to note that  the linear split takes more memory compared to a quadratic split method. (affects space complexity)
//recalculate the new minboundingrectangle at the end
shared_ptr<Node> FlightRTree::splitNode(shared_ptr<Node>& node) {
    auto newNode = make_shared<Node>(node->isLeaf);
    if (node->isLeaf) {
        sort(node->entries.begin(), node->entries.end(), [](const FlightInterval& a, const FlightInterval& b) { //returns true if A is before b in sorted order
            return a.start < b.start;
        });
        size_t half = node->entries.size() / 2;
        vector<FlightInterval> tempspace;
        for (size_t i = 0; i < half; ++i) {
            tempspace.push_back(node->entries[i]);
        }
        for (size_t i = half; i < node->entries.size(); ++i) {
            newNode->entries.push_back(node->entries[i]);
            newNode->updateMBR(node->entries[i]);
        }
        node->entries = move(tempspace);
        recalculateMBR(node);
        recalculateMBR(newNode);
    } else {
        sort(node->children.begin(), node->children.end(), [](const shared_ptr<Node>& a, const shared_ptr<Node>& b) { //returns true if A is before b in sorted order
            return a->minbonding_start < b->minbonding_start;
        });
        size_t mid = node->children.size() / 2;
        for (size_t i = mid; i < node->children.size(); ++i) {
            newNode->children.push_back(node->children[i]);
            newNode->updateMBR(node->children[i]);
        }
        node->children.resize(mid);
        recalculateMBR(node);
        recalculateMBR(newNode);
    }
    return newNode;
}


// Helper method to recalculate the MBR of a node,
// If  node is leaf, update the MBR using all its entries
// else, update the MBR using all its children
void FlightRTree::recalculateMBR(const shared_ptr<Node>& node) {
    node->minbonding_start = numeric_limits<double>::max();
    node->minbonding_end = numeric_limits<double>::lowest();
    if (node->isLeaf) {
        for (size_t i = 0; i < node->entries.size(); ++i) {
            node->updateMBR(node->entries[i]);
        }
    } else {
        for (size_t i = 0; i < node->children.size(); ++i) {
            node->updateMBR(node->children[i]);
        }
    }
}

// Search for flights that contain a specific time point
vector<FlightInterval> FlightRTree::search(double queryTime) const {
    vector<FlightInterval> results;
    if (queryTime >= root->minbonding_start && queryTime <= root->minbonding_end) {
        searchHelper(root, queryTime, results);
    } else {
        cout << "none found";
    }
    return results;
}

// Recursive search helper for the R tree
void FlightRTree::searchHelper(const shared_ptr<Node>& node, double queryTime, vector<FlightInterval>& results) const {
    if (node->isLeaf) {
        for (size_t i = 0; i < node->entries.size(); ++i) {
            FlightInterval entry = node->entries[i];
            if (entry.start <= queryTime && queryTime <= entry.getTotalTime()) {
                results.push_back(entry);
            }
        }
    } else {
        for (size_t i = 0; i < node->children.size(); ++i) {
            shared_ptr<Node> child = node->children[i];
            if (child->minbonding_start <= queryTime && queryTime <= child->minbonding_end) {
                searchHelper(child, queryTime, results);
            }
        }
    }
}


// Traverse and print the tree structure for the R tree
void FlightRTree::traverse() const {
    cout << "Flight R Tree Traversal:" << endl;
    traverseHelper(root);
}


// Recursive traversal helper for the R Tree
void FlightRTree::traverseHelper(const shared_ptr<Node>& node, int depth) const {
    string indent(depth * 1.5, ' ');
    cout << indent << "Node: [" << node->minbonding_start << ", " << node->minbonding_end << "]" << endl;
    if (node->isLeaf) {
        for (size_t i = 0; i < node->entries.size(); ++i) {
            FlightInterval entry = node->entries[i];
            cout << indent << "Flight: " << entry.flightNumb << " [" << entry.start << ", " << entry.getTotalTime() << "]" << endl;
        }
    } else {
        for (size_t i = 0; i < node->children.size(); ++i) {
            shared_ptr<Node> child = node->children[i];
            traverseHelper(child, depth + 1);
        }
    }
}


void FlightRTree::loadFromCSV(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open file " << filename << endl;
        return;
    }
    string line;
    getline(file, line);
    while (getline(file, line)) { //parsing,formating, insertion from file
        stringstream ss(line);
        string unixTimeStr, flightNumberStr, durationStr;
        getline(ss, unixTimeStr, ',');
        getline(ss, flightNumberStr, ',');
        getline(ss, durationStr);
        double unixTime = stod(unixTimeStr);
        double duration = stod(durationStr);
        insert(flightNumberStr, unixTime, duration);
    }
}

