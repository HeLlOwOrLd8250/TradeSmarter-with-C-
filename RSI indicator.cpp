<<<<<<< HEAD
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

struct Candle {
    double open;
    double high;
    double low;
    double close;
};

struct TradeResult {
    double success_rate;
    double avg_return;
    int total_trades;
    std::vector<int> rsi_positions;
};

double calculate_rsi(const std::vector<double>& closes, int current_index, int period = 14) {
    if (current_index < period) return 50.0;
    double gain = 0.0, loss = 0.0;
    for (int i = current_index - period + 1; i <= current_index; ++i) {
        double change = closes[i] - closes[i - 1];
        if (change > 0) gain += change;
        else loss -= change;
    }
    if (loss == 0) return 100.0;
    double rs = gain / loss;
    return 100.0 - (100.0 / (1.0 + rs));
}

TradeResult run_rsi_strategy(const std::vector<Candle>& candles, double profit_threshold) {
    std::vector<double> closes;
    for (const auto& candle : candles)
        closes.push_back(candle.close);

    std::vector<int> rsi_positions(closes.size(), 0);
    int profitable_trades = 0;
    double total_return = 0.0;
    int total_trades = 0;
    bool was_above_60 = false;
    bool was_below_40 = false;
    double entry_price = 0.0;
    enum Position { NONE, LONG, SHORT } state = NONE;

    for (size_t i = 15; i < closes.size(); ++i) {
        double rsi = calculate_rsi(closes, i);
        double price_change = (closes[i] - closes[i - 1]) / closes[i - 1];

        // Signal logic
        if (rsi > 60 && price_change > 0.05) {
            if (!was_above_60) {
                rsi_positions[i] = 1; // Buy
                was_above_60 = true;
            }
            was_below_40 = false;
        } else if (was_above_60 && rsi < 60) {
            rsi_positions[i] = -1; // Exit long
            was_above_60 = false;
        } else if (rsi < 40 && price_change < -0.05) {
            if (!was_below_40) {
                rsi_positions[i] = -2; // Short
                was_below_40 = true;
            }
            was_above_60 = false;
        } else if (was_below_40 && rsi > 40) {
            rsi_positions[i] = 2; // Exit short
            was_below_40 = false;
        } else {
            rsi_positions[i] = 0;
        }

        // Trade execution
        if (state == NONE) {
            if (rsi_positions[i] == 1) { // Buy signal
                state = LONG;
                entry_price = closes[i];
            } else if (rsi_positions[i] == -2) { // Short signal
                state = SHORT;
                entry_price = closes[i];
            }
        } else if (state == LONG && rsi_positions[i] == -1) {
            double exit_price = closes[i];
            double ret = (exit_price - entry_price) / entry_price;
            total_return += ret;
            if (ret > profit_threshold) profitable_trades++;
            total_trades++;
            state = NONE;
        } else if (state == SHORT && rsi_positions[i] == 2) {
            double exit_price = closes[i];
            double ret = (entry_price - exit_price) / entry_price;
            total_return += ret;
            if (ret > profit_threshold) profitable_trades++;
            total_trades++;
            state = NONE;
        }
    }

    // Close open positions
    if (state != NONE) {
        double final_price = closes.back();
        double ret = (state == LONG) ? 
            (final_price - entry_price) / entry_price : 
            (entry_price - final_price) / entry_price;
        total_return += ret;
        if (ret > profit_threshold) profitable_trades++;
        total_trades++;
    }

    double success_rate = total_trades > 0 ? 
        (double)profitable_trades / total_trades * 100 : 0;
    double avg_return = total_trades > 0 ? 
        (total_return / total_trades) * 100 : 0;

    return {success_rate, avg_return, total_trades, rsi_positions};
}

