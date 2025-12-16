#include "MedicineAI.hpp"
#include <dlib/matrix.h>
#include <dlib/statistics.h>
#include <dlib/svm.h>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <deque>
#include <mutex>
#include <chrono>
#include <ctime>

namespace MedicineAI {

// Forward declare implementation class
class MedicineTracker::Impl {
public:
    std::mutex data_mutex;
    std::mutex model_mutex;
    std::map<std::string, std::string> config;
    std::map<std::string, Medication> medication_db;
    std::vector<AdherenceLog> adherence_logs;
    std::map<std::string, std::vector<AdherenceLog>> user_logs;
    std::map<std::string, std::vector<AdherenceLog>> med_logs;
    
    struct ModelParams {
        double smoothing_alpha = 0.3;
        double trend_beta = 0.1;
        int min_training_samples = 10;
        int64_t last_training_time = 0;
        int training_interval_hours = 24;
    } model_params;
    
    struct TrainedModel {
        dlib::matrix<double> weights;
        dlib::matrix<double> feature_means;
        dlib::matrix<double> feature_stds;
        bool is_trained = false;
    };
    std::map<std::string, TrainedModel> user_models;
    
    struct CacheEntry {
        PredictionResult result;
        int64_t timestamp;
    };
    mutable std::map<std::string, CacheEntry> prediction_cache;
    
