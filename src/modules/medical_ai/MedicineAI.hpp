#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <functional>
#include <dlib/matrix.h>


namespace MedicineAI {

// medication database structures

/**
 * Represents the form factor of a medication
 */
enum class MedicationForm {
    TABLET,
    CAPSULE,
    LIQUID,
    INJECTION,
    TOPICAL,
    INHALER,
    OTHER
};

/**
 * core medication information stored in the system database
 */
struct Medication {
    std::string name;              // Medication name (for example:  "Ibuprofen")
    std::string id;                // Unique identifier
    double dose_mg;                // Dosage in milligrams
    MedicationForm form;           // Form factor (tablet, capsule, whatever)
    std::string manufacturer;      // Manufacturer name
    std::string notes;             // add any extra notes or warnings
    int64_t expiration_date;       // unix timestamp of expiration
    
    Medication() : dose_mg(0.0), form(MedicationForm::TABLET), expiration_date(0) {}
};


// adherence logging struct

/**
 * Records a single medication adherence event
 */
struct AdherenceLog {
    std::string user_id;           // Who took the medication
    std::string medication_id;     // What medication was taken
    double dose_taken;             // Actual dose taken (in mg)
    int64_t timestamp;             // When it was taken (Unix timestamp)
    std::string notes;             // Optional notes about the event
    bool is_return;                // True if medication was returned (not taken)
    
    AdherenceLog() : dose_taken(0.0), timestamp(0), is_return(false) {}
};


// prediction and analytics structure

/**
 * Result of usage prediction for a single user and medication
  *
 */
struct PredictionResult {
    double predicted_daily_usage;        // Predicted pills/doses per day
    int64_t predicted_next_usage_time;   // Unix timestamp of next expected usage
    double predicted_24h_usage;          // Total usage in next 24 hours
    double confidence_score;             // Prediction confidence (0.0 to 1.0)
    
    PredictionResult() : predicted_daily_usage(0.0), predicted_next_usage_time(0),
                         predicted_24h_usage(0.0), confidence_score(0.0) {}
};

/**
 * Statistical analysis for a single user's medication usage
   *
 */
struct UserStatistics {
    double mean_usage;             // Average usage per event
    double std_dev;                // Standard deviation of usage
    double min_usage;              // Minimum single usage
    double max_usage;              // Maximum single usage
    double median_usage;           // Median usage
    double total_consumed;         // Total amount consumed
    int total_events;              // Total number of usage events
    int total_returns;             // Number of return events
    double adherence_rate;         // Percentage of scheduled doses taken
    
    UserStatistics() : mean_usage(0.0), std_dev(0.0), min_usage(0.0),
                       max_usage(0.0), median_usage(0.0), total_consumed(0.0),
                       total_events(0), total_returns(0), adherence_rate(0.0) {}
};

/**
 * sys wide statistics across all users and medications
 */
struct SystemStatistics {
    double total_mean;             // Mean usage across all events
    double total_std_dev;          // Standard deviation across all events
    double total_consumed;         // Total medication consumed system-wide
    int total_events;              // Total usage events
    int total_returns;             // Total return events
    int active_users;              // Number of active users
    int active_medications;        // Number of medications in use
    
    SystemStatistics() : total_mean(0.0), total_std_dev(0.0), total_consumed(0.0),
                         total_events(0), total_returns(0), active_users(0),
                         active_medications(0) {}
};

/**
 * resupply recommendation for next ISS shipment
 */
struct ResupplyRecommendation {
    std::string medication_id;     // Which medication to resupply
    std::string medication_name;   // Human-readable name
    double recommended_amount;     // Recommended quantity (in doses/pills)
    double confidence;             // Confidence in recommendation (0.0 to 1.0)
    int64_t estimated_depletion;   // When current supply will run out
    std::string reasoning;         // Explanation of the recommendation
    
    ResupplyRecommendation() : recommended_amount(0.0), confidence(0.0),
                               estimated_depletion(0) {}
};

/**
 * Complete resupply report for ISS shipment planning
 */
struct ResupplyReport {
    std::vector<ResupplyRecommendation> recommendations;
    int64_t report_timestamp;           // When this report was generated
    int64_t target_shipment_date;       // Target date for next resupply
    double total_mass_kg;               // Total estimated mass of shipment
    std::string summary;                // Executive summary
    
