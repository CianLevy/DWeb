#include "priority_queue.hpp"
#include "NFD/daemon/fw/magic_utils.hpp"

void MinHeap::insert(std::shared_ptr<heapEntry> entry){
    int current_index = heap.size();
    heap.push_back(entry);
    entry->current_index = current_index;
    
    std::shared_ptr<heapEntry> parent = getParent(entry);

    while (parent && parent->popularity > entry->popularity){
        swap(parent, entry);
        parent = getParent(entry);
    }

}

void MinHeap::update(std::shared_ptr<heapEntry> entry, uint32_t new_popularity){
    if (new_popularity >= entry->popularity){
        entry->popularity = new_popularity;

        std::shared_ptr<heapEntry> curr = min(getLeftChild(entry), getRightChild(entry));

        while (curr && curr->popularity <= entry->popularity){
            swap(curr, entry);
            curr = min(getLeftChild(entry), getRightChild(entry));
        }
    }
    else if (new_popularity < entry->popularity){
        entry->popularity = new_popularity;

        std::shared_ptr<heapEntry> curr = getParent(entry);

        while (curr && curr->popularity > entry->popularity){
            swap(curr, entry);
            curr = getParent(entry); 
        }
    }
}

std::shared_ptr<heapEntry> MinHeap::min(std::shared_ptr<heapEntry> A, std::shared_ptr<heapEntry> B){
    if ((!B && A) || (A && A->popularity < B->popularity))
        return A;
    else if (B)
        return B;
    else
        return nullptr;
}

void MinHeap::remove(std::shared_ptr<heapEntry> entry){
    std::shared_ptr<heapEntry> end = heap.at(heap.size() - 1);
    swap(entry, end);
    heap.pop_back();

    uint32_t previous_pop = end->popularity;

    end->popularity = entry->popularity;

    update(end, previous_pop);

}


std::shared_ptr<heapEntry> MinHeap::pop(){
    std::shared_ptr<heapEntry> res = nullptr;

    if (heap.size() > 0){
        swap(heap.at(0), heap.at(heap.size() - 1));
        res = heap.back();
        heap.pop_back();

        if (heap.size() > 0){
            std::shared_ptr<heapEntry> new_root = heap.at(0);
            std::shared_ptr<heapEntry> curr = min(getLeftChild(new_root), getRightChild(new_root));

            while (curr && curr->popularity < new_root->popularity){
                swap(curr, new_root);
                curr = min(getLeftChild(new_root), getRightChild(new_root));
            }
        }
    }

    return res;
}


void MinHeap::swap(std::shared_ptr<heapEntry> A, std::shared_ptr<heapEntry> B){
    heap.at(A->current_index) = B;
    heap.at(B->current_index) = A;

    int temp_index = A->current_index;
    A->current_index = B->current_index;
    B->current_index = temp_index;  
}

std::shared_ptr<heapEntry> MinHeap::getParent(std::shared_ptr<heapEntry> entry){
    if (entry->current_index > 0){
        int parent_index = (entry->current_index - 1) / 2;
        return heap.at(parent_index);
    }
    else
        return nullptr;    
}

std::shared_ptr<heapEntry> MinHeap::getLeftChild(std::shared_ptr<heapEntry> entry){
    int index = 2 * entry->current_index + 1;

    if (index < heap.size())
        return heap.at(index);
    else
        return nullptr;
}

std::shared_ptr<heapEntry> MinHeap::getRightChild(std::shared_ptr<heapEntry> entry){
    int index = 2 * entry->current_index + 2;

    if (index < heap.size())
        return heap.at(index);
    else
        return nullptr;
}


void MinHeap::print(){
    for (auto entry : heap){
        std::cout << entry->popularity << " ";

        if (heap.at(0)->popularity > entry->popularity)
            std::cout << "Popularity heap misorder detected" << std::endl;
    }
    std::cout << std::endl;
}

uint32_t MinHeap::peekPopularity(){
    if (heap.size() > 0)
        return heap.at(0)->popularity;
    else
        return 0;
}

uint32_t MinHeap::getCurrentSize(){
    return (uint32_t)heap.size();
}

bool MinHeap::isFull(){
    return heap.size() >= max_size;
}