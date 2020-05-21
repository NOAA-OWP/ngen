#include "GiuhJsonReader.h"

using namespace giuh;

std::shared_ptr<giuh_kernel> GiuhJsonReader::build_giuh_kernel(std::string catchment_id, std::string comid,
        ptree catchment_data_node) {
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

    return std::make_shared<giuh_kernel>(giuh_kernel(catchment_id, comid, cdf_times, cumulative_freqs));
}

std::unique_ptr<ptree> GiuhJsonReader::find_data_node_for_comid(std::string comid) {
    // Traverse tree until finding node with correct id
    for (ptree::iterator pos = data_json_tree->begin(); pos != data_json_tree->end(); ++pos) {
        // TODO: confirm this logic is correct, with respect to iterator members and what they retrieve
        // When the right node is found, use it to build a referenced kernel, and return the reference
        auto first = pos->first;
        if (pos->first == comid) {
            return std::make_unique<ptree>(pos->second);
        }
    }
    return nullptr;
}

std::string GiuhJsonReader::get_associated_comid(std::string catchment_id) {
    return id_map != nullptr && id_map->count(catchment_id) == 1 ? id_map->at(catchment_id) : "";
}

std::shared_ptr<giuh_kernel> GiuhJsonReader::get_giuh_kernel_for_id(std::string catchment_id) {
    if (!is_data_json_file_readable()) {
        return nullptr;
    }
    std::string comid = get_associated_comid(catchment_id);
    if (comid == "") {
        return nullptr;
    }
    // Traverse tree until finding node with correct id
    std::unique_ptr<ptree> data_node = find_data_node_for_comid(comid);
    return data_node == nullptr ? nullptr : build_giuh_kernel(catchment_id, comid, *data_node);
}

bool GiuhJsonReader::is_giuh_kernel_for_id_exists(std::string catchment_id) {
    if (!is_data_json_file_readable() || !is_id_map_json_file_readable()) {
        return false;
    }
    std::string comid = get_associated_comid(catchment_id);
    if (comid == "") {
        return false;
    }
    // Traverse tree until finding node with correct id
    for (ptree::iterator pos = data_json_tree->begin(); pos != data_json_tree->end(); ++pos) {
        // TODO: confirm this logic is correct, with respect to iterator members and what they retrieve
        // When the right node is found, use it to build a referenced kernel, and return the reference
        if (pos->first == comid) {
            return true;
        }
    }
    return false;
}

bool GiuhJsonReader::is_data_json_file_readable() {
    return data_json_file_readable;
}

bool GiuhJsonReader::is_id_map_json_file_readable() {
    return id_map_json_file_readable;
}
