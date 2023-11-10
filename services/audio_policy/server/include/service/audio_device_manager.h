/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef ST_AUDIO_DEVICE_MANAGER_H
#define ST_AUDIO_DEVICE_MANAGER_H

#include <list>
#include <string>
#include <memory>
#include <unordered_map>
#include "audio_info.h"
#include "audio_device_info.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;

typedef function<bool(const std::unique_ptr<AudioDeviceDescriptor> &desc)> IsPresentFunc;

class AudioDeviceManager {
public:
    static AudioDeviceManager& GetAudioDeviceManager()
    {
        static AudioDeviceManager audioDeviceManager;
        return audioDeviceManager;
    }

    void AddNewDevice(const sptr<AudioDeviceDescriptor> &devDesc);
    void RemoveNewDevice(const sptr<AudioDeviceDescriptor> &devDesc);
    void OnXmlParsingCompleted(const unordered_map<AudioDevicePrivacyType, list<DevicePrivacyInfo>> &xmlData);
    int32_t GetDeviceUsageFromType(const DeviceType devType) const;
    void ParseDeviceXml();

    vector<unique_ptr<AudioDeviceDescriptor>> GetRemoteRenderDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetRemoteCaptureDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCommRenderPrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCommRenderPublicDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCommCapturePrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCommCapturePublicDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetMediaRenderPrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetMediaRenderPublicDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetMediaCapturePrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetMediaCapturePublicDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCapturePrivacyDevices();
    vector<unique_ptr<AudioDeviceDescriptor>> GetCapturePublicDevices();
    unique_ptr<AudioDeviceDescriptor> GetCommRenderDefaultDevices();
    unique_ptr<AudioDeviceDescriptor> GetRenderDefaultDevices();
    unique_ptr<AudioDeviceDescriptor> GetCaptureDefaultDevices();
    unordered_map<AudioDevicePrivacyType, list<DevicePrivacyInfo>> GetDevicePrivacyMaps();
    vector<unique_ptr<AudioDeviceDescriptor>> GetAvailableDevicesByUsage(AudioDeviceUsage usage);
    void GetAvailableDevicesWithUsage(const AudioDeviceUsage usage,
        const list<DevicePrivacyInfo> &deviceInfos, const sptr<AudioDeviceDescriptor> &dev,
        std::vector<unique_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors);

private:
    AudioDeviceManager() {};
    ~AudioDeviceManager() {};
    bool DeviceAttrMatch(const shared_ptr<AudioDeviceDescriptor> &devDesc, AudioDevicePrivacyType &privacyType,
        DeviceRole &devRole, DeviceUsage &devUsage);

    void FillArrayWhenDeviceAttrMatch(const shared_ptr<AudioDeviceDescriptor> &devDesc,
        AudioDevicePrivacyType privacyType, DeviceRole devRole, DeviceUsage devUsage, string logName,
        vector<shared_ptr<AudioDeviceDescriptor>> &descArray);

    void RemoveMatchDeviceInArray(const AudioDeviceDescriptor &devDesc, string logName,
        vector<shared_ptr<AudioDeviceDescriptor>> &descArray);

    void MakePairedDeviceDescriptor(const shared_ptr<AudioDeviceDescriptor> &devDesc);
    void MakePairedDeviceDescriptor(const shared_ptr<AudioDeviceDescriptor> &devDesc, DeviceRole devRole);
    void UpdateConnectedDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc, bool isConnected);
    void AddConnectedDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc);
    void RemoveConnectedDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc);
    void AddRemoteRenderDev(const shared_ptr<AudioDeviceDescriptor> &devDesc);
    void AddRemoteCaptureDev(const shared_ptr<AudioDeviceDescriptor> &devDesc);
    void AddDefaultDevices(const sptr<AudioDeviceDescriptor> &devDesc);

    void UpdateDeviceInfo(shared_ptr<AudioDeviceDescriptor> &deviceDesc);
    void AddCommunicationDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc);
    void AddMediaDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc);
    void AddCaptureDevices(const shared_ptr<AudioDeviceDescriptor> &devDesc);
    void HandleScoWithDefaultCategory(const shared_ptr<AudioDeviceDescriptor> &devDesc);
    void AddAvailableDevicesByUsage(const AudioDeviceUsage usage,
        const DevicePrivacyInfo &deviceInfo, const sptr<AudioDeviceDescriptor> &dev,
        std::vector<unique_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors);
    void GetDefaultAvailableDevicesByUsage(AudioDeviceUsage usage,
        vector<unique_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors);

    list<DevicePrivacyInfo> privacyDeviceList_;
    list<DevicePrivacyInfo> publicDeviceList_;

    vector<shared_ptr<AudioDeviceDescriptor>> remoteRenderDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> remoteCaptureDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> commRenderPrivacyDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> commRenderPublicDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> commCapturePrivacyDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> commCapturePublicDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> mediaRenderPrivacyDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> mediaRenderPublicDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> mediaCapturePrivacyDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> mediaCapturePublicDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> capturePrivacyDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> capturePublicDevices_;
    vector<shared_ptr<AudioDeviceDescriptor>> connectedDevices_;
    unordered_map<AudioDevicePrivacyType, list<DevicePrivacyInfo>> devicePrivacyMaps_ = {};
    sptr<AudioDeviceDescriptor> earpiece_;
    sptr<AudioDeviceDescriptor> speaker_;
    sptr<AudioDeviceDescriptor> defalutMic_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif //ST_AUDIO_DEVICE_MANAGER_H