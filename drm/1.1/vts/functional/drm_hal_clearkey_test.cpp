/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "drm_hal_clearkey_test@1.1"

#include <android/hardware/drm/1.1/ICryptoFactory.h>
#include <android/hardware/drm/1.0/ICryptoPlugin.h>
#include <android/hardware/drm/1.1/IDrmFactory.h>
#include <android/hardware/drm/1.0/IDrmPlugin.h>
#include <android/hardware/drm/1.1/IDrmPlugin.h>
#include <android/hardware/drm/1.0/types.h>
#include <android/hardware/drm/1.1/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidlmemory/mapping.h>
#include <log/log.h>
#include <memory>
#include <openssl/aes.h>
#include <random>

#include "VtsHalHidlTargetTestBase.h"
#include "VtsHalHidlTargetTestEnvBase.h"

namespace drm = ::android::hardware::drm;
using ::android::hardware::drm::V1_0::BufferType;
using ::android::hardware::drm::V1_0::DestinationBuffer;
using ::android::hardware::drm::V1_0::ICryptoPlugin;
using ::android::hardware::drm::V1_0::KeyedVector;
using ::android::hardware::drm::V1_0::KeyValue;
using ::android::hardware::drm::V1_0::KeyRequestType;
using ::android::hardware::drm::V1_0::KeyType;
using ::android::hardware::drm::V1_0::Mode;
using ::android::hardware::drm::V1_0::Pattern;
using ::android::hardware::drm::V1_0::SecureStop;
using ::android::hardware::drm::V1_0::SecureStopId;
using ::android::hardware::drm::V1_0::SessionId;
using ::android::hardware::drm::V1_0::SharedBuffer;
using ::android::hardware::drm::V1_0::Status;
using ::android::hardware::drm::V1_0::SubSample;
using ::android::hardware::drm::V1_0::SubSample;

using ::android::hardware::drm::V1_1::HdcpLevel;
using ::android::hardware::drm::V1_1::ICryptoFactory;
using ::android::hardware::drm::V1_1::IDrmFactory;
using ::android::hardware::drm::V1_1::IDrmPlugin;
using ::android::hardware::drm::V1_1::SecurityLevel;
using ::android::hardware::drm::V1_1::SecurityLevel;

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::sp;

using std::string;
using std::unique_ptr;
using std::random_device;
using std::map;
using std::mt19937;
using std::vector;

/**
 * These clearkey tests use white box knowledge of the legacy clearkey
 * plugin to verify that the HIDL HAL services and interfaces are working.
 * It is not intended to verify any vendor's HAL implementation. If you
 * are looking for vendor HAL tests, see drm_hal_vendor_test.cpp
 */
#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

// To be used in mpd to specify drm scheme for players
static const uint8_t kClearKeyUUID[16] = {
    0xE2, 0x71, 0x9D, 0x58, 0xA9, 0x85, 0xB3, 0xC9,
    0x78, 0x1A, 0xB0, 0x30, 0xAF, 0x78, 0xD3, 0x0E};

// Test environment for drm
class DrmHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static DrmHidlEnvironment* Instance() {
        static DrmHidlEnvironment* instance = new DrmHidlEnvironment;
        return instance;
    }

    virtual void HidlSetUp() override { ALOGI("SetUp DrmHidlEnvironment"); }

    virtual void HidlTearDown() override { ALOGI("TearDown DrmHidlEnvironment"); }

   private:
    DrmHidlEnvironment() {}

    GTEST_DISALLOW_COPY_AND_ASSIGN_(DrmHidlEnvironment);
};


class DrmHalClearkeyTest : public ::testing::VtsHalHidlTargetTestBase {
public:
    virtual void SetUp() override {
        const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();

        ALOGD("DrmHalClearkeyTest: Running test %s.%s", test_info->test_case_name(),
                test_info->name());

        auto manager = android::hardware::defaultServiceManager();
        ASSERT_NE(nullptr, manager.get());
        manager->listByInterface(IDrmFactory::descriptor,
                [&](const hidl_vec<hidl_string> &registered) {
                    for (const auto &instance : registered) {
                        sp<IDrmFactory> drmFactory =
                                ::testing::VtsHalHidlTargetTestBase::getService<IDrmFactory>(instance);
                        drmPlugin = createDrmPlugin(drmFactory);
                        if (drmPlugin != nullptr) {
                            break;
                        }
                    }
                }
            );

        manager->listByInterface(ICryptoFactory::descriptor,
                [&](const hidl_vec<hidl_string> &registered) {
                    for (const auto &instance : registered) {
                        sp<ICryptoFactory> cryptoFactory =
                                ::testing::VtsHalHidlTargetTestBase::getService<ICryptoFactory>(instance);
                        cryptoPlugin = createCryptoPlugin(cryptoFactory);
                        if (cryptoPlugin != nullptr) {
                            break;
                        }
                    }
                }
            );

        ASSERT_NE(nullptr, drmPlugin.get()) << "Can't find clearkey drm@1.1 plugin";
        ASSERT_NE(nullptr, cryptoPlugin.get()) << "Can't find clearkey crypto@1.1 plugin";
    }


    virtual void TearDown() override {}

    SessionId openSession();
    void closeSession(const SessionId& sessionId);
    hidl_vec<uint8_t> loadKeys(const SessionId& sessionId, const KeyType& type);
    sp<IMemory> getDecryptMemory(size_t size, size_t index);

  private:
    sp<IDrmPlugin> createDrmPlugin(sp<IDrmFactory> drmFactory) {
        if (drmFactory == nullptr) {
            return nullptr;
        }
        sp<IDrmPlugin> plugin = nullptr;
        auto res = drmFactory->createPlugin(
                kClearKeyUUID, "",
                        [&](Status status, const sp<drm::V1_0::IDrmPlugin>& pluginV1_0) {
                    EXPECT_EQ(Status::OK, status);
                    plugin = IDrmPlugin::castFrom(pluginV1_0);
                });

        if (!res.isOk()) {
            ALOGE("createDrmPlugin remote call failed");
        }
        return plugin;
    }

