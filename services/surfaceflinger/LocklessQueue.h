/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <atomic>
#include <optional>

// Single consumer multi producer queue. We can understand the two operations independently to see
// why they are without race condition.
//
// push is responsible for maintaining a linked list stored in mPush, and called from multiple
// threads without lock. We can see that if two threads never observe the same value from
// mPush.load, it just functions as a normal linked list. In the case where two threads observe the
// same value, one of them has to execute the compare_exchange first. The one that doesn't execute
// the compare exchange first, will receive false from compare_exchange. previousHead is updated (by
// compare_exchange) to the most recent value of mPush, and we try again. It's relatively clear to
// see that the process can repeat with an arbitrary number of threads.
//
// Pop is much simpler. If mPop is empty (as it begins) it atomically exchanges
// the entire push list with null. This is safe, since the only other reader (push)
// of mPush will retry if it changes in between it's read and atomic compare. We
// then store the list and pop one element.
//
// If we already had something in the pop list we just pop directly.
template <typename T>
class LocklessQueue {
public:
    bool isEmpty() { return (mPush.load() == nullptr) && (mPop.load() == nullptr); }

    void push(T value) {
        Entry* entry = new Entry(std::move(value));
        Entry* previousHead = mPush.load(/*std::memory_order_relaxed*/);
        do {
            entry->mNext = previousHead;
        } while (!mPush.compare_exchange_weak(previousHead, entry)); /*std::memory_order_release*/
    }

    std::optional<T> pop() {
        Entry* popped = mPop.load(/*std::memory_order_acquire*/);
        if (popped) {
            // Single consumer so this is fine
            mPop.store(popped->mNext /* , std::memory_order_release */);
            auto value = std::move(popped->mValue);
            delete popped;
            return value;
        } else {
            Entry* grabbedList = mPush.exchange(nullptr /* , std::memory_order_acquire */);
            if (!grabbedList) return std::nullopt;
            // Reverse the list
            while (grabbedList->mNext) {
                Entry* next = grabbedList->mNext;
                grabbedList->mNext = popped;
                popped = grabbedList;
                grabbedList = next;
            }
            mPop.store(popped /* , std::memory_order_release */);
            auto value = std::move(grabbedList->mValue);
            delete grabbedList;
            return value;
        }
    }

private:
    class Entry {
    public:
        T mValue;
        std::atomic<Entry*> mNext;
        Entry(T value) : mValue(value) {}
    };
    std::atomic<Entry*> mPush = nullptr;
    std::atomic<Entry*> mPop = nullptr;
};
