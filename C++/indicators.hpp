#ifndef INDICATORS_HPP
#define INDICATORS_HPP
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>

inline bool is_nan(double x){return std::isnan(x);}
constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

/*────────────────────  EMA (NaN-aware)  ────────────────────*/
inline std::vector<double> ema_safe(const std::vector<double>& src,int p){
    size_t n=src.size(); std::vector<double> out(n,NaN);
    double k=2.0/(p+1.0), prev=0.0; int cnt=0;
    for(size_t i=0;i<n;++i){
        if(is_nan(src[i])){ if(cnt>=p) out[i]=prev; continue; }
        if(cnt<p){ prev+=src[i]; if(++cnt==p){ prev/=p; out[i]=prev; } }
        else{ prev=src[i]*k+prev*(1.0-k); out[i]=prev; }
    } return out;
}

/*────────────────────  SMA & STD  ────────────────────*/
inline std::vector<double> sma(const std::vector<double>& v,int p){
    size_t n=v.size(); std::vector<double> out(n,NaN);
    double sum=0.0; int cnt=0;
    for(size_t i=0;i<n;++i){
        if(!is_nan(v[i])){ sum+=v[i]; ++cnt; }
        if(i>=static_cast<size_t>(p)){
            if(!is_nan(v[i-p])){ sum-=v[i-p]; --cnt; }
        }
        if(cnt==p) out[i]=sum/p;
    } return out;
}
inline std::vector<double> sd(const std::vector<double>& v,
                              const std::vector<double>& ma,int p){
    size_t n=v.size(); std::vector<double> out(n,NaN);
    double ss=0.0; int cnt=0;
    for(size_t i=0;i<n;++i){
        if(!is_nan(v[i])&&!is_nan(ma[i])){ ss+=std::pow(v[i]-ma[i],2); ++cnt; }
        if(i>=static_cast<size_t>(p)){
            if(!is_nan(v[i-p])&&!is_nan(ma[i-p])){
                ss-=std::pow(v[i-p]-ma[i-p],2); --cnt;
            }
        }
        if(cnt==p) out[i]=std::sqrt(ss/p);
    } return out;
}

/*────────────────────  True Range & ATR  ────────────────────*/
inline std::vector<double> true_range(const std::vector<double>& h,
                                      const std::vector<double>& l,
                                      const std::vector<double>& c){
    size_t n=c.size(); std::vector<double> tr(n,NaN);
    for(size_t i=0;i<n;++i){
        double hl=h[i]-l[i];
        double hc=i?std::fabs(h[i]-c[i-1]):hl;
        double lc=i?std::fabs(l[i]-c[i-1]):hl;
        tr[i]=std::max({hl,hc,lc});
    } return tr;
}
inline std::vector<double> atr(const std::vector<double>& h,
                               const std::vector<double>& l,
                               const std::vector<double>& c,int p=10){
    return ema_safe(true_range(h,l,c),p);
}

/*────────────────────  MACD,  RSI,  Supertrend  ────────────────────*/
struct MACD{std::vector<double> macd,signal,hist;};
std::vector<double> rsi(const std::vector<double>&,int=7);  // Reduced default period to 7 for more sensitivity
std::vector<double> supertrend(const std::vector<double>&,
                               const std::vector<double>&,
                               const std::vector<double>&,
                               int=7,double=2.0);  // Reduced defaults: period=7, multiplier=2.0 for more signals

/* MACD */
inline MACD macd(const std::vector<double>& close,int f=12,int s=26,int sig=9){
    size_t n=close.size(); MACD m;
    m.macd.assign(n,NaN); m.signal.assign(n,NaN); m.hist.assign(n,NaN);
    auto fema=ema_safe(close,f), sema=ema_safe(close,s);
    for(size_t i=0;i<n;++i)
        if(!is_nan(fema[i])&&!is_nan(sema[i])) m.macd[i]=fema[i]-sema[i];
    m.signal=ema_safe(m.macd,sig);
    for(size_t i=0;i<n;++i)
        if(!is_nan(m.macd[i])&&!is_nan(m.signal[i])) m.hist[i]=m.macd[i]-m.signal[i];
    return m;
}

/* RSI */
inline std::vector<double> rsi(const std::vector<double>& c,int p){
    size_t n=c.size(); std::vector<double> out(n,NaN);
    if(n<=static_cast<size_t>(p)) return out;
    double g=0,l=0;
    for(int i=1;i<=p;++i){
        double d=c[i]-c[i-1]; (d>=0?g:l)+=std::fabs(d);
    } g/=p; l/=p; out[p]=100.0-100.0/(1+g/l);
    for(size_t i=p+1;i<n;++i){
        double d=c[i]-c[i-1]; double up=d>0?d:0, dn=d<0?-d:0;
        g=(g*(p-1)+up)/p; l=(l*(p-1)+dn)/p;
        out[i]=100.0-100.0/(1+g/l);
    } return out;
}

