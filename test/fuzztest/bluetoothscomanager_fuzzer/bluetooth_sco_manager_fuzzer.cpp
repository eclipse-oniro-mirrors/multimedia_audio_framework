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

#include <iostream>
#include "bluetooth_device_manager.h"
#include "bluetooth_device_utils.h"
#include "audio_info.h"
#include "audio_engine_log.h"
#include "idevice_status_observer.h"
#include "../fuzz_utils.h"
#include "bluetooth_sco_manager.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;
using namespace Bluetooth;

FuzzUtils &g_fuzzUtils = FuzzUtils::GetInstance();
const size_t FUZZ_INPUT_SIZE_THRESHOLD = 10;
const int32_t TRANSPORT = 2;

typedef void (*TestFuncs)();

void UpdateScoStateWhenDisconnectedFuzzTest()
{
    BluetoothScoManager scoManager;
    HfpScoConnectState scoState = HfpScoConnectState::SCO_CONNECTED;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    int reason = g_fuzzUtils.GetData<int>();
    scoManager.UpdateScoStateWhenDisconnected(scoState, device, reason);
}

void UpdateScoStateWhenConnectedFuzzTest()
{
    BluetoothScoManager scoManager;
    HfpScoConnectState scoState = HfpScoConnectState::SCO_DISCONNECTED;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    int reason = g_fuzzUtils.GetData<int>();
    scoManager.UpdateScoStateWhenConnected(scoState, device, reason);
    scoState = HfpScoConnectState::SCO_CONNECTED;
    scoManager.UpdateScoStateWhenConnected(scoState, device, reason);
}

void UpdateScoStateWhenConnectingFuzzTest()
{
    BluetoothScoManager scoManager;
    HfpScoConnectState scoState = HfpScoConnectState::SCO_DISCONNECTED;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    int reason = g_fuzzUtils.GetData<int>();
    scoManager.UpdateScoStateWhenConnecting(scoState, device, reason);
}

void UpdateScoStateWhenDisconnectingFuzzTest()
{
    BluetoothScoManager scoManager;
    HfpScoConnectState scoState = HfpScoConnectState::SCO_DISCONNECTED;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    int reason = g_fuzzUtils.GetData<int>();
    scoManager.UpdateScoStateWhenDisconnecting(scoState, device, reason);
}

void WriteScoStateFaultEventFuzzTest()
{
    BluetoothScoManager scoManager;
    HfpScoConnectState scoState = HfpScoConnectState::SCO_DISCONNECTED;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    int reason = g_fuzzUtils.GetData<int>();
    scoManager.WriteScoStateFaultEvent(scoState, device, reason);
}

void ForceUpdateScoCategoryFuzzTest()
{
    BluetoothScoManager scoManager;
    scoManager.ForceUpdateScoCategory();
}

void ProcCacheRequestFuzzTest()
{
    BluetoothScoManager scoManager;
    scoManager.ProcCacheRequest();
}

void HandleScoConnectFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    ScoCategory category = ScoCategory::SCO_DEFAULT;
    scoManager.HandleScoConnect(category, device);
}

void HandleScoDisconnectFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    scoManager.currentScoState_= g_fuzzUtils.GetData<AudioScoState>();
    scoManager.HandleScoDisconnect(device);
}

void HandleScoConnectNoLockFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    ScoCategory category = ScoCategory::SCO_DEFAULT;
    scoManager.HandleScoConnectNoLock(category, device);
    scoManager.currentScoState_= g_fuzzUtils.GetData<AudioScoState>();
    scoManager.HandleScoConnectNoLock(category, device);
}

void ProcConnectReqWhenDisconnectedFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    ScoCategory category = ScoCategory::SCO_DEFAULT;
    scoManager.ProcConnectReqWhenDisconnected(category, device);
}

void ProcConnectReqWhenConnectedFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    ScoCategory category = ScoCategory::SCO_DEFAULT;
    scoManager.ProcConnectReqWhenConnected(category, device);
}

void ProcConnectReqWhenConnectingFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    ScoCategory category = ScoCategory::SCO_DEFAULT;
    scoManager.ProcConnectReqWhenConnecting(category, device);
}

void IsNeedSwitchScoCategoryFuzzTest()
{
    BluetoothScoManager scoManager;
    ScoCategory category = ScoCategory::SCO_DEFAULT;
    scoManager.IsNeedSwitchScoCategory(category);
    scoManager.currentScoCategory_ = ScoCategory::SCO_VIRTUAL;
    scoManager.IsNeedSwitchScoCategory(category);
}

void ProcDisconnectReqWhenConnectedFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    scoManager.ProcDisconnectReqWhenConnected(device);
}

void ProcDisconnectReqWhenConnectingFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    scoManager.ProcDisconnectReqWhenConnecting(device);
}

void IsSameHfpDeviceFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device1("00:11:22:33:44:55", TRANSPORT);
    BluetoothRemoteDevice device2("00:11:22:33:44:55", TRANSPORT);
    scoManager.IsSameHfpDevice(device1, device2);
}

void ConnectAndDisconnectScoFuzzTest()
{
    BluetoothScoManager scoManager;
    ScoCategory category = ScoCategory::SCO_DEFAULT;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    scoManager.ConnectSco(category, device);
    scoManager.DisconnectSco(category, device);
    category = ScoCategory::SCO_RECOGNITION;
    scoManager.ConnectSco(category, device);
    scoManager.DisconnectSco(category, device);
}

void TryRestoreHfpDeviceFuzzTest()
{
    BluetoothScoManager scoManager;
    ScoCategory category = ScoCategory::SCO_DEFAULT;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    scoManager.TryRestoreHfpDevice(category, device);
}

void DisconnectScoReliableFuzzTest()
{
    BluetoothScoManager scoManager;
    ScoCategory category = ScoCategory::SCO_DEFAULT;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    scoManager.DisconnectScoReliable(category, device);
}

void GetAudioScoStateFuzzTest()
{
    BluetoothScoManager scoManager;
    scoManager.GetAudioScoState();
}

void GetAudioScoCategoryFuzzTest()
{
    BluetoothScoManager scoManager;
    scoManager.GetAudioScoCategory();
}

void ResetScoStateFuzzTest()
{
    BluetoothScoManager scoManager;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    scoManager.ResetScoState(device);
}

void SetAudioScoStateFuzzTest()
{
    BluetoothScoManager scoManager;
    AudioScoState state = AudioScoState::CONNECTING;
    scoManager.SetAudioScoState(state);
}

void OnScoStateTimeOutFuzzTest()
{
    BluetoothScoManager scoManager;
    scoManager.OnScoStateTimeOut();
}

void GetAudioScoDeviceFuzzTest()
{
    BluetoothScoManager scoManager;
    scoManager.GetAudioScoDevice();
}

void UpdateScoStateFuzzTest()
{
    BluetoothScoManager scoManager;
    HfpScoConnectState scoState = HfpScoConnectState::SCO_DISCONNECTED;
    BluetoothRemoteDevice device("00:11:22:33:44:55", TRANSPORT);
    scoManager.currentScoDevice_ = device;
    int reason = g_fuzzUtils.GetData<int>();
    scoManager.currentScoState_= g_fuzzUtils.GetData<AudioScoState>();
    scoManager.UpdateScoState(scoState, device, reason);
}

vector<TestFuncs> g_testFuncs = {
    UpdateScoStateWhenDisconnectedFuzzTest,
    UpdateScoStateWhenConnectedFuzzTest,
    UpdateScoStateWhenConnectingFuzzTest,
    UpdateScoStateWhenDisconnectingFuzzTest,
    WriteScoStateFaultEventFuzzTest,
    ForceUpdateScoCategoryFuzzTest,
    ProcCacheRequestFuzzTest,
    HandleScoConnectFuzzTest,
    HandleScoDisconnectFuzzTest,
    HandleScoConnectNoLockFuzzTest,
    ProcConnectReqWhenDisconnectedFuzzTest,
    ProcConnectReqWhenConnectedFuzzTest,
    ProcConnectReqWhenConnectingFuzzTest,
    IsNeedSwitchScoCategoryFuzzTest,
    ProcDisconnectReqWhenConnectedFuzzTest,
    ProcDisconnectReqWhenConnectingFuzzTest,
    IsSameHfpDeviceFuzzTest,
    ConnectAndDisconnectScoFuzzTest,
    TryRestoreHfpDeviceFuzzTest,
    DisconnectScoReliableFuzzTest,
    GetAudioScoStateFuzzTest,
    GetAudioScoCategoryFuzzTest,
    ResetScoStateFuzzTest,
    SetAudioScoStateFuzzTest,
    OnScoStateTimeOutFuzzTest,
    GetAudioScoDeviceFuzzTest,
    UpdateScoStateFuzzTest,
};
} // namespace AudioStandard
} // namesapce OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < OHOS::AudioStandard::FUZZ_INPUT_SIZE_THRESHOLD) {
        return 0;
    }

    OHOS::AudioStandard::g_fuzzUtils.fuzzTest(data, size, OHOS::AudioStandard::g_testFuncs);
    return 0;
}
