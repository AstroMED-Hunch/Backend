// Nimul's integration source that Bridges MedicineAI with Marco's QR/ArUco tracking system
#include "MedicineAI.hpp"
#include "Module.hpp"
#include "Module_Registry.hpp"
#include "mlayout/Parser.hpp"
#include <iostream>
#include <chrono>
#include <map>
#include <string>

/**
 * Medicine AI Module - Integrates with Marco's module system
 * This connects the QR code tracking system to the AI prediction engine
 */
class MedicineAI_Module : public Module {
public:
    MedicineAI_Module() : tracker(nullptr) {}

    virtual ~MedicineAI_Module() {
        if (tracker) delete tracker;
    }

    std::string get_module_name() const override {
        return "MedicineAI";
    }

    /**
     * Initialize the medicine AI system
     * Loads configuration from Marco's layout system
     */
    void initialize() override {
        std::cout << "[MedicineAI] Initializing medicine tracking and AI system..." << std::endl;

        tracker = new MedicineAI::MedicineTracker();

        // Load configuration from Marco's system
        if (layout) {
            tracker->LoadConfiguration([this](const std::string& key) -> std::string {
                try {
                    return this->layout->get_config_value(key);
                } catch (...) {
                    return "";
                }
            });

            std::cout << "[MedicineAI] Configuration loaded from layout system" << std::endl;
        }

        // Load existing data from storage
        try {
            std::string storage_path = "./medicine_data";
            if (layout) {
                try {
                    storage_path = layout->get_config_value("medicine_ai_storage_path");
                } catch (...) {}
            }

            if (tracker->LoadFromStorage(storage_path)) {
                std::cout << "[MedicineAI] Loaded existing data from: " << storage_path << std::endl;
            } else {
                std::cout << "[MedicineAI] No existing data found, starting fresh" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[MedicineAI] Error loading data: " << e.what() << std::endl;
        }

        // Initialize some example medications
        InitializeDefaultMedications();

        // Set up periodic training
        last_training_time = std::chrono::system_clock::now();

        std::cout << "[MedicineAI] Initialization complete!" << std::endl;
    }

    /**
     * Main run loop - processes camera frames and detects medication events
     * Integrates with Marco's ArUco marker detection
     */
    void run(cv::Mat cap) override {
        // Check if it's time to retrain models
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - last_training_time);
        if (elapsed.count() >= 24) {
            std::cout << "[MedicineAI] Running periodic model training..." << std::endl;
            tracker->TrainModels(false);
            last_training_time = now;

            // Save data periodically
            try {
                std::string storage_path = "./medicine_data";
                if (layout) {
                    try {
                        storage_path = layout->get_config_value("medicine_ai_storage_path");
                    } catch (...) {}
                }
                tracker->SaveToStorage(storage_path);
                std::cout << "[MedicineAI] Data saved to storage" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[MedicineAI] Error saving data: " << e.what() << std::endl;
            }
        }

        // Process detected markers from Marco's ArUco system
        // This is where we detect medication pickup/return events
        ProcessMarkerEvents();

        // Display statistics and predictions (optional, for debugging)
        if (show_debug_info) {
            DisplayDebugInfo(cap);
        }
    }

    void shutdown() override {
        std::cout << "[MedicineAI] Shutting down..." << std::endl;

        // Final data save
        try {
            std::string storage_path = "./medicine_data";
            if (layout) {
                try {
                    storage_path = layout->get_config_value("medicine_ai_storage_path");
                } catch (...) {}
            }
            tracker->SaveToStorage(storage_path);
            std::cout << "[MedicineAI] Final data save complete" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[MedicineAI] Error during shutdown: " << e.what() << std::endl;
        }
    }

    /**
     * Public API for external code to log medication events
     * This is the main interface for Marco's system to report events
     */
    void LogMedicationPickup(const std::string& user_id, const std::string& medication_id,
                            double amount, int marker_id) {
        int64_t timestamp = GetCurrentTimestamp();
        tracker->LogPickup(user_id, medication_id, amount, timestamp);

        std::cout << "[MedicineAI] Logged pickup: User=" << user_id
                  << ", Med=" << medication_id
                  << ", Amount=" << amount
                  << ", Marker=" << marker_id << std::endl;

        // Generate and display prediction
        auto prediction = tracker->PredictUsage(user_id, medication_id, timestamp);
        std::cout << "  -> Predicted daily usage: " << prediction.predicted_daily_usage
                  << " (confidence: " << prediction.confidence_score << ")" << std::endl;
    }

