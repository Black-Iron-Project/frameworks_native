/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VIBRATORSERVICE_UNITTEST_UTIL_H_
#define VIBRATORSERVICE_UNITTEST_UTIL_H_

#include <aidl/android/hardware/vibrator/IVibrator.h>

#include <vibratorservice/VibratorHalWrapper.h>

namespace android {

namespace vibrator {

using aidl::android::hardware::vibrator::ActivePwle;
using aidl::android::hardware::vibrator::Braking;
using aidl::android::hardware::vibrator::BrakingPwle;
using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::PrimitivePwle;

// -------------------------------------------------------------------------------------------------

class TestFactory {
public:
    static CompositeEffect createCompositeEffect(CompositePrimitive primitive,
                                                 std::chrono::milliseconds delay, float scale) {
        CompositeEffect effect;
        effect.primitive = primitive;
        effect.delayMs = delay.count();
        effect.scale = scale;
        return effect;
    }

    static PrimitivePwle createActivePwle(float startAmplitude, float startFrequency,
                                          float endAmplitude, float endFrequency,
                                          std::chrono::milliseconds duration) {
        ActivePwle pwle;
        pwle.startAmplitude = startAmplitude;
        pwle.endAmplitude = endAmplitude;
        pwle.startFrequency = startFrequency;
        pwle.endFrequency = endFrequency;
        pwle.duration = duration.count();
        return pwle;
    }

    static PrimitivePwle createBrakingPwle(Braking braking, std::chrono::milliseconds duration) {
        BrakingPwle pwle;
        pwle.braking = braking;
        pwle.duration = duration.count();
        return pwle;
    }

    static std::function<void()> createCountingCallback(int32_t* counter) {
        return [counter]() { *counter += 1; };
    }

private:
    TestFactory() = delete;
    ~TestFactory() = delete;
};

class TestCounter {
public:
    TestCounter(int32_t init = 0) : mMutex(), mCondVar(), mCount(init) {}

    int32_t get() {
        std::lock_guard<std::mutex> lock(mMutex);
        return mCount;
    }

    void increment() {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mCount += 1;
        }
        mCondVar.notify_all();
    }

    void tryWaitUntilCountIsAtLeast(int32_t count, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mMutex);
        mCondVar.wait_for(lock, timeout, [&] { return mCount >= count; });
    }

private:
    std::mutex mMutex;
    std::condition_variable mCondVar;
    int32_t mCount GUARDED_BY(mMutex);
};

// -------------------------------------------------------------------------------------------------

} // namespace vibrator

} // namespace android

#endif // VIBRATORSERVICE_UNITTEST_UTIL_H_
