/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "audio_zone_unit_test_base.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

class AudioZoneUnitTest : public AudioZoneUnitTestPreset {
};

static std::shared_ptr<AudioZone> CreateZone(const std::string &name)
{
    std::shared_ptr<AudioZoneClientManager> manager = nullptr;
    AudioZoneContext context;
    manager = std::make_shared<AudioZoneClientManager>(nullptr);
    EXPECT_NE(manager, nullptr);
    std::shared_ptr<AudioZone> zone = std::make_shared<AudioZone>(
        manager, name, context);
    EXPECT_NE(zone, nullptr);
    return zone;
}

static void ClearZone()
{
    auto zoneList = AudioZoneService::GetInstance().GetAllAudioZone();
    for (auto zone : zoneList) {
        AudioZoneService::GetInstance().ReleaseAudioZone(zone->zoneId_);
    }
}

/**
 * @tc.name  : Test AudioZone.
 * @tc.number: AudioZone_001
 * @tc.desc  : Test create audio zone.
 */
HWTEST_F(AudioZoneUnitTest, AudioZone_001, TestSize.Level1)
{
    ClearZone();
    auto zone = CreateZone("TestZone");
    auto desc = zone->GetDescriptor();
    EXPECT_NE(desc, nullptr);
    EXPECT_EQ(desc->zoneId_, zone->GetId());
    EXPECT_EQ(desc->name_, "TestZone");
}

/**
 * @tc.name  : Test AudioZone.
 * @tc.number: AudioZone_002
 * @tc.desc  : Test bind key to audio zone
 */
HWTEST_F(AudioZoneUnitTest, AudioZone_002, TestSize.Level1)
{
    ClearZone();
    auto zone = CreateZone("TestZone");
    zone->BindByKey(AudioZoneBindKey(1, "d1"));
    zone->BindByKey(AudioZoneBindKey(1));
    zone->BindByKey(AudioZoneBindKey(2, "d1", "test"));
    zone->BindByKey(AudioZoneBindKey(2, "", "test"));
    zone->BindByKey(AudioZoneBindKey(2, "", "temp"));
    zone->RemoveKey(AudioZoneBindKey(2, "", "temp"));
    EXPECT_EQ(zone->IsContainKey(AudioZoneBindKey(2, "", "temp")), false);
    EXPECT_EQ(zone->IsContainKey(AudioZoneBindKey(2, "", "test")), true);
    EXPECT_EQ(zone->IsContainKey(AudioZoneBindKey(2, "", "test")), false);
    EXPECT_EQ(zone->IsContainKey(AudioZoneBindKey(1)), true);
    EXPECT_EQ(zone->IsContainKey(AudioZoneBindKey(1, "")), true);
}

/**
 * @tc.name  : Test AudioZone.
 * @tc.number: AudioZone_003
 * @tc.desc  : Test bind key to audio zone
 */
HWTEST_F(AudioZoneUnitTest, AudioZone_003, TestSize.Level1)
{
    ClearZone();
    auto zone = CreateZone("TestZone");
    auto device1 = CreateDevice(DEVICE_TYPE_SPEAKER, OUTPUT_DEVICE, "", "LocalDevice");
    auto device2 = CreateDevice(DEVICE_TYPE_MIC, INPUT_DEVICE, "", "LocalDevice");
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> devices;
    devices.push_back(device1);
    devices.push_back(device2);
    EXPECT_EQ(zone->AddDeviceDescriptor(devices), 0);
    EXPECT_EQ(zone->IsDeviceConnect(device1), true);
    EXPECT_EQ(zone->IsDeviceConnect(device2), true);

    EXPECT_EQ(zone->DisableDeviceDescriptor(device2), 0);
    EXPECT_EQ(zone->IsDeviceConnect(device2), false);
    EXPECT_EQ(zone->EnableDeviceDescriptor(device2), 0);
    EXPECT_EQ(zone->IsDeviceConnect(device2), true);

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> tempDevices;
    tempDevices.push_back(device2);
    EXPECT_EQ(zone->RemoveDeviceDescriptor(tempDevices), 0);
    EXPECT_EQ(zone->IsDeviceConnect(device2), false);

    EXPECT_EQ(zone->AddDeviceDescriptor(devices), 0);
    auto fechOutputDevice = zone->FetchOutputDevices(STREAM_USAGE_MUSIC, 0, ROUTER_TYPE_DEFAULT);
    auto fechInputDevice = zone->FetchInputDevice(SOURCE_TYPE_MIC, 0);
    EXPECT_EQ(fechOutputDevice.size(), 1);
    EXPECT_EQ(fechOutputDevice[0]->IsSameDeviceDesc(device1), true);
    EXPECT_NE(fechInputDevice, nullptr);
    EXPECT_EQ(fechInputDevice->IsSameDeviceDesc(device2), true);
}