    ResupplyReport() : report_timestamp(0), target_shipment_date(0), total_mass_kg(0.0) {}
};

// data transfer struct (for external integration)

/**
 * right here i put a more simple structure for transferring data to/from external systems
 * This avoids exposing internal implementation details
 */
struct ExternalDataPacket {
    std::string user_id;
    std::string medication_id;
    int64_t timestamp;
    double amount;
    bool is_pickup;  // true = pickup, false = return
    
    ExternalDataPacket() : timestamp(0), amount(0.0), is_pickup(true) {}
};

// here's our primary tracker class

/**
 * Main medicine tracking and prediction system
 * This class handles all medication tracking, AI predictions, and analytics
 */
class MedicineTracker {
public:
    MedicineTracker();
    ~MedicineTracker();
    
    // Prevent copying (internal state is complex)
    MedicineTracker(const MedicineTracker&) = delete;
    MedicineTracker& operator=(const MedicineTracker&) = delete;
    
    //  Configuration Management
    
    /**
     * Load configuration from the main application's layout system
     * @param config_getter Function that retrieves config values by key
     */
    void LoadConfiguration(std::function<std::string(const std::string&)> config_getter);
    
    /**
     * Set a specific configuration parameter
     * @param key Configuration key
     * @param value Configuration value
     */
    void SetConfigValue(const std::string& key, const std::string& value);
    
    /**
     * Get a configuration value
     * @param key Configuration key
     * @return Configuration value or empty string if not found
     */
    std::string GetConfigValue(const std::string& key) const;
    
    // Medication Database Management
    
    /**
     * Add a new medication to the database
     * @param med Medication details
     * @return true if successful, false if medication already exists
     */
    bool AddMedication(const Medication& med);
    
    /**
     * Update an existing medication's information
     * @param medication_id ID of medication to update
     * @param med New medication details
     * @return true if successful, false if medication not found
     */
    bool UpdateMedication(const std::string& medication_id, const Medication& med);
    
    /**
     * Retrieve medication details by ID
     * @param medication_id ID of medication
     * @return Medication details, or empty Medication if not found
     */
    Medication GetMedication(const std::string& medication_id) const;
    
    /**
     * Get all medications in the database
     * @return Vector of all medications
     */
    std::vector<Medication> GetAllMedications() const;
    
    /**
     * Remove a medication from the database
     * @param medication_id ID of medication to remove
     * @return true if successful
     */
    bool RemoveMedication(const std::string& medication_id);
    
    /**
     * Search medications by name (case-insensitive partial match)
     * @param name_query Search query
     * @return Vector of matching medications
     */
    std::vector<Medication> SearchMedications(const std::string& name_query) const;
    
    //Event Logging (Adherence Tracking)
    
    /**
     * Log a medication pickup event
     * @param user_id User who picked up medication
     * @param medication_id Which medication was picked up
     * @param amount Amount picked up (number of doses)
     * @param timestamp When the pickup occurred (Unix timestamp)
     */
    void LogPickup(const std::string& user_id, const std::string& medication_id,
                   double amount, int64_t timestamp);
    
    /**
     * Log a medication return event (not consumed)
     * @param user_id User who returned medication
     * @param medication_id Which medication was returned
     * @param amount Amount returned
     * @param timestamp When the return occurred
     */
    void LogReturn(const std::string& user_id, const std::string& medication_id,
                   double amount, int64_t timestamp);
    
    /**
     * Log a complete adherence event (full details)
     * @param log Complete adherence log entry
     */
    void LogAdherence(const AdherenceLog& log);
    
    /**
     * Get adherence history for a specific user
     * @param user_id User to query
     * @return Vector of all adherence logs for that user
     */
    std::vector<AdherenceLog> GetUserAdherenceHistory(const std::string& user_id) const;
    
    /**
     * Get adherence history for a specific medication
     * @param medication_id Medication to query
     * @return Vector of all adherence logs for that medication
     */
    std::vector<AdherenceLog> GetMedicationAdherenceHistory(const std::string& medication_id) const;
    
