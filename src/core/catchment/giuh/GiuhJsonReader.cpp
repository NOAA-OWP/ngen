#include "GiuhJsonReader.h"

using namespace giuh;

std::shared_ptr<giuh_kernel> GiuhJsonReader::build_giuh_kernel(std::string catchment_id, ptree catchment_data_node) {
    // Get times and freqs and convert to vectors
    std::vector<double> cumulative_freqs, cdf_times;

    // TODO: account for error condition of unmatching JSON structure for freqs
    for (ptree::value_type freqs : catchment_data_node.get_child("CDF.CumulativeFreq")) {
        cumulative_freqs.push_back(freqs.second.get_value<double>());
    }

    // TODO: account for error condition of unmatching JSON structure for times
    for (ptree::value_type times : catchment_data_node.get_child("CDF.Time")) {
        cdf_times.push_back(times.second.get_value<double>());
    }

    return std::make_shared<giuh_kernel>(giuh_kernel(catchment_id, cdf_times, cumulative_freqs));
}

std::unique_ptr<ptree> GiuhJsonReader::find_data_node_for_catchment_id(std::string catchment_id) {
    // Traverse tree until finding node with correct id
    for (ptree::iterator pos = json_tree->begin(); pos != json_tree->end(); ++pos) {
        // TODO: confirm this logic is correct, with respect to iterator members and what they retrieve
        // When the right node is found, use it to build a referenced kernel, and return the reference
        if (pos->first == catchment_id) {
            return std::make_unique<ptree>(pos->second);
        }
    }
    return nullptr;
}

std::shared_ptr<giuh_kernel> GiuhJsonReader::get_giuh_kernel_for_id(std::string catchment_id) {
    if (!is_json_file_exists() || !is_json_file_readable()) {
        return nullptr;
    }
    // Traverse tree until finding node with correct id
    std::unique_ptr<ptree> data_node = find_data_node_for_catchment_id(catchment_id);
    return data_node == nullptr ? nullptr : build_giuh_kernel(catchment_id, *data_node);
}

bool GiuhJsonReader::is_giuh_kernel_for_id_exists(std::string catchment_id) {
    if (!is_json_file_exists() || !is_json_file_readable()) {
        return false;
    }
    // Traverse tree until finding node with correct id
    for (ptree::iterator pos = json_tree->begin(); pos != json_tree->end(); ++pos) {
        // TODO: confirm this logic is correct, with respect to iterator members and what they retrieve
        // When the right node is found, use it to build a referenced kernel, and return the reference
        if (pos->first == catchment_id) {
            return true;
        }
    }
    return false;
}

bool GiuhJsonReader::is_json_file_exists() {
    return file_exists;
}

bool GiuhJsonReader::is_json_file_readable() {
    return file_readable;
}