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
#include "audio_converter_parser.h"
#include <map>
#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace OHOS {
namespace AudioStandard {

static constexpr char AUDIO_CONVERTER_CONFIG_FILE[] = "/system/etc/audio/audio_converter_config.xml";

static constexpr int32_t FILE_CONTENT_ERROR = -2;
static constexpr int32_t FILE_PARSE_ERROR = -3;

enum XML_ERROR {
    XML_PARSE_RECOVER = 1 << 0,   // recover on errors
    XML_PARSE_NOERROR = 1 << 5,   // suppress error reports
    XML_PARSE_NOWARNING = 1 << 6, // suppress warning reports
    XML_PARSE_PEDANTIC = 1 << 7   // pedantic error reporting
};

static std::map<std::string, AudioChannelLayout> str2layout = {
    {"CH_LAYOUT_UNKNOWN", CH_LAYOUT_UNKNOWN},
    {"CH_LAYOUT_MONO", CH_LAYOUT_MONO},
    {"CH_LAYOUT_STEREO", CH_LAYOUT_STEREO},
    {"CH_LAYOUT_STEREO_DOWNMIX", CH_LAYOUT_STEREO_DOWNMIX},
    {"CH_LAYOUT_2POINT1", CH_LAYOUT_2POINT1},
    {"CH_LAYOUT_3POINT0", CH_LAYOUT_3POINT0},
    {"CH_LAYOUT_SURROUND", CH_LAYOUT_SURROUND},
    {"CH_LAYOUT_3POINT1", CH_LAYOUT_3POINT1},
    {"CH_LAYOUT_4POINT0", CH_LAYOUT_4POINT0},
    {"CH_LAYOUT_QUAD_SIDE", CH_LAYOUT_QUAD_SIDE},
    {"CH_LAYOUT_QUAD", CH_LAYOUT_QUAD},
    {"CH_LAYOUT_2POINT0POINT2", CH_LAYOUT_2POINT0POINT2},
    {"CH_LAYOUT_4POINT1", CH_LAYOUT_4POINT1},
    {"CH_LAYOUT_5POINT0", CH_LAYOUT_5POINT0},
    {"CH_LAYOUT_5POINT0_BACK", CH_LAYOUT_5POINT0_BACK},
    {"CH_LAYOUT_2POINT1POINT2", CH_LAYOUT_2POINT1POINT2},
    {"CH_LAYOUT_3POINT0POINT2", CH_LAYOUT_3POINT0POINT2},
    {"CH_LAYOUT_5POINT1", CH_LAYOUT_5POINT1},
    {"CH_LAYOUT_5POINT1_BACK", CH_LAYOUT_5POINT1_BACK},
    {"CH_LAYOUT_6POINT0", CH_LAYOUT_6POINT0},
    {"CH_LAYOUT_HEXAGONAL", CH_LAYOUT_HEXAGONAL},
    {"CH_LAYOUT_3POINT1POINT2", CH_LAYOUT_3POINT1POINT2},
    {"CH_LAYOUT_6POINT0_FRONT", CH_LAYOUT_6POINT0_FRONT},
    {"CH_LAYOUT_6POINT1", CH_LAYOUT_6POINT1},
    {"CH_LAYOUT_6POINT1_BACK", CH_LAYOUT_6POINT1_BACK},
    {"CH_LAYOUT_6POINT1_FRONT", CH_LAYOUT_6POINT1_FRONT},
    {"CH_LAYOUT_7POINT0", CH_LAYOUT_7POINT0},
    {"CH_LAYOUT_7POINT0_FRONT", CH_LAYOUT_7POINT0_FRONT},
    {"CH_LAYOUT_7POINT1", CH_LAYOUT_7POINT1},
    {"CH_LAYOUT_OCTAGONAL", CH_LAYOUT_OCTAGONAL},
    {"CH_LAYOUT_5POINT1POINT2", CH_LAYOUT_5POINT1POINT2},
    {"CH_LAYOUT_7POINT1_WIDE", CH_LAYOUT_7POINT1_WIDE},
    {"CH_LAYOUT_7POINT1_WIDE_BACK", CH_LAYOUT_7POINT1_WIDE_BACK},
    {"CH_LAYOUT_5POINT1POINT4", CH_LAYOUT_5POINT1POINT4},
    {"CH_LAYOUT_7POINT1POINT2", CH_LAYOUT_7POINT1POINT2},
    {"CH_LAYOUT_7POINT1POINT4", CH_LAYOUT_7POINT1POINT4},
    {"CH_LAYOUT_10POINT2", CH_LAYOUT_10POINT2},
    {"CH_LAYOUT_9POINT1POINT4", CH_LAYOUT_9POINT1POINT4},
    {"CH_LAYOUT_9POINT1POINT6", CH_LAYOUT_9POINT1POINT6},
    {"CH_LAYOUT_HEXADECAGONAL", CH_LAYOUT_HEXADECAGONAL},
    {"CH_LAYOUT_AMB_ORDER1_ACN_N3D", CH_LAYOUT_HOA_ORDER1_ACN_N3D},
    {"CH_LAYOUT_AMB_ORDER1_ACN_SN3D", CH_LAYOUT_HOA_ORDER1_ACN_SN3D},
    {"CH_LAYOUT_AMB_ORDER1_FUMA", CH_LAYOUT_HOA_ORDER1_FUMA},
    {"CH_LAYOUT_AMB_ORDER2_ACN_N3D", CH_LAYOUT_HOA_ORDER2_ACN_N3D},
    {"CH_LAYOUT_AMB_ORDER2_ACN_SN3D", CH_LAYOUT_HOA_ORDER2_ACN_SN3D},
    {"CH_LAYOUT_AMB_ORDER2_FUMA", CH_LAYOUT_HOA_ORDER2_FUMA},
    {"CH_LAYOUT_AMB_ORDER3_ACN_N3D", CH_LAYOUT_HOA_ORDER3_ACN_N3D},
    {"CH_LAYOUT_AMB_ORDER3_ACN_SN3D", CH_LAYOUT_HOA_ORDER3_ACN_SN3D},
    {"CH_LAYOUT_AMB_ORDER3_FUMA", CH_LAYOUT_HOA_ORDER3_FUMA},
};

AudioConverterParser::AudioConverterParser()
{
    AUDIO_INFO_LOG("AudioConverterParser created");
}
AudioConverterParser::~AudioConverterParser()
{
    AUDIO_INFO_LOG("AudioConverterParser released");
}

static int32_t LoadConfigCheck(xmlDoc* doc, xmlNode* currNode)
{
    CHECK_AND_RETURN_RET_LOG(currNode != nullptr, FILE_PARSE_ERROR,
        "error: could not parse file %{public}s", AUDIO_CONVERTER_CONFIG_FILE);
    bool ret = xmlStrcmp(currNode->name, reinterpret_cast<const xmlChar*>("audio_converter_conf"));
    CHECK_AND_RETURN_RET_LOG(!ret, FILE_CONTENT_ERROR,
        "Missing tag - audio_converter_conf: %{public}s", AUDIO_CONVERTER_CONFIG_FILE);
    CHECK_AND_RETURN_RET_LOG(currNode->xmlChildrenNode != nullptr, FILE_CONTENT_ERROR,
        "Missing node - audio_converter_conf: %s", AUDIO_CONVERTER_CONFIG_FILE);

    return 0;
}

static void LoadConfigLibrary(ConverterConfig &result, xmlNode* currNode)
{
    if (!xmlHasProp(currNode, reinterpret_cast<const xmlChar*>("name"))) {
        AUDIO_WARNING_LOG("missing information: library has no name attribute");
    } else if (!xmlHasProp(currNode, reinterpret_cast<const xmlChar*>("path"))) {
        AUDIO_WARNING_LOG("missing information: library has no path attribute");
    } else {
        std::string libName = reinterpret_cast<char*>
                              (xmlGetProp(currNode, reinterpret_cast<const xmlChar*>("name")));
        std::string libPath = reinterpret_cast<char*>
                              (xmlGetProp(currNode, reinterpret_cast<const xmlChar*>("path")));
        result.library = {libName, libPath};
    }
}

static void LoadConfigChannelLayout(ConverterConfig &result, xmlNode* currNode)
{
    if (!xmlHasProp(currNode, reinterpret_cast<const xmlChar*>("out_channel_layout"))) {
        AUDIO_ERR_LOG("missing information: config has no out_channel_layout attribute");
    } else {
        std::string strChannelLayout = reinterpret_cast<char*>
            (xmlGetProp(currNode, reinterpret_cast<const xmlChar*>("out_channel_layout")));
        if (str2layout.count(strChannelLayout) == 0) {
            AUDIO_ERR_LOG("unsupported format: invalid channel layout");
        } else {
            result.outChannelLayout = str2layout[strChannelLayout];
        }
    }
}

static void LoadConfigVersion(ConverterConfig &result, xmlNode* currNode)
{
    bool ret = xmlHasProp(currNode, reinterpret_cast<const xmlChar*>("version"));
    CHECK_AND_RETURN_LOG(ret, "missing information: audio_converter_conf node has no version attribute");

    float pVersion = atof(reinterpret_cast<char*>
        (xmlGetProp(currNode, reinterpret_cast<const xmlChar*>("version"))));
    result.version = pVersion;
}

int32_t AudioConverterParser::LoadConfig(ConverterConfig &result)
{
    xmlDoc *doc = nullptr;
    xmlNode *rootElement = nullptr;
    AUDIO_INFO_LOG("AudioConverterParser::LoadConfig");
    doc = xmlReadFile(AUDIO_CONVERTER_CONFIG_FILE, nullptr, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    CHECK_AND_RETURN_RET_LOG(doc != nullptr, FILE_PARSE_ERROR,
        "error: could not parse file %{public}s", AUDIO_CONVERTER_CONFIG_FILE);

    rootElement = xmlDocGetRootElement(doc);
    xmlNode *currNode = rootElement;

    int32_t ret = 0;

    if ((ret = LoadConfigCheck(doc, currNode)) != 0) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return ret;
    }

    LoadConfigVersion(result, currNode);
    currNode = currNode->xmlChildrenNode;

    while (currNode != nullptr) {
        if (currNode->type != XML_ELEMENT_NODE) {
            currNode = currNode->next;
            continue;
        }

        if (!xmlStrcmp(currNode->name, reinterpret_cast<const xmlChar*>("library"))) {
            LoadConfigLibrary(result, currNode);
        } else if (!xmlStrcmp(currNode->name, reinterpret_cast<const xmlChar*>("converter_conf"))) {
            LoadConfigChannelLayout(result, currNode);
        }

        currNode = currNode->next;
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return ret;
}

} // namespace AudioStandard
} // namespace OHOS