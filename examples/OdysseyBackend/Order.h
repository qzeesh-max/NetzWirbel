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
#include <iostream>
#include <iomanip>
#include <cstdint>

class Order {
public:
    enum Side {
        buy = 1,
        sell = 2
    };

    Order(const std::string& clOrdId,
          const std::string& symbol,
          const std::string& owner,
          Side side,
          double price,
          uint32_t quantity)
        : clOrdId_(clOrdId),
          symbol_(symbol),
          owner_(owner),
          side_(side),
          price_(price),
          quantity_(quantity),
          openQuantity_(quantity),
          executedQuantity_(0),
          avgExecutedPrice_(0.0),
          lastExecutedPrice_(0.0),
          lastExecutedQuantity_(0),
          status_(0) {} // '0' = New

    const std::string& getClOrdID() const { return clOrdId_; }
    const std::string& getSymbol() const { return symbol_; }
    const std::string& getOwner() const { return owner_; }
    Side getSide() const { return side_; }
    double getPrice() const { return price_; }
    uint32_t getQuantity() const { return quantity_; }

    uint32_t getOpenQuantity() const { return openQuantity_; }
    uint32_t getExecutedQuantity() const { return executedQuantity_; }
    double getAvgExecutedPrice() const { return avgExecutedPrice_; }
    double getLastExecutedPrice() const { return lastExecutedPrice_; }
    uint32_t getLastExecutedQuantity() const { return lastExecutedQuantity_; }
    uint8_t getStatus() const { return status_; }

    bool isFilled() const { return quantity_ == executedQuantity_; }
    bool isClosed() const { return openQuantity_ == 0; }

    void execute(double price, uint32_t quantity) {
        avgExecutedPrice_ = ((quantity * price) + (avgExecutedPrice_ * executedQuantity_)) / (quantity + executedQuantity_);
        openQuantity_ -= quantity;
        executedQuantity_ += quantity;
        lastExecutedPrice_ = price;
        lastExecutedQuantity_ = quantity;
        status_ = isFilled() ? 2 : 1; // '2' = Filled, '1' = Partially filled
    }

    void cancel() {
        openQuantity_ = 0;
        status_ = 4; // '4' = Canceled
    }

    void replace(const std::string& clOrdId, double price, uint32_t quantity) {
        clOrdId_ = clOrdId;
        price_ = price;
        quantity_ = quantity;
        openQuantity_ = quantity_ - executedQuantity_;
        status_ = 0; // '0' = New (or replaced)
    }

    void setStatus(uint8_t status) {
        status_ = status;
    }

private:
    std::string clOrdId_;
    std::string symbol_;
    std::string owner_;
    Side side_;
    double price_;
    uint32_t quantity_;
    uint32_t openQuantity_;
    uint32_t executedQuantity_;
    double avgExecutedPrice_;
    double lastExecutedPrice_;
    uint32_t lastExecutedQuantity_;
    uint8_t status_;
};
