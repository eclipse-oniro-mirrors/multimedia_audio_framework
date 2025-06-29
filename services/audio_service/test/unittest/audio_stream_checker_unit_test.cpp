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
#include <gtest/gtest.h>
#include "audio_stream_checker.h"
#include "audio_errors.h"
#include "audio_utils.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

class AudioStreamCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void AudioStreamCheckerTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void AudioStreamCheckerTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void AudioStreamCheckerTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testcases
}

void AudioStreamCheckerTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testcases
}

/**
 * @tc.name  : Test InitChecker API
 * @tc.type  : FUNC
 * @tc.number: InitCheckerTest_001
 */
HWTEST(AudioStreamCheckerTest, InitCheckerTest_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->InitChecker(para, 100000, 100000);
    int32_t size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test RecordFrame API
 * @tc.type  : FUNC
 * @tc.number: RecordFrame_001
 */
HWTEST(AudioStreamCheckerTest, RecordFrame_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->RecordMuteFrame();
    int32_t num = checker->checkParaVector_[0].muteFrameNum;
    EXPECT_GT(num, 0);
    checker->RecordNodataFrame();
    num = checker->checkParaVector_[0].noDataFrameNum;
    EXPECT_GT(num, 0);
    checker->RecordNormalFrame();
    num = checker->checkParaVector_[0].normalFrameCount;
    EXPECT_GT(num, 0);
}

/**
 * @tc.name  : Test GetAppUid API
 * @tc.type  : FUNC
 * @tc.number: GetAppUid_001
 */
HWTEST(AudioStreamCheckerTest, GetAppUid_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    cfg.appInfo.appUid = 20002000;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    int32_t uid = checker->GetAppUid();
    EXPECT_EQ(uid, 20002000);
}

/**
 * @tc.name  : Test DeleteCheckerPara API
 * @tc.type  : FUNC
 * @tc.number: DeleteCheckerPara_001
 */
HWTEST(AudioStreamCheckerTest, DeleteCheckerPara_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->DeleteCheckerPara(100000, 100000);
    int32_t size = checker->checkParaVector_.size();
    EXPECT_EQ(size, 0);
}

/**
 * @tc.name  : Test MonitorCheckFrame API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrame_001
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrame_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 0;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->RecordMuteFrame();
    checker->RecordNormalFrame();
    checker->MonitorCheckFrame();
    DataTransferStateChangeType status = checker->checkParaVector_[0].lastStatus;
    EXPECT_EQ(status, DATA_TRANS_STOP);
}

/**
 * @tc.name  : Test MonitorCheckFrame API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrame_002
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrame_002, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 0;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    for (int i = 0; i < 4; i++) {
        checker->RecordNormalFrame();
    }
    checker->MonitorCheckFrame();
    DataTransferStateChangeType status = checker->checkParaVector_[0].lastStatus;
    EXPECT_EQ(status, DATA_TRANS_RESUME);
}

/**
 * @tc.name  : Test MonitorCheckFrame API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrame_003
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrame_003, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 0;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->RecordMuteFrame();
    checker->RecordNormalFrame();
    checker->MonitorCheckFrame();
    for (int i = 0; i < 4; i++) {
        checker->RecordNormalFrame();
    }
    checker->MonitorCheckFrame();
    DataTransferStateChangeType status = checker->checkParaVector_[0].lastStatus;
    EXPECT_EQ(status, DATA_TRANS_RESUME);
}

/**
 * @tc.name  : Test MonitorOnAllCallback API
 * @tc.type  : FUNC
 * @tc.number: MonitorOnAllCallback_001
 */
HWTEST(AudioStreamCheckerTest, MonitorOnAllCallback_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->MonitorOnAllCallback(AUDIO_STREAM_START, false);
    int32_t size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorOnAllCallback API
 * @tc.type  : FUNC
 * @tc.number: MonitorOnAllCallback_002
 */
HWTEST(AudioStreamCheckerTest, MonitorOnAllCallback_002, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 2;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->MonitorOnAllCallback(DATA_TRANS_RESUME, true);
    int32_t size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorOnAllCallback API
 * @tc.type  : FUNC
 * @tc.number: MonitorOnAllCallback_003
 */
HWTEST(AudioStreamCheckerTest, MonitorOnAllCallback_003, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 2;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->monitorSwitch_ = false;
    checker->MonitorOnAllCallback(DATA_TRANS_RESUME, true);
    int32_t size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test OnRemoteAppDied API
 * @tc.type  : FUNC
 * @tc.number: OnRemoteAppDied_001
 */
HWTEST(AudioStreamCheckerTest, OnRemoteAppDied_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->OnRemoteAppDied(100000);
    int size = checker->checkParaVector_.size();
    EXPECT_EQ(size, 0);
}

/**
 * @tc.name  : Test MonitorCheckFrameSub API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrameSub_001
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrameSub_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.hasInitCheck = false;
    checker->MonitorCheckFrameSub(checkerPara);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorCheckFrameSub API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrameSub_002
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrameSub_002, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.hasInitCheck = true;
    checkerPara.isMonitorMuteFrame = true;
    checkerPara.isMonitorNoDataFrame = true;
    checker->MonitorCheckFrameSub(checkerPara);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorCheckFrameSub API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrameSub_003
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrameSub_003, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.hasInitCheck = true;
    checkerPara.para.timeInterval = 2000000000;
    checkerPara.lastUpdateTime = ClockTime::GetCurNano();
    checker->MonitorCheckFrameSub(checkerPara);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorCheckFrameAction API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrameAction_001
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrameAction_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.lastStatus = DATA_TRANS_STOP;
    checkerPara.sumFrameCount = 100;
    int64_t abnormalFrameNum = 60;
    float badFrameRatio = 0.5f;
    checker->MonitorCheckFrameAction(checkerPara, abnormalFrameNum, badFrameRatio);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorCheckFrameAction API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrameAction_002
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrameAction_002, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.lastStatus = AUDIO_STREAM_PAUSE;
    checkerPara.sumFrameCount = 100;
    int64_t abnormalFrameNum = 60;
    float badFrameRatio = 0.5f;
    checker->MonitorCheckFrameAction(checkerPara, abnormalFrameNum, badFrameRatio);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorCheckFrameAction API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrameAction_003
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrameAction_003, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.lastStatus = AUDIO_STREAM_START;
    checkerPara.sumFrameCount = 100;
    int64_t abnormalFrameNum = 60;
    float badFrameRatio = 0.5f;
    checker->MonitorCheckFrameAction(checkerPara, abnormalFrameNum, badFrameRatio);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorCheckFrameAction API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrameAction_004
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrameAction_004, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.lastStatus = DATA_TRANS_RESUME;
    checkerPara.sumFrameCount = 100;
    int64_t abnormalFrameNum = 40;
    float badFrameRatio = 0.5f;
    checker->MonitorCheckFrameAction(checkerPara, abnormalFrameNum, badFrameRatio);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}
/**
 * @tc.name  : Test MonitorCheckFrameAction API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrameAction_005
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrameAction_005, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.lastStatus = AUDIO_STREAM_PAUSE;
    checkerPara.sumFrameCount = 100;
    int64_t abnormalFrameNum = 40;
    float badFrameRatio = 0.5f;
    checker->MonitorCheckFrameAction(checkerPara, abnormalFrameNum, badFrameRatio);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}
/**
 * @tc.name  : Test MonitorCheckFrameAction API
 * @tc.type  : FUNC
 * @tc.number: MonitorCheckFrameAction_006
 */
HWTEST(AudioStreamCheckerTest, MonitorCheckFrameAction_006, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.lastStatus = DATA_TRANS_STOP;
    checkerPara.sumFrameCount = 100;
    int64_t abnormalFrameNum = 40;
    float badFrameRatio = 0.5f;
    checker->MonitorCheckFrameAction(checkerPara, abnormalFrameNum, badFrameRatio);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorOnCallback API
 * @tc.type  : FUNC
 * @tc.number: MonitorOnCallback_001
 */
HWTEST(AudioStreamCheckerTest, MonitorOnCallback_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->monitorSwitch_ = false;
    CheckerParam checkerPara;
    checker->MonitorOnCallback(AUDIO_STREAM_START, true, checkerPara);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorOnCallback API
 * @tc.type  : FUNC
 * @tc.number: MonitorOnCallback_002
 */
HWTEST(AudioStreamCheckerTest, MonitorOnCallback_002, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.sumFrameCount = 0;
    checkerPara.hasInitCheck = true;
    checker->MonitorOnCallback(AUDIO_STREAM_START, true, checkerPara);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test MonitorOnCallback API
 * @tc.type  : FUNC
 * @tc.number: MonitorOnCallback_003
 */
HWTEST(AudioStreamCheckerTest, MonitorOnCallback_003, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.sumFrameCount = 100;
    checkerPara.hasInitCheck = true;
    checker->MonitorOnCallback(AUDIO_STREAM_START, true, checkerPara);
    int size = checker->checkParaVector_.size();
    EXPECT_GT(size, 0);
}

/**
 * @tc.name  : Test RecordStandbyTime API
 * @tc.type  : FUNC
 * @tc.number: RecordStandbyTime_001
 */
HWTEST(AudioStreamCheckerTest, RecordStandbyTime_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->RecordStandbyTime(true);
    int64_t time = checker->checkParaVector_[0].standbyStartTime;
    EXPECT_GT(time, 0);
}

/**
 * @tc.name  : Test RecordStandbyTime API
 * @tc.type  : FUNC
 * @tc.number: RecordStandbyTime_002
 */
HWTEST(AudioStreamCheckerTest, RecordStandbyTime_002, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    checker->RecordStandbyTime(false);
    int64_t time = checker->checkParaVector_[0].standbyStopTime;
    EXPECT_GT(time, 0);
}

/**
 * @tc.name  : Test CalculateFrameAfterStandby API
 * @tc.type  : FUNC
 * @tc.number: CalculateFrameAfterStandby_001
 */
HWTEST(AudioStreamCheckerTest, CalculateFrameAfterStandby_001, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.isMonitorNoDataFrame = true;
    checkerPara.standbyStartTime = ClockTime::GetCurNano();
    checkerPara.standbyStopTime = checkerPara.standbyStartTime + 1000000000;
    int64_t abnormalFrameNum = 0;
    checker->CalculateFrameAfterStandby(checkerPara, abnormalFrameNum);
    EXPECT_GT(abnormalFrameNum, 0);
}

/**
 * @tc.name  : Test CalculateFrameAfterStandby API
 * @tc.type  : FUNC
 * @tc.number: CalculateFrameAfterStandby_002
 */
HWTEST(AudioStreamCheckerTest, CalculateFrameAfterStandby_002, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.isMonitorNoDataFrame = true;
    checkerPara.standbyStartTime = ClockTime::GetCurNano() - 1000000000;
    checkerPara.standbyStopTime = 0;
    int64_t abnormalFrameNum = 0;
    checker->CalculateFrameAfterStandby(checkerPara, abnormalFrameNum);
    EXPECT_GT(abnormalFrameNum, 0);
}

/**
 * @tc.name  : Test CalculateFrameAfterStandby API
 * @tc.type  : FUNC
 * @tc.number: CalculateFrameAfterStandby_003
 */
HWTEST(AudioStreamCheckerTest, CalculateFrameAfterStandby_003, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.isMonitorNoDataFrame = true;
    checkerPara.standbyStartTime = 0;
    checkerPara.lastUpdateTime = ClockTime::GetCurNano();
    checkerPara.standbyStopTime = checkerPara.lastUpdateTime + 1000000000;
    int64_t abnormalFrameNum = 0;
    checker->CalculateFrameAfterStandby(checkerPara, abnormalFrameNum);
    EXPECT_GT(abnormalFrameNum, 0);
}

/**
 * @tc.name  : Test CalculateFrameAfterStandby API
 * @tc.type  : FUNC
 * @tc.number: CalculateFrameAfterStandby_004
 */
HWTEST(AudioStreamCheckerTest, CalculateFrameAfterStandby_004, TestSize.Level1)
{
    AudioProcessConfig cfg;
    DataTransferMonitorParam para;
    para.badDataTransferTypeBitMap = 3;
    para.timeInterval = 2000000000;
    para.badFramesRatio = 50;
    std::shared_ptr<AudioStreamChecker> checker = std::make_shared<AudioStreamChecker>(cfg);
    checker->InitChecker(para, 100000, 100000);
    CheckerParam checkerPara;
    checkerPara.isMonitorNoDataFrame = true;
    checkerPara.standbyStartTime = 0;
    checkerPara.standbyStopTime = 0;
    int64_t abnormalFrameNum = 0;
    checker->CalculateFrameAfterStandby(checkerPara, abnormalFrameNum);
    EXPECT_EQ(abnormalFrameNum, 0);
}
}
}