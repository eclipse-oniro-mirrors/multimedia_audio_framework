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
#define LOG_TAG "OHAudioBufferBase"
#endif

#include "oh_audio_buffer.h"

#include <cinttypes>
#include <climits>
#include <memory>
#include <sys/mman.h>
#include "ashmem.h"

#include "audio_errors.h"
#include "audio_service_log.h"
#include "futex_tool.h"
#include "audio_utils.h"
#include "audio_parcel_helper.h"

namespace OHOS {
namespace AudioStandard {
namespace {
    static const size_t MAX_MMAP_BUFFER_SIZE = 10 * 1024 * 1024; // 10M
    static const std::string STATUS_INFO_BUFFER = "status_info_buffer";
    static constexpr int MINFD = 2;
}
class AudioSharedMemoryImpl : public AudioSharedMemory {
public:
    uint8_t *GetBase() override;
    size_t GetSize() override;
    int GetFd() override;
    std::string GetName() override;

    AudioSharedMemoryImpl(size_t size, const std::string &name);

    AudioSharedMemoryImpl(int fd, size_t size, const std::string &name);

    ~AudioSharedMemoryImpl();

    int32_t Init();

    bool Marshalling(Parcel &parcel) const override;

private:
    void Close();

    uint8_t *base_;
    int fd_;
    size_t size_;
    std::string name_;
};

class ScopedFd {
public:
    explicit ScopedFd(int fd) : fd_(fd) {}
    ~ScopedFd()
    {
        if (fd_ > MINFD) {
            CloseFd(fd_);
        }
    }
private:
    int fd_ = -1;
};

AudioSharedMemoryImpl::AudioSharedMemoryImpl(size_t size, const std::string &name)
    : base_(nullptr), fd_(INVALID_FD), size_(size), name_(name)
{
    AUDIO_DEBUG_LOG("AudioSharedMemory ctor with size: %{public}zu name: %{public}s", size_, name_.c_str());
}

AudioSharedMemoryImpl::AudioSharedMemoryImpl(int fd, size_t size, const std::string &name)
    : base_(nullptr), fd_(dup(fd)), size_(size), name_(name)
{
    AUDIO_DEBUG_LOG("AudioSharedMemory ctor with fd %{public}d size %{public}zu name %{public}s", fd_, size_,
        name_.c_str());
}

AudioSharedMemoryImpl::~AudioSharedMemoryImpl()
{
    AUDIO_DEBUG_LOG(" %{public}s enter ~AudioSharedMemoryImpl()", name_.c_str());
    Close();
}

int32_t AudioSharedMemoryImpl::Init()
{
    CHECK_AND_RETURN_RET_LOG((size_ > 0 && size_ < MAX_MMAP_BUFFER_SIZE), ERR_INVALID_PARAM,
        "Init falied: size out of range: %{public}zu", size_);
    bool isFromRemote = false;
    if (fd_ >= 0) {
        if (fd_ == STDIN_FILENO || fd_ == STDOUT_FILENO || fd_ == STDERR_FILENO) {
            AUDIO_WARNING_LOG("fd is special fd: %{public}d", fd_);
        }
        isFromRemote = true;
        int size = AshmemGetSize(fd_); // hdi fd may not support
        if (size < 0 || static_cast<size_t>(size) != size_) {
            AUDIO_WARNING_LOG("AshmemGetSize faied, get %{public}d", size);
        }
    } else {
        fd_ = AshmemCreate(name_.c_str(), size_);
        if (fd_ == STDIN_FILENO || fd_ == STDOUT_FILENO || fd_ == STDERR_FILENO) {
            AUDIO_WARNING_LOG("fd is special fd: %{public}d", fd_);
        }
        CHECK_AND_RETURN_RET_LOG((fd_ >= 0), ERR_OPERATION_FAILED, "Init falied: fd %{public}d", fd_);
    }

    void *addr = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    CHECK_AND_RETURN_RET_LOG(addr != MAP_FAILED, ERR_OPERATION_FAILED, "Init falied: fd %{public}d size %{public}zu",
        fd_, size_);
    base_ = static_cast<uint8_t *>(addr);
    AUDIO_DEBUG_LOG("Init %{public}s <%{public}s> done.", (isFromRemote ? "remote" : "local"),
        name_.c_str());
    return SUCCESS;
}

bool AudioSharedMemoryImpl::Marshalling(Parcel &parcel) const
{
    // Parcel -> MessageParcel
    MessageParcel &msgParcel = static_cast<MessageParcel &>(parcel);
    CHECK_AND_RETURN_RET_LOG((size_ > 0 && size_ < MAX_MMAP_BUFFER_SIZE), false, "invalid size: %{public}zu", size_);
    return msgParcel.WriteFileDescriptor(fd_) &&
        msgParcel.WriteUint64(static_cast<uint64_t>(size_)) &&
        msgParcel.WriteString(name_);
}

void AudioSharedMemoryImpl::Close()
{
    if (base_ != nullptr) {
        (void)munmap(base_, size_);
        base_ = nullptr;
        size_ = 0;
        AUDIO_DEBUG_LOG("%{public}s munmap done", name_.c_str());
    }

    if (fd_ >= 0) {
        (void)CloseFd(fd_);
        fd_ = INVALID_FD;
        AUDIO_DEBUG_LOG("%{public}s close fd done", name_.c_str());
    }
}

uint8_t *AudioSharedMemoryImpl::GetBase()
{
    return base_;
}

size_t AudioSharedMemoryImpl::GetSize()
{
    return size_;
}

std::string AudioSharedMemoryImpl::GetName()
{
    return name_;
}

int AudioSharedMemoryImpl::GetFd()
{
    return fd_;
}

std::shared_ptr<AudioSharedMemory> AudioSharedMemory::CreateFormLocal(size_t size, const std::string &name)
{
    std::shared_ptr<AudioSharedMemoryImpl> sharedMemory = std::make_shared<AudioSharedMemoryImpl>(size, name);
    CHECK_AND_RETURN_RET_LOG(sharedMemory->Init() == SUCCESS,
        nullptr, "CreateFormLocal failed");
    return sharedMemory;
}

std::shared_ptr<AudioSharedMemory> AudioSharedMemory::CreateFromRemote(int fd, size_t size, const std::string &name)
{
    int minfd = 2; // ignore stdout, stdin and stderr.
    CHECK_AND_RETURN_RET_LOG(fd > minfd, nullptr, "CreateFromRemote failed: invalid fd: %{public}d", fd);
    std::shared_ptr<AudioSharedMemoryImpl> sharedMemory = std::make_shared<AudioSharedMemoryImpl>(fd, size, name);
    if (sharedMemory->Init() != SUCCESS) {
        AUDIO_ERR_LOG("CreateFromRemote failed");
        return nullptr;
    }
    return sharedMemory;
}

int32_t AudioSharedMemory::WriteToParcel(const std::shared_ptr<AudioSharedMemory> &memory, MessageParcel &parcel)
{
    std::shared_ptr<AudioSharedMemoryImpl> memoryImpl = std::static_pointer_cast<AudioSharedMemoryImpl>(memory);
    CHECK_AND_RETURN_RET_LOG(memoryImpl != nullptr, ERR_OPERATION_FAILED, "invalid pointer.");

    int32_t fd = memoryImpl->GetFd();

    size_t size = memoryImpl->GetSize();
    CHECK_AND_RETURN_RET_LOG((size > 0 && size < MAX_MMAP_BUFFER_SIZE), ERR_INVALID_PARAM,
        "invalid size: %{public}zu", size);
    uint64_t sizeTmp = static_cast<uint64_t>(size);

    std::string name = memoryImpl->GetName();

    parcel.WriteFileDescriptor(fd);
    parcel.WriteUint64(sizeTmp);
    parcel.WriteString(name);

    return SUCCESS;
}

std::shared_ptr<AudioSharedMemory> AudioSharedMemory::ReadFromParcel(MessageParcel &parcel)
{
    int fd = parcel.ReadFileDescriptor();

    uint64_t sizeTmp = parcel.ReadUint64();
    CHECK_AND_RETURN_RET_LOG((sizeTmp > 0 && sizeTmp < MAX_MMAP_BUFFER_SIZE), nullptr, "failed with invalid size");
    size_t size = static_cast<size_t>(sizeTmp);

    std::string name = parcel.ReadString();

    std::shared_ptr<AudioSharedMemory> memory = AudioSharedMemory::CreateFromRemote(fd, size, name);
    if (memory == nullptr || memory->GetBase() == nullptr) {
        AUDIO_ERR_LOG("ReadFromParcel failed");
        memory = nullptr;
    }
    CloseFd(fd);
    return memory;
}

bool AudioSharedMemory::Marshalling(Parcel &parcel) const
{
    return true;
}

AudioSharedMemory *AudioSharedMemory::Unmarshalling(Parcel &parcel)
{
    // Parcel -> MessageParcel
    MessageParcel &msgParcel = static_cast<MessageParcel &>(parcel);
    int fd = msgParcel.ReadFileDescriptor();
    int minfd = 2; // ignore stdout, stdin and stderr.
    CHECK_AND_RETURN_RET_LOG(fd > minfd, nullptr, "CreateFromRemote failed: invalid fd: %{public}d", fd);
    ScopedFd scopedFd(fd);

    uint64_t sizeTmp = msgParcel.ReadUint64();
    CHECK_AND_RETURN_RET_LOG((sizeTmp > 0 && sizeTmp < MAX_MMAP_BUFFER_SIZE), nullptr, "failed with invalid size");
    size_t size = static_cast<size_t>(sizeTmp);

    std::string name = msgParcel.ReadString();

    auto memory = new(std::nothrow) AudioSharedMemoryImpl(fd, size, name);
    if (memory == nullptr) {
        AUDIO_ERR_LOG("not enough memory");
        return nullptr;
    }

    if (memory->Init() != SUCCESS || memory->GetBase() == nullptr) {
        AUDIO_ERR_LOG("Init failed or GetBase failed");
        delete memory;
        return nullptr;
    }
    return memory;
}

OHAudioBufferBase::OHAudioBufferBase(AudioBufferHolder bufferHolder, uint32_t totalSizeInFrame,
    uint32_t byteSizePerFrame) : bufferHolder_(bufferHolder), totalSizeInFrame_(totalSizeInFrame),
    byteSizePerFrame_(byteSizePerFrame), totalSizeInByte_(totalSizeInFrame * byteSizePerFrame),
    audioMode_(AUDIO_MODE_PLAYBACK),
    basicBufferInfo_(nullptr)
{
    AUDIO_DEBUG_LOG("ctor with holder:%{public}d mode:%{public}d", bufferHolder_, audioMode_);
}

int32_t OHAudioBufferBase::SizeCheck()
{
    if (totalSizeInFrame_ > UINT_MAX / byteSizePerFrame_) {
        AUDIO_ERR_LOG("failed: totalSizeInFrame: %{public}u byteSizePerFrame: %{public}u",
            totalSizeInFrame_, byteSizePerFrame_);
        return ERR_INVALID_PARAM;
    }

    // data buffer size check
    CHECK_AND_RETURN_RET_LOG((totalSizeInByte_ < MAX_MMAP_BUFFER_SIZE), ERR_INVALID_PARAM, "too large totalSizeInByte "
        "%{public}zu", totalSizeInByte_);

    return SUCCESS;
}

int32_t OHAudioBufferBase::Init(int dataFd, int infoFd, size_t statusInfoExtSize)
{
    int32_t ret = SizeCheck();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_INVALID_PARAM, "failed: invalid size.");

