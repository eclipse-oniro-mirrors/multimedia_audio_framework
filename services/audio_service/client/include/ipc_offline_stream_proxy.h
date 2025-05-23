/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef IPC_OFFLINE_STREAM_PROXY_H
#define IPC_OFFLINE_STREAM_PROXY_H

#include "message_parcel.h"

#include "ipc_offline_stream.h"

namespace OHOS {
namespace AudioStandard {
class IpcOfflineStreamProxy : public IRemoteProxy<IpcOfflineStream> {
public:
    explicit IpcOfflineStreamProxy(const sptr<IRemoteObject> &impl);
    virtual ~IpcOfflineStreamProxy();

    // override for IpcOfflineStream
#ifdef FEATURE_OFFLINE_EFFECT
    int32_t CreateOfflineEffectChain(const std::string &chainName) override;

    int32_t ConfigureOfflineEffectChain(const AudioStreamInfo &inInfo,
        const AudioStreamInfo &outInfo) override;

    int32_t PrepareOfflineEffectChain(std::shared_ptr<AudioSharedMemory> &inBuffer,
        std::shared_ptr<AudioSharedMemory> &outBuffer) override;

    int32_t ProcessOfflineEffectChain(uint32_t inSize, uint32_t outSize) override;

    void ReleaseOfflineEffectChain() override;
#endif
private:
    static inline BrokerDelegator<IpcOfflineStreamProxy> delegator_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // IPC_OFFLINE_STREAM_PROXY_H
