#include "MatchingEngine.h"
#include "utils.h"
#include "redis.h"

string MatchingEngine::printHello()
{
    return "Hello from C++, 2!";
}

vector<Trade> MatchingEngine::insertOrder(OrderRequest &order)
{
    logToTradeFile("try to insert order: " + order.id + " order type: " + order.getType() + " price: " + to_string(order.getPrice()) + " amount: " + to_string(order.getNotionalAmount()));
    std::lock_guard<std::mutex> lock(orderBookMutex);
        vector<Trade>
            trades;
    if (order.type == "BUY")
    {
        vector<Trade> buyTrades = matchBuyOrder(order);
        for (auto &&trade : buyTrades)
        {
            trades.emplace_back(move(trade));
        }
    }
    else if (order.type == "SELL")
    {
        vector<Trade> sellTrades = matchSellOrder(order);
        for (auto &&trade : sellTrades)
        {
            trades.emplace_back(move(trade));
        }
    }
    else
    {
        throw invalid_argument("Invalid order type");
    }
    // logToFile("finished to insert order: " + order.id);

    return trades;
}

vector<Trade> MatchingEngine::matchBuyOrder(OrderRequest &buyOrder)
{
    vector<Trade> trades;
    while (!sellOrders.empty() && buyOrder.notionalAmount > 0 && buyOrder.getPrice() >= (*sellOrders.begin()).getPrice())
    {
        OrderRequest sellOrder = *sellOrders.begin();
        sellOrders.erase(sellOrders.begin());
        trades.push_back(executeTrade(buyOrder, sellOrder));

        if (sellOrder.getNotionalAmount() > 0)
        {
            sellOrders.insert(sellOrder);
        }
        else
        {
            processFullyFulfilledOrder(sellOrder);
        }

        if (buyOrder.notionalAmount == 0)
        {
            processFullyFulfilledOrder(buyOrder);
            break;
        }
    };
    if (buyOrder.getNotionalAmount() > 0)
    {
        buyOrders.insert(buyOrder);
    }
    return trades;
}

vector<Trade> MatchingEngine::matchSellOrder(OrderRequest &sellOrder)
{
    vector<Trade> trades;
    while (!buyOrders.empty() && sellOrder.notionalAmount > 0 && sellOrder.getPrice() <= (*buyOrders.begin()).getPrice())
    {
        OrderRequest buyOrder = *buyOrders.begin();
        buyOrders.erase(buyOrders.begin());
        trades.push_back(executeTrade(buyOrder, sellOrder));

        if (buyOrder.getNotionalAmount() > 0)
        {
            buyOrders.insert(buyOrder);
        }
        else
        {
            processFullyFulfilledOrder(buyOrder);
        }

        if (sellOrder.getNotionalAmount() == 0)
        {
            processFullyFulfilledOrder(sellOrder);
            break;
        }
    };
    if (sellOrder.getNotionalAmount() > 0)
    {
        sellOrders.insert(sellOrder);
    }

    return trades;
}

Trade MatchingEngine::executeTrade(OrderRequest &buyOrder, OrderRequest &sellOrder)
{
    long tradePrice = sellOrder.price;
    int tradeAmount = min(buyOrder.notionalAmount, sellOrder.notionalAmount);
    string log = "Buy Order Id: " + buyOrder.getId() + "\n\nSell Order Id: " + sellOrder.getId() + "\n\nTrade Amount: " + to_string(tradeAmount) + "\n\nBuy Price: " + to_string(buyOrder.getPrice())+ "\n\nSell Price: " + to_string(sellOrder.getPrice())+"\n\n";
    logToFile(log);

    buyOrder.notionalAmount -= tradeAmount;
    sellOrder.notionalAmount -= tradeAmount;

    auto now = std::chrono::system_clock::now();
    auto minute_timestamp = std::chrono::duration_cast<std::chrono::minutes>(
                                now.time_since_epoch())
                                .count() *
                            60000;

    std::string symbolKey = fmt::format("candle:{}", buyOrder.symbol);
    auto redis = RedisClient::getInstance();

    std::string existingCandles;
    bool exists = redis->get(symbolKey, existingCandles);

    nlohmann::json candlesData;
    if (exists)
    {
        try
        {
            candlesData = nlohmann::json::parse(existingCandles);
        }
        catch (...)
        {
            candlesData = nlohmann::json::object();
        }
    }

    // Create datetime string for current minute
    std::time_t time = minute_timestamp / 1000;
    std::tm *tm = std::localtime(&time);
    std::ostringstream datetime_ss;
    datetime_ss << std::put_time(tm, "%Y-%m-%d %H:%M");
    std::string formatted_datetime = datetime_ss.str();

    double current_price = static_cast<double>(buyOrder.price);

    // Update min/max for current minute
    if (candlesData.contains(formatted_datetime))
    {
        double existing_min = candlesData[formatted_datetime]["min"].get<double>();
        double existing_max = candlesData[formatted_datetime]["max"].get<double>();
        candlesData[formatted_datetime]["min"] = std::min(existing_min, current_price);
        candlesData[formatted_datetime]["max"] = std::max(existing_max, current_price);
    }
    else
    {
        candlesData[formatted_datetime] = {
            {"symbol", buyOrder.symbol},
            {"min", current_price},
            {"max", current_price},
            {"timestamp", minute_timestamp}};
    }

    // Store updated candles data
    redis->set(symbolKey, candlesData.dump());

    // Store trade data as before
    std::string tradeJson = fmt::format(
        R"({{"id":"{}","symbol":"{}","price":{},"timestamp":{}}})",
        buyOrder.id,
        buyOrder.symbol,
        buyOrder.price,
        now.time_since_epoch().count());

    redis->pushTrade(tradeJson);

    return Trade(
        buyOrder.id,
        sellOrder.id,
        tradeAmount,
        tradePrice,
        buyOrder.getOriginalNotionalAmount(),
        sellOrder.getOriginalNotionalAmount());
}

void MatchingEngine::processFullyFulfilledOrder(OrderRequest &order)
{

    if (lastTenFulfilledOrders.size() >= 10)
    {
        lastTenFulfilledOrders.pop_front();
    }
    lastTenFulfilledOrders.emplace_back(std::move(order));
}

OrderBookSummary MatchingEngine::getMatchingEngineSummary()
{
    OrderBookSummary summary;
    summary.symbol = symbol;
    summary.lastTenFulfilledOrders = lastTenFulfilledOrders;

    auto tempBuyOrders = buyOrders;
    for (int i = 0; i < 5 && !tempBuyOrders.empty(); i++)
    {
        const OrderRequest &order = *tempBuyOrders.begin();
        summary.topBuys.emplace_back(
            order.getPrice(),
            order.getNotionalAmount(),
            order.getOriginalNotionalAmount(),
            order.getId());
        tempBuyOrders.erase(tempBuyOrders.begin());
    }

    auto tempSellOrders = sellOrders;
    for (int i = 0; i < 5 && !tempSellOrders.empty(); i++)
    {
        const OrderRequest &order = *tempSellOrders.begin();
        summary.lowestSells.emplace_back(
            order.getPrice(),
            order.getNotionalAmount(),
            order.getOriginalNotionalAmount(), order.getId());
        tempSellOrders.erase(tempSellOrders.begin());
    }

    return summary;
}
void MatchingEngine::clearOrderBooks() {
    std::lock_guard<std::mutex> lock(orderBookMutex);
    
    // Clear all order books
    buyOrders.clear();
    sellOrders.clear();
    
    // Log the clear operation
    logToFile("Order books cleared");
}