    // init for statusInfoBuffer
    size_t statusInfoSize = sizeof(BasicBufferInfo) + statusInfoExtSize;
    if (infoFd != INVALID_FD && (bufferHolder_ == AUDIO_CLIENT || bufferHolder_ == AUDIO_SERVER_INDEPENDENT)) {
        statusInfoMem_ = AudioSharedMemory::CreateFromRemote(infoFd, statusInfoSize, STATUS_INFO_BUFFER);
    } else {
        statusInfoMem_ = AudioSharedMemory::CreateFormLocal(statusInfoSize, STATUS_INFO_BUFFER);
    }
    CHECK_AND_RETURN_RET_LOG(statusInfoMem_ != nullptr, ERR_OPERATION_FAILED, "BasicBufferInfo mmap failed.");

    // init for dataBuffer
    if (dataFd == INVALID_FD && bufferHolder_ == AUDIO_SERVER_SHARED) {
        dataMem_ = AudioSharedMemory::CreateFormLocal(totalSizeInByte_, "server_client_buffer");
    } else {
        std::string memoryDesc = (bufferHolder_ == AUDIO_SERVER_ONLY ? "server_hdi_buffer" : "server_client_buffer");
        if (bufferHolder_ == AUDIO_SERVER_ONLY_WITH_SYNC) {
            AUDIO_INFO_LOG("Init sever_hdi_buffer with sync info");
            dataMem_ = AudioSharedMemory::CreateFromRemote(dataFd, totalSizeInByte_ + BASIC_SYNC_INFO_SIZE, memoryDesc);
        } else { // AUDIO_SERVER_ONLY
            dataMem_ = AudioSharedMemory::CreateFromRemote(dataFd, totalSizeInByte_, memoryDesc);
        }
    }
    CHECK_AND_RETURN_RET_LOG(dataMem_ != nullptr, ERR_OPERATION_FAILED, "dataMem_ mmap failed.");
    if (bufferHolder_ == AUDIO_SERVER_ONLY_WITH_SYNC) {
        syncReadFrame_ = reinterpret_cast<uint32_t *>(dataMem_->GetBase() + totalSizeInByte_);
        syncWriteFrame_ = syncReadFrame_ + sizeof(uint32_t);
    }

