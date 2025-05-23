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

#ifndef ST_AUDIO_ZONE_SERVICE_H
#define ST_AUDIO_ZONE_SERVICE_H

#include <unordered_map>
#include <memory>
#include "audio_interrupt_service.h"
#include "i_standard_audio_zone_client.h"
#include "audio_policy_server_handler.h"

namespace OHOS {
namespace AudioStandard {
class AudioZone;
class AudioZoneClientManager;

class AudioZoneService {
public:
    static AudioZoneService &GetInstance();

    void Init(std::shared_ptr<AudioPolicyServerHandler> handler,
        std::shared_ptr<AudioInterruptService> interruptService);
    void DeInit();
    
    int32_t CreateAudioZone(const std::string &name, const AudioZoneContext &context);
    void ReleaseAudioZone(int32_t zoneId);
    const std::vector<std::shared_ptr<AudioZoneDescriptor>> GetAllAudioZone();
    const std::shared_ptr<AudioZoneDescriptor> GetAudioZone(int32_t zoneId);
    
    int32_t BindDeviceToAudioZone(int32_t zoneId,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> devices);
    int32_t UnBindDeviceToAudioZone(int32_t zoneId,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> devices);
    int32_t UpdateDeviceFromGlobalForAllZone(std::shared_ptr<AudioDeviceDescriptor> device);

    int32_t RegisterAudioZoneClient(pid_t clientPid, sptr<IStandardAudioZoneClient> client);
    void UnRegisterAudioZoneClient(pid_t clientPid);
    int32_t EnableAudioZoneReport(pid_t clientPid, bool enable);
    int32_t EnableAudioZoneChangeReport(pid_t clientPid, int32_t zoneId, bool enable);

    int32_t AddUidToAudioZone(int32_t zoneId, int32_t uid);
    int32_t RemoveUidFromAudioZone(int32_t zoneId, int32_t uid);
    int32_t FindAudioZoneByUid(int32_t uid);

    int32_t EnableSystemVolumeProxy(pid_t clientPid, int32_t zoneId, bool enable);

    std::list<std::pair<AudioInterrupt, AudioFocuState>> GetAudioInterruptForZone(int32_t zoneId);
    std::list<std::pair<AudioInterrupt, AudioFocuState>> GetAudioInterruptForZone(int32_t zoneId,
        const std::string &deviceTag);
    int32_t EnableAudioZoneInterruptReport(pid_t clientPid, int32_t zoneId,
        const std::string &deviceTag, bool enable);

    int32_t ActivateAudioInterrupt(int32_t zoneId, const AudioInterrupt &audioInterrupt,
        bool isUpdatedAudioStrategy = false);
    int32_t DeactivateAudioInterrupt(int32_t zoneId, const AudioInterrupt &audioInterrupt);
    int32_t InjectInterruptToAudioZone(int32_t zoneId,
        const std::list<std::pair<AudioInterrupt, AudioFocuState>> &interrupts);
    int32_t InjectInterruptToAudioZone(int32_t zoneId, const std::string &deviceTag,
        const std::list<std::pair<AudioInterrupt, AudioFocuState>> &interrupts);
    
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> FetchOutputDevices(int32_t zoneId,
        StreamUsage streamUsage, int32_t clientUid, const RouterType &bypassType);
    std::shared_ptr<AudioDeviceDescriptor> FetchInputDevice(int32_t zoneId,
       SourceType sourceType, int32_t clientUid);
    
    const std::string GetZoneStringDescriptor(int32_t zoneId);

private:
    AudioZoneService() = default;
    ~AudioZoneService() = default;

    std::shared_ptr<AudioInterruptService> interruptService_;
    std::shared_ptr<AudioZoneClientManager> zoneClientManager_;
    std::unordered_map<int32_t, std::shared_ptr<AudioZone>> zoneMaps_;
    std::set<pid_t> zoneReportClientList_;
    std::mutex zoneMutex_;

    std::shared_ptr<AudioZone> FindZone(int32_t zoneId);
    int32_t AddKeyToAudioZone(int32_t zoneId, int32_t uid, const std::string &deviceTag,
        const std::string &streamTag);
    int32_t RemoveKeyFromAudioZone(int32_t zoneId, int32_t uid, const std::string &deviceTag,
        const std::string &streamTag);
    int32_t FindAudioZoneByKey(int32_t uid, const std::string &deviceTag, const std::string &streamTag);
    bool CheckIsZoneValid(int32_t zoneId);
    void RemoveDeviceFromGlobal(std::shared_ptr<AudioDeviceDescriptor> device);
};
} // namespace AudioStandard
} // namespace OHOS

#endif // ST_AUDIO_ZONE_SERVICE_H