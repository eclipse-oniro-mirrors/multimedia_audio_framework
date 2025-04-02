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

#ifndef LOG_TAG
#define LOG_TAG "AudioServiceAppStateListener"
#endif

#include <map>

#include "audio_common_log.h"
#include "app_state_listener.h"
#include "dfx_msg_manager.h"
#include "audio_utils.h"
#include "system_ability_definition.h"
#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "iservice_registry.h"


namespace OHOS {
namespace AudioStandard {

static const std::map<AppExecFwk::ApplicationState, DfxAppState> DFX_APPSTATE_MAP = {
    {AppExecFwk::ApplicationState::APP_STATE_CREATE, DFX_APP_STATE_START},
    {AppExecFwk::ApplicationState::APP_STATE_FOREGROUND, DFX_APP_STATE_FOREGROUND},
    {AppExecFwk::ApplicationState::APP_STATE_BACKGROUND, DFX_APP_STATE_BACKGROUND},
    {AppExecFwk::ApplicationState::APP_STATE_END, DFX_APP_STATE_END},
    {AppExecFwk::ApplicationState::APP_STATE_TERMINATED, DFX_APP_STATE_END}
};

AppStateListener::AppStateListener()
{
    AUDIO_INFO_LOG("enter");
}

void AppStateListener::OnAppStateChanged(const AppExecFwk::AppProcessData& appProcessData)
{
    for (const auto& appData : appProcessData.appDatas) {
        AUDIO_INFO_LOG("app state changed, bundleName=%{public}s uid=%{public}d pid=%{public}d state=%{public}d",
            appData.appName.c_str(), appData.uid, appProcessData.pid, appProcessData.appState);
        HandleAppStateChange(appProcessData.pid, appData.uid, static_cast<int32_t>(appProcessData.appState));
    }
}

AppExecFwk::BundleInfo AppStateListener::GetBundleInfoFromUid(int32_t callingUid)
{
    AudioXCollie audioXCollie("AudioPolicyServer::PerStateChangeCbCustomizeCallback::getUidByBundleName",
        GET_BUNDLE_TIME_OUT_SECONDS);
    std::string bundleName {"uid:" + std::to_string(callingUid)};
    AppExecFwk::BundleInfo bundleInfo;
    WatchTimeout guard("SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager():GetBundleInfoFromUid");
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(systemAbilityManager != nullptr, bundleInfo, "systemAbilityManager is nullptr");
    guard.CheckCurrTimeout();

    sptr<IRemoteObject> remoteObject = systemAbilityManager->CheckSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    CHECK_AND_RETURN_RET_PRELOG(remoteObject != nullptr, bundleInfo, "remoteObject is nullptr");

    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = OHOS::iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    CHECK_AND_RETURN_RET_LOG(bundleMgrProxy != nullptr, bundleInfo, "bundleMgrProxy is nullptr");

    WatchTimeout reguard("bundleMgrProxy->GetNameForUid:GetBundleInfoFromUid");
    bundleMgrProxy->GetNameForUid(callingUid, bundleName);

    bundleMgrProxy->GetBundleInfoV9(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_ABILITIES |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_REQUESTED_PERMISSION |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_HASH_VALUE,
        bundleInfo,
        AppExecFwk::Constants::ALL_USERID);
    reguard.CheckCurrTimeout();

    return bundleInfo;
}


void AppStateListener::HandleAppStateChange(int32_t pid, int32_t uid, int32_t state)
{
    auto pos = DFX_APPSTATE_MAP.find(static_cast<AppExecFwk::ApplicationState>(state));
    auto appState = (pos == DFX_APPSTATE_MAP.end()) ? DFX_APP_STATE_UNKNOWN : pos->second;
    CHECK_AND_RETURN_LOG(pos != DFX_APPSTATE_MAP.end(), "invalid app state%{public}d", state);

    AUDIO_INFO_LOG("app state changed, pid=%{public}d state=%{public}d", pid, state);
    auto &manager = DfxMsgManager::GetInstance();
    if (appState == DFX_APP_STATE_START) {
        if (manager.CheckCanAddAppInfo(uid)) {
            auto info = GetBundleInfoFromUid(uid);
            manager.SaveAppInfo({uid, info.name, info.versionName});
        }
        manager.UpdateAppState(uid, appState, true);
    } else {
        manager.UpdateAppState(uid, appState);
    }
}
}
}
