/*******************************************************************************
*   (c) 2019 ZondaX GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#include "testcases.h"
#include "expected_output.h"
#include <fmt/core.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <hexutils.h>

std::string remove_prefix(const std::string &prefix, const Json::Value &s) {
    auto tmp = s.asString();
    if (tmp.empty())
        return "";
    assert(tmp.rfind(prefix, 0) == 0);
    return tmp.substr(prefix.size());
}

testcaseData_t ReadRawTestCase(const std::shared_ptr<Json::Value> &jsonSource, int index) {
    testcaseData_t answer;
    auto v = (*jsonSource)[index];
    auto description = std::string("");

    description = v["transaction"]["kind"].asString();
    description.erase(remove_if(description.begin(), description.end(), isspace), description.end());
    description = remove_prefix("string:", description);

    auto tx = v["transaction"];

    auto bytes_hexstring = v["bytes"].asString();
    assert(bytes_hexstring.size() % 2 == 0);
    auto blob = std::vector<uint8_t>(v["bytes"].asString().size() / 2);
    parseHexString(blob.data(), blob.size(), bytes_hexstring.c_str());

    auto chainid = remove_prefix("string:", tx["chainId"]);
    bool testnet = false;
    if (chainid != "iov-mainnet") {
        testnet = true;
    }

    auto feeTokens = tx["fee"]["tokens"];

    auto multisig = std::vector<uint64_t>();
    if (tx["multisig"].isArray()) {
        for (const auto &elem : tx["multisig"]) {
            multisig.push_back(elem.asUInt64());
        }
    }

    return {
            description,
            ////////
            chainid,
            remove_prefix("string:", tx["sender"]),
            remove_prefix("string:", tx["recipient"]),
            multisig,
            remove_prefix("string:", tx["memo"]),
            remove_prefix("string:", tx["amount"]["quantity"]),
            tx["amount"]["fractionalDigits"].asUInt64(),
            remove_prefix("string:", tx["amount"]["tokenTicker"]),
            remove_prefix("string:", feeTokens["quantity"]),
            feeTokens["fractionalDigits"].asUInt64(),
            remove_prefix("string:", feeTokens["tokenTicker"]),
            v["nonce"].asUInt64(),
            bytes_hexstring,
            true,
            testnet,
            blob,
    };
}

testcaseData_t ReadTestCaseData(const std::shared_ptr<Json::Value> &jsonSource, int index) {
    testcaseData_t tcd = ReadRawTestCase(jsonSource, index);
    // Anotate with expected ui output
    tcd.expected_ui_output = GenerateExpectedUIOutput(tcd);
    return tcd;
}

std::vector<testcase_t> GetJsonTestCases(const std::string &filename) {
    auto answer = std::vector<testcase_t>();

    Json::CharReaderBuilder builder;
    std::shared_ptr<Json::Value> obj(new Json::Value());

    std::ifstream inFile(filename);
    EXPECT_TRUE(inFile.is_open())
                        << "\n"
                        << "******************\n"
                        << "Failed to open : " << filename << std::endl
                        << "Check that your working directory points to the tests directory\n"
                        << "In CLion use $PROJECT_DIR$\\tests\n"
                        << "******************\n";
    if (!inFile.is_open()) { return answer; }

    // Retrieve all test cases
    JSONCPP_STRING errs;
    Json::parseFromStream(builder, inFile, obj.get(), &errs);
    std::cout << "Number of testcases: " << obj->size() << std::endl;
    answer.reserve(obj->size());

    for (int i = 0; i < obj->size(); i++) {
        auto v = (*obj)[i];

        auto description = std::string("");

        description = v["transaction"]["kind"].asString();
        description.erase(remove_if(description.begin(), description.end(), [](char v) -> bool {
            return v == ':' || v == ' ' || v == '/';
        }), description.end());

        answer.push_back(testcase_t{obj, i, description});
    }

    return answer;
}