    /**
     * Get all adherence logs within a date range
     * @param start_date Start timestamp (0 = beginning of time)
     * @param end_date End timestamp (0 = now)
     * @return Vector of adherence logs in range
     */
    std::vector<AdherenceLog> GetAdherenceHistoryInRange(int64_t start_date, int64_t end_date) const;
    
    //Statistics and Analytics
    
    /**
     * Get statistical analysis for a specific user
     * @param user_id User to analyze
     * @return Statistical summary
     */
    UserStatistics GetUserStatistics(const std::string& user_id) const;
    
    /**
     * Get system-wide statistics
     * @return System-wide statistical summary
     */
    SystemStatistics GetSystemStatistics() const;
    
    /**
     * Calculate adherence rate for a user and medication
     * @param user_id User to check
     * @param medication_id Medication to check
     * @param days_back Number of days to look back
     * @return Adherence rate (0.0 to 1.0)
     */
    double CalculateAdherenceRate(const std::string& user_id, 
                                  const std::string& medication_id,
                                  int days_back = 30) const;
    
    /**
     * Get list of all active users (users with at least one log entry)
     * @return Vector of user IDs
     */
    std::vector<std::string> GetActiveUsers() const;
    
    /**
     * Get list of all medications currently being used
     * @return Vector of medication IDs
     */
    std::vector<std::string> GetActiveMedications() const;
    
    //AI Predictions
    
    /**
     * Predict future usage for a specific user and medication
     * @param user_id User to predict for
     * @param medication_id Medication to predict
     * @param current_timestamp Current time (Unix timestamp)
     * @return Prediction results
     */
    PredictionResult PredictUsage(const std::string& user_id,
                                  const std::string& medication_id,
                                  int64_t current_timestamp) const;
    
    /**
     * Generate resupply recommendations for next ISS shipment
     * @param target_shipment_date Target date for shipment (Unix timestamp)
     * @param mission_duration_days How many days the shipment should last
     * @return Complete resupply report with recommendations
     */
    ResupplyReport GenerateResupplyReport(int64_t target_shipment_date,
                                         int mission_duration_days = 180) const;
    
    /**
     * Train or retrain the AI models with current data
     * This should be called periodically to improve predictions
     * @param force_retrain If true, force complete retraining even if recent
     */
    void TrainModels(bool force_retrain = false);
    
    /**
     * Get information about model training status
     * @param user_id User to check
     * @param last_training_time Output: when model was last trained
     * @param num_samples Output: number of samples used in training
     * @return true if model exists for this user
     */
    bool GetModelInfo(const std::string& user_id, 
                     int64_t& last_training_time,
                     int& num_samples) const;
    
    //Data Persistence
    
    /**
     * Save all data to persistent storage (SD card)
     * @param base_path Base directory path for data files
     * @return true if successful
     */
    bool SaveToStorage(const std::string& base_path);
    
    /**
     * Load all data from persistent storage
     * @param base_path Base directory path for data files
     * @return true if successful
     */
    bool LoadFromStorage(const std::string& base_path);
    
    /**
     * Export data in CSV format for external analysis
     * @param output_path Path to output CSV file
     * @param start_date Start date for export (0 = all data)
     * @param end_date End date for export (0 = all data)
     * @return true if successful
     */
    bool ExportToCSV(const std::string& output_path,
                    int64_t start_date = 0,
                    int64_t end_date = 0) const;
    
    /**
     * Import data from CSV file
     * @param input_path Path to input CSV file
     * @param append If true, append to existing data; if false, replace
     * @return true if successful
     */
    bool ImportFromCSV(const std::string& input_path, bool append = true);
    
    /**
     * Clear all adherence logs (medications remain)
     * Use with caution!
     * @return true if successful
     */
    bool ClearAllLogs();
    
    /**
     * Create a backup of all data
     * @param backup_path Path where backup should be created
     * @return true if successful
     */
    bool CreateBackup(const std::string& backup_path) const;
    
