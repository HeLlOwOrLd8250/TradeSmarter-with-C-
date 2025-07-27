#include "indicators.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <algorithm>

static bool to_double(std::string tok, double &out) {
    tok.erase(std::remove(tok.begin(), tok.end(), '"'), tok.end());
    try { out = std::stod(tok); } catch(...) { return false; }
    return true;
}

// New: Volume-weighted features
std::vector<double> volume_weighted_rsi(const std::vector<double>& prices,
                                      const std::vector<double>& volumes,
                                      int period=14) {
    auto rsi_val = rsi(prices, period);
    double min_vol = *std::min_element(volumes.begin(), volumes.end());
    double vol_range = *std::max_element(volumes.begin(), volumes.end()) - min_vol;
    
    for(size_t i=0; i<rsi_val.size(); ++i) {
        if(!is_nan(rsi_val[i]) && !is_nan(volumes[i])) {
            double vol_norm = (volumes[i] - min_vol) / vol_range;
            double vol_scale = 0.8 + 0.4 * vol_norm;
            rsi_val[i] = 50 + (rsi_val[i]-50)*vol_scale;
        }
    }
    return rsi_val;
}

int main(int argc, char* argv[]) {
    if(argc != 3) { std::cerr << "Usage: export_features.exe <raw> <out>\n"; return 1; }
    
    // Data loading (unchanged)
    std::ifstream fin(argv[1]);
    if(!fin) { std::cerr << "Cannot open " << argv[1] << '\n'; return 1; }

    std::vector<std::string> date; 
    std::vector<double> o, h, l, c, adj, v;
    std::string line; 
    
    std::getline(fin, line); // header
    while(std::getline(fin, line)) {
        std::stringstream ss(line); 
        std::string d, tok; 
        double tmp;
        
        if(!std::getline(ss, d, ',')) continue;
        date.push_back(d.substr(0,10));
        
        // Parse OHLCVA data
        if(!std::getline(ss, tok, ',') || !to_double(tok, tmp)) continue; o.push_back(tmp);
        if(!std::getline(ss, tok, ',') || !to_double(tok, tmp)) continue; h.push_back(tmp);
        if(!std::getline(ss, tok, ',') || !to_double(tok, tmp)) continue; l.push_back(tmp);
        if(!std::getline(ss, tok, ',') || !to_double(tok, tmp)) continue; c.push_back(tmp);
        if(!std::getline(ss, tok, ',') || !to_double(tok, tmp)) continue; adj.push_back(tmp);
        std::getline(ss, tok); to_double(tok, tmp); v.push_back(tmp);
    }

    // Calculate indicators
    size_t n = c.size();
    auto M = macd(c);
    auto R = volume_weighted_rsi(c, v); // Modified: Volume-weighted RSI
    auto ST = supertrend(h, l, c);
    auto BB = boll_percent(c);
    auto S = stoch(h, l, c);
    auto ATR = atr(h, l, c);
    auto ROC = roc(c);
    auto OBV = obv(c, v);
    
    // New: VWAP indicator
    std::vector<double> vwap(n);
    for(size_t i=0; i<n; ++i) {
        vwap[i] = (h[i] + l[i] + c[i])/3 * v[i];
    }

    // Write features
    std::ofstream fout(argv[2]);
    if(!fout) { std::cerr << "Cannot write " << argv[2] << '\n'; return 1; }
    
    fout << "date,close,macd_hist,rsi,supertrend_signal,"
            "bb_percent,stoch_k,stoch_d,atr_pct,roc,obv,vwap\n";
    
    size_t kept = 0;
    for(size_t i=0; i<n; ++i) {
        if(is_nan(M.hist[i]) || is_nan(R[i]) || is_nan(ST[i]) || 
           is_nan(BB[i]) || is_nan(S.k[i]) || is_nan(S.d[i]) || 
           is_nan(ATR[i]) || is_nan(ROC[i]) || is_nan(OBV[i]) || is_nan(vwap[i]))
            continue;
            
        fout << date[i] << ',' << std::fixed << std::setprecision(6) << c[i] << ','
             << M.hist[i] << ',' << R[i] << ',' << (c[i] > ST[i]) << ','
             << BB[i] << ',' << S.k[i] << ',' << S.d[i] << ',' << (ATR[i]/c[i]) << ','
             << ROC[i] << ',' << OBV[i] << ',' << vwap[i] << '\n';
        ++kept;
    }

    std::cout << "Parsed rows : " << n << "\nExported    : " << kept << "\n"
              << "✓ Features written to " << argv[2] << '\n';
    return 0;
}