    dataBase_ = dataMem_->GetBase();

    basicBufferInfo_ = reinterpret_cast<BasicBufferInfo *>(statusInfoMem_->GetBase());

    InitBasicBufferInfo();

    if (bufferHolder_ == AUDIO_SERVER_SHARED || bufferHolder_ == AUDIO_SERVER_ONLY || bufferHolder_ ==
            AUDIO_SERVER_ONLY_WITH_SYNC) {
        basicBufferInfo_->handlePos.store(0);
        basicBufferInfo_->handleTime.store(0);
        basicBufferInfo_->totalSizeInFrame = totalSizeInFrame_;
        basicBufferInfo_->byteSizePerFrame = byteSizePerFrame_;
        basicBufferInfo_->streamStatus.store(STREAM_INVALID);
    }

    AUDIO_DEBUG_LOG("Init done.");
    return SUCCESS;
}

std::shared_ptr<OHAudioBufferBase> OHAudioBufferBase::CreateFromLocal(uint32_t totalSizeInFrame,
    uint32_t byteSizePerFrame)
{
    AUDIO_DEBUG_LOG("totalSizeInFrame %{public}d, byteSizePerFrame"
        " %{public}d", totalSizeInFrame, byteSizePerFrame);

    AudioBufferHolder bufferHolder = AudioBufferHolder::AUDIO_SERVER_SHARED;
    std::shared_ptr<OHAudioBufferBase> buffer = std::make_shared<OHAudioBufferBase>(bufferHolder, totalSizeInFrame,
        byteSizePerFrame);
    CHECK_AND_RETURN_RET_LOG(buffer->Init(INVALID_FD, INVALID_FD, 0) == SUCCESS,
        nullptr, "failed to init.");
    return buffer;
}

std::shared_ptr<OHAudioBufferBase> OHAudioBufferBase::CreateFromRemote(uint32_t totalSizeInFrame,
    uint32_t byteSizePerFrame, AudioBufferHolder bufferHolder,
    int dataFd, int infoFd)
{
    AUDIO_DEBUG_LOG("dataFd %{public}d, infoFd %{public}d", dataFd, infoFd);

    int minfd = 2; // ignore stdout, stdin and stderr.
    CHECK_AND_RETURN_RET_LOG(dataFd > minfd, nullptr, "invalid dataFd: %{public}d", dataFd);

    if (infoFd != INVALID_FD) {
        CHECK_AND_RETURN_RET_LOG(infoFd > minfd, nullptr, "invalid infoFd: %{public}d", infoFd);
    }
    std::shared_ptr<OHAudioBufferBase> buffer = std::make_shared<OHAudioBufferBase>(bufferHolder, totalSizeInFrame,
        byteSizePerFrame);
    if (buffer->Init(dataFd, infoFd, 0) != SUCCESS) {
        AUDIO_ERR_LOG("failed to init.");
        return nullptr;
    }
    return buffer;
}

int32_t OHAudioBufferBase::WriteToParcel(const std::shared_ptr<OHAudioBufferBase> &buffer, MessageParcel &parcel)
{
    AUDIO_DEBUG_LOG("WriteToParcel start.");
    AudioBufferHolder bufferHolder = buffer->GetBufferHolder();
    CHECK_AND_RETURN_RET_LOG(bufferHolder == AudioBufferHolder::AUDIO_SERVER_SHARED ||
        bufferHolder == AudioBufferHolder::AUDIO_SERVER_INDEPENDENT,
        ERROR_INVALID_PARAM, "buffer holder error:%{public}d", bufferHolder);

    auto initInfo = buffer->GetInitializationInfo();

    parcel.WriteUint32(bufferHolder);
    parcel.WriteUint32(initInfo.totalSizeInFrame);
    parcel.WriteUint32(initInfo.byteSizePerFrame);

    parcel.WriteFileDescriptor(initInfo.dataFd);
    parcel.WriteFileDescriptor(initInfo.infoFd);

    AUDIO_DEBUG_LOG("WriteToParcel done.");
    return SUCCESS;
}

std::shared_ptr<OHAudioBufferBase> OHAudioBufferBase::ReadFromParcel(MessageParcel &parcel)
{
    AUDIO_DEBUG_LOG("ReadFromParcel start.");
    uint32_t holder = parcel.ReadUint32();
    AudioBufferHolder bufferHolder = static_cast<AudioBufferHolder>(holder);
    if (bufferHolder != AudioBufferHolder::AUDIO_SERVER_SHARED &&
        bufferHolder != AudioBufferHolder::AUDIO_SERVER_INDEPENDENT) {
        AUDIO_ERR_LOG("ReadFromParcel buffer holder error:%{public}d", bufferHolder);
        return nullptr;
    }
    bufferHolder = bufferHolder == AudioBufferHolder::AUDIO_SERVER_SHARED ?
         AudioBufferHolder::AUDIO_CLIENT : bufferHolder;
    uint32_t totalSizeInFrame = parcel.ReadUint32();
    uint32_t byteSizePerFrame = parcel.ReadUint32();

    int dataFd = parcel.ReadFileDescriptor();
    int infoFd = parcel.ReadFileDescriptor();

    std::shared_ptr<OHAudioBufferBase> buffer = OHAudioBufferBase::CreateFromRemote(totalSizeInFrame,
        byteSizePerFrame, bufferHolder, dataFd, infoFd);
    if (buffer == nullptr) {
        AUDIO_ERR_LOG("ReadFromParcel failed.");
    } else if (totalSizeInFrame != buffer->basicBufferInfo_->totalSizeInFrame ||
        byteSizePerFrame != buffer->basicBufferInfo_->byteSizePerFrame) {
        AUDIO_WARNING_LOG("data in shared memory diff.");
    } else {
        AUDIO_DEBUG_LOG("Read some data done.");
    }
    CloseFd(dataFd);
    CloseFd(infoFd);
    AUDIO_DEBUG_LOG("ReadFromParcel done.");
    return buffer;
}

bool OHAudioBufferBase::Marshalling(Parcel &parcel) const
{
    MessageParcel &messageParcel = static_cast<MessageParcel &>(parcel);
    AudioBufferHolder bufferHolder = bufferHolder_;
    CHECK_AND_RETURN_RET_LOG(bufferHolder == AudioBufferHolder::AUDIO_SERVER_SHARED ||
        bufferHolder == AudioBufferHolder::AUDIO_SERVER_INDEPENDENT,
        false, "buffer holder error:%{public}d", bufferHolder);
    CHECK_AND_RETURN_RET_LOG(dataMem_ != nullptr, false, "dataMem_ is nullptr.");
    CHECK_AND_RETURN_RET_LOG(statusInfoMem_ != nullptr, false, "statusInfoMem_ is nullptr.");

    return messageParcel.WriteUint32(bufferHolder) &&
        messageParcel.WriteUint32(totalSizeInFrame_) &&
        messageParcel.WriteUint32(byteSizePerFrame_) &&
        messageParcel.WriteFileDescriptor(dataMem_->GetFd()) &&
        messageParcel.WriteFileDescriptor(statusInfoMem_->GetFd());
}

OHAudioBufferBase *OHAudioBufferBase::Unmarshalling(Parcel &parcel)
{
    AUDIO_DEBUG_LOG("ReadFromParcel start.");
    MessageParcel &messageParcel = static_cast<MessageParcel &>(parcel);
    uint32_t holder = messageParcel.ReadUint32();
    AudioBufferHolder bufferHolder = static_cast<AudioBufferHolder>(holder);
    if (bufferHolder != AudioBufferHolder::AUDIO_SERVER_SHARED &&
        bufferHolder != AudioBufferHolder::AUDIO_SERVER_INDEPENDENT) {
        AUDIO_ERR_LOG("ReadFromParcel buffer holder error:%{public}d", bufferHolder);
        return nullptr;
    }
    bufferHolder = bufferHolder == AudioBufferHolder::AUDIO_SERVER_SHARED ?
         AudioBufferHolder::AUDIO_CLIENT : bufferHolder;
    uint32_t totalSizeInFrame = messageParcel.ReadUint32();
    uint32_t byteSizePerFrame = messageParcel.ReadUint32();

    int dataFd = messageParcel.ReadFileDescriptor();
    int infoFd = messageParcel.ReadFileDescriptor();

    int minfd = 2; // ignore stdout, stdin and stderr.
    CHECK_AND_RETURN_RET_LOG(dataFd > minfd, nullptr, "invalid dataFd: %{public}d", dataFd);

    if (infoFd != INVALID_FD) {
        CHECK_AND_RETURN_RET_LOG(infoFd > minfd, nullptr, "invalid infoFd: %{public}d", infoFd);
    }
    auto buffer = new(std::nothrow) OHAudioBufferBase(bufferHolder, totalSizeInFrame, byteSizePerFrame);
    if (buffer == nullptr || buffer->Init(dataFd, infoFd, 0) != SUCCESS || buffer->basicBufferInfo_ == nullptr) {
        AUDIO_ERR_LOG("failed to init.");
        if (buffer != nullptr) delete buffer;
        CloseFd(dataFd);
        CloseFd(infoFd);
        return nullptr;
    }

    if (totalSizeInFrame != buffer->basicBufferInfo_->totalSizeInFrame ||
        byteSizePerFrame != buffer->basicBufferInfo_->byteSizePerFrame) {
        AUDIO_WARNING_LOG("data in shared memory diff.");
    } else {
        AUDIO_DEBUG_LOG("Read some data done.");
    }
    CloseFd(dataFd);
    CloseFd(infoFd);
    AUDIO_DEBUG_LOG("ReadFromParcel done.");
    return buffer;
}

void* OHAudioBufferBase::GetStatusInfoExtPtr()
{
    CHECK_AND_RETURN_RET_LOG(statusInfoMem_ != nullptr, nullptr, "not inited");
    return (statusInfoMem_->GetBase() + sizeof(BasicBufferInfo));
}

OHAudioBufferBase::InitializationInfo OHAudioBufferBase::GetInitializationInfo()
{
    InitializationInfo info = {
        .bufferHolder = bufferHolder_,
        .totalSizeInFrame = totalSizeInFrame_,
        .byteSizePerFrame = byteSizePerFrame_,
        .dataFd = dataMem_->GetFd(),
        .infoFd = statusInfoMem_->GetFd()
    };

    return info;
}

uint32_t OHAudioBufferBase::GetSessionId()
{
    return sessionId_;
}

int32_t OHAudioBufferBase::SetSessionId(uint32_t sessionId)
{
    sessionId_ = sessionId;
    return SUCCESS;
}

AudioBufferHolder OHAudioBufferBase::GetBufferHolder()
{
    return bufferHolder_;
}

int32_t OHAudioBufferBase::GetSizeParameter(uint32_t &totalSizeInFrame, uint32_t &byteSizePerFrame)
{
    totalSizeInFrame = totalSizeInFrame_;
    byteSizePerFrame = byteSizePerFrame_;

    return SUCCESS;
}

uint32_t OHAudioBufferBase::GetTotalSizeInFrame()
{
    return totalSizeInFrame_;
}

std::atomic<StreamStatus> *OHAudioBufferBase::GetStreamStatus()
{
    if (basicBufferInfo_ == nullptr) {
        return nullptr;
    }
    return &basicBufferInfo_->streamStatus;
}

uint32_t OHAudioBufferBase::GetUnderrunCount()
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, 0,
        "Get nullptr, buffer is not inited.");
    return basicBufferInfo_->underrunCount.load();
}

