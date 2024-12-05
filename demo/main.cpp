#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include "read_csv.h"
#include "inline_query.h"
#include "print_table.h"
#include "make_table.h"
#include "connections.h"
#include "select_from_table.h"
#include "print_result.h"
#include "table_structure.h"
#include "joins.h"
#include <filesystem>
// Helper function to print section headers
void print_section(const std::string& section_name) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "  " << section_name << "\n";
    std::cout << std::string(80, '=') << "\n";
}

// Helper class for timing operations
class Timer {
    std::chrono::high_resolution_clock::time_point start_time;
public:
    Timer() : start_time(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed() {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end_time - start_time).count();
    }
};

int main() {
    try {
        Timer total_timer;

        print_section("1. Connection Setup");
        if (!KDBConnection::connect("localhost", 6000)) {
            throw std::runtime_error("Failed to connect to KDB+");
        }
        std::cout << "Successfully connected to KDB+ instance\n";

        print_section("2. Data Loading");
        Timer load_timer;
        
        // Load market data
        std::string current_path = std::filesystem::current_path().string();
        std::string quotes_file = current_path + "/demo/quotes_example.csv";
        std::string trades_file = current_path + "/demo/trades_example.csv";
        std::cout << quotes_file << std::endl;
        if (!read_csv("quotes", quotes_file.c_str(), true) ||
            !read_csv("trades", trades_file.c_str(), true)) {
            throw std::runtime_error("Failed to load market data");
        }
        std::cout << "Data loading completed in " << load_timer.elapsed() << " seconds\n";

        print_section("3. Basic Data Selection");
        
        // Demonstrate iloc (index-based selection)
        std::cout << "\nDemonstrating iloc - First 5 rows of trades:\n";
        std::vector<int> row_indices = {0, 1, 2, 3, 4};
        std::vector<int> all_cols = {};  // Empty for all columns
        print_result(iloc("trades", row_indices, all_cols));
        
        std::cout << "\nDemonstrating iloc - First 5 rows of quotes:\n";
        print_result(iloc("quotes", row_indices, all_cols));


        // Demonstrate loc with conditions
        std::cout << "\nDemonstrating loc - Quotes with large spreads (>0.1):\n";
        print_result(loc("quotes", "Ask_Price - Bid_Price > 0.1"));

        std::cout << "\nDemonstrating loc - High volume trades (>400):\n";
        print_result(loc("trades", "Trade_Size > 400"));

        std::cout << "\nDemonstrating loc - Combined conditions (large trades with wide spreads):\n";
        print_result(loc("trades", "Trade_Size > 300, Spread > 0.08"));

        print_section("4. Basic Market Metrics");
        
        // Calculate basic order book metrics
        inline_query(R"(
            // Update quotes with basic metrics
            update
                spread: Ask_Price - Bid_Price,
                mid_price: 0.5 * (Ask_Price + Bid_Price),
                quoted_value: (Bid_Size * Bid_Price + Ask_Size * Ask_Price),
                order_imbalance: (Bid_Size - Ask_Size) % (Bid_Size + Ask_Size),
                total_depth: Bid_Size + Ask_Size
            from `quotes;

            // Basic quote statistics
            quote_stats: select
                avg_spread: avg spread,
                max_spread: max spread,
                min_spread: min spread,
                avg_depth: avg total_depth,
                max_depth: max total_depth,
                avg_bid_size: avg Bid_Size,
                avg_ask_size: avg Ask_Size,
                price_range: (max mid_price) - (min mid_price)
            from quotes;

            // Order book imbalance metrics
            imbalance_metrics: select
                imbalance_ratio: order_imbalance,
                total_depth,
                weighted_imbalance: order_imbalance * log total_depth,
                mid_price,
                spread
            from quotes;

            // Trade metrics
            trade_metrics: select
                vwap: sum[Trade_Price * Trade_Size] % sum Trade_Size,
                twap: avg Trade_Price,
                num_trades: count i,
                total_volume: sum Trade_Size,
                avg_trade_size: avg Trade_Size,
                max_trade_size: max Trade_Size,
                min_trade_size: min Trade_Size,
                price_range: max[Trade_Price] - min[Trade_Price]
            from trades;

            // Time-based metrics (100ms buckets)
            time_metrics: select
                vwap: sum[Trade_Price * Trade_Size] % sum Trade_Size,
                twap: avg Trade_Price,
                trade_count: count i,
                volume: sum Trade_Size,
                avg_spread: avg spread,
                volume_imbalance: (sum[Trade_Size * Trade_Price >= Ask_Price] -
                                      sum[Trade_Size * Trade_Price <= Bid_Price]) %
                                     (sum Trade_Size),
                trade_count_imbalance: (sum[1 * Trade_Price >= Ask_Price] -
                                           sum[1 * Trade_Price <= Bid_Price]) %
                                          (count i)
            by 100 xbar `timestamp$Timestamp from trades lj quotes;

            // Price volatility analysis
            volatility_metrics: select
                high: max Trade_Price,
                low: min Trade_Price,
                open: first Trade_Price,
                close: last Trade_Price,
                volume: sum Trade_Size
            by 100 xbar `timestamp$Timestamp from trades
        )");

        std::cout << "\nQuote Statistics:\n";
        print_result(inline_query("quote_stats").get_result());

        std::cout << "\nTrade Metrics:\n";
        print_result(inline_query("trade_metrics").get_result());

        std::cout << "\nTime-based Metrics (Sample):\n";
        print_result(inline_query("5#time_metrics").get_result());

        print_section("4. Advanced Analysis");

        // Demonstrate asof join for trade/quote matching
        std::cout << "\nA. As-of Join - Match trades with prevailing quotes:\n";
        std::vector<std::string> join_cols = {};  // No additional join columns needed
        auto asof_result = joins::asof_join(
            "trades",
            "quotes",
            "trade_quote_asof",
            "Timestamp",
            "Timestamp",
            join_cols
        );
        if (asof_result) {
            print_result(inline_query("5#trade_quote_asof").get_result());
        }

        // Demonstrate window join for previous 1-second quote context
        std::cout << "\nB. Window Join - Match trades with quotes within 1 second window:\n";
        join_cols = {"idx"};
        auto window_result = joins::window_join(
            "trades",
            "quotes",
            "trade_quote_window",
            "Timestamp",
            "Timestamp",
            1.0,  // 1-second window
            join_cols
        );
        if (window_result) {
            print_result(inline_query("5#trade_quote_window").get_result());
        }

        // Use left join to keep all trades even without matching quotes (although this demo has all matching quotes)
        std::cout << "\nC. Left Join - Keep all trades with available quote context:\n";
        auto left_result = joins::left_join(
            "trades",
            "quotes",
            "trade_quote_left",
            {}  // Natural join on matching columns, will use first common column
        );
        if (left_result) {
            print_result(inline_query("5#trade_quote_left").get_result());
        }

        // Calculate metrics using the joined data
        inline_query(R"(
            // Price impact analysis using asof join results
            price_impact: select from trade_quote_asof
                where not null mid_price;

            // Trade analysis with quote context
            trade_analysis: select
                avg_spread: avg spread,
                avg_price_impact: avg abs(Trade_Price - mid_price) % mid_price,
                avg_trade_size: avg Trade_Size,
                total_notional: sum Trade_Price * Trade_Size
            from trade_quote_asof
            where not null mid_price;

            // Window-based liquidity analysis
            window_liquidity: select
                avg_trade_size: avg Trade_Size,
                avg_quoted_size: avg (Bid_Size + Ask_Size),
                size_ratio: avg[Trade_Size] % avg[Bid_Size + Ask_Size],
                avg_price_impact: avg abs(Trade_Price - mid_price) % mid_price
            from trade_quote_window
            where not null mid_price;
        )");

        std::cout << "\nTrade Analysis with Quote Context:\n";
        print_result(inline_query("trade_analysis").get_result());

        std::cout << "\nWindow-based Liquidity Analysis:\n";
        print_result(inline_query("window_liquidity").get_result());


        print_section("5. Performance Summary");
        std::cout << "\nTotal execution time: " << total_timer.elapsed() << " seconds\n";
        
        print_section("6. Cleanup");
        
        // Clean up temporary tables
        inline_query(R"(
            delete market_state from `.;
            delete impact_analysis from `.;
            delete book_pressure from `.;
            delete time_weighted_metrics from `.;
            delete quote_stats from `.;
            delete trade_metrics from `.;
            delete time_metrics from `.;
            delete volatility_metrics from `.;
            delete imbalance_metrics from `.;
        )");

        KDBConnection::disconnect();
        std::cout << "\nAnalysis complete. KDB+ connection closed.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
