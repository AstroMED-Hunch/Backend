// Complete C-compatible external API
#include "MedicineAI.hpp"
#include <cstring>

extern "C" {

// tracker lifecycle

void* MedicineAI_CreateTracker() {
    return new MedicineAI::MedicineTracker();
}

void MedicineAI_DestroyTracker(void* tracker) {
    delete static_cast<MedicineAI::MedicineTracker*>(tracker);
}

// med database operation

bool MedicineAI_AddMedication(void* tracker, const char* id, const char* name,
                              double dose_mg, int form, const char* manufacturer) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    
    MedicineAI::Medication med;
    med.id = id;
    med.name = name;
    med.dose_mg = dose_mg;
    med.form = static_cast<MedicineAI::MedicationForm>(form);
    med.manufacturer = manufacturer;
    med.expiration_date = 0; // Will be set by caller if needed
    
    return t->AddMedication(med);
}

bool MedicineAI_GetMedication(void* tracker, const char* medication_id,
                              char* name_out, int name_len,
                              double* dose_out, int* form_out) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    
    MedicineAI::Medication med = t->GetMedication(medication_id);
    
    if (med.id.empty()) return false;
    
    if (name_out && name_len > 0) {
        strncpy(name_out, med.name.c_str(), name_len - 1);
        name_out[name_len - 1] = '\0';
    }
    
    if (dose_out) *dose_out = med.dose_mg;
    if (form_out) *form_out = static_cast<int>(med.form);
    
    return true;
}

int MedicineAI_GetMedicationCount(void* tracker) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    return static_cast<int>(t->GetAllMedications().size());
}

bool MedicineAI_RemoveMedication(void* tracker, const char* medication_id) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    return t->RemoveMedication(medication_id);
}

// event logger

void MedicineAI_LogPickup(void* tracker, const char* user_id,
                          const char* medication_id, double amount,
                          int64_t timestamp) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    t->LogPickup(user_id, medication_id, amount, timestamp);
}

void MedicineAI_LogReturn(void* tracker, const char* user_id,
                          const char* medication_id, double amount,
                          int64_t timestamp) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    t->LogReturn(user_id, medication_id, amount, timestamp);
}

void MedicineAI_LogEvent(void* tracker, MedicineAI::ExternalDataPacket packet) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    
    if (packet.is_pickup) {
        t->LogPickup(packet.user_id, packet.medication_id, 
                    packet.amount, packet.timestamp);
    } else {
        t->LogReturn(packet.user_id, packet.medication_id,
                    packet.amount, packet.timestamp);
    }
}


// statistics

void MedicineAI_GetUserStatistics(void* tracker, const char* user_id,
                                  double* mean_usage, double* std_dev,
                                  double* total_consumed, int* total_events,
                                  double* adherence_rate) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    
    MedicineAI::UserStatistics stats = t->GetUserStatistics(user_id);
    
    if (mean_usage) *mean_usage = stats.mean_usage;
    if (std_dev) *std_dev = stats.std_dev;
    if (total_consumed) *total_consumed = stats.total_consumed;
    if (total_events) *total_events = stats.total_events;
    if (adherence_rate) *adherence_rate = stats.adherence_rate;
}

void MedicineAI_GetSystemStatistics(void* tracker,
                                    double* total_mean, double* total_std_dev,
                                    double* total_consumed, int* total_events,
                                    int* active_users, int* active_medications) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    
    MedicineAI::SystemStatistics stats = t->GetSystemStatistics();
    
    if (total_mean) *total_mean = stats.total_mean;
    if (total_std_dev) *total_std_dev = stats.total_std_dev;
    if (total_consumed) *total_consumed = stats.total_consumed;
    if (total_events) *total_events = stats.total_events;
    if (active_users) *active_users = stats.active_users;
    if (active_medications) *active_medications = stats.active_medications;
}

double MedicineAI_CalculateAdherenceRate(void* tracker, const char* user_id,
                                         const char* medication_id, int days_back) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    return t->CalculateAdherenceRate(user_id, medication_id, days_back);
}

// predictions

MedicineAI::PredictionResult MedicineAI_GetPrediction(void* tracker,
                                                       const char* user_id,
                                                       const char* medication_id,
                                                       int64_t timestamp) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    return t->PredictUsage(user_id, medication_id, timestamp);
}

void MedicineAI_PredictUsage(void* tracker, const char* user_id,
                             const char* medication_id, int64_t timestamp,
                             double* predicted_daily_usage,
                             int64_t* predicted_next_usage_time,
                             double* predicted_24h_usage,
                             double* confidence_score) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    
    MedicineAI::PredictionResult result = t->PredictUsage(user_id, medication_id, timestamp);
    
    if (predicted_daily_usage) *predicted_daily_usage = result.predicted_daily_usage;
    if (predicted_next_usage_time) *predicted_next_usage_time = result.predicted_next_usage_time;
    if (predicted_24h_usage) *predicted_24h_usage = result.predicted_24h_usage;
    if (confidence_score) *confidence_score = result.confidence_score;
}

// resupply reports

bool MedicineAI_GenerateResupply(void* tracker, int64_t target_date,
                                 int mission_days,
                                 MedicineAI::ResupplyReport* out_report) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    
    if (!out_report) return false;
    
    *out_report = t->GenerateResupplyReport(target_date, mission_days);
    return true;
}

int MedicineAI_GetResupplyRecommendationCount(void* tracker,
                                              int64_t target_date,
                                              int mission_days) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    
    MedicineAI::ResupplyReport report = t->GenerateResupplyReport(target_date, mission_days);
    return static_cast<int>(report.recommendations.size());
}

// model training

void MedicineAI_TrainModels(void* tracker, bool force_retrain) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    t->TrainModels(force_retrain);
}


// data persistence

bool MedicineAI_SaveToStorage(void* tracker, const char* base_path) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    return t->SaveToStorage(base_path);
}

bool MedicineAI_LoadFromStorage(void* tracker, const char* base_path) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    return t->LoadFromStorage(base_path);
}

bool MedicineAI_ExportToCSV(void* tracker, const char* output_path,
                            int64_t start_date, int64_t end_date) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    return t->ExportToCSV(output_path, start_date, end_date);
}

// configuration

void MedicineAI_SetConfigValue(void* tracker, const char* key, const char* value) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    t->SetConfigValue(key, value);
}

// adherence history

int MedicineAI_GetUserAdherenceHistoryCount(void* tracker, const char* user_id) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    return static_cast<int>(t->GetUserAdherenceHistory(user_id).size());
}

int MedicineAI_GetMedicationAdherenceHistoryCount(void* tracker,
                                                  const char* medication_id) {
    auto* t = static_cast<MedicineAI::MedicineTracker*>(tracker);
    return static_cast<int>(t->GetMedicationAdherenceHistory(medication_id).size());
}

} // extern "C"