    void LogMedicationReturn(const std::string& user_id, const std::string& medication_id,
                            double amount, int marker_id) {
        int64_t timestamp = GetCurrentTimestamp();
        tracker->LogReturn(user_id, medication_id, amount, timestamp);

        std::cout << "[MedicineAI] Logged return: User=" << user_id
                  << ", Med=" << medication_id
                  << ", Amount=" << amount
                  << ", Marker=" << marker_id << std::endl;
    }

    /**
     * Generate ISS resupply report
     */
    void GenerateResupplyReport(int mission_duration_days = 180) {
        int64_t target_date = GetCurrentTimestamp() + (mission_duration_days * 86400);
        auto report = tracker->GenerateResupplyReport(target_date, mission_duration_days);

        std::cout << "\n..." << std::endl;
        std::cout << "ISS RESUPPLY REPORT" << std::endl;
        std::cout << "..." << std::endl;
        std::cout << "Report generated: " << TimeToString(report.report_timestamp) << std::endl;
        std::cout << "Target shipment date: " << TimeToString(report.target_shipment_date) << std::endl;
        std::cout << "Mission duration: " << mission_duration_days << " days" << std::endl;
        std::cout << "Total estimated mass: " << report.total_mass_kg << " kg" << std::endl;
        std::cout << "\nRECOMMENDATIONS:" << std::endl;
        std::cout << "..." << std::endl;

        for (const auto& rec : report.recommendations) {
            std::cout << "\nMedication: " << rec.medication_name << " (ID: " << rec.medication_id << ")" << std::endl;
            std::cout << "  Recommended amount: " << rec.recommended_amount << " doses" << std::endl;
            std::cout << "  Confidence: " << (rec.confidence * 100) << "%" << std::endl;
            std::cout << "  Estimated depletion: " << TimeToString(rec.estimated_depletion) << std::endl;
            std::cout << "  Reasoning: " << rec.reasoning << std::endl;
        }

        std::cout << "\n..." << std::endl;
        std::cout << "Summary: " << report.summary << std::endl;
        std::cout << "...\n" << std::endl;

        // Export to CSV for ISS mission planners
        std::string csv_path = "./medicine_data/resupply_report_" +
                              std::to_string(report.report_timestamp) + ".csv";
        ExportResupplyToCSV(report, csv_path);
        std::cout << "Report exported to: " << csv_path << std::endl;
    }

    /**
     * Get statistics for a specific user
     */
    void DisplayUserStatistics(const std::string& user_id) {
        auto stats = tracker->GetUserStatistics(user_id);

        std::cout << "\n..." << std::endl;
        std::cout << "USER STATISTICS: " << user_id << std::endl;
        std::cout << "..." << std::endl;
        std::cout << "Total events: " << stats.total_events << std::endl;
        std::cout << "Total consumed: " << stats.total_consumed << " doses" << std::endl;
        std::cout << "Mean usage per event: " << stats.mean_usage << std::endl;
        std::cout << "Standard deviation: " << stats.std_dev << std::endl;
        std::cout << "Min usage: " << stats.min_usage << std::endl;
        std::cout << "Max usage: " << stats.max_usage << std::endl;
        std::cout << "Median usage: " << stats.median_usage << std::endl;
        std::cout << "Adherence rate: " << (stats.adherence_rate * 100) << "%" << std::endl;
        std::cout << "Total returns: " << stats.total_returns << std::endl;
        std::cout << "...\n" << std::endl;
    }

