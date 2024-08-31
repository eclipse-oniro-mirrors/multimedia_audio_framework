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
#ifndef LOG_TAG
#define LOG_TAG "ClientTypeManager"
#endif

#include "client_type_manager.h"
#ifdef FEATURE_APPGALLERY
#include "appgallery_service_client_appinfo_category.h"
#include "appgallery_service_client_param.h"
#endif

#include "audio_policy_log.h"

namespace OHOS {
namespace AudioStandard {
#ifdef FEATURE_APPGALLERY
const int32_t GAME_CATEGORY_ID = 2;
const std::string SERVICE_NAME = "audio_service";
#endif

ClientTypeManager *ClientTypeManager::GetInstance()
{
    static ClientTypeManager clientTypeManager;
    return &clientTypeManager;
}

void ClientTypeManager::GetAndSaveClientType(uint32_t uid, const std::string &bundleName)
{
    AUDIO_INFO_LOG("uid: %{public}u, bundle name %{public}s", uid, bundleName.c_str());
#ifdef FEATURE_APPGALLERY
    auto it = clientTypeMap_.find(uid);
    if (it != clientTypeMap_.end()) {
        AUDIO_INFO_LOG("Uid already in map");
        return;
    }
    if (bundleName == "") {
        AUDIO_WARNING_LOG("Get bundle name for %{public}u failed", uid);
        return;
    }

    std::vector<AppGalleryServiceClient::AppCategoryInfo> resultArray;
    std::vector<std::string> bundleNames;
    bundleNames.push_back(bundleName);

    int32_t ret = AppGalleryServiceClient::CategoryManager::GetCategoryFromSystem(bundleNames, resultArray,
        SERVICE_NAME);
    if (ret != 0) {
        AUDIO_WARNING_LOG("Get category failed, ret %{public}d", ret);
        return;
    }
    for (AppGalleryServiceClient::AppCategoryInfo &item : resultArray) {
        if (item.primaryCategoryId == GAME_CATEGORY_ID) {
            AUDIO_INFO_LOG("Is game type");
            clientTypeMap_.insert_or_assign(uid, CLIENT_TYPE_GAME);
        } else {
            AUDIO_INFO_LOG("Is not game type");
            clientTypeMap_.insert_or_assign(uid, CLIENT_TYPE_OTHERS);
        }
    }
#else
    AUDIO_WARNING_LOG("Get client type is not supported");
#endif
}

ClientType ClientTypeManager::GetClientTypeByUid(uint32_t uid)
{
    AUDIO_INFO_LOG("uid %{public}u", uid);
    auto it = clientTypeMap_.find(uid);
    if (it == clientTypeMap_.end()) {
        AUDIO_INFO_LOG("Cannot find uid");
        return CLIENT_TYPE_OTHERS;
    }
    return it->second;
}
} // namespace AudioStandard
} // namespace OHOS
