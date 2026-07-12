/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include "../../examples/BanzaiExchange/FixParser.h"
#include "../../examples/BanzaiExchange/FixParser.cpp"

TEST(FixParserTest, ParseLogon) {
    std::string raw = "8=FIX.4.2\x01""9=12\x01""35=A\x01""10=000\x01";
    FixMessage msg = FixMessage::parse(raw);
    EXPECT_EQ(msg.msgType, "A");
    EXPECT_EQ(msg.getField(35), "A");
}

TEST(FixParserTest, SerializeNewOrderSingle) {
    FixMessage nos("D");
    nos.setField(11, "ORD1");
    nos.setField(55, "CSCO");
    nos.setField(38, "100");
    nos.setField(44, "10.5");
    
}

TEST(FixParserTest, ParseCancelReject) {
    std::string raw = "8=FIX.4.2\x01""9=34\x01""35=9\x01""11=CXL1\x01""41=ORD1\x01""39=8\x01""434=1\x01""10=000\x01";
    FixMessage msg = FixMessage::parse(raw);
    EXPECT_EQ(msg.msgType, "9");
    EXPECT_EQ(msg.getField(11), "CXL1");
    EXPECT_EQ(msg.getField(41), "ORD1");
    EXPECT_EQ(msg.getField(434), "1");
}

TEST(FixParserTest, SerializeCancelRequest) {
    FixMessage cxl("F");
    cxl.setField(11, "CXL1");
    cxl.setField(41, "ORD1");
    cxl.setField(55, "CSCO");
    
    std::string raw = cxl.toString(2, "SENDER", "TARGET");
    EXPECT_TRUE(raw.find("35=F\x01") != std::string::npos);
    EXPECT_TRUE(raw.find("11=CXL1\x01") != std::string::npos);
    EXPECT_TRUE(raw.find("41=ORD1\x01") != std::string::npos);
}

TEST(FixParserTest, SerializeCancelReplaceRequest) {
    FixMessage rep("G");
    rep.setField(11, "REP1");
    rep.setField(41, "ORD1");
    rep.setField(38, "200");
    rep.setField(44, "11.5");
    
    std::string raw = rep.toString(3, "SENDER", "TARGET");
    EXPECT_TRUE(raw.find("35=G\x01") != std::string::npos);
    EXPECT_TRUE(raw.find("11=REP1\x01") != std::string::npos);
    EXPECT_TRUE(raw.find("41=ORD1\x01") != std::string::npos);
    EXPECT_TRUE(raw.find("38=200\x01") != std::string::npos);
    EXPECT_TRUE(raw.find("44=11.5\x01") != std::string::npos);
}

TEST(FixParserTest, SerializeMarketDataRequestWithGroups) {
    FixMessage mdr("V");
    mdr.setField(262, "MDR1");
    mdr.setField(263, "0");
    mdr.setField(264, "0");
    mdr.setField(267, "2");
    mdr.setField(269, "0");
    mdr.setField(269, "1");
    mdr.setField(146, "1");
    mdr.setField(55, "CSCO");

    std::string raw = mdr.toString(4, "SENDER", "TARGET");
    EXPECT_TRUE(raw.find("35=V\x01") != std::string::npos);
    EXPECT_TRUE(raw.find("267=2\x01") != std::string::npos);
    
    size_t pos267 = raw.find("267=2\x01");
    size_t pos269_0 = raw.find("269=0\x01");
    size_t pos269_1 = raw.find("269=1\x01");
    EXPECT_TRUE(pos267 != std::string::npos);
    EXPECT_TRUE(pos269_0 != std::string::npos);
    EXPECT_TRUE(pos269_1 != std::string::npos);
    EXPECT_TRUE(pos267 < pos269_0);
    EXPECT_TRUE(pos269_0 < pos269_1);
}

TEST(FixParserTest, ParseBusinessMessageReject) {
    std::string raw = "8=FIX.4.2\x019=45\x01""35=j\x01""45=4\x01""372=V\x01""380=2\x01""58=stock is not known\x01""10=000\x01";
    FixMessage msg = FixMessage::parse(raw);
    EXPECT_EQ(msg.msgType, "j");
    EXPECT_EQ(msg.getField(45), "4");
    EXPECT_EQ(msg.getField(372), "V");
    EXPECT_EQ(msg.getField(380), "2");
    EXPECT_EQ(msg.getField(58), "stock is not known");
}