bool OHAudioBufferBase::SetUnderrunCount(uint32_t count)
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, false,
        "Get nullptr, buffer is not inited.");
    basicBufferInfo_->underrunCount.store(count);
    return true;
}

bool OHAudioBufferBase::GetHandleInfo(uint64_t &frames, int64_t &nanoTime)
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, false,
        "Get nullptr, failed to GetHandleInfo.");

    frames = basicBufferInfo_->handlePos.load();
    nanoTime = basicBufferInfo_->handleTime.load();
    return true;
}

void OHAudioBufferBase::SetHandleInfo(uint64_t frames, int64_t nanoTime)
{
    basicBufferInfo_->handlePos.store(frames);
    basicBufferInfo_->handleTime.store(nanoTime);
}

float OHAudioBufferBase::GetStreamVolume()
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, MAX_FLOAT_VOLUME, "buffer is not inited!");
    float vol = basicBufferInfo_->streamVolume.load();
    if (vol < MIN_FLOAT_VOLUME) {
        AUDIO_WARNING_LOG("vol < 0.0, invalid volume! using 0.0 instead.");
        return MIN_FLOAT_VOLUME;
    } else if (vol > MAX_FLOAT_VOLUME) {
        AUDIO_WARNING_LOG("vol > 0.0, invalid volume! using 1.0 instead.");
        return MAX_FLOAT_VOLUME;
    }
    return vol;
}

