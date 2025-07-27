#!/usr/bin/env python
"""
Enhanced neural-network trader with regime awareness (single output)
────────────────────────────────────────────────────
• Attention layer over raw factors  
• Asymmetric cost-sensitive loss (λ = 1.5 for false-positives)  
• Class imbalance handled with per-row sample-weights  
• Date-aware split → most-recent 5 years = test window  
• P&L proxy: average net % per trade after user-set round-trip cost
"""

import argparse, warnings, joblib
from pathlib import Path
import numpy as np
import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report
import tensorflow as tf

warnings.filterwarnings("ignore")
tf.get_logger().setLevel("ERROR")

# ──────────────────────────────── model ────────────────────────────────
def create_model(n_features: int) -> tf.keras.Model:
    inp = tf.keras.layers.Input(shape=(n_features,))
    att = tf.keras.layers.Dense(n_features, activation="softmax", name="attention")(inp)
    x = tf.keras.layers.Multiply()([inp, att])
    x = tf.keras.layers.BatchNormalization()(x)
    x = tf.keras.layers.Dense(256, activation="swish")(x)
    x = tf.keras.layers.Dropout(0.40)(x)
    x = tf.keras.layers.Dense(128, activation="swish")(x)
    x = tf.keras.layers.Dropout(0.30)(x)
    out = tf.keras.layers.Dense(1, activation="sigmoid", name="main")(x)
    return tf.keras.Model(inputs=inp, outputs=out)

def asymmetric_loss(lambda_fp: float = 1.5):
    bce = tf.keras.losses.BinaryCrossentropy(from_logits=False)
    def _loss(y_true, y_pred):
        fp = bce(y_true, y_pred) * (1 - y_true) * lambda_fp
        fn = bce(y_true, y_pred) * y_true
        return tf.reduce_mean(fp + fn)
    return _loss

# ──────────────────────────────── CLI ──────────────────────────────────
parser = argparse.ArgumentParser(
    description="Train an attention-based NN with cost-sensitive loss "
                "and evaluate on a 5-year hold-out slice.")
parser.add_argument("--features", default="..\\data\\features.csv")
parser.add_argument("--cost",     type=float, default=0.0002,
                    help="Round-trip cost deducted per trade (default 2 bp)")
args = parser.parse_args()

# ───────────────────────── 1 ▸ Load & engineer ─────────────────────────
df = pd.read_csv(Path(args.features))
df["date"] = pd.to_datetime(df["date"])

OUTLIER_DATES = [
    ("2000-03-01", "2002-10-31"),  # dot-com bust
    ("2001-09-10", "2001-09-21"),  # 9/11 halt & re-open
    ("2008-09-01", "2009-03-31"),  # GFC capitulation
    ("2010-05-06", "2010-05-13"),  # flash-crash week
    ("2020-02-15", "2020-04-15"),  # COVID waterfall
    ("2022-02-24", "2022-03-15"),  # Ukraine shock
]
for start, end in OUTLIER_DATES:
    mask = (df["date"] >= start) & (df["date"] <= end)
    df   = df[~mask]

# Microsoft-specific outlier periods for trading model
MICROSOFT_OUTLIER_DATES = [
    ("1998-05-18", "2001-06-28"),  # Microsoft antitrust case - from DOJ filing to appeals court decision
    ("2000-03-10", "2002-10-31"),  # Dot-com bubble burst - NASDAQ peak to trough, major tech selloff
    ("2001-09-10", "2001-09-21"),  # 9/11 terrorist attacks - market halt and reopening volatility
    ("2007-01-30", "2007-06-30"),  # Windows Vista launch problems - compatibility and performance issues
    ("2008-09-01", "2009-03-31"),  # Global Financial Crisis - Lehman Brothers collapse and market crash
    ("2010-05-06", "2010-05-13"),  # Flash crash - algorithmic trading crash
    ("2013-04-25", "2015-07-31"),  # Nokia acquisition period - from announcement to $7.6B writeoff
    ("2020-02-15", "2020-04-15"),  # COVID-19 market crash - fastest bear market in history
    ("2022-02-24", "2022-03-15"),  # Ukraine invasion - geopolitical shock and market volatility
    ("2024-07-19", "2024-07-19"),  # CrowdStrike/Microsoft global IT outage - $23B market cap loss
    ("2024-07-30", "2024-08-02"),  # Q4 2024 Azure/AI revenue disappointment - 6% stock drop
    ("2024-10-31", "2024-11-01"),  # Q1 2025 disappointing revenue guidance - 6% stock drop
    ("2025-01-30", "2025-03-21"),  # Disappointing Q2 2025 guidance and 7-week losing streak
]

