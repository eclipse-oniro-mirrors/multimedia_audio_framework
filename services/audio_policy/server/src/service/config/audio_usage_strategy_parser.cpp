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

#include "audio_usage_strategy_parser.h"
#include <string>
#include <iostream>
#include <vector>

namespace OHOS {
namespace AudioStandard {
bool AudioUsageStrategyParser::LoadConfiguration()
{
    doc_ = xmlReadFile(DEVICE_CONFIG_FILE, nullptr, 0);
    if (doc_ == nullptr) {
        AUDIO_ERR_LOG("xmlReadFile failed");
        return false;
    }

    return true;
}

bool AudioUsageStrategyParser::Parse()
{
    xmlNode *root = xmlDocGetRootElement(doc_);
    if (root == nullptr) {
        AUDIO_ERR_LOG("xmlDocGetRootElement Failed");
        return false;
    }

    if (!ParseInternal(root)) {
        return false;
    }
    return true;
}

void AudioUsageStrategyParser::Destroy()
{
    if (doc_ != nullptr) {
        xmlFreeDoc(doc_);
    }
}

bool AudioUsageStrategyParser::ParseInternal(xmlNode *node)
{
    xmlNode *currNode = node;
    for (; currNode; currNode = currNode->next) {
        if (XML_ELEMENT_NODE == currNode->type &&
            (!xmlStrcmp(currNode->name, reinterpret_cast<const xmlChar*>("adapter")))) {
                char *pValue = reinterpret_cast<char*>(xmlGetProp(currNode,
                    reinterpret_cast<xmlChar*>(const_cast<char*>("name"))));
                if (strcmp(pValue, "streamUsage") == 0) {
                    ParserStreamUsageList(currNode->xmlChildrenNode);
                } else if (strcmp(pValue, "sourceType") == 0) {
                    ParserSourceTypeList(currNode->xmlChildrenNode);
                }
            } else {
                ParseInternal((currNode->xmlChildrenNode));
            }
    }
    return true;
}

void AudioUsageStrategyParser::ParserStreamUsageList(xmlNode *node)
{
    xmlNode *strategyNode = node;
    std::string streamUsageName = "";
    while (strategyNode != nullptr) {
        if (strategyNode->type == XML_ELEMENT_NODE &&
            (!xmlStrcmp(strategyNode->name, reinterpret_cast<const xmlChar*>("strategy")))) {
            char *strategyName = reinterpret_cast<char*>(xmlGetProp(strategyNode,
                reinterpret_cast<xmlChar*>(const_cast<char*>("name"))));
            
            char *streamUsages = reinterpret_cast<char*>(xmlGetProp(strategyNode,
                reinterpret_cast<xmlChar*>(const_cast<char*>("streamUsage"))));
            ParserStreamUsageInfo(strategyName, streamUsages);
            xmlFree(strategyName);
            xmlFree(streamUsages);
        }
        strategyNode = strategyNode->next;
    }
}

void AudioUsageStrategyParser::ParserSourceTypeList(xmlNode *node)
{
    xmlNode *strategyNode = node;
    std::string streamUsageName = "";
    while (strategyNode != nullptr) {
        if (strategyNode->type == XML_ELEMENT_NODE &&
            (!xmlStrcmp(strategyNode->name, reinterpret_cast<const xmlChar*>("strategy")))) {
            char *strategyName = reinterpret_cast<char*>(xmlGetProp(strategyNode,
                reinterpret_cast<xmlChar*>(const_cast<char*>("name"))));
            char *sourceTypes = reinterpret_cast<char*>(xmlGetProp(strategyNode,
                reinterpret_cast<xmlChar*>(const_cast<char*>("sourceType"))));
            ParserSourceTypeInfo(strategyName, sourceTypes);
            xmlFree(strategyName);
            xmlFree(sourceTypes);
        }
        strategyNode = strategyNode->next;
    }
}

std::vector<std::string> AudioUsageStrategyParser::split(const std::string &line, const std::string &sep)
{
    std::vector<std::string> buf;
    int temp = 0;
    std::string::size_type pos = 0;
    while (true) {
        pos = line.find(sep, temp);
        if (pos == std::string::npos) {
            break;
        }
        buf.push_back(line.substr(temp, pos-temp));
        temp = pos + sep.length();
    }
    buf.push_back(line.substr(temp, line.length()));
    return buf;
}

void AudioUsageStrategyParser::ParserStreamUsage(const std::vector<std::string> &buf,
    const std::string &routerName)
{
    StreamUsage usage;
    for (auto &name : buf) {
        auto pos = streamUsageMap.find(name);
        if (pos != streamUsageMap.end()) {
            usage = pos->second;
        }
        renderConfigMap_[usage] = routerName;
    }
}

void AudioUsageStrategyParser::ParserStreamUsageInfo(const std::string &strategyName,
    const std::string &streamUsage)
{
    std::vector<std::string> buf = split(streamUsage, ",");
    if (strategyName == "MEDIA_RENDER") {
        ParserStreamUsage(buf, "MediaRenderRouters");
    } else if (strategyName == "CALL_RENDER") {
        ParserStreamUsage(buf, "CallRenderRouters");
    } else if (strategyName == "RING_RENDER") {
        ParserStreamUsage(buf, "RingRenderRouters");
    } else if (strategyName == "TONE_RENDER") {
        ParserStreamUsage(buf, "ToneRenderRouters");
    }
}

void AudioUsageStrategyParser::ParserSourceTypes(const std::vector<std::string> &buf,
    const std::string &sourceTypes)
{
    SourceType sourceType;
    for (auto &name : buf) {
        auto pos = sourceTypeMap.find(name);
        if (pos != sourceTypeMap.end()) {
            sourceType = pos->second;
        }
        capturerConfigMap_[sourceType] = sourceTypes;
    }
}

void AudioUsageStrategyParser::ParserSourceTypeInfo(const std::string &strategyName, const std::string &sourceTypes)
{
    std::vector<std::string> buf = split(sourceTypes, ",");
    if (strategyName == "RECORD_CAPTURE") {
        ParserSourceTypes(buf, "RecordCaptureRouters");
    } else if (strategyName == "CALL_CAPTURE") {
        ParserSourceTypes(buf, "CallCaptureRouters");
    }
}
} // namespace AudioStandard
} // namespace OHOS