int main() {
    // 100-day sample data (Infosys-like volatility)
    std::vector<Candle> candles = {
        {100.28, 100.33, 99.45, 100.0},
        {96.58, 97.89, 94.41, 96.12},
        {90.42, 90.62, 90.02, 90.56},
        {89.77, 90.99, 88.6, 90.63},
        {90.69, 92.28, 89.22, 91.2},
        {85.42, 86.61, 84.32, 84.9},
        {81.54, 82.09, 80.66, 80.81},
        {76.78, 77.7, 75.02, 76.25},
        {78.76, 80.29, 78.11, 78.7},
        {79.8, 80.78, 77.91, 79.27},
        {80.46, 80.53, 79.76, 80.13},
        {77.11, 78.13, 76.96, 77.77},
        {75.56, 76.11, 74.79, 75.35},
        {71.95, 73.64, 71.02, 72.29},
        {72.91, 74.46, 72.67, 73.39},
        {72.86, 73.79, 71.35, 72.15},
        {74.52, 75.68, 73.68, 74.01},
        {68.91, 69.53, 68.62, 69.16},
        {74.0, 74.47, 72.49, 73.45},
        {72.98, 73.65, 71.99, 72.38},
        {69.9, 70.27, 69.0, 69.81},
        {73.55, 74.02, 72.08, 73.7},
        {73.19, 73.87, 73.03, 73.8},
        {75.56, 76.19, 75.02, 75.12},
        {74.61, 75.4, 72.44, 73.88},
        {76.85, 78.73, 75.81, 77.61},
        {77.65, 79.01, 77.47, 78.01},
        {77.23, 78.77, 75.87, 77.3},
        {74.74, 75.01, 73.38, 74.74},
        {78.3, 79.62, 77.35, 78.62},
        {75.19, 76.0, 73.63, 74.8},
        {74.37, 75.6, 74.34, 75.12},
        {80.24, 81.57, 79.14, 79.63},
        {75.27, 76.69, 74.57, 74.7},
        {73.91, 75.69, 72.78, 74.55},
        {70.64, 71.45, 70.26, 70.67},
        {74.24, 74.67, 73.44, 74.35},
        {76.28, 77.22, 74.77, 76.74},
        {78.25, 79.16, 78.06, 78.35},
        {75.09, 76.22, 74.74, 75.33},
        {71.76, 73.29, 71.43, 72.38},
        {77.04, 77.15, 76.12, 76.49},
        {77.85, 78.51, 76.4, 78.3},
        {79.04, 80.32, 77.76, 79.08},
        {75.04, 76.3, 74.41, 75.65},
        {75.65, 76.66, 73.82, 75.3},
        {70.93, 71.55, 69.71, 71.07},
        {68.14, 69.18, 67.57, 68.57},
        {66.11, 67.67, 65.53, 66.44},
        {69.87, 69.94, 68.41, 69.8},
        {73.77, 75.14, 71.84, 73.08},
        {69.65, 69.96, 69.09, 69.67},
        {65.21, 66.65, 64.86, 65.37},
        {67.91, 68.54, 66.61, 67.97},
        {72.77, 73.81, 72.46, 72.68},
        {71.27, 72.1, 69.85, 70.61},
        {72.41, 73.91, 71.69, 73.06},
        {76.14, 78.14, 76.02, 76.67},
        {73.44, 74.43, 72.95, 73.3},
        {69.94, 70.29, 68.57, 69.4},
        {70.45, 71.38, 69.71, 70.56},
        {74.41, 75.93, 74.06, 74.85},
        {74.01, 74.46, 73.29, 73.76},
        {75.71, 77.06, 74.2, 76.36},
        {80.97, 82.01, 80.54, 81.66},
        {87.27, 88.81, 85.97, 86.61},
        {83.01, 84.18, 81.45, 82.46},
        {88.36, 88.37, 86.65, 88.08},
        {85.89, 87.5, 85.38, 85.61},
        {80.36, 81.9, 79.93, 81.0},
        {82.55, 82.88, 81.15, 82.19},
        {79.46, 80.91, 78.11, 79.47},
        {74.82, 75.35, 74.81, 74.93},
        {77.99, 78.4, 76.62, 77.77},
        {78.22, 78.35, 78.1, 78.33},
        {83.2, 84.11, 81.15, 82.53},
        {82.9, 83.7, 82.38, 83.48},
        {88.66, 90.19, 86.56, 88.14},
        {84.14, 84.74, 82.83, 84.56},
        {88.94, 90.21, 88.67, 89.11},
        {95.16, 97.02, 92.94, 94.47},
        {98.57, 100.98, 97.91, 99.51},
        {106.15, 107.98, 103.8, 105.51},
        {102.65, 102.87, 100.29, 102.07},
        {106.6, 108.94, 105.62, 107.19},
        {104.88, 105.36, 104.22, 104.27},
        {99.45, 101.52, 97.52, 99.79},
        {96.98, 97.75, 94.81, 96.7},
        {98.04, 98.27, 95.3, 97.19},
        {93.68, 94.17, 92.62, 92.82},
        {92.39, 92.97, 90.85, 91.97},
        {91.91, 93.18, 91.44, 92.12},
        {93.87, 96.57, 92.86, 94.81},
        {98.2, 99.51, 97.01, 97.72},
        {92.14, 92.75, 91.26, 91.84},
        {96.74, 97.32, 95.72, 96.31},
        {94.89, 95.64, 94.65, 95.07},
        {94.84, 96.12, 92.31, 94.01},
        {95.15, 96.58, 95.15, 95.53},
        {92.55, 93.76, 91.34, 92.68}
    };

    double profit_threshold = 0.005; // 0.5% profit threshold
    TradeResult result = run_rsi_strategy(candles, profit_threshold);

    std::cout << "RSI Strategy Results (100 days):\n";
    std::cout << "Total Trades: " << result.total_trades << "\n";
    std::cout << "Profitable Trades (%): " << std::fixed << std::setprecision(2) 
              << result.success_rate << "%\n";
    std::cout << "Average Return per Trade (%): " << result.avg_return << "%\n";

    // Optional: Print signals for last 10 days
    std::cout << "\nLast 10 days signals: ";
    size_t start_index = (result.rsi_positions.size() > 10) ? 
                         result.rsi_positions.size() - 10 : 0;
    for (size_t i = start_index; i < result.rsi_positions.size(); ++i) {
        std::cout << result.rsi_positions[i] << " ";
    }
    std::cout << "\n";

    return 0;
}
=======
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