bool OHAudioBufferBase::SetStreamVolume(float streamVolume)
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, false, "buffer is not inited!");
    if (streamVolume < MIN_FLOAT_VOLUME || streamVolume > MAX_FLOAT_VOLUME) {
        AUDIO_ERR_LOG("invlaid volume:%{public}f", streamVolume);
        return false;
    }
    basicBufferInfo_->streamVolume.store(streamVolume);
    return true;
}

float OHAudioBufferBase::GetMuteFactor()
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, MAX_FLOAT_VOLUME, "buffer is not inited!");
    float factor = basicBufferInfo_->muteFactor.load();
    if (factor < MIN_FLOAT_VOLUME) {
        AUDIO_WARNING_LOG("vol < 0.0, invalid muteFactor! using 0.0 instead.");
        return MIN_FLOAT_VOLUME;
    } else if (factor > MAX_FLOAT_VOLUME) {
        AUDIO_WARNING_LOG("vol > 0.0, invalid muteFactor! using 1.0 instead.");
        return MAX_FLOAT_VOLUME;
    }
    return factor;
}

bool OHAudioBufferBase::SetMuteFactor(float muteFactor)
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, false, "buffer is not inited!");
    if (muteFactor != MIN_FLOAT_VOLUME && muteFactor != MAX_FLOAT_VOLUME) {
        AUDIO_ERR_LOG("invlaid factor:%{public}f", muteFactor);
        return false;
    }
    basicBufferInfo_->muteFactor.store(muteFactor);
    return true;
}

float OHAudioBufferBase::GetDuckFactor()
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, MAX_FLOAT_VOLUME, "buffer is not inited!");
    float factor = basicBufferInfo_->duckFactor.load();
    if (factor < MIN_FLOAT_VOLUME) {
        AUDIO_WARNING_LOG("vol < 0.0, invalid duckFactor! using 0.0 instead.");
        return MIN_FLOAT_VOLUME;
    } else if (factor > MAX_FLOAT_VOLUME) {
        AUDIO_WARNING_LOG("vol > 0.0, invalid duckFactor! using 1.0 instead.");
        return MAX_FLOAT_VOLUME;
    }
    return factor;
}

bool OHAudioBufferBase::SetDuckFactor(float duckFactor)
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, false, "buffer is not inited!");
    if (duckFactor < MIN_FLOAT_VOLUME || duckFactor > MAX_FLOAT_VOLUME) {
        AUDIO_ERR_LOG("invlaid factor:%{public}f", duckFactor);
        return false;
    }
    basicBufferInfo_->duckFactor.store(duckFactor);
    return true;
}

int32_t OHAudioBufferBase::GetWritableDataFrames()
{
    int32_t result = -1; // failed
    uint64_t write = basicBufferInfo_->curWriteFrame.load();
    uint64_t read = basicBufferInfo_->curReadFrame.load();
    CHECK_AND_RETURN_RET_LOG(write >= read, result, "invalid write and read position.");
    uint32_t temp = write - read;
    CHECK_AND_RETURN_RET_LOG(temp <= INT32_MAX && temp <= totalSizeInFrame_,
        result, "failed to GetWritableDataFrames.");
    result = static_cast<int32_t>(totalSizeInFrame_ - temp);
    return result;
}

int32_t OHAudioBufferBase::GetReadableDataFrames()
{
    int32_t result = -1; // failed
    uint64_t write = basicBufferInfo_->curWriteFrame.load();
    uint64_t read = basicBufferInfo_->curReadFrame.load();
    CHECK_AND_RETURN_RET_LOG(write >= read, result, "invalid write and read position.");
    uint32_t temp = write - read;
    CHECK_AND_RETURN_RET_LOG(temp <= INT32_MAX && temp <= totalSizeInFrame_,
        result, "failed to GetWritableDataFrames.");
    result = static_cast<int32_t>(temp);
    return result;
}

int32_t OHAudioBufferBase::ResetCurReadWritePos(uint64_t readFrame, uint64_t writeFrame, bool wakeFutex)
{
    CHECK_AND_RETURN_RET_LOG(readFrame <= writeFrame && writeFrame - readFrame < totalSizeInFrame_,
        ERR_INVALID_PARAM, "Invalid read or write position:read%{public}" PRIu64" write%{public}" PRIu64".",
        readFrame, writeFrame);
    uint64_t tempBase = (readFrame / totalSizeInFrame_) * totalSizeInFrame_;
    basicBufferInfo_->basePosInFrame.store(tempBase);
    basicBufferInfo_->curWriteFrame.store(writeFrame);
    basicBufferInfo_->curReadFrame.store(readFrame);

    AUDIO_DEBUG_LOG("Reset position:read%{public}" PRIu64" write%{public}" PRIu64".", readFrame, writeFrame);

    CHECK_AND_RETURN_RET(wakeFutex, SUCCESS);

    WakeFutexIfNeed();

    return SUCCESS;
}