    /**
     * Get system-wide statistics
     */
    void DisplaySystemStatistics() {
        auto stats = tracker->GetSystemStatistics();

        std::cout << "\n..." << std::endl;
        std::cout << "SYSTEM-WIDE STATISTICS" << std::endl;
        std::cout << "..." << std::endl;
        std::cout << "Active users: " << stats.active_users << std::endl;
        std::cout << "Active medications: " << stats.active_medications << std::endl;
        std::cout << "Total events: " << stats.total_events << std::endl;
        std::cout << "Total consumed: " << stats.total_consumed << " doses" << std::endl;
        std::cout << "Mean usage: " << stats.total_mean << std::endl;
        std::cout << "Standard deviation: " << stats.total_std_dev << std::endl;
        std::cout << "Total returns: " << stats.total_returns << std::endl;
        std::cout << "...\n" << std::endl;
    }

    /**
     * Add a new medication to the database
     */
    bool AddMedication(const std::string& id, const std::string& name, double dose_mg,
                      MedicineAI::MedicationForm form, const std::string& manufacturer) {
        MedicineAI::Medication med;
        med.id = id;
        med.name = name;
        med.dose_mg = dose_mg;
        med.form = form;
        med.manufacturer = manufacturer;
        med.expiration_date = GetCurrentTimestamp() + (365 * 86400); // 1 year default

        if (tracker->AddMedication(med)) {
            std::cout << "[MedicineAI] Added medication: " << name << " (ID: " << id << ")" << std::endl;
            return true;
        }

        std::cerr << "[MedicineAI] Failed to add medication (already exists?): " << id << std::endl;
        return false;
    }

    /**
     * Get tracker instance for advanced usage
     */
    MedicineAI::MedicineTracker* GetTracker() { return tracker; }

    // Configuration flags
    bool show_debug_info = false;

private:
    MedicineAI::MedicineTracker* tracker;
    std::chrono::system_clock::time_point last_training_time;

    // Track marker states to detect pickup/return events
    std::map<int, bool> marker_last_visible;
    std::map<int, std::string> marker_to_medication;
    std::map<int, std::string> marker_to_user;

    /**
     * Initialize some default medications for the ISS - credits Abdullah for finding me the meds
     */
    void InitializeDefaultMedications() {
        std::cout << "[MedicineAI] Initializing default medication database..." << std::endl;

        // Common ISS medications
        AddMedication("MED001", "Acetaminophen", 500.0, MedicineAI::MedicationForm::TABLET, "Generic");
        AddMedication("MED002", "Ibuprofen", 200.0, MedicineAI::MedicationForm::TABLET, "Generic");
        AddMedication("MED003", "Aspirin", 325.0, MedicineAI::MedicationForm::TABLET, "Bayer");
        AddMedication("MED004", "Promethazine", 25.0, MedicineAI::MedicationForm::TABLET, "Generic");
        AddMedication("MED005", "Scopolamine", 1.5, MedicineAI::MedicationForm::TABLET, "Generic");
        AddMedication("MED006", "Diphenhydramine", 25.0, MedicineAI::MedicationForm::CAPSULE, "Benadryl");
        AddMedication("MED007", "Loperamide", 2.0, MedicineAI::MedicationForm::TABLET, "Imodium");
        AddMedication("MED008", "Pseudoephedrine", 30.0, MedicineAI::MedicationForm::TABLET, "Sudafed");
        AddMedication("MED009", "Zolpidem", 10.0, MedicineAI::MedicationForm::TABLET, "Ambien");
        AddMedication("MED010", "Modafinil", 100.0, MedicineAI::MedicationForm::TABLET, "Provigil");

        std::cout << "[MedicineAI] Default medications initialized" << std::endl;
    }

    /**
     * Process marker detection events from Marco's ArUco system
     * Detects when medications are picked up or returned based on marker visibility
     */
    void ProcessMarkerEvents() {
        if (!layout) return;

        // Iterate through all code groups in Marco's system
        for (auto& code_group : layout->code_groups) {
            for (auto& marker : code_group->markers) {
                int marker_id = marker->get_code_id();
                bool currently_visible = marker->is_visible;

                // Check if visibility state changed
                auto it = marker_last_visible.find(marker_id);
                if (it != marker_last_visible.end()) {
                    bool was_visible = it->second;

                    if (!was_visible && currently_visible) {
                        // Marker just became visible - medication picked up
                        OnMarkerPickedUp(marker_id);
                    } else if (was_visible && !currently_visible) {
                        // Marker just disappeared - medication returned
                        OnMarkerReturned(marker_id);
                    }
                }

                // Update state
                marker_last_visible[marker_id] = currently_visible;
            }
        }
    }

