Trade Smarter SOC'25 by Apoorva Ansh (22b2407)
This project implements a sophisticated trading bot that leverages a custom-built neural network and a suite of technical indicators to analyze Microsoft (MSFT) stock data. The core of the project is a C++ engine for high-speed feature calculation and a Python-based deep learning model for generating trading signals.

Features ✨
-High-Performance Feature Engineering: Key trading indicators are calculated in C++ for maximum efficiency.
-Attention-Based Neural Network: A TensorFlow model that includes an attention mechanism to focus on the most relevant features for predicting stock price movements.
-Asymmetric Loss Function: The model is trained with a custom loss function that penalizes false positives more heavily, aiming to reduce unprofitable buy signals.
-Robust Evaluation: The strategies are tested on a 5-year out-of-sample data slice, with transaction costs factored in to simulate real-world trading conditions.
-Dual-Path Execution: You can either run the full pipeline from scratch or use the pre-computed files for a quick evaluation.

Technical Indicators
The model's features are derived from a combination of standard and custom-engineered technical indicators.

Indicator Suite
-Moving Average Convergence Divergence (MACD): Calculated using a fast EMA of 12, a slow EMA of 26, and a 9-period signal line. The histogram (macd_hist) is the primary feature used.
-Relative Strength Index (RSI): A momentum oscillator that measures the speed and change of price movements. The default period has been reduced to 7 for increased sensitivity. A volume-weighted version is also calculated to give more importance to price changes that occur on higher volume.
-Supertrend: An indicator that helps identify the primary trend direction and potential entry/exit points. The default settings were adjusted to a period of 7 and a multiplier of 2.0 to generate more frequent signals.
-Bollinger Bands (%B): This measures where the current price is in relation to the Bollinger Bands. It's normalized to a 0-1 scale, where values near 0 represent the lower band and values near 1 represent the upper band.
-Stochastic Oscillator (%K & %D): A momentum indicator that compares a particular closing price of a security to a range of its prices over a certain period of time. It includes both the main %K line and a 3-period EMA of %K, known as %D.
-Rate of Change (ROC): Measures the percentage change in price between the current price and the price a certain number of periods ago (12 periods by default).
-On-Balance Volume (OBV): A cumulative momentum indicator that uses volume flow to predict changes in stock price.
-Volume-Weighted Average Price (VWAP): An approximation is calculated for each day by taking the average of the high, low, and close, multiplied by the volume.
-Calendar Features: To capture cyclical patterns, the model is fed features like the sine and cosine of the month, day of the month, and day of the week, along with a binary flag for Mondays.

Methodology & Assumptions
The project follows a clear workflow from data processing to final evaluation, with several key assumptions made to create a realistic trading simulation.

Workflow
-Feature Calculation (C++): The export_features.cpp program reads raw OHLCV (Open, High, Low, Close, Volume) data. It utilizes the functions in indicators.hpp to compute the technical indicators and exports them to features.csv. NaN values are pruned to ensure a clean dataset.
-Model Training (Python): The train_validate_test.py script loads the features.csv file. It then scales the features, applies a date-based split to create a 5-year test set, and trains the neural network. The trained model then makes predictions on the test set, which are saved to nn_predictions.csv.
-Reporting (Python): The main_report.py script evaluates the performance of both the individual indicators and the combined neural network strategy using the test data, printing a final summary report.

Assumptions
-Transaction Costs: A round-trip cost of 0.02% (2 basis points) is deducted for each trade to account for brokerage fees and slippage.
-Outlier Removal: To prevent the model from being skewed by extreme and anomalous market events, specific date ranges are filtered out from the dataset. These include major market crashes like the dot-com bust, the 2008 financial crisis, and the COVID-19 crash, as well as Microsoft-specific events like the Nokia acquisition period and major IT outages.
-Target Definition: A trade signal is generated based on a neutrality band. A "buy" signal (target=1) is defined as a price increase of more than 0.2% on the next day, and a "sell/hold" signal (target=0) is defined as a price decrease of more than 0.2%.
-Class Imbalance: The dataset naturally has an imbalance between up and down days. This is handled by applying sample weights during training, where buy signals (y_tr == 1) are given a higher weight (1.4) to make the model pay more attention to them.

Getting Started 🚀
Before running, ensure you have the necessary dependencies installed.

    pip install -r requirements.txt

Approach 1: Run the Full Pipeline from Scratch
This approach compiles the C++ code and runs the entire pipeline to generate all files from the raw data.

1. Compile the C++ Feature Extractor:
Navigate to the C++ directory and compile the source files.

       g++ export_features.cpp -o export_features

2. Generate features.csv:
From the root directory of the project, run the compiled executable.

       ./C++/export_features ./data/MSFT_1986-03-13_2025-04-06.csv ./data/features.csv

3. Train the Model and Generate Predictions:
Run the Python script to train the neural network and create the nn_predictions.csv file.

        python python/train_validate_test.py --features ./data/features.csv

4. View the Final Report:
 Run the reporting script to see the performance evaluation.

        python python/main_report.py

Approach 2: Quick Evaluation (Using Pre-computed Files)
If you want to skip the compilation and training steps, you can use the features.csv and nn_predictions.csv files already included in the repository to generate the final report directly.

1. View the Final Report:
Simply run the reporting script from the root directory.

       python python/main_report.py

Performance Results 📊
The following results are based on the 5-year testing period, accounting for a 0.02% transaction cost per trade.

Individual Indicator Strategy Performance
Indicator

Individual Indicator Strategy Performance (Testing Data Only):
MACD: Trades=53, Success Rate=35.85%, Per-Trade Return=-0.26%
RSI: Trades=12, Success Rate=75.00%, Per-Trade Return=5.05%
SuperTrend: Trades=69, Success Rate=23.19%, Per-Trade Return=0.14%
Stochastic: Trades=34, Success Rate=79.41%, Per-Trade Return=2.47%

Combined Neural Network Strategy Performance (Testing Data Only):
Trades=72, Success Rate=59.72%, Per-Trade Return=1.12%

