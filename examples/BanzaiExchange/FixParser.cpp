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
 *
 * This product includes software developed by quickfixengine.org (http://www.quickfixengine.org/).
 */

#include "FixParser.h"
#include <sstream>
#include <iomanip>
#include <chrono>

std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d-%H:%M:%S", &tm);
    return std::string(buf);
}

std::string FixMessage::toString(int seqNum, const std::string& senderCompID, const std::string& targetCompID) const {
    std::stringstream body;
    body << FIX::FIELD::MsgType << "=" << msgType << "\x01";
    body << FIX::FIELD::SenderCompID << "=" << senderCompID << "\x01";
    body << FIX::FIELD::TargetCompID << "=" << targetCompID << "\x01";
    body << FIX::FIELD::MsgSeqNum << "=" << seqNum << "\x01";
    body << FIX::FIELD::SendingTime << "=" << getTimestamp() << "\x01";
    
    for (const auto& pair : fields) {
        body << pair.first << "=" << pair.second << "\x01";
    }
    
    std::string bodyStr = body.str();
    
    std::stringstream header;
    header << FIX::FIELD::BeginString << "=FIX.4.2\x01";
    header << FIX::FIELD::BodyLength << "=" << bodyStr.length() << "\x01";
    
    std::string fullMsg = header.str() + bodyStr;
    
    int checksum = 0;
    for (char c : fullMsg) {
        checksum += (unsigned char)c;
    }
    checksum %= 256;
    
    std::stringstream finalMsg;
    finalMsg << fullMsg << FIX::FIELD::CheckSum << "=" << std::setw(3) << std::setfill('0') << checksum << "\x01";
    
    return finalMsg.str();
}

FixMessage FixMessage::parse(const std::string& raw) {
    FixMessage msg;
    size_t start = 0;
    while (start < raw.length()) {
        size_t end = raw.find('\x01', start);
        if (end == std::string::npos) break;
        std::string pair = raw.substr(start, end - start);
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            int tag = std::stoi(pair.substr(0, eq));
            std::string val = pair.substr(eq + 1);
            if (tag == FIX::FIELD::MsgType) {
                msg.msgType = val;
            }
            msg.fields.push_back({tag, val});
        }
        start = end + 1;
    }
    return msg;
}
