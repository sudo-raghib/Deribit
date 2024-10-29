#include "deribit_client.h"
#include <iostream>
#include <thread>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib> 
#include <limits.h>

int main() {
    

    std::string client_id;
    std::string client_secret;

    const char* client_id_c = std::getenv("DERIBIT_CLIENT_ID");
    const char* client_secret_c = std::getenv("DERIBIT_CLIENT_SECRET");

    if (client_id_c && client_secret_c) {
        client_id = client_id_c;
        client_secret = client_secret_c;
        std::cout << "Using credentials from environment variables." << std::endl;
    } else {
        // Try to read credentials from config.json
        std::ifstream config_file("config.json");
        if (config_file) {
            nlohmann::json config;
            try {
                config_file >> config;
                if (config.contains("client_id") && config.contains("client_secret")) {
                    client_id = config["client_id"];
                    client_secret = config["client_secret"];
                    std::cout << "Using credentials from config.json." << std::endl;
                } else {
                    std::cerr << "config.json must contain client_id and client_secret." << std::endl;
                    return 1;
                }
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Failed to open config.json." << std::endl;
            return 1;
        }
    }

    DeribitClient client(client_id, client_secret);

    if (!client.authenticate()) {
        return -1;
    }
    
std::string order_id;
client.getInstrument(
    "BTC-PERPETUAL",
    [](const nlohmann::json& response) {
        if (response.contains("result")) {
            double contract_size = response["result"]["contract_size"];
            double min_trade_amount = response["result"]["min_trade_amount"];
            double tick_size = response["result"]["tick_size"];
            std::cout << "Instrument Details:\n" << response.dump(4) << "\n";
            std::cout << "Contract Size: " << contract_size << "\n"
                      << "Min Trade Amount: " << min_trade_amount << "\n"
                      << "Tick Size: " << tick_size << "\n";
        } else {
            std::cerr << "Failed to retrieve instrument details.\n";
        }
    }
);

std::this_thread::sleep_for(std::chrono::seconds(2));
// Place a limit buy order
client.placeOrder(
    "BTC-PERPETUAL",
    "buy",
    "limit",
    10.0,    // amount, must be a multiple of contract_size
    5000.0,  // price, significantly below market price
    [&order_id](const nlohmann::json& response) {
        if (response.contains("result") && response["result"].contains("order")) {
            order_id = response["result"]["order"]["order_id"];
            std::cout << "Order placed successfully. Order ID: " << order_id << "\n";
            std::cout << "Place Order Response:\n" << response.dump(4) << "\n";
        } else {
            std::cerr << "Failed to place order.\n";
        }
    }
);

std::this_thread::sleep_for(std::chrono::seconds(2));

// Modify the order
if (!order_id.empty()) {
    double new_amount = 20.0;   // New amount, must be a multiple of contract_size
    double new_price = 51000.0; // New price

    client.modifyOrder(
        order_id,
        new_amount,
        new_price,
        [](const nlohmann::json& response) {
            if (response.contains("error")) {
                std::cerr << "Modify Order Error: " << response["error"]["message"]
                          << " (Reason: " << response["error"]["data"]["reason"] << ")\n";
            } else {
                std::cout << "Order modified successfully.\n";
                std::cout << "Modify Order Response:\n" << response.dump(4) << "\n";
            }
        }
    );

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Cancel the order 
    client.cancelOrder(
        order_id,
        [](const nlohmann::json& response) {
            if (response.contains("error")) {
                std::cerr << "Cancel Order Error: " << response["error"]["message"] << "\n";
            } else {
                std::cout << "Order canceled successfully.\n";
                std::cout << "Cancel Order Response:\n" << response.dump(4) << "\n";
            }
        }
    );

    std::this_thread::sleep_for(std::chrono::seconds(2));
} else {
    std::cerr << "Order ID is empty. Cannot modify or cancel order.\n";
}

// Place a market buy order
client.placeOrder(
    "BTC-PERPETUAL",
    "buy",
    "market",
    10.0,     // amount, must be a multiple of contract_size
    0.0,      // price is ignored for market orders
    [](const nlohmann::json& response) {
        if (response.contains("result") && response["result"].contains("order")) {
            std::cout << "Market order placed successfully.\n";
            std::cout << "Place Market Order Response:\n" << response.dump(4) << "\n";
        } else {
            std::cerr << "Failed to place market order.\n";
        }
    }
);

std::this_thread::sleep_for(std::chrono::seconds(2));

// Get current positions
client.getPositions(
    "BTC",
    [](const nlohmann::json& response) {
        std::cout << "Current Positions:\n";
        if (response.contains("result")) {
            std::cout << response.dump(4) << "\n";
        } else {
            std::cerr << "Failed to get positions.\n";
        }
    }
);

// Get the order book
client.getOrderBook(
    "BTC-PERPETUAL",
    [](const nlohmann::json& response) {
        if (response.contains("result")) {
            std::cout << "Order Book:\n" << response.dump(4) << "\n";
        } else {
            std::cerr << "Failed to retrieve order book.\n";
        }
    }
);

std::this_thread::sleep_for(std::chrono::seconds(5));

}