uint64_t OHAudioBufferBase::GetCurWriteFrame()
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, 0, "basicBufferInfo_ is null");
    return basicBufferInfo_->curWriteFrame.load();
}

uint64_t OHAudioBufferBase::GetCurReadFrame()
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, 0, "basicBufferInfo_ is null");
    return basicBufferInfo_->curReadFrame.load();
}

uint64_t OHAudioBufferBase::GetBasePosInFrame()
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, 0, "basicBufferInfo_ is null");
    return basicBufferInfo_->basePosInFrame.load();
}

int32_t OHAudioBufferBase::SetCurWriteFrame(uint64_t writeFrame, bool wakeFutex)
{
    uint64_t basePos = basicBufferInfo_->basePosInFrame.load();
    uint64_t oldWritePos = basicBufferInfo_->curWriteFrame.load();
    if (writeFrame == oldWritePos) {
        return SUCCESS;
    }
    CHECK_AND_RETURN_RET_LOG(writeFrame > oldWritePos, ERR_INVALID_PARAM, "Too small writeFrame:%{public}" PRIu64".",
        writeFrame);

    uint64_t deltaToBase = writeFrame - basePos; // writeFrame % spanSizeInFrame_ --> 0

    // check new pos in range: base ~ base + 2*total
    CHECK_AND_RETURN_RET_LOG(deltaToBase < (totalSizeInFrame_ + totalSizeInFrame_),
        ERR_INVALID_PARAM, "Invalid writeFrame %{public}" PRIu64" out of base range.", writeFrame);

    // check new pos in (read + cache) range: read ~ read + totalSize - 1*spanSize
    uint64_t curRead = basicBufferInfo_->curReadFrame.load();
    CHECK_AND_RETURN_RET_LOG(writeFrame >= curRead && writeFrame - curRead <= totalSizeInFrame_,
        ERR_INVALID_PARAM, "Invalid writeFrame %{public}" PRIu64" out of cache range, curRead %{public}" PRIu64".",
        writeFrame, curRead);

    basicBufferInfo_->curWriteFrame.store(writeFrame);

    CHECK_AND_RETURN_RET(wakeFutex, SUCCESS);

    WakeFutexIfNeed();

    return SUCCESS;
}

int32_t OHAudioBufferBase::SetCurReadFrame(uint64_t readFrame, bool wakeFutex)
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, ERR_INVALID_PARAM, "basicBufferInfo_ is nullptr");
    uint64_t oldBasePos = basicBufferInfo_->basePosInFrame.load();
    uint64_t oldReadPos = basicBufferInfo_->curReadFrame.load();
    if (readFrame == oldReadPos) {
        return SUCCESS;
    }

    // new read position should not be bigger than write position or less than old read position
    CHECK_AND_RETURN_RET_LOG(readFrame >= oldReadPos && readFrame <= basicBufferInfo_->curWriteFrame.load(),
        ERR_INVALID_PARAM, "Invalid readFrame %{public}" PRIu64".", readFrame);

    uint64_t deltaToBase = readFrame - oldBasePos;

    if (deltaToBase >= totalSizeInFrame_) {
        basicBufferInfo_->basePosInFrame.store(oldBasePos + totalSizeInFrame_); // move base position
    }

    basicBufferInfo_->curReadFrame.store(readFrame);

    CHECK_AND_RETURN_RET(wakeFutex, SUCCESS);

    WakeFutexIfNeed();

    return SUCCESS;
}

int32_t OHAudioBufferBase::GetOffsetByFrame(uint64_t posInFrame, size_t &offset)
{
    uint64_t basePos = basicBufferInfo_->basePosInFrame.load();
    uint64_t maxDelta = 2 * totalSizeInFrame_; // 0 ~ 2*totalSizeInFrame_
    CHECK_AND_RETURN_RET_LOG(posInFrame >= basePos && posInFrame - basePos < maxDelta,
        ERR_INVALID_PARAM, "Invalid position:%{public}" PRIu64".", posInFrame);

    uint32_t deltaToBase = posInFrame - basePos;
    if (deltaToBase >= totalSizeInFrame_) {
        deltaToBase -= totalSizeInFrame_;
    }
    CHECK_AND_RETURN_RET_LOG(deltaToBase < UINT32_MAX && deltaToBase < totalSizeInFrame_, ERR_INVALID_PARAM,
        "invalid deltaToBase, posInFrame %{public}" PRIu64" basePos %{public}" PRIu64".", posInFrame, basePos);

    offset = deltaToBase * byteSizePerFrame_;
    return SUCCESS;
}

int32_t OHAudioBufferBase::GetOffsetByFrameForWrite(uint64_t writePosInFrame, size_t &offset)
{
    uint64_t basePos = basicBufferInfo_->basePosInFrame.load();
    uint64_t readPos = basicBufferInfo_->curReadFrame.load();
    uint64_t maxWriteDelta = 2 * totalSizeInFrame_; // 0 ~ 2*totalSizeInFrame_
    CHECK_AND_RETURN_RET_LOG(writePosInFrame >= basePos && writePosInFrame - basePos < maxWriteDelta &&
        writePosInFrame >= readPos, ERR_INVALID_PARAM, "Invalid write position:%{public}" PRIu64".", writePosInFrame);
    return GetOffsetByFrame(writePosInFrame, offset);
}

int32_t OHAudioBufferBase::GetOffsetByFrameForRead(uint64_t readPosInFrame, size_t &offset)
{
    uint64_t basePos = basicBufferInfo_->basePosInFrame.load();
    CHECK_AND_RETURN_RET_LOG(readPosInFrame >= basePos && readPosInFrame - basePos < totalSizeInFrame_,
        ERR_INVALID_PARAM, "Invalid read position:%{public}" PRIu64".", readPosInFrame);
    return GetOffsetByFrame(readPosInFrame, offset);
}