    void RebuildIndexes();
    std::vector<double> ExtractFeatures(const std::vector<AdherenceLog>&, const std::string&,
                                       const std::string&, int64_t) const;
    double CalculateMedian(std::vector<double>) const;
    double CalculateTrendSlope(const std::vector<AdherenceLog>&) const;
    void TrainUserModel(const std::string&);
    PredictionResult PredictWithModel(const std::string&, const std::string&, int64_t) const;
    bool SaveMedicationsToFile(const std::string&) const;
    bool SaveLogsToFile(const std::string&) const;
    bool LoadMedicationsFromFile(const std::string&);
    bool LoadLogsFromFile(const std::string&);
};

// Helper implementations
void MedicineTracker::Impl::RebuildIndexes() {
    user_logs.clear();
    med_logs.clear();
    for (const auto& log : adherence_logs) {
        user_logs[log.user_id].push_back(log);
        med_logs[log.medication_id].push_back(log);
    }
    for (auto& p : user_logs) std::sort(p.second.begin(), p.second.end(),
        [](auto& a, auto& b) { return a.timestamp < b.timestamp; });
    for (auto& p : med_logs) std::sort(p.second.begin(), p.second.end(),
        [](auto& a, auto& b) { return a.timestamp < b.timestamp; });
}

double MedicineTracker::Impl::CalculateMedian(std::vector<double> v) const {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t mid = v.size() / 2;
    return (v.size() % 2 == 0) ? (v[mid-1] + v[mid]) / 2.0 : v[mid];
}

double MedicineTracker::Impl::CalculateTrendSlope(const std::vector<AdherenceLog>& logs) const {
    if (logs.size() < 2) return 0.0;
    int n = logs.size();
    double sx = 0, sy = 0, sxy = 0, sxx = 0;
    int64_t t0 = logs[0].timestamp;
    for (int i = 0; i < n; ++i) {
        double x = (logs[i].timestamp - t0) / 86400.0;
        double y = logs[i].dose_taken;
        sx += x; sy += y; sxy += x*y; sxx += x*x;
    }
    return (n*sxy - sx*sy) / (n*sxx - sx*sx + 1e-10);
}

std::vector<double> MedicineTracker::Impl::ExtractFeatures(const std::vector<AdherenceLog>& logs,
    const std::string& user_id, const std::string& med_id, int64_t t) const {
    std::vector<double> f(15, 0.0);
    std::vector<AdherenceLog> rel;
    for (auto& l : logs) if (l.user_id == user_id && l.medication_id == med_id) rel.push_back(l);
    if (rel.empty()) return f;
    
    f[0] = rel.size();
    double total = 0;
    for (auto& l : rel) if (!l.is_return) total += l.dose_taken;
    f[1] = total / rel.size();
    double var = 0;
    for (auto& l : rel) if (!l.is_return) { double d = l.dose_taken - f[1]; var += d*d; }
    f[2] = std::sqrt(var / rel.size());
    f[3] = (t - rel.back().timestamp) / 3600.0;
    if (rel.size() > 1) {
        double gap = 0;
        for (size_t i = 1; i < rel.size(); ++i) gap += (rel[i].timestamp - rel[i-1].timestamp) / 3600.0;
        f[4] = gap / (rel.size() - 1);
    }
    f[5] = CalculateTrendSlope(rel);
    
    std::time_t tt = t;
    std::tm* tm = std::localtime(&tt);
    double h = tm->tm_hour, d = tm->tm_wday;
    f[6] = std::sin(2*M_PI*h/24); f[7] = std::cos(2*M_PI*h/24);
    f[8] = std::sin(2*M_PI*d/7); f[9] = std::cos(2*M_PI*d/7);
    
    std::vector<double> recent;
    int64_t week = t - 7*86400;
    for (auto& l : rel) if (l.timestamp >= week && !l.is_return) recent.push_back(l.dose_taken);
    if (!recent.empty()) {
        f[10] = std::accumulate(recent.begin(), recent.end(), 0.0) / recent.size();
        var = 0;
        for (double r : recent) { double d = r - f[10]; var += d*d; }
        f[11] = std::sqrt(var / recent.size());
    }
    
    int rets = 0;
    for (auto& l : rel) if (l.is_return) rets++;
    f[12] = (double)rets / rel.size();
    f[13] = (t - rel.front().timestamp) / 86400.0;
    return f;
}

void MedicineTracker::Impl::TrainUserModel(const std::string& uid) {
    std::lock_guard<std::mutex> lk(model_mutex);
    auto it = user_logs.find(uid);
    if (it == user_logs.end() || it->second.size() < (size_t)model_params.min_training_samples) return;
    
    auto& logs = it->second;
    int n = logs.size() - 1;
    if (n < 1) return;
    
    dlib::matrix<double> X(n, 15), Y(n, 1);
    for (int i = 0; i < n; ++i) {
        auto feat = ExtractFeatures(logs, uid, logs[i].medication_id, logs[i].timestamp);
        for (int j = 0; j < 15 && j < (int)feat.size(); ++j) X(i,j) = feat[j];
        Y(i,0) = logs[i+1].dose_taken;
    }
    
    dlib::matrix<double> means(1,15), stds(1,15);
    for (int j = 0; j < 15; ++j) {
        double s = 0, sq = 0;
        for (int i = 0; i < n; ++i) { s += X(i,j); sq += X(i,j)*X(i,j); }
        means(0,j) = s/n;
        stds(0,j) = std::sqrt(std::max(sq/n - means(0,j)*means(0,j), 1e-10));
        for (int i = 0; i < n; ++i) X(i,j) = (X(i,j) - means(0,j)) / stds(0,j);
    }
    
    try {
        double lam = 0.1;
        auto XtX = dlib::trans(X) * X;
        auto I = dlib::identity_matrix<double>(15);
        auto inv = dlib::inv(XtX + lam*I);
        auto w = inv * dlib::trans(X) * Y;
        
        TrainedModel m;
        m.weights = w; m.feature_means = means; m.feature_stds = stds; m.is_trained = true;
        user_models[uid] = m;
    } catch (...) {}
}

PredictionResult MedicineTracker::Impl::PredictWithModel(const std::string& uid,
    const std::string& mid, int64_t ts) const {
    std::lock_guard<std::mutex> lk(model_mutex);
    PredictionResult res;
    
    auto mit = user_models.find(uid);
    if (mit == user_models.end() || !mit->second.is_trained) {
        auto lit = user_logs.find(uid);
        if (lit != user_logs.end() && !lit->second.empty()) {
            auto& logs = lit->second;
            double tot = 0; int cnt = 0;
            for (auto& l : logs) if (l.medication_id == mid && !l.is_return) { tot += l.dose_taken; cnt++; }
            if (cnt > 0) {
                res.predicted_daily_usage = tot/cnt;
                res.predicted_24h_usage = res.predicted_daily_usage;
                res.confidence_score = 0.3;
                if (logs.size() > 1) {
                    double avg = 0;
                    for (size_t i = 1; i < logs.size(); ++i) avg += (logs[i].timestamp - logs[i-1].timestamp);
                    avg /= (logs.size()-1);
                    res.predicted_next_usage_time = ts + (int64_t)avg;
                }
            }
        }
        return res;
    }
    
    auto& m = mit->second;
    auto lit = user_logs.find(uid);
    if (lit == user_logs.end()) return res;
    
    auto feat = ExtractFeatures(lit->second, uid, mid, ts);
    if (feat.size() != 15) return res;
    
    dlib::matrix<double> X(1,15);
    for (size_t i = 0; i < 15; ++i) X(0,i) = (feat[i] - m.feature_means(0,i)) / m.feature_stds(0,i);
    auto pred = X * m.weights;
    
    res.predicted_daily_usage = std::max(0.0, pred(0,0));
    res.predicted_24h_usage = res.predicted_daily_usage;
    res.confidence_score = 0.85;
    if (feat[4] > 0) res.predicted_next_usage_time = ts + (int64_t)(feat[4]*3600);
    return res;
}

bool MedicineTracker::Impl::SaveMedicationsToFile(const std::string& fp) const {
    std::ofstream f(fp);
    if (!f) return false;
    f << "id,name,dose_mg,form,manufacturer,notes,expiration_date\n";
    for (auto& p : medication_db) {
        auto& m = p.second;
        f << m.id << "," << m.name << "," << m.dose_mg << "," << (int)m.form << ","
          << m.manufacturer << "," << m.notes << "," << m.expiration_date << "\n";
    }
    return true;
}

bool MedicineTracker::Impl::SaveLogsToFile(const std::string& fp) const {
    std::ofstream f(fp);
    if (!f) return false;
    f << "timestamp,user_id,medication_id,dose_taken,is_return,notes\n";
    for (auto& l : adherence_logs)
        f << l.timestamp << "," << l.user_id << "," << l.medication_id << ","
          << l.dose_taken << "," << (l.is_return?1:0) << "," << l.notes << "\n";
    return true;
}

bool MedicineTracker::Impl::LoadMedicationsFromFile(const std::string& fp) {
    std::ifstream f(fp);
    if (!f) return false;
    std::string line;
    std::getline(f, line);
    medication_db.clear();
    while (std::getline(f, line)) {
        std::stringstream ss(line);
        Medication m;
        std::string form;
        std::getline(ss, m.id, ',');
        std::getline(ss, m.name, ',');
        ss >> m.dose_mg; ss.ignore();
        std::getline(ss, form, ',');
        m.form = (MedicationForm)std::stoi(form);
        std::getline(ss, m.manufacturer, ',');
        std::getline(ss, m.notes, ',');
        ss >> m.expiration_date;
        medication_db[m.id] = m;
    }
    return true;
}

bool MedicineTracker::Impl::LoadLogsFromFile(const std::string& fp) {
    std::ifstream f(fp);
    if (!f) return false;
    std::string line;
    std::getline(f, line);
    adherence_logs.clear();
    while (std::getline(f, line)) {
        std::stringstream ss(line);
        AdherenceLog l;
        int ret;
        ss >> l.timestamp; ss.ignore();
        std::getline(ss, l.user_id, ',');
        std::getline(ss, l.medication_id, ',');
        ss >> l.dose_taken; ss.ignore();
        ss >> ret; ss.ignore();
        std::getline(ss, l.notes);
        l.is_return = (ret != 0);
        adherence_logs.push_back(l);
    }
    RebuildIndexes();
    return true;
}

// Main class
MedicineTracker::MedicineTracker() : pImpl(new Impl()) {}
MedicineTracker::~MedicineTracker() = default;

void MedicineTracker::LoadConfiguration(std::function<std::string(const std::string&)> cfg) {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    try { pImpl->config["smoothing_alpha"] = cfg("medicine_ai_smoothing_alpha");
          pImpl->model_params.smoothing_alpha = std::stod(pImpl->config["smoothing_alpha"]); }
    catch (...) { pImpl->config["smoothing_alpha"] = "0.3"; pImpl->model_params.smoothing_alpha = 0.3; }
    try { pImpl->config["trend_beta"] = cfg("medicine_ai_trend_beta");
          pImpl->model_params.trend_beta = std::stod(pImpl->config["trend_beta"]); }
    catch (...) { pImpl->config["trend_beta"] = "0.1"; pImpl->model_params.trend_beta = 0.1; }
    try { pImpl->config["min_training_samples"] = cfg("medicine_ai_min_samples");
          pImpl->model_params.min_training_samples = std::stoi(pImpl->config["min_training_samples"]); }
    catch (...) { pImpl->config["min_training_samples"] = "10"; pImpl->model_params.min_training_samples = 10; }
    try { pImpl->config["data_storage_path"] = cfg("medicine_ai_storage_path"); }
    catch (...) { pImpl->config["data_storage_path"] = "./medicine_data/"; }
}

void MedicineTracker::SetConfigValue(const std::string& k, const std::string& v) {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    pImpl->config[k] = v;
    if (k == "smoothing_alpha") pImpl->model_params.smoothing_alpha = std::stod(v);
    else if (k == "trend_beta") pImpl->model_params.trend_beta = std::stod(v);
    else if (k == "min_training_samples") pImpl->model_params.min_training_samples = std::stoi(v);
}

bool MedicineTracker::AddMedication(const Medication& m) {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    if (pImpl->medication_db.find(m.id) != pImpl->medication_db.end()) return false;
    pImpl->medication_db[m.id] = m;
    return true;
}

bool MedicineTracker::UpdateMedication(const std::string& id, const Medication& m) {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    if (pImpl->medication_db.find(id) == pImpl->medication_db.end()) return false;
    pImpl->medication_db[id] = m;
    return true;
}

Medication MedicineTracker::GetMedication(const std::string& id) const {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    auto it = pImpl->medication_db.find(id);
    return (it != pImpl->medication_db.end()) ? it->second : Medication();
}

std::vector<Medication> MedicineTracker::GetAllMedications() const {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    std::vector<Medication> res;
    for (auto& p : pImpl->medication_db) res.push_back(p.second);
    return res;
}

bool MedicineTracker::RemoveMedication(const std::string& id) {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    return pImpl->medication_db.erase(id) > 0;
}

void MedicineTracker::LogPickup(const std::string& uid, const std::string& mid, double amt, int64_t ts) {
    AdherenceLog l; l.user_id = uid; l.medication_id = mid; l.dose_taken = amt; l.timestamp = ts; l.is_return = false;
    LogAdherence(l);
}

void MedicineTracker::LogReturn(const std::string& uid, const std::string& mid, double amt, int64_t ts) {
    AdherenceLog l; l.user_id = uid; l.medication_id = mid; l.dose_taken = amt; l.timestamp = ts; l.is_return = true;
    LogAdherence(l);
}

void MedicineTracker::LogAdherence(const AdherenceLog& l) {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    pImpl->adherence_logs.push_back(l);
    pImpl->user_logs[l.user_id].push_back(l);
    pImpl->med_logs[l.medication_id].push_back(l);
    pImpl->prediction_cache.erase(l.user_id + ":" + l.medication_id);
    
    std::string path = pImpl->config["data_storage_path"] + "adherence_log.csv";
    std::ofstream f(path, std::ios::app);
    if (f) f << l.timestamp << "," << l.user_id << "," << l.medication_id << ","
             << l.dose_taken << "," << (l.is_return?1:0) << "," << l.notes << "\n";
}

std::vector<AdherenceLog> MedicineTracker::GetUserAdherenceHistory(const std::string& uid) const {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    auto it = pImpl->user_logs.find(uid);
    return (it != pImpl->user_logs.end()) ? it->second : std::vector<AdherenceLog>();
}

std::vector<AdherenceLog> MedicineTracker::GetMedicationAdherenceHistory(const std::string& mid) const {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    auto it = pImpl->med_logs.find(mid);
    return (it != pImpl->med_logs.end()) ? it->second : std::vector<AdherenceLog>();
}

UserStatistics MedicineTracker::GetUserStatistics(const std::string& uid) const {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    UserStatistics s;
    auto it = pImpl->user_logs.find(uid);
    if (it == pImpl->user_logs.end() || it->second.empty()) return s;
    
    auto& logs = it->second;
    std::vector<double> doses;
    for (auto& l : logs) {
        if (!l.is_return) { doses.push_back(l.dose_taken); s.total_consumed += l.dose_taken; }
        else s.total_returns++;
        s.total_events++;
    }
    
    if (!doses.empty()) {
        s.mean_usage = std::accumulate(doses.begin(), doses.end(), 0.0) / doses.size();
        double var = 0;
        for (double d : doses) { double diff = d - s.mean_usage; var += diff*diff; }
        s.std_dev = std::sqrt(var / doses.size());
        s.min_usage = *std::min_element(doses.begin(), doses.end());
        s.max_usage = *std::max_element(doses.begin(), doses.end());
        s.median_usage = pImpl->CalculateMedian(doses);
    }
    
    s.adherence_rate = (s.total_events > 0) ? (double)(s.total_events - s.total_returns) / s.total_events : 0.0;
    return s;
}

SystemStatistics MedicineTracker::GetSystemStatistics() const {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    SystemStatistics s;
    std::vector<double> all_doses;
    std::set<std::string> users, meds;
    
    for (auto& l : pImpl->adherence_logs) {
        if (!l.is_return) { all_doses.push_back(l.dose_taken); s.total_consumed += l.dose_taken; }
        else s.total_returns++;
        s.total_events++;
        users.insert(l.user_id);
        meds.insert(l.medication_id);
    }
    
    if (!all_doses.empty()) {
        s.total_mean = std::accumulate(all_doses.begin(), all_doses.end(), 0.0) / all_doses.size();
        double var = 0;
        for (double d : all_doses) { double diff = d - s.total_mean; var += diff*diff; }
        s.total_std_dev = std::sqrt(var / all_doses.size());
    }
    
    s.active_users = users.size();
    s.active_medications = meds.size();
    return s;
}

double MedicineTracker::CalculateAdherenceRate(const std::string& uid, const std::string& mid, int days) const {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    auto it = pImpl->user_logs.find(uid);
    if (it == pImpl->user_logs.end()) return 0.0;
    
    int64_t cutoff = std::time(nullptr) - days*86400;
    int taken = 0, total = 0;
    for (auto& l : it->second) {
        if (l.medication_id == mid && l.timestamp >= cutoff) {
            total++;
            if (!l.is_return) taken++;
        }
    }
    return (total > 0) ? (double)taken / total : 0.0;
}

PredictionResult MedicineTracker::PredictUsage(const std::string& uid, const std::string& mid, int64_t ts) const {
    std::string key = uid + ":" + mid;
    auto it = pImpl->prediction_cache.find(key);
    if (it != pImpl->prediction_cache.end() && (ts - it->second.timestamp < 3600)) return it->second.result;
    
    PredictionResult res = pImpl->PredictWithModel(uid, mid, ts);
    pImpl->prediction_cache[key] = {res, ts};
    return res;
}

ResupplyReport MedicineTracker::GenerateResupplyReport(int64_t target, int days) const {
    std::lock_guard<std::mutex> lk(pImpl->data_mutex);
    ResupplyReport rep;
    rep.report_timestamp = std::time(nullptr);
    rep.target_shipment_date = target;
    
    std::map<std::string, double> med_usage;
    for (auto& p : pImpl->med_logs) {
        double tot = 0;
        for (auto& l : p.second) if (!l.is_return) tot += l.dose_taken;
        if (!p.second.empty()) {
            int64_t span = p.second.back().timestamp - p.second.front().timestamp;
            if (span > 0) med_usage[p.first] = (tot / (span / 86400.0)) * days;
        }
    }
    
    for (auto& p : med_usage) {
        ResupplyRecommendation r;
        r.medication_id = p.first;
        auto mit = pImpl->medication_db.find(p.first);
        r.medication_name = (mit != pImpl->medication_db.end()) ? mit->second.name : "Unknown";
        r.recommended_amount = p.second * 1.2; // 20% safety margin
        r.confidence = 0.75;
        r.estimated_depletion = rep.report_timestamp + (int64_t)(days * 86400);
        r.reasoning = "Based on historical usage with 20% safety margin";
        rep.recommendations.push_back(r);
        rep.total_mass_kg += r.recommended_amount * 0.001; // rough estimate
    }
    
    rep.summary = "Generated " + std::to_string(rep.recommendations.size()) + " recommendations for " +
                  std::to_string(days) + "-day mission";
    return rep;
}

void MedicineTracker::TrainModels(bool force) {
    int64_t now = std::time(nullptr);
    if (!force && (now - pImpl->model_params.last_training_time < pImpl->model_params.training_interval_hours * 3600))
        return;
    
    for (auto& p : pImpl->user_logs) pImpl->TrainUserModel(p.first);
    pImpl->model_params.last_training_time = now;
}

bool MedicineTracker::SaveToStorage(const std::string& base) {
    return pImpl->SaveMedicationsToFile(base + "/medications.csv") &&
           pImpl->SaveLogsToFile(base + "/adherence_logs.csv");
}

bool MedicineTracker::LoadFromStorage(const std::string& base) {
    return pImpl->LoadMedicationsFromFile(base + "/medications.csv") &&
           pImpl->LoadLogsFromFile(base + "/adherence_logs.csv");
}

bool MedicineTracker::ExportToCSV(const std::string& path, int64_t start, int64_t end) const {
    std::ofstream f(path);
    if (!f) return false;
    f << "timestamp,user_id,medication_id,medication_name,dose_taken,is_return,notes\n";
    for (auto& l : pImpl->adherence_logs) {
        if ((start == 0 || l.timestamp >= start) && (end == 0 || l.timestamp <= end)) {
            auto mit = pImpl->medication_db.find(l.medication_id);
            std::string name = (mit != pImpl->medication_db.end()) ? mit->second.name : "Unknown";
            f << l.timestamp << "," << l.user_id << "," << l.medication_id << "," << name << ","
              << l.dose_taken << "," << (l.is_return?1:0) << "," << l.notes << "\n";
        }
    }
    return true;
}

} // namespace MedicineAI