struct Candle {
    double open;
    double high;
    double low;
    double close;
};

struct TradeResult {
    double success_rate;
    double avg_return;
    int total_trades;
    std::vector<int> rsi_positions;
};

double calculate_rsi(const std::vector<double>& closes, int current_index, int period = 14) {
    if (current_index < period) return 50.0;
    double gain = 0.0, loss = 0.0;
    for (int i = current_index - period + 1; i <= current_index; ++i) {
        double change = closes[i] - closes[i - 1];
        if (change > 0) gain += change;
        else loss -= change;
    }
    if (loss == 0) return 100.0;
    double rs = gain / loss;
    return 100.0 - (100.0 / (1.0 + rs));
}

TradeResult run_rsi_strategy(const std::vector<Candle>& candles, double profit_threshold) {
    std::vector<double> closes;
    for (const auto& candle : candles)
        closes.push_back(candle.close);

    std::vector<int> rsi_positions(closes.size(), 0);
    int profitable_trades = 0;
    double total_return = 0.0;
    int total_trades = 0;
    bool was_above_60 = false;
    bool was_below_40 = false;
    double entry_price = 0.0;
    enum Position { NONE, LONG, SHORT } state = NONE;

    for (size_t i = 15; i < closes.size(); ++i) {
        double rsi = calculate_rsi(closes, i);
        double price_change = (closes[i] - closes[i - 1]) / closes[i - 1];

        // Signal logic
        if (rsi > 60 && price_change > 0.05) {
            if (!was_above_60) {
                rsi_positions[i] = 1; // Buy
                was_above_60 = true;
            }
            was_below_40 = false;
        } else if (was_above_60 && rsi < 60) {
            rsi_positions[i] = -1; // Exit long
            was_above_60 = false;
        } else if (rsi < 40 && price_change < -0.05) {
            if (!was_below_40) {
                rsi_positions[i] = -2; // Short
                was_below_40 = true;
            }
            was_above_60 = false;
        } else if (was_below_40 && rsi > 40) {
            rsi_positions[i] = 2; // Exit short
            was_below_40 = false;
        } else {
            rsi_positions[i] = 0;
        }

        // Trade execution
        if (state == NONE) {
            if (rsi_positions[i] == 1) { // Buy signal
                state = LONG;
                entry_price = closes[i];
            } else if (rsi_positions[i] == -2) { // Short signal
                state = SHORT;
                entry_price = closes[i];
            }
        } else if (state == LONG && rsi_positions[i] == -1) {
            double exit_price = closes[i];
            double ret = (exit_price - entry_price) / entry_price;
            total_return += ret;
            if (ret > profit_threshold) profitable_trades++;
            total_trades++;
            state = NONE;
        } else if (state == SHORT && rsi_positions[i] == 2) {
            double exit_price = closes[i];
            double ret = (entry_price - exit_price) / entry_price;
            total_return += ret;
            if (ret > profit_threshold) profitable_trades++;
            total_trades++;
            state = NONE;
        }
    }

    // Close open positions
    if (state != NONE) {
        double final_price = closes.back();
        double ret = (state == LONG) ? 
            (final_price - entry_price) / entry_price : 
            (entry_price - final_price) / entry_price;
        total_return += ret;
        if (ret > profit_threshold) profitable_trades++;
        total_trades++;
    }

    double success_rate = total_trades > 0 ? 
        (double)profitable_trades / total_trades * 100 : 0;
    double avg_return = total_trades > 0 ? 
        (total_return / total_trades) * 100 : 0;

    return {success_rate, avg_return, total_trades, rsi_positions};
}