    sp<ICryptoPlugin> createCryptoPlugin(sp<ICryptoFactory> cryptoFactory) {
        if (cryptoFactory == nullptr) {
            return nullptr;
        }
        sp<ICryptoPlugin> plugin = nullptr;
        hidl_vec<uint8_t> initVec;
        auto res = cryptoFactory->createPlugin(
                kClearKeyUUID, initVec,
                        [&](Status status, const sp<drm::V1_0::ICryptoPlugin>& pluginV1_0) {
                    EXPECT_EQ(Status::OK, status);
                    plugin = pluginV1_0;
                });
        if (!res.isOk()) {
            ALOGE("createCryptoPlugin remote call failed");
        }
        return plugin;
    }

protected:
    sp<IDrmPlugin> drmPlugin;
    sp<ICryptoPlugin> cryptoPlugin;
};



/**
 * Helper method to open a session and verify that a non-empty
 * session ID is returned
 */
SessionId DrmHalClearkeyTest::openSession() {
    SessionId sessionId;

    auto res = drmPlugin->openSession(
            [&sessionId](Status status, const SessionId& id) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_NE(0u, id.size());
                sessionId = id;
            });
    EXPECT_OK(res);
    return sessionId;
}

/**
 * Helper method to close a session
 */
void DrmHalClearkeyTest::closeSession(const SessionId& sessionId) {
    EXPECT_TRUE(drmPlugin->closeSession(sessionId).isOk());
}

/**
 * Test that the plugin returns valid connected and max HDCP levels
 */
TEST_F(DrmHalClearkeyTest, GetHdcpLevels) {
    auto res = drmPlugin->getHdcpLevels(
            [&](Status status, const HdcpLevel &connectedLevel,
                const HdcpLevel &maxLevel) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_GE(connectedLevel, HdcpLevel::HDCP_NONE);
                EXPECT_LE(maxLevel, HdcpLevel::HDCP_NO_OUTPUT);
            });
    EXPECT_OK(res);
}

/**
 * Test that the plugin returns valid open and max session counts
 */
TEST_F(DrmHalClearkeyTest, GetSessionCounts) {
    auto res = drmPlugin->getNumberOfSessions(
            [&](Status status, uint32_t currentSessions,
                    uint32_t maxSessions) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_GT(maxSessions, (uint32_t)0);
                EXPECT_GE(currentSessions, (uint32_t)0);
                EXPECT_LE(currentSessions, maxSessions);
            });
    EXPECT_OK(res);
}

/**
 * Test that the plugin returns a valid security level for
 * a valid session
 */
TEST_F(DrmHalClearkeyTest, GetSecurityLevel) {
    SessionId session = openSession();
    auto res = drmPlugin->getSecurityLevel(session,
            [&](Status status, SecurityLevel level) {
                EXPECT_EQ(Status::OK, status);
                EXPECT_GE(level, SecurityLevel::SW_SECURE_CRYPTO);
                EXPECT_LE(level, SecurityLevel::HW_SECURE_ALL);
            });
    EXPECT_OK(res);
    closeSession(session);
}

/**
 * Test that the plugin returns the documented error
 * when requesting the security level for an invalid sessionId
 */
TEST_F(DrmHalClearkeyTest, GetSecurityLevelInvalidSessionId) {
    SessionId session;
    auto res = drmPlugin->getSecurityLevel(session,
            [&](Status status, SecurityLevel /*level*/) {
                EXPECT_EQ(Status::BAD_VALUE, status);
            });
    EXPECT_OK(res);
}

/**
 * Test that setting all valid security levels on a valid sessionId
 * is supported
 */
TEST_F(DrmHalClearkeyTest, SetSecurityLevel) {
    SessionId session = openSession();
    for (uint32_t level = static_cast<uint32_t>(SecurityLevel::SW_SECURE_CRYPTO);
         level <= static_cast<uint32_t>(SecurityLevel::HW_SECURE_ALL); level++) {
        EXPECT_EQ(Status::OK, drmPlugin->setSecurityLevel(session, static_cast<SecurityLevel>(level)));

        // check that the level got set
        auto res = drmPlugin->getSecurityLevel(session,
                [&](Status status, SecurityLevel readLevel) {
                    EXPECT_EQ(Status::OK, status);
                    EXPECT_EQ(level, static_cast<uint32_t>(readLevel));
                });
        EXPECT_OK(res);
    }
    closeSession(session);
}

/**
 * Test that setting an invalid security level on a valid
 * sessionId is prohibited with the documented error code.
 */
TEST_F(DrmHalClearkeyTest, SetInvalidSecurityLevel) {
    SessionId session = openSession();
    SecurityLevel level = static_cast<SecurityLevel>(
            static_cast<uint32_t>(SecurityLevel::HW_SECURE_ALL) + 1);
    Status status = drmPlugin->setSecurityLevel(session, level);
    EXPECT_EQ(Status::BAD_VALUE, status);
    closeSession(session);
}

/**
 * Test that attempting to set security level on an invalid
 * (empty) sessionId is prohibited with the documented error
 * code.
 */
TEST_F(DrmHalClearkeyTest, SetSecurityLevelInvalidSessionId) {
    SessionId session;
    SecurityLevel level = SecurityLevel::SW_SECURE_CRYPTO;
    EXPECT_EQ(Status::BAD_VALUE, drmPlugin->setSecurityLevel(session, level));
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(DrmHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    DrmHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