/**
 * @tc.name  : Test AudioZone.
 * @tc.number: AudioZone_004
 * @tc.desc  : Test release audio zone
 */
HWTEST_F(AudioZoneUnitTest, AudioZone_004, TestSize.Level1)
{
    ClearZone();
    AudioZoneContext context;
    auto zoneId1 = AudioZoneService::GetInstance().CreateAudioZone("TestZone1", context);
    EXPECT_NE(zoneId1, 0);
    auto zoneId2 = AudioZoneService::GetInstance().CreateAudioZone("TestZone2", context);
    EXPECT_NE(zoneId2, 0);

    auto zoneList = AudioZoneService::GetInstance().GetAllAudioZone();
    EXPECT_EQ(zoneList.size(), 2);
    EXPECT_NE(AudioZoneService::GetInstance().GetAudioZone(zoneId1), nullptr);
    EXPECT_NE(AudioZoneService::GetInstance().GetAudioZone(zoneId2), nullptr);
    EXPECT_EQ(AudioZoneService::GetInstance().GetAudioZone(zoneId1 + zoneId2), nullptr);
    AudioZoneService::GetInstance().ReleaseAudioZone(zoneId1);
    zoneList = AudioZoneService::GetInstance().GetAllAudioZone();
    EXPECT_EQ(zoneList.size(), 1);

    AudioZoneService::GetInstance().ReleaseAudioZone(zoneId1 + zoneId2);
    zoneList = AudioZoneService::GetInstance().GetAllAudioZone();
    EXPECT_EQ(zoneList.size(), 1);

    AudioZoneService::GetInstance().ReleaseAudioZone(zoneId2);
    zoneList = AudioZoneService::GetInstance().GetAllAudioZone();
    EXPECT_EQ(zoneList.size(), 0);
}

/**
 * @tc.name  : Test AudioZone.
 * @tc.number: AudioZone_005
 * @tc.desc  : Test audio zone report
 */
HWTEST_F(AudioZoneUnitTest, AudioZone_005, TestSize.Level1)
{
    ClearZone();
    AudioZoneContext context;
    EXPECT_EQ(AudioZoneService::GetInstance().EnableAudioZoneReport(TEST_PID_1000, true), 0);
    auto client = RegisterTestClient(TEST_PID_1000);
    EXPECT_NE(client, nullptr);
    auto zoneId1 = AudioZoneService::GetInstance().CreateAudioZone("TestZone1", context);
    EXPECT_NE(zoneId1, 0);
}

/**
 * @tc.name  : Test AudioZone.
 * @tc.number: AudioZone_006
 * @tc.desc  : Test audio zone change report
 */
HWTEST_F(AudioZoneUnitTest, AudioZone_006, TestSize.Level1)
{
    ClearZone();
    AudioZoneContext context;
    EXPECT_NE(AudioZoneService::GetInstance().EnableAudioZoneChangeReport(TEST_PID_1000, 1, true), 0);
    auto client = RegisterTestClient(TEST_PID_1000);
    EXPECT_NE(client, nullptr);
    auto zoneId1 = AudioZoneService::GetInstance().CreateAudioZone("TestZone1", context);
    EXPECT_NE(zoneId1, 0);
    EXPECT_EQ(AudioZoneService::GetInstance().EnableAudioZoneChangeReport(TEST_PID_1000, zoneId1, true), 0);

    EXPECT_EQ(AudioZoneService::GetInstance().AddUidToAudioZone(zoneId1, TEST_PID_1000), 0);
    EXPECT_NE(client->recvEvent_.type, AUDIO_ZONE_CHANGE_EVENT);
}
} // namespace AudioStandard
} // namespace OHOS