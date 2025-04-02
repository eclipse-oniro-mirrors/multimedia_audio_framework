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

#ifndef AUDIO_SCHEDULE_H
#define AUDIO_SCHEDULE_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void ScheduleReportData(uint32_t pid, uint32_t tid, const char *bundleName);
void ScheduleThreadInServer(uint32_t pid, uint32_t tid);
void UnscheduleThreadInServer(uint32_t pid, uint32_t tid);
void OnAddResSchedService(uint32_t audioServerPid);
void UnscheduleReportData(uint32_t pid, uint32_t tid, const char* bundleName);
bool SetEndpointThreadPriority();
bool ResetEndpointThreadPriority();

#ifdef __cplusplus
}
#endif

#endif // AUDIO_SCHEDULE_H