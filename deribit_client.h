#ifndef DERIBIT_CLIENT_H
#define DERIBIT_CLIENT_H

#include <string>
#include <functional>
#include <nlohmann/json.hpp>

using ResponseCallback = std::function<void(const nlohmann::json&)>;

class DeribitClient {
public:
    DeribitClient(const std::string& client_id, const std::string& client_secret);

    bool authenticate();

    void placeOrder(
        const std::string& instrument_name,
        const std::string& side,      // "buy" or "sell"
        const std::string& type,      // "limit", "market", etc.
        double amount,
        double price,
        ResponseCallback callback);

    void cancelOrder(
        const std::string& order_id,
        ResponseCallback callback);

    void modifyOrder(
        const std::string& order_id,
        double amount,
        double price,
        ResponseCallback callback);


    void getOrderBook(
        const std::string& instrument_name,
        ResponseCallback callback);

    void getPositions(
        const std::string& currency,  // "BTC", "ETH", etc.
        ResponseCallback callback);

    void getInstrument(
        const std::string& instrument_name,
        ResponseCallback callback);

    void getOrderState(
        const std::string& order_id,
        ResponseCallback callback);

private:
    std::string client_id_;
    std::string client_secret_;
    std::string access_token_;

    std::string api_url_;

    // Request ID generator
    int request_id_ = 1;
    int getNextRequestId();

    bool performRequest(
        const std::string& url,
        const nlohmann::json& request_body,
        nlohmann::json& response);

    std::string buildAuthHeader() const;
};

#endif