// C API
extern "C" {

void* MedicineAI_CreateTracker() {
    return new MedicineAI::MedicineTracker();
}

void MedicineAI_DestroyTracker(void* t) {
    delete static_cast<MedicineAI::MedicineTracker*>(t);
}

void MedicineAI_LogEvent(void* t, MedicineAI::ExternalDataPacket pkt) {
    auto* tracker = static_cast<MedicineAI::MedicineTracker*>(t);
    if (pkt.is_pickup) tracker->LogPickup(pkt.user_id, pkt.medication_id, pkt.amount, pkt.timestamp);
    else tracker->LogReturn(pkt.user_id, pkt.medication_id, pkt.amount, pkt.timestamp);
}

MedicineAI::PredictionResult MedicineAI_GetPrediction(void* t, const char* uid, const char* mid, int64_t ts) {
    auto* tracker = static_cast<MedicineAI::MedicineTracker*>(t);
    return tracker->PredictUsage(uid, mid, ts);
}

bool MedicineAI_GenerateResupply(void* t, int64_t target, int days, MedicineAI::ResupplyReport* out) {
    auto* tracker = static_cast<MedicineAI::MedicineTracker*>(t);
    if (!out) return false;
    *out = tracker->GenerateResupplyReport(target, days);
    return true;
}

} // extern "C"