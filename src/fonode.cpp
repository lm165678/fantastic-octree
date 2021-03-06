#include "fonode.h"

#include <iterator>
#include <algorithm>
#include <iostream>

foNode* foNode::waitOn(foNode *signaler, vk::PipelineStageFlags stage) {
    waitNodes.push_back(signaler);
    waitNodeStages[signaler] = stage;
    return this;
}

foNode* foNode::signalTo(foNode *waiter, vk::PipelineStageFlags stage) {
    signalNodes.push_back(waiter);
    waiter->waitOn(this, stage);
    if(semaphores != nullptr) semaphoreHandles.push_back(semaphores->makeSemaphore());
    return this;
}

void foNode::stopSignal(foNode* waiter) {
    signalNodes.erase(std::find(signalNodes.begin(), signalNodes.end(), waiter));
}

void foNode::stopWait(foNode* signaler) {
    waitNodes.erase(std::find(waitNodes.begin(), waitNodes.end(), signaler));
}


void foNode::prepare(Sync *semaphores) {
    this->semaphores = semaphores;
    waitSemaphores.resize(waitNodes.size());
    waitStages.resize(waitNodes.size());
    signalSemaphores.resize(signalNodes.size());
    semaphoreHandles.resize(signalNodes.size());
    for(uint32_t i = 0; i < signalNodes.size(); i++) {
        semaphoreHandles[i] = semaphores->makeSemaphore();
    }
    postsync();
}

void foNode::sync() {
    signalCount = 0;
    
    waitCount = tempWaitCount;
    tempWaitCount = 0;
    
    for(uint32_t i = 0; i < signalNodes.size(); i++) {
        vk::Semaphore sem = semaphores->getSemaphore(semaphoreHandles[i]);
        if(signalNodes[i]->prepareSignal(this, sem)) {
            signalCount++;
            signalSemaphores.push_back(sem);
        }
    }
    
}

void foNode::postsync() {
    waitSemaphores.clear();
    waitStages.clear();
    signalSemaphores.clear();
}

void foNode::reset() {
    for(foNode* signaler : waitNodes) {
        signaler->stopSignal(this);
    }
    for(foNode* waiter : signalNodes) {
        waiter->stopWait(this);
    }
    waitNodes.clear();
    waitNodeStages.clear();
    signalNodes.clear();
}

bool foNode::prepareSignal(foNode *signaler, vk::Semaphore sem) {
    if(isActive()) {
        tempWaitCount++;
        waitSemaphores.push_back(sem);
        waitStages.push_back(waitNodeStages[signaler]);
        return true;
    } else {
        return false;
    }
}

bool foNode::prepareSignal(vk::PipelineStageFlags stages, vk::Semaphore sem) {
    if(isActive()) {
        tempWaitCount++;
        waitSemaphores.push_back(sem);
        waitStages.push_back(stages);
        return true;
    } else {
        return false;
    }
}

bool foNode::prepareSignal(foNode* signaler, vk::PipelineStageFlags stages, vk::Semaphore sem) {
    if(isActive()) {
        tempWaitCount++;
        waitSemaphores.push_back(sem);
        waitStages.push_back(stages);
        signaler->signalCount++;
        signaler->signalSemaphores.push_back(sem);
        return true;
    } else {
        return false;
    }
}
