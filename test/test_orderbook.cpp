#include <gtest/gtest.h>
#include  <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <string_view>


#include "Orderbook.h"

enum class ActionType{

    Add,
    Modify,
    Cancel,
};

struct Information{

    ActionType type_;
    Side side_;
    OrderType orderType_;
    Price price_;
    Quantity quantity_;
    OrderId orderId_;
};

using Informations = std::vector<Information>;

struct Result{

    std::size_t allCount_;
    std::size_t bidCount_;
    std::size_t askCount_;
};

using Results = std::vector<Result>;

struct InputHandler{

    private:
        std::uint64_t ToNumber(const std::string_view& str ) const{

            std::int64_t value{};
            std::from_chars(str.data(), str.data() + str.size(), value);
            if (value <0)
                throw std::logic_error("Value is below zero.");
            return static_cast<std::uint64_t>(value);
        }

        bool TryParseResult(const std::string_view& str, Result& result)const{

            if(str.at(0) != 'R')
                return false;
            
            auto values = Split(str, ' ');
            result.allCount_ = ToNumber(values.at(1));
            result.bidCount_ = ToNumber(values.at(2));
            result.askCount_ = ToNumber(values.at(3));
            return true;
        }

        bool TryParseInformation( const std::string_view& str, Information& info) const{

            auto value = str.at(0);
            auto values = Split(str, ' ');
            if(value == 'A'){

                info.type_ = ActionType::Add;
                info.side_ = ParseSide(values.at(1));
                info.orderType_ = ParseOrderType(values.at(2));
                info.price_ = ParsePrice(values.at(3));
                info.quantity_ = ParseQuantity(values.at(4));
                info.orderId_ = ParseOrderId(values.at(5));

            }
            else if ( value =='M'){
                info.type_ = ActionType::Modify;
                info.orderId_ = ParseOrderId(values.at(1));
                info.side_ = ParseSide(values.at(2));
                info.price_ = ParsePrice(values.at(3));
                info.quantity_ = ParseQuantity(values.at(4));

            }
            else if(value =='C'){
                info.type_ = ActionType::Cancel;
                info.orderId_ = ParseOrderId(values.at(1));

            }
            else return false;

            return true;
        }

        std::vector<std::string_view> Split(const std::string_view& str, char delimeter) const{

            std::vector<std::string_view> columns{};
            columns.reserve(5);
            std::size_t startIndex{}, endIndex{};
            while((endIndex = str.find(delimeter, startIndex))&& endIndex != std::string::npos){

                auto distance = endIndex - startIndex;
                auto column = str.substr(startIndex, distance);
                startIndex = endIndex +1;
                columns.push_back(column);
            }

            columns.push_back(str.substr(startIndex));
            return columns;
        }    

        Side ParseSide( const std::string_view& str)const{

            if ( str == "B")
                return Side::Buy;
            else if( str == "S")
                return Side::Sell;
            else throw std::logic_error("Unknown Side");

        }

        OrderType ParseOrderType( const std::string_view& str) const{

            if (str == "FillAndKill")
                return OrderType::FillAndKill;
            else if (str == "GoodTillCancel")
                return OrderType::GoodTillCancel;
            else if (str == "GoodForDay")
                return OrderType::GoodForDay;
            else if (str == "FillOrKill")
                return OrderType::FillOrKill;
            else if (str == "Market")
                return OrderType::Market;
            else throw std::logic_error("Unknown OrderType");
        }

        Price ParsePrice(const std::string_view& str )const{

            if (str.empty())
                throw std::logic_error("Unknown Price");
            return ToNumber(str);

        }

        Quantity ParseQuantity( const std::string_view& str) const{

            if (str.empty())
                throw std::logic_error("Unknown Quantity");
            return ToNumber(str);
        }
        Quantity ParseOrderId( const std::string_view& str) const{

            if (str.empty())
                throw std::logic_error("Unknown OrderId");
            return ToNumber(str);
        }

    public:

        std::tuple<Informations, Result> GetInformations(const std::filesystem:: path& path)const{

            Informations infos;
            infos.reserve(1'000);

            std::string line;
            std::ifstream file { path };
            while (std::getline(file, line)){

                if(line.empty())
                    break;
                const bool isResult = line.at(0) == 'R';
                const bool isUpdate = !isResult;

                if(isUpdate){

                    Information update;

                    auto isValid = TryParseInformation(line, update);
                    if(!isValid);
                        throw std::logic_error(std::format("Invalid update: {}", line));
                    
                    infos.push_back(update);

                }
                else{

                    if(!file.eof())
                        throw std::logic_error("Result must be at the end of the file only.");

                    Result result;

                    auto isValid = TryParseResult(line, result);
                    if(!isValid)
                        continue;

                    return {infos, result};
                }

            }
            throw std::logic_error("No result specified.");

        }
};

class OrderbookTestsFixture : public ::testing::TestWithParam<const char*>{

    private:
        const static inline std::filesystem::path Root { std::filesystem::current_path()};
        const static inline std::filesystem::path TestFolder{ "testfolder"};
    public:
        const static inline std::filesystem::path TestFolderPath{ Root / TestFolder};
};

TEST_P(OrderbookTestsFixture, OrderbookTestSuite){

    //Arrange
    const auto file = OrderbookTestsFixture::TestFolderPath / GetParam();
    

    InputHandler handler;
    const auto [updates, result] = handler.GetInformations(file);

    auto GetOrder = [](const Information& information){
        return std::make_shared<Order>(
            information.orderType_,
            information.orderId_,
            information.side_,
            information.price_,
            information.quantity_
        );
    };

    auto GetModifyOrder = [](const Information& information){

        return OrderModify{
            information.orderId_,
            information.side_,
            information.price_,
            information.quantity_
        };
    };

    //Act
    Orderbook orderbook;
    for (const auto& update: updates){

        switch(update.type_){
            case ActionType::Add:
                //const Trades& trades = 
                orderbook.AddOrder(GetOrder(update));
                break;
            case ActionType::Modify:
                //const Trades& trades = 
                orderbook.ModifyOrder(GetModifyOrder(update));
                break;
            case ActionType::Cancel:
                orderbook.CancelOrder(update.orderId_);
                break;
            default:
                throw std::logic_error("Unsupported Update");

        }
    }

    // Assert
    const auto& orderbookInfos = orderbook.GetOrderInfos();
    ASSERT_EQ(orderbook.Size(), result.allCount_);
    ASSERT_EQ(orderbookInfos.GetBids().size(), result.bidCount_);
    ASSERT_EQ(orderbookInfos.GetAsks().size(), result.askCount_);
}

INSTANTIATE_TEST_SUITE_P( Tests, OrderbookTestsFixture, ::testing::ValuesIn({
    "Match_GoodTillCancel.txt",
    "Match_FillAndKill.txt",
    "Match_fillOrKill_Hit.txt",
    "Match_FillOrKill_Miss.txt",
    "Cancel_Succes.txt",
    "Modify_Side.txt",
    "Match_Market.txt"
}));