    /**
     * Restore from a backup
     * @param backup_path Path to backup file
     * @return true if successful
     */
    bool RestoreFromBackup(const std::string& backup_path);
    
private:
    // Private implementation details hidden from API users (PIMPL pattern)
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace MedicineAI

/**
 * Create a new MedicineTracker instance
 * @return Pointer to tracker instance
 */
void* MedicineAI_CreateTracker();

/**
 * Destroy a MedicineTracker instance
 * @param tracker Tracker to destroy
 */
void MedicineAI_DestroyTracker(void* tracker);

/**
 * Add medication through C API
 * @param tracker Tracker instance
 * @param id Medication ID (null-terminated string)
 * @param name Medication name (null-terminated string)
 * @param dose_mg Dosage in milligrams
 * @param form Form type (0=tablet, 1=capsule, etc.)
 * @param manufacturer Manufacturer name (null-terminated string)
 * @return true if successful
 */
bool MedicineAI_AddMedication(void* tracker, const char* id, const char* name,
                              double dose_mg, int form, const char* manufacturer);

/**
 * Get medication info through C API
 * @param tracker Tracker instance
 * @param medication_id Medication ID (null-terminated string)
 * @param name_out Output buffer for name
 * @param name_len Size of name buffer
 * @param dose_out Output for dose
 * @param form_out Output for form
 * @return true if medication found
 */
bool MedicineAI_GetMedication(void* tracker, const char* medication_id,
                              char* name_out, int name_len,
                              double* dose_out, int* form_out);

/**
 * Get total number of medications
 * @param tracker Tracker instance
 * @return Number of medications
 */
int MedicineAI_GetMedicationCount(void* tracker);

/**
 * Remove medication through C API
 * @param tracker Tracker instance
 * @param medication_id Medication ID to remove
 * @return true if successful
 */
bool MedicineAI_RemoveMedication(void* tracker, const char* medication_id);

/**
 * Log pickup event through C API
 * @param tracker Tracker instance
 * @param user_id User ID (null-terminated string)
 * @param medication_id Medication ID (null-terminated string)
 * @param amount Amount picked up
 * @param timestamp Unix timestamp
 */
void MedicineAI_LogPickup(void* tracker, const char* user_id,
                          const char* medication_id, double amount,
                          int64_t timestamp);

/**
 * Log return event through C API
 * @param tracker Tracker instance
 * @param user_id User ID (null-terminated string)
 * @param medication_id Medication ID (null-terminated string)
 * @param amount Amount returned
 * @param timestamp Unix timestamp
 */
void MedicineAI_LogReturn(void* tracker, const char* user_id,
                          const char* medication_id, double amount,
                          int64_t timestamp);

/**
 * Log event through C API using data packet
 * @param tracker Tracker instance
 * @param packet Data packet with event details
 */
void MedicineAI_LogEvent(void* tracker, MedicineAI::ExternalDataPacket packet);

/**
 * Get user statistics through C API
 * @param tracker Tracker instance
 * @param user_id User ID (null-terminated string)
 * @param mean_usage Output for mean usage
 * @param std_dev Output for standard deviation
 * @param total_consumed Output for total consumed
 * @param total_events Output for total events
 * @param adherence_rate Output for adherence rate
 */
void MedicineAI_GetUserStatistics(void* tracker, const char* user_id,
                                  double* mean_usage, double* std_dev,
                                  double* total_consumed, int* total_events,
                                  double* adherence_rate);

/**
 * Get system statistics through C API
 * @param tracker Tracker instance
 * @param total_mean Output for total mean
 * @param total_std_dev Output for total std dev
 * @param total_consumed Output for total consumed
 * @param total_events Output for total events
 * @param active_users Output for active users count
 * @param active_medications Output for active medications count
 */
void MedicineAI_GetSystemStatistics(void* tracker,
                                    double* total_mean, double* total_std_dev,
                                    double* total_consumed, int* total_events,
                                    int* active_users, int* active_medications);

/**
 * Calculate adherence rate through C API
 * @param tracker Tracker instance
 * @param user_id User ID (null-terminated string)
 * @param medication_id Medication ID (null-terminated string)
 * @param days_back Number of days to look back
 * @return Adherence rate (0.0 to 1.0)
 */
double MedicineAI_CalculateAdherenceRate(void* tracker, const char* user_id,
                                         const char* medication_id, int days_back);

/**
 * Get prediction through C API
 * @param tracker Tracker instance
 * @param user_id User ID (null-terminated string)
 * @param medication_id Medication ID (null-terminated string)
 * @param timestamp Current timestamp
 * @return Prediction result
 */
MedicineAI::PredictionResult MedicineAI_GetPrediction(void* tracker,
                                                      const char* user_id,
                                                      const char* medication_id,
                                                      int64_t timestamp);

/**
 * Predict usage through C API with output parameters
 * @param tracker Tracker instance
 * @param user_id User ID (null-terminated string)
 * @param medication_id Medication ID (null-terminated string)
 * @param timestamp Current timestamp
 * @param predicted_daily_usage Output for daily usage prediction
 * @param predicted_next_usage_time Output for next usage time prediction
 * @param predicted_24h_usage Output for 24h usage prediction
 * @param confidence_score Output for confidence score
 */
void MedicineAI_PredictUsage(void* tracker, const char* user_id,
                             const char* medication_id, int64_t timestamp,
                             double* predicted_daily_usage,
                             int64_t* predicted_next_usage_time,
                             double* predicted_24h_usage,
                             double* confidence_score);

/**
 * Generate resupply report through C API
 * @param tracker Tracker instance
 * @param target_date Target shipment date
 * @param mission_days Mission duration in days
 * @param out_report Output parameter for report
 * @return true if successful
 */
bool MedicineAI_GenerateResupply(void* tracker, int64_t target_date,
                                 int mission_days,
                                 MedicineAI::ResupplyReport* out_report);

/**
 * Get number of resupply recommendations
 * @param tracker Tracker instance
 * @param target_date Target shipment date
 * @param mission_days Mission duration in days
 * @return Number of recommendations
 */
int MedicineAI_GetResupplyRecommendationCount(void* tracker,
                                              int64_t target_date,
                                              int mission_days);

/**
 * Train models through C API
 * @param tracker Tracker instance
 * @param force_retrain Force retraining even if recent
 */
void MedicineAI_TrainModels(void* tracker, bool force_retrain);

/**
 * Save to storage through C API
 * @param tracker Tracker instance
 * @param base_path Base path for storage (null-terminated string)
 * @return true if successful
 */
bool MedicineAI_SaveToStorage(void* tracker, const char* base_path);

/**
 * Load from storage through C API
 * @param tracker Tracker instance
 * @param base_path Base path for storage (null-terminated string)
 * @return true if successful
 */
bool MedicineAI_LoadFromStorage(void* tracker, const char* base_path);

/**
 * Export to CSV through C API
 * @param tracker Tracker instance
 * @param output_path Output CSV path (null-terminated string)
 * @param start_date Start date (0 = all)
 * @param end_date End date (0 = all)
 * @return true if successful
 */
bool MedicineAI_ExportToCSV(void* tracker, const char* output_path,
                            int64_t start_date, int64_t end_date);

/**
 * Set configuration value through C API
 * @param tracker Tracker instance
 * @param key Configuration key (null-terminated string)
 * @param value Configuration value (null-terminated string)
 */
void MedicineAI_SetConfigValue(void* tracker, const char* key, const char* value);

/**
 * Get configuration value through C API
 * @param tracker Tracker instance
 * @param key Configuration key (null-terminated string)
 * @param value_out Output buffer for value
 * @param value_len Size of value buffer
 * @return true if key exists
 */
bool MedicineAI_GetConfigValue(void* tracker, const char* key,
                               char* value_out, int value_len);

/**
 * Get count of user adherence history
 * @param tracker Tracker instance
 * @param user_id User ID (null-terminated string)
 * @return Number of log entries
 */
int MedicineAI_GetUserAdherenceHistoryCount(void* tracker, const char* user_id);

/**
 * Get count of medication adherence history
 * @param tracker Tracker instance
 * @param medication_id Medication ID (null-terminated string)
 * @return Number of log entries
 */
int MedicineAI_GetMedicationAdherenceHistoryCount(void* tracker,
                                                  const char* medication_id);