int32_t OHAudioBufferBase::GetBufferByOffset(size_t offset, size_t dataLength, RingBufferWrapper &buffer)
{
    CHECK_AND_RETURN_RET_LOG(offset < totalSizeInByte_, ERR_INVALID_PARAM, "invalid offset:%{public}zu", offset);
    CHECK_AND_RETURN_RET_LOG((dataLength <= totalSizeInByte_) && (dataLength > 0), ERR_INVALID_PARAM,
        "invalid dataLength: %{public}zu", dataLength);

    size_t bufLengthToDataBaseEnd = (totalSizeInByte_ - offset);

    RingBufferWrapper bufferWrapper;
    bufferWrapper.dataLength = dataLength;

    bufferWrapper.basicBufferDescs[0].buffer = dataBase_ + offset;
    bufferWrapper.basicBufferDescs[0].bufLength = std::min(bufLengthToDataBaseEnd, dataLength);

    if (dataLength > bufLengthToDataBaseEnd) {
        bufferWrapper.basicBufferDescs[1].buffer = dataBase_;
        bufferWrapper.basicBufferDescs[1].bufLength = dataLength - bufLengthToDataBaseEnd;
    } else {
        bufferWrapper.basicBufferDescs[1].buffer = nullptr;
        bufferWrapper.basicBufferDescs[1].bufLength = 0;
    }

    buffer = bufferWrapper;

    return SUCCESS;
}

int32_t OHAudioBufferBase::TryGetContinuousBufferByOffset(size_t offset, size_t dataLength, BufferDesc &bufferDesc)
{
    RingBufferWrapper buffer;
    int32_t ret = GetBufferByOffset(offset, dataLength, buffer);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "failed!");

    size_t firstBuffLenth = buffer.basicBufferDescs[0].bufLength;
    CHECK_AND_RETURN_RET_LOG(dataLength == firstBuffLenth, ERR_INVALID_PARAM,
        "err dataLength: %{public}zu firstBuffLenth: %{public}zu",
        dataLength, firstBuffLenth);

    bufferDesc.buffer = buffer.basicBufferDescs[0].buffer;
    bufferDesc.bufLength = dataLength;
    bufferDesc.dataLength = dataLength;
    return SUCCESS;
}

// [beginPosInFrame, endPosInFrame)
int32_t OHAudioBufferBase::GetBufferByFrame(uint64_t beginPosInFrame, uint64_t sizeInFrame, RingBufferWrapper &buffer)
{
    CHECK_AND_RETURN_RET_LOG((sizeInFrame > 0) && (sizeInFrame <= totalSizeInFrame_),
        ERR_INVALID_PARAM, "invalid param begin: %{public}" PRIu64 "sizeInFrame: %{public}" PRIu64 "",
        beginPosInFrame, sizeInFrame);

    size_t offset;
    int32_t ret = GetOffsetByFrame(beginPosInFrame, offset);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "getOffset err: %{public}d", ret);

    size_t dataLength = sizeInFrame * byteSizePerFrame_;

    return GetBufferByOffset(offset, dataLength, buffer);
}

int32_t OHAudioBufferBase::GetAllWritableBufferFromPosFrame(uint64_t writePosInFrame, RingBufferWrapper &buffer)
{
    uint64_t basePos = basicBufferInfo_->basePosInFrame.load();
    uint64_t readPos = basicBufferInfo_->curReadFrame.load();
    uint64_t maxWriteDelta = 2 * totalSizeInFrame_; // 0 ~ 2*totalSizeInFrame_
    CHECK_AND_RETURN_RET_LOG(writePosInFrame >= basePos && writePosInFrame - basePos < maxWriteDelta &&
        writePosInFrame >= readPos && (readPos + totalSizeInFrame_ >= writePosInFrame),
        ERR_INVALID_PARAM, "Invalid write position:%{public}" PRIu64".", writePosInFrame);

    uint64_t sizeInFrame = readPos + totalSizeInFrame_ - writePosInFrame;
    if (sizeInFrame == 0) {
        buffer.Reset();
        return SUCCESS;
    }

    return GetBufferByFrame(writePosInFrame, sizeInFrame, buffer);
}

int32_t OHAudioBufferBase::GetAllWritableBuffer(RingBufferWrapper &buffer)
{
    uint64_t writePosInFrame = GetCurWriteFrame();
    return GetAllWritableBufferFromPosFrame(writePosInFrame, buffer);
}

int32_t OHAudioBufferBase::GetAllReadableBufferFromPosFrame(uint64_t readPosInFrame, RingBufferWrapper &buffer)
{
    uint64_t basePos = basicBufferInfo_->basePosInFrame.load();
    uint64_t writePos = basicBufferInfo_->curWriteFrame.load();

    CHECK_AND_RETURN_RET_LOG((readPosInFrame >= basePos) && (readPosInFrame - basePos < totalSizeInFrame_) &&
        (readPosInFrame <= writePos),
        ERR_INVALID_PARAM, "Invalid read position:%{public}" PRIu64".", readPosInFrame);

    uint64_t sizeInFrame = writePos - readPosInFrame;
    if (sizeInFrame == 0) {
        buffer.Reset();
        return SUCCESS;
    }

    return GetBufferByFrame(readPosInFrame, sizeInFrame, buffer);
}

int32_t OHAudioBufferBase::GetAllReadableBuffer(RingBufferWrapper &buffer)
{
    uint64_t readPosInFrame = GetCurReadFrame();
    return GetAllReadableBufferFromPosFrame(readPosInFrame, buffer);
}

int64_t OHAudioBufferBase::GetLastWrittenTime()
{
    return lastWrittenTime_;
}

void OHAudioBufferBase::SetLastWrittenTime(int64_t time)
{
    lastWrittenTime_ = time;
}

uint32_t OHAudioBufferBase::GetSyncWriteFrame()
{
    if (bufferHolder_ != AUDIO_SERVER_ONLY_WITH_SYNC || syncWriteFrame_ == nullptr) {
        AUDIO_WARNING_LOG("sync info not support with holder: %{public}d", bufferHolder_);
        return 0;
    }
    return *syncWriteFrame_;
}

bool OHAudioBufferBase::SetSyncWriteFrame(uint32_t writeFrame)
{
    if (bufferHolder_ != AUDIO_SERVER_ONLY_WITH_SYNC || syncWriteFrame_ == nullptr) {
        AUDIO_WARNING_LOG("sync info not support with holder: %{public}d", bufferHolder_);
        return false;
    }
    *syncWriteFrame_ = writeFrame;
    return true;
}

