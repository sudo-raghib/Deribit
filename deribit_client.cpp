#include "deribit_client.h"
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <iomanip>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

DeribitClient::DeribitClient(const std::string& client_id, const std::string& client_secret)
    : client_id_(client_id),
      client_secret_(client_secret),
      access_token_(""),
      api_url_("https://test.deribit.com/api/v2")
{
}

int DeribitClient::getNextRequestId() {
    return request_id_++;
}

std::string DeribitClient::buildAuthHeader() const {
    return "Authorization: Bearer " + access_token_;
}

bool DeribitClient::performRequest(
    const std::string& url,
    const nlohmann::json& request_body,
    nlohmann::json& response)
{
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();

    if (!curl) {
        std::cerr << "Failed to initialize CURL\n";
        return false;
    }

    std::string readBuffer;
    struct curl_slist* headers = NULL;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string method = request_body["method"].get<std::string>();
    if (method.find("private/") == 0 && method != "public/auth") {
        if (!access_token_.empty()) {
            std::string auth_header = buildAuthHeader();
            headers = curl_slist_append(headers, auth_header.c_str());
        } else {
            std::cerr << "Access token is empty. Please authenticate first.\n";
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return false;
        }
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    std::string request_body_str = request_body.dump();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body_str.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "CURL Error: " << curl_easy_strerror(res) << "\n";
        return false;
    }

    try {
        response = nlohmann::json::parse(readBuffer);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "JSON Parsing Error: " << e.what() << "\n";
        return false;
    }
}

bool DeribitClient::authenticate() {
    std::string url = api_url_ + "/public/auth";

    nlohmann::json request_body = {
        {"jsonrpc", "2.0"},
        {"id", getNextRequestId()},
        {"method", "public/auth"},
        {"params", {
            {"grant_type", "client_credentials"},
            {"client_id", client_id_},
            {"client_secret", client_secret_}
        }}
    };

    nlohmann::json response;
    bool success = performRequest(url, request_body, response);

    if (success && response.contains("result")) {
        access_token_ = response["result"]["access_token"];
        std::cout << "Authenticated successfully.\n";
        return true;
    } else {
        std::cerr << "Authentication failed. Response: " << response.dump(4) << "\n";
        return false;
    }
}

void DeribitClient::placeOrder(
    const std::string& instrument_name,
    const std::string& side,
    const std::string& type,
    double amount,
    double price,
    ResponseCallback callback)
{
    std::string url = api_url_ + "/private/" + side;

    nlohmann::json params = {
        {"instrument_name", instrument_name},
        {"amount", amount},
        {"type", type}
    };

    if (type == "limit") {
        params["price"] = price;
    }

    nlohmann::json request_body = {
        {"jsonrpc", "2.0"},
        {"id", getNextRequestId()},
        {"method", "private/" + side},
        {"params", params}
    };

    nlohmann::json response;
    bool success = performRequest(url, request_body, response);

    if (callback) {
        callback(response);
    }

    if (!success) {
        std::cerr << "Failed to place order.\n";
    }
}

void DeribitClient::cancelOrder(
    const std::string& order_id,
    ResponseCallback callback)
{
    std::string url = api_url_ + "/private/cancel";

    nlohmann::json params = {
        {"order_id", order_id}
    };

    nlohmann::json request_body = {
        {"jsonrpc", "2.0"},
        {"id", getNextRequestId()},
        {"method", "private/cancel"},
        {"params", params}
    };

    nlohmann::json response;
    bool success = performRequest(url, request_body, response);

    if (callback) {
        callback(response);
    }

    if (!success) {
        std::cerr << "Failed to cancel order.\n";
    }
}

void DeribitClient::modifyOrder(
    const std::string& order_id,
    double amount,
    double price,
    ResponseCallback callback)
{
    std::string url = api_url_ + "/private/edit";

    nlohmann::json params = {
        {"order_id", order_id},
        {"amount", amount},
        {"price", price}
    };

    nlohmann::json request_body = {
        {"jsonrpc", "2.0"},
        {"id", getNextRequestId()},
        {"method", "private/edit"},
        {"params", params}
    };

    nlohmann::json response;
    bool success = performRequest(url, request_body, response);

    if (callback) {
        callback(response);
    }

    if (!success) {
        std::cerr << "Failed to modify order.\n";
    }
}

void DeribitClient::getOrderBook(
    const std::string& instrument_name,
    ResponseCallback callback)
{
    std::string url = api_url_ + "/public/get_order_book";

    nlohmann::json params = {
        {"instrument_name", instrument_name}
    };

    nlohmann::json request_body = {
        {"jsonrpc", "2.0"},
        {"id", getNextRequestId()},
        {"method", "public/get_order_book"},
        {"params", params}
    };

    nlohmann::json response;
    bool success = performRequest(url, request_body, response);

    if (callback) {
        callback(response);
    }

    if (!success) {
        std::cerr << "Failed to get order book.\n";
    }
}

void DeribitClient::getPositions(
    const std::string& currency,
    ResponseCallback callback)
{
    std::string url = api_url_ + "/private/get_positions";

    nlohmann::json params = {
        {"currency", currency},
        {"kind", "future"}
    };

    nlohmann::json request_body = {
        {"jsonrpc", "2.0"},
        {"id", getNextRequestId()},
        {"method", "private/get_positions"},
        {"params", params}
    };

    nlohmann::json response;
    bool success = performRequest(url, request_body, response);

    if (callback) {
        callback(response);
    }

    if (!success) {
        std::cerr << "Failed to get positions.\n";
    }
}


void DeribitClient::getInstrument(
    const std::string& instrument_name,
    ResponseCallback callback)
{
    std::string url = api_url_ + "/public/get_instrument";

    nlohmann::json params = {
        {"instrument_name", instrument_name}
    };

    nlohmann::json request_body = {
        {"jsonrpc", "2.0"},
        {"id", getNextRequestId()},
        {"method", "public/get_instrument"},
        {"params", params}
    };

    nlohmann::json response;
    bool success = performRequest(url, request_body, response);

    if (callback) {
        callback(response);
    }

    if (!success) {
        std::cerr << "Failed to get instrument details.\n";
    }
}

void DeribitClient::getOrderState(
    const std::string& order_id,
    ResponseCallback callback)
{
    std::string url = api_url_ + "/private/get_order_state";

    nlohmann::json params = {
        {"order_id", order_id}
    };

    nlohmann::json request_body = {
        {"jsonrpc", "2.0"},
        {"id", getNextRequestId()},
        {"method", "private/get_order_state"},
        {"params", params}
    };

    nlohmann::json response;
    bool success = performRequest(url, request_body, response);

    if (callback) {
        callback(response);
    }

    if (!success) {
        std::cerr << "Failed to get order state.\n";
    }
}