int main() {
    // 100-day sample data (Infosys-like volatility)
    std::vector<Candle> candles = {
        {100.28, 100.33, 99.45, 100.0},
        {96.58, 97.89, 94.41, 96.12},
        {90.42, 90.62, 90.02, 90.56},
        {89.77, 90.99, 88.6, 90.63},
        {90.69, 92.28, 89.22, 91.2},
        {85.42, 86.61, 84.32, 84.9},
        {81.54, 82.09, 80.66, 80.81},
        {76.78, 77.7, 75.02, 76.25},
        {78.76, 80.29, 78.11, 78.7},
        {79.8, 80.78, 77.91, 79.27},
        {80.46, 80.53, 79.76, 80.13},
        {77.11, 78.13, 76.96, 77.77},
        {75.56, 76.11, 74.79, 75.35},
        {71.95, 73.64, 71.02, 72.29},
        {72.91, 74.46, 72.67, 73.39},
        {72.86, 73.79, 71.35, 72.15},
        {74.52, 75.68, 73.68, 74.01},
        {68.91, 69.53, 68.62, 69.16},
        {74.0, 74.47, 72.49, 73.45},
        {72.98, 73.65, 71.99, 72.38},
        {69.9, 70.27, 69.0, 69.81},
        {73.55, 74.02, 72.08, 73.7},
        {73.19, 73.87, 73.03, 73.8},
        {75.56, 76.19, 75.02, 75.12},
        {74.61, 75.4, 72.44, 73.88},
        {76.85, 78.73, 75.81, 77.61},
        {77.65, 79.01, 77.47, 78.01},
        {77.23, 78.77, 75.87, 77.3},
        {74.74, 75.01, 73.38, 74.74},
        {78.3, 79.62, 77.35, 78.62},
        {75.19, 76.0, 73.63, 74.8},
        {74.37, 75.6, 74.34, 75.12},
        {80.24, 81.57, 79.14, 79.63},
        {75.27, 76.69, 74.57, 74.7},
        {73.91, 75.69, 72.78, 74.55},
        {70.64, 71.45, 70.26, 70.67},
        {74.24, 74.67, 73.44, 74.35},
        {76.28, 77.22, 74.77, 76.74},
        {78.25, 79.16, 78.06, 78.35},
        {75.09, 76.22, 74.74, 75.33},
        {71.76, 73.29, 71.43, 72.38},
        {77.04, 77.15, 76.12, 76.49},
        {77.85, 78.51, 76.4, 78.3},
        {79.04, 80.32, 77.76, 79.08},
        {75.04, 76.3, 74.41, 75.65},
        {75.65, 76.66, 73.82, 75.3},
        {70.93, 71.55, 69.71, 71.07},
        {68.14, 69.18, 67.57, 68.57},
        {66.11, 67.67, 65.53, 66.44},
        {69.87, 69.94, 68.41, 69.8},
        {73.77, 75.14, 71.84, 73.08},
        {69.65, 69.96, 69.09, 69.67},
        {65.21, 66.65, 64.86, 65.37},
        {67.91, 68.54, 66.61, 67.97},
        {72.77, 73.81, 72.46, 72.68},
        {71.27, 72.1, 69.85, 70.61},
        {72.41, 73.91, 71.69, 73.06},
        {76.14, 78.14, 76.02, 76.67},
        {73.44, 74.43, 72.95, 73.3},
        {69.94, 70.29, 68.57, 69.4},
        {70.45, 71.38, 69.71, 70.56},
        {74.41, 75.93, 74.06, 74.85},
        {74.01, 74.46, 73.29, 73.76},
        {75.71, 77.06, 74.2, 76.36},
        {80.97, 82.01, 80.54, 81.66},
        {87.27, 88.81, 85.97, 86.61},
        {83.01, 84.18, 81.45, 82.46},
        {88.36, 88.37, 86.65, 88.08},
        {85.89, 87.5, 85.38, 85.61},
        {80.36, 81.9, 79.93, 81.0},
        {82.55, 82.88, 81.15, 82.19},
        {79.46, 80.91, 78.11, 79.47},
        {74.82, 75.35, 74.81, 74.93},
        {77.99, 78.4, 76.62, 77.77},
        {78.22, 78.35, 78.1, 78.33},
        {83.2, 84.11, 81.15, 82.53},
        {82.9, 83.7, 82.38, 83.48},
        {88.66, 90.19, 86.56, 88.14},
        {84.14, 84.74, 82.83, 84.56},
        {88.94, 90.21, 88.67, 89.11},
        {95.16, 97.02, 92.94, 94.47},
        {98.57, 100.98, 97.91, 99.51},
        {106.15, 107.98, 103.8, 105.51},
        {102.65, 102.87, 100.29, 102.07},
        {106.6, 108.94, 105.62, 107.19},
        {104.88, 105.36, 104.22, 104.27},
        {99.45, 101.52, 97.52, 99.79},
        {96.98, 97.75, 94.81, 96.7},
        {98.04, 98.27, 95.3, 97.19},
        {93.68, 94.17, 92.62, 92.82},
        {92.39, 92.97, 90.85, 91.97},
        {91.91, 93.18, 91.44, 92.12},
        {93.87, 96.57, 92.86, 94.81},
        {98.2, 99.51, 97.01, 97.72},
        {92.14, 92.75, 91.26, 91.84},
        {96.74, 97.32, 95.72, 96.31},
        {94.89, 95.64, 94.65, 95.07},
        {94.84, 96.12, 92.31, 94.01},
        {95.15, 96.58, 95.15, 95.53},
        {92.55, 93.76, 91.34, 92.68}
    };

    double profit_threshold = 0.005; // 0.5% profit threshold
    TradeResult result = run_rsi_strategy(candles, profit_threshold);

    std::cout << "RSI Strategy Results (100 days):\n";
    std::cout << "Total Trades: " << result.total_trades << "\n";
    std::cout << "Profitable Trades (%): " << std::fixed << std::setprecision(2) 
              << result.success_rate << "%\n";
    std::cout << "Average Return per Trade (%): " << result.avg_return << "%\n";

    // Optional: Print signals for last 10 days
    std::cout << "\nLast 10 days signals: ";
    size_t start_index = (result.rsi_positions.size() > 10) ? 
                         result.rsi_positions.size() - 10 : 0;
    for (size_t i = start_index; i < result.rsi_positions.size(); ++i) {
        std::cout << result.rsi_positions[i] << " ";
    }
    std::cout << "\n";

    return 0;
}
>>>>>>> 0675421 (Initial commit: Add project files for SOS Trade Smarter (chatbot, text-to-audio, Llama fine-tuning, trading indicators, ML models, and data))