uint32_t OHAudioBufferBase::GetSyncReadFrame()
{
    if (bufferHolder_ != AUDIO_SERVER_ONLY_WITH_SYNC || syncReadFrame_ == nullptr) {
        AUDIO_WARNING_LOG("sync info not support with holder: %{public}d", bufferHolder_);
        return 0;
    }
    return *syncReadFrame_;
}

bool OHAudioBufferBase::SetSyncReadFrame(uint32_t readFrame)
{
    if (bufferHolder_ != AUDIO_SERVER_ONLY_WITH_SYNC || syncReadFrame_ == nullptr) {
        AUDIO_WARNING_LOG("sync info not support with holder: %{public}d", bufferHolder_);
        return false;
    }
    *syncReadFrame_ = readFrame;
    return true;
}

std::atomic<uint32_t> *OHAudioBufferBase::GetFutex()
{
    if (basicBufferInfo_ == nullptr) {
        AUDIO_WARNING_LOG("basicBufferInfo_ is nullptr");
        return nullptr;
    }
    return &basicBufferInfo_->futexObj;
}

uint8_t *OHAudioBufferBase::GetDataBase()
{
    return dataBase_;
}

size_t OHAudioBufferBase::GetDataSize()
{
    return totalSizeInByte_;
}

void OHAudioBufferBase::GetRestoreInfo(RestoreInfo &restoreInfo)
{
    CHECK_AND_RETURN_LOG(basicBufferInfo_ != nullptr, "basicBufferInfo_ is nullptr");
    restoreInfo = basicBufferInfo_->restoreInfo;
}

void OHAudioBufferBase::SetRestoreInfo(RestoreInfo restoreInfo)
{
    CHECK_AND_RETURN_LOG(basicBufferInfo_ != nullptr, "basicBufferInfo_ is nullptr");
    basicBufferInfo_->restoreInfo = restoreInfo;
}

void OHAudioBufferBase::GetTimeStampInfo(uint64_t &position, uint64_t &timeStamp)
{
    CHECK_AND_RETURN_LOG(basicBufferInfo_ != nullptr, "basicBufferInfo_ is nullptr");
    position = basicBufferInfo_->position.load();
    timeStamp = basicBufferInfo_->timeStamp.load();
}

void OHAudioBufferBase::SetTimeStampInfo(uint64_t position, uint64_t timeStamp)
{
    CHECK_AND_RETURN_LOG(basicBufferInfo_ != nullptr, "basicBufferInfo_ is nullptr");
    basicBufferInfo_->position.store(position);
    basicBufferInfo_->timeStamp.store(timeStamp);
}

RestoreStatus OHAudioBufferBase::GetRestoreStatus()
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, RESTORE_ERROR, "basicBufferInfo_ is nullptr");
    return basicBufferInfo_->restoreStatus.load();
}

// Compare and swap restore status. If current restore status is NEED_RESTORE, turn it into RESTORING
// to avoid multiple restore.
RestoreStatus OHAudioBufferBase::CheckRestoreStatus()
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, RESTORE_ERROR, "basicBufferInfo_ is nullptr");
    RestoreStatus expectedStatus = NEED_RESTORE;
    basicBufferInfo_->restoreStatus.compare_exchange_strong(expectedStatus, RESTORING);
    return expectedStatus;
}

// Allow client to set restore status to NO_NEED_FOR_RESTORE if unnecessary restore happens. Restore status
// can be set to NEED_RESTORE only when it is currently NO_NEED_FOR_RESTORE(and vice versa).
RestoreStatus OHAudioBufferBase::SetRestoreStatus(RestoreStatus restoreStatus)
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, RESTORE_ERROR, "basicBufferInfo_ is nullptr");
    RestoreStatus expectedStatus = RESTORE_ERROR;
    if (restoreStatus == NEED_RESTORE) {
        expectedStatus = NO_NEED_FOR_RESTORE;
        basicBufferInfo_->restoreStatus.compare_exchange_strong(expectedStatus, NEED_RESTORE);
    } else if (restoreStatus == NO_NEED_FOR_RESTORE) {
        expectedStatus = RESTORING;
        basicBufferInfo_->restoreStatus.compare_exchange_strong(expectedStatus, NO_NEED_FOR_RESTORE);
    }
    return expectedStatus;
}

void OHAudioBufferBase::SetStopFlag(bool isNeedStop)
{
    CHECK_AND_RETURN_LOG(basicBufferInfo_ != nullptr, "basicBufferInfo_ is nullptr");
    basicBufferInfo_->isNeedStop.store(isNeedStop);
}

bool OHAudioBufferBase::GetStopFlag() const
{
    CHECK_AND_RETURN_RET_LOG(basicBufferInfo_ != nullptr, false, "basicBufferInfo_ is nullptr");
    bool isNeedStop = basicBufferInfo_->isNeedStop.exchange(false);
    return isNeedStop;
}

FutexCode OHAudioBufferBase::WaitFor(int64_t timeoutInNs, const OnIndexChange &pred)
{
    return FutexTool::FutexWait(GetFutex(), timeoutInNs, [&pred] () {
        return pred();
    });
}

void OHAudioBufferBase::InitBasicBufferInfo()
{
    // As basicBufferInfo_ is created from memory, we need to set the value with 0.
    basicBufferInfo_->basePosInFrame.store(0);
    basicBufferInfo_->curReadFrame.store(0);
    basicBufferInfo_->curWriteFrame.store(0);

    basicBufferInfo_->underrunCount.store(0);

    basicBufferInfo_->position.store(0);
    basicBufferInfo_->timeStamp.store(0);

    basicBufferInfo_->streamVolume.store(MAX_FLOAT_VOLUME);
    basicBufferInfo_->duckFactor.store(MAX_FLOAT_VOLUME);
    basicBufferInfo_->muteFactor.store(MAX_FLOAT_VOLUME);
}

void OHAudioBufferBase::WakeFutexIfNeed(uint32_t wakeVal)
{
    if (basicBufferInfo_) {
        FutexTool::FutexWake(&(basicBufferInfo_->futexObj), wakeVal);
    }
}

void OHAudioBufferBase::WakeFutex(uint32_t wakeVal)
{
    WakeFutexIfNeed(wakeVal);
}
} // namespace AudioStandard
} // namespace OHOS