    /**
     * Handle marker pickup event
     */
    void OnMarkerPickedUp(int marker_id) {
        // Map marker to medication and user
        // In a real system, this would use the layout configuration
        std::string user_id = GetUserForMarker(marker_id);
        std::string med_id = GetMedicationForMarker(marker_id);

        if (!user_id.empty() && !med_id.empty()) {
            LogMedicationPickup(user_id, med_id, 1.0, marker_id);
        }
    }

    /**
     * Handle marker return event
     */
    void OnMarkerReturned(int marker_id) {
        std::string user_id = GetUserForMarker(marker_id);
        std::string med_id = GetMedicationForMarker(marker_id);

        if (!user_id.empty() && !med_id.empty()) {
            LogMedicationReturn(user_id, med_id, 1.0, marker_id);
        }
    }

    /**
     * Map marker ID to user (configured in layout file)
     * Format: marker_<ID>_user = "username"
     */
    std::string GetUserForMarker(int marker_id) {
        // Try to get from configuration
        if (layout) {
            try {
                std::string key = "marker_" + std::to_string(marker_id) + "_user";
                std::string user = layout->get_config_value(key);
                if (!user.empty()) {
                    return user;
                }
            } catch (...) {
                // Key not found, continue to default
            }
        }

        // Default mapping if not configured
        // Uses marker ID ranges
        if (marker_id >= 110 && marker_id < 200) return "creamy";
        else if (marker_id >= 200 && marker_id < 300) return "marco";
        else if (marker_id >= 300 && marker_id < 400) return "crew_gamma";
        else return "unknown_user";
    }

    /**
     * Map marker ID to medication (configured in layout file)
     * Format: marker_<ID>_medication = "MED001"
     */
    std::string GetMedicationForMarker(int marker_id) {
        // Try to get from configuration
        if (layout) {
            try {
                std::string key = "marker_" + std::to_string(marker_id) + "_medication";
                std::string med = layout->get_config_value(key);
                if (!med.empty()) {
                    return med;
                }
            } catch (...) {
                // Key not found, continue to default
            }
        }

        // Default mapping based on marker ID ranges
        // Ibuprofen: 110-139
        if (marker_id >= 110 && marker_id < 140) return "MED001";
        // Aspirin: 140-169 or 230-269
        else if ((marker_id >= 140 && marker_id < 170) || (marker_id >= 230 && marker_id < 270)) return "MED002";
        // Acetaminophen: 170-199 or 270-299
        else if ((marker_id >= 170 && marker_id < 200) || (marker_id >= 270 && marker_id < 300)) return "MED003";
        // Diphenhydramine: 300+
        else if (marker_id >= 300) return "MED004";
        else return "MED_UNKNOWN";
    }

    /**
     * Display debug information on the video frame
     */
    void DisplayDebugInfo(cv::Mat& frame) {
        // This would display statistics and predictions on the video feed
        // Implementation depends on specific visualization needs
    }

    /**
     * Get current Unix timestamp
     */
    int64_t GetCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    /**
     * Convert timestamp to human-readable string
     */
    std::string TimeToString(int64_t timestamp) {
        std::time_t t = timestamp;
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        return std::string(buffer);
    }

    /**
     * Export resupply report to CSV
     */
    void ExportResupplyToCSV(const MedicineAI::ResupplyReport& report, const std::string& path) {
        std::ofstream file(path);
        if (!file.is_open()) return;

        file << "medication_id,medication_name,recommended_amount,confidence,estimated_depletion,reasoning\n";
        for (const auto& rec : report.recommendations) {
            file << rec.medication_id << ","
                 << rec.medication_name << ","
                 << rec.recommended_amount << ","
                 << rec.confidence << ","
                 << rec.estimated_depletion << ","
                 << rec.reasoning << "\n";
        }
        file.close();
    }
};

// Register the module with Marco's system
MAKE_MODULE(MedicineAI_Module, "medicine_ai");