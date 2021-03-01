#ifndef NFD_DAEMON_TABLE_MIN_HEAP
#define NFD_DAEMON_TABLE_MIN_HEAP



#include <string>
#include <iostream>
#include <memory>
#include <vector>

#include "cs-entry.hpp"

using EntryRef = nfd::cs::Table::const_iterator;

struct heapEntry{
    uint32_t popularity;
    std::string name;
    int current_index;
    EntryRef entry;
};

class MinHeap{


public:
void insert(std::shared_ptr<heapEntry> entry);
void update(std::shared_ptr<heapEntry> entry, uint32_t new_popularity);
void remove(std::shared_ptr<heapEntry> entry);
std::shared_ptr<heapEntry> pop();

uint32_t peekPopularity();

void print();



private:
void swap(std::shared_ptr<heapEntry> A, std::shared_ptr<heapEntry> B);
std::shared_ptr<heapEntry> min(std::shared_ptr<heapEntry> A, std::shared_ptr<heapEntry> B);
std::shared_ptr<heapEntry> getParent(std::shared_ptr<heapEntry> entry);
std::shared_ptr<heapEntry> getLeftChild(std::shared_ptr<heapEntry> entry);
std::shared_ptr<heapEntry> getRightChild(std::shared_ptr<heapEntry> entry);

std::vector<std::shared_ptr<heapEntry>> heap;

};

#endif