# Apply outlier filtering in your model
for start, end in MICROSOFT_OUTLIER_DATES:
    mask = (df["date"] >= start) & (df["date"] <= end)
    df = df[~mask]

# cyclical calendar features
d = df["date"]
df["sin_mo"]  = np.sin(2*np.pi*d.dt.month   / 12)
df["cos_mo"]  = np.cos(2*np.pi*d.dt.month   / 12)
df["sin_dom"] = np.sin(2*np.pi*d.dt.day     / 31)
df["cos_dom"] = np.cos(2*np.pi*d.dt.day     / 31)
df["sin_dow"] = np.sin(2*np.pi*d.dt.weekday / 5)
df["cos_dow"] = np.cos(2*np.pi*d.dt.weekday / 5)
df["is_mon"]  = (d.dt.weekday == 0).astype(int)

df = df[df["date"] >= "2000-01-01"].sort_values("date").copy()

if "vwap" not in df.columns:
    df["vwap"] = df["close"]

# target with ±0.2 % neutrality band
r = df["close"].pct_change().shift(-1)
df["target"] = np.where(r > 0.002, 1,
                 np.where(r < -0.002, 0, np.nan))
df.dropna(subset=["target"], inplace=True)

# ───────────────────────── 2 ▸ Split ───────────────────────────────────
last_day   = df["date"].max()
test_start = last_day - pd.DateOffset(years=5) + pd.Timedelta(days=1)
train_df   = df[df["date"] <  test_start].copy()
test_df    = df[df["date"] >= test_start].copy()

FEATURES = [
    "macd_hist","rsi","supertrend_signal","bb_percent",
    "stoch_k","stoch_d","atr_pct","roc","obv","vwap",
    "sin_mo","cos_mo","sin_dom","cos_dom",
    "sin_dow","cos_dow","is_mon"
]

X_full = train_df[FEATURES].values.astype("float32")
y_full = train_df["target"].values.astype("float32")

scaler = StandardScaler().fit(X_full)
X_full = scaler.transform(X_full)
X_test = scaler.transform(test_df[FEATURES].values.astype("float32"))
y_test = test_df["target"].values.astype("float32")

X_tr, X_val, y_tr, y_val = train_test_split(
    X_full, y_full, test_size=0.20, random_state=42, stratify=y_full)

w_tr  = np.where(y_tr  == 1, 1.4, 1.0)
w_val = np.where(y_val == 1, 1.4, 1.0)

# ───────────────────────── 3 ▸ Build & fit ─────────────────────────────
tf.keras.utils.set_random_seed(42)
model = create_model(len(FEATURES))
model.compile(
    optimizer=tf.keras.optimizers.Adam(2e-4),
    loss=asymmetric_loss(1.5),
    metrics=["accuracy"]
)

es = tf.keras.callbacks.EarlyStopping(
    monitor="val_loss",
    patience=15, mode="min",
    restore_best_weights=True)

model.fit(
    X_tr, y_tr,
    sample_weight=w_tr,
    validation_data=(X_val, y_val, w_val),
    epochs=200,
    batch_size=128,
    callbacks=[es],
    verbose=2
)

# ───────────────────────── 4 ▸ Evaluation ──────────────────────────────
# ───────────────────────── 4 ▸ Evaluation ──────────────────────────────
proba = model.predict(X_test, verbose=0).ravel()
pred  = (proba > 0.50).astype(int)  # Binary prediction: 1 for buy (up), 0 for hold/sell (down)
report = classification_report(y_test, pred, digits=3, output_dict=True)

positions = np.where(pred == 1, 1, -1)  # 1 for long (buy), -1 for short/hold
px_ret    = test_df["close"].pct_change().shift(-1).fillna(0).values
net_ret   = positions * px_ret - args.cost
avg_trade = 100 * net_ret.mean()

# Save predictions to CSV (updated to include 'close')
predictions_df = pd.DataFrame({
    'date': test_df['date'],
    'close': test_df['close'],  # Add this line to include close prices
    'probability': proba,
    'prediction': pred  # 1 for buy, 0 for hold/sell (adjust based on your trading logic)
})
predictions_df.to_csv('nn_predictions.csv', index=False)
print(f"\nSaved test predictions to 'nn_predictions.csv' (including 'close')")
