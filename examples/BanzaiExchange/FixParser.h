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

#pragma once
#include <string>
#include <vector>
#include "FixFieldNumbers.h"
#include "FixValues.h"

std::string getTimestamp();

class FixMessage {
public:
    std::string msgType;
    std::vector<std::pair<int, std::string>> fields;

    FixMessage() {}
    FixMessage(const std::string& type) : msgType(type) {}

    void setField(int tag, const std::string& value) {
        fields.push_back({tag, value});
    }

    std::string getField(int tag) const {
        for (const auto& pair : fields) {
            if (pair.first == tag) return pair.second;
        }
        return "";
    }
    
    std::vector<std::string> getFields(int tag) const {
        std::vector<std::string> values;
        for (const auto& pair : fields) {
            if (pair.first == tag) values.push_back(pair.second);
        }
        return values;
    }

    std::string toString(int seqNum, const std::string& senderCompID, const std::string& targetCompID) const;
    static FixMessage parse(const std::string& raw);
};