/* Supertrend */
inline std::vector<double> supertrend(const std::vector<double>& h,
                                      const std::vector<double>& l,
                                      const std::vector<double>& c,
                                      int p,double mlt){
    size_t n=c.size(); std::vector<double> st(n,NaN);
    auto a=atr(h,l,c,p);
    for(size_t i=0;i<n;++i){
        if(is_nan(a[i])) a[i]=0.0;
        double hl2=0.5*(h[i]+l[i]);
        double up=hl2+mlt*a[i], low=hl2-mlt*a[i];
        if(i==0){ st[i]=low; continue; }
        st[i]=(c[i]>st[i-1])?std::max(low,st[i-1])
                            :std::min(up ,st[i-1]);
    } return st;
}

/*────────────────────  NEW INDICATORS  ────────────────────*/
/* Bollinger %B */
inline std::vector<double> boll_percent(const std::vector<double>& c,int p=20,double k=2.0){
    auto ma=sma(c,p); auto sdv=sd(c,ma,p);
    size_t n=c.size(); std::vector<double> out(n,NaN);
    for(size_t i=0;i<n;++i)
        if(!is_nan(ma[i])&&!is_nan(sdv[i])&&sdv[i]!=0)
            out[i]=(c[i]-ma[i])/(k*sdv[i]) + 0.5;       // 0=bott,1=top
    return out;
}

/* Stochastic %K & %D */
struct STOCH{std::vector<double> k,d;};
inline STOCH stoch(const std::vector<double>& h,
                   const std::vector<double>& l,
                   const std::vector<double>& c,
                   int klen=14,int dlen=3){
    size_t n=c.size(); STOCH s;
    s.k.assign(n,NaN); s.d.assign(n,NaN);
    for(size_t i=0;i<n;++i){
        if(i+1<static_cast<size_t>(klen)) continue;
        double hh=*std::max_element(h.begin()+i+1-klen,h.begin()+i+1);
        double ll=*std::min_element(l.begin()+i+1-klen,l.begin()+i+1);
        if(hh==ll) continue;
        s.k[i]=100.0*(c[i]-ll)/(hh-ll);
    }
    s.d=ema_safe(s.k,dlen);
    return s;
}

/* Rate of Change */
inline std::vector<double> roc(const std::vector<double>& c,int p=12){
    size_t n=c.size(); std::vector<double> out(n,NaN);
    for(size_t i=p;i<n;++i)
        if(!is_nan(c[i-p])&&c[i-p]!=0) out[i]=100.0*(c[i]-c[i-p])/c[i-p];
    return out;
}

/* On-Balance Volume */
inline std::vector<double> obv(const std::vector<double>& c,
                               const std::vector<double>& v){
    size_t n=c.size(); std::vector<double> out(n,NaN);
    double running=0.0;
    for(size_t i=1;i<n;++i){
        if(is_nan(c[i])||is_nan(c[i-1])) continue;
        running+=(c[i]>c[i-1]?v[i]:(c[i]<c[i-1]? -v[i]:0));
        out[i]=running;
    } return out;
}

inline std::vector<double> vwma(const std::vector<double>& prices,
                               const std::vector<double>& volumes,
                               int period=20) {
    size_t n = prices.size();
    std::vector<double> out(n, NaN);
    double sum_price = 0.0, sum_vol = 0.0;
    int cnt = 0;
    
    for(size_t i=0; i<n; ++i) {
        if(!is_nan(prices[i])) {
            sum_price += prices[i] * volumes[i];
            sum_vol += volumes[i];
            ++cnt;
        }
        
        if(i >= static_cast<size_t>(period)) {
            size_t j = i - period;
            if(!is_nan(prices[j])) {
                sum_price -= prices[j] * volumes[j];
                sum_vol -= volumes[j];
                --cnt;
            }
        }
        
        if(cnt == period && sum_vol != 0) {
            out[i] = sum_price / sum_vol;
        }
    }
    return out;
}

/* Chande Momentum Oscillator */
inline std::vector<double> cmo(const std::vector<double>& prices, int period=14) {
    size_t n = prices.size();
    std::vector<double> out(n, NaN);
    
    for(size_t i=period; i<n; ++i) {
        double sum_up = 0.0, sum_down = 0.0;
        
        for(size_t j=i-period+1; j<=i; ++j) {
            double diff = prices[j] - prices[j-1];
            if(diff > 0) sum_up += diff;
            else sum_down -= diff;
        }
        
        if(sum_up + sum_down != 0) {
            out[i] = 100.0 * (sum_up - sum_down) / (sum_up + sum_down);
        }
    }
    return out;
